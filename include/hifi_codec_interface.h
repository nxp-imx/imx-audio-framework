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

/*
 * In order to avoid license problem of Cadence header files, this
 * header file is used to wrapper the Cadence codec's header files
 * and provide new interface for the hifi framework to use.
 */

#ifndef _HIFI_CODEC_INTERFACE_H_
#define _HIFI_CODEC_INTERFACE_H_

#include <stdarg.h>

typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
typedef int int32;
typedef short int16;
typedef char int8;

#ifndef bool
	#define bool int
#endif

#ifndef TRUE
	#define TRUE 1
#endif

#ifndef FALSE
	#define FALSE 0
#endif

#ifndef NULL
	#define NULL  (void *)0
#endif


typedef void * HiFiCodec_Handle;


typedef enum
{
    XA_SUCCESS = 0,

    XA_ERROR_STREAM,
    XA_PARA_ERROR,
    XA_INSUFFICIENT_MEM,
    XA_ERR_UNKNOWN,
    XA_PROFILE_NOT_SUPPORT,
    XA_INIT_ERR,
    XA_NO_OUTPUT,

    XA_NOT_ENOUGH_DATA = 0x100,
    XA_CAPIBILITY_CHANGE = 0x200, /* output format changes, caller should reget format from getparameter API */
    XA_END_OF_STREAM = 0x300, /* no output */
} HIFI_ERROR_TYPE;

/* Parameter type to Set /Get */
typedef enum
{
/* Set parmameters */
/* common  */
    XA_SAMPLERATE= 0,
    XA_CHANNEL,
    XA_FRAMED,        /* one whole frame input */
    XA_DEPTH,
    XA_CODEC_DATA,
    XA_BITRATE,
    XA_DOWNMIX_STEREO,
    XA_STREAM_TYPE,
    XA_CHAN_MAP_TABLE,
    //UNIA_CHANNEL_MASK,
    XA_TO_STEREO,

/* dedicate for mp3 dec */
    XA_MP3_DEC_CRC_CHECK = 0x120,
    XA_MP3_DEC_MCH_ENABLE,
    XA_MP3_DEC_NONSTD_STRM_SUPPORT,

/* dedicate for bsac dec */
    XA_BSAC_DEC_DECODELAYERS = 0x130,

/* dedicate for aacplus dec */
    XA_AACPLUS_DEC_BDOWNSAMPLE = 0x140,
    XA_AACPLUS_DEC_BBITSTREAMDOWNMIX,
    XA_AACPLUS_DEC_CHANROUTING,

/* dedicate for dabplus dec */
    XA_DABPLUS_DEC_BDOWNSAMPLE = 0x150,
    XA_DABPLUS_DEC_BBITSTREAMDOWNMIX,
    XA_DABPLUS_DEC_CHANROUTING,

/* dedicate for sbc enc */
    XA_SBC_ENC_SUBBANDS = 0x160,
    XA_SBC_ENC_BLOCKS,
    XA_SBC_ENC_SNR,
    XA_SBC_ENC_BITPOOL,
    XA_SBC_ENC_CHMODE,

} HIFI_ParaType;


#define XA_STREAM_DABPLUS_BASE  0x30
typedef enum
{
    /* AAC/AACPLUS file format */
    XA_STREAM_UNKNOWN = 0,
    XA_STREAM_ADTS,
    XA_STREAM_ADIF,
    XA_STREAM_RAW,

    XA_STREAM_LATM,
    XA_STREAM_LATM_OUTOFBAND_CONFIG,
    XA_STREAM_LOAS,

    /* DABPLUS file format */
    XA_STREAM_DABPLUS_RAW_SIDEINFO = XA_STREAM_DABPLUS_BASE,
    XA_STREAM_DABPLUS,

    /* BSAC file raw format */
    XA_STREAM_BSAC_RAW

} HIFI_StreamType;

/* sbc_enc-specific channel modes */
typedef enum {
    XA_CHMODE_MONO =   0,
    XA_CHMODE_DUAL =   1,
    XA_CHMODE_STEREO = 2,
    XA_CHMODE_JOINT =  3
}HIFI_SbcEncChmode;

typedef enum
{
	CODEC_MP3_DEC = 1,
	CODEC_AAC_DEC,
	CODEC_DAB_DEC,
	CODEC_MP2_DEC,
	CODEC_BSAC_DEC,
	CODEC_DRM_DEC,
	CODEC_SBC_DEC,
	CODEC_SBC_ENC,
	CODEC_DEMO_DEC,

}AUDIOFORMAT;

/*********************************************************************
 * Uni Audio memory callback funtion pointer table.
 *********************************************************************/
typedef struct
{
    void* (*Malloc) (void *hifi_config, uint32 size);
    void  (*Free) (void *hifi_config, void * ptr);

    int   (*hifi_printf)(const char *fmt, ...);

    void *p_xa_process_api;
    void *hifi_config;
} HiFiCodecMemoryOps; /* callback operation callback table */


typedef struct
{
    int32 cmd;
    int32 val;
} HiFiCodecSetParameter;


typedef struct
{
    uint32 pcmbytes;
    uint32 sfreq;
    uint32 channels;
    uint32 bits;
    uint32 consumed_bytes;
    uint32 cycles;
} HiFiCodecGetParameter;


int32 UniACodecQueryInterface(uint32 id, void ** func);


typedef int32 (*tUniACodecQueryInterface)(uint32 id, void ** func);

typedef const char * (*UniACodecVersionInfo)();
typedef HiFiCodec_Handle (*UniACodecCreate)(HiFiCodecMemoryOps * memOps, AUDIOFORMAT type);
typedef int32 (*UniACodecInit)(HiFiCodec_Handle pua_handle);
typedef int32 (*UniACodecDelete)(HiFiCodec_Handle pua_handle);
typedef int32 (*UniACodecReset) (HiFiCodec_Handle pua_handle);
typedef int32 (*UniACodecSetParameter) (HiFiCodec_Handle pua_handle, HiFiCodecSetParameter * parameter);
typedef int32 (*UniACodecGetParameter) (HiFiCodec_Handle pua_handle, HiFiCodecGetParameter * parameter);
typedef int32 (*UniACodec_decode_frame) (HiFiCodec_Handle pua_handle,
                                         uint8 * InputBuf,
                                         uint32 InputSize,
                                         uint32 * offset,
                                         uint8 ** OutputBuf,
                                         uint32 *OutputSize,
                                         uint32 input_over);

typedef char * (*UniACodec_get_last_error) (HiFiCodec_Handle pua_handle);


/*******************************************************************
 *
API function ID
*******************************************************************/

enum /* API function ID */
{
    ACODEC_API_GET_VERSION_INFO  = 0x0,
    /* creation & open */
    ACODEC_API_CREATE_CODEC     = 0x1,
    ACODEC_API_INIT_CODEC     = 0x2,
    /* reset */
    ACODEC_API_RESET_CODEC = 0x3,
	/* delete */
    ACODEC_API_DELETE_CODEC = 0x4,

    /* set parameter */
    ACODEC_API_SET_PARAMETER  = 0x10,
    ACODEC_API_GET_PARAMETER  = 0x11,

    /* process frame */
    ACODEC_API_DECODE_FRAME  = 0x20,

    ACODEC_API_GET_LAST_ERROR = 0x1000,

};

#endif /* _HIFI_CODEC_INTERFACE_H_ */

