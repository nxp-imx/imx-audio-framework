//*****************************************************************
// Copyright 2018 NXP
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//*****************************************************************

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


#ifdef  TGT_OS_ANDROID
#define CORE_LIB_PATH   "/vendor/lib/"
#else
#define CORE_LIB_PATH   "/usr/lib/imx-mm/audio-codec/hifi/"
#endif

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
int32 UniACodecQueryInterface (uint32 id, void ** func) {
    if (NULL==func)
            return ACODEC_PARA_ERROR;

    switch(id) {
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
            *func=NULL;
            break;
    }

    return ACODEC_SUCCESS;
}

/*
 * DSPDecVersionInfo - get core decoder version info
 */
const char * DSPDecVersionInfo() {
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
UniACodec_Handle DSPDecCreate(UniACodecMemoryOps * memOps, AUDIOFORMAT type) {
    DSP_Handle * pDSP_handle = NULL;
    uint32 frame_maxlen;
	uint32 process_id = 0;
    char lib_path[200];
    char lib_wrap_path[200];
    int err = 0;

    if(NULL == memOps)
        return NULL;

    pDSP_handle =  memOps->Malloc(sizeof(DSP_Handle));
    if(NULL == pDSP_handle) {
#ifdef DEBUG
        printf("memory allocation error for pDSP_handle\n");
#endif
        return NULL;
    }
    memset(pDSP_handle, 0, sizeof(DSP_Handle));

    memcpy(&(pDSP_handle->sMemOps), memOps, sizeof(UniACodecMemoryOps));
    pDSP_handle->codec_type = type;

    strcpy(lib_path, CORE_LIB_PATH);
    strcpy(lib_wrap_path, CORE_LIB_PATH);

    frame_maxlen = 4096;
    pDSP_handle->outbuf_alloc_size = 16384;

    pDSP_handle->dsp_binary_info.file = lib_path;
    pDSP_handle->dsp_wrap_binary_info.file = lib_wrap_path;

    pDSP_handle->dsp_binary_info.lib_type = HIFI_CODEC_LIB;
    pDSP_handle->dsp_wrap_binary_info.lib_type = HIFI_CODEC_WRAP_LIB;

    switch (pDSP_handle->codec_type) {
        case MP2:
            pDSP_handle->dsp_binary_info.type = CODEC_MP2_DEC;
            strcat(lib_path, "lib_dsp_mp2_dec.so");
            break;
        case MP3:
            pDSP_handle->dsp_binary_info.type = CODEC_MP3_DEC;
            strcat(lib_path, "lib_dsp_mp3_dec.so");
            break;
        case AAC:
        case AAC_PLUS:
            pDSP_handle->dsp_binary_info.type = CODEC_AAC_DEC;
            strcat(lib_path, "lib_dsp_aac_dec.so");
            break;
        case DAB_PLUS:
            pDSP_handle->dsp_binary_info.type = CODEC_DAB_DEC;
            strcat(lib_path, "lib_dsp_dabplus_dec.so");
            break;
        case BSAC:
            pDSP_handle->dsp_binary_info.type = CODEC_BSAC_DEC;
            strcat(lib_path, "lib_dsp_bsac_dec.so");
            break;
        case DRM:
            pDSP_handle->dsp_binary_info.type = CODEC_DRM_DEC;
            strcat(lib_path, "lib_dsp_drm_dec.so");
            break;
        case SBCDEC:
            pDSP_handle->dsp_binary_info.type = CODEC_SBC_DEC;
            strcat(lib_path, "lib_dsp_sbc_dec.so");
            break;
        case SBCENC:
            pDSP_handle->dsp_binary_info.type = CODEC_SBC_ENC;
            strcat(lib_path, "lib_dsp_sbc_enc.so");
            break;
        default:
#ifdef DEBUG
            printf("HIFI4 DSP doesn't support this audio type, please check it!\n");
#endif
            goto Err2;
            break;
    }
    strcat(lib_wrap_path, "lib_dsp_codec_wrap.so");

    pDSP_handle->inner_buf.data = memOps->Malloc(4096);
    if(NULL == pDSP_handle->inner_buf.data) {
#ifdef DEBUG
        printf("memory allocation error for inner_buf.data\n");
#endif
        goto Err2;
    }

    pDSP_handle->inner_buf.buf_size = 4096;
    pDSP_handle->inner_buf.threshold = 4096;
    memset(pDSP_handle->inner_buf.data, 0, 4096);

    pDSP_handle->fd_hifi = open("/dev/mxc_hifi4", O_RDWR);
    if(pDSP_handle->fd_hifi < 0)
    {
#ifdef DEBUG
        printf("Unable to open device\n");
#endif
        goto Err2;
    }

	err = ioctl(pDSP_handle->fd_hifi, HIFI4_CLIENT_REGISTER, &process_id);
	if(err) {
#ifdef DEBUG
        printf("Load codec err in DSPDecCreate(), err = %d\n", err);
#endif
        goto Err1;
	}
	pDSP_handle->process_id = process_id;

	pDSP_handle->dsp_wrap_binary_info.process_id = process_id;
	pDSP_handle->dsp_wrap_binary_info.type = pDSP_handle->dsp_binary_info.type;
    err = ioctl(pDSP_handle->fd_hifi, HIFI4_LOAD_CODEC, &(pDSP_handle->dsp_wrap_binary_info));
    if(err) {
#ifdef DEBUG
        printf("Load codec wrapper err in DSPDecCreate(), err = %d\n", err);
#endif
        goto Err;
    }

	pDSP_handle->dsp_binary_info.process_id = process_id;
    err = ioctl(pDSP_handle->fd_hifi, HIFI4_LOAD_CODEC, &(pDSP_handle->dsp_binary_info));
    if(err) {
#ifdef DEBUG
        printf("Load codec err in DSPDecCreate(), err = %d\n", err);
#endif
        goto Err;
    }

    err = ioctl(pDSP_handle->fd_hifi, HIFI4_INIT_CODEC, &(pDSP_handle->process_id));
    if(err) {
#ifdef DEBUG
        printf("Init codec err in DSPDecInit(), err = %d\n", err);
#endif
        goto Err;
    }

    pDSP_handle->memory_allocated = FALSE;

#ifdef DEBUG
    printf("inner_buf threshold = %d, inner output buf size = %d\n", frame_maxlen, pDSP_handle->outbuf_alloc_size);
#endif

    return (UniACodec_Handle)pDSP_handle;

Err:
#ifdef DEBUG
    printf("Create Decoder Failed, Please Check it!\n");
#endif
    err = ioctl(pDSP_handle->fd_hifi, HIFI4_CLIENT_UNREGISTER, &(pDSP_handle->process_id));
Err1:
    close(pDSP_handle->fd_hifi);
Err2:
    if(pDSP_handle->inner_buf.data) {
        pDSP_handle->sMemOps.Free(pDSP_handle->inner_buf.data);
    }
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
UA_ERROR_TYPE DSPDecDelete(UniACodec_Handle pua_handle) {
    DSP_Handle * pDSP_handle = (DSP_Handle *)pua_handle;
    int err = 0;

    if (NULL == pua_handle)
        return ACODEC_PARA_ERROR;

    err = ioctl(pDSP_handle->fd_hifi, HIFI4_CODEC_CLOSE, &(pDSP_handle->process_id));
    err = ioctl(pDSP_handle->fd_hifi, HIFI4_CLIENT_UNREGISTER, &(pDSP_handle->process_id));
    if(err) {
#ifdef DEBUG
        printf("Close HIFI4 Failed, Please Check it, err = 0x%x\n", err);
#endif
    }

    close(pDSP_handle->fd_hifi);

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
UA_ERROR_TYPE DSPDecReset(UniACodec_Handle pua_handle) {
    DSP_Handle * pDSP_handle = (DSP_Handle *)pua_handle;
    int ret = ACODEC_SUCCESS;

    /*
     * The HIFI4_RESET_CODEC command is used to reset codec buffers and its related parameters
     * in dsp firmware. However, the related buffers of dsp core lib have not been allocated
     * before hifi4 open. So don't reset codec before hifi4 open.
     */
    if(pDSP_handle->memory_allocated) {
        ret = ioctl(pDSP_handle->fd_hifi, HIFI4_RESET_CODEC, &(pDSP_handle->process_id));
        if(ret) {
#ifdef DEBUG
            printf("Reset HIFI4 Failed, Please Check it, ret = 0x%x\n", ret);
#endif
            goto Fail;
        }
    }
    memset(&(pDSP_handle->dsp_decode_info), 0, sizeof(struct decode_info));
    memset(&(pDSP_handle->dsp_prop_info), 0, sizeof(struct prop_info));
    memset(&(pDSP_handle->dsp_prop_config), 0, sizeof(struct prop_config));

    ResetInnerBuf(&(pDSP_handle->inner_buf),pDSP_handle->inner_buf.threshold, pDSP_handle->inner_buf.threshold);

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
UA_ERROR_TYPE DSPDecSetPara(UniACodec_Handle pua_handle, UA_ParaType  ParaType, UniACodecParameter * parameter) {
    DSP_Handle * pDSP_handle = (DSP_Handle *)pua_handle;
    UA_ERROR_TYPE ret = ACODEC_SUCCESS;
    int err = XA_SUCCESS;
	int type;

    switch (ParaType) {
        case UNIA_SAMPLERATE:
            pDSP_handle->dsp_prop_config.val = parameter->samplerate;
            break;
        case UNIA_CHANNEL:
            if (parameter->channels == 0 || parameter->channels > 8)
                return ACODEC_PARA_ERROR;
            else
                pDSP_handle->dsp_prop_config.val = parameter->channels;
            break;
        case UNIA_FRAMED:
            pDSP_handle->framed = parameter->framed;
            break;
        case UNIA_DEPTH:
            if (parameter->depth != 16 && parameter->depth != 24 && parameter->depth !=32)
                return ACODEC_PARA_ERROR;
            else {
                pDSP_handle->dsp_prop_config.val = parameter->depth;
                pDSP_handle->depth_is_set = 1;
            }
            break;
        case UNIA_CODEC_DATA:
            pDSP_handle->codecData = parameter->codecData;
            break;
        case UNIA_DOWNMIX_STEREO:
            pDSP_handle->downmix = parameter->downmix;
            break;
        case UNIA_TO_STEREO:
            pDSP_handle->dsp_prop_config.val = parameter->mono_to_stereo;
            break;
        case UNIA_STREAM_TYPE:
            pDSP_handle->dsp_prop_config.val = parameter->stream_type;
            break;
        case UNIA_BITRATE:
            pDSP_handle->dsp_prop_config.val = parameter->bitrate;
            break;
        case UNIA_OUTPUT_PCM_FORMAT:
            pDSP_handle->outputFormat = parameter->outputFormat;
            break;
        case UNIA_CHAN_MAP_TABLE:
            pDSP_handle->chan_map_tab = parameter->chan_map_tab;
            break;
        default:
            break;
    }

    if((pDSP_handle->codec_type == MP2) || (pDSP_handle->codec_type == MP3)) {
        switch (ParaType) {
        /******************dedicate for mp3 dec and mp2 dec*******************/
            case UNIA_MP3_DEC_CRC_CHECK:
                pDSP_handle->dsp_prop_config.val = parameter->crc_check;
                break;
            case UNIA_MP3_DEC_MCH_ENABLE:
                pDSP_handle->dsp_prop_config.val = parameter->mch_enable;
				break;
            case UNIA_MP3_DEC_NONSTD_STRM_SUPPORT:
                pDSP_handle->dsp_prop_config.val = parameter->nonstd_strm_support;
                break;
            default:
                break;
        }
    } else if(pDSP_handle->codec_type == BSAC) {
        switch (ParaType) {
        /******************dedicate for bsac dec****************************/
            case UNIA_BSAC_DEC_DECODELAYERS:
                pDSP_handle->dsp_prop_config.val = parameter->layers;
                break;
            default:
                break;
        }
    } else if((pDSP_handle->codec_type == AAC) || (pDSP_handle->codec_type == AAC_PLUS)) {
        switch (ParaType) {
        /******************dedicate for aacplus dec*************************/
            case UNIA_CHANNEL:
                if(pDSP_handle->dsp_prop_config.val > 2) {
#ifdef DEBUG
                    printf("Error: multi-channnel decoding doesn't support for AACPLUS\n");
#endif
                    return ACODEC_PROFILE_NOT_SUPPORT;
                }
                break;
            case UNIA_AACPLUS_DEC_BDOWNSAMPLE:
                pDSP_handle->dsp_prop_config.val = parameter->bdownsample;
                break;
            case UNIA_AACPLUS_DEC_BBITSTREAMDOWNMIX:
                pDSP_handle->dsp_prop_config.val = parameter->bbitstreamdownmix;
                break;
            case UNIA_AACPLUS_DEC_CHANROUTING:
                pDSP_handle->dsp_prop_config.val = parameter->chanrouting;
                break;
            default:
                break;
        }
    } else if(pDSP_handle->codec_type == DAB_PLUS) {
        switch (ParaType) {
        /*****************dedicate for dabplus dec******************/
            case UNIA_DABPLUS_DEC_BDOWNSAMPLE:
                pDSP_handle->dsp_prop_config.val = parameter->bdownsample;
                break;
            case UNIA_DABPLUS_DEC_BBITSTREAMDOWNMIX:
                pDSP_handle->dsp_prop_config.val = parameter->bbitstreamdownmix;
                break;
            case UNIA_DABPLUS_DEC_CHANROUTING:
                pDSP_handle->dsp_prop_config.val = parameter->chanrouting;
                break;
            default:
                break;
        }
    } else if(pDSP_handle->codec_type == SBCENC) {
        switch (ParaType) {
        /*******************dedicate for sbc enc******************/
            case UNIA_SBC_ENC_SUBBANDS:
                pDSP_handle->dsp_prop_config.val = parameter->enc_subbands;
                break;
            case UNIA_SBC_ENC_BLOCKS:
                pDSP_handle->dsp_prop_config.val = parameter->enc_blocks;
                break;
            case UNIA_SBC_ENC_SNR:
                pDSP_handle->dsp_prop_config.val = parameter->enc_snr;
                break;
            case UNIA_SBC_ENC_BITPOOL:
                pDSP_handle->dsp_prop_config.val = parameter->enc_bitpool;
                break;
            case UNIA_SBC_ENC_CHMODE:
                pDSP_handle->dsp_prop_config.val = parameter->enc_chmode;
                break;
            default:
                break;
        }
    }

    pDSP_handle->dsp_prop_config.cmd = ParaType;
    pDSP_handle->dsp_prop_config.codec_id = pDSP_handle->dsp_binary_info.type;
    pDSP_handle->dsp_prop_config.process_id = pDSP_handle->process_id;

    err = ioctl(pDSP_handle->fd_hifi, HIFI4_SET_CONFIG, &(pDSP_handle->dsp_prop_config));

#ifdef DEBUG
    printf("SetPara: cmd = 0x%x, value = %d\n", pDSP_handle->dsp_prop_config.cmd, pDSP_handle->dsp_prop_config.val);
#endif

	if(err)
        ret = ACODEC_INIT_ERR;

    return ret;
}

/*
 * DSPDecGetPara - get parameter from DSP wrapper according to the parameter type
 *
 * @pua_handle: handle of the DSP codec wrapper
 * @ParaType: parameter type to get
 * @parameter: pointer to parameter structure
 *
 * Returns: ACODEC_SUCCESS in case of success and other codes in case of error
 */
UA_ERROR_TYPE DSPDecGetPara(UniACodec_Handle pua_handle, UA_ParaType  ParaType, UniACodecParameter * parameter) {
    DSP_Handle * pDSP_handle = (DSP_Handle *)pua_handle;
    int err = XA_SUCCESS;

    pDSP_handle->dsp_prop_info.process_id = pDSP_handle->process_id;

    switch (ParaType) {
        case UNIA_SAMPLERATE:
             err = ioctl(pDSP_handle->fd_hifi, HIFI4_GET_PCM_PROP, &(pDSP_handle->dsp_prop_info));
             parameter->samplerate= pDSP_handle->dsp_prop_info.samplerate;
            break;
        case UNIA_CHANNEL:
            err = ioctl(pDSP_handle->fd_hifi, HIFI4_GET_PCM_PROP, &(pDSP_handle->dsp_prop_info));
            parameter->channels = pDSP_handle->dsp_prop_info.channels;
            break;
        case UNIA_DEPTH:
            err = ioctl(pDSP_handle->fd_hifi, HIFI4_GET_PCM_PROP, &(pDSP_handle->dsp_prop_info));
            parameter->depth = pDSP_handle->dsp_prop_info.bits;
            break;
        case UNIA_CONSUMED_LENGTH:
            err = ioctl(pDSP_handle->fd_hifi, HIFI4_GET_PCM_PROP, &(pDSP_handle->dsp_prop_info));
            parameter->consumed_length= pDSP_handle->dsp_prop_info.consumed_bytes;
            break;
        case UNIA_OUTPUT_PCM_FORMAT:
            parameter->outputFormat = pDSP_handle->outputFormat;
            break;
        case UNIA_CODEC_DESCRIPTION:
            pDSP_handle->codcDesc = "dsp codec version";
            parameter->codecDesc = &(pDSP_handle->codcDesc);
            break;
        case UNIA_OUTBUF_ALLOC_SIZE:
            switch (pDSP_handle->codec_type) {
                case MP2:
                case MP3:
                    parameter->outbuf_alloc_size = 16384;
                    break;
                case AAC:
                case AAC_PLUS:
                    parameter->outbuf_alloc_size = 16384;
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
	printf("Get parameter: cmd = 0x%x\n", ParaType);
#endif

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
UA_ERROR_TYPE DSPDecFrameDecode( UniACodec_Handle pua_handle, uint8 *InputBuf,  uint32 InputSize,
                                 uint32 * offset, uint8 ** OutputBuf, uint32 * OutputSize) {
    UA_ERROR_TYPE ret = ACODEC_SUCCESS;
    DSP_Handle * pDSP_handle = (DSP_Handle *)pua_handle;
    uint8 *inbuf_data;
    uint32 *inner_offset, *inner_size;
    uint8 *pOut = NULL;
    int err = XA_SUCCESS;
    bool buf_from_out = FALSE;
    uint32* channel_map = NULL;

    if(*OutputBuf != NULL) {
        buf_from_out = TRUE;
    }

    if(pDSP_handle->stream_type == STREAM_ADIF) {
        pDSP_handle->framed = 0;
    }

#ifdef DEBUG
    printf("InputSize = %d, offset = %d\n", InputSize, *offset);
#endif

    ret = InputBufHandle( &(pDSP_handle->inner_buf), InputBuf, InputSize, offset, pDSP_handle->framed);
    if ( ret != ACODEC_SUCCESS) {
        return ret;
    }

    inbuf_data = pDSP_handle->inner_buf.data;
    inner_offset = &(pDSP_handle->inner_buf.inner_offset);
    inner_size = &(pDSP_handle->inner_buf.inner_size);

    if((pDSP_handle->dsp_decode_info.input_over == 1) && (pDSP_handle->dsp_decode_info.out_buf_off <= 0))
    {
        ret = ACODEC_END_OF_STREAM;
        pDSP_handle->dsp_decode_info.input_over = 0;

        return ret;
    }

    if(!InputBuf && (*inner_size <= 0))
    {
        pDSP_handle->dsp_decode_info.input_over = 1;
    }

    if(!pDSP_handle->ID3flag && !memcmp(inbuf_data + (*inner_offset), "ID3", 3))
    {
        char *pBuff = inbuf_data + (*inner_offset);
        pDSP_handle->tagsize = (pBuff[6]<<21) | (pBuff[7]<<14) | (pBuff[8]<<7) | pBuff[9];
        pDSP_handle->tagsize += 10;
        pDSP_handle->ID3flag = TRUE;
    }
    if(pDSP_handle->ID3flag)
    {
        if(*inner_size >= pDSP_handle->tagsize)
        {
            pDSP_handle->ID3flag = FALSE;
            *inner_offset += pDSP_handle->tagsize;
            *inner_size -= pDSP_handle->tagsize;
            pDSP_handle->consumed_length += pDSP_handle->tagsize;
            pDSP_handle->tagsize = 0;

            memmove(inbuf_data, inbuf_data + (*inner_offset), *inner_size);
            *inner_offset = 0;
        }
        else
        {
            pDSP_handle->tagsize -= *inner_size;
            *inner_offset += *inner_size;
            pDSP_handle->consumed_length += *inner_size;
            *inner_size = 0;
            return ACODEC_NOT_ENOUGH_DATA;
        }
    }

    if(pDSP_handle->memory_allocated == FALSE) {
        err = SetDefaultFeature(pua_handle);        /* Set the default function of dsp codec */
        err = ioctl(pDSP_handle->fd_hifi, HIFI4_CODEC_OPEN, &(pDSP_handle->process_id));
        if(err) {
#ifdef DEBUG
            printf("hifi4 open error, please check it!\n");
#endif
            return ACODEC_INIT_ERR;
        }
        pDSP_handle->memory_allocated = TRUE;
    }

    if(!(*OutputBuf)) {
        pDSP_handle->dsp_out_buf = pDSP_handle->sMemOps.Malloc(pDSP_handle->outbuf_alloc_size);
        if(NULL == pDSP_handle->dsp_out_buf) {
#ifdef DEBUG
            printf("memory allocation error for output buffer\n");
#endif
            return ACODEC_INSUFFICIENT_MEM;
        }
        memset(pDSP_handle->dsp_out_buf, 0, pDSP_handle->outbuf_alloc_size);
        *OutputBuf = pDSP_handle->dsp_out_buf;
    }

    pOut = *OutputBuf;

    pDSP_handle->dsp_decode_info.in_buf_addr = inbuf_data;
    pDSP_handle->dsp_decode_info.in_buf_size = *inner_size + *inner_offset;
    pDSP_handle->dsp_decode_info.in_buf_off = *inner_offset;

#ifdef DEBUG
	printf("in_buf_size = %d, in_buf_off = %d\n", pDSP_handle->dsp_decode_info.in_buf_size, pDSP_handle->dsp_decode_info.in_buf_off);
#endif

    pDSP_handle->dsp_decode_info.out_buf_addr = pOut;
    pDSP_handle->dsp_decode_info.out_buf_size = pDSP_handle->outbuf_alloc_size;
    pDSP_handle->dsp_decode_info.out_buf_off = 0;
    pDSP_handle->dsp_decode_info.process_id = pDSP_handle->process_id;

    err = ioctl(pDSP_handle->fd_hifi, HIFI4_DECODE_ONE_FRAME, &(pDSP_handle->dsp_decode_info));

    *inner_size -= (pDSP_handle->dsp_decode_info.in_buf_off - *inner_offset);
    *inner_offset = pDSP_handle->dsp_decode_info.in_buf_off;
    if (buf_from_out)
        memcpy(*OutputBuf, pOut, pDSP_handle->dsp_decode_info.out_buf_off);
    *OutputSize = pDSP_handle->dsp_decode_info.out_buf_off;

    if (err == XA_SUCCESS) {
#ifdef DEBUG
        printf("NO_ERROR: consumed input length = %d, output size = %d\n", pDSP_handle->dsp_decode_info.in_buf_off - *inner_offset, pDSP_handle->dsp_decode_info.out_buf_off);
#endif

        pDSP_handle->dsp_prop_info.process_id = pDSP_handle->process_id;
        err = ioctl(pDSP_handle->fd_hifi, HIFI4_GET_PCM_PROP, &(pDSP_handle->dsp_prop_info));
        pDSP_handle->samplerate = pDSP_handle->dsp_prop_info.samplerate;
        pDSP_handle->channels = pDSP_handle->dsp_prop_info.channels;

        if((pDSP_handle->channels == 1) &&
           ((pDSP_handle->codec_type == AAC)      ||
            (pDSP_handle->codec_type == AAC_PLUS) ||
            (pDSP_handle->codec_type == BSAC)     ||
            (pDSP_handle->codec_type == DAB_PLUS)
           )
          ) {
            cancel_unused_channel_data(*OutputBuf, *OutputSize, pDSP_handle->dsp_prop_info.bits);

            *OutputSize >>= 1;
        }

        if((pDSP_handle->codec_type != SBCENC)
            && (pDSP_handle->chan_map_tab.size!=0)
            && (pDSP_handle->outputFormat.chan_pos_set==FALSE)){
            if (pDSP_handle->channels <= pDSP_handle->chan_map_tab.size){
                int channel;
                channel = pDSP_handle->channels;
                channel_map = pDSP_handle->chan_map_tab.channel_table[channel];
                if(channel_map){
                    memcpy(pDSP_handle->outputFormat.layout,channel_map, sizeof(uint32)*channel);
                    pDSP_handle->outputFormat.chan_pos_set = TRUE;
                }
            }
            else {
                if(*OutputSize ==  0) {
                    if((*OutputBuf)&&(buf_from_out==FALSE)) {
                        pDSP_handle->sMemOps.Free(*OutputBuf);
                        pDSP_handle->dsp_out_buf = NULL;
                        *OutputBuf = NULL;
                    }
                }
                return ACODEC_ERROR_STREAM;
            }
        }

        if ( (pDSP_handle->codec_type != SBCENC)
               && ((pDSP_handle->outputFormat.samplerate != pDSP_handle->samplerate)
               || (pDSP_handle->outputFormat.channels != pDSP_handle->channels)
               || (memcmp(pDSP_handle->outputFormat.layout, pDSP_handle->layout_bak, sizeof(uint32)*pDSP_handle->channels)))
               && (pDSP_handle->channels != 0)){
#ifdef DEBUG
            printf("output format changed\n");
#endif

            pDSP_handle->outputFormat.width = pDSP_handle->dsp_prop_info.bits;
            pDSP_handle->outputFormat.depth = pDSP_handle->dsp_prop_info.bits;
            pDSP_handle->outputFormat.channels = pDSP_handle->channels;
            pDSP_handle->outputFormat.interleave = TRUE;
            pDSP_handle->outputFormat.samplerate = pDSP_handle->samplerate;

            if ((pDSP_handle->channels > 2) && (pDSP_handle->channels <= 8) && (!pDSP_handle->outputFormat.chan_pos_set)){
                if (aacd_channel_layouts[pDSP_handle->channels]){
                    memcpy(pDSP_handle->outputFormat.layout, aacd_channel_layouts[pDSP_handle->channels], sizeof(uint32)*pDSP_handle->channels);
                }
            }
            if (pDSP_handle->channels == 2){
                pDSP_handle->outputFormat.layout[0] = UA_CHANNEL_FRONT_LEFT;
                pDSP_handle->outputFormat.layout[1] = UA_CHANNEL_FRONT_RIGHT;
            }

            memcpy(pDSP_handle->layout_bak, pDSP_handle->outputFormat.layout, sizeof(uint32)*pDSP_handle->channels);

            if((*OutputSize > 0) && (pDSP_handle->channels > 0))
            {
                channel_pos_convert(pDSP_handle, (uint8 *)(*OutputBuf), *OutputSize, pDSP_handle->channels, pDSP_handle->dsp_prop_info.bits);
            }

            ret =  ACODEC_CAPIBILITY_CHANGE;
        }

        if(*OutputSize ==  0) {
            if((*OutputBuf)&&(buf_from_out==FALSE)) {
                pDSP_handle->sMemOps.Free(*OutputBuf);
                pDSP_handle->dsp_out_buf = NULL;
                *OutputBuf = NULL;
            }
        }

        return ret;
    }

#ifdef DEBUG
    printf("HAS_ERROR: err = 0x%x\n", (int)err);
#endif

    switch (pDSP_handle->codec_type) {
        case AAC:
        case AAC_PLUS:
        /******************dedicate for aacplus dec*******************/
            if (err == XA_NOT_ENOUGH_DATA) {

                ret = ACODEC_NOT_ENOUGH_DATA;
            } else if (err == XA_CAPIBILITY_CHANGE) {

                ret = ACODEC_CAPIBILITY_CHANGE;
            } else {
                if (err == XA_ERROR_STREAM) {
                    ret = ACODEC_ERROR_STREAM;
                }
            }
            pDSP_handle->last_err = err;
            break;
        case MP2:
        case MP3:
        /******************dedicate for mp3 dec*******************/
            if (err == XA_PROFILE_NOT_SUPPORT) {

                ret= ACODEC_PROFILE_NOT_SUPPORT;
            } else if (err == XA_NOT_ENOUGH_DATA) {

                ret= ACODEC_NOT_ENOUGH_DATA;
            } else {
                if (err == XA_ERROR_STREAM) {
                    ret= ACODEC_ERROR_STREAM;
                }
            }
            pDSP_handle->last_err = err;
            break;
        case BSAC:
        /******************dedicate for bsac dec*******************/
            if (err == XA_NOT_ENOUGH_DATA) {

                ret= ACODEC_NOT_ENOUGH_DATA;
            } else {
                if (err == XA_ERROR_STREAM) {
                    ret= ACODEC_ERROR_STREAM;
                }
            }
            pDSP_handle->last_err = err;
            break;
        case DRM:
        /******************dedicate for drm dec*******************/
            if (err == XA_NOT_ENOUGH_DATA) {

                ret = ACODEC_NOT_ENOUGH_DATA;
            } else {
                if (err == XA_ERROR_STREAM) {
                    ret = ACODEC_ERROR_STREAM;
                }
            }
            pDSP_handle->last_err = err;
            break;
        case SBCDEC:
        /******************dedicate for sbc dec*******************/
            if (err == XA_NOT_ENOUGH_DATA) {

                ret = ACODEC_NOT_ENOUGH_DATA;
            } else {
                if (err == XA_ERROR_STREAM) {
                    ret = ACODEC_ERROR_STREAM;
                }
            }
            pDSP_handle->last_err = err;
            break;
        case SBCENC:
        /******************dedicate for sbc enc*******************/
            if (err == XA_NOT_ENOUGH_DATA) {

                ret = ACODEC_NOT_ENOUGH_DATA;
            } else {
                if (err == XA_ERROR_STREAM) {
                    ret = ACODEC_ERROR_STREAM;
                }
            }
            pDSP_handle->last_err = err;
            break;
        case DAB_PLUS:
        /******************dedicate for dabplus dec*******************/
            if (err == XA_NOT_ENOUGH_DATA) {

                ret = ACODEC_NOT_ENOUGH_DATA;
            } else {
                if (err == XA_ERROR_STREAM) {
                    ret = ACODEC_ERROR_STREAM;
                }
            }
            pDSP_handle->last_err = err;
            break;
        default:
            break;
	}

    if(!(pDSP_handle->dsp_decode_info.out_buf_off)) {
        if((*OutputBuf) && (buf_from_out == FALSE)) {
            pDSP_handle->sMemOps.Free(*OutputBuf);
            pDSP_handle->dsp_out_buf = NULL;
            *OutputBuf = NULL;
        }
    }

    return ret;
}

char * DSPDecLastErr(UniACodec_Handle pua_handle) {
    DSP_Handle * pDSP_handle = (DSP_Handle *)pua_handle;
    int32 last_err = pDSP_handle->last_err;
    lastErr *errors = NULL;

    return NULL;
}
