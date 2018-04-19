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
#include "xf-component.h"
#include "mydefs.h"
#include "dpu_lib_load.h"

/*******************************************************************************
 * Generic codec structure
 ******************************************************************************/

typedef struct XACodecBase XACodecBase;

/* ...memory buffer initialization */
typedef DSP_ERROR_TYPE  (*xa_codec_memtab_f)(XACodecBase *codec, u32 size, u32 align);

/* ...preprocessing operation */
typedef DSP_ERROR_TYPE  (*xa_codec_preprocess_f)(XACodecBase *);

/* ...processing operation */
typedef DSP_ERROR_TYPE  (*xa_codec_process_f)(XACodecBase *, u32 *, u32 *);

/* ...postprocessing operation */
typedef DSP_ERROR_TYPE  (*xa_codec_postprocess_f)(XACodecBase *, u32, u32, u32);

/* ...parameter setting function */
typedef DSP_ERROR_TYPE  (*xa_codec_setparam_f)(XACodecBase *, s32, void *p);

/* ...parameter retrival function */
typedef DSP_ERROR_TYPE  (*xa_codec_getparam_f)(XACodecBase *, s32, void *p);

typedef struct {
	UniACodecVersionInfo    VersionInfo;
	UniACodecCreate         Create;
	UniACodecDelete         Delete;
	UniACodecInit           Init;
	UniACodecReset          Reset;
	UniACodecSetParameter   SetPara;
	UniACodecGetParameter   GetPara;
	UniACodec_decode_frame  Process;
	UniACodec_get_last_error    GetLastError;
}sCodecFun;

/*******************************************************************************
 * Codec instance structure
 ******************************************************************************/

struct XACodecBase
{
    /***************************************************************************
     * Control data
     **************************************************************************/

    /* ...generic component handle */
    xf_component_t          component;

    /* ...global structure pointer */
    dsp_main_struct *dsp_config;

    /* ...codec API entry point (function) */
    tUniACodecQueryInterface codecwrapinterface;

    /* ...codec API handle, passed to *process */
    void *codecinterface;

    /* ...dsp codec wrapper handle */
    DSPCodec_Handle pWrpHdl;

    /* ...dsp codec wrapper api */
    sCodecFun WrapFun;

    /* codec identifier */
    u32 codec_id;

    /* loading library info of codec wrap */
    dpu_lib_stat_t lib_codec_wrap_stat;

    /* loading library info of codec */
    dpu_lib_stat_t lib_codec_stat;

    /* ...codec control state */
    u32                     state;

    /***************************************************************************
     * Codec-specific methods
     **************************************************************************/

    /* ...memory buffer initialization */
    xa_codec_memtab_f       memtab;

    /* ...preprocessing function */
    xa_codec_preprocess_f   preprocess;

    /* ...preprocessing function */
    xa_codec_process_f   process;

    /* ...postprocessing function */
    xa_codec_postprocess_f  postprocess;

    /* ...configuration parameter setting function */
    xa_codec_setparam_f     setparam;

    /* ...configuration parameter retrieval function */
    xa_codec_getparam_f     getparam;

    /* ...command-processing table */
    DSP_ERROR_TYPE (* const * command)(XACodecBase *, xf_message_t *);

    /* ...command-processing table size */
    u32                     command_num;
};

/*******************************************************************************
 * Base codec execution flags
 ******************************************************************************/

/* ...codec pre-initialize completed */
#define XA_BASE_FLAG_PREINIT            (1 << 0)

/* ...codec static initialization completed */
#define XA_BASE_FLAG_POSTINIT           (1 << 1)

/* ...codec runtime initialization sequence */
#define XA_BASE_FLAG_RUNTIME_INIT       (1 << 2)

/* ...codec steady execution state */
#define XA_BASE_FLAG_EXECUTION          (1 << 3)

/* ...execution stage completed */
#define XA_BASE_FLAG_COMPLETED          (1 << 4)

/* ...data processing scheduling flag */
#define XA_BASE_FLAG_SCHEDULE           (1 << 5)

/* ...base codec flags accessor */
#define __XA_BASE_FLAGS(flags)          ((flags) & ((1 << 6) - 1))

/* ...custom execution flag */
#define __XA_BASE_FLAG(f)               ((f) << 6)


/*******************************************************************************
 * Public API
 ******************************************************************************/
/* ...codec hook invocation */
#define CODEC_API(codec, func, ...)                                 \
({                                                                  \
    DSP_ERROR_TYPE __e = (codec)->func((codec), ##__VA_ARGS__);    \
                                                                    \
    if (__e != XA_SUCCESS)                                          \
    {                                                               \
		LOG1("warning: %x\n", __e);                                 \
    }                                                               \
    __e;                                                            \
})

/* ...SET-PARAM processing */
extern DSP_ERROR_TYPE xa_base_set_param(XACodecBase *base, xf_message_t *m);

/* ...GET-PARAM message processing */
extern DSP_ERROR_TYPE xa_base_get_param(XACodecBase *base, xf_message_t *m);

/* ...data processing scheduling */
extern void xa_base_schedule(XACodecBase *base, u32 dts);

/* ...cancel internal scheduling message */
extern void xa_base_cancel(XACodecBase *base);

/* ...base codec factory */
extern XACodecBase * xa_base_factory(dsp_main_struct *dsp_config, u32 size, void *process);

/* ...base codec destructor */
extern void xa_base_destroy(XACodecBase *base);

#endif  /* __XA_CLASS_BASE_H */
