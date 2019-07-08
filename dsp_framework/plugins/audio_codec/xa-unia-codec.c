/*******************************************************************************
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
 *
 *****************************************************************************/

#include "xf-types.h"
#include "xf-debug.h"
#include "mydefs.h"
#include "dpu_lib_load.h"
#include "dsp_codec_interface.h"
#include "xf-audio-apicmd.h"
/******************************************************************************
 * Internal functions definitions
 *****************************************************************************/

struct sCodecFun {
	UniACodecVersionInfo    VersionInfo;
	UniACodecCreate         Create;
	UniACodecDelete         Delete;
	UniACodecReset          Reset;
	UniACodecSetParameter   SetPara;
	UniACodecGetParameter   GetPara;
	UniACodec_decode_frame  Process;
	UniACodec_get_last_error    GetLastError;
};

/* ...API structure */
struct XFUniaCodec {
	/* ...codec identifier */
	u32 codec_id;

	/* ...codec input buffer pointer */
	u8 *inptr;

	/* ...codec output buffer pointer */
	u8 *outptr;

	/* ...codec input data size */
	u32 in_size;

	/* ...codec output data size */
	u32 out_size;

	/* ...codec consumed data size */
	u32 consumed;

	/* ...codec input over flag */
	u32 input_over;

	/* ...codec API entry point (function) */
	tUniACodecQueryInterface codecwrapinterface;

	/* ...codec API handle, passed to *process */
	void *codecinterface;

	/* ...dsp codec wrapper handle */
	UniACodec_Handle pWrpHdl;

	/* ...dsp codec wrapper api */
	struct sCodecFun WrapFun;

	/* ...private data pointer */
	void *private_data;

};

/******************************************************************************
 * Commands processing
 *****************************************************************************/

static UA_ERROR_TYPE xf_uniacodec_get_api_size(struct XFUniaCodec *d,
						u32 i_idx,
						void *pv_value)
{
	/* ...retrieve API structure size */
	*(u32 *)pv_value = sizeof(*d);

	return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xf_uniacodec_preinit(struct XFUniaCodec *d,
					   u32 i_idx,
					   void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	/* ...set codec id */
	d->codec_id = i_idx;

	/* ...set private data pointer */
	d->private_data = pv_value;

	return ret;
}

static UA_ERROR_TYPE xf_uniacodec_init(struct XFUniaCodec *d,
					u32 i_idx,
					void *pv_value)
{
	struct dsp_main_struct *dsp_config =
		(struct dsp_main_struct *)d->private_data;

	UniACodecMemoryOps  memops;
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	if (!d->codecwrapinterface) {
		LOG("base->codecwrapinterface Pointer is NULL\n");
		return ACODEC_INIT_ERR;
	}

	d->codecwrapinterface(ACODEC_API_CREATE_CODEC, (void **)&d->WrapFun.Create);
	d->codecwrapinterface(ACODEC_API_DEC_FRAME, (void **)&d->WrapFun.Process);
	d->codecwrapinterface(ACODEC_API_RESET_CODEC, (void **)&d->WrapFun.Reset);
	d->codecwrapinterface(ACODEC_API_DELETE_CODEC, (void **)&d->WrapFun.Delete);
	d->codecwrapinterface(ACODEC_API_GET_PARAMETER, (void **)&d->WrapFun.GetPara);
	d->codecwrapinterface(ACODEC_API_SET_PARAMETER, (void **)&d->WrapFun.SetPara);
	d->codecwrapinterface(ACODEC_API_GET_LAST_ERROR, (void **)&d->WrapFun.GetLastError);

	memops.Malloc = (void *)MEM_scratch_ua_malloc;
	memops.Free = (void *)MEM_scratch_ua_mfree;

	if (!d->WrapFun.Create) {
		LOG("WrapFun.Create Pointer is NULL\n");
		return ACODEC_INIT_ERR;
	}

	d->pWrpHdl = d->WrapFun.Create(&memops);
	if (!d->pWrpHdl) {
		LOG("Create codec error in codec wrapper\n");
		return ACODEC_INIT_ERR;
	}

	return ret;
}

static UA_ERROR_TYPE xf_uniacodec_postinit(struct XFUniaCodec *d,
					    u32 i_idx,
					    void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	return ret;
}

static UA_ERROR_TYPE xf_uniacodec_setparam(struct XFUniaCodec *d,
					    u32 i_idx,
					    void *pv_value)
{
	UniACodecParameter parameter;
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	if (!d->WrapFun.SetPara) {
		LOG("WrapFun.SetPara Pointer is NULL\n");
		return ACODEC_INIT_ERR;
	}
	switch (i_idx) {
	case UNIA_SAMPLERATE:
		parameter.samplerate = *(u32 *)pv_value;
		break;
	case UNIA_CHANNEL:
		parameter.channels = *(u32 *)pv_value;
		break;
	case UNIA_DEPTH:
		parameter.depth = *(u32 *)pv_value;
		break;
	case UNIA_DOWNMIX_STEREO:
		parameter.downmix = *(u32 *)pv_value;
		break;
	case UNIA_STREAM_TYPE:
		parameter.stream_type = *(u32 *)pv_value;
		break;
	case UNIA_BITRATE:
		parameter.bitrate = *(u32 *)pv_value;
		break;
	case UNIA_TO_STEREO:
		parameter.mono_to_stereo = *(u32 *)pv_value;
		break;
	case UNIA_FRAMED:
		parameter.framed = *(u32 *)pv_value;
		break;
	case UNIA_CODEC_ID:
		parameter.codec_id = *(u32 *)pv_value;
		break;
	case UNIA_CODEC_ENTRY_ADDR:
		parameter.codec_entry_addr = *(u32 *)pv_value;
		break;
#ifdef DEBUG
	case UNIA_FUNC_PRINT:
		parameter.Printf = NULL;
		break;
#endif
	case UNIA_CODEC_DATA:
		parameter.codecData.buf = NULL;
		parameter.codecData.size = *(u32 *)pv_value;
		break;
	default:
		break;
	}

	if ((d->codec_id == CODEC_MP2_DEC) ||
		(d->codec_id == CODEC_MP3_DEC)) {
		switch (i_idx) {
		/*****dedicate for mp3 dec and mp2 dec*****/
		case UNIA_MP3_DEC_CRC_CHECK:
			parameter.crc_check = *(u32 *)pv_value;
			break;
		case UNIA_MP3_DEC_MCH_ENABLE:
			parameter.mch_enable = *(u32 *)pv_value;
			break;
		case UNIA_MP3_DEC_NONSTD_STRM_SUPPORT:
			parameter.nonstd_strm_support = *(u32 *)pv_value;
			break;
		default:
			break;
		}
	} else if (d->codec_id == CODEC_BSAC_DEC) {
		switch (i_idx) {
		/*****dedicate for bsac dec***************/
		case UNIA_BSAC_DEC_DECODELAYERS:
			parameter.layers = *(u32 *)pv_value;
			break;
		default:
			break;
		}
	} else if ((d->codec_id == CODEC_AAC_DEC)) {
		switch (i_idx) {
		/*****dedicate for aacplus dec***********/
		case UNIA_CHANNEL:
			if (*(u32 *)pv_value > 2) {
				return ACODEC_PROFILE_NOT_SUPPORT;
			}
			break;
		case UNIA_AACPLUS_DEC_BDOWNSAMPLE:
			parameter.bdownsample = *(u32 *)pv_value;
			break;
		case UNIA_AACPLUS_DEC_BBITSTREAMDOWNMIX:
			parameter.bbitstreamdownmix = *(u32 *)pv_value;
			break;
		case UNIA_AACPLUS_DEC_CHANROUTING:
			parameter.chanrouting = *(u32 *)pv_value;
			break;
		default:
			break;
		}
	} else if (d->codec_id == CODEC_DAB_DEC) {
		switch (i_idx) {
		/*****************dedicate for dabplus dec******************/
		case UNIA_DABPLUS_DEC_BDOWNSAMPLE:
			parameter.bdownsample = *(u32 *)pv_value;
			break;
		case UNIA_DABPLUS_DEC_BBITSTREAMDOWNMIX:
			parameter.bbitstreamdownmix = *(u32 *)pv_value;
			break;
		case UNIA_DABPLUS_DEC_CHANROUTING:
			parameter.chanrouting = *(u32 *)pv_value;
			break;
		default:
			break;
		}
	} else if (d->codec_id == CODEC_SBC_ENC) {
		switch (i_idx) {
		/*******************dedicate for sbc enc******************/
		case UNIA_SBC_ENC_SUBBANDS:
			parameter.enc_subbands = *(u32 *)pv_value;
			break;
		case UNIA_SBC_ENC_BLOCKS:
			parameter.enc_blocks = *(u32 *)pv_value;
			break;
		case UNIA_SBC_ENC_SNR:
			parameter.enc_snr = *(u32 *)pv_value;
			break;
		case UNIA_SBC_ENC_BITPOOL:
			parameter.enc_bitpool = *(u32 *)pv_value;
			break;
		case UNIA_SBC_ENC_CHMODE:
			parameter.enc_chmode = *(u32 *)pv_value;
			break;
		default:
			break;
		}
	} else if (d->codec_id == CODEC_FSL_WMA_DEC) {
		switch (i_idx) {
		/*******************dedicate for wma dec******************/
		case UNIA_WMA_BlOCKALIGN:
			parameter.blockalign = *(u32 *)pv_value;
			break;
		case UNIA_WMA_VERSION:
			parameter.version = *(u32 *)pv_value;
			break;
		}
	}


	ret = d->WrapFun.SetPara(d->pWrpHdl, i_idx, &parameter);

	return ret;
}

static UA_ERROR_TYPE xf_uniacodec_getparam(struct XFUniaCodec *d,
					    u32 i_idx,
					    void *pv_value)
{
	UniACodecParameter param;
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	if (!d->WrapFun.GetPara) {
		LOG("WrapFun.GetPara Pointer is NULL\n");
		return ACODEC_INIT_ERR;
	}
	/* ...retrieve the collection of codec  parameters */
	ret = d->WrapFun.GetPara(d->pWrpHdl, i_idx, &param);
	if (ret)
		return ACODEC_PARA_ERROR;

	switch (i_idx) {
	case UNIA_SAMPLERATE:
		*(u32 *)pv_value = param.samplerate;
		break;
	case UNIA_CHANNEL:
		*(u32 *)pv_value = param.channels;
		break;
	case UNIA_OUTPUT_PCM_FORMAT:
		break;
	case UNIA_CODEC_DESCRIPTION:
		break;
	case UNIA_BITRATE:
		*(u32 *)pv_value = param.bitrate;
		break;
	case UNIA_CONSUMED_LENGTH:
		*(u32 *)pv_value = param.consumed_length;
		break;
	case UNIA_OUTBUF_ALLOC_SIZE:
		break;
	case UNIA_DEPTH:
		*(u32 *)pv_value = param.depth;
		break;
	case UA_TYPE_MAX:
		break;
	default:
		*(u32 *)pv_value = 0;
		break;
	}

	return ret;
}

static UA_ERROR_TYPE xf_uniacodec_execute(struct XFUniaCodec *d,
					   u32 i_idx,
					   void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;
	u32 offset = 0;

	if (!d->WrapFun.Process) {
		LOG("WrapFun.Process Pointer is NULL\n");
		return ACODEC_INIT_ERR;
	}

	LOG4("in_buf = %x, in_size = %x, offset = %d, out_buf = %x\n",
	     d->inptr, d->in_size, d->consumed, d->outptr);

	/* pass to ua wrapper input is over */
	if (d->input_over && d->in_size == 0 && d->codec_id >= CODEC_FSL_OGG_DEC)
		d->inptr = NULL;

	ret = d->WrapFun.Process(d->pWrpHdl,
				d->inptr,
				d->in_size,
				&offset,
				(u8 **)&d->outptr,
				&d->out_size);

	d->consumed = offset;
	LOG3("process: consumed = %d, produced = %d, ret = %d\n",
	     d->consumed, d->out_size, ret);
	return ret;
}

static UA_ERROR_TYPE xf_uniacodec_set_input_ptr(struct XFUniaCodec *d,
						 u32 i_idx,
						 void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	d->inptr = (u8 *)pv_value;

	return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xf_uniacodec_set_output_ptr(struct XFUniaCodec *d,
						  u32 i_idx,
						  void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	d->outptr = (u8 *)pv_value;

	return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xf_uniacodec_set_input_bytes(struct XFUniaCodec *d,
						   u32 i_idx,
						   void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	d->in_size = *(u32 *)pv_value;

	return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xf_uniacodec_get_output_bytes(struct XFUniaCodec *d,
						    u32 i_idx,
						    void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	*(u32 *)pv_value = d->out_size;

	return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xf_uniacodec_get_consumed_bytes(struct XFUniaCodec *d,
						      u32 i_idx,
						      void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	*(u32 *)pv_value = d->consumed;

	return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xf_uniacodec_input_over(struct XFUniaCodec *d,
					      u32 i_idx,
					      void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	d->input_over = 1;
	xf_uniacodec_setparam(d, UNIA_INPUT_OVER, NULL);

	return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xf_uniacodec_set_lib_entry(struct XFUniaCodec *d,
						 u32 i_idx,
						 void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	if (i_idx == DSP_CODEC_WRAP_LIB) {
		d->codecwrapinterface = (tUniACodecQueryInterface)pv_value;
	} else if (i_idx == DSP_CODEC_LIB) {
		d->codecinterface = pv_value;
	} else {
		LOG("Unknown lib type\n");
		ret = ACODEC_INIT_ERR;
	}

	return ret;
}

static UA_ERROR_TYPE xf_uniacodec_runtime_init(struct XFUniaCodec *d,
						u32 i_idx,
						void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	if (d->WrapFun.Reset)
		ret = d->WrapFun.Reset(d->pWrpHdl);

	d->input_over = 0;
	d->in_size = 0;
	d->consumed = 0;

	return ret;
}

static UA_ERROR_TYPE xf_uniacodec_cleanup(struct XFUniaCodec *d,
					   u32 i_idx,
					   void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	/* ...destory codec resources */
	if (d->WrapFun.Delete)
		ret = d->WrapFun.Delete(d->pWrpHdl);

	return ret;
}

/*******************************************************************************
 * API command hooks
 ******************************************************************************/

static UA_ERROR_TYPE (* const xf_unia_codec_api[])(struct XFUniaCodec *, u32, void *) = {
	[XF_API_CMD_GET_API_SIZE]      = xf_uniacodec_get_api_size,
	[XF_API_CMD_PRE_INIT]          = xf_uniacodec_preinit,
	[XF_API_CMD_INIT]              = xf_uniacodec_init,
	[XF_API_CMD_POST_INIT]         = xf_uniacodec_postinit,
	[XF_API_CMD_SET_PARAM]         = xf_uniacodec_setparam,
	[XF_API_CMD_GET_PARAM]         = xf_uniacodec_getparam,
	[XF_API_CMD_EXECUTE]           = xf_uniacodec_execute,
	[XF_API_CMD_SET_INPUT_PTR]     = xf_uniacodec_set_input_ptr,
	[XF_API_CMD_SET_OUTPUT_PTR]    = xf_uniacodec_set_output_ptr,
	[XF_API_CMD_SET_INPUT_BYTES]   = xf_uniacodec_set_input_bytes,
	[XF_API_CMD_GET_OUTPUT_BYTES]  = xf_uniacodec_get_output_bytes,
	[XF_API_CMD_GET_CONSUMED_BYTES]  = xf_uniacodec_get_consumed_bytes,
	[XF_API_CMD_INPUT_OVER]        = xf_uniacodec_input_over,
	[XF_API_CMD_RUNTIME_INIT]      = xf_uniacodec_runtime_init,
	[XF_API_CMD_CLEANUP]           = xf_uniacodec_cleanup,
	[XF_API_CMD_SET_LIB_ENTRY]     = xf_uniacodec_set_lib_entry,
};

/* ...total numer of commands supported */
#define XF_UNIACODEC_API_COMMANDS_NUM        \
	(sizeof(xf_unia_codec_api) / sizeof(xf_unia_codec_api[0]))

/*****************************************************************************
 * API entry point
 ****************************************************************************/

u32 xf_unia_codec(xf_codec_handle_t handle,
		  u32 i_cmd,
		  u32 i_idx,
		  void *pv_value)
{
	struct XFUniaCodec *uniacodec = (struct XFUniaCodec *)handle;

	/* ...check if command index is sane */
	XF_CHK_ERR(i_cmd < XF_UNIACODEC_API_COMMANDS_NUM, ACODEC_PARA_ERROR);

	/* ...see if command is defined */
	XF_CHK_ERR(xf_unia_codec_api[i_cmd] != NULL, ACODEC_PARA_ERROR);

	/* ...execute requested command */
	return xf_unia_codec_api[i_cmd](uniacodec, i_idx, pv_value);
}
