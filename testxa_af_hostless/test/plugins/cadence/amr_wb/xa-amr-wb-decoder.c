/*
* Copyright (c) 2015-2021 Cadence Design Systems Inc.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
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
*/
/*******************************************************************************
 * xa-amrwb-decoder.c
 *
 * AMR-WB decoder plugin - thin wrapper around AMR-WB-DEC library
 ******************************************************************************/

#define MODULE_TAG                      AMRWB_DEC

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include <stdint.h>

#include "xa_type_def.h"
/* ...debugging facility */
#include "xf-debug.h"
#include "xa_amr_wb_codec_api.h"
#include "xa_amr_wb_dec_definitions.h"
#include "audio/xa-audio-decoder-api.h"

#ifdef XAF_PROFILE
#include "xaf-clk-test.h"
extern clk_t dec_cycles;
#endif

/* ...API structure */
typedef struct XA_AMR_WB_Decoder
{
    /* ...AMR dec module state */
    UWORD32            state;

    /* ...number of channels (channel mask?) */
    UWORD32            channels;

    /* ...PCM sample width */
    UWORD32            pcm_width;

    /* ...sampling rate */
    UWORD32            sample_rate;

    /* ...number of bytes in input/output buffer */
    UWORD32            buffer_size;

    /* ...input buffer */
    void               *input;

    /* ...output buffer */
    void               *output;

    /* ...scratch buffer pointer */
    void               *scratch;

    /* ...number of available bytes in the input buffer */
    UWORD32            input_avail;

    /* ...number of bytes consumed from input buffer */
    UWORD32            consumed;

    /* ...number of produced bytes */
    UWORD32            produced;
    
    /* ...AMR-WB decoder internal data, needs 8 byte alignment */
    UWORD32            internal_data __attribute__((aligned(8)));

}   XA_AMR_WB_Decoder;

/*******************************************************************************
 * AMR-WB Decoder state flags
 ******************************************************************************/

#define XA_AMR_WB_FLAG_PREINIT_DONE      (1 << 0)
#define XA_AMR_WB_FLAG_POSTINIT_DONE     (1 << 1)
#define XA_AMR_WB_FLAG_RUNNING           (1 << 2)
#define XA_AMR_WB_FLAG_OUTPUT            (1 << 3)
#define XA_AMR_WB_FLAG_EOS_RECEIVED      (1 << 4)
#define XA_AMR_WB_FLAG_COMPLETE          (1 << 5)

/*******************************************************************************
 * Supportive functions
 ******************************************************************************/
/* This function is reused from the AMR-WB decoder test bench:
\xa_amr_wb_codec\test\src\xa_amr_wb_dec_sample_testbench.c 
The original code - marked by #ifdef ORI_TEST has been preserved for reference */
/* Read encoded speech data in 3GPP conformance format. */
static int
#ifdef ORI_TEST
read_serial (FILE *fp,
#else
read_serial (WORD16 *enc_data,
#endif
	     WORD16 *enc_speech,
	     xa_amr_wb_rx_frame_type_t *frame_type,
	     xa_amr_wb_mode_t *mode)
{
  static const WORD16 nb_bits[] =
    { 132, 177, 253, 285, 317, 365, 397, 461, 477, 35 };

  int n;
  WORD16 type_of_frame_type, ftype, coding_mode;

#ifdef ORI_TEST
  n = (int)xa_fread(&type_of_frame_type, sizeof(WORD16), 1, fp);
  n += (int)xa_fread(&ftype, sizeof(WORD16), 1, fp);
  n += (int)xa_fread(&coding_mode, sizeof(WORD16), 1, fp);
  
  if (n != 3)
    return 0;
#else
  type_of_frame_type = *enc_data++;
  ftype = *enc_data++;
  coding_mode = *enc_data++;
#endif

#if __XTENSA_EB__
  /* Assume the conformance input is in little-endian (Intel) byte
     order. */
  type_of_frame_type = AE_SHA32(type_of_frame_type);
  ftype = AE_SHA32(ftype);
  coding_mode = AE_SHA32(coding_mode);
#endif /* __XTENSA_EB__ */

#ifdef ORI_TEST
  if (coding_mode < 0 || coding_mode >= 11)
    {
      FIO_PRINTF(stderr, "Invalid mode received: %d (check file format)\n", coding_mode);
      exit(1);
    }
#else
  XF_CHK_ERR((coding_mode >= 0 && coding_mode < 11), XA_AMR_WB_DEC_EXEC_FATAL_INVALID_DATA);
#endif
  *mode = coding_mode;
  
  if (type_of_frame_type == XA_AMR_WB_FRAME_TYPE_TX)
    {
      switch (ftype)
        {
        case XA_AMR_WB_TX_SPEECH:
	  ftype = XA_AMR_WB_RX_SPEECH_GOOD;
	  break;
        case XA_AMR_WB_TX_SID_FIRST:
	  ftype = XA_AMR_WB_RX_SID_FIRST;
	  break;
        case XA_AMR_WB_TX_SID_UPDATE:
	  ftype = XA_AMR_WB_RX_SID_UPDATE;
	  break;
        case XA_AMR_WB_TX_NO_DATA:
	  ftype = XA_AMR_WB_RX_NO_DATA;
	  break;
        }
    }
#ifdef ORI_TEST
  else if (type_of_frame_type != XA_AMR_WB_FRAME_TYPE_RX)
    {
        
      FIO_PRINTF(stderr, "Invalid type of frame type: %d (check file format)\n",
	      type_of_frame_type);
      exit(1);
    }
#else
   else  
    {
      XF_CHK_ERR((type_of_frame_type == XA_AMR_WB_FRAME_TYPE_RX), XA_AMR_WB_DEC_EXEC_FATAL_INVALID_DATA);
    }
#endif

  if ((ftype == XA_AMR_WB_RX_SID_FIRST) ||
      (ftype == XA_AMR_WB_RX_SID_UPDATE) ||
      (ftype == XA_AMR_WB_RX_NO_DATA) ||
      (ftype == XA_AMR_WB_RX_SID_BAD))
    {
      coding_mode = XA_AMR_WB_MRDTX;
    }

  *frame_type = ftype;
  
  n = nb_bits[coding_mode];
#ifdef ORI_TEST
  if (xa_fread(enc_speech, sizeof(WORD16), n, fp) != n)
    return 0;
#else
    {
      int Idx;
      for (Idx = 0; Idx < n; Idx++)
        enc_speech[Idx] = *enc_data++;
    }
#endif

#if __XTENSA_EB__
    {
        /* Assume the conformance input is in little-endian (Intel) byte
        order. */
        int i;
        for (i = 0; i < n; i++)
            enc_speech[i] = AE_SHA32(enc_speech[i]);
    }
#endif /* __XTENSA_EB__ */

#ifdef ORI_TEST
  return n;
#else
  return (2*(n + 3)); // Returning number of bytes consumed
#endif
}

/*******************************************************************************
 * Commands processing
 ******************************************************************************/

static XA_ERRORCODE xa_amr_wb_decoder_get_api_size(XA_AMR_WB_Decoder *d, WORD32 i_idx, pVOID pv_value)
{
    /* ...sanity check */
    //XF_CHK_ERR(d, XA_API_FATAL_INVALID_CMD_TYPE);

    *(WORD32 *)pv_value = xa_amr_wb_dec_get_handle_byte_size();
    *(WORD32 *)pv_value += sizeof(XA_AMR_WB_Decoder);

    return XA_NO_ERROR;
}

static XA_ERRORCODE xa_amr_wb_decoder_init(XA_AMR_WB_Decoder *d, WORD32 i_idx, pVOID pv_value)
{
    /* ...sanity check */
    XF_CHK_ERR(d, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...process particular initialization type */
    switch (i_idx)
    {
    case XA_CMD_TYPE_INIT_API_PRE_CONFIG_PARAMS:
        {
            /* Mark AMR-WB component Pre-Initialization is DONE */
            d->state |= XA_AMR_WB_FLAG_PREINIT_DONE;
            return XA_NO_ERROR;
        }

    case XA_CMD_TYPE_INIT_API_POST_CONFIG_PARAMS:
        {
            /* ...post-configuration initialization (all parameters are set) */
            XF_CHK_ERR(d->state & XA_AMR_WB_FLAG_PREINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

            /* Mark post-initialization is complete */
            d->state |= XA_AMR_WB_FLAG_POSTINIT_DONE;
            return XA_NO_ERROR;
        }

    case XA_CMD_TYPE_INIT_PROCESS:
        {
            XA_ERRORCODE err_code;
            
            /* ...kick run-time initialization process; make sure AMR dec component is setup */
            XF_CHK_ERR(d->state & XA_AMR_WB_FLAG_POSTINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

            /* ...Call the core library initialization function */
            err_code = xa_amr_wb_dec_init((xa_codec_handle_t)(&d->internal_data), d->scratch, XA_AMR_WB_FMT_CONFORMANCE);
            XF_CHK_ERR(err_code == XA_NO_ERROR, XA_API_FATAL_INVALID_CMD_TYPE);
            
            /* ...enter into execution stage */
            d->state |= XA_AMR_WB_FLAG_RUNNING;
            return XA_NO_ERROR;
        }

    case XA_CMD_TYPE_INIT_DONE_QUERY:
        {
            /* ...check if initialization is done; make sure pointer is sane */
            XF_CHK_ERR(pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

            /* ...put current status */
            *(WORD32 *)pv_value = (d->state & XA_AMR_WB_FLAG_RUNNING ? 1 : 0);
            return XA_NO_ERROR;
        }

    default:
        /* ...unrecognised command type */
        TRACE(ERROR, _x("Unrecognised command type: %X"), i_idx);
        return XA_API_FATAL_INVALID_CMD_TYPE;
    }
}

static XA_ERRORCODE xa_amr_wb_decoder_set_config_param(XA_AMR_WB_Decoder *d, WORD32 i_idx, pVOID pv_value)
{
    UWORD32     i_value = *(UWORD32 *)pv_value;

    /* ...sanity check */
    XF_CHK_ERR(d, XA_API_FATAL_INVALID_CMD_TYPE);

    /* Since for AMR-WB these config parameters fixed we are using the hard coded values */
    switch (i_idx)
    {
    case XA_CODEC_CONFIG_PARAM_CHANNELS:
    case XA_AMR_WB_DEC_CONFIG_PARAM_NUM_CHANNELS:
        /* ...return number of output channels */
        XF_CHK_ERR(i_value != 1, XA_AMR_WB_DEC_CONFIG_NONFATAL_RANGE);
        break;

    case XA_CODEC_CONFIG_PARAM_SAMPLE_RATE:
    case XA_AMR_WB_DEC_CONFIG_PARAM_SAMP_FREQ:
        /* ...return output sampling frequency */
        XF_CHK_ERR(i_value != XA_AMR_WB_CONFIG_PARAM_SAMP_FREQ, XA_AMR_WB_DEC_CONFIG_NONFATAL_RANGE);
        break;

    case XA_CODEC_CONFIG_PARAM_PCM_WIDTH:
    case XA_AMR_WB_DEC_CONFIG_PARAM_PCM_WDSZ:
        /* ...return sample bit-width */
        XF_CHK_ERR(i_value != XA_AMR_WB_CONFIG_PARAM_PCM_WDSZ, XA_AMR_WB_DEC_CONFIG_NONFATAL_RANGE);
        break;

    default:
        /* ...unrecognised command */
        TRACE(ERROR, _x("Invalid index: %X"), i_idx);
        return XA_API_FATAL_INVALID_CMD_TYPE;
    }

    return XA_NO_ERROR;
}

static inline XA_ERRORCODE xa_amr_wb_decoder_get_config_param(XA_AMR_WB_Decoder *d, WORD32 i_idx, pVOID pv_value)
{
    pUWORD32    pui_value    = (pUWORD32) pv_value;

    /* ...sanity check */
    XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    /* Since for AMR-WB these config parameters fixed we are using the hard coded values */
    switch (i_idx)
    {
    case XA_CODEC_CONFIG_PARAM_PCM_WIDTH:
    case XA_AMR_WB_DEC_CONFIG_PARAM_PCM_WDSZ:
        /* ...return sample bit-width */
        *pui_value = XA_AMR_WB_CONFIG_PARAM_PCM_WDSZ;
        break;

    case XA_CODEC_CONFIG_PARAM_SAMPLE_RATE:
    case XA_AMR_WB_DEC_CONFIG_PARAM_SAMP_FREQ:
        /* ...return output sampling frequency */
        *pui_value = XA_AMR_WB_CONFIG_PARAM_SAMP_FREQ;
        break;

    case XA_CODEC_CONFIG_PARAM_CHANNELS:
    case XA_AMR_WB_DEC_CONFIG_PARAM_NUM_CHANNELS:
        /* ...return number of output channels */
        *pui_value = XA_AMR_WB_CONFIG_PARAM_NUM_CHANNELS;
        break;

    default:
        /* ...unrecognised command */
        TRACE(ERROR, _x("Invalid index: %X"), i_idx);
        return XA_API_FATAL_INVALID_CMD_TYPE;
    }

    return XA_NO_ERROR;
}

static XA_ERRORCODE xa_amr_wb_decoder_execute(XA_AMR_WB_Decoder *d, WORD32 i_idx, pVOID pv_value)
{
    int num_bytes; 
    pWORD16 enc_speech;
    xa_amr_wb_rx_frame_type_t frame_type;
    xa_amr_wb_mode_t mode;

    /* ...sanity check */
    XF_CHK_ERR(d, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...process individual command type */
    switch (i_idx)
    {
    case XA_CMD_TYPE_DO_EXECUTE:
        XF_CHK_ERR(d->input_avail > 0, XA_API_FATAL_INVALID_CMD_TYPE);
        enc_speech = d->input;
        num_bytes = read_serial(d->input, enc_speech, &frame_type, &mode);
        XF_CHK_ERR(num_bytes > 0, XA_API_FATAL_INVALID_CMD_TYPE);
        XF_CHK_ERR(num_bytes <= d->input_avail, XA_API_FATAL_INVALID_CMD_TYPE);

        {
            XA_ERRORCODE err_code;

            err_code = xa_amr_wb_dec((xa_codec_handle_t)(&d->internal_data), mode, frame_type, (pUWORD8)enc_speech, d->output);
            XF_CHK_ERR(err_code == XA_NO_ERROR, XA_API_FATAL_INVALID_CMD_TYPE);

            d->consumed = num_bytes;
            d->produced = (XA_AMR_WB_MAX_SAMPLES_PER_FRAME<<1);
            d->state |= XA_AMR_WB_FLAG_OUTPUT;
            if ((d->consumed == d->input_avail) && (d->state & XA_AMR_WB_FLAG_EOS_RECEIVED)) /* Signal done */
                d->state |= XA_AMR_WB_FLAG_COMPLETE;              

            return XA_NO_ERROR;
        }

    case XA_CMD_TYPE_DONE_QUERY:
        *(WORD32 *)pv_value = (d->state & XA_AMR_WB_FLAG_COMPLETE ? 1 : 0);
        return XA_NO_ERROR;
        
    case XA_CMD_TYPE_DO_RUNTIME_INIT:
        /* ...reset AMR dec component operation */
        return xa_amr_wb_decoder_init(d, i_idx, pv_value);
        
    default:
        /* ...unrecognised command */
        TRACE(ERROR, _x("Invalid index: %X"), i_idx);
        return XA_API_FATAL_INVALID_CMD_TYPE;
    }
}

static XA_ERRORCODE xa_amr_wb_decoder_set_input_bytes(XA_AMR_WB_Decoder *d, WORD32 i_idx, pVOID pv_value)
{
    UWORD32     size;

    /* ...sanity check */
    XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...track index must be valid */
    XF_CHK_ERR(i_idx == 0, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...return frame buffer minimal size only after post-initialization is done */
    XF_CHK_ERR(d->state & XA_AMR_WB_FLAG_POSTINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...input buffer must exist */
    XF_CHK_ERR(d->input, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...input frame length should not be zero (in bytes) */
    XF_CHK_ERR((size = (UWORD32)*(WORD32 *)pv_value) > 0, XA_AMR_WB_DEC_EXEC_NONFATAL_INPUT);

    /* ...all is correct; set input buffer length in bytes */
    d->input_avail = size;
    return XA_NO_ERROR;
}

static XA_ERRORCODE xa_amr_wb_decoder_get_output_bytes(XA_AMR_WB_Decoder *d, WORD32 i_idx, pVOID pv_value)
{
    /* ...sanity check */
    XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...track index must be valid */
    XF_CHK_ERR(i_idx == 1, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...return frame buffer minimal size only after post-initialization is done */
    XF_CHK_ERR(d->state & XA_AMR_WB_FLAG_RUNNING, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...output buffer must exist */
    XF_CHK_ERR(d->output, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...return number of produced bytes */
    *(WORD32 *)pv_value = d->produced;
    //d->produced = 0;
    return XA_NO_ERROR;
}

static XA_ERRORCODE xa_amr_wb_decoder_get_curidx_input_buf(XA_AMR_WB_Decoder *d, WORD32 i_idx, pVOID pv_value)
{
    /* ...sanity check */
    XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...track index must be valid */
    XF_CHK_ERR(i_idx == 0, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...input buffer must exist */
    XF_CHK_ERR(d->input, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...return number of bytes consumed */
    *(WORD32 *)pv_value = d->consumed;
    d->consumed = 0;
    return XA_NO_ERROR;
}

static XA_ERRORCODE xa_amr_wb_decoder_input_over(XA_AMR_WB_Decoder *d, WORD32 i_idx, pVOID pv_value)
{
    /* ...sanity check */
    XF_CHK_ERR(d, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...put end-of-stream flag */
    d->state |= XA_AMR_WB_FLAG_EOS_RECEIVED;
    return XA_NO_ERROR;
}

static XA_ERRORCODE xa_amr_wb_decoder_get_memtabs_size(XA_AMR_WB_Decoder *d, WORD32 i_idx, pVOID pv_value)
{
    /* ...sanity check */
    XF_CHK_ERR(d, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...check AMR dec component is pre-initialized */
    XF_CHK_ERR(d->state & XA_AMR_WB_FLAG_PREINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...we have all our tables inside API handle - good? tbd */
    *(WORD32 *)pv_value = 0;
    return XA_NO_ERROR;
}

static XA_ERRORCODE xa_amr_wb_decoder_set_memtabs_ptr(XA_AMR_WB_Decoder *d, WORD32 i_idx, pVOID pv_value)
{
    /* ...sanity check */
    XF_CHK_ERR(d, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...check AMR dec component is pre-initialized */
    XF_CHK_ERR(d->state & XA_AMR_WB_FLAG_PREINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...do not do anything; just return success - tbd */
    return XA_NO_ERROR;
}

static XA_ERRORCODE xa_amr_wb_decoder_get_n_memtabs(XA_AMR_WB_Decoder *d, WORD32 i_idx, pVOID pv_value)
{
    /* ...sanity check */
    XF_CHK_ERR(d, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...we have 1 input buffer, 1 output buffer and 1 scratch buffer*/
    *(WORD32 *)pv_value = 3;

    return XA_NO_ERROR;
}

static XA_ERRORCODE xa_amr_wb_decoder_get_mem_info_size(XA_AMR_WB_Decoder *d, WORD32 i_idx, pVOID pv_value)
{
    /* ...sanity check */
    XF_CHK_ERR(d, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...return frame buffer minimal size only after post-initialization is done */
    XF_CHK_ERR(d->state & XA_AMR_WB_FLAG_POSTINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

    switch (i_idx)
    {
    case 0:
        /* ...input buffers */
        *(WORD32 *)pv_value = (XA_AMR_WB_MAX_SPEECH_BITS_PER_FRAME<<1);
        return XA_NO_ERROR;

    case 1:
        /* ...output buffer */
        *(WORD32 *)pv_value = (XA_AMR_WB_MAX_SAMPLES_PER_FRAME<<1);
        return XA_NO_ERROR;

    case 2:
        /* ...scratch buffer */
        *(WORD32 *)pv_value = xa_amr_wb_dec_get_scratch_byte_size();
        return XA_NO_ERROR;

    default:
        /* ...invalid index */
        return XF_CHK_ERR(0, XA_API_FATAL_INVALID_CMD_TYPE);
    }

    return XA_NO_ERROR;
}

static XA_ERRORCODE xa_amr_wb_decoder_get_mem_info_alignment(XA_AMR_WB_Decoder *d, WORD32 i_idx, pVOID pv_value)
{
    /* ...sanity check */
    XF_CHK_ERR(d, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...return frame buffer minimal size only after post-initialization is done */
    XF_CHK_ERR(d->state & XA_AMR_WB_FLAG_POSTINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...all buffers are 4-bytes aligned */
    *(WORD32 *)pv_value = 4;
    return XA_NO_ERROR;
}

static XA_ERRORCODE xa_amr_wb_decoder_get_mem_info_type(XA_AMR_WB_Decoder *d, WORD32 i_idx, pVOID pv_value)
{
    /* ...sanity check */
    XF_CHK_ERR(d, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...return frame buffer minimal size only after post-initialization is done */
    XF_CHK_ERR(d->state & XA_AMR_WB_FLAG_POSTINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

    switch (i_idx)
    {
    case 0:
        /* ...input buffers */
        *(WORD32 *)pv_value = XA_MEMTYPE_INPUT;
        return XA_NO_ERROR;

    case 1:
        /* ...output buffer */
        *(WORD32 *)pv_value = XA_MEMTYPE_OUTPUT;
        return XA_NO_ERROR;

    case 2:
        /* ...scratch buffer */
        *(WORD32 *)pv_value = XA_MEMTYPE_SCRATCH;
        return XA_NO_ERROR;

    default:
        /* ...invalid index */
        return XF_CHK_ERR(0, XA_API_FATAL_INVALID_CMD_TYPE);
    }
}

static XA_ERRORCODE xa_amr_wb_decoder_set_mem_ptr(XA_AMR_WB_Decoder *d, WORD32 i_idx, pVOID pv_value)
{
    /* ...sanity check */
    XF_CHK_ERR(d, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...return frame buffer minimal size only after post-initialization is done */
    XF_CHK_ERR(d->state & XA_AMR_WB_FLAG_POSTINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...select memory buffer */
    switch (i_idx)
    {
    case 0:
        /* ...input buffers */
        d->input = pv_value;
        return XA_NO_ERROR;

    case 1:
        /* ...output buffer */
        d->output = pv_value;
        return XA_NO_ERROR;

    case 2:
        /* ...scratch buffer */
        d->scratch = pv_value;
        return XA_NO_ERROR;

    default:
        /* ...invalid index */
        return XF_CHK_ERR(0, XA_API_FATAL_INVALID_CMD_TYPE);
    }

    return XA_NO_ERROR;
}

/*******************************************************************************
 * API command hooks
 ******************************************************************************/

static XA_ERRORCODE (* const xa_amr_wb_decoder_api[])(XA_AMR_WB_Decoder *, WORD32, pVOID) =
{
    [XA_API_CMD_GET_API_SIZE]           = xa_amr_wb_decoder_get_api_size,

    [XA_API_CMD_INIT]                   = xa_amr_wb_decoder_init,
    [XA_API_CMD_SET_CONFIG_PARAM]       = xa_amr_wb_decoder_set_config_param,
    [XA_API_CMD_GET_CONFIG_PARAM]       = xa_amr_wb_decoder_get_config_param,

    [XA_API_CMD_EXECUTE]                = xa_amr_wb_decoder_execute,
    [XA_API_CMD_SET_INPUT_BYTES]        = xa_amr_wb_decoder_set_input_bytes,
    [XA_API_CMD_GET_OUTPUT_BYTES]       = xa_amr_wb_decoder_get_output_bytes,
    [XA_API_CMD_GET_CURIDX_INPUT_BUF]   = xa_amr_wb_decoder_get_curidx_input_buf,
    [XA_API_CMD_INPUT_OVER]             = xa_amr_wb_decoder_input_over,

    [XA_API_CMD_GET_MEMTABS_SIZE]       = xa_amr_wb_decoder_get_memtabs_size,
    [XA_API_CMD_SET_MEMTABS_PTR]        = xa_amr_wb_decoder_set_memtabs_ptr,
    [XA_API_CMD_GET_N_MEMTABS]          = xa_amr_wb_decoder_get_n_memtabs,
    [XA_API_CMD_GET_MEM_INFO_SIZE]      = xa_amr_wb_decoder_get_mem_info_size,
    [XA_API_CMD_GET_MEM_INFO_ALIGNMENT] = xa_amr_wb_decoder_get_mem_info_alignment,
    [XA_API_CMD_GET_MEM_INFO_TYPE]      = xa_amr_wb_decoder_get_mem_info_type,
    [XA_API_CMD_SET_MEM_PTR]            = xa_amr_wb_decoder_set_mem_ptr,
};

/* ...total numer of commands supported */
#define XA_AMR_WB_DEC_API_COMMANDS_NUM   (sizeof(xa_amr_wb_decoder_api) / sizeof(xa_amr_wb_decoder_api[0]))

/*******************************************************************************
 * API entry point
 ******************************************************************************/

XA_ERRORCODE xa_amr_wb_decoder(xa_codec_handle_t p_xa_module_obj, WORD32 i_cmd, WORD32 i_idx, pVOID pv_value)
{
    XA_ERRORCODE ret;
#ifdef XAF_PROFILE
    clk_t comp_start, comp_stop;
#endif
    XA_AMR_WB_Decoder    *d = (XA_AMR_WB_Decoder *) p_xa_module_obj;

    /* ...check if command index is sane */
    XF_CHK_ERR(i_cmd < XA_AMR_WB_DEC_API_COMMANDS_NUM, XA_API_FATAL_INVALID_CMD);

    /* ...see if command is defined */
    XF_CHK_ERR(xa_amr_wb_decoder_api[i_cmd], XA_API_FATAL_INVALID_CMD);

#ifdef XAF_PROFILE
    comp_start = clk_read_start(CLK_SELN_THREAD);
#endif
    
    /* ...execute requested command */
    ret = xa_amr_wb_decoder_api[i_cmd](d, i_idx, pv_value);

#ifdef XAF_PROFILE
    comp_stop = clk_read_stop(CLK_SELN_THREAD);
    dec_cycles += clk_diff(comp_stop, comp_start);
#endif
    
    return ret;
}
