/*
 * Copyright 2018-2020 NXP
 *
 * This header file is used to wrapper the Cadence codec's header files
 * and provide new interface for the dsp framework to use.
 */
#ifndef DSP_CODEC_INTERFACE_H
#define DSP_CODEC_INTERFACE_H

#include "fsl_unia.h"

#ifdef TGT_OS_ANDROID
#define CORE_LIB_PATH   "/vendor/lib/"
#else
#define CORE_LIB_PATH   "/usr/lib/imx-mm/audio-codec/dsp/"
#endif

/* codec comp define range: 0x1 ~ 0x1F */
typedef enum {
	CODEC_MP3_DEC = 1,
	CODEC_AAC_DEC,
	CODEC_DAB_DEC,
	CODEC_MP2_DEC,
	CODEC_BSAC_DEC,
	CODEC_DRM_DEC,
	CODEC_SBC_DEC,
	CODEC_SBC_ENC,
	/* Carefully add fsl codecs for it may cause bad performace
	 * on some platforms.
	*/
	CODEC_FSL_OGG_DEC,
	CODEC_FSL_MP3_DEC,
	CODEC_FSL_AAC_DEC,
	CODEC_FSL_AAC_PLUS_DEC,
	CODEC_FSL_AC3_DEC,
	CODEC_FSL_DDP_DEC,
	CODEC_FSL_NBAMR_DEC,
	CODEC_FSL_WBAMR_DEC,
	CODEC_FSL_WMA_DEC,
	CODEC_OPUS_DEC,
	CODEC_PCM_DEC,
	CODEC_DEMO_DEC,

} AUDIOCODECFORMAT;

typedef struct {
	AUDIOCODECFORMAT           codec_type;
	const char                 *dec_id;
	const char                 *codec_name;
	const char                 *codecwrap_name;
	/* ...Codec data ignored */
	WORD32                     codecdata_ignored;
} CODECINFO;

enum lib_type {
	DSP_CODEC_LIB = 1,
	DSP_CODEC_WRAP_LIB
};

#define CODEC_LIB_MASK          0x3
#define GET_LIB_TYPE(a)         (a & CODEC_LIB_MASK)
#define GET_CODEC_TYPE(a)       ((a >> 2) & 0x1F)

#endif
