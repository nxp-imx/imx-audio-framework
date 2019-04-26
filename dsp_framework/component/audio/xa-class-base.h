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
 * xa-class-base.h
 *
 * Generic Xtensa Audio codecs interfaces
 *
 ******************************************************************************/

#ifndef __XA_CLASS_BASE_H
#define __XA_CLASS_BASE_H

/*******************************************************************************
 * Includes
 ******************************************************************************/

/* ...audio-specific API */
#include "dsp_codec_interface.h"
#include "fsl_unia.h"
#include "xf-audio-apicmd.h"
#include "xf-component.h"
#include "mydefs.h"
#include "dpu_lib_load.h"

/*******************************************************************************
 * Generic codec structure
 ******************************************************************************/
struct XACodecBase;

/* ...memory buffer initialization */
typedef UA_ERROR_TYPE (*xa_codec_memtab_f)(struct XACodecBase *codec,
							u32 size,
							u32 align);

/* ...preprocessing operation */
typedef UA_ERROR_TYPE  (*xa_codec_preprocess_f)(struct XACodecBase *);

/* ...postprocessing operation */
typedef UA_ERROR_TYPE  (*xa_codec_postprocess_f)(struct XACodecBase *, u32);

/* ...parameter setting function */
typedef UA_ERROR_TYPE  (*xa_codec_setparam_f)(struct XACodecBase *,
								s32,
								void *p);

/* ...parameter retrival function */
typedef UA_ERROR_TYPE  (*xa_codec_getparam_f)(struct XACodecBase *,
								s32,
								void *p);

/*******************************************************************************
 * Codec instance structure
 ******************************************************************************/

struct XACodecBase {
	/*************************************************
	 * Control data
	 ************************************************/

	/* ...generic component handle */
	struct xf_component     component;

	/* ...codec API entry point (function) */
	xf_codec_func_t         *process;

	/* ...codec API handle, passed to *process */
	xf_codec_handle_t       api;

	/* ...codec API entry point (addr) */
	void *                  codecinterface;

	/* codec identifier */
	u32 codec_id;

	/* ...codec control state */
	u32                     state;

	/***********************************************
	 * Codec-specific methods
	 **********************************************/

	/* ...memory buffer initialization */
	xa_codec_memtab_f       memtab;

	/* ...preprocessing function */
	xa_codec_preprocess_f   preprocess;

	/* ...postprocessing function */
	xa_codec_postprocess_f  postprocess;

	/* ...configuration parameter setting function */
	xa_codec_setparam_f     setparam;

	/* ...configuration parameter retrieval function */
	xa_codec_getparam_f     getparam;

	/* ...command-processing table */
	UA_ERROR_TYPE (* const *command)(struct XACodecBase *,
					  struct xf_message *);

	/* ...command-processing table size */
	u32                     command_num;
};

/*******************************************************************************
 * Base codec execution flags
 ******************************************************************************/

/* ...codec pre-initialize completed */
#define XA_BASE_FLAG_PREINIT            (1 << 0)

/* ...codec initialize completed */
#define XA_BASE_FLAG_INIT               (1 << 1)

/* ...codec static initialization completed */
#define XA_BASE_FLAG_POSTINIT           (1 << 2)

/* ...codec runtime initialization sequence */
#define XA_BASE_FLAG_RUNTIME_INIT       (1 << 3)

/* ...codec steady execution state */
#define XA_BASE_FLAG_EXECUTION          (1 << 4)

/* ...execution stage completed */
#define XA_BASE_FLAG_COMPLETED          (1 << 5)

/* ...data processing scheduling flag */
#define XA_BASE_FLAG_SCHEDULE           (1 << 6)

/* ...base codec flags accessor */
#define __XA_BASE_FLAGS(flags)          ((flags) & ((1 << 7) - 1))

/* ...custom execution flag */
#define __XA_BASE_FLAG(f)               ((f) << 7)

/*******************************************************************************
 * Public API
 ******************************************************************************/

/* ...low-level codec API function execution */
#define XA_API(codec, cmd, idx, pv)                                               \
({                                                                                \
	UA_ERROR_TYPE  __e;                                                          \
	__e = (codec)->process((xf_codec_handle_t)((codec)->api), (cmd), (idx), (pv));\
	if (__e != ACODEC_SUCCESS) {                                                      \
		LOG1("XA_API error: %x\n", __e);                                          \
	}                                                                             \
	__e;                                                                          \
})

/* ...codec hook invocation */
#define CODEC_API(codec, func, ...)                                 \
({                                                                  \
	UA_ERROR_TYPE __e;                                             \
	__e = (codec)->func((codec), ##__VA_ARGS__);                    \
	if (__e != ACODEC_SUCCESS) {                                        \
		LOG1(" CODEC_API error: %x\n", __e);                        \
	}                                                               \
	__e;                                                            \
})

/* ...SET-PARAM processing */
UA_ERROR_TYPE xa_base_set_param(struct XACodecBase *base, struct xf_message *m);

/* ...GET-PARAM message processing */
UA_ERROR_TYPE xa_base_get_param(struct XACodecBase *base, struct xf_message *m);

/* ...data processing scheduling */
void xa_base_schedule(struct XACodecBase *base, u32 dts);

/* ...cancel internal scheduling message */
void xa_base_cancel(struct XACodecBase *base);

/* ...base codec factory */
struct XACodecBase *xa_base_factory(struct dsp_main_struct *dsp_config,
				    u32 size,
				    xf_codec_func_t *process,
				    u32 type);

/* ...base codec destructor */
void xa_base_destroy(struct XACodecBase *base);

#endif  /* __XA_CLASS_BASE_H */
