/*******************************************************************************
 * Copyright (C) 2017 Cadence Design Systems, Inc.
 * Copyright 2018 NXP
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to use this Software with Cadence processor cores only and
 * not with any other processors and platforms, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/


/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "xf-types.h"
#include "xf-debug.h"
#include "xf-hal.h"
#include "mydefs.h"
#include "dsp_codec_interface.h"
#include "dsp_renderer_interface.h"
#include "xf-audio-apicmd.h"
#include "audio/xa-renderer-api.h"
#include "sai.h"
#include "esai.h"
#include "edma.h"
#include "irqstr.h"
#include "io.h"

/*******************************************************************************
 * Internal functions definitions
 ******************************************************************************/
#define MAX_PERIOD_COUNT  8

#define SAI_INT_NUM       314
#define EDMA_SAI_INT_NUM  315


#define ESAI_INT_NUM       409
#define EDMA_ESAI_INT_NUM  410

/* ...API structure */
struct XADevRenderer
{
	/* Run-time data */

	/* ...number of frames in ring-buffer; always power-of-two */
	u32                     ring_num;

	u32		        ring_size;

	/* ...size of PCM sample (respects channels and PCM width) */
	u32                     sample_size;

	/* ...total number of rendered samples */
	u32                     rendered;

	/* ...total number of submitted samples */
	u32                     submitted;

	/***************************************************************************
	 * Internal stuff
	 **************************************************************************/

	/* ...identifier of the core we are running on */
	u32                     core;

	/* ...component state */
	u32                     state;

	/* ...notification callback pointer */
	xa_renderer_cb_t       *cdata;

	/* ...input buffer pointer */
	void                    *input;

	/* ...number of samples consumed */
	u32                     consumed;

	u32                     avail;

	/***************************************************************************
	 * Renderer configuration data
	 **************************************************************************/

	/* ...period size (in samples - power-of-two) */
	u32                    frame_size;

	/* ...number of channels */
	u32                    channels;

	/* ...current sampling rate */
	u32                    rate;

	/* ...sample width */
	u32                    pcm_width;

	u32                    threshold;
	/* ...private data pointer */
	void                  *private_data;

	void                  *dma_buf;

	void                  *dev_addr;

	void                  *edma_addr;

	void                  *irqstr_addr;

	/* struct nxp_edma_hw_tcd  tcd[MAX_PERIOD_COUNT];*/
	void                  *tcd;

	void                  *tcd_align32;

	void                  (*dev_init)(volatile void * dev_addr, int mode,
					  int channel, int rate, int width, int mclk_rate);
	void                  (*dev_start)(volatile void * dev_addr, int tx);
	void                  (*dev_stop)(volatile void * dev_addr, int tx);
	void                  (*dev_isr)(volatile void * dev_addr);

	u32                   dev_Int;
	u32                   dev_fifo_off;
	u32                   dma_Int;
};

/*******************************************************************************
 * Renderer state flags
 ******************************************************************************/

#define XA_RENDERER_FLAG_PREINIT_DONE   (1 << 0)
#define XA_RENDERER_FLAG_POSTINIT_DONE  (1 << 1)
#define XA_RENDERER_FLAG_RUNNING        (1 << 2)
#define XA_RENDERER_FLAG_PAUSED         (1 << 3)
#define XA_RENDERER_FLAG_COMPLETE       (1 << 4)
#define XA_RENDERER_FLAG_SUSPENDED      (1 << 5)
#define XA_RENDERER_FLAG_IDLE           (1 << 6)
#define XA_RENDERER_FLAG_BUSY           (1 << 7)

/*******************************************************************************
 * Local macros definitions
 ******************************************************************************/

#define xa_hw_renderer_atomic_read(var)         \
	(*((volatile u32 *)&(var)))

#define xa_hw_renderer_atomic_write(var, val)   \
	(*((volatile u32 *)&(var)) = (val))

#define xa_hw_renderer_paused(d)                \
	(xa_hw_renderer_atomic_read((d)->state) & XA_RENDERER_FLAG_PAUSED)

#define xa_hw_renderer_running(d)               \
	(xa_hw_renderer_atomic_read((d)->state) & XA_RENDERER_FLAG_RUNNING)

/*******************************************************************************
 * Local functions
 ******************************************************************************/

/* ...underrun/suspend recovery */
static inline int xa_hw_renderer_recover(struct XADevRenderer *d)
{

}

/* ...start HW-renderer operation */
static inline int xa_hw_renderer_start(struct XADevRenderer *d)
{
	LOG(("HW-renderer started\n"));

	irqstr_start(d->irqstr_addr);
	edma_start(d->edma_addr, d->tcd_align32, 0);
	d->dev_start(d->dev_addr, 1);

	return 0;
}

/* ...pause renderer operation */
static inline void xa_hw_renderer_pause(struct XADevRenderer *d)
{
	/* ...stop updating software write index; that makes it to output silence */
	LOG(("HW-renderer paused\n"));
	irqstr_stop(d->irqstr_addr);
	edma_stop(d->edma_addr);
	d->dev_stop(d->dev_addr, 1);
}

/* ...resume renderer operation */
static inline void xa_hw_renderer_resume(struct XADevRenderer *d)
{
	/* ...rethink it - tbd */
	LOG(("HW-renderer resumed\n"));
	irqstr_start(d->irqstr_addr);
	edma_start(d->edma_addr, d->tcd_align32, 0);
	d->dev_start(d->dev_addr, 1);
}

/* ...close hardware renderer */
static inline void xa_hw_renderer_close(struct XADevRenderer *d)
{
	LOG(("HW-renderer closed\n"));
	irqstr_stop(d->irqstr_addr);
	edma_stop(d->edma_addr);
	d->dev_stop(d->dev_addr, 1);
}

/* ...emulation of renderer interrupt service routine */
static void xa_hw_renderer_isr(struct XADevRenderer *d)
{
	s32     avail;
	u32     status;

	/* period eclapse */
	status = read32(d->irqstr_addr + IRQSTEER_CHnSTATUS(IRQ_TO_MASK_OFFSET(d->dev_Int+32)));

	LOG2("xa_hw_renderer_isr status %x, %d\n", status, d->rendered);

	if (status &  (1 << IRQ_TO_MASK_SHIFT(d->dev_Int+32)))
		d->dev_isr(d->dev_addr);

	if (status &  (1 << IRQ_TO_MASK_SHIFT(d->dma_Int+32))) {
		edma_irq_handler(d->edma_addr);

		d->rendered = d->rendered + d->frame_size;
		/* ...notify user on input-buffer (idx = 0) consumption */
		d->cdata->cb(d->cdata, 0);

		if (d->rendered >= d->submitted) {
			xa_hw_renderer_close(d);
			d->state ^= XA_RENDERER_FLAG_RUNNING | XA_RENDERER_FLAG_IDLE;
		}
	}
}

/*******************************************************************************
 * HW-renderer control functions
 ******************************************************************************/

/* ...initialize hardware renderer */
static inline int xa_hw_renderer_init(struct XADevRenderer *d)
{
	int             r;

	struct dsp_main_struct *dsp_config =
		(struct dsp_main_struct *)d->private_data;

	/* ...make sure ring buffer size is a power-of-two */
	XF_CHK_ERR(xf_is_power_of_two(d->ring_num), -EINVAL);

	/* ...make sure frame size is a power of two */
	XF_CHK_ERR(xf_is_power_of_two(d->frame_size), -EINVAL);

	/* ...calculate sample size in bytes */
	d->sample_size = d->channels * (d->pcm_width == 16 ? 2 : 4);

	/* ...calculate total ring-buffer size in samples */
	d->ring_size = d->frame_size * d->ring_num;

	/* ...reset rendered sample counter */
	d->rendered = d->submitted = 0;

	/* ...mark audio device is in suspended state */
	d->state |= XA_RENDERER_FLAG_IDLE;

	d->avail = d->frame_size * d->ring_num;

	d->threshold = d->frame_size * d->ring_num;

	LOG2(("HW-renderer initialized: frame=%u, num=%u\n"), d->frame_size, d->ring_num);

	/* alloc internal buffer for DMA/SAI/ESAI*/
	d->dma_buf = MEM_scratch_malloc(&dsp_config->scratch_mem_info, d->ring_size * d->sample_size);
	d->tcd = MEM_scratch_malloc(&dsp_config->scratch_mem_info,
				MAX_PERIOD_COUNT * sizeof(struct nxp_edma_hw_tcd) + 32);

	d->tcd_align32 = (void *)(((u32)d->tcd + 31) & ~31);


	/*It is better to send address through the set_param */
	d->dev_addr    =  (void *)ESAI_ADDR;
	d->dev_Int     =  ESAI_INT_NUM;
	d->dev_fifo_off=  REG_ESAI_ETDR;

	d->dma_Int     =   EDMA_ESAI_INT_NUM;
	d->edma_addr   =  (void *)EDMA_ADDR_ESAI_TX;
	d->irqstr_addr =  (void *)IRQSTR_ADDR;

	d->dev_init     = esai_init;
	d->dev_start    = esai_start;
	d->dev_stop     = esai_stop;
	d->dev_isr      = esai_irq_handler;

	/*init hw device*/
	d->dev_init(d->dev_addr, 1, d->channels,  d->rate, d->pcm_width, 24576000);

	edma_init(d->edma_addr, DMA_MEM_TO_DEV,
				d->tcd_align32,
				d->dev_addr + d->dev_fifo_off,
				0,
				d->dma_buf,
				d->frame_size * d->sample_size,
				2);

	irqstr_init(d->irqstr_addr, d->dev_Int, d->dma_Int);

	_xtos_set_interrupt_handler_arg(INT_NUM_IRQSTR_DSP_6, xa_hw_renderer_isr, d);
	_xtos_ints_on((1 << INT_NUM_IRQSTR_DSP_6) | (1 << 7));

	LOG("hw_init finished\n");
	return 0;
}

/* ...copy number of samples into ring-buffer  */
static inline u32 xa_hw_copy_buffer(struct XADevRenderer *d, const void *b, u32 k)
{
	int     rc;
	int     off = (d->submitted % d->ring_size) * d->sample_size;

	memcpy((char *)d->dma_buf + off,  b,  k * d->sample_size);

	return (u32)rc;
}

/* ...submit data (in bytes) into internal renderer ring-buffer */
static inline u32 xa_hw_renderer_submit(struct XADevRenderer *d, void *b, u32 n)
{
	u32     status;
	s32     avail = d->ring_size - (d->submitted - d->rendered);
	s32     k;

	/* ...operate interlocked with timer interrupt (disable all interrupts) */
//	status = xf_isr_disable(d->core);

	/* ...track buffer indices (all in samples) */
	LOG2((">> avail=%x, n=%x\n"), avail, n);

	/* ...determine amount of samples to copy */
	if ((k = (avail < n ? avail : n)) > 0)
	{
		/* ...write samples into alsa device */
		xa_hw_copy_buffer(d, b, k);

		avail = avail - k;
	}
	else
	{
		/* ...no room in ALSA buffer; mark device is busy */
		d->state |= XA_RENDERER_FLAG_BUSY;
	}

	LOG2(("<< avail=%x, k=%x\n"), avail, k);

	d->avail = avail;


	/* ...restore interrupts state */
//	xf_isr_restore(d->core, status);

	/* ...recover ALSA renderer as needed */
	//(void) xa_hw_renderer_recover(d);

	/* ...increment total number of submitted samples (informative only) */
	d->submitted += k;

	LOG4("submit %d, %d, %d, %d\n", n, avail, k, d->submitted);

	if ((d->state & XA_RENDERER_FLAG_IDLE) && ((d->submitted - d->rendered) >= d->threshold))
	{
		/* trigger start*/
		xa_hw_renderer_start(d);

		d->state ^= XA_RENDERER_FLAG_IDLE | XA_RENDERER_FLAG_RUNNING;
	}

	/* ...return number of samples written */
	return k;
}

/* ...state retrieval function */
static inline u32 xa_hw_renderer_get_state(struct XADevRenderer *d)
{
	if (d->state & XA_RENDERER_FLAG_RUNNING)
		return XA_RENDERER_STATE_RUN;
	else if (d->state & XA_RENDERER_FLAG_PAUSED)
		return XA_RENDERER_STATE_PAUSE;
	else
		return XA_RENDERER_STATE_IDLE;
}

/* ...HW-renderer control function */
static inline DSP_ERROR_TYPE xa_hw_renderer_control(struct XADevRenderer *d, u32 state)
{
	switch (state)
	{
	case XA_RENDERER_STATE_RUN:
		/* ...renderer must be in paused state */
		XF_CHK_ERR(d->state & XA_RENDERER_FLAG_PAUSED, XA_PARA_ERROR);

		/* ...resume renderer operation */
		xa_hw_renderer_resume(d);

		/* ...mark renderer is running */
		d->state ^= XA_RENDERER_FLAG_RUNNING | XA_RENDERER_FLAG_PAUSED;

		return XA_SUCCESS;

	case XA_RENDERER_STATE_PAUSE:
		/* ...renderer must be in running state */
		XF_CHK_ERR(d->state & XA_RENDERER_FLAG_RUNNING, XA_PARA_ERROR);

		/* ...pause renderer operation */
		xa_hw_renderer_pause(d);

		/* ...mark renderer is paused */
		d->state ^= XA_RENDERER_FLAG_RUNNING | XA_RENDERER_FLAG_PAUSED;

		return XA_SUCCESS;

	case XA_RENDERER_STATE_IDLE:
		/* ...command is valid in any active state; stop renderer operation */
		xa_hw_renderer_close(d);

		/* ...reset renderer flags */
		d->state &= ~(XA_RENDERER_FLAG_RUNNING | XA_RENDERER_FLAG_PAUSED);

		return XA_SUCCESS;

	default:
		/* ...unrecognized command */
		return XF_CHK_ERR(0, XA_PARA_ERROR);
	}
}

/*******************************************************************************
 * Commands processing
 ******************************************************************************/

/* ...standard codec initialization routine */
static DSP_ERROR_TYPE xa_renderer_get_api_size(struct XADevRenderer *d, u32 i_idx, void* pv_value)
{
	/* ...check parameters are sane */
	XF_CHK_ERR(pv_value, XA_PARA_ERROR);

	/* ...retrieve API structure size */
	*(u32 *)pv_value = sizeof(*d);

	return XA_SUCCESS;
}

static DSP_ERROR_TYPE xf_renderer_preinit(struct XADevRenderer *d, u32 i_idx, void *pv_value)
{
	DSP_ERROR_TYPE ret = XA_SUCCESS;

	/* ...set private data pointer */
	d->private_data = pv_value;

	d->state = XA_RENDERER_FLAG_PREINIT_DONE;

	return ret;
}

/* ...standard codec initialization routine */
static DSP_ERROR_TYPE xa_renderer_init(struct XADevRenderer *d, u32 i_idx, void* pv_value)
{
	/* ...sanity check - pointer must be valid */
	XF_CHK_ERR(d, XA_PARA_ERROR);

	/* ...process particular initialization type */

	d->pcm_width  = 16;
	d->channels   = 1;
	d->rate       = 48000;
	d->frame_size = 2048;  /*1024 * 4 = 4096*/
	d->ring_num   = 2;

	LOG("xa_renderer_init\n");

	return XA_SUCCESS;
}

static DSP_ERROR_TYPE xf_renderer_postinit(struct XADevRenderer *d, u32 i_idx, void *pv_value)
{
	DSP_ERROR_TYPE ret = XA_SUCCESS;

	XF_CHK_ERR(xa_hw_renderer_init(d) == 0, XA_PARA_ERROR);

	return ret;
}

/* ...set renderer configuration parameter */
static DSP_ERROR_TYPE xa_renderer_set_config_param(struct XADevRenderer *d, u32 i_idx, void* pv_value)
{
	u32     i_value;

	/* ...sanity check - pointers must be sane */
	XF_CHK_ERR(d && pv_value, XA_PARA_ERROR);

	/* ...pre-initialization must be completed */
	XF_CHK_ERR(d->state & XA_RENDERER_FLAG_PREINIT_DONE, XA_PARA_ERROR);

	LOG1("xa_renderer_set_config_param, %d\n", i_idx);
	/* ...process individual configuration parameter */
	switch (i_idx)
	{
	case XA_RENDERER_CONFIG_PARAM_PCM_WIDTH:
		/* ...command is valid only in configuration state */
		XF_CHK_ERR((d->state & XA_RENDERER_FLAG_POSTINIT_DONE) == 0, XA_PARA_ERROR);

		/* ...get requested PCM width */
		i_value = (u32) *(u32 *)pv_value;

		/* ...check value is permitted (16 bits only) */
		XF_CHK_ERR(i_value == 16, XA_PARA_ERROR);

		/* ...apply setting */
		d->pcm_width = i_value;

		return XA_SUCCESS;

	case XA_RENDERER_CONFIG_PARAM_CHANNELS:
		/* ...command is valid only in configuration state */
		XF_CHK_ERR((d->state & XA_RENDERER_FLAG_POSTINIT_DONE) == 0, XA_PARA_ERROR);

		/* ...get requested channel number */
		i_value = (u32) *(u32 *)pv_value;

		/* ...allow stereo only */
		XF_CHK_ERR(i_value == 2 || i_value == 1, XA_PARA_ERROR);

		/* ...apply setting */
		d->channels = (u32)i_value;

		return XA_SUCCESS;

	case XA_RENDERER_CONFIG_PARAM_SAMPLE_RATE:
		/* ...command is valid only in configuration state */
		XF_CHK_ERR((d->state & XA_RENDERER_FLAG_POSTINIT_DONE) == 0, XA_PARA_ERROR);

		/* ...get requested sampling rate */
		i_value = (u32) *(u32 *)pv_value;

		/* ...allow 44.1 and 48KHz only - tbd */
		XF_CHK_ERR(i_value == 44100 || i_value == 48000, XA_PARA_ERROR);

		/* ...apply setting */
		d->rate = (u32)i_value;

		return XA_SUCCESS;

	case XA_RENDERER_CONFIG_PARAM_FRAME_SIZE:
		/* ...command is valid only in configuration state */
		XF_CHK_ERR((d->state & XA_RENDERER_FLAG_POSTINIT_DONE) == 0, XA_PARA_ERROR);

		/* ...get requested frame size */
		i_value = (u32) *(u32 *)pv_value;

		/* ...check it is a power-of-two */
		XF_CHK_ERR(xf_is_power_of_two(i_value), XA_PARA_ERROR);

		/* ...apply setting */
		d->frame_size = (u32)i_value;

		return XA_SUCCESS;

	case XA_RENDERER_CONFIG_PARAM_CB:
		/* ...set opaque callback data function */
		d->cdata = (xa_renderer_cb_t *)pv_value;

		return XA_SUCCESS;

	case XA_RENDERER_CONFIG_PARAM_STATE:
		/* ...runtime state control parameter valid only in execution state */
		XF_CHK_ERR(d->state & XA_RENDERER_FLAG_POSTINIT_DONE, XA_PARA_ERROR);

		/* ...get requested state */
		i_value = (u32) *(u32 *)pv_value;

		/* ...pass to state control hook */
		return xa_hw_renderer_control(d, i_value);

	default:
		/* ...unrecognized parameter */
		return XF_CHK_ERR(0, XA_PARA_ERROR);
	}
}

/* ...retrieve configuration parameter */
static DSP_ERROR_TYPE xa_renderer_get_config_param(struct XADevRenderer *d, u32 i_idx, void* pv_value)
{
	/* ...sanity check - renderer must be initialized */
	XF_CHK_ERR(d && pv_value, XA_PARA_ERROR);

	/* ...make sure pre-initialization is completed */
	XF_CHK_ERR(d->state & XA_RENDERER_FLAG_PREINIT_DONE, XA_PARA_ERROR);

	/* ...process individual configuration parameter */
	switch (i_idx)
	{
	case XA_RENDERER_CONFIG_PARAM_PCM_WIDTH:
		/* ...return current PCM width */
		*(u32 *)pv_value = d->pcm_width;
		return XA_SUCCESS;

	case XA_RENDERER_CONFIG_PARAM_CHANNELS:
		/* ...return current channel number */
		*(u32 *)pv_value = d->channels;
		return XA_SUCCESS;

	case XA_RENDERER_CONFIG_PARAM_SAMPLE_RATE:
		/* ...return current sampling rate */
		*(u32 *)pv_value = d->rate;
		return XA_SUCCESS;

	case XA_RENDERER_CONFIG_PARAM_FRAME_SIZE:
		/* ...return current audio frame length (in samples) */
		*(u32 *)pv_value = d->frame_size;
		return XA_SUCCESS;

	case XA_RENDERER_CONFIG_PARAM_STATE:
		/* ...return current execution state */
		*(u32 *)pv_value = xa_hw_renderer_get_state(d);
		return XA_SUCCESS;

	default:
		/* ...unrecognized parameter */
		return XF_CHK_ERR(0, XA_PARA_ERROR);
	}
}

/* ...execution command */
static DSP_ERROR_TYPE xa_renderer_execute(struct XADevRenderer *d, u32 i_idx, void* pv_value)
{

	/* ...sanity check - pointer must be valid */
	XF_CHK_ERR(d, XA_PARA_ERROR);

	/* ...renderer must be in running state */
	XF_CHK_ERR(d->state & (XA_RENDERER_FLAG_RUNNING | XA_RENDERER_FLAG_IDLE), XA_PARA_ERROR);

	LOG("xa_renderer_execute\n");
	/* ...process individual command type */
	return XA_SUCCESS;
}

/* ...set number of input bytes */
static DSP_ERROR_TYPE xa_renderer_set_input_bytes(struct XADevRenderer *d, u32 i_idx, void* pv_value)
{
	u32     size;

	/* ...sanity check - check parameters */
	XF_CHK_ERR(d && pv_value, XA_PARA_ERROR);

	/* ...make sure it is an input port  */
	XF_CHK_ERR(i_idx == 0, XA_PARA_ERROR);

	/* ...renderer must be initialized */
	XF_CHK_ERR(d->state & XA_RENDERER_FLAG_POSTINIT_DONE, XA_PARA_ERROR);

	/* ...input buffer pointer must be valid */
	XF_CHK_ERR(d->input, XA_PARA_ERROR);

	/* ...check buffer size is sane */
	XF_CHK_ERR((size = *(u32 *)pv_value / d->sample_size) > 0, XA_PARA_ERROR);

	/* ...make sure we have integral amount of samples */
	XF_CHK_ERR(size * d->sample_size == *(u32 *)pv_value, XA_PARA_ERROR);

	/* ...submit chunk of data into our ring-buffer */
	d->consumed = d->sample_size * xa_hw_renderer_submit(d, d->input, size);

	/* ...all is correct */
	return XA_SUCCESS;
}

/* ...get number of consumed bytes */
static DSP_ERROR_TYPE xf_renderer_get_consumed_bytes(struct XADevRenderer *d, u32 i_idx, void* pv_value)
{
	/* ...sanity check - check parameters */
	XF_CHK_ERR(d && pv_value, XA_PARA_ERROR);

	/* ...input buffer index must be valid */
	XF_CHK_ERR(i_idx == 0, XA_PARA_ERROR);

	/* ...renderer must be running */
	XF_CHK_ERR(d->state & XA_RENDERER_FLAG_RUNNING, XA_PARA_ERROR);

	/* ...input buffer must exist */
	XF_CHK_ERR(d->input, XA_PARA_ERROR);

	/* ...return number of bytes consumed */
	*(u32 *)pv_value = d->consumed;

	/* ...and reset input buffer index */
	//d->input = NULL;

	return XA_SUCCESS;
}

/*******************************************************************************
 * Memory information API
 ******************************************************************************/

/* ..get total amount of data for memory tables */
static DSP_ERROR_TYPE xa_renderer_get_memtabs_size(struct XADevRenderer *d, u32 i_idx, void* pv_value)
{
	/* ...basic sanity checks */
	XF_CHK_ERR(d && pv_value, XA_PARA_ERROR);

	/* ...check renderer is pre-initialized */
	XF_CHK_ERR(d->state & XA_RENDERER_FLAG_PREINIT_DONE, XA_PARA_ERROR);

	/* ...we have all our tables inside API structure */
	*(u32 *)pv_value = 0;

	return XA_SUCCESS;
}

/* ..set memory tables pointer */
static DSP_ERROR_TYPE xa_renderer_set_memtabs_ptr(struct XADevRenderer *d, u32 i_idx, void* pv_value)
{
	/* ...basic sanity checks */
	XF_CHK_ERR(d && pv_value, XA_PARA_ERROR);

	/* ...check renderer is pre-initialized */
	XF_CHK_ERR(d->state & XA_RENDERER_FLAG_PREINIT_DONE, XA_PARA_ERROR);

	/* ...do not do anything; just return success - tbd */
	return XA_SUCCESS;
}

/* ...return total amount of memory buffers */
static DSP_ERROR_TYPE xa_renderer_get_n_memtabs(struct XADevRenderer *d, u32 i_idx, void* pv_value)
{
	/* ...basic sanity checks */
	XF_CHK_ERR(d && pv_value, XA_PARA_ERROR);

	/* ...we have 1 input buffer */
	*(u32 *)pv_value = 1;

	return XA_SUCCESS;
}

/* ...return memory buffer data */
static DSP_ERROR_TYPE xa_renderer_get_mem_info_size(struct XADevRenderer *d, u32 i_idx, void* pv_value)
{
	u32     i_value;

	/* ...basic sanity check */
	XF_CHK_ERR(d && pv_value, XA_PARA_ERROR);

	/* ...command valid only after post-initialization step */
	XF_CHK_ERR(d->state & XA_RENDERER_FLAG_POSTINIT_DONE, XA_PARA_ERROR);

	switch (i_idx)
	{
	case 0:
		/* ...input buffer specification; we accept all sizes, so return 0 */
		i_value = 0;
		break;
	default:
		/* ...invalid index */
		return XF_CHK_ERR(0, XA_PARA_ERROR);
	}

	/* ...return buffer size to caller */
	*(u32 *)pv_value = (u32) i_value;

	return XA_SUCCESS;
}

/* ...return memory alignment data */
static DSP_ERROR_TYPE xa_renderer_get_mem_info_alignment(struct XADevRenderer *d, u32 i_idx, void* pv_value)
{
	/* ...basic sanity check */
	XF_CHK_ERR(d && pv_value, XA_PARA_ERROR);

	/* ...command valid only after post-initialization step */
	XF_CHK_ERR(d->state & XA_RENDERER_FLAG_POSTINIT_DONE, XA_PARA_ERROR);

	/* ...all buffers are at least 4-bytes aligned */
	*(u32 *)pv_value = 4;

	return XA_SUCCESS;
}

/* ...return memory type data */
static DSP_ERROR_TYPE xa_renderer_get_mem_info_type(struct XADevRenderer *d, u32 i_idx, void* pv_value)
{
	/* ...basic sanity check */
	XF_CHK_ERR(d && pv_value, XA_PARA_ERROR);

	/* ...command valid only after post-initialization step */
	XF_CHK_ERR(d->state & XA_RENDERER_FLAG_POSTINIT_DONE, XA_PARA_ERROR);
}

/* ...set memory pointer */
static DSP_ERROR_TYPE xa_renderer_set_input_ptr(struct XADevRenderer *d, u32 i_idx, void* pv_value)
{
	/* ...basic sanity check */
	XF_CHK_ERR(d && pv_value, XA_PARA_ERROR);

	/* ...codec must be initialized */
	XF_CHK_ERR(d->state & XA_RENDERER_FLAG_POSTINIT_DONE, XA_PARA_ERROR);

	/* ...select memory buffer */
	switch (i_idx)
	{
	case 0:
		/* ...input buffer */
		d->input = pv_value;
		return XA_SUCCESS;

	default:
		/* ...invalid index */
		return XF_CHK_ERR(0, XA_PARA_ERROR);
	}
}

static DSP_ERROR_TYPE xf_renderer_input_over(struct XADevRenderer *d,
					      u32 i_idx,
					      void *pv_value)
{
	DSP_ERROR_TYPE ret = XA_SUCCESS;

	return XA_SUCCESS;
}

static DSP_ERROR_TYPE xf_renderer_cleanup(struct XADevRenderer *d,
					   u32 i_idx,
					   void *pv_value)
{
	DSP_ERROR_TYPE ret = XA_SUCCESS;
	struct dsp_main_struct *dsp_config =
		(struct dsp_main_struct *)d->private_data;

	MEM_scratch_mfree(&dsp_config->scratch_mem_info, d->dma_buf);
	MEM_scratch_mfree(&dsp_config->scratch_mem_info, d->tcd);

	return ret;
}

/*******************************************************************************
 * API command hooks
 ******************************************************************************/

static DSP_ERROR_TYPE (* const xa_renderer_api[])(struct XADevRenderer *, u32, void*) = 
{
	[XF_API_CMD_GET_API_SIZE]           = xa_renderer_get_api_size,
	[XF_API_CMD_PRE_INIT]               = xf_renderer_preinit,
	[XF_API_CMD_INIT]                   = xa_renderer_init,
	[XF_API_CMD_POST_INIT]              = xf_renderer_postinit,
	[XF_API_CMD_SET_PARAM]              = xa_renderer_set_config_param,
	[XF_API_CMD_GET_PARAM]	            = xa_renderer_get_config_param,
	[XF_API_CMD_EXECUTE]                = xa_renderer_execute,
	[XF_API_CMD_SET_INPUT_BYTES]        = xa_renderer_set_input_bytes,
	[XF_API_CMD_SET_INPUT_PTR]          = xa_renderer_set_input_ptr,
	[XF_API_CMD_INPUT_OVER]             = xf_renderer_input_over,
	[XF_API_CMD_GET_CONSUMED_BYTES]     = xf_renderer_get_consumed_bytes,
	[XF_API_CMD_CLEANUP]                = xf_renderer_cleanup,
};

/* ...total numer of commands supported */
#define XA_RENDERER_API_COMMANDS_NUM   (sizeof(xa_renderer_api) / sizeof(xa_renderer_api[0]))

/*******************************************************************************
 * API entry point
 ******************************************************************************/

DSP_ERROR_TYPE xa_renderer(xf_codec_handle_t handle, u32 i_cmd, u32 i_idx, void* pv_value)
{
	struct XADevRenderer *renderer = (struct XADevRenderer *)handle;

	/* ...check if command index is sane */
	XF_CHK_ERR(i_cmd < XA_RENDERER_API_COMMANDS_NUM, XA_PARA_ERROR);

	/* ...see if command is defined */
	XF_CHK_ERR(xa_renderer_api[i_cmd], XA_PARA_ERROR);

	/* ...execute requested command */
	return xa_renderer_api[i_cmd](renderer, i_idx, pv_value);
}
