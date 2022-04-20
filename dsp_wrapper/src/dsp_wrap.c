/*****************************************************************
 * Copyright 2018-2020 NXP
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
 *
 *****************************************************************/

#include <string.h>
#include <stdbool.h>
#include "dsp_wrap.h"

#define WRAP_SHORT_NAME "DSP decoder Wrapper"
#define CODEC_VERSION_STR (dsp_decode_versionInfo())
#define WRAP_VERSION_STR \
	(WRAP_SHORT_NAME \
	" " "" \
	" " "build on" \
	" " __DATE__ " " __TIME__)

/*
 * UniACodecQueryInterface - Query a codec's interface
 *
 * Entry function for a core codec wrapper library. This is the first
 * function that is called after the codec shared library is opened
 * and returns all other API function pointers implemented by the core
 * audio codec wrapper such as creating/deleting the core codec.
 *
 * @id, the ID of the API function to query.
 * @func, the related API function pointer. If the related API is optional
 * and not implemented by the cored codec, set this value to NULL.
 *
 * Returns: ACODEC_SUCCESS for success and other error codes for error.
 */
WORD32 UniACodecQueryInterface(UWORD32 id, void **func)
{
	if (!func)
		return ACODEC_PARA_ERROR;

	switch (id) {
	case ACODEC_API_GET_VERSION_INFO:
		*func = (void *)DSPDecVersionInfo;
		break;

	case ACODEC_API_CREATE_CODEC_PLUS:
		*func = (void *)DSPDecCreate;
		break;

	case ACODEC_API_DELETE_CODEC:
		*func = (void *)DSPDecDelete;
		break;

	case ACODEC_API_RESET_CODEC:
		*func = (void *)DSPDecReset;
		break;

	case ACODEC_API_SET_PARAMETER:
		*func = (void *)DSPDecSetPara;
		break;

	case ACODEC_API_GET_PARAMETER:
		*func = (void *)DSPDecGetPara;
		break;

	case ACODEC_API_DEC_FRAME:
		*func = (void *)DSPDecFrameDecode;
		break;

	case ACODEC_API_ENC_FRAME:
		*func = (void *)DSPDecFrameDecode;
		break;

	case ACODEC_API_GET_LAST_ERROR:
		*func = (void *)DSPDecLastErr;
		break;

	default:
		*func = NULL;
		break;
	}

	return ACODEC_SUCCESS;
}

/*
 * DSPDecVersionInfo - get core decoder version info
 */
const char *DSPDecVersionInfo()
{
	return (const char *)WRAP_VERSION_STR;
}

static struct ChipInfo soc_info[2] = {
	{
		.code = MX8ULP,
		.name = "i.MX8ULP",
	},
	{
		.code = CC_OTHERS,
		.name = "CC_OTHERS",
	}
};

static enum ChipCode getChipCodeFromSocid(void)
{
	FILE *fp = NULL;
	char soc_name[100];
	enum ChipCode code = CC_UNKNOW;
	int num = sizeof(soc_info) / sizeof(struct ChipInfo);
	int i;

	fp = fopen("/sys/devices/soc0/soc_id", "r");
	if (fp == NULL) {
		fprintf(stderr, "open /sys/devices/soc0/soc_id failed.\n");
		return CC_UNKNOW;
	}

	if (fscanf(fp, "%100s", soc_name) != 1) {
		fprintf(stderr, "fscanf soc_id failed.\n");
		fclose(fp);
		return CC_UNKNOW;
	}
	fclose(fp);

	for(i=0; i<num; i++) {
		if(!strcmp(soc_name, soc_info[i].name)) {
			code = soc_info[i].code;
			break;
		}
	}

	return code;
}

#ifndef XA_DISABLE_EVENT
WORD32 app_event_handler(pVOID comp_ptr, UWORD32 config_param_id, pVOID config_buf_ptr, UWORD32 buf_size, UWORD32 comp_error_flag)
{
	int ret;
	xaf_comp_t *p_comp;
	xf_handle_t *p_handle;
	xf_user_msg_t msg;
	if (!comp_error_flag)
		return 1;
	UWORD32 decode_err = *(UWORD32 *)config_buf_ptr;

	p_comp = (xaf_comp_t *)comp_ptr;
	p_handle = &p_comp->handle;

	msg.opcode = XF_EVENT;
	msg.buffer = malloc(buf_size);
	memcpy(msg.buffer, config_buf_ptr, buf_size);

	p_handle->response(p_handle, &msg);

	return 0;
}
#endif

/* codec info */
static const CODECINFO codecinfo_factory[] = {
	{ CODEC_MP3_DEC,            "audio-decoder/mp3",            "lib_dsp_mp3_dec.so",     "lib_dsp_codec_wrap.so",          1 },
	{ CODEC_AAC_DEC,            "audio-decoder/aac",            "lib_dsp_aac_dec.so",     "lib_dsp_codec_wrap.so",          1 },
	{ CODEC_BSAC_DEC,           "audio-decoder/bsac",           "lib_dsp_bsac_dec.so",    "lib_dsp_codec_wrap.so",          1 },
	{ CODEC_DAB_DEC,            "audio-decoder/dabplus",        "lib_dsp_dabplus_dec.so", "lib_dsp_codec_wrap.so",          1 },
	{ CODEC_MP2_DEC,            "audio-decoder/mp2",            "lib_dsp_mp2_dec.so",     "lib_dsp_codec_wrap.so",          1 },
	{ CODEC_DRM_DEC,            "audio-decoder/drm",            "lib_dsp_drm_dec.so",     "lib_dsp_codec_wrap.so",          1 },
	{ CODEC_SBC_DEC,            "audio-decoder/sbc",            "lib_dsp_sbc_dec.so",     "lib_dsp_codec_wrap.so",          1 },
	{ CODEC_SBC_ENC,            "audio-encoder/sbc",            "lib_dsp_sbc_enc.so",     "lib_dsp_codec_wrap.so",          1 },
	{ CODEC_OPUS_DEC,           "audio-decoder/opus",           NULL,                     "lib_dsp_codec_opus_dec_wrap.so", 1 },
	{ CODEC_FSL_OGG_DEC,        "audio-decoder/fsl-ogg",        NULL,                     "lib_vorbisd_wrap_dsp.so",        0 },
	{ CODEC_FSL_MP3_DEC,        "audio-decoder/fsl-mp3",        NULL,                     "lib_mp3d_wrap_dsp.so",           0 },
	{ CODEC_FSL_AAC_DEC,        "audio-decoder/fsl-aac",        NULL,                     "lib_aacd_wrap_dsp.so",           0 },
	{ CODEC_FSL_AAC_PLUS_DEC,   "audio-decoder/fsl-aacplus",    NULL,                     "lib_aacd_wrap_dsp.so",           0 },
	{ CODEC_FSL_AC3_DEC,        "audio-decoder/fsl-ac3",        NULL,                     "lib_ac3d_wrap_dsp.so",           0 },
	{ CODEC_FSL_DDP_DEC,        "audio-decoder/fsl-ddp",        NULL,                     "lib_ddpd_wrap_dsp.so",           0 },
	{ CODEC_FSL_NBAMR_DEC,      "audio-decoder/fsl-nbamr",      NULL,                     "lib_nbamrd_wrap_dsp.so",         0 },
	{ CODEC_FSL_WBAMR_DEC,      "audio-decoder/fsl-wbamr",      NULL,                     "lib_wbamrd_wrap_dsp.so",         0 },
	{ CODEC_FSL_WMA_DEC,        "audio-decoder/fsl-wma",        NULL,                     "lib_wma10d_wrap_dsp.so",         0 },
	{ CODEC_PCM_DEC,            "audio-decoder/pcm",            NULL,                     NULL,                             0 },
};

#define CODECINFO_LEN (sizeof(codecinfo_factory) / sizeof(codecinfo_factory[0]))
/*
 * DSPDecCreate - DSP wrapper creation
 *
 * Create DSP handle and set initial parameter
 *
 * @memOps: memory operation callback table
 * @type: codec type
 *
 * Returns: UniACodec_Handle in case of success and NULL in case of failure
 */
UniACodec_Handle DSPDecCreate(UniACodecMemoryOps *memOps, AUDIOFORMAT type)
{
	struct DSP_Handle *pDSP_handle = NULL;
	UWORD32 frame_maxlen;
	int codec_type = 0;
	int err = 0;
	enum ChipCode chip_info;
	void *p_adev = NULL;
	xaf_adev_config_t *adev_config;
        xaf_comp_config_t comp_config;
	void *dec_inbuf[2];
	const char *dec_id;
	int  idx;

	printf("--- wrapper create ---\n");
	if (!memOps)
		return NULL;

	pDSP_handle =  memOps->Malloc(sizeof(struct DSP_Handle));
	if (!pDSP_handle) {
		fprintf(stderr, "memory allocation error for pDSP_handle\n");
		return NULL;
	}
	memset(pDSP_handle, 0, sizeof(struct DSP_Handle));

	memcpy(&pDSP_handle->sMemOps, memOps, sizeof(UniACodecMemoryOps));
	pDSP_handle->audio_type = type;

	frame_maxlen = INBUF_SIZE;
	pDSP_handle->outbuf_alloc_size = OUTBUF_SIZE;

	chip_info = getChipCodeFromSocid();

	switch (pDSP_handle->audio_type) {
	case MP2:
		codec_type = CODEC_MP2_DEC;
		break;
	case MP3:
		if (chip_info == MX8ULP) {
			codec_type = CODEC_MP3_DEC;
		} else {
			codec_type = CODEC_FSL_MP3_DEC;
		}
		break;
	case AAC:
	case AAC_PLUS:
		if (chip_info == MX8ULP) {
			codec_type = CODEC_AAC_DEC;
		} else {
			codec_type = CODEC_FSL_AAC_PLUS_DEC;
		}
		break;
	case DAB_PLUS:
		codec_type = CODEC_DAB_DEC;
		break;
	case BSAC:
		codec_type = CODEC_BSAC_DEC;
		break;
	case DRM:
		codec_type = CODEC_DRM_DEC;
		break;
	case SBCDEC:
		codec_type = CODEC_SBC_DEC;
		break;
	case SBCENC:
		codec_type = CODEC_SBC_ENC;
		break;
	case OGG:
		codec_type = CODEC_FSL_OGG_DEC;
		break;
	case AC3:
		codec_type = CODEC_FSL_AC3_DEC;
		break;
	case DD_PLUS:
		codec_type = CODEC_FSL_DDP_DEC;
		break;
	case NBAMR:
		codec_type = CODEC_FSL_NBAMR_DEC;
		break;
	case WBAMR:
		codec_type = CODEC_FSL_WBAMR_DEC;
		break;
	case WMA:
		codec_type = CODEC_FSL_WMA_DEC;
		break;
	default:
		fprintf(stderr, "DSP doesn't support this audio type!\n");
		goto Err2;
		break;
	}
	pDSP_handle->codec_type = codec_type;

	/* return err for bad perf on i.mx8ulp */
	if ((codec_type == CODEC_FSL_OGG_DEC ||
		codec_type == CODEC_FSL_AC3_DEC ||
		codec_type == CODEC_FSL_DDP_DEC ||
		codec_type == CODEC_FSL_NBAMR_DEC ||
		codec_type == CODEC_FSL_WBAMR_DEC ||
		codec_type == CODEC_FSL_WMA_DEC)
		&& chip_info == MX8ULP) {
		fprintf(stderr, "Stop decoding %d codec for bad performance\n", codec_type);
		goto Err2;
	}

	pDSP_handle->inner_buf.data = memOps->Malloc(INBUF_SIZE);
	if (!pDSP_handle->inner_buf.data) {
		fprintf(stderr, "memory allocation error for inner_buf.data\n");
		goto Err2;
	}

	pDSP_handle->inner_buf.buf_size = INBUF_SIZE;
	pDSP_handle->inner_buf.threshold = INBUF_SIZE;
	memset(pDSP_handle->inner_buf.data, 0, INBUF_SIZE);
	pDSP_handle->codecoffset = 0;
	pDSP_handle->codecdata_copy = false;

	for (idx = 0; idx < CODECINFO_LEN; idx++) {
		if (codecinfo_factory[idx].codec_type == codec_type) {
			pDSP_handle->codecdata_ignored = codecinfo_factory[idx].codecdata_ignored;
			dec_id = codecinfo_factory[idx].dec_id;
			break;
		}
	}

	adev_config = &pDSP_handle->adev_config;
	err = xaf_adev_config_default_init(adev_config);

	mem_init(adev_config);
	adev_config->pmem_malloc = mem_malloc;
	adev_config->pmem_free = mem_free;
#ifndef XA_DISABLE_EVENT
	adev_config->app_event_handler_cb = app_event_handler;
#endif
	err = xaf_adev_open((pVOID *)&pDSP_handle->p_adev, adev_config);
	if (err) {
		fprintf(stderr, "open dev error: %d\n", err);
		goto Err2;
	}
	p_adev = pDSP_handle->p_adev;

	/* ...create decoder p_comp */
	WORD32 comp_type = XAF_DECODER;
	/* ??? not use dec_info */
        xaf_comp_config_default_init(&comp_config);
	comp_config.comp_id = dec_id;
	comp_config.comp_type = comp_type;
	comp_config.num_input_buffers = 1;
	comp_config.num_output_buffers = 1;
	comp_config.pp_inbuf = (pVOID (*)[XAF_MAX_INBUFS])&dec_inbuf[0];
#ifndef XA_DISABLE_EVENT
	comp_config.error_channel_ctl = XAF_ERR_CHANNEL_ALL;
#endif
	err = xaf_comp_create((pVOID)p_adev, (pVOID *)&pDSP_handle->p_comp, &comp_config);
	if (err) {
		fprintf(stderr, "create comp error: %d\n", err);
		goto Err1;
	}

	/* ...load codec library */
	err = xaf_load_library(p_adev, pDSP_handle->p_comp, dec_id);
	if (err) {
		fprintf(stderr, "load lib error: %d\n", err);
		goto Err;
	}

	pDSP_handle->memory_allocated = false;
	pDSP_handle->comp_status = XAF_STARTING;

	fprintf(stdout, "Audio Device Ready\n");

	return (UniACodec_Handle)pDSP_handle;

Err:
	fprintf(stderr, "Create Decoder Failed, Please Check it!\n");
	xaf_comp_delete(pDSP_handle->p_comp);
Err1:
	xaf_adev_close(pDSP_handle->p_adev, XAF_ADEV_NORMAL_CLOSE);
Err2:
	if (pDSP_handle->inner_buf.data)
		pDSP_handle->sMemOps.Free(pDSP_handle->inner_buf.data);

	pDSP_handle->sMemOps.Free(pDSP_handle);

	return NULL;
}

/*
 * DSPDecDelete - DSP wrapper deletion
 *
 * Delete DSP handle and free memory
 *
 * @pua_handle: DSP handle to delete
 *
 * Returns: ACODEC_SUCCESS in case of success other codes in case of errors.
 */
UA_ERROR_TYPE DSPDecDelete(UniACodec_Handle pua_handle)
{
	struct DSP_Handle *pDSP_handle = (struct DSP_Handle *)pua_handle;
	xaf_adev_t *p_adev = pDSP_handle->p_adev;
	int err = 0;

	if (!pua_handle)
		return ACODEC_PARA_ERROR;

	xaf_comp_delete(pDSP_handle->p_comp);
	xaf_adev_close(pDSP_handle->p_adev, XAF_ADEV_NORMAL_CLOSE);
	mem_exit(p_adev->xf_g_ap->g_mem_obj);

	if (pDSP_handle->inner_buf.data) {
		pDSP_handle->sMemOps.Free(pDSP_handle->inner_buf.data);
		pDSP_handle->inner_buf.data = NULL;
	}

	pDSP_handle->sMemOps.Free(pDSP_handle);
	pua_handle = NULL;

	return ACODEC_SUCCESS;
}

/*
 * DSPDecReset - function to reset DSP codec wrapper
 * (e.g flushing internal buffer)
 *
 * @pua_handle: handle of DSP codec wrapper
 *
 * Returns: ACODEC_SUCCESS in case of success and other codes in case of error
 */
UA_ERROR_TYPE DSPDecReset(UniACodec_Handle pua_handle)
{
	struct DSP_Handle *pDSP_handle = (struct DSP_Handle *)pua_handle;
	int ret = ACODEC_SUCCESS;
	int param[2] = { 0 };

	param[0] = UNIA_RESET_BUF;
	ret = xaf_comp_set_config(pDSP_handle->p_comp, 1, &param[0]);
	if (ret) {
		printf("Reset DSP buf failed\n");
		goto Fail;
	}
	ResetInnerBuf(&pDSP_handle->inner_buf,
		      pDSP_handle->inner_buf.threshold,
		      pDSP_handle->inner_buf.threshold);

	pDSP_handle->offset_copy = 0;

Fail:
	return ret;
}

/*
 * DSPDecSetPara - set parameter to DSP wrapper according to the parameter type
 *
 * @pua_handle: handle of the DSP codec wrapper
 * @ParaType: parameter type to set
 * @parameter: pointer to parameter structure
 *
 * Returns: ACODEC_SUCCESS in case of success and other codes in case of error
 */
UA_ERROR_TYPE DSPDecSetPara(UniACodec_Handle pua_handle,
			    UA_ParaType ParaType,
			    UniACodecParameter *parameter) {
	struct DSP_Handle *pDSP_handle = (struct DSP_Handle *)pua_handle;
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;
	int param[2] = { 0 };
	int err = 0;
	int type;
	xf_buffer_t *b = NULL;
	int *p_buf = NULL;

	switch (ParaType) {
	case UNIA_SAMPLERATE:
		param[1] = parameter->samplerate;
		break;
	case UNIA_CHANNEL:
		if (parameter->channels == 0 || parameter->channels > 8)
			return ACODEC_PARA_ERROR;
		param[1] = parameter->channels;
		break;
	case UNIA_FRAMED:
		pDSP_handle->framed = parameter->framed;
		param[1] = parameter->framed;
		break;
	case UNIA_DEPTH:
		if (parameter->depth != 16 &&
		    parameter->depth != 24 &&
			parameter->depth != 32)
			return ACODEC_PARA_ERROR;
		param[1] = parameter->depth;
		pDSP_handle->depth_is_set = 1;
		break;
	case UNIA_CODEC_DATA:
		pDSP_handle->codecData = parameter->codecData;
		param[1] = parameter->codecData.size;
		break;
	case UNIA_DOWNMIX_STEREO:
		pDSP_handle->downmix = parameter->downmix;
		break;
	case UNIA_TO_STEREO:
		param[1] = parameter->mono_to_stereo;
		break;
	case UNIA_STREAM_TYPE:
		pDSP_handle->stream_type = parameter->stream_type;
		param[1] = parameter->stream_type;
		break;
	case UNIA_BITRATE:
		param[1] = parameter->bitrate;
		break;
	/*****dedicate for mp3 dec and mp2 dec*****/
	case UNIA_MP3_DEC_CRC_CHECK:
		param[1] = parameter->crc_check;
		break;
	case UNIA_MP3_DEC_MCH_ENABLE:
		param[1] = parameter->mch_enable;
		break;
	case UNIA_MP3_DEC_NONSTD_STRM_SUPPORT:
		param[1] = parameter->nonstd_strm_support;
		break;
	/*****dedicate for bsac dec***************/
	case UNIA_BSAC_DEC_DECODELAYERS:
		param[1] = parameter->layers;
		break;
	/*****dedicate for aacplus dec***********/
	case UNIA_AACPLUS_DEC_BDOWNSAMPLE:
		param[1] = parameter->bdownsample;
		break;
	case UNIA_AACPLUS_DEC_BBITSTREAMDOWNMIX:
		param[1] = parameter->bbitstreamdownmix;
		break;
	case UNIA_AACPLUS_DEC_CHANROUTING:
		param[1] = parameter->chanrouting;
		break;
	/*****************dedicate for dabplus dec******************/
	case UNIA_DABPLUS_DEC_BDOWNSAMPLE:
		param[1] = parameter->bdownsample;
		break;
	case UNIA_DABPLUS_DEC_BBITSTREAMDOWNMIX:
		param[1] = parameter->bbitstreamdownmix;
		break;
	case UNIA_DABPLUS_DEC_CHANROUTING:
		param[1] = parameter->chanrouting;
		break;
	/*******************dedicate for sbc enc******************/
	case UNIA_SBC_ENC_SUBBANDS:
		param[1] = parameter->enc_subbands;
		break;
	case UNIA_SBC_ENC_BLOCKS:
		param[1] = parameter->enc_blocks;
		break;
	case UNIA_SBC_ENC_SNR:
		param[1] = parameter->enc_snr;
		break;
	case UNIA_SBC_ENC_BITPOOL:
		param[1] = parameter->enc_bitpool;
		break;
	case UNIA_SBC_ENC_CHMODE:
		param[1] = parameter->enc_chmode;
		break;
	/*******************dedicate for wma dec******************/
	case UNIA_WMA_BlOCKALIGN:
		param[1] = parameter->blockalign;
		pDSP_handle->blockalign = parameter->blockalign;
		break;
	case UNIA_WMA_VERSION:
		param[1] = parameter->version;
		break;
	case UNIA_CHAN_MAP_TABLE:
	{
		int i;
		int *channel_tab;
		void *p_buf;
		CHAN_TABLE *channel_map;
		pDSP_handle->chan_map_tab = parameter->chan_map_tab;
		b = xf_buffer_get(pDSP_handle->p_adev->proxy.aux);
		if (!b)
			return ACODEC_INSUFFICIENT_MEM;
		channel_map = b->address;
		param[1] = (UWORD32)xf_proxy_b2a(&pDSP_handle->p_adev->proxy, channel_map);
		channel_map->size = parameter->chan_map_tab.size;
		p_buf = (void *)(channel_map + 1);
		for(i = 1; i < 10; i++) {
			channel_tab = parameter->chan_map_tab.channel_table[i];
			if (channel_tab) {
				channel_map->channel_table[i] = i;
				memcpy(p_buf, channel_tab, sizeof(int) * i);
				p_buf += i;
			} else
				channel_map->channel_table[i] = NULL;
		}
		break;
	}
	default:
		fprintf(stderr, "SetPara: para not support!\n");
		return ACODEC_SUCCESS;
	}

	param[0] = ParaType;
	err = xaf_comp_set_config(pDSP_handle->p_comp, 1, &param[0]);

#ifdef DEBUG
	fprintf(stdout, "SetPara: cmd = 0x%x, value = %d\n", ParaType, param[1]);
#endif
	if (b)
		xf_buffer_put(b);

	if (err)
		ret = ACODEC_INIT_ERR;
	return ret;
}

/*
 * DSPDecGetPara - get parameter from DSP wrapper
 *
 * @pua_handle: handle of the DSP codec wrapper
 * @ParaType: parameter type to get
 * @parameter: pointer to parameter structure
 *
 * Returns: ACODEC_SUCCESS in case of success and other codes in case of error
 */
UA_ERROR_TYPE DSPDecGetPara(UniACodec_Handle pua_handle,
			    UA_ParaType ParaType,
			    UniACodecParameter *parameter)
{
	struct DSP_Handle *pDSP_handle = (struct DSP_Handle *)pua_handle;
	int msg[2] = { 0 };
	int err = ACODEC_SUCCESS;

	switch (ParaType) {
	case UNIA_SAMPLERATE:
		msg[0] = UNIA_SAMPLERATE;
		err = xaf_comp_get_config(pDSP_handle->p_comp, 1, &msg[0]);
		parameter->samplerate = msg[1];
		break;
	case UNIA_CHANNEL:
		msg[0] = UNIA_CHANNEL;
		err = xaf_comp_get_config(pDSP_handle->p_comp, 1, &msg[0]);
		parameter->channels = msg[1];
		break;
	case UNIA_DEPTH:
		msg[0] = UNIA_DEPTH;
		err = xaf_comp_get_config(pDSP_handle->p_comp, 1, &msg[0]);
		parameter->depth = msg[1];
		break;
	case UNIA_BITRATE:
		msg[0] = UNIA_BITRATE;
		err = xaf_comp_get_config(pDSP_handle->p_comp, 1, &msg[0]);
		parameter->bitrate = msg[1];
		break;
	case UNIA_CONSUMED_LENGTH:
		msg[0] = UNIA_CONSUMED_LENGTH;
		err = xaf_comp_get_config(pDSP_handle->p_comp, 1, &msg[0]);
		parameter->consumed_length = msg[1];
		break;
	case UNIA_OUTPUT_PCM_FORMAT:
		{
#if 0
			UniAcodecOutputPCMFormat outputFormat;
			outputFormat.samplerate = pDSP_handle->samplerate;
			/* ??? differ width with depth */
			outputFormat.width = pDSP_handle->depth;
			outputFormat.depth = pDSP_handle->depth;
			outputFormat.channels = pDSP_handle->channels;
			outputFormat.endian = 0;
			outputFormat.interleave = 0;
			memcpy(&outputFormat.layout[0], &pDSP_handle->layout_bak[0], UA_CHANNEL_MAX);
			outputFormat.chan_pos_set = 1;
			parameter->outputFormat = outputFormat;
#endif
			parameter->outputFormat = pDSP_handle->outputFormat;
		}
		break;
	case UNIA_CODEC_DESCRIPTION:
		pDSP_handle->codcDesc = "dsp codec version";
		parameter->codecDesc = &pDSP_handle->codcDesc;
		break;
	case UNIA_OUTBUF_ALLOC_SIZE:
		switch (pDSP_handle->audio_type) {
		case MP2:
		case MP3:
			parameter->outbuf_alloc_size = 16384;
			break;
		case AAC:
		case AAC_PLUS:
			parameter->outbuf_alloc_size = 18432;
			break;
		case AC3:
			parameter->outbuf_alloc_size = 1536*sizeof(long)*6;
			break;
		case DD_PLUS:
			parameter->outbuf_alloc_size = 36864;
			break;
		case NBAMR:
			parameter->outbuf_alloc_size = 240*sizeof(short);
			break;
		case WBAMR:
			parameter->outbuf_alloc_size = 320*sizeof(short);
			break;
		case WMA:
			parameter->outbuf_alloc_size = 8192*3*8*2;
			break;
		default:
			parameter->outbuf_alloc_size = 16384;
			break;
		}
		break;
	default:
		break;
	}
#ifdef DEBUG
	fprintf(stdout, "Get parameter: cmd = 0x%x\n", ParaType);
#endif
	if (err)
		return ACODEC_PARA_ERROR;

	return ACODEC_SUCCESS;
}

/*
 * DSPDecFrameDecode: function to decode a frame
 *
 * @pua_handle (in)    : handle of DSP codec wrapper
 * @InputBuf   (in)    : input buffer of bitstream, if set to NULL decoder will
 *                       decode all data buffer within the wrapper
 * @InputSize  (in)    : input buffer size
 * @offset     (in/out): offset of the input buffer
 * @OutputBuf  (in/out): decoder output buffer, if set to NULL
 *                       decoder wrapper will malloc buffer inside the decoder
 * @OutputSize (in/out): decoder output buffer size
 */
UA_ERROR_TYPE DSPDecFrameDecode(UniACodec_Handle pua_handle,
				UWORD8 *InputBuf,
				UWORD32 InputSize,
				UWORD32 *offset,
				UWORD8 **OutputBuf,
				UWORD32 *OutputSize)
{
	int err = ACODEC_SUCCESS;
	struct DSP_Handle *pDSP_handle = (struct DSP_Handle *)pua_handle;
	int param[6];
	UWORD8 *inbuf_data;
	UWORD32 *inner_offset, *inner_size;
	UWORD8 *pIn = NULL, *pOut = NULL;
	bool buf_from_out = false;
	UWORD32 *channel_map = NULL;
	UWORD32 in_size = 0, in_off = 0, out_size = 0;
	unsigned int *codecoffset = &pDSP_handle->codecoffset;
	int *offset_copy = &pDSP_handle->offset_copy;

	if (*OutputBuf)
		buf_from_out = true;

	if (pDSP_handle->stream_type == STREAM_ADIF)
		pDSP_handle->framed = 0;
	if (pDSP_handle->audio_type == NBAMR)
		pDSP_handle->framed = 1;

	if (!pDSP_handle->codecData.buf)
		pDSP_handle->codecdata_ignored = true;
#ifdef DEBUG
	fprintf(stdout, "InputSize = %d, offset = %d\n", InputSize, *offset);
#endif
	if (pDSP_handle->codecData.buf && (pDSP_handle->codecdata_copy == false)
			&& (pDSP_handle->codecdata_ignored == false)) {
		UWORD32 need_copy = pDSP_handle->codecData.size - *codecoffset;
		UWORD32 actual_copy = (need_copy > INBUF_SIZE) ? INBUF_SIZE : need_copy;
		int offset = 0;
		InputBufHandle(&pDSP_handle->inner_buf,
					pDSP_handle->codecData.buf + *codecoffset,
					actual_copy,
					&offset,
					0);
		*codecoffset += offset;
		printf("act copy %d, offset %d, codec offset %d\n", actual_copy, offset, *codecoffset);
	}

	if ((pDSP_handle->codecdata_copy == true) || pDSP_handle->codecdata_ignored == true) {
		if (!pDSP_handle->inptr_busy) {
			err = InputBufHandle(&pDSP_handle->inner_buf,
						 InputBuf,
						 InputSize,
						 offset_copy,
						 pDSP_handle->framed);
			if (err != ACODEC_SUCCESS) {
				*offset = *offset_copy;
				*offset_copy = 0;
				return err;
			}
		}
	}

	if (pDSP_handle->codecData.size <= *codecoffset)
		pDSP_handle->codecdata_copy = true;

	inbuf_data = pDSP_handle->inner_buf.data;
	inner_offset = &pDSP_handle->inner_buf.inner_offset;
	inner_size = &pDSP_handle->inner_buf.inner_size;

	if (pDSP_handle->codec_type != CODEC_FSL_MP3_DEC) {
		if (!pDSP_handle->ID3flag && !memcmp(inbuf_data +
						(*inner_offset), "ID3", 3)) {
			UWORD8 *pBuff = inbuf_data + (*inner_offset);

			pDSP_handle->tagsize = (pBuff[6] << 21) |
				(pBuff[7] << 14) | (pBuff[8] << 7) | pBuff[9];
			pDSP_handle->tagsize += 10;
			pDSP_handle->ID3flag = true;
		}
		if (pDSP_handle->ID3flag) {
			if (*inner_size >= pDSP_handle->tagsize) {
				pDSP_handle->ID3flag = false;
				*inner_offset += pDSP_handle->tagsize;
				*inner_size -= pDSP_handle->tagsize;
				pDSP_handle->consumed_length += pDSP_handle->tagsize;
				pDSP_handle->tagsize = 0;

				memmove(inbuf_data,
					inbuf_data + (*inner_offset), *inner_size);
				*inner_offset = 0;
			} else {
				pDSP_handle->tagsize -= *inner_size;
				*inner_offset += *inner_size;
				pDSP_handle->consumed_length += *inner_size;
				*inner_size = 0;
				return ACODEC_NOT_ENOUGH_DATA;
			}
		}
	}

	if (pDSP_handle->memory_allocated == false) {
		/* Set the default function of dsp codec */
		err = SetDefaultFeature(pua_handle);
		if (err) {
			fprintf(stderr, "dsp open error, please check it!\n");
			return ACODEC_INIT_ERR;
		}
		pDSP_handle->memory_allocated = true;
	}

	if (!(*OutputBuf)) {
		pDSP_handle->dsp_out_buf =
		  pDSP_handle->sMemOps.Malloc(pDSP_handle->outbuf_alloc_size);
		if (!pDSP_handle->dsp_out_buf) {
			fprintf(stderr,"memory allocation error for output buffer\n");
			return ACODEC_INSUFFICIENT_MEM;
		}
		memset(pDSP_handle->dsp_out_buf,
		       0, pDSP_handle->outbuf_alloc_size);
		*OutputBuf = pDSP_handle->dsp_out_buf;
	}

	pOut = *OutputBuf;
	pIn = inbuf_data + *inner_offset;
	in_size = *inner_size;
	in_off = 0;

#ifdef DEBUG
	fprintf(stdout,"inner buffer data size = %d, inner buffer offset = %d\n",
	      in_size, *inner_offset);
#endif
	err = comp_process(pDSP_handle, pIn, in_size, &in_off, pOut, &out_size);

	*inner_size -= in_off;
	*inner_offset += in_off;

	if (pDSP_handle->inptr_busy == false && (*offset_copy >= InputSize) || err == ACODEC_ERROR_STREAM) {
		*offset = *offset_copy;
		*offset_copy = 0;
	}

	if (buf_from_out)
		memcpy(*OutputBuf, pOut, out_size);
	*OutputSize = out_size;

	pDSP_handle->last_output_size = out_size;

	if (err == ACODEC_SUCCESS) {
#ifdef DEBUG
		fprintf(stdout, "NO_ERROR: consumed length = %d, output size = %d\n",
		      in_size, out_size);
#endif
		if (pDSP_handle->codec_type < CODEC_FSL_OGG_DEC) {
			if ((pDSP_handle->channels == 1) &&
				((pDSP_handle->audio_type == AAC)      ||
				(pDSP_handle->audio_type == AAC_PLUS) ||
				(pDSP_handle->audio_type == BSAC)     ||
				(pDSP_handle->audio_type == DAB_PLUS))) {
				cancel_unused_channel_data(*OutputBuf,
						*OutputSize, pDSP_handle->depth);
				*OutputSize >>= 1;
			}
		}

		param[0] = UNIA_SAMPLERATE;
		param[2] = UNIA_CHANNEL;
		param[4] = UNIA_DEPTH;

		xaf_comp_get_config(pDSP_handle->p_comp, 3, &param[0]);

		pDSP_handle->samplerate = param[1];
		pDSP_handle->channels = param[3];
		pDSP_handle->depth = param[5];

		if ((pDSP_handle->codec_type != CODEC_SBC_ENC) &&
				(pDSP_handle->chan_map_tab.size != 0) &&
				(pDSP_handle->outputFormat.chan_pos_set == false) &&
				(pDSP_handle->channels <= pDSP_handle->chan_map_tab.size)) {
			int channel;
			channel = pDSP_handle->channels;
			channel_map = pDSP_handle->chan_map_tab.channel_table[channel];
			if (channel_map) {
				memcpy(pDSP_handle->outputFormat.layout,
				       channel_map, sizeof(UWORD32) * channel);
				pDSP_handle->outputFormat.chan_pos_set = true;
			}
		}

		if ((pDSP_handle->codec_type != CODEC_SBC_ENC) &&
				((pDSP_handle->outputFormat.samplerate != pDSP_handle->samplerate) ||
				(pDSP_handle->outputFormat.channels != pDSP_handle->channels) ||
				(memcmp(pDSP_handle->outputFormat.layout, pDSP_handle->layout_bak,
					sizeof(UWORD32) * pDSP_handle->channels))) &&
				(pDSP_handle->channels != 0)) {
#ifdef DEBUG
			printf("output format changed\n");
#endif
			pDSP_handle->outputFormat.width = pDSP_handle->depth;
			pDSP_handle->outputFormat.depth = pDSP_handle->depth;
			pDSP_handle->outputFormat.channels = pDSP_handle->channels;
			pDSP_handle->outputFormat.interleave = true;
			pDSP_handle->outputFormat.samplerate = pDSP_handle->samplerate;

#if 0
			if ((pDSP_handle->channels > 2)  && (pDSP_handle->channels <= 8) &&
					(!pDSP_handle->outputFormat.chan_pos_set)) {
				if (aacd_channel_layouts[pDSP_handle->channels])
					memcpy(pDSP_handle->outputFormat.layout,
					       aacd_channel_layouts[pDSP_handle->channels],
						   sizeof(UWORD32) * pDSP_handle->channels);
			}
#endif
			if (pDSP_handle->channels == 2) {
				pDSP_handle->outputFormat.layout[0] = UA_CHANNEL_FRONT_LEFT;
				pDSP_handle->outputFormat.layout[1] = UA_CHANNEL_FRONT_RIGHT;
			}

			memcpy(pDSP_handle->layout_bak, pDSP_handle->outputFormat.layout,
				sizeof(UWORD32) * pDSP_handle->channels);

#if 0
			if ((*OutputSize > 0) && (pDSP_handle->channels > 0))
				channel_pos_convert(pDSP_handle,
						    (uint8 *)(*OutputBuf),
						    *OutputSize,
						    pDSP_handle->channels,
						    pDSP_handle->depth);
#endif
			err = ACODEC_CAPIBILITY_CHANGE;
		}
		if (!out_size)
			err |= ACODEC_NO_OUTPUT;

		return err;
	}

#ifdef DEBUG
	fprintf(stderr, "HAS_ERROR: err = 0x%x\n", (int)err);
#endif

	if (!(out_size)) {
		if ((*OutputBuf) && (buf_from_out == false)) {
			pDSP_handle->sMemOps.Free(*OutputBuf);
			pDSP_handle->dsp_out_buf = NULL;
			*OutputBuf = NULL;
		}
	}


	return err;
}

char *DSPDecLastErr(UniACodec_Handle pua_handle)
{
	struct DSP_Handle *pDSP_handle = (struct DSP_Handle *)pua_handle;
	WORD32 last_err = pDSP_handle->last_err;
	struct lastErr *errors = NULL;

	return NULL;
}
