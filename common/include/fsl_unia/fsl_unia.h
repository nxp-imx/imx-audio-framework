
/***********************************************************************
 * Copyright (c) 2011-2014, Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 * All modifications are confidential and proprietary information
 * of Freescale Semiconductor, Inc. ALL RIGHTS RESERVED.
 ***********************************************************************/

/*
 *
 *  History :
 *  Date             Author              Version    Description
 *
 *  Sep, 2011        Lyon               1.0        Initial Version
 *
 */

#ifndef _Uni_ADEC_WRAPPER_H_
#define _Uni_ADEC_WRAPPER_H_

#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN
#endif


/* Warning:
 * It's not recommended to use any enum types as API arguments or return value!
 * Please use data types that can explicitly tell the data length and asign them the listed enum values.
 * It's because different compilers can treat enum as different types such as integer or unsinged char.
 * If the ACodec library and plug-in(filter) are built by different compliers, the data length mismatch
 * will cause error.
 */
 typedef void * UniACodec_Handle;
//typedef void * UniACodec_Config;
//typedef void * UniACodec_Info;

/*
 * Common error codes of ACodec,
 * within the range [-100 , +100].
 * Different ACodecs can extend the format specific errors OUTSIDE this range,
 * in their own API header files.
 */
typedef enum
{
    ACODEC_SUCCESS = 0,
      
    ACODEC_ERROR_STREAM,
    ACODEC_PARA_ERROR,
    ACODEC_INSUFFICIENT_MEM,
    ACODEC_ERR_UNKNOWN,
    ACODEC_PROFILE_NOT_SUPPORT,
    ACODEC_INIT_ERR,
    ACODEC_NO_OUTPUT,
    
    ACODEC_NOT_ENOUGH_DATA = 0x100,
    ACODEC_CAPIBILITY_CHANGE = 0x200, /* output format changes, caller should reget format from getparameter API */
    ACODEC_END_OF_STREAM = 0x300, /* no output */
} UA_ERROR_TYPE;


/* Parameter type to Set /Get */
typedef enum
{
/* Set parmameters */
/* common  */
    UNIA_SAMPLERATE= 0,
    UNIA_CHANNEL,
    UNIA_FRAMED,        /* one whole frame input */
    UNIA_DEPTH,
    UNIA_CODEC_DATA,
    UNIA_BITRATE,
    UNIA_DOWNMIX_STEREO,
    UNIA_STREAM_TYPE,
    UNIA_CHAN_MAP_TABLE,
    //UNIA_CHANNEL_MASK,
    UNIA_TO_STEREO,
    /* dedicate for Cadence codec wrapper */
    UNIA_INPUT_OVER,
    UNIA_CODEC_ID,
    UNIA_CODEC_ENTRY_ADDR,
    /* dedicate for debug */
    UNIA_FUNC_PRINT,
    UNIA_LOAD_LIB,
    UNIA_UNLOAD_LIB,
    UNIA_RESET_BUF,

/* dedicate for wma */
    UNIA_WMA_BlOCKALIGN= 0x100,
    UNIA_WMA_VERSION,

/*dedicate for RealAudio */
    UNIA_RA_FLAVOR_INDEX = 0x110,
    UNIA_RA_FRAME_BITS,

/* dedicate for mp3 dec */
    UNIA_MP3_DEC_CRC_CHECK = 0x120,
    UNIA_MP3_DEC_MCH_ENABLE,
    UNIA_MP3_DEC_NONSTD_STRM_SUPPORT,

/* dedicate for bsac dec */
    UNIA_BSAC_DEC_DECODELAYERS = 0x130,

/* dedicate for aacplus dec */
    UNIA_AACPLUS_DEC_BDOWNSAMPLE = 0x140,
    UNIA_AACPLUS_DEC_BBITSTREAMDOWNMIX,
    UNIA_AACPLUS_DEC_CHANROUTING,

/* dedicate for dabplus dec */
    UNIA_DABPLUS_DEC_BDOWNSAMPLE = 0x150,
    UNIA_DABPLUS_DEC_BBITSTREAMDOWNMIX,
    UNIA_DABPLUS_DEC_CHANROUTING,

/* dedicate for sbc enc */
    UNIA_SBC_ENC_SUBBANDS = 0x160,
    UNIA_SBC_ENC_BLOCKS,
    UNIA_SBC_ENC_SNR,
    UNIA_SBC_ENC_BITPOOL,
    UNIA_SBC_ENC_CHMODE,

/* Get parmameters */
    UNIA_CODEC_DESCRIPTION= 0x200, 
    UNIA_OUTPUT_PCM_FORMAT,
    UNIA_CONSUMED_LENGTH,
    UNIA_OUTBUF_ALLOC_SIZE,  /* used for allocate output buffer outside */
    
    UA_TYPE_MAX
} UA_ParaType;



typedef enum
{
  UA_CHANNEL_FRONT_MONO,
  UA_CHANNEL_FRONT_LEFT,
  UA_CHANNEL_FRONT_RIGHT,
  
  UA_CHANNEL_REAR_CENTER,
  UA_CHANNEL_REAR_LEFT,
  UA_CHANNEL_REAR_RIGHT,

  UA_CHANNEL_LFE,

  UA_CHANNEL_FRONT_CENTER,
  UA_CHANNEL_FRONT_LEFT_CENTER,
  UA_CHANNEL_FRONT_RIGHT_CENTER,

  UA_CHANNEL_SIDE_LEFT,
  UA_CHANNEL_SIDE_RIGHT,

  UA_CHANNEL_MAX
  
}UA_CHANNEL_LAYOUT;

#define STREAM_NBAMR_BASE  0x10
#define STREAM_WBAMR_BASE  0x20
#define STREAM_DABPLUS_BASE  0x30
typedef enum
{
    /* AAC/AACPLUS file format */
    STREAM_UNKNOW = 0,
    STREAM_ADTS,
    STREAM_ADIF,
    STREAM_RAW,

    STREAM_LATM,
    STREAM_LATM_OUTOFBAND_CONFIG,
    STREAM_LOAS,

    /* NB-AMR file format */
    STREAM_NBAMR_ETSI = STREAM_NBAMR_BASE,
    STREAM_NBAMR_MMSIO,
    STREAM_NBAMR_IF1IO,
    STREAM_NBAMR_IF2IO,

    /* WB-AMR file format */
    STREAM_WBAMR_DEFT = STREAM_WBAMR_BASE,
    STREAM_WBAMR_ITU,
    STREAM_WBAMR_MIME,
    STREAM_WBAMR_IF2,
    STREAM_WBAMR_IF1,

    /* DABPLUS file format */
    STREAM_DABPLUS_RAW_SIDEINFO = STREAM_DABPLUS_BASE,
    STREAM_DABPLUS,

    /* BSAC file raw format */
    STREAM_BSAC_RAW

} STREAM_TYPE;

/* sbc_enc-specific channel modes */
enum sbc_enc_chmode {
    CHMODE_MONO =   0,
    CHMODE_DUAL =   1,
    CHMODE_STEREO = 2,
    CHMODE_JOINT =  3
};

/*
 * Audio codec types.
 */
typedef enum
{
    AAC = 0,
    AAC_PLUS,
    MP3,
    WMA,
    AC3,
    OGG,
    DD_PLUS,
    RA,
    FLAC,
    NBAMR,
    WBAMR,
    BSAC,
    DRM,
    SBCENC,
    SBCDEC,
    DAB_PLUS,
    MP2,

    FORMAT_UNKNOW = 0x200
}AUDIOFORMAT;

/*********************************************************************
 * Uni Audio memory callback funtion pointer table.
 *********************************************************************/
typedef struct
{
    void* (*Calloc) (UWORD32 numElements, UWORD32 size);
    void* (*Malloc) (size_t size);
    void  (*Free) (void * ptr);
    void* (*ReAlloc)(void * ptr, UWORD32 size);
}UniACodecMemoryOps; /* callback operation callback table */

typedef struct
{
  UWORD32 samplerate;
  UWORD32 width;
  UWORD32 depth;
  UWORD32 channels;
  UWORD32 endian;
  bool interleave;
  UWORD32 layout[UA_CHANNEL_MAX];
  bool chan_pos_set;  // indicate if channel position is set outside or use codec default
}UniAcodecOutputPCMFormat;


typedef struct 
{
     UWORD32 size;		/* The size in bytes of the data in this buffer */
     char * buf;		/* Buffer pointer */
}UniACodecParameterBuffer;

typedef struct
{   UWORD32 size;
    UWORD32* channel_table[10]; //assume the max channel is less than 10
}CHAN_TABLE;

typedef struct
{
#if !defined(RVDS)
    union {
#endif
        UWORD32 samplerate;
        UWORD32 channels;
        UWORD32 bitrate;
        UWORD32 depth;
        UWORD32 blockalign;
        UWORD32 version;
        bool downmix;
        bool framed;
        UniACodecParameterBuffer codecData;
        STREAM_TYPE stream_type;
        CHAN_TABLE chan_map_tab;

        /* for real audio decoder */
        UWORD32 frame_bits;
        UWORD32 flavor_index;
		
        char ** codecDesc;
        UniAcodecOutputPCMFormat outputFormat;
        UWORD32 consumed_length;
        UWORD32 outbuf_alloc_size;

/* defined for dsp */
        UWORD32 chanmap;
        UWORD32 mono_to_stereo;
        UWORD32 layers;

        /* dedicate for mp3 and mp2 dec */
        UWORD32 crc_check;
        UWORD32 mch_enable;

        /* dedicate for bsac, aacplus and dabplus dec */
        UWORD32 bdownsample;
        UWORD32 bbitstreamdownmix;
        UWORD32 chanrouting;

        /*dedicate for sbc enc */
        UWORD32 enc_subbands;
        UWORD32 enc_blocks;
        UWORD32 enc_snr;
        UWORD32 enc_bitpool;
        UWORD32 enc_chmode;

        /* dedicate for mp3 dec only */
        UWORD32 nonstd_strm_support;

		/* for Cadence wrapper */
		UWORD32 codec_id;
		UWORD32 codec_entry_addr;

		/* for debug */
		UWORD32 (*Printf)(char* fmt, ...);
/* end */
#if !defined(RVDS)
    };
#endif
}UniACodecParameter;


/* typedef struct
{
    UWORD32 (*wma10dec_callback) (void *state, uint64 offset, UWORD32 * num_bytes, UWORD8 **ppData, void* pAppContext, UWORD32 *compress_payload);

}UniACodecCallbackOps;
*/
/*********************************************************************************************************
 *                  API Funtion Prototypes List
 *
 * There are mandotory and optional APIs.
 * A core wrapper must implent the mandory APIs while need not implement the optional one.
 * And in its DLL entry point "UniACodecInit", it shall set the not-implemented function pointers to NULL.
 *
 *********************************************************************************************************/
/************************************************************************************************************
 *
 *               DLL entry point (mandatory) - to query ACodec interface
 *
 ************************************************************************************************************/
/* prototype of entry point */
typedef WORD32 (*tUniACodecQueryInterface)(UWORD32 id, void ** func);

/*
Every core ACodec shall implement this function and tell a specific API function pointer.
If the queried API is not implemented, the ACodec shall set funtion pointer to NULL and return ACODEC_SUCCESS. */

EXTERN WORD32 UniACodecQueryInterface(UWORD32 id, void ** func);


/*******************************************************************
 * Codec Version Info
*******************************************************************/
typedef const char * (*UniACodecVersionInfo)();

/*******************************************************************
 * Codec  Create & Delete
*******************************************************************/
typedef UniACodec_Handle (*UniACodecCreate)(  UniACodecMemoryOps * memOps);
typedef UniACodec_Handle (*UniACodecCreatePlus)(  UniACodecMemoryOps * memOps, AUDIOFORMAT type);

typedef WORD32 (*UniACodecDelete) (UniACodec_Handle pua_handle);

typedef WORD32 (*UniACodecReset) (UniACodec_Handle pua_handle);

/*******************************************************************
 * Codec  Initializaation
*******************************************************************/
/*typedef WORD32  (*UniACodecInit) (UniACodec_Handle pua_handle,
                                    UWORD8 * InputBuffer,
                                    UWORD32 inputLength,
                                    UWORD8 * codec_data);
*/
/*******************************************************************
 * Codec Query Memory
*******************************************************************/
//typedef WORD32  (*UniACodecQueryMem) (UniACodec_Config pADec_config);

/*******************************************************************
 * Codec set  & get parameter
*******************************************************************/
typedef WORD32 (*UniACodecSetParameter) (UniACodec_Handle pua_handle, UA_ParaType ParaType, UniACodecParameter * parameter);

typedef WORD32 (*UniACodecGetParameter) (UniACodec_Handle pua_handle, UA_ParaType ParaType, UniACodecParameter * parameter);

/*******************************************************************
 * Codec decode & encode frame
*******************************************************************/
typedef WORD32 (*UniACodec_decode_frame) (UniACodec_Handle pua_handle,
                                         UWORD8 * InputBuf,
                                         UWORD32 InputSize,
                                         UWORD32 * offset,
                                         UWORD8 ** OutputBuf,
                                         UWORD32 *OutputSize);

/*
typedef WORD32 (*UniACodec_encode_frame) (UniACodec_Handle pua_handle,
                                         UWORD8 * InputBuf,
                                         UWORD32 InputLength,
                                         UWORD32 * out_buf
                                         UWORD32 end_flag
                                         UA_ERROR_TYPE error_ret);
*/

typedef char * (*UniACodec_get_last_error) (UniACodec_Handle pua_handle);

/*******************************************************************
 *
API function ID
*******************************************************************/

enum /* API function ID */
{
    ACODEC_API_GET_VERSION_INFO  = 0x0,
    /* creation & deletion */
    ACODEC_API_CREATE_CODEC     = 0x1,
    ACODEC_API_CREATE_CODEC_PLUS  = 0x04,
    ACODEC_API_DELETE_CODEC     = 0x2,
    /* reset */
    ACODEC_API_RESET_CODEC = 0x3,

    /* set parameter */
    ACODEC_API_SET_PARAMETER  = 0x10,
    ACODEC_API_GET_PARAMETER  = 0x11,

    /* process frame */
    ACODEC_API_DEC_FRAME    = 0x20,
    ACODEC_API_ENC_FRAME    = 0x21,

    ACODEC_API_GET_LAST_ERROR = 0x1000,

};



#endif /* _Uni_ADEC_WRAPPER_H_ */

