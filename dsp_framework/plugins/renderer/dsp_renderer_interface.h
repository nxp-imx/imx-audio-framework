#ifndef _DSP_RENDER_INTERFACE_H_
#define _DSP_RENDER_INTERFACE_H_

#include "audio/xa-renderer-api.h"
/* render type */
#define RENDER_ESAI 0x10
#define RENDER_SAI 0x11

/* hardware address */
#define IRQSTR_QXP_ADDR       0x51080000
#define IRQSTR_QM_ADDR        0x510A0000
#define IRQSTR_MP_ADDR        0x30A80000

/* mp board */
#define SAI_MP_ADDR	      0x30c30000
#define EASRC_MP_ADDR	      0x30c90000
#define SDMA2_ADDR	      0x30e10000
#define SDMA3_ADDR	      0x30e00000

/* qxp&qm board */
#define ESAI_ADDR             0x59010000
#define EDMA_ADDR_ESAI_TX     0x59270000
#define EDMA_ADDR_ESAI_RX     0x59260000

#define ASRC_ADDR		0x59000000
#define EDMA_ADDR_ASRC_RXA	0x59200000
#define EDMA_ADDR_ASRC_TXA	0x59230000

/* module interrupte number */
/* mp board */
#define SAI_MP_INT_NUM 50
#define SDMA2_INT_NUM  103
#define SDMA3_INT_NUM  34
#define EASRC_MP_INT_NUM     122

/* qxp&qm board */
#define SAI_INT_NUM       314
#define EDMA_SAI_INT_NUM  315
#define ESAI_INT_NUM       409
#define EDMA_ESAI_INT_NUM  410
#define ASRC_INT_NUM      372
#define EDMA_ASRC_INT_NUM 374

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
	/* ...component state */
	u32                     suspend_state;
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
	void                  *fe_dev_addr;

	void                  *edma_addr;
	void                  *sdma_addr;
	void                  *fe_edma_addr;

	void                  *irqstr_addr;

	void                  *sdma;
	/* struct nxp_edma_hw_tcd  tcd[MAX_PERIOD_COUNT];*/
	void                  *tcd;
	void                  *tcd_align32;

	void                  *fe_tcd;
	void                  *fe_tcd_align32;

	void                  (*dev_init)(volatile void * dev_addr, int mode,
					  int channel, int rate, int width, int mclk_rate);
	void                  (*dev_start)(volatile void * dev_addr, int tx);
	void                  (*dev_stop)(volatile void * dev_addr, int tx);
	void                  (*dev_isr)(volatile void * dev_addr);
	void                  (*dev_suspend)(volatile void * dev_addr, u32 *cache_addr);
	void                  (*dev_resume)(volatile void * dev_addr, u32 *cache_addr);
	void                  (*fe_dev_isr)(volatile void * dev_addr);


	void                  (*fe_dev_init)(volatile void * dev_addr, int mode,
					     int channel, int rate, int width, int mclk_rate);
	void                  (*fe_dev_start)(volatile void * dev_addr, int tx);
	void                  (*fe_dev_stop)(volatile void * dev_addr, int tx);
	void                  (*fe_dev_suspend)(volatile void * dev_addr, u32 *cache_addr);
	void                  (*fe_dev_resume)(volatile void * dev_addr, u32 *cache_addr);


	u32                   dev_Int;
	u32                   dev_fifo_off;
	u32                   dma_Int;
	u32                   sdma_Int;

	u32                   fe_dev_Int;
	u32                   fe_dma_Int;
	u32                   fe_dev_fifo_in_off;
	u32                   fe_dev_fifo_out_off;
	u32                   irq_2_dsp;

	u32                   dev_cache[40];
	u32                   fe_dev_cache[40];
	u32                   edma_cache[40];
	u32                   fe_edma_cache[40];
};

#endif
