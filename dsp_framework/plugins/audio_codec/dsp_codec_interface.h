/*
 * Copyright 2018-2020 NXP
 *
 * In order to avoid license problem of Cadence header files, this
 * header file is used to wrapper the Cadence codec's header files
 * and provide new interface for the dsp framework to use.
 */
#ifndef DSP_CODEC_INTERFACE_H
#define DSP_CODEC_INTERFACE_H

#include "fsl_unia.h"

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

#endif
