/*******************************************************************************
* Copyright (C) 2017 Cadence Design Systems, Inc.
* Copyright 2020 NXP
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
 * xa-pcm.c
 *
 * PCM format converter plugin
 *
 ******************************************************************************/

#define MODULE_TAG                      PCM

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "xf-types.h"
#include "xf-debug.h"
#include "mydefs.h"
#include "dpu_lib_load.h"
#include "dsp_codec_interface.h"
#include "xf-audio-apicmd.h"
#include "xa-pcm-api.h"

/*******************************************************************************
 * Tracing configuration
 ******************************************************************************/

/*******************************************************************************
 * Local typedefs
 ******************************************************************************/

/* ...API structure */
typedef struct XAPcmCodec
{
    /* ...codec state */
    u32                 state;

    /* ...sampling rate of input/output signal (informative only) */
    u32                 sample_rate;

    /* ...number of input/output channels */
    u8                  in_channels, out_channels;

    /* ...PCM sample width of input/output streams */
    u8                  in_pcm_width, out_pcm_width;

    /* ...input/output stride size */
    u8                  in_stride, out_stride;

    /* ...channel routing map between input and output */
    u32                 chan_routing;

    /* ...data processing hook */
    UA_ERROR_TYPE      (*process)(struct XAPcmCodec *);

    /* ...number of samples in input/output buffers */
    u32                 insize, outsize;

    /* ...input/output memory indices */
    u32                 input_idx, output_idx;

    /* ...input/output buffers passed from/to caller */
    void               *input, *output;

    /* ...number of input bytes consumed/produced */
    u32                 consumed, produced;

    /* ...debug - file handles */
    int                 f_input, f_output;

}   XAPcmCodec;

/*******************************************************************************
 * Local execution flags
 ******************************************************************************/

#define XA_PCM_FLAG_PREINIT_DONE        (1 << 0)
#define XA_PCM_FLAG_POSTINIT_DONE       (1 << 1)
#define XA_PCM_FLAG_RUNNING             (1 << 2)
#define XA_PCM_FLAG_EOS                 (1 << 3)
#define XA_PCM_FLAG_COMPLETE            (1 << 4)

/*******************************************************************************
 * Local constants definitions
 ******************************************************************************/

/* ...process at most 1024 samples per call */
#define XA_PCM_MAX_SAMPLES              1024

/*******************************************************************************
 * Internal processing functions
 ******************************************************************************/

/* ...identity translation of PCM16/24 */
static UA_ERROR_TYPE xa_pcm_do_execute_copy(XAPcmCodec *d)
{
    u32     n = d->insize;
    u8      k = d->in_channels;
    u32     length = n * k * (d->in_pcm_width == 16 ? 2 : 4);
    s16    *input = d->input, *output = d->output;
    int    *buf_in = (int *)d->input;
    int    *buf_out = (int *)d->output;

    /*TRACE(PROCESS, _b("Copy PCM%d %p to %p (%u samples)"), d->in_pcm_width, input, output, n);*/

    /* ...check if we have all data setup */
    XF_CHK_ERR(input && n && output, XA_PCM_EXEC_FATAL_STATE);

    /* ...copy the samples without any processing */
    memcpy(output, input, length);

    /* ...set number of consumed/produced bytes */
    d->consumed = length;
    d->produced = length;

    /* ...reset input buffer length */
    d->insize = 0;

    /* ...copy input to output */
    return ACODEC_SUCCESS;
}

/* ...data processing for PCM16, channel mapping case */
static UA_ERROR_TYPE xa_pcm_do_execute_pcm16_chmap(XAPcmCodec *d)
{
    u32     n = d->insize, i;
    u8      k = d->in_channels, j;
    u32     chmap = d->chan_routing, map;
    s16    *input = d->input, *output = d->output;
    u32     length = n * k * (d->in_pcm_width == 16 ? 2 : 4);

    /*TRACE(PROCESS, _b("Map PCM16 %p to %p (%u samples, map: %X)"), input, output, n, chmap);    */

    /* ...check if we have all data setup */
    XF_CHK_ERR(input && n && output, XA_PCM_EXEC_FATAL_STATE);

#if 0
    /* ...convert individual samples (that function could be CPU-optimized - tbd) */
    for (i = 0; i < n; i++, input += k)
    {
        /* ...process individual channels in a sample */
        for (j = 0, map = chmap; j < k; j++, map >>= 4)
        {
            u8      m = map & 0xF;

            /* ...fill output channel (zero unused channel) */
            *output++ = (m < 8 ? input[m] : 0);
        }
    }

    /* ...set number of consumed/produced bytes */
    d->consumed = (u32)((u8 *)input - (u8 *)d->input);
    d->produced = (u32)((u8 *)output - (u8 *)d->output);
#else
    memcpy(output, input, length);
    /* ...set number of consumed/produced bytes */
    d->consumed = length;
    d->produced = length;
#endif
    /* ...reset input buffer length */
    d->insize = 0;

    /* ...copy input to output */
    return ACODEC_SUCCESS;
}

/* ...data processing for PCM24/PCM32, channel mapping case */
static UA_ERROR_TYPE xa_pcm_do_execute_pcm24_chmap(XAPcmCodec *d)
{
    u32     n = d->insize, i;
    u8      k = d->in_channels, j;
    u32     chmap = d->chan_routing, map;
    s32    *input = d->input, *output = d->output;

    /*TRACE(PROCESS, _b("Map PCM24 %p to %p (%u samples, map: %X)"), input, output, n, chmap);*/

    /* ...check if we have all data setup */
    XF_CHK_ERR(input && n && output, XA_PCM_EXEC_FATAL_STATE);

    /* ...convert individual samples (that function could be CPU-optimized - tbd) */
    for (i = 0; i < n; i++, input += k)
    {
        /* ...process individual channels in a sample */
        for (j = 0, map = chmap; j < k; j++, map >>= 4)
        {
            u8      m = map & 0xF;

            /* ...fill output channel (zero unused channel) */
            *output++ = (m < 8 ? input[m] : 0);
        }
    }

    /* ...set number of consumed/produced bytes */
    d->consumed = (u32)((u8 *)input - (u8 *)d->input);
    d->produced = (u32)((u8 *)output - (u8 *)d->output);

    /* ...reset input buffer length */
    d->insize = 0;

    /* ...copy input to output */
    return ACODEC_SUCCESS;
}

/* ...convert multichannel 24-bit PCM to 16-bit PCM with channel mapping */
static UA_ERROR_TYPE xa_pcm_do_execute_pcm24_to_pcm16(XAPcmCodec *d)
{
    u32     n = d->insize, i;
    u8      k = d->in_channels, j;
    u32     chmap = d->chan_routing, map;
    s32    *input = d->input;
    s16    *output = d->output;

    /*TRACE(PROCESS, _b("Convert PCM24 %p to PCM16 %p (%u samples, map: %X)"), input, output, n, chmap);*/

    /* ...check if we have all data setup */
    XF_CHK_ERR(input && n && output, XA_PCM_EXEC_FATAL_STATE);

    /* ...convert individual samples (that function could be CPU-optimized - tbd) */
    for (i = 0; i < n; i++, input += k)
    {
        /* ...process individual channels in a sample */
        for (j = 0, map = chmap; j < k; j++, map >>= 4)
        {
            u8      m = map & 0xF;

            /* ...convert and zero out unused channels */
            *output++ = (m < 8 ? input[m] >> 16 : 0);
        }
    }

    /* ...set number of consumed/produced bytes */
    d->consumed = (u32)((u8 *)input - (u8 *)d->input);
    d->produced = (u32)((u8 *)output - (u8 *)d->output);

    /* ...dump output data */
    //BUG(write(d->f_input, d->input, d->consumed) != d->consumed, _x("%m"));
    //BUG(write(d->f_output, d->output, d->produced) != d->produced, _x("%m"));

    /* ...reset input buffer length (tbd - need that?) */
    d->insize = 0;

    /* ...copy input to output */
    return ACODEC_SUCCESS;
}

/* ...convert multichannel 16-bit PCM to 24-bit PCM with channel mapping */
static UA_ERROR_TYPE xa_pcm_do_execute_pcm16_to_pcm24(XAPcmCodec *d)
{
    u32     n = d->insize, i;
    u8      k = d->in_channels, j;
    u32     chmap = d->chan_routing, map;
    s16    *input = d->input;
    s32    *output = d->output;

    /*TRACE(PROCESS, _b("Convert PCM16 %p to PCM24 %p (%u samples, map: %X)"), input, output, n, chmap);*/

    /* ...check if we have all data setup */
    XF_CHK_ERR(input && n && output, XA_PCM_EXEC_FATAL_STATE);

    /* ...convert individual samples (that function could be CPU-optimized - tbd) */
    for (i = 0; i < n; i++, input += k)
    {
        /* ...process individual channels in a sample */
        for (j = 0, map = chmap; j < k; j++, map >>= 4)
        {
            u8      m = map & 0xF;

            /* ...convert and zero out unused channels */
            *output++ = (m < 8 ? input[m] << 16 : 0);
        }
    }

    /* ...set number of consumed/produced bytes */
    d->consumed = (u32)((u8 *)input - (u8 *)d->input);
    d->produced = (u32)((u8 *)output - (u8 *)d->output);

    /* ...reset input buffer length (tbd - need that?) */
    d->insize = 0;

    /* ...copy input to output */
    return ACODEC_SUCCESS;
}

/* ...determine if we need to do a channel routing */
static inline int xa_pcm_is_identity_mapping(u32 chmap, u8 k)
{
    u8      j;

    for (j = 0; j < k; j++, chmap >>= 4)
        if ((chmap & 0xF) != j)
            return 0;

    return 1;
}

/* ...runtime initialization */
static inline UA_ERROR_TYPE xa_pcm_do_runtime_init(XAPcmCodec *d)
{
    u8      in_width = d->in_pcm_width, out_width = d->out_pcm_width;
    u8      in_ch = d->in_channels, out_ch = d->out_channels;
    u32     chmap = d->chan_routing;

    /* ...check for supported processing schemes */
    if (in_width == out_width)
    {
        /* ...check if we need to do a channel mapping */
        if (in_ch != out_ch || !xa_pcm_is_identity_mapping(chmap, in_ch))
        {
            /* ...mapping is needed */
            d->process = (in_width == 16 ? xa_pcm_do_execute_pcm16_chmap : xa_pcm_do_execute_pcm24_chmap);
        }
        else
        {
            /* ...setup identity translation */
            d->process = xa_pcm_do_execute_copy;
        }
    }
    else
    {
        /* ...samples converion is required */
        d->process = (in_width == 16 ? xa_pcm_do_execute_pcm16_to_pcm24 : xa_pcm_do_execute_pcm24_to_pcm16);
    }

    /* ...mark the runtime initialization is completed */
    d->state = XA_PCM_FLAG_PREINIT_DONE | XA_PCM_FLAG_POSTINIT_DONE | XA_PCM_FLAG_RUNNING;

    /*TRACE(INIT, _b("PCM format converter initialized: PCM%u -> PCM%u, ich=%u, och=%u, map=%X"), in_width, out_width, in_ch, out_ch, chmap);*/

    return ACODEC_SUCCESS;
}

/*******************************************************************************
 * Commands processing
 ******************************************************************************/

/* ...standard codec initialization routine */
static UA_ERROR_TYPE xa_pcm_get_api_size(XAPcmCodec *d, u32 i_idx, void *pv_value)
{
    /* ...return API structure size */
    *(s32 *)pv_value = sizeof(*d);

    return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xa_pcm_preinit(XAPcmCodec *d,
					   u32 i_idx,
					   void *pv_value)
{
    /* ...sanity check */
    XF_CHK_ERR(d, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...pre-configuration initialization; reset internal data */
    memset(d, 0, sizeof(*d));

    /* ...set default parameters */
    d->in_pcm_width = d->out_pcm_width = 16;
    d->in_channels = d->out_channels = 2;
    d->chan_routing = (0 << 0) | (1 << 4);
    d->sample_rate = 48000;

    /* ...open debug files */
    //BUG((d->f_input = open("pcm-in.dat", O_WRONLY | O_CREAT, 0664)) < 0, _x("%m"));
    //BUG((d->f_output = open("pcm-out.dat", O_WRONLY | O_CREAT, 0664)) < 0, _x("%m"));

    /* ...mark pre-initialization is done */
    d->state = XA_PCM_FLAG_PREINIT_DONE;

    return ACODEC_SUCCESS;
}

/* ...standard codec initialization routine */
static UA_ERROR_TYPE xa_pcm_init(XAPcmCodec *d, u32 i_idx, void *pv_value)
{
    /* ...sanity check */
    XF_CHK_ERR(d, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...run-time initialization process; make sure post-init is complete */
    /*XF_CHK_ERR(d->state & XA_PCM_FLAG_POSTINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);*/

    /* ...initialize runtime for specified transformation function */
    return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xa_pcm_postinit(struct XAPcmCodec *d,
					    u32 i_idx,
					    void *pv_value)
{
    /* ...sanity check */
    XF_CHK_ERR(d, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...post-configuration initialization (all parameters are set) */
    XF_CHK_ERR(d->state & XA_PCM_FLAG_PREINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...calculate input sample stride size */
    d->in_stride = d->in_channels * (d->in_pcm_width == 16 ? 2 : 4);
    d->out_stride = d->out_channels * (d->out_pcm_width == 16 ? 2 : 4);

    /* ...mark post-initialization is complete */
    d->state |= XA_PCM_FLAG_POSTINIT_DONE;

    return xa_pcm_do_runtime_init(d);
}

/* ...set configuration parameter */
static UA_ERROR_TYPE xa_pcm_set_config_param(XAPcmCodec *d, u32 i_idx, void *pv_value)
{
    s32      i_value;

    /* ...sanity check */
    /*XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);*/

    /* ...configuration is allowed only in PRE-CONFIG state */
    XF_CHK_ERR(d->state == XA_PCM_FLAG_PREINIT_DONE, XA_PCM_CONFIG_NONFATAL_STATE);

    /* ...get integer parameter value */
    if (pv_value)
        i_value = *(s32 *)pv_value;

    /* ...process individual configuration parameter */
    switch (i_idx)
    {
    case XA_PCM_CONFIG_PARAM_SAMPLE_RATE:
        /* ...accept any sampling rate */
        d->sample_rate = (u32)i_value;
        return ACODEC_SUCCESS;

    case XA_PCM_CONFIG_PARAM_IN_PCM_WIDTH:
        /* ...return input sample bit-width */
        XF_CHK_ERR(i_value == 16 || i_value == 32, XA_PCM_CONFIG_NONFATAL_RANGE);
        d->in_pcm_width = (u8)i_value;
        return ACODEC_SUCCESS;

    case XA_PCM_CONFIG_PARAM_IN_CHANNELS:
        /* ...support at most 8-channels stream */
        XF_CHK_ERR(i_value > 0 && i_value <= 8, XA_PCM_CONFIG_NONFATAL_RANGE);
        d->in_channels = (u8)i_value;
        return ACODEC_SUCCESS;

    case XA_PCM_CONFIG_PARAM_OUT_PCM_WIDTH:
        /* ...we only support PCM16 and PCM24 */
        XF_CHK_ERR(i_value == 16 || i_value == 32, XA_PCM_CONFIG_NONFATAL_RANGE);
        d->out_pcm_width = (u8)i_value;
        return ACODEC_SUCCESS;

    case XA_PCM_CONFIG_PARAM_OUT_CHANNELS:
        /* ...support at most 8-channels stream */
        XF_CHK_ERR(i_value > 0 && i_value <= 8, XA_API_FATAL_INVALID_CMD_TYPE);
        d->out_channels = (u8)i_value;
        return ACODEC_SUCCESS;

    case XA_PCM_CONFIG_PARAM_CHANROUTING:
        /* ...accept any channel routing mask */
        d->chan_routing = (u32)i_value;
        return ACODEC_SUCCESS;

    case XA_PCM_CONFIG_PARAM_FUNC_PRINT:
	return ACODEC_SUCCESS;

    default:
        /* ...unrecognized parameter */
        return XF_CHK_ERR(0, XA_API_FATAL_INVALID_CMD_TYPE);
    }
}

/* ...retrieve configuration parameter */
static UA_ERROR_TYPE xa_pcm_get_config_param(XAPcmCodec *d, u32 i_idx, void *pv_value)
{
    /* ...sanity check */
    XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...make sure pre-configuration is completed */
    XF_CHK_ERR(d->state & XA_PCM_FLAG_PREINIT_DONE, XA_PCM_CONFIG_NONFATAL_STATE);

    /* ...process individual parameter */
    switch (i_idx)
    {
    case XA_PCM_CONFIG_PARAM_SAMPLE_RATE:
        /* ...return output sampling frequency */
        *(s32 *)pv_value = d->sample_rate;
        return ACODEC_SUCCESS;

    case XA_PCM_CONFIG_PARAM_IN_PCM_WIDTH:
        /* ...return input sample bit-width */
        *(s32 *)pv_value = d->in_pcm_width;
        return ACODEC_SUCCESS;

    case XA_PCM_CONFIG_PARAM_IN_CHANNELS:
        /* ...return number of input channels */
        *(s32 *)pv_value = d->in_channels;
        return ACODEC_SUCCESS;

    case XA_PCM_CONFIG_PARAM_OUT_PCM_WIDTH:
        /* ...return output sample bit-width */
        *(s32 *)pv_value = d->out_pcm_width;
        return ACODEC_SUCCESS;

    case XA_PCM_CONFIG_PARAM_OUT_CHANNELS:
        /* ...return number of output channels */
        *(s32 *)pv_value = d->out_channels;
        return ACODEC_SUCCESS;

    case XA_PCM_CONFIG_PARAM_CHANROUTING:
        /* ...return current channel routing mask */
        *(s32 *)pv_value = d->chan_routing;
        return ACODEC_SUCCESS;

    default:
        /* ...unrecognized parameter */
        return XF_CHK_ERR(0, XA_API_FATAL_INVALID_CMD_TYPE);
    }
}

/* ...execution command */
static UA_ERROR_TYPE xa_pcm_execute(XAPcmCodec *d, u32 i_idx, void *pv_value)
{
    /* ...basic sanity check */
    XF_CHK_ERR(d, XA_API_FATAL_INVALID_CMD_TYPE);
   /* ...codec must be in running state */
    XF_CHK_ERR(d->state & XA_PCM_FLAG_RUNNING, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...process individual command type */
    /* ...do data processing (tbd - result code is bad) */
    d->produced = 0;

    if (d->insize != 0)
    {
        XF_CHK_ERR(!d->process(d), XA_PCM_EXEC_FATAL_STATE);
    }

    /* ...process end-of-stream condition */
    (d->state & XA_PCM_FLAG_EOS ? d->state ^= XA_PCM_FLAG_EOS | XA_PCM_FLAG_COMPLETE : 0);

    if (d->state & XA_PCM_FLAG_COMPLETE)
	return ACODEC_END_OF_STREAM;

    return ACODEC_SUCCESS;
}

/* ...set number of input bytes */
static UA_ERROR_TYPE xa_pcm_set_input_bytes(XAPcmCodec *d, u32 i_idx, void *pv_value)
{
    u32     in_stride = d->in_stride;
    u32     insize;

    /* ...sanity check */
    XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...track index must be valid */
    XF_CHK_ERR(i_idx == 0, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...component must be initialized */
    XF_CHK_ERR(d->state & XA_PCM_FLAG_POSTINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...input buffer must exist */
    XF_CHK_ERR(d->input, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...convert bytes into samples (don't like division, but still...) */
    insize = *(s32 *)pv_value / in_stride;

    /* ...make sure we have integral amount of samples */
    XF_CHK_ERR(*(s32 *)pv_value == insize * in_stride, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...limit input buffer size to maximal value*/
    d->insize = (insize > XA_PCM_MAX_SAMPLES ? XA_PCM_MAX_SAMPLES : insize);

    return ACODEC_SUCCESS;
}

/* ...get number of output bytes produced */
static UA_ERROR_TYPE xa_pcm_get_output_bytes(XAPcmCodec *d, u32 i_idx, void *pv_value)
{
    /* ...sanity check */
    XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...buffer index must be sane */
    XF_CHK_ERR(i_idx == 1, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...component must be initialized */
    XF_CHK_ERR(d->state & XA_PCM_FLAG_POSTINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...output buffer must exist */
    XF_CHK_ERR(d->output, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...return number of produced bytes (and reset instantly? - tbd) */
    *(s32 *)pv_value = d->produced;

    return ACODEC_SUCCESS;
}

/* ...get number of consumed bytes */
static UA_ERROR_TYPE xa_pcm_get_curidx_input_buf(XAPcmCodec *d, u32 i_idx, void *pv_value)
{
    /* ...sanity check */
    XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...index must be valid */
    XF_CHK_ERR(i_idx == 0, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...input buffer must exist */
    XF_CHK_ERR(d->input, XA_PCM_EXEC_NONFATAL_INPUT);

    /* ...return number of bytes consumed */
    *(s32 *)pv_value = d->consumed;

    return ACODEC_SUCCESS;
}

/* ...end-of-stream processing */
static UA_ERROR_TYPE xa_pcm_input_over(XAPcmCodec *d, u32 i_idx, void *pv_value)
{
    /* ...sanity check */
    XF_CHK_ERR(d, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...put end-of-stream flag */
    d->state |= XA_PCM_FLAG_EOS;

    /*TRACE(PROCESS, _b("Input-over-condition signalled"));*/

    return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xf_pcm_set_input_ptr(XAPcmCodec *d,
						 u32 i_idx,
						 void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	d->input = (u8 *)pv_value;

	return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xf_pcm_set_output_ptr(XAPcmCodec *d,
						  u32 i_idx,
						  void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	d->output = (u8 *)pv_value;

	return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xf_pcm_runtime_init(XAPcmCodec *d,
						u32 i_idx,
						void *pv_value)
{
	return ACODEC_SUCCESS;
}


#if 0
/* ..get total amount of data for memory tables */
static UA_ERROR_TYPE xa_pcm_get_memtabs_size(XAPcmCodec *d, s32 i_idx, void *pv_value)
{
    /* ...basic sanity checks */
    XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...check mixer is pre-initialized */
    XF_CHK_ERR(d->state & XA_PCM_FLAG_PREINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...we have all our tables inside API structure */
    *(s32 *)pv_value = 0;

    return ACODEC_SUCCESS;
}

/* ...return total amount of memory buffers */
static UA_ERROR_TYPE xa_pcm_get_n_memtabs(XAPcmCodec *d, s32 i_idx, void *pv_value)
{
    /* ...basic sanity checks */
    XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...we have 1 input buffer and 1 output buffer */
    *(s32 *)pv_value = 1 + 1;

    return ACODEC_SUCCESS;
}

/* ...return memory type data */
static UA_ERROR_TYPE xa_pcm_get_mem_info_type(XAPcmCodec *d, s32 i_idx, void *pv_value)
{
    /* ...basic sanity check */
    XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...codec must be in post-init state */
    XF_CHK_ERR(d->state & XA_PCM_FLAG_POSTINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...check buffer type */
    switch (i_idx)
    {
    case 0:
        *(s32 *)pv_value = XA_MEMTYPE_INPUT;
        return ACODEC_SUCCESS;

    case 1:
        *(s32 *)pv_value = XA_MEMTYPE_OUTPUT;
        return ACODEC_SUCCESS;

    default:
        return XF_CHK_ERR(0, XA_API_FATAL_INVALID_CMD_TYPE);
    }
}

/* ...return memory buffer size */
static UA_ERROR_TYPE xa_pcm_get_mem_info_size(XAPcmCodec *d, s32 i_idx, void *pv_value)
{
    /* ...basic sanity check */
    XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...codec must be in post-init state */
    XF_CHK_ERR(d->state & XA_PCM_FLAG_POSTINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...determine particular buffer */
    switch (i_idx)
    {
    case 0:
        /* ...input buffer size can be any */
        *(s32 *)pv_value = 0;
        return ACODEC_SUCCESS;

    case 1:
        /* ...output buffer size is dependent on stride */
        *(s32 *)pv_value = XA_PCM_MAX_SAMPLES * d->out_stride;
        return ACODEC_SUCCESS;

    default:
        /* ...invalid buffer index */
        return XF_CHK_ERR(0, XA_API_FATAL_INVALID_CMD_TYPE);
    }
}

/* ...return memory alignment data */
static UA_ERROR_TYPE xa_pcm_get_mem_info_alignment(XAPcmCodec *d, s32 i_idx, void *pv_value)
{
    /* ...basic sanity check */
    XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...codec must be in post-initialization state */
    XF_CHK_ERR(d->state & XA_PCM_FLAG_POSTINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...all buffers are 4-bytes aligned */
    *(s32 *)pv_value = 4;

    return ACODEC_SUCCESS;
}

/* ...set memory pointer */
static UA_ERROR_TYPE xa_pcm_set_mem_ptr(XAPcmCodec *d, s32 i_idx, void *pv_value)
{
    /* ...basic sanity check */
    XF_CHK_ERR(d, XA_API_FATAL_INVALID_CMD_TYPE);
    XF_CHK_ERR(pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...codec must be in post-initialized state */
    XF_CHK_ERR(d->state & XA_PCM_FLAG_POSTINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...select memory buffer */
    switch (i_idx)
    {
    case 0:
        /* ...input buffer */
        d->input = pv_value;
        return ACODEC_SUCCESS;

    case 1:
        /* ...output buffer */
        d->output = pv_value;
        return ACODEC_SUCCESS;

    default:
        /* ...invalid index */
        return XF_CHK_ERR(0, XA_API_FATAL_INVALID_CMD_TYPE);
    }
}
#endif
/*******************************************************************************
 * API command hooks
 ******************************************************************************/

static UA_ERROR_TYPE (* const xa_pcm_api[])(XAPcmCodec *, u32, void *) =
{
    [XF_API_CMD_GET_API_SIZE]           = xa_pcm_get_api_size,
    [XF_API_CMD_PRE_INIT]               = xa_pcm_preinit,
    [XF_API_CMD_INIT]                   = xa_pcm_init,
    [XF_API_CMD_POST_INIT]              = xa_pcm_postinit,
    [XF_API_CMD_SET_PARAM]              = xa_pcm_set_config_param,
    [XF_API_CMD_GET_PARAM]              = xa_pcm_get_config_param,

    [XF_API_CMD_EXECUTE]                = xa_pcm_execute,
    [XF_API_CMD_SET_INPUT_BYTES]        = xa_pcm_set_input_bytes,
    [XF_API_CMD_GET_OUTPUT_BYTES]       = xa_pcm_get_output_bytes,
    [XF_API_CMD_GET_CONSUMED_BYTES]     = xa_pcm_get_curidx_input_buf,
    [XF_API_CMD_INPUT_OVER]             = xa_pcm_input_over,

    [XF_API_CMD_SET_INPUT_PTR]          = xf_pcm_set_input_ptr,
    [XF_API_CMD_SET_OUTPUT_PTR]         = xf_pcm_set_output_ptr,
    [XF_API_CMD_RUNTIME_INIT]           = xf_pcm_runtime_init,

/*
    [XF_API_CMD_GET_MEMTABS_SIZE]       = xa_pcm_get_memtabs_size,
    [XA_API_CMD_GET_N_MEMTABS]          = xa_pcm_get_n_memtabs,
    [XA_API_CMD_GET_MEM_INFO_TYPE]      = xa_pcm_get_mem_info_type,
    [XA_API_CMD_GET_MEM_INFO_SIZE]      = xa_pcm_get_mem_info_size,
    [XA_API_CMD_GET_MEM_INFO_ALIGNMENT] = xa_pcm_get_mem_info_alignment,
    [XA_API_CMD_SET_MEM_PTR]            = xa_pcm_set_mem_ptr,
*/
};

/* ...total numer of commands supported */
#define XA_PCM_API_COMMANDS_NUM     (sizeof(xa_pcm_api) / sizeof(xa_pcm_api[0]))

/*******************************************************************************
 * API entry point
 ******************************************************************************/

UA_ERROR_TYPE xa_pcm_codec(xf_codec_handle_t p_xa_module_obj, s32 i_cmd, s32 i_idx, void *pv_value)
{
    XAPcmCodec *d = (XAPcmCodec *) p_xa_module_obj;

    /* ...check if command index is sane */
    XF_CHK_ERR(i_cmd < XA_PCM_API_COMMANDS_NUM, XA_API_FATAL_INVALID_CMD);

    /* ...see if command is defined */
    XF_CHK_ERR(xa_pcm_api[i_cmd], XA_API_FATAL_INVALID_CMD);

    /* ...execute requested command */
    return xa_pcm_api[i_cmd](d, i_idx, pv_value);
}
