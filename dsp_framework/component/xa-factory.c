/*******************************************************************************
 * Copyright (C) 2017 Cadence Design Systems, Inc.
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

 ******************************************************************************/

/*******************************************************************************
 * xa-factory.c
 *
 * DSP processing framework core - component factory
 *
 ******************************************************************************/

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "mydefs.h"
#include "dsp_codec_interface.h"
#include "xf-audio-apicmd.h"

/*******************************************************************************
 * Tracing tags
 ******************************************************************************/

/*******************************************************************************
 * Local types definitions
 ******************************************************************************/

/* ...component descriptor */
struct xf_component_id {
	/* ...class id (string identifier) */
	const char         *id;

	/* ...audio codec type */
	u32 type;

	/* ...class constructor */
	struct xf_component * (*factory)(struct dsp_main_struct *dsp_config,
					 xf_codec_func_t *process,
					 u32 type);

	/* ...component API function */
	xf_codec_func_t *process;

};

/*******************************************************************************
 * External functions
 ******************************************************************************/

/* ...components API functions */
extern u32 xf_unia_codec(xf_codec_handle_t handle,
			 u32 i_cmd,
			 u32 i_idx,
			 void *pv_value);

/* ...component class factories */
extern struct xf_component *xa_audio_codec_factory(struct dsp_main_struct *dsp_config,
						   xf_codec_func_t *process,
						   u32 type);

/*******************************************************************************
 * Local constants definitions
 ******************************************************************************/

/* ...component class id */
static const struct xf_component_id xf_component_id[] = {
	{ "audio-decoder/mp3",      CODEC_MP3_DEC,     xa_audio_codec_factory,     xf_unia_codec },
	{ "audio-decoder/aac",      CODEC_AAC_DEC,     xa_audio_codec_factory,     xf_unia_codec },
	{ "audio-decoder/bsac",     CODEC_BSAC_DEC,    xa_audio_codec_factory,     xf_unia_codec },
	{ "audio-decoder/dabplus",  CODEC_DAB_DEC,     xa_audio_codec_factory,     xf_unia_codec },
	{ "audio-decoder/mp2",      CODEC_MP2_DEC,     xa_audio_codec_factory,     xf_unia_codec },
	{ "audio-decoder/drm",      CODEC_DRM_DEC,     xa_audio_codec_factory,     xf_unia_codec },
	{ "audio-decoder/sbc",      CODEC_SBC_DEC,     xa_audio_codec_factory,     xf_unia_codec },
	{ "audio-encoder/sbc",      CODEC_SBC_ENC,     xa_audio_codec_factory,     xf_unia_codec },
};

/* ...number of items in the map */
#define XF_COMPONENT_ID_MAX (sizeof(xf_component_id) / sizeof(xf_component_id[0]))

/*******************************************************************************
 * Enry points
 ******************************************************************************/

struct xf_component *xf_component_factory(struct dsp_main_struct *dsp_config,
					  const char *id,
					  u32 length)
{
	u32     i;

	/* ...find component-id in static map */
	for (i = 0; i < XF_COMPONENT_ID_MAX; i++) {
		/* ...symbolic search; would prefer GUIDs in some form */
		if (!strncmp(id, xf_component_id[i].id, length)) {
			/* ...pass control to specific class factory */
			return xf_component_id[i].factory(dsp_config,
						xf_component_id[i].process,
						xf_component_id[i].type);
		}
	}

	/* ...component string id is not recognized */
	LOG1("Unknown component type: %s\n", id);

	return NULL;
}
