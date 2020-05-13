/*****************************************************************
 * Copyright 2018 NXP
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
#include "fsl_unia.h"
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
int32 UniACodecQueryInterface(uint32 id, void **func)
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
	uint32 frame_maxlen;
	int comp_type = 0;
	int err = 0;

	if (!memOps)
		return NULL;

	pDSP_handle =  memOps->Malloc(sizeof(struct DSP_Handle));
	if (!pDSP_handle) {
#ifdef DEBUG
		TRACE("memory allocation error for pDSP_handle\n");
#endif
		return NULL;
	}
	memset(pDSP_handle, 0, sizeof(struct DSP_Handle));

	memcpy(&pDSP_handle->sMemOps, memOps, sizeof(UniACodecMemoryOps));
	pDSP_handle->codec_type = type;

	frame_maxlen = INBUF_SIZE;
	pDSP_handle->outbuf_alloc_size = OUTBUF_SIZE;

	switch (pDSP_handle->codec_type) {
	case MP2:
		comp_type = CODEC_MP2_DEC;
		break;
	case MP3:
		comp_type = CODEC_FSL_MP3_DEC;
		break;
	case AAC:
		comp_type = CODEC_FSL_AAC_DEC;
		break;
	case AAC_PLUS:
		comp_type = CODEC_AAC_DEC;
		break;
	case DAB_PLUS:
		comp_type = CODEC_DAB_DEC;
		break;
	case BSAC:
		comp_type = CODEC_BSAC_DEC;
		break;
	case DRM:
		comp_type = CODEC_DRM_DEC;
		break;
	case SBCDEC:
		comp_type = CODEC_SBC_DEC;
		break;
	case SBCENC:
		comp_type = CODEC_SBC_ENC;
		break;
	case OGG:
		comp_type = CODEC_FSL_OGG_DEC;
		break;
	case AC3:
		comp_type = CODEC_FSL_AC3_DEC;
		break;
	case DD_PLUS:
		comp_type = CODEC_FSL_DDP_DEC;
		break;
	case NBAMR:
		comp_type = CODEC_FSL_NBAMR_DEC;
		break;
	case WBAMR:
		comp_type = CODEC_FSL_WBAMR_DEC;
		break;
	case WMA:
		comp_type = CODEC_FSL_WMA_DEC;
		break;
	default:
#ifdef DEBUG
		TRACE("DSP doesn't support this audio type!\n");
#endif
		goto Err2;
		break;
	}

	pDSP_handle->inner_buf.data = memOps->Malloc(INBUF_SIZE);
	if (!pDSP_handle->inner_buf.data) {
#ifdef DEBUG
		TRACE("memory allocation error for inner_buf.data\n");
#endif
		goto Err2;
	}

	pDSP_handle->inner_buf.buf_size = INBUF_SIZE;
	pDSP_handle->inner_buf.threshold = INBUF_SIZE;
	memset(pDSP_handle->inner_buf.data, 0, INBUF_SIZE);
	pDSP_handle->codecoffset = 0;
	pDSP_handle->codecdata_copy = FALSE;

	/* ...open DSP proxy - specify "DSP#0" */
	err = xaf_adev_open(&pDSP_handle->adev);
	if (err) {
#ifdef DEBUG
		printf("open proxy error, err = %d\n", err);
#endif
		goto Err1;
	}

	/* ...create pipeline */
	err = xaf_pipeline_create(&pDSP_handle->adev, &pDSP_handle->pipeline);
	if (err) {
#ifdef DEBUG
		printf("create pipeline error, err = %d\n", err);
#endif
		goto Err1;
	}

    /* ...create component */
	err = xaf_comp_create(&pDSP_handle->adev,
			      &pDSP_handle->component,
			      comp_type);
	if (err) {
#ifdef DEBUG
		printf("create component failed, type = %d, err = %d\n",
		       comp_type, err);
#endif
		goto Err;
	}

	pDSP_handle->memory_allocated = FALSE;

#ifdef DEBUG
	TRACE("inner_buf threshold = %d, inner out_buf size = %d\n",
	      frame_maxlen, pDSP_handle->outbuf_alloc_size);
#endif

	return (UniACodec_Handle)pDSP_handle;

Err:
#ifdef DEBUG
	TRACE("Create Decoder Failed, Please Check it!\n");
#endif
	xaf_comp_delete(&pDSP_handle->component);
Err1:
	xaf_pipeline_delete(&pDSP_handle->pipeline);
	xaf_adev_close(&pDSP_handle->adev);
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
	int err = 0;

	if (!pua_handle)
		return ACODEC_PARA_ERROR;

	xaf_comp_delete(&pDSP_handle->component);
	xaf_pipeline_delete(&pDSP_handle->pipeline);
	xaf_adev_close(&pDSP_handle->adev);

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
	int count, i;
	struct xaf_info_s p_info;

	/*
	 * The XF_FLUSH command is used to reset codec buffers and
	 * its related parameters in dsp firmware. However, the related
	 * buffers of dsp core lib have not been allocated before dsp open.
	 * So don't reset codec before dsp open.
	 */
	if (pDSP_handle->memory_allocated) {
		ret = xaf_comp_flush(&pDSP_handle->component);
		if (ret) {
#ifdef DEBUG
			TRACE("Reset DSP Failed, ret = 0x%x\n", ret);
#endif
			goto Fail;
		}

		/* when flush component, empty the message pipe */
		count = xaf_comp_get_msg_count(&pDSP_handle->component);
		if (count)
			for (i = 0; i < count; i++)
				xaf_comp_get_status(&pDSP_handle->component,
						    &p_info);

		/* set input and output buffer available */
		pDSP_handle->inptr_busy = FALSE;
		pDSP_handle->outptr_busy = FALSE;
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
	struct xf_set_param_msg param = {0, 0};
	int err = 0;
	int type;

	switch (ParaType) {
	case UNIA_SAMPLERATE:
		param.mixData.value = parameter->samplerate;
		break;
	case UNIA_CHANNEL:
		if (parameter->channels == 0 || parameter->channels > 8)
			return ACODEC_PARA_ERROR;
		param.mixData.value = parameter->channels;
		break;
	case UNIA_FRAMED:
		pDSP_handle->framed = parameter->framed;
		param.mixData.value = parameter->framed;
		break;
	case UNIA_DEPTH:
		if (parameter->depth != 16 &&
		    parameter->depth != 24 &&
			parameter->depth != 32)
			return ACODEC_PARA_ERROR;

		param.mixData.value = parameter->depth;
		pDSP_handle->depth_is_set = 1;
		break;
	case UNIA_CODEC_DATA:
		pDSP_handle->codecData = parameter->codecData;
		param.mixData.value = parameter->codecData.size;
		break;
	case UNIA_DOWNMIX_STEREO:
		pDSP_handle->downmix = parameter->downmix;
		break;
	case UNIA_TO_STEREO:
		param.mixData.value = parameter->mono_to_stereo;
		break;
	case UNIA_STREAM_TYPE:
		pDSP_handle->stream_type = parameter->stream_type;
		param.mixData.value = parameter->stream_type;
		break;
	case UNIA_BITRATE:
		param.mixData.value = parameter->bitrate;
		break;
	case UNIA_OUTPUT_PCM_FORMAT:
		pDSP_handle->outputFormat = parameter->outputFormat;
		break;
	case UNIA_CHAN_MAP_TABLE:
		pDSP_handle->chan_map_tab = parameter->chan_map_tab;
		param.mixData.chan_map_tab.size = parameter->chan_map_tab.size;
		{
			int i;
			u32 *channel_map;
			u32 *dest;
			u32 dest_phy;
			dest = pDSP_handle->component.paramptr;
			for(i = 1; i < 10; i++) {
				channel_map = parameter->chan_map_tab.channel_table[i];
				if (channel_map) {
					memcpy(dest, channel_map, sizeof(uint32) * i);
					dest_phy =  (int)(intptr_t)xf_proxy_b2a(&pDSP_handle->adev.proxy, dest);
					param.mixData.chan_map_tab.channel_table[i] = dest_phy;
					dest += i;
				}
			}
		}
		break;
	default:
		break;
	}

	if ((pDSP_handle->codec_type == MP2) ||
	    (pDSP_handle->codec_type == MP3)) {
		switch (ParaType) {
		/*****dedicate for mp3 dec and mp2 dec*****/
		case UNIA_MP3_DEC_CRC_CHECK:
			param.mixData.value = parameter->crc_check;
			break;
		case UNIA_MP3_DEC_MCH_ENABLE:
			param.mixData.value = parameter->mch_enable;
			break;
		case UNIA_MP3_DEC_NONSTD_STRM_SUPPORT:
			param.mixData.value = parameter->nonstd_strm_support;
			break;
		default:
			break;
		}
	} else if (pDSP_handle->codec_type == BSAC) {
		switch (ParaType) {
		/*****dedicate for bsac dec***************/
		case UNIA_BSAC_DEC_DECODELAYERS:
			param.mixData.value = parameter->layers;
			break;
		default:
			break;
		}
	} else if (pDSP_handle->codec_type == AAC) {
		switch (ParaType) {
		case UNIA_CHAN_MAP_TABLE:
			ParaType = -1;
			break;
		}
	} else if(pDSP_handle->codec_type == AAC_PLUS) {
		switch (ParaType) {
		/*****dedicate for aacplus dec***********/
		case UNIA_CHANNEL:
			if (param.mixData.value > 2) {
#ifdef DEBUG
				TRACE("Error: multi-channnel decoding doesn't support for AACPLUS\n");
#endif
				return ACODEC_PROFILE_NOT_SUPPORT;
			}
			break;
		case UNIA_AACPLUS_DEC_BDOWNSAMPLE:
			param.mixData.value = parameter->bdownsample;
			break;
		case UNIA_AACPLUS_DEC_BBITSTREAMDOWNMIX:
			param.mixData.value = parameter->bbitstreamdownmix;
			break;
		case UNIA_AACPLUS_DEC_CHANROUTING:
			param.mixData.value = parameter->chanrouting;
			break;
		default:
			break;
		}
	} else if (pDSP_handle->codec_type == DAB_PLUS) {
		switch (ParaType) {
		/*****************dedicate for dabplus dec******************/
		case UNIA_DABPLUS_DEC_BDOWNSAMPLE:
			param.mixData.value = parameter->bdownsample;
			break;
		case UNIA_DABPLUS_DEC_BBITSTREAMDOWNMIX:
			param.mixData.value = parameter->bbitstreamdownmix;
			break;
		case UNIA_DABPLUS_DEC_CHANROUTING:
			param.mixData.value = parameter->chanrouting;
			break;
		default:
			break;
		}
	} else if (pDSP_handle->codec_type == SBCENC) {
		switch (ParaType) {
		/*******************dedicate for sbc enc******************/
		case UNIA_SBC_ENC_SUBBANDS:
			param.mixData.value = parameter->enc_subbands;
			break;
		case UNIA_SBC_ENC_BLOCKS:
			param.mixData.value = parameter->enc_blocks;
			break;
		case UNIA_SBC_ENC_SNR:
			param.mixData.value = parameter->enc_snr;
			break;
		case UNIA_SBC_ENC_BITPOOL:
			param.mixData.value = parameter->enc_bitpool;
			break;
		case UNIA_SBC_ENC_CHMODE:
			param.mixData.value = parameter->enc_chmode;
			break;
		default:
			break;
		}
	} else if (pDSP_handle->codec_type == OGG || pDSP_handle->codec_type == DD_PLUS
			|| pDSP_handle->codec_type == AC3) {
		switch (ParaType) {
		case UNIA_OUTPUT_PCM_FORMAT:
			ParaType = -1;
			break;
		}
	} else if (pDSP_handle->codec_type == WMA) {
		switch (ParaType) {
		case UNIA_OUTPUT_PCM_FORMAT:
		case UNIA_CHAN_MAP_TABLE:
			ParaType = -1;
			break;
		case UNIA_CODEC_DATA:
			param.mixData.value = parameter->codecData.size;
			break;
		case UNIA_WMA_BlOCKALIGN:
			param.mixData.value = parameter->blockalign;
			pDSP_handle->blockalign = parameter->blockalign;
			break;
		case UNIA_WMA_VERSION:
			param.mixData.value = parameter->version;
			break;
		}
	}

	param.id = ParaType;
	err = xaf_comp_set_config(&pDSP_handle->component, 1, &param);

#ifdef DEBUG
	TRACE("SetPara: cmd = 0x%x, value = %d\n", ParaType, param.mixData.value);
#endif

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
	struct xf_get_param_msg  msg = {0, 0};
	int err = XA_SUCCESS;

	switch (ParaType) {
	case UNIA_SAMPLERATE:
		msg.id = UNIA_SAMPLERATE;
		err = xaf_comp_get_config(&pDSP_handle->component, 1, &msg);
		parameter->samplerate = msg.mixData.value;
		break;
	case UNIA_CHANNEL:
		msg.id = UNIA_CHANNEL;
		err = xaf_comp_get_config(&pDSP_handle->component, 1, &msg);
		parameter->channels = msg.mixData.value;
		break;
	case UNIA_DEPTH:
		msg.id = UNIA_DEPTH;
		err = xaf_comp_get_config(&pDSP_handle->component, 1, &msg);
		parameter->depth = msg.mixData.value;
		break;
	case UNIA_CONSUMED_LENGTH:
		msg.id = UNIA_CONSUMED_LENGTH;
		parameter->consumed_length = pDSP_handle->wrap_consumed;
		break;
	case UNIA_OUTPUT_PCM_FORMAT:
		msg.id = UNIA_OUTPUT_PCM_FORMAT;
		err = xaf_comp_get_config(&pDSP_handle->component, 1, &msg);
		memcpy(&parameter->outputFormat, &msg.mixData.outputFormat, sizeof(UniAcodecOutputPCMFormat));
		break;
	case UNIA_CODEC_DESCRIPTION:
		pDSP_handle->codcDesc = "dsp codec version";
		parameter->codecDesc = &pDSP_handle->codcDesc;
		break;
	case UNIA_OUTBUF_ALLOC_SIZE:
		switch (pDSP_handle->codec_type) {
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
	TRACE("Get parameter: cmd = 0x%x\n", ParaType);
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
				uint8 *InputBuf,
				uint32 InputSize,
				uint32 *offset,
				uint8 **OutputBuf,
				uint32 *OutputSize)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;
	struct DSP_Handle *pDSP_handle = (struct DSP_Handle *)pua_handle;
	struct xf_get_param_msg  param[3];
	uint8 *inbuf_data;
	uint32 *inner_offset, *inner_size;
	uint8 *pIn = NULL, *pOut = NULL;
	int err = XA_SUCCESS;
	bool buf_from_out = FALSE;
	uint32 *channel_map = NULL;
	uint32 in_size = 0, in_off = 0, out_size = 0;
	unsigned int *codecoffset = &pDSP_handle->codecoffset;
	int *offset_copy = &pDSP_handle->offset_copy;
	int *wrap_consumed = &pDSP_handle->wrap_consumed;
	*wrap_consumed = 0;

	if (*OutputBuf)
		buf_from_out = TRUE;

	if (pDSP_handle->stream_type == STREAM_ADIF)
		pDSP_handle->framed = 0;

#ifdef DEBUG
	TRACE("InputSize = %d, offset = %d\n", InputSize, *offset);
#endif
	if (pDSP_handle->codecData.buf && (pDSP_handle->codec_type == OGG || pDSP_handle->component.comp_type == CODEC_FSL_AAC_DEC ||
		pDSP_handle->codec_type == WMA)) {
		if (pDSP_handle->codecdata_copy == FALSE) {
			InputBufHandle(&pDSP_handle->inner_buf,
						pDSP_handle->codecData.buf,
						pDSP_handle->codecData.size,
						codecoffset,
						0);
		}
	} else
		pDSP_handle->codecdata_copy = TRUE;

	if (pDSP_handle->codecdata_copy == TRUE) {
		if (!pDSP_handle->inptr_busy) {
			ret = InputBufHandle(&pDSP_handle->inner_buf,
						 InputBuf,
						 InputSize,
						 offset_copy,
						 pDSP_handle->framed);
			if (ret != ACODEC_SUCCESS) {
				*offset = *offset_copy;
				*offset_copy = 0;
				*wrap_consumed = *offset;
				return ret;
			}
		}
	}

	if (pDSP_handle->codecData.size <= *codecoffset)
		pDSP_handle->codecdata_copy = TRUE;

	inbuf_data = pDSP_handle->inner_buf.data;
	inner_offset = &pDSP_handle->inner_buf.inner_offset;
	inner_size = &pDSP_handle->inner_buf.inner_size;

	if (pDSP_handle->component.comp_type < CODEC_FSL_OGG_DEC) {
		if ((pDSP_handle->input_over == TRUE) &&
			(!pDSP_handle->outptr_busy) &&
			(pDSP_handle->last_output_size <= 0)) {
			ret = ACODEC_END_OF_STREAM;
			pDSP_handle->input_over = FALSE;

			return ret;
		}
	}

	if (!InputBuf && (*inner_size <= 0))
		pDSP_handle->input_over = TRUE;

	if (pDSP_handle->component.comp_type != CODEC_FSL_MP3_DEC) {
		if (!pDSP_handle->ID3flag && !memcmp(inbuf_data +
						(*inner_offset), "ID3", 3)) {
			uint8 *pBuff = inbuf_data + (*inner_offset);

			pDSP_handle->tagsize = (pBuff[6] << 21) |
				(pBuff[7] << 14) | (pBuff[8] << 7) | pBuff[9];
			pDSP_handle->tagsize += 10;
			pDSP_handle->ID3flag = TRUE;
		}
		if (pDSP_handle->ID3flag) {
			if (*inner_size >= pDSP_handle->tagsize) {
				pDSP_handle->ID3flag = FALSE;
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

	if (pDSP_handle->memory_allocated == FALSE) {
		/* Set the default function of dsp codec */
		err = SetDefaultFeature(pua_handle);
		if (err) {
#ifdef DEBUG
			TRACE("dsp open error, please check it!\n");
#endif
			return ACODEC_INIT_ERR;
		}
		pDSP_handle->memory_allocated = TRUE;
	}

	if (!(*OutputBuf)) {
		pDSP_handle->dsp_out_buf =
		  pDSP_handle->sMemOps.Malloc(pDSP_handle->outbuf_alloc_size);
		if (!pDSP_handle->dsp_out_buf) {
#ifdef DEBUG
			TRACE("memory allocation error for output buffer\n");
#endif
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
	TRACE("inner buffer data size = %d, inner buffer offset = %d\n",
	      in_size, *inner_offset);
#endif

	err = comp_process(pDSP_handle, pIn, in_size, &in_off, pOut, &out_size);

	*inner_size -= in_off;
	*inner_offset += in_off;

	if (pDSP_handle->inptr_busy == FALSE && (*offset_copy >= InputSize)) {
		*offset = *offset_copy;
		*offset_copy = 0;
		*wrap_consumed = *offset;
	}

	if (buf_from_out)
		memcpy(*OutputBuf, pOut, out_size);
	*OutputSize = out_size;

	pDSP_handle->last_output_size = out_size;

	if (err == XA_SUCCESS || err == ACODEC_CAPIBILITY_CHANGE) {
#ifdef DEBUG
		TRACE("NO_ERROR: consumed length = %d, output size = %d\n",
		      in_size, out_size);
#endif

		param[0].id = UNIA_SAMPLERATE;
		param[1].id = UNIA_CHANNEL;
		param[2].id = UNIA_DEPTH;

		xaf_comp_get_config(&pDSP_handle->component, 3, &param[0]);

		pDSP_handle->samplerate = param[0].mixData.value;
		pDSP_handle->channels = param[1].mixData.value;
		pDSP_handle->depth = param[2].mixData.value;

		if (pDSP_handle->component.comp_type < CODEC_FSL_OGG_DEC) {
			if ((pDSP_handle->channels == 1) &&
				((pDSP_handle->codec_type == AAC)      ||
				(pDSP_handle->codec_type == AAC_PLUS) ||
				(pDSP_handle->codec_type == BSAC)     ||
				(pDSP_handle->codec_type == DAB_PLUS))) {
				cancel_unused_channel_data(*OutputBuf,
						*OutputSize, pDSP_handle->depth);

				*OutputSize >>= 1;
			}
		}

		if (*OutputSize ==  0) {
			if (*OutputBuf && (buf_from_out == FALSE)) {
				pDSP_handle->sMemOps.Free(*OutputBuf);
				pDSP_handle->dsp_out_buf = NULL;
				*OutputBuf = NULL;
			}
		}
		return err;
	}

#ifdef DEBUG
	TRACE("HAS_ERROR: err = 0x%x\n", (int)err);
#endif

	if (err == XA_NOT_ENOUGH_DATA) {
		if (InputBuf && InputSize > *offset)
			err = XA_SUCCESS;
		else
			err = ACODEC_NOT_ENOUGH_DATA;
	}
	switch (pDSP_handle->codec_type) {
	case AAC:
	case AAC_PLUS:
		/******************dedicate for aacplus dec*******************/
		if (err == XA_NOT_ENOUGH_DATA) {
			if (InputBuf && InputSize > *offset)
				ret = XA_SUCCESS;
			else
				ret = ACODEC_NOT_ENOUGH_DATA;
		} else if (err == XA_CAPIBILITY_CHANGE) {
			ret = ACODEC_CAPIBILITY_CHANGE;
		} else if (err == XA_ERROR_STREAM) {
			ret = ACODEC_ERROR_STREAM;
		} else if (err == XA_ERR_UNKNOWN) {
			ret = ACODEC_ERR_UNKNOWN;
		} else if (err == XA_END_OF_STREAM) {
			ret = ACODEC_END_OF_STREAM;
		} else
			ret = err;

		pDSP_handle->last_err = err;
		break;
	case MP2:
	case MP3:
		/******************dedicate for mp3 dec*******************/
		if (err == XA_PROFILE_NOT_SUPPORT) {
			ret = ACODEC_PROFILE_NOT_SUPPORT;
		} else if (err == XA_NOT_ENOUGH_DATA) {
			ret = ACODEC_NOT_ENOUGH_DATA;
		} else {
			if (err == XA_ERROR_STREAM)
				ret = ACODEC_ERROR_STREAM;
		}
		if (pDSP_handle->component.comp_type == CODEC_FSL_MP3_DEC && err == XA_END_OF_STREAM)
			ret = ACODEC_END_OF_STREAM;
		pDSP_handle->last_err = err;
		break;
	case BSAC:
		/******************dedicate for bsac dec*******************/
		if (err == XA_NOT_ENOUGH_DATA) {
			ret = ACODEC_NOT_ENOUGH_DATA;
		} else {
			if (err == XA_ERROR_STREAM)
				ret = ACODEC_ERROR_STREAM;
			if (err == XA_END_OF_STREAM)
				ret = ACODEC_END_OF_STREAM;
		}
		pDSP_handle->last_err = err;
		break;
	case DRM:
		/******************dedicate for drm dec*******************/
		if (err == XA_NOT_ENOUGH_DATA) {
			ret = ACODEC_NOT_ENOUGH_DATA;
		} else {
			if (err == XA_ERROR_STREAM)
				ret = ACODEC_ERROR_STREAM;
		}
		pDSP_handle->last_err = err;
		break;
	case SBCDEC:
		/******************dedicate for sbc dec*******************/
		if (err == XA_NOT_ENOUGH_DATA) {
			ret = ACODEC_NOT_ENOUGH_DATA;
		} else {
			if (err == XA_ERROR_STREAM)
				ret = ACODEC_ERROR_STREAM;
		}
		pDSP_handle->last_err = err;
		break;
	case SBCENC:
		/******************dedicate for sbc enc*******************/
		if (err == XA_NOT_ENOUGH_DATA) {
			ret = ACODEC_NOT_ENOUGH_DATA;
		} else {
			if (err == XA_ERROR_STREAM)
				ret = ACODEC_ERROR_STREAM;
		}
		pDSP_handle->last_err = err;
		break;
	case DAB_PLUS:
		/******************dedicate for dabplus dec*******************/
		if (err == XA_NOT_ENOUGH_DATA) {
			ret = ACODEC_NOT_ENOUGH_DATA;
		} else {
			if (err == XA_ERROR_STREAM)
				ret = ACODEC_ERROR_STREAM;
		}
		pDSP_handle->last_err = err;
		break;
	case OGG:
		if (err == XA_END_OF_STREAM)
			ret = ACODEC_END_OF_STREAM;
		break;
	case AC3:
		if (err == XA_END_OF_STREAM)
			ret = ACODEC_END_OF_STREAM;
		if (err == XA_NOT_ENOUGH_DATA)
			ret = ACODEC_NOT_ENOUGH_DATA;
		break;
	case DD_PLUS:
		if (err == XA_END_OF_STREAM)
			ret = ACODEC_END_OF_STREAM;
		break;
	case NBAMR:
		if (err == XA_END_OF_STREAM)
			ret = ACODEC_END_OF_STREAM;
		else if (err == XA_ERROR_STREAM)
			ret = ACODEC_ERROR_STREAM;
		break;
	case WBAMR:
		if (err == XA_END_OF_STREAM)
			ret = ACODEC_END_OF_STREAM;
		else if (err == XA_ERROR_STREAM)
			ret = ACODEC_ERROR_STREAM;
		break;
	case WMA:
		if (err == XA_NOT_ENOUGH_DATA) {
			if (InputSize > *offset)
				ret = ACODEC_SUCCESS;
			else
				ret = ACODEC_NOT_ENOUGH_DATA;
			}
		if (err == XA_END_OF_STREAM)
			ret = ACODEC_END_OF_STREAM;
		else if (err == XA_ERROR_STREAM)
			ret = ACODEC_ERROR_STREAM;
		break;
	default:
		break;
	}

	if (!(out_size)) {
		if ((*OutputBuf) && (buf_from_out == FALSE)) {
			pDSP_handle->sMemOps.Free(*OutputBuf);
			pDSP_handle->dsp_out_buf = NULL;
			*OutputBuf = NULL;
		}
	}

	return ret;
}

char *DSPDecLastErr(UniACodec_Handle pua_handle)
{
	struct DSP_Handle *pDSP_handle = (struct DSP_Handle *)pua_handle;
	int32 last_err = pDSP_handle->last_err;
	struct lastErr *errors = NULL;

	return NULL;
}
