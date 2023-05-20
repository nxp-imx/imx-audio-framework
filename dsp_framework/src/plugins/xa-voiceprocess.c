/*
* Copyright 2023 NXP
* SPDX-License-Identifier: BSD-3-Clause
*
*/
/*******************************************************************************
 * xa-voiceprocess.c
 *
 * Voice Process plugin
 ******************************************************************************/

#define MODULE_TAG                     VOICE_PROCESS

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include <stdint.h>
#include <string.h>

#include <xtensa/tie/xt_timer.h>

#include "audio/xa-mimo-proc-api.h"
#include "xa-voiceprocess-api.h"

#ifdef DEBUG
#include "xf-debug.h"
#include "debug.h"
#include "printf.h"
#endif

#ifdef XAF_PROFILE
#include "xaf-clk-test.h"
extern clk_t voice_proc_cycles;
#endif

#include "mydefs.h"

#include "fsl_unia.h"
#include "dsp_codec_interface.h"
#include "memory.h"
#include "dpu_lib_load.h"
/*******************************************************************************
 * Internal plugin-specific definitions
 ******************************************************************************/

#define XA_VOC_PROC_CFG_DEFAULT_PCM_WIDTH	16
#define XA_VOC_PROC_CFG_DEFAULT_CHANNELS	2
#define XA_VOC_PROC_CFG_FRAME_SIZE_BYTES	1024*4

#define XA_VOC_PROC_CFG_PERSIST_SIZE	4
#define XA_VOC_PROC_CFG_SCRATCH_SIZE	4

#define XA_MIMO_IN_PORTS		2
#define XA_MIMO_OUT_PORTS		2

#define _MAX(a, b)	(((a) > (b))?(a):(b))
#define _MIN(a, b)	(((a) < (b))?(a):(b))

/*******************************************************************************
 * Internal functions definitions
 ******************************************************************************/

/* ...API structure */

typedef uint32_t (*Voice_proc_execute)(void *p_voice_handle, void *input[2], int input_length[2],
		int *consumed[2], void *output[2], int *produced[2]);

struct VoiceWrapFun {
	UniACodecVersionInfo    VersionInfo;
	UniACodecCreate         Create;
	UniACodecDelete         Delete;
	UniACodecReset          Reset;
	UniACodecSetParameter   SetPara;
	UniACodecGetParameter   GetPara;
	Voice_proc_execute      Process;
	UniACodec_get_last_error    GetLastError;
};

typedef struct XAVoiceProc
{
    /* ... state */
    UWORD32                 state;

    /* ...sampling rate */
    UWORD32                 sample_rate;

    /* ... input port parameters */
    /* ...number of bytes in input buffer */
    UWORD32                 in_buffer_size;

    /* ...input buffers */
    void               	    *input[XA_MIMO_IN_PORTS];

    UWORD32                 consumed[XA_MIMO_IN_PORTS];

    /* ...number of valid samples in individual buffers */
    UWORD32                 input_length[XA_MIMO_IN_PORTS];

    /* ...number of input ports */
    UWORD32                 num_in_ports;


    /* ... output port parameters */
    /* ...number of bytes in input/output buffer */
    UWORD32                 out_buffer_size;

    /* ...output buffer */
    void               	    *output[XA_MIMO_OUT_PORTS];

    UWORD32                 produced[XA_MIMO_OUT_PORTS];

    UWORD32                 num_out_ports; /* ... number of output ports active */

    /* ...scratch buffer pointer */
    void               	    *scratch;
    void               	    *persist;

    UWORD32		    persist_size;
    UWORD32		    scratch_size;

    UWORD32                 pcm_width;
    UWORD32                 channels;

    UWORD32                 frame_size;
    UWORD32                 port_pcm_width[XA_MIMO_IN_PORTS + XA_MIMO_OUT_PORTS];

    WORD16		    port_state[XA_MIMO_IN_PORTS + XA_MIMO_OUT_PORTS];

    void                    *p_voice_handler;

    struct dpu_lib_stat_t lib_voice_wrap_stat;

    tUniACodecQueryInterface voice_wrap_interface;

    struct VoiceWrapFun WrapFun;
    UniACodec_Handle pWrpHdl;

}   XAVoiceProc;

/*******************************************************************************
 *  state flags
 ******************************************************************************/

#define XA_VOC_PROC_FLAG_PREINIT_DONE      (1 << 0)
#define XA_VOC_PROC_FLAG_POSTINIT_DONE     (1 << 1)
#define XA_VOC_PROC_FLAG_RUNNING           (1 << 2)
#define XA_VOC_PROC_FLAG_OUTPUT            (1 << 3)
#define XA_VOC_PROC_FLAG_COMPLETE          (1 << 4)
#define XA_VOC_PROC_FLAG_EXEC_DONE         (1 << 5)
#define XA_VOC_PROC_FLAG_PORT_PAUSED       (1 << 6)
#define XA_VOC_PROC_FLAG_PORT_CONNECTED    (1 << 7)

#define MAX_16BIT (32767)
#define MIN_16BIT (-32768)

/* ... preinitialization (default parameters) */
static inline void xa_voice_proc_preinit(XAVoiceProc *d)
{
    /* ...pre-configuration initialization; reset internal data */
    memset(d, 0, sizeof(*d));

    /* ...set default parameters */
    d->num_in_ports 	= XA_MIMO_IN_PORTS;
    d->num_out_ports 	= XA_MIMO_OUT_PORTS;

    d->sample_rate	= 48000;
    d->pcm_width 	= XA_VOC_PROC_CFG_DEFAULT_PCM_WIDTH;
    d->channels 	= XA_VOC_PROC_CFG_DEFAULT_CHANNELS;

    d->frame_size       = 1024;

    d->in_buffer_size 	= XA_VOC_PROC_CFG_FRAME_SIZE_BYTES;
    d->out_buffer_size 	= XA_VOC_PROC_CFG_FRAME_SIZE_BYTES;
    d->persist_size 	= XA_VOC_PROC_CFG_PERSIST_SIZE;
    d->scratch_size 	= XA_VOC_PROC_CFG_SCRATCH_SIZE;
}

static XA_ERRORCODE xa_voice_proc_init(XAVoiceProc *d)
{
    return XA_NO_ERROR;
}

static XA_ERRORCODE xa_voice_proc_lib_execute(XAVoiceProc *d)
{
	uint32_t ret = XA_NO_ERROR;
	if (!d->WrapFun.Process)
		return XA_FATAL_ERROR;
	int *consumed[2];
	int *produced[2];

	if (d->input_length[0] < d->frame_size * d->channels * d->port_pcm_width[0]/8 ||
	    d->input_length[1] < d->frame_size * d->channels * d->port_pcm_width[1]/8)
		return XA_NO_ERROR;

	consumed[0] = &d->consumed[0];
	consumed[1] = &d->consumed[1];
	produced[0] = &d->produced[0];
	produced[1] = &d->produced[1];

	ret = d->WrapFun.Process(d->pWrpHdl, d->input, d->input_length, consumed, d->output, produced);

	d->input_length[0] -= d->consumed[0];
	d->input_length[1] -= d->consumed[1];

	if (d->produced[0] || d->produced[1])
		d->state |= XA_VOC_PROC_FLAG_OUTPUT;

	return ret;
}

static XA_ERRORCODE xa_voice_proc_do_execute(XAVoiceProc *d)
{
    WORD32     i;
    WORD32     product0, product1;

    if((d->num_in_ports == 2) && (d->num_out_ports == 2))
    {
      /* 2 in, 2 out */
      WORD16    *pIn0 = (WORD16 *) d->input[0];
      WORD16    *pIn1 = (WORD16 *) d->input[1];
      WORD16    *pOut0 = (WORD16 *) d->output[0];
      WORD16    *pOut1 = (WORD16 *) d->output[1];

      /* reset consumed/produced counters */
      for (i = 0;i < (d->num_in_ports); i++)
        d->consumed[i] = 0;

      for (i = 0;i < (d->num_out_ports); i++)
        d->produced[i] = 0;

      TRACE(PROCESS, _b("in length:(%d, %d)"), d->input_length[0], d->input_length[1]);

      if( ((d->port_state[0] & XA_VOC_PROC_FLAG_COMPLETE) && (d->input_length[0] == 0)) &&
          ((d->port_state[1] & XA_VOC_PROC_FLAG_COMPLETE) && (d->input_length[1] == 0))
      )
      {
        d->state |= XA_VOC_PROC_FLAG_EXEC_DONE;
        return XA_NO_ERROR;
      }

      xa_voice_proc_lib_execute(d);

      TRACE(PROCESS, _b("consumed: (%u, %u) bytes, produced: (%u, %u) bytes (%u samples)"), d->consumed[0], d->consumed[1], d->produced[0], d->produced[1]);

      if(d->port_state[0] & XA_VOC_PROC_FLAG_COMPLETE)
      {
        d->state |= XA_VOC_PROC_FLAG_EXEC_DONE;
      }
    }
    else
    {
        TRACE(ERROR, _x("unsupported input(%d)/output(%d) port numbers"), d->num_in_ports, d->num_out_ports);
        return XA_VOICE_PROC_EXEC_FATAL_STATE;
    }

    /* ...return success result code */
    return XA_NO_ERROR;
}

/* ...runtime reset */
static XA_ERRORCODE xa_voice_proc_do_runtime_init(XAVoiceProc *d)
{
    /* ...no special processing is needed here */
    return XA_NO_ERROR;
}

/*******************************************************************************
 * Commands processing
 ******************************************************************************/
/* ...codec API size query */
static XA_ERRORCODE xa_vp_get_api_size(XAVoiceProc *d, WORD32 i_idx, pVOID pv_value)
{
    /* ...check parameters are sane */
    XF_CHK_ERR(pv_value, XA_API_FATAL_INVALID_CMD_TYPE);
    /* ...retrieve API structure size */
    *(WORD32 *)pv_value = sizeof(*d);

    return XA_NO_ERROR;
}

/* ...standard codec initialization routine */
static XA_ERRORCODE xa_vp_init(XAVoiceProc *d, WORD32 i_idx, pVOID pv_value)
{
    /* ...sanity check - vp must be valid */
    XF_CHK_ERR(d, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...process particular initialization type */
    switch (i_idx)
    {
    case XA_CMD_TYPE_INIT_API_PRE_CONFIG_PARAMS:
    {
        /* ...pre-configuration initialization; reset internal data */
        xa_voice_proc_preinit(d);

        /* ...and mark vp has been created */
        d->state = XA_VOC_PROC_FLAG_PREINIT_DONE;

        return XA_NO_ERROR;
    }

    case XA_CMD_TYPE_INIT_API_POST_CONFIG_PARAMS:
    {
        /* ...post-configuration initialization (all parameters are set) */
        XF_CHK_ERR(d->state & XA_VOC_PROC_FLAG_PREINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

        /* ...calculate input/output buffer size in bytes */
        //d->in_buffer_size = d->channels * d->frame_size * (d->pcm_width == 16 ? sizeof(WORD16) : sizeof(WORD32));

        xa_voice_proc_init(d);

        /* ...mark post-initialization is complete */
        d->state |= XA_VOC_PROC_FLAG_POSTINIT_DONE;

        return XA_NO_ERROR;
    }

    case XA_CMD_TYPE_INIT_PROCESS:
    {
        /* ...kick run-time initialization process; make sure vp is setup */
        XF_CHK_ERR(d->state & XA_VOC_PROC_FLAG_POSTINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

        /* ...enter into execution stage */
        d->state |= XA_VOC_PROC_FLAG_RUNNING;

        return XA_NO_ERROR;
    }

    case XA_CMD_TYPE_INIT_DONE_QUERY:
    {
        /* ...check if initialization is done; make sure pointer is sane */
        XF_CHK_ERR(pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

        /* ...put current status */
        *(WORD32 *)pv_value = (d->state & XA_VOC_PROC_FLAG_RUNNING ? 1 : 0);

        return XA_NO_ERROR;
    }

    default:
        /* ...unrecognized command type */
        TRACE(ERROR, _x("Unrecognized command type: %X"), i_idx);
        return XA_API_FATAL_INVALID_CMD_TYPE;
    }
}

static XA_ERRORCODE xa_vp_deinit(XAVoiceProc *d, WORD32 i_idx, pVOID pv_value)
{
	XF_CHK_ERR(d, XA_API_FATAL_INVALID_CMD_TYPE);

	return XA_NO_ERROR;
}

static XA_ERRORCODE xa_voice_init_process(struct XAVoiceProc *d)
{
	UniACodecMemoryOps  memops;
	XA_ERRORCODE ret = XA_NO_ERROR;;

	if (!d->voice_wrap_interface) {
		LOG("base->voice_wrap_interface Pointer is NULL\n");
		return XA_FATAL_ERROR;
	}

	d->voice_wrap_interface(ACODEC_API_CREATE_CODEC, (void **)&d->WrapFun.Create);
	d->voice_wrap_interface(ACODEC_API_DEC_FRAME, (void **)&d->WrapFun.Process);
	d->voice_wrap_interface(ACODEC_API_RESET_CODEC, (void **)&d->WrapFun.Reset);
	d->voice_wrap_interface(ACODEC_API_DELETE_CODEC, (void **)&d->WrapFun.Delete);
	d->voice_wrap_interface(ACODEC_API_GET_PARAMETER, (void **)&d->WrapFun.GetPara);
	d->voice_wrap_interface(ACODEC_API_SET_PARAMETER, (void **)&d->WrapFun.SetPara);
	d->voice_wrap_interface(ACODEC_API_GET_LAST_ERROR, (void **)&d->WrapFun.GetLastError);

	memops.Malloc = (void *)xf_uniacodec_malloc;
	memops.Free = (void *)xf_uniacodec_free;

	if (!d->WrapFun.Create) {
		LOG("WrapFun.Create Pointer is NULL\n");
		return ACODEC_INIT_ERR;
	}

	d->pWrpHdl = d->WrapFun.Create(&memops);
	if (!d->pWrpHdl) {
		LOG("Create codec error in codec wrapper\n");
		return ACODEC_INIT_ERR;
	}

	return ret;
}

static XA_ERRORCODE xa_load_voice_lib(struct XAVoiceProc *d, void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;
        UWORD32 core = 0;
	struct icm_xtlib_pil_info  *cmd;
	struct dpu_lib_stat_t *lib_stat;
	void *lib_interface;
	UniACodecParameter parameter;
	int lib_type;

	cmd = (struct icm_xtlib_pil_info *)xf_ipc_a2b(core, *(UWORD32 *)pv_value);

	cmd->pil_info.dst_addr      = xf_ipc_a2b(core, (UWORD32)cmd->pil_info.dst_addr);
	cmd->pil_info.dst_data_addr = xf_ipc_a2b(core, (UWORD32)cmd->pil_info.dst_data_addr);
	cmd->pil_info.start_sym     = xf_ipc_a2b(core, (UWORD32)cmd->pil_info.start_sym);
	cmd->pil_info.text_addr     = xf_ipc_a2b(core, (UWORD32)cmd->pil_info.text_addr);
	cmd->pil_info.init          = xf_ipc_a2b(core, (UWORD32)cmd->pil_info.init);
	cmd->pil_info.fini          = xf_ipc_a2b(core, (UWORD32)cmd->pil_info.fini);
	cmd->pil_info.rel           = xf_ipc_a2b(core, (UWORD32)cmd->pil_info.rel);
	cmd->pil_info.hash          = xf_ipc_a2b(core, (UWORD32)cmd->pil_info.hash);
	cmd->pil_info.symtab        = xf_ipc_a2b(core, (UWORD32)cmd->pil_info.symtab);
	cmd->pil_info.strtab        = xf_ipc_a2b(core, (UWORD32)cmd->pil_info.strtab);

	lib_type = GET_LIB_TYPE(cmd->lib_type);

	lib_stat = &d->lib_voice_wrap_stat;
	d->voice_wrap_interface = (tUniACodecQueryInterface)dpu_process_init_pi_lib(&cmd->pil_info, lib_stat, 0);
	if (!d->voice_wrap_interface) {
		LOG("load codec wrap lib failed\n");
		return XA_API_FATAL_INVALID_CMD;
	}
	LOG1("load voice wrap lib: %x\n", d->voice_wrap_interface);

	ret = xa_voice_init_process(d);
	if (ret) {
		LOG("codec init failed\n");
		return XA_API_FATAL_INVALID_CMD;
	}

#ifdef DEBUG
	parameter.Printf = (void *)__dsp_printf;
	d->WrapFun.SetPara(d->pWrpHdl, UNIA_FUNC_PRINT, &parameter);
#endif

	return XA_NO_ERROR;
}

static XA_ERRORCODE xa_unload_voice_lib(struct XAVoiceProc *d, void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;
	struct icm_xtlib_pil_info  *cmd;
	struct dpu_lib_stat_t *lib_stat;
	int do_cleanup = 0;
	int lib_type;

	cmd = (struct icm_xtlib_pil_info *)xf_ipc_a2b(0, *(UWORD32 *)pv_value);
	lib_type = GET_LIB_TYPE(cmd->lib_type);
	lib_stat = &d->lib_voice_wrap_stat;

	if (lib_stat && lib_stat->stat == lib_loaded) {
		ret = d->WrapFun.Delete(d->pWrpHdl);
		dpu_process_unload_pi_lib(lib_stat);
	}

	return XA_NO_ERROR;
}

/* ...set vp configuration parameter */
static XA_ERRORCODE xa_vp_set_config_param(XAVoiceProc *d, WORD32 i_idx, pVOID pv_value)
{
    UWORD32     i_value;

    /* ...sanity check - vp pointer must be sane */
    XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...pre-initialization must be completed, vp must be idle */
    XF_CHK_ERR(d->state & XA_VOC_PROC_FLAG_PREINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...get parameter value  */
    i_value = (UWORD32) *(WORD32 *)pv_value;

    /* ...process individual configuration parameter */
    switch (i_idx)
    {
    case XA_MIMO_PROC_CONFIG_PARAM_SAMPLE_RATE:
    case UNIA_SAMPLERATE:
         {
            /* ...set vp sample rate */
            switch((UWORD32)i_value)
            {
                case 4000:
                case 8000:
                case 11025:
                case 12000:
                case 16000:
                case 22050:
                case 24000:
                case 32000:
                case 44100:
                case 48000:
                case 64000:
                case 88200:
                case 96000:
                case 128000:
                case 176400:
                case 192000:
                    d->sample_rate = (UWORD32)i_value;
                    break;
                default:
                    XF_CHK_ERR(0, XA_VOICE_PROC_CONFIG_FATAL_RANGE);
            }
	break;
        }

    case XA_MIMO_PROC_CONFIG_PARAM_PCM_WIDTH:
    case UNIA_DEPTH:
        d->port_pcm_width[0] = (UWORD32)(i_value & 0xFF);
        d->port_pcm_width[1] = (UWORD32)((i_value & 0xFF00) >> 8);
        d->port_pcm_width[2] = (UWORD32)((i_value & 0xFF0000) >> 16);
        d->port_pcm_width[3] = (UWORD32)((i_value & 0xFF000000) >> 24);
        d->pcm_width = d->port_pcm_width[0];
	break;

    case XA_MIMO_PROC_CONFIG_PARAM_CHANNELS:
    case UNIA_CHANNEL:
        /* ...allow stereo only */
        XF_CHK_ERR((i_value <= 2) && (i_value > 0), XA_VOICE_PROC_CONFIG_FATAL_RANGE);
        d->channels = (UWORD32)i_value;
	break;

    case XA_MIMO_PROC_CONFIG_PARAM_PORT_PAUSE:
        {
          XF_CHK_ERR((i_value < (d->num_in_ports + d->num_out_ports)), XA_VOICE_PROC_CONFIG_FATAL_RANGE);
          d->port_state[i_value] |= XA_VOC_PROC_FLAG_PORT_PAUSED;
          TRACE(PROCESS, _b("Pause on port:%d"), i_value);
        }
	break;

    case XA_MIMO_PROC_CONFIG_PARAM_PORT_RESUME:
        {
          XF_CHK_ERR((i_value < (d->num_in_ports + d->num_out_ports)), XA_VOICE_PROC_CONFIG_FATAL_RANGE);
          d->port_state[i_value] &= ~XA_VOC_PROC_FLAG_PORT_PAUSED;
          TRACE(PROCESS, _b("Resume on port:%d"), i_value);
        }
	break;

    case XA_MIMO_PROC_CONFIG_PARAM_PORT_CONNECT:
        {
          XF_CHK_ERR((i_value < (d->num_in_ports + d->num_out_ports)), XA_VOICE_PROC_CONFIG_FATAL_RANGE);
          d->port_state[i_value] |= XA_VOC_PROC_FLAG_PORT_CONNECTED;
          TRACE(PROCESS, _b("Connect on port:%d"), i_value);
        }
	break;

    case XA_MIMO_PROC_CONFIG_PARAM_PORT_DISCONNECT:
        {
          XF_CHK_ERR((i_value < (d->num_in_ports + d->num_out_ports)), XA_VOICE_PROC_CONFIG_FATAL_RANGE);
          d->port_state[i_value] &= ~XA_VOC_PROC_FLAG_PORT_CONNECTED;
          TRACE(PROCESS, _b("Disconnect on port:%d"), i_value);
        }
	break;

    case XA_MIMO_PROC_CONFIG_PARAM_SUSPEND:
	break;

    case XA_MIMO_PROC_CONFIG_PARAM_SUSPEND_RESUME:
	break;

    case UNIA_LOAD_LIB:
	return xa_load_voice_lib(d, pv_value);

    case UNIA_UNLOAD_LIB:
	return xa_unload_voice_lib(d, pv_value);

    default:
        TRACE(ERROR, _x("Invalid parameter: %X"), i_idx);
        return XA_API_FATAL_INVALID_CMD_TYPE;
    }

    return XA_NO_ERROR;
}

/* ...retrieve configuration parameter */
static XA_ERRORCODE xa_vp_get_config_param(XAVoiceProc *d, WORD32 i_idx, pVOID pv_value)
{
    /* ...sanity check - vp must be initialized */
    XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...make sure pre-initialization is completed */
    XF_CHK_ERR(d->state & XA_VOC_PROC_FLAG_PREINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...process individual configuration parameter */
    switch (i_idx)
    {
    case XA_MIMO_PROC_CONFIG_PARAM_SAMPLE_RATE:
    case UNIA_SAMPLERATE:
        /* ...return vp sample rate */
        *(WORD32 *)pv_value = d->sample_rate;
        return XA_NO_ERROR;

    case XA_MIMO_PROC_CONFIG_PARAM_PCM_WIDTH:
    case UNIA_DEPTH:
        /* ...return current PCM width */
        *(WORD32 *)pv_value = d->pcm_width;
        return XA_NO_ERROR;

    case XA_MIMO_PROC_CONFIG_PARAM_CHANNELS:
    case UNIA_CHANNEL:
        /* ...return current channel number */
        *(WORD32 *)pv_value = d->channels;
        return XA_NO_ERROR;

    default:
        TRACE(ERROR, _x("Invalid parameter: %X"), i_idx);
        return XA_API_FATAL_INVALID_CMD_TYPE;
    }
}

/* ...execution command */
static XA_ERRORCODE xa_vp_execute(XAVoiceProc *d, WORD32 i_idx, pVOID pv_value)
{
    XA_ERRORCODE ret;
#ifdef XAF_PROFILE
    clk_t voice_proc_start, voice_proc_stop;
#endif

    /* ...sanity check - vp must be valid */
    XF_CHK_ERR(d, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...vp must be in running state */
    XF_CHK_ERR(d->state & XA_VOC_PROC_FLAG_RUNNING, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...process individual command type */
    switch (i_idx)
    {
    case XA_CMD_TYPE_DO_EXECUTE:
        /* ...perform pcm-gain of the channels */
#ifdef XAF_PROFILE
        voice_proc_start = clk_read_start(CLK_SELN_THREAD);
#endif
        ret = xa_voice_proc_do_execute(d);
#ifdef XAF_PROFILE
        voice_proc_stop = clk_read_stop(CLK_SELN_THREAD);
        voice_proc_cycles += clk_diff(voice_proc_stop, voice_proc_start);
#endif
        return ret;

    case XA_CMD_TYPE_DONE_QUERY:
        /* ...check if processing is complete */
        XF_CHK_ERR(pv_value, XA_API_FATAL_INVALID_CMD_TYPE);
        *(WORD32 *)pv_value = (d->state & XA_VOC_PROC_FLAG_EXEC_DONE ? 1 : 0);
        return XA_NO_ERROR;

    case XA_CMD_TYPE_DO_RUNTIME_INIT:
        /* ...reset vp operation */
        return xa_voice_proc_do_runtime_init(d);

    default:
        /* ...unrecognized command */
        TRACE(ERROR, _x("Invalid index: %X"), i_idx);
        return XA_API_FATAL_INVALID_CMD_TYPE;
    }
}

/* ...set number of input bytes */
static XA_ERRORCODE xa_vp_set_input_bytes(XAVoiceProc *d, WORD32 i_idx, pVOID pv_value)
{
    UWORD32     size;

    /* ...sanity check - check parameters */
    XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...track index must be valid */
    XF_CHK_ERR(i_idx >= 0 && i_idx < d->num_in_ports, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...vp must be initialized */
    XF_CHK_ERR(d->state & XA_VOC_PROC_FLAG_POSTINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...input buffer must exist */
    XF_CHK_ERR(d->input[i_idx], XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...input frame length should not be zero (in bytes) */
    XF_CHK_ERR((size = (UWORD32)*(WORD32 *)pv_value) >= 0, XA_VOICE_PROC_CONFIG_FATAL_RANGE);

    /* ...all is correct; set input buffer length in bytes */
    d->input_length[i_idx] = size;

    return XA_NO_ERROR;
}

/* ...get number of output bytes */
static XA_ERRORCODE xa_vp_get_output_bytes(XAVoiceProc *d, WORD32 i_idx, pVOID pv_value)
{
    int p_idx;
    /* ...sanity check - check parameters */
    XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    p_idx = i_idx-d->num_in_ports;
    /* ...track index must be zero */
    XF_CHK_ERR((p_idx >= 0) && (p_idx < d->num_out_ports), XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...vp must be running */
    XF_CHK_ERR(d->state & XA_VOC_PROC_FLAG_RUNNING, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...output buffer must exist */
    //XF_CHK_ERR(d->output, XA_voice_proc_EXEC_FATAL_OUTPUT);
    XF_CHK_ERR(d->output, XA_VOICE_PROC_EXEC_FATAL_STATE);

    /* ...return number of produced bytes */
    *(WORD32 *)pv_value = (d->state & XA_VOC_PROC_FLAG_OUTPUT ? d->produced[p_idx] : 0);

    return XA_NO_ERROR;
}

/* ...get number of consumed bytes */
static XA_ERRORCODE xa_vp_get_curidx_input_buf(XAVoiceProc *d, WORD32 i_idx, pVOID pv_value)
{
    /* ...sanity check - check parameters */
    XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...track index must be valid */
    XF_CHK_ERR(i_idx >= 0 && i_idx < XA_MIMO_IN_PORTS, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...vp must be running */
    XF_CHK_ERR(d->state & XA_VOC_PROC_FLAG_RUNNING, XA_VOICE_PROC_EXEC_FATAL_STATE);

    /* ...input buffer must exist */
    //XF_CHK_ERR(d->input[i_idx], XA_voice_proc_EXEC_FATAL_INPUT);
    XF_CHK_ERR(d->input[i_idx], XA_VOICE_PROC_EXEC_FATAL_STATE);

    /* ...return number of bytes consumed (always consume fixed-length chunk) */
    *(WORD32 *)pv_value = d->consumed[i_idx];

    return XA_NO_ERROR;
}

/* ...end-of-stream processing */
static XA_ERRORCODE xa_vp_input_over(XAVoiceProc *d, WORD32 i_idx, pVOID pv_value)
{
    /* ...sanity check */
    int i, input_over_count = 0;
    XF_CHK_ERR(d, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ... port specific end-of-stream flag */
    d->port_state[i_idx] |= XA_VOC_PROC_FLAG_COMPLETE;
    for(i=0; i<d->num_in_ports; i++)
    {
    	input_over_count += (XA_VOC_PROC_FLAG_COMPLETE & d->port_state[i])?1:0;
    }

    /* ...put overall end-of-stream flag */
    if(input_over_count == d->num_in_ports)
	d->state |= XA_VOC_PROC_FLAG_COMPLETE;

    TRACE(PROCESS, _b("Input-over-condition signalled for port %d"), i_idx);

    return XA_NO_ERROR;
}

/*******************************************************************************
 * Memory information API
 ******************************************************************************/

/* ..get total amount of data for memory tables */
static XA_ERRORCODE xa_vp_get_memtabs_size(XAVoiceProc *d, WORD32 i_idx, pVOID pv_value)
{
    /* ...basic sanity checks */
    XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...check vp is pre-initialized */
    XF_CHK_ERR(d->state & XA_VOC_PROC_FLAG_PREINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...we have all our tables inside API structure - good? tbd */
    *(WORD32 *)pv_value = 0;

    return XA_NO_ERROR;
}

/* ..set memory tables pointer */
static XA_ERRORCODE xa_vp_set_memtabs_ptr(XAVoiceProc *d, WORD32 i_idx, pVOID pv_value)
{
    /* ...basic sanity checks */
    XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...check vp is pre-initialized */
    XF_CHK_ERR(d->state & XA_VOC_PROC_FLAG_PREINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...do not do anything; just return success - tbd */
    return XA_NO_ERROR;
}

/* ...return total amount of memory buffers */
static XA_ERRORCODE xa_vp_get_n_memtabs(XAVoiceProc *d, WORD32 i_idx, pVOID pv_value)
{
    /* ...basic sanity checks */
    XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...we have N input ports, M output ports and 1 persistent and 1 scratch buffer */
    *(WORD32 *)pv_value = d->num_in_ports + d->num_out_ports + 1 + 1;

    return XA_NO_ERROR;
}

/* ...return memory buffer data */
static XA_ERRORCODE xa_vp_get_mem_info_size(XAVoiceProc *d, WORD32 i_idx, pVOID pv_value)
{
    /* ...basic sanity check */
    XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...return frame buffer minimal size only after post-initialization is done */
    XF_CHK_ERR(d->state & XA_VOC_PROC_FLAG_POSTINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...all buffers are of the same length */
    WORD32 n_mems = (d->num_in_ports + d->num_out_ports + 1 + 1);
    if(i_idx < d->num_in_ports)
    {
        /* ...input buffers */
        if (i_idx == 0)
            *(WORD32 *)pv_value = d->frame_size * d->channels * d->port_pcm_width[0]/8;
        else
            *(WORD32 *)pv_value = d->frame_size * d->channels * d->port_pcm_width[1]/8;
    }
    else
    if(i_idx < (d->num_in_ports + d->num_out_ports))
    {
        /* ...output buffer */
        if (i_idx == d->num_in_ports)
            *(WORD32 *)pv_value = d->frame_size * d->channels * d->port_pcm_width[2]/8;
        else
            *(WORD32 *)pv_value = d->frame_size * d->channels * d->port_pcm_width[3]/8;
    }
    else
    if(i_idx < (n_mems-1))
    {
        /* ...persist buffer */
        *(WORD32 *)pv_value = d->persist_size;
    }
    else
    if(i_idx < (n_mems))
    {
        /* ...scratch buffer */
        *(WORD32 *)pv_value = d->scratch_size;
    }
    else{
	return XA_API_FATAL_INVALID_CMD_TYPE;
    }

    return XA_NO_ERROR;
}

/* ...return memory alignment data */
static XA_ERRORCODE xa_vp_get_mem_info_alignment(XAVoiceProc *d, WORD32 i_idx, pVOID pv_value)
{
    /* ...basic sanity check */
    XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...return frame buffer minimal size only after post-initialization is done */
    XF_CHK_ERR(d->state & XA_VOC_PROC_FLAG_POSTINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...all buffers are 4-bytes aligned */
    *(WORD32 *)pv_value = 4;

    return XA_NO_ERROR;
}

/* ...return memory type data */
static XA_ERRORCODE xa_vp_get_mem_info_type(XAVoiceProc *d, WORD32 i_idx, pVOID pv_value)
{
    /* ...basic sanity check */
    XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...return frame buffer minimal size only after post-initialization is done */

    XF_CHK_ERR(d->state & XA_VOC_PROC_FLAG_POSTINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

    WORD32 n_mems = (d->num_in_ports + d->num_out_ports + 1 + 1);
    if(i_idx < d->num_in_ports)
    {
        /* ...input buffers */
        *(WORD32 *)pv_value = XA_MEMTYPE_INPUT;
    }
    else
    if(i_idx < (d->num_in_ports + d->num_out_ports))
    {
        /* ...output buffer */
        *(WORD32 *)pv_value = XA_MEMTYPE_OUTPUT;
    }
    else
    if(i_idx < (n_mems-1))
    {
        /* ...scratch buffer */
        *(WORD32 *)pv_value = XA_MEMTYPE_PERSIST;
    }
    else
    if(i_idx < (n_mems))
    {
        /* ...scratch buffer */
        *(WORD32 *)pv_value = XA_MEMTYPE_SCRATCH;
    }
    else{
	return XA_API_FATAL_INVALID_CMD_TYPE;
    }
    return XA_NO_ERROR;
}

/* ...set memory pointer */
static XA_ERRORCODE xa_vp_set_mem_ptr(XAVoiceProc *d, WORD32 i_idx, pVOID pv_value)
{
    /* ...basic sanity check */
    XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

    /* ...codec must be initialized */
    XF_CHK_ERR(d->state & XA_VOC_PROC_FLAG_POSTINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

    WORD32 n_mems = (d->num_in_ports + d->num_out_ports + 1 + 1);
    if(i_idx < d->num_in_ports)
    {
        /* ...input buffers */
        d->input[i_idx] = pv_value;
    }
    else
    if(i_idx < (d->num_in_ports + d->num_out_ports))
    {
        /* ...output buffer */
        d->output[i_idx-d->num_in_ports] = pv_value;
    }
    else
    if(i_idx < (n_mems-1))
    {
        /* ...persistent buffer */
        d->persist = pv_value;
    }
    else
    if(i_idx < (n_mems))
    {
        /* ...scratch buffer */
        d->scratch = pv_value;
    }
    else{
	return XA_API_FATAL_INVALID_CMD_TYPE;
    }
    return XA_NO_ERROR;
}

/*******************************************************************************
 * API command hooks
 ******************************************************************************/

static XA_ERRORCODE (* const xa_vp_api[])(XAVoiceProc *, WORD32, pVOID) = 
{
    [XA_API_CMD_GET_API_SIZE]           = xa_vp_get_api_size,

    [XA_API_CMD_INIT]                   = xa_vp_init, 
    [XA_API_CMD_DEINIT]                 = xa_vp_deinit,
    [XA_API_CMD_SET_CONFIG_PARAM]       = xa_vp_set_config_param,
    [XA_API_CMD_GET_CONFIG_PARAM]       = xa_vp_get_config_param,

    [XA_API_CMD_EXECUTE]                = xa_vp_execute,
    [XA_API_CMD_SET_INPUT_BYTES]        = xa_vp_set_input_bytes,
    [XA_API_CMD_GET_OUTPUT_BYTES]       = xa_vp_get_output_bytes,
    [XA_API_CMD_GET_CURIDX_INPUT_BUF]   = xa_vp_get_curidx_input_buf,
    [XA_API_CMD_INPUT_OVER]             = xa_vp_input_over,

    [XA_API_CMD_GET_MEMTABS_SIZE]       = xa_vp_get_memtabs_size,
    [XA_API_CMD_SET_MEMTABS_PTR]        = xa_vp_set_memtabs_ptr,
    [XA_API_CMD_GET_N_MEMTABS]          = xa_vp_get_n_memtabs,
    [XA_API_CMD_GET_MEM_INFO_SIZE]      = xa_vp_get_mem_info_size,
    [XA_API_CMD_GET_MEM_INFO_ALIGNMENT] = xa_vp_get_mem_info_alignment,
    [XA_API_CMD_GET_MEM_INFO_TYPE]      = xa_vp_get_mem_info_type,
    [XA_API_CMD_SET_MEM_PTR]            = xa_vp_set_mem_ptr,
};

/* ...total number of commands supported */
#define XA_VP_API_COMMANDS_NUM   (sizeof(xa_vp_api) / sizeof(xa_vp_api[0]))

/*******************************************************************************
 * API entry point
 ******************************************************************************/

XA_ERRORCODE xa_voiceprocess(xa_codec_handle_t p_xa_module_obj, WORD32 i_cmd, WORD32 i_idx, pVOID pv_value)
{
    XAVoiceProc    *d = (XAVoiceProc *) p_xa_module_obj;

    /* ...check if command index is sane */
    XF_CHK_ERR(i_cmd < XA_VP_API_COMMANDS_NUM, XA_API_FATAL_INVALID_CMD);

    /* ...see if command is defined */
    XF_CHK_ERR(xa_vp_api[i_cmd], XA_API_FATAL_INVALID_CMD);

    /* ...execute requested command */
    return (xa_vp_api[i_cmd](d, i_idx, pv_value));
}

