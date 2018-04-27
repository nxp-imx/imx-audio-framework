/*******************************************************************************
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
*
******************************************************************************/
#include "xf-types.h"
#include "xf-debug.h"
#include "mydefs.h"
#include "dpu_lib_load.h"
#include "dsp_codec_interface.h"
#include "xf-audio-apicmd.h"

/*******************************************************************************
 * Internal functions definitions
 ******************************************************************************/

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
} sCodecFun;

/* ...API structure */
typedef struct xfuniacodec
{
    /* ...codec identifier */
    u32 codec_id;

    /* ...codec input buffer pointer */
    u8 *inptr;

    /* ...codec output buffer pointer */
    u8 *outptr;

    /* ...codec input data size */
    u32 in_size;

    /* ...codec output data size */
    u32 out_size;

    /* ...codec consumed data size */
    u32 consumed;

    /* ...codec input over flag */
    u32 input_over;

    /* ...codec API entry point (function) */
    tUniACodecQueryInterface codecwrapinterface;

    /* ...codec API handle, passed to *process */
    void *codecinterface;

    /* ...dsp codec wrapper handle */
    DSPCodec_Handle pWrpHdl;

    /* ...dsp codec wrapper api */
    sCodecFun WrapFun;

    /* ...private data pointer */
    void *private_data;

} XFUniaCodec;

/*******************************************************************************
 * Commands processing
 ******************************************************************************/

static DSP_ERROR_TYPE xf_uniacodec_get_api_size(XFUniaCodec *d, u32 i_idx, void *pv_value)
{
    /* ...retrieve API structure size */
    *(u32 *)pv_value = sizeof(*d);

    return XA_SUCCESS;
}

static DSP_ERROR_TYPE xf_uniacodec_preinit(XFUniaCodec *d, u32 i_idx, void *pv_value)
{
    DSP_ERROR_TYPE ret = XA_SUCCESS;

    /* ...set codec id */
    d->codec_id = i_idx;

    /* ...set private data pointer */
    d->private_data = pv_value;

    return ret;
}

static DSP_ERROR_TYPE xf_uniacodec_init(XFUniaCodec *d, u32 i_idx, void *pv_value)
{
    dsp_main_struct *dsp_config = (dsp_main_struct *)d->private_data;
    DSPCodecMemoryOps memops;
    DSP_ERROR_TYPE ret = XA_SUCCESS;

    if(d->codecwrapinterface == NULL)
    {
        LOG("base->codecwrapinterface Pointer is NULL\n");
        return XA_INIT_ERR;
    }

    d->codecwrapinterface(ACODEC_API_CREATE_CODEC, (void **)&d->WrapFun.Create);
    d->codecwrapinterface(ACODEC_API_DELETE_CODEC, (void **)&d->WrapFun.Delete);
    d->codecwrapinterface(ACODEC_API_INIT_CODEC, (void **)&d->WrapFun.Init);
    d->codecwrapinterface(ACODEC_API_RESET_CODEC, (void **)&d->WrapFun.Reset);
    d->codecwrapinterface(ACODEC_API_SET_PARAMETER, (void **)&d->WrapFun.SetPara);
    d->codecwrapinterface(ACODEC_API_SET_PARAMETER, (void **)&d->WrapFun.SetPara);
    d->codecwrapinterface(ACODEC_API_GET_PARAMETER, (void **)&d->WrapFun.GetPara);
    d->codecwrapinterface(ACODEC_API_DECODE_FRAME, (void **)&d->WrapFun.Process);
    d->codecwrapinterface(ACODEC_API_GET_LAST_ERROR, (void **)&d->WrapFun.GetLastError);

    memops.Malloc = (void *)MEM_scratch_malloc;
    memops.Free = (void *)MEM_scratch_mfree;
#ifdef DEBUG
    memops.dsp_printf = NULL;
#endif

    memops.p_xa_process_api = d->codecinterface;
    memops.dsp_config = &dsp_config->scratch_mem_info;

    if(d->WrapFun.Create == NULL)
    {
        LOG("WrapFun.Create Pointer is NULL\n");
        return XA_INIT_ERR;
    }

    d->pWrpHdl = d->WrapFun.Create(&memops, d->codec_id);
    if(d->pWrpHdl == NULL) {
        LOG("Create codec error in codec wrapper\n");
        return XA_INIT_ERR;
    }

    return ret;
}

static DSP_ERROR_TYPE xf_uniacodec_postinit(XFUniaCodec *d, u32 i_idx, void *pv_value)
{
    DSP_ERROR_TYPE ret = XA_SUCCESS;

    if(d->WrapFun.Init == NULL)
    {
        LOG("WrapFun.Init Pointer is NULL\n");
        return XA_INIT_ERR;
    }

    /* ...initialize codec memory */
    ret = d->WrapFun.Init(d->pWrpHdl);

    return ret;
}

static DSP_ERROR_TYPE xf_uniacodec_setparam(XFUniaCodec *d, u32 i_idx, void *pv_value)
{
    DSPCodecSetParameter param;
    DSP_ERROR_TYPE ret = XA_SUCCESS;

    if(d->WrapFun.SetPara == NULL)
    {
        LOG("WrapFun.SetPara Pointer is NULL\n");
        return XA_INIT_ERR;
    }

    param.cmd = i_idx;
    param.val = *(u32 *)pv_value;

    ret = d->WrapFun.SetPara(d->pWrpHdl, &param);

    return ret;
}

static DSP_ERROR_TYPE xf_uniacodec_getparam(XFUniaCodec *d, u32 i_idx, void *pv_value)
{
    DSPCodecGetParameter param;
    DSP_ERROR_TYPE ret = XA_SUCCESS;

    if(d->WrapFun.GetPara == NULL)
    {
        LOG("WrapFun.GetPara Pointer is NULL\n");
        return XA_INIT_ERR;
    }

    /* ...retrieve the collection of codec  parameters */
    ret = d->WrapFun.GetPara(d->pWrpHdl, &param);
    if(ret)
        return XA_PARA_ERROR;

    /* ...retrieve the collection of codec  parameters */
    switch(i_idx)
    {
        case XA_SAMPLERATE:
            *(u32 *)pv_value = param.sfreq;
            break;
        case XA_CHANNEL:
            *(u32 *)pv_value = param.channels;
            break;
        case XA_DEPTH:
            *(u32 *)pv_value = param.bits;
            break;
        case XA_CONSUMED_LENGTH:
            *(u32 *)pv_value = param.consumed_bytes;
            break;
        case XA_CONSUMED_CYCLES:
            *(u32 *)pv_value = param.cycles;
            break;
        default:
            *(u32 *)pv_value = 0;
            break;
    }

    return ret;
}

static DSP_ERROR_TYPE xf_uniacodec_execute(XFUniaCodec *d, u32 i_idx, void *pv_value)
{
    DSP_ERROR_TYPE ret = XA_SUCCESS;
    u32 offset = 0;

    if(d->WrapFun.Process == NULL)
    {
        LOG("WrapFun.Process Pointer is NULL\n");
        return XA_INIT_ERR;
    }

    LOG4("in_buf = %x, in_size = %x, offset = %d, out_buf = %x\n", d->inptr, d->in_size, d->consumed, d->outptr);
    ret = d->WrapFun.Process(d->pWrpHdl,
                                d->inptr,
                                d->in_size,
                                &offset,
                                (u8 **)&d->outptr,
                                &d->out_size,
                                d->input_over);

    d->consumed = offset;

    LOG3("xa_codec_process successfully: consumed = %d, produced = %d, ret = %d\n", d->consumed, d->out_size, ret);

    return ret;
}

static DSP_ERROR_TYPE xf_uniacodec_set_input_ptr(XFUniaCodec *d, u32 i_idx, void *pv_value)
{
    DSP_ERROR_TYPE ret = XA_SUCCESS;

    d->inptr = (u8 *)pv_value;

    return XA_SUCCESS;
}

static DSP_ERROR_TYPE xf_uniacodec_set_output_ptr(XFUniaCodec *d, u32 i_idx, void *pv_value)
{
    DSP_ERROR_TYPE ret = XA_SUCCESS;

    d->outptr = (u8 *)pv_value;

    return XA_SUCCESS;
}

static DSP_ERROR_TYPE xf_uniacodec_set_input_bytes(XFUniaCodec *d, u32 i_idx, void *pv_value)
{
    DSP_ERROR_TYPE ret = XA_SUCCESS;

    d->in_size = *(u32 *)pv_value;

    return XA_SUCCESS;
}

static DSP_ERROR_TYPE xf_uniacodec_get_output_bytes(XFUniaCodec *d, u32 i_idx, void *pv_value)
{
    DSP_ERROR_TYPE ret = XA_SUCCESS;

    *(u32 *)pv_value = d->out_size;

    return XA_SUCCESS;
}

static DSP_ERROR_TYPE xf_uniacodec_get_consumed_bytes(XFUniaCodec *d, u32 i_idx, void *pv_value)
{
    DSP_ERROR_TYPE ret = XA_SUCCESS;

    *(u32 *)pv_value = d->consumed;

    return XA_SUCCESS;
}

static DSP_ERROR_TYPE xf_uniacodec_input_over(XFUniaCodec *d, u32 i_idx, void *pv_value)
{
    DSP_ERROR_TYPE ret = XA_SUCCESS;

    d->input_over = 1;

    return XA_SUCCESS;
}

static DSP_ERROR_TYPE xf_uniacodec_set_lib_entry(XFUniaCodec *d, u32 i_idx, void *pv_value)
{
    DSP_ERROR_TYPE ret = XA_SUCCESS;

    if(i_idx == DSP_CODEC_WRAP_LIB)
        d->codecwrapinterface = (tUniACodecQueryInterface)pv_value;
    else if(i_idx == DSP_CODEC_LIB)
        d->codecinterface = pv_value;
    else
    {
        LOG("Unknown lib type\n");
        ret = XA_INIT_ERR;
    }

    return ret;
}

static DSP_ERROR_TYPE xf_uniacodec_runtime_init(XFUniaCodec *d, u32 i_idx, void *pv_value)
{
    DSP_ERROR_TYPE ret = XA_SUCCESS;

    if(d->WrapFun.Reset)
        ret = d->WrapFun.Reset(d->pWrpHdl);

    d->input_over = 0;
    d->in_size = 0;
    d->consumed = 0;

    return ret;
}

static DSP_ERROR_TYPE xf_uniacodec_cleanup(XFUniaCodec *d, u32 i_idx, void *pv_value)
{
    DSP_ERROR_TYPE ret = XA_SUCCESS;

    /* ...destory codec resources */
    if(d->WrapFun.Delete)
        ret = d->WrapFun.Delete(d->pWrpHdl);

    return ret;
}

/*******************************************************************************
 * API command hooks
 ******************************************************************************/

static DSP_ERROR_TYPE (* const xf_unia_codec_api[])(XFUniaCodec *, u32, void *) =
{
    [XF_API_CMD_GET_API_SIZE]      = xf_uniacodec_get_api_size,
    [XF_API_CMD_PRE_INIT]          = xf_uniacodec_preinit,
    [XF_API_CMD_INIT]              = xf_uniacodec_init,
    [XF_API_CMD_POST_INIT]         = xf_uniacodec_postinit,
    [XF_API_CMD_SET_PARAM]         = xf_uniacodec_setparam,
    [XF_API_CMD_GET_PARAM]         = xf_uniacodec_getparam,
    [XF_API_CMD_EXECUTE]           = xf_uniacodec_execute,
    [XF_API_CMD_SET_INPUT_PTR]     = xf_uniacodec_set_input_ptr,
    [XF_API_CMD_SET_OUTPUT_PTR]    = xf_uniacodec_set_output_ptr,
    [XF_API_CMD_SET_INPUT_BYTES]   = xf_uniacodec_set_input_bytes,
    [XF_API_CMD_GET_OUTPUT_BYTES]  = xf_uniacodec_get_output_bytes,
    [XF_API_CMD_GET_CONSUMED_BYTES]  = xf_uniacodec_get_consumed_bytes,
    [XF_API_CMD_INPUT_OVER]        = xf_uniacodec_input_over,
    [XF_API_CMD_RUNTIME_INIT]      = xf_uniacodec_runtime_init,
    [XF_API_CMD_CLEANUP]           = xf_uniacodec_cleanup,
    [XF_API_CMD_SET_LIB_ENTRY]     = xf_uniacodec_set_lib_entry,
};

/* ...total numer of commands supported */
#define XF_UNIACODEC_API_COMMANDS_NUM   (sizeof(xf_unia_codec_api) / sizeof(xf_unia_codec_api[0]))

/*******************************************************************************
 * API entry point
 ******************************************************************************/

u32 xf_unia_codec(xf_codec_handle_t handle, u32 i_cmd, u32 i_idx, void *pv_value)
{
    XFUniaCodec *uniacodec = (XFUniaCodec *)handle;

    /* ...check if command index is sane */
    XF_CHK_ERR(i_cmd < XF_UNIACODEC_API_COMMANDS_NUM, XA_PARA_ERROR);

    /* ...see if command is defined */
    XF_CHK_ERR(xf_unia_codec_api[i_cmd] != NULL, XA_PARA_ERROR);

    /* ...execute requested command */
    return xf_unia_codec_api[i_cmd](uniacodec, i_idx, pv_value);
}
