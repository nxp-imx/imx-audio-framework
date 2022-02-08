/*
* Copyright (c) 2015-2021 Cadence Design Systems Inc.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
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
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#include "audio/xa-pcm-split-api.h"
#include "audio/xa-pcm-gain-api.h"
#include "audio/xa_mp3_dec_api.h"
#include "audio/xa_aac_dec_api.h"
#include "audio/xa-mixer-api.h"
#include "audio/xa_src_pp_api.h"
#include "xaf-utils-test.h"
#include "xaf-fio-test.h"

#define RUNTIME_ACTIONS

#define AUDIO_FRMWK_BUF_SIZE   (256 << 13)
#define AUDIO_COMP_BUF_SIZE    (1024 << 10)

enum {
    XA_COMP = -1,
    XA_MP3_DEC0,
    XA_MP3_DEC1,
    XA_AAC_DEC0,
    XA_AAC_DEC1,
    XA_MIXER0,
    XA_MIMO12_0,
    XA_GAIN0,
    XA_SRC_PP0,
    NUM_COMP_IN_GRAPH
};

char comp_string[NUM_COMP_IN_GRAPH * 15] = { "MP3DEC0:0, MP3DEC1:1, AACDEC0:2, AACDEC1:3, MIXER:4, PCM-SPLIT:5, PCMGAIN:6, SRC:7" };
#define COMPONENT_STRING FIO_PRINTF(stdout, "Component_ID : %s\n\n", comp_string);

#define NUM_IN_THREADS 3
#define NUM_EXTRA_PROBE_THREADS 2
#define NUM_OUT_THREADS 2

#define NUM_IO_THREADS (NUM_IN_THREADS + NUM_OUT_THREADS)

#define TESTBENCH_USAGE FIO_PRINTF(stdout, "\nUsage: %s -infile:<>.mp3 -infile:<>.mp3 -infile:<>.adts -infile:<>.adts -outfile:<>.pcm -outfile:<>.pcm <Optional Parameters> \nOnly mono, 16 bits, 44.1 kHz mp3 or adts input files supported, Fourth input file is optional which can be given for runtime connect/disconnect test. \n\n", argv[0]);

#ifdef RUNTIME_ACTIONS
#define PRINT_USAGE \
    TESTBENCH_USAGE;\
    COMPONENT_STRING;\
    runtime_param_usage(); \
    runtime_param_reconnect_usage(); \
    runtime_param_footnote();
#else
#define PRINT_USAGE \
    TESTBENCH_USAGE; \
    COMPONENT_STRING;
#endif

//component parameters

unsigned int num_bytes_read, num_bytes_write;
extern int audio_frmwk_buf_size;
extern int audio_comp_buf_size;
double strm_duration;

#ifdef XAF_PROFILE
extern long long tot_cycles, frmwk_cycles, fread_cycles, fwrite_cycles;
extern long long dsp_comps_cycles, aac_dec_cycles, dec_cycles, mix_cycles, pcm_split_cycles, pcm_gain_cycles, src_cycles;
extern double dsp_mcps;
#endif

/* Global vaiables */
void *p_adev = NULL;
xf_thread_t comp_thread[NUM_COMP_IN_GRAPH];
xf_id_t g_comp_id[NUM_COMP_IN_GRAPH];
int comp_ninbuf[NUM_COMP_IN_GRAPH];
int comp_noutbuf[NUM_COMP_IN_GRAPH];
xaf_comp_type comp_type[NUM_COMP_IN_GRAPH];
unsigned char comp_stack[NUM_COMP_IN_GRAPH][STACK_SIZE];
void *comp_thread_args[NUM_COMP_IN_GRAPH][NUM_THREAD_ARGS];
int(*comp_setup[NUM_COMP_IN_GRAPH])(void * p_comp, xaf_format_t *, int, ...);
xaf_format_t comp_format[NUM_COMP_IN_GRAPH];
int comp_probe[NUM_COMP_IN_GRAPH];
int comp_probe_mask[NUM_COMP_IN_GRAPH];
void *p_input[NUM_COMP_IN_GRAPH], *p_output[NUM_COMP_IN_GRAPH];
void *p_comp_inbuf[NUM_COMP_IN_GRAPH][2];
void *p_comp[NUM_COMP_IN_GRAPH];
extern FILE *mcps_p_output;
COMP_STATE comp_state[NUM_COMP_IN_GRAPH];
int comp_create_order[] = { XA_MP3_DEC0, XA_MP3_DEC1, XA_AAC_DEC0,XA_MIXER0, XA_MIMO12_0, XA_GAIN0, XA_SRC_PP0,XA_AAC_DEC1 };
int inp_thread_create_order[] = { XA_MP3_DEC0, XA_MP3_DEC1, XA_AAC_DEC0, XA_AAC_DEC1 };


/* Dummy unused functions */
//XA_ERRORCODE xa_mp3_decoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
//XA_ERRORCODE xa_aac_decoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
//XA_ERRORCODE xa_mixer(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_mp3_encoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4) { return 0; }
//XA_ERRORCODE xa_src_pp_fx(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_renderer(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4) { return 0; }
XA_ERRORCODE xa_capturer(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4) { return 0; }
XA_ERRORCODE xa_amr_wb_decoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4) { return 0; }
XA_ERRORCODE xa_hotword_decoder(xa_codec_handle_t p_xa_module_obj, WORD32 i_cmd, WORD32 i_idx, pVOID pv_value) { return 0; }
XA_ERRORCODE xa_vorbis_decoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4) { return 0; }
//XA_ERRORCODE xa_pcm_gain(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_dummy_aec22(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4) { return 0; }
XA_ERRORCODE xa_dummy_aec23(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4) { return 0; }
//XA_ERRORCODE xa_pcm_split(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_mimo_mix(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4) { return 0; }
XA_ERRORCODE xa_dummy_wwd(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4) { return 0; }
XA_ERRORCODE xa_dummy_hbuf(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4) { return 0; }
XA_ERRORCODE xa_opus_encoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4) { return 0; }
XA_ERRORCODE xa_dummy_wwd_msg(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4) { return 0; }
XA_ERRORCODE xa_dummy_hbuf_msg(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4) { return 0; }


#define MAX_INP_STRMS           4
#define MAX_OUT_STRMS           2
#define MIN_INP_STRMS           3

#define PCM_SPLIT_SAMPLE_RATE         44100
#define PCM_SPLIT_IN_NUM_CH           1
#define PCM_SPLIT_PCM_WIDTH           16

static int mimo12_setup(void *p_comp, xaf_format_t *p_format, int nvar_args, ...)
{
#define PCM_SPLIT_NUM_SET_PARAMS    3
    int param[(PCM_SPLIT_NUM_SET_PARAMS + 1) * 2];
    int probe_enabled;
    va_list varg_list;

    va_start(varg_list, nvar_args);

    param[0 * 2 + 0] = XA_PCM_SPLIT_CONFIG_PARAM_CHANNELS;
    param[0 * 2 + 1] = p_format->channels;
    param[1 * 2 + 0] = XA_PCM_SPLIT_CONFIG_PARAM_SAMPLE_RATE;
    param[1 * 2 + 1] = p_format->sample_rate;
    param[2 * 2 + 0] = XA_PCM_SPLIT_CONFIG_PARAM_PCM_WIDTH;
    param[2 * 2 + 1] = p_format->pcm_width;

    probe_enabled = va_arg(varg_list, int);

    if (probe_enabled)
    {
        param[3 * 2 + 0] = XAF_COMP_CONFIG_PARAM_PROBE_ENABLE;
        param[3 * 2 + 1] = va_arg(varg_list, int);

        fprintf(stderr, "PCM_SPLIT_SETUP: PROBE_ENABLED\n");
    }
    va_end(varg_list);

    return(xaf_comp_set_config(p_comp, PCM_SPLIT_NUM_SET_PARAMS + probe_enabled, &param[0]));
}

#define AAC_DEC_PCM_WIDTH       16
static int aac_setup(void *p_comp, xaf_format_t *p_format, int nvar_args, ...)
{
#define AAC_DEC_NUM_SET_PARAMS 1
    int param[(AAC_DEC_NUM_SET_PARAMS + 1) * 2];
    int probe_enabled;
    va_list varg_list;

    va_start(varg_list, nvar_args);

    param[0 * 2 + 0] = XA_AACDEC_CONFIG_PARAM_PCM_WDSZ;
    param[0 * 2 + 1] = p_format->pcm_width;

    probe_enabled = va_arg(varg_list, int);
    if (probe_enabled)
    {
        param[1 * 2 + 0] = XAF_COMP_CONFIG_PARAM_PROBE_ENABLE;
        param[1 * 2 + 1] = va_arg(varg_list, int);

        fprintf(stderr, "AACDEC SETUP: PROBE_ENABLED\n");
    }
    va_end(varg_list);

    return(xaf_comp_set_config(p_comp, AAC_DEC_NUM_SET_PARAMS + probe_enabled, &param[0]));
}

#define MP3_DEC_PCM_WIDTH       16
static int mp3_setup(void *p_comp, xaf_format_t *p_format, int nvar_args, ...)
{
#define MP3_DEC_NUM_SET_PARAMS 1
    int param[(MP3_DEC_NUM_SET_PARAMS + 1) * 2];
    int probe_enabled;
    va_list varg_list;

    va_start(varg_list, nvar_args);

    param[0 * 2 + 0] = XA_MP3DEC_CONFIG_PARAM_PCM_WDSZ;
    param[0 * 2 + 1] = p_format->pcm_width;

    probe_enabled = va_arg(varg_list, int);
    if (probe_enabled)
    {
        param[1 * 2 + 0] = XAF_COMP_CONFIG_PARAM_PROBE_ENABLE;
        param[1 * 2 + 1] = va_arg(varg_list, int);

        fprintf(stderr, "MP3DEC SETUP: PROBE ENABLED\n");
    }
    va_end(varg_list);

    return(xaf_comp_set_config(p_comp, MP3_DEC_NUM_SET_PARAMS + probe_enabled, &param[0]));
}

#define PCM_GAIN_SAMPLE_WIDTH   16
#define PCM_GAIN_NUM_CH         1
#define PCM_GAIN_SAMPLE_RATE    44100
#define PCM_GAIN_IDX_FOR_GAIN   1

static int pcm_gain_setup(void *p_comp, xaf_format_t *p_format, int nvar_args, ...)
{
#define PCM_GAIN_NUM_SET_PARAMS 5
    int param[(PCM_GAIN_NUM_SET_PARAMS + 1) * 2];
    int probe_enabled;
    int frame_size = XAF_INBUF_SIZE;

    va_list varg_list;
    va_start(varg_list, nvar_args);

    int gain_idx = PCM_GAIN_IDX_FOR_GAIN;    //gain index range is 0 to 6 -> {0db, -6db, -12db, -18db, 6db, 12db, 18db}
    frame_size = va_arg(varg_list, int);

    param[0 * 2 + 0] = XA_PCM_GAIN_CONFIG_PARAM_CHANNELS;
    param[0 * 2 + 1] = p_format->channels;
    param[1 * 2 + 0] = XA_PCM_GAIN_CONFIG_PARAM_SAMPLE_RATE;
    param[1 * 2 + 1] = p_format->sample_rate;
    param[2 * 2 + 0] = XA_PCM_GAIN_CONFIG_PARAM_PCM_WIDTH;
    param[2 * 2 + 1] = p_format->pcm_width;
    param[3 * 2 + 0] = XA_PCM_GAIN_CONFIG_PARAM_FRAME_SIZE;
    param[3 * 2 + 1] = frame_size;
    param[4 * 2 + 0] = XA_PCM_GAIN_CONFIG_PARAM_GAIN_FACTOR;
    param[4 * 2 + 1] = gain_idx;

    probe_enabled = va_arg(varg_list, int);
    if (probe_enabled)
    {
        param[5 * 2 + 0] = XAF_COMP_CONFIG_PARAM_PROBE_ENABLE;
        param[5 * 2 + 1] = va_arg(varg_list, int);

        fprintf(stderr, "PCMGAIN SETUP: PROBE_ENABLED\n");
    }
    va_end(varg_list);

    fprintf(stderr, "PCMGAIN SETUP: Frame Size=%d\n", frame_size);

    return(xaf_comp_set_config(p_comp, PCM_GAIN_NUM_SET_PARAMS + probe_enabled, &param[0]));
}


#define MIXER_SAMPLE_RATE       44100
#define MIXER_NUM_CH            1
#define MIXER_PCM_WIDTH         16

static int mixer_setup(void *p_comp, xaf_format_t *p_format, int nvar_args, ...)
{
#define MIXER_NUM_SET_PARAMS    3
    int param[(MIXER_NUM_SET_PARAMS + 1) * 2];
    int probe_enabled;
    va_list varg_list;

    va_start(varg_list, nvar_args);

    param[0 * 2 + 0] = XA_MIXER_CONFIG_PARAM_SAMPLE_RATE;
    param[0 * 2 + 1] = p_format->sample_rate;
    param[1 * 2 + 0] = XA_MIXER_CONFIG_PARAM_CHANNELS;
    param[1 * 2 + 1] = p_format->channels;
    param[2 * 2 + 0] = XA_MIXER_CONFIG_PARAM_PCM_WIDTH;
    param[2 * 2 + 1] = p_format->pcm_width;

    probe_enabled = va_arg(varg_list, int);
    if (probe_enabled)
    {
        param[3 * 2 + 0] = XAF_COMP_CONFIG_PARAM_PROBE_ENABLE;
        param[3 * 2 + 1] = va_arg(varg_list, int);

        fprintf(stderr, "MIXER SETUP: PROBE ENABLED\n");
    }
    va_end(varg_list);

    return(xaf_comp_set_config(p_comp, MIXER_NUM_SET_PARAMS + probe_enabled, &param[0]));
}

#define SRC_PP_MAX_SRC_FRAME_ADJUST     2
#define SRC_PP_MAX_INPUT_CHUNK_LEN  512

#define SRC_PP_SAMPLE_WIDTH     16
#define SRC_PP_NUM_CH           1
#define SRC_PP_SAMPLE_RATE_IN       44100
#define SRC_PP_SAMPLE_RATE_OUT      16000

static int src_setup(void *p_comp, xaf_format_t *p_format, int nvar_args, ...)
{
#define SRC_PP_NUM_SET_PARAMS    5
    int param[(SRC_PP_NUM_SET_PARAMS + 1) * 2];
    int in_frame_size, pcm_width_bytes, out_sample_rate;
    int probe_enabled;
    va_list varg_list;

    va_start(varg_list, nvar_args);

    out_sample_rate = va_arg(varg_list, int);
    pcm_width_bytes = p_format->pcm_width / 8;

    in_frame_size = XAF_INBUF_SIZE / (pcm_width_bytes*SRC_PP_NUM_CH*SRC_PP_MAX_SRC_FRAME_ADJUST);
    in_frame_size = (in_frame_size > SRC_PP_MAX_INPUT_CHUNK_LEN) ? SRC_PP_MAX_INPUT_CHUNK_LEN : in_frame_size;

    param[0 * 2 + 0] = XA_SRC_PP_CONFIG_PARAM_INPUT_CHANNELS;
    param[0 * 2 + 1] = p_format->channels;
    param[1 * 2 + 0] = XA_SRC_PP_CONFIG_PARAM_INPUT_SAMPLE_RATE;
    param[1 * 2 + 1] = p_format->sample_rate;
    param[2 * 2 + 0] = XA_SRC_PP_CONFIG_PARAM_OUTPUT_SAMPLE_RATE;
    param[2 * 2 + 1] = out_sample_rate;
    param[3 * 2 + 0] = XA_SRC_PP_CONFIG_PARAM_INPUT_CHUNK_SIZE;
    param[3 * 2 + 1] = in_frame_size;
    param[4 * 2 + 0] = XA_SRC_PP_CONFIG_PARAM_BYTES_PER_SAMPLE;
    param[4 * 2 + 1] = (pcm_width_bytes == 4) ? 3 : 2; //src library only supports 16 or MSB-aligned 24 bit input

    probe_enabled = va_arg(varg_list, int);
    if (probe_enabled)
    {
        param[5 * 2 + 0] = XAF_COMP_CONFIG_PARAM_PROBE_ENABLE;
        param[5 * 2 + 1] = va_arg(varg_list, int);

        fprintf(stderr, "SRC SETUP: PROBE_ENABLED\n");
    }
    va_end(varg_list);

    return(xaf_comp_set_config(p_comp, SRC_PP_NUM_SET_PARAMS + probe_enabled, &param[0]));
}

#if 0 /* unused function */
static int get_comp_config(void *p_comp, xaf_format_t *p_format)
{
    int param[6];
    int ret;

    TST_CHK_PTR(p_comp, "get_comp_config");
    TST_CHK_PTR(p_format, "get_comp_config");

    param[0] = XA_PCM_SPLIT_CONFIG_PARAM_CHANNELS;
    param[2] = XA_PCM_SPLIT_CONFIG_PARAM_PCM_WIDTH;
    param[4] = XA_PCM_SPLIT_CONFIG_PARAM_SAMPLE_RATE;

    ret = xaf_comp_get_config(p_comp, 3, &param[0]);
    if (ret < 0)
        return ret;

    p_format->channels = param[1];
    p_format->pcm_width = param[3];
    p_format->sample_rate = param[5];

    return 0;
}
#endif

void fio_quit()
{
    return;
}

#ifndef XA_DISABLE_EVENT
int playback_event_handler(event_info_t *event)
{
    fprintf(stderr, "Playback Event Handler: Got event, id = %08x\n", event->event_id);

    if (!event->comp_error_flag)
        /* ...only fatal error is handled in playback */
        return 0;

    int i,cid; 
    void *p_error_comp = (void *)event->comp_addr;

    for(i=0; i<NUM_COMP_IN_GRAPH; i++)
    {
        cid = comp_create_order[i];
        if(p_comp[cid] == p_error_comp)
            break;
    }
    int error_code = *(int *)event->event_buf;

    fprintf(stderr, "Playback Event Handler: Error code =%x received for cid =%d \n",error_code, cid);

    if(error_code > 0)           
    {
        fprintf(stderr, "Playback Event Handler: Non Fatal error received, not taking any action\n");
        return 0;
    }

    switch(cid)
    {
        case XA_MP3_DEC0:
        case XA_MP3_DEC1:
        case XA_AAC_DEC0:
        case XA_AAC_DEC1:
            if (__xf_thread_get_state(&comp_thread[cid]) == XF_THREAD_STATE_BLOCKED)
            {
                __xf_thread_cancel(&comp_thread[cid]);
                fprintf(stderr, "Playback Event Handler: Thread cancelled for cid %d\n",cid);
            }

            fprintf(stderr, "Playback Event Handler: Issuing disconnect w/delete command for cid %d\n", cid);
            gpcomp_disconnect(cid, 1, XA_MIXER0, cid, 1); 
            break;

        case XA_MIXER0:
        case XA_MIMO12_0:
        case XA_GAIN0:
        case XA_SRC_PP0:
            fprintf(stderr,"Playback Event Handler: Cannot handle this error, destroying pipeline\n");
            TST_CHK_API(XAF_INVALIDVAL_ERR, "playback_event_handler");
            break;

        default:
            fprintf(stderr,"Playback Event Handler: Invalid component ID \n");
            break;
    }

    return 0;
}
#endif /* XA_DISABLE_EVENT */

int comp_connect(int component_id, int port, int component_dest_id, int port_dest, int comp_create_delete_flag)
{
    g_force_input_over[component_id] = 0;
    
    if ( (component_id >= XA_MIXER0) || ( component_dest_id != XA_MIXER0 )  )
    {
        fprintf(stdout, "Runtime Action: Command failed. Connect is not supported for this component \n");
        return -1;
    }    

    if ( (comp_create_delete_flag) && (comp_state[component_id] != COMP_CREATED) )
    {
        int i;
        int read_length;
        int dec_info[4];
        xaf_comp_status comp_status;


        TST_CHK_API_COMP_CREATE(p_adev, &p_comp[component_id], g_comp_id[component_id], comp_ninbuf[component_id], comp_noutbuf[component_id], &p_comp_inbuf[component_id][0], comp_type[component_id], "xaf_comp_create");
        comp_state[component_id] = COMP_CREATED; /* created info */
        TST_CHK_API(comp_setup[component_id](p_comp[component_id], &comp_format[component_id], 2, comp_probe[component_id], comp_probe_mask[component_id]), "comp_setup");
        TST_CHK_API(xaf_comp_process(p_adev, p_comp[component_id], NULL, 0, XAF_START_FLAG), "xaf_comp_process");
        for (i = 0; i < 2; i++)
        {
            TST_CHK_API(read_input(p_comp_inbuf[component_id][i], XAF_INBUF_SIZE, &read_length, p_input[component_id], comp_type[component_id]), "read_input");

            if (read_length)
                TST_CHK_API(xaf_comp_process(p_adev, p_comp[component_id], p_comp_inbuf[component_id][i], read_length, XAF_INPUT_READY_FLAG), "xaf_comp_process");
            else
            {    
                TST_CHK_API(xaf_comp_process(p_adev, p_comp[component_id], NULL, 0, XAF_INPUT_OVER_FLAG), "xaf_comp_process");
                break;
            }
        }
        TST_CHK_API(xaf_comp_get_status(p_adev, p_comp[component_id], &comp_status, &dec_info[0]), "xaf_comp_get_status");
        if (comp_status != XAF_INIT_DONE)
        {
	    if ( read_length != XAF_INBUF_SIZE )
	    {
		FIO_PRINTF(stderr, "Failed to init. No sufficient input for initialization.\n");
		return -1;

	    }
            FIO_PRINTF(stderr, "Failed to init\n");
            exit(-1);
        }

        TST_CHK_API(xaf_connect(p_comp[component_id], port, p_comp[component_dest_id], port_dest, 4), "xaf_connect");
        fprintf(stdout, "Runtime Action: Component %d : CONNECT command issued\n", component_id);

        comp_thread_args[component_id][1] = (void *)p_comp[component_id];
        __xf_thread_create(&comp_thread[component_id], comp_process_entry, comp_thread_args[component_id], "Component Thread", comp_stack[component_id], STACK_SIZE, XAF_APP_THREADS_PRIORITY);

    }
    else if ( (comp_create_delete_flag == 0) && (comp_state[component_id] == COMP_CREATED) )
    {
        TST_CHK_API(xaf_connect(p_comp[component_id], port, p_comp[component_dest_id], port_dest, 4), "xaf_connect");
        fprintf(stdout, "Runtime Action: Component %d : CONNECT command issued\n", component_id);
    }
    else
    {        
        return -1;
    }

    return 0;
}

int comp_disconnect(int component_id, int port, int component_dest_id, int port_dest, int comp_create_delete_flag)
{
    if ( (component_id >= XA_MIXER0) || ( component_dest_id != XA_MIXER0 )  )
    {
        fprintf(stdout, "Runtime Action: Command failed. Disconnect is not supported for this component \n");
        return -1;
    }
        
    if ( comp_state[component_id] != COMP_CREATED )
    {       
        return -1;
    }

    if (comp_create_delete_flag)
    {
        if (g_active_disconnect_comp[component_id])
        {
            fprintf(stdout, "Disconnect already active for component: %d\n", component_id);
            return 0;
        }
    
        g_active_disconnect_comp[component_id] = 1;

        int err_ret;
        g_force_input_over[component_id] = 1;
        err_ret = __xf_thread_join(&comp_thread[component_id], NULL);
        err_ret = __xf_thread_destroy(&comp_thread[component_id]);
        TST_CHK_API(xaf_disconnect(p_comp[component_id], port, p_comp[component_dest_id], port_dest), "xaf_disconnect");
        fprintf(stdout, "Runtime Action: Component %d : DISCONNECT command issued\n", component_id);
        TST_CHK_API(xaf_comp_delete(p_comp[component_id]), "xaf_comp_delete");
        comp_state[component_id] = COMP_DELETED; /* delete info */
        p_comp[component_id] = NULL;

        g_active_disconnect_comp[component_id] = 0;
    }
    else
    {
        TST_CHK_API(xaf_disconnect(p_comp[component_id], port, p_comp[component_dest_id], port_dest), "xaf_disconnect");
        fprintf(stdout, "Runtime Action: Component %d : DISCONNECT command issued\n", component_id);
    }


    return 0;
}

int main_task(int argc, char **argv)
{
    xaf_comp_status comp_status;
    int dec_info[4];
    FILE *fp, *ofp;
    int buf_length = XAF_INBUF_SIZE;
    int read_length;
    int i, j, k, cid;
    const char *ext;
    pUWORD8 ver_info[3] = { 0,0,0 };    //{ver,lib_rev,api_rev}
    unsigned short board_id = 0;
    mem_obj_t* mem_handle;
    int num_in_strms = 0;
    int num_out_strms = 0;
    int comp_framesize[NUM_COMP_IN_GRAPH];
    int out_sample_rate;
    int frame_size_cmdl[MAX_INP_STRMS];
    int nframe_size_cmdl = 0;

#if 1 // Used in utils to abort blocked threads
    extern int g_num_comps_in_graph;
    extern xf_thread_t *g_comp_thread;
    g_num_comps_in_graph = NUM_COMP_IN_GRAPH;
    g_comp_thread = comp_thread;
#endif

#ifdef RUNTIME_ACTIONS
    void *runtime_params;
#endif

#ifndef XA_DISABLE_EVENT
    xa_app_initialize_event_list(MAX_EVENTS); 
    g_app_handler_fn = playback_event_handler;

    extern UWORD32 g_enable_error_channel_flag;
    g_enable_error_channel_flag = XAF_ERR_CHANNEL_ALL;
#endif

#ifdef XAF_PROFILE
    frmwk_cycles = 0;
    fread_cycles = 0;
    fwrite_cycles = 0;
    dsp_comps_cycles = 0;
    tot_cycles = 0;
    num_bytes_read = 0;
    num_bytes_write = 0;
    aac_dec_cycles = 0; dec_cycles = 0; mix_cycles = 0; pcm_split_cycles = 0; src_cycles = 0; pcm_gain_cycles = 0;
#endif

    audio_frmwk_buf_size = AUDIO_FRMWK_BUF_SIZE;
    audio_comp_buf_size = AUDIO_COMP_BUF_SIZE;

    // NOTE: set_wbna() should be called before any other dynamic
    // adjustment of the region attributes for cache.
    set_wbna(&argc, argv);

    /* ...start xos */
    board_id = start_rtos();

   /* ...get xaf version info*/
    TST_CHK_API(xaf_get_verinfo(ver_info), "xaf_get_verinfo");

    /* ...show xaf version info*/
    TST_CHK_API(print_verinfo(ver_info, (pUWORD8)"\'playback-usecase\'"), "print_verinfo");

    /* ...initialize tracing facility */
    TRACE_INIT("Xtensa Audio Framework - /'playback-usecase/' Sample App");

    /* ...check input arguments */
#if 0
    if (argc < (1 + MIN_INP_STRMS) || argc >(1 + MAX_INP_STRMS + MAX_OUT_STRMS))
    {
        PRINT_USAGE;
        return 0;
    }
#endif
    for (j = 0; j < NUM_COMP_IN_GRAPH; j++)
    {
        comp_probe[j] = 0;
        comp_probe_mask[j] = 0;
    }

    for (j = 0; j < MAX_INP_STRMS; j++)
    {
        p_input[j] = NULL;
        frame_size_cmdl[j] = 0;
    }

    for (j = 0; j < MAX_OUT_STRMS; j++)
        p_output[j] = NULL;

    for (i = 0; i < (argc - 1); i++)
    {
        char *filename_ptr;
        // Open input files
        if (NULL != strstr(argv[i + 1], "-infile:"))
        {
            filename_ptr = (char *)&(argv[i + 1][8]);
            ext = strrchr(argv[i + 1], '.');
            if (ext != NULL)
            {
                ext++;

                if (!strcmp(ext, "mp3")) {
                }
                else if (!strcmp(ext, "adts")) {
                }
                else {
                    FIO_PRINTF(stderr, "Unknown Decoder Extension '%s'\n", ext);
                    exit(-1);
                }
            }
            else
            {
                FIO_PRINTF(stderr, "Failed to open infile %d\n", i + 1);
                exit(-1);
            }
            /* ...open file */
            if ((fp = fio_fopen(filename_ptr, "rb")) == NULL)
            {
                FIO_PRINTF(stderr, "Failed to open '%s': %d\n", argv[i + 1], errno);
                exit(-1);
            }
            else
                FIO_PRINTF(stderr, "opened infile:'%s'\n", argv[i + 1]);
            p_input[num_in_strms] = fp;
            num_in_strms++;
        }
        // Open output file
        else if (NULL != strstr(argv[i + 1], "-outfile:"))
        {
            filename_ptr = (char *)&(argv[i + 1][9]);

            if ((ofp = fio_fopen(filename_ptr, "wb")) == NULL)
            {
                FIO_PRINTF(stderr, "Failed to open '%s': %d\n", filename_ptr, errno);
                exit(-1);
            }
            else
                FIO_PRINTF(stderr, "opened outfile:'%s'\n", argv[i + 1]);
            p_output[num_out_strms] = ofp;
            num_out_strms++;
        }
        /* ...only for internal testing */
        else if (NULL != strstr(argv[i + 1], "-frame_size:"))
        {
            char *frame_size_ptr = (char *)&(argv[i + 1][12]);

            if ((*frame_size_ptr) == '\0')
            {
                FIO_PRINTF(stderr, "Framesize is not provided\n");
                exit(-1);
            }
            frame_size_cmdl[nframe_size_cmdl] = atoi(frame_size_ptr);
            printf("frame_size=%d\n", frame_size_cmdl[nframe_size_cmdl]);
            nframe_size_cmdl++;
        }
        else if ((NULL != strstr(argv[i + 1], "-pr:")) || (NULL != strstr(argv[i + 1], "-probe:")) || (NULL != strstr(argv[i + 1], "-D:")) || (NULL != strstr(argv[i + 1], "-C:")))
        {
            continue; //parsed in utils as part of runtime actions
        }
        else if (NULL != strstr(argv[i + 1], "-probe-cfg:"))
        {
            char *token;
            char *ptr1 = NULL;
            char *pr_string;
            int  port_num;

            pr_string = (char *)&(argv[i + 1][11]);

            token = strtok_r(pr_string, ",", &ptr1);    //Component ID

            cid = atoi(token);
            if ((cid < 0) || (cid >= NUM_COMP_IN_GRAPH))
            {
                fprintf(stderr, "\n\nProbe-Config-Parse: Invalid component ID. Allowed range: 0-%d\n", NUM_COMP_IN_GRAPH - 1);
                return -1;
            }

            comp_probe[cid] = 1;

            for (token = strtok_r(NULL, ",", &ptr1); token != NULL; token = strtok_r(NULL, ",", &ptr1))
            {
                FILE *fp;
                char probefname[64];
                port_num = atoi(token);
                if ((port_num < 0) || (port_num > 7))
                {
                    fprintf(stderr, "Invalid port number %d. Allowed range: 0-7\n", port_num);
                    return -1;
                }
                sprintf(probefname, "comp%d_port%d.bin", cid, port_num);
                fp = fopen(probefname, "wb");
                fclose(fp);
                comp_probe_mask[cid] |= XAF_PORT_MASK(port_num);
            }
        }
        else
        {
            PRINT_USAGE;
            return 0;
        }
    }//for(;i;)

#ifdef RUNTIME_ACTIONS
    {
        int ret = parse_runtime_params(&runtime_params, argc - 1, argv, NUM_COMP_IN_GRAPH);
        if (ret) return ret;
    }
#endif

    if (num_in_strms == 0 || ofp == NULL)
    {
        PRINT_USAGE;
        exit(-1);
    }

    int out_thread_create_order[] = { XA_GAIN0, XA_SRC_PP0 };
    int extra_probe_threads_order[] = { XA_MIXER0, XA_MIMO12_0 };
    int thread_create_order[] = { XA_MP3_DEC0, XA_MP3_DEC1, XA_AAC_DEC0, XA_GAIN0, XA_SRC_PP0, XA_AAC_DEC1 };

    /* update connect/disconnect function pointers */
    gpcomp_connect = comp_connect;
    gpcomp_disconnect = comp_disconnect;

    for (i = 0; i < NUM_COMP_IN_GRAPH; i++)
    {
        comp_state[i] = COMP_INVALID;

        cid = comp_create_order[i];
        p_comp[cid] = NULL;
        memset(&comp_format[cid], 0, sizeof(xaf_format_t));
        comp_setup[cid] = NULL;
        comp_type[cid] = -1;
        g_comp_id[cid] = NULL;
        comp_ninbuf[cid] = 0;
        comp_noutbuf[cid] = 0;
        comp_framesize[cid] = XAF_INBUF_SIZE; /* XAF_INBUF_SIZE:4096 */

        switch (cid)
        {
        case XA_GAIN0:
            comp_format[cid].sample_rate = PCM_GAIN_SAMPLE_RATE;
            comp_format[cid].channels = PCM_GAIN_NUM_CH;
            comp_format[cid].pcm_width = PCM_GAIN_SAMPLE_WIDTH;
            comp_setup[cid] = pcm_gain_setup;
            comp_type[cid] = XAF_POST_PROC;
            g_comp_id[cid] = "post-proc/pcm_gain";
            comp_ninbuf[cid] = 0;
            comp_noutbuf[cid] = 1;
            if (frame_size_cmdl[cid - XA_GAIN0])
                comp_framesize[cid] = frame_size_cmdl[cid - XA_GAIN0];
            mcps_p_output = p_output[cid - XA_GAIN0];
            break;

        case XA_SRC_PP0:
            comp_format[cid].sample_rate = SRC_PP_SAMPLE_RATE_IN;
            comp_format[cid].channels = SRC_PP_NUM_CH;
            comp_format[cid].pcm_width = SRC_PP_SAMPLE_WIDTH;
            comp_setup[cid] = src_setup;
            out_sample_rate = SRC_PP_SAMPLE_RATE_OUT;
            comp_type[cid] = XAF_POST_PROC;
            g_comp_id[cid] = "audio-fx/src-pp";
            comp_ninbuf[cid] = 0;
            comp_noutbuf[cid] = 1;
            break;

        case XA_MIMO12_0:
            comp_format[cid].sample_rate = PCM_SPLIT_SAMPLE_RATE;
            comp_format[cid].channels = PCM_SPLIT_IN_NUM_CH;
            comp_format[cid].pcm_width = PCM_SPLIT_PCM_WIDTH;
            comp_setup[cid] = mimo12_setup;
            comp_type[cid] = XAF_MIMO_PROC_12;
            g_comp_id[cid] = "mimo-proc12/pcm_split";
            comp_ninbuf[cid] = 0;
            comp_noutbuf[cid] = 0;
            break;

        case XA_MP3_DEC0:
        case XA_MP3_DEC1:
            comp_format[cid].pcm_width = MP3_DEC_PCM_WIDTH;
            comp_setup[cid] = mp3_setup;
            comp_type[cid] = XAF_DECODER;
            g_comp_id[cid] = "audio-decoder/mp3";
            comp_ninbuf[cid] = 2;
            comp_noutbuf[cid] = 0;
            break;

        case XA_AAC_DEC0:
        case XA_AAC_DEC1:
            comp_format[cid].pcm_width = AAC_DEC_PCM_WIDTH;
            comp_setup[cid] = aac_setup;
            comp_type[cid] = XAF_DECODER;
            g_comp_id[cid] = "audio-decoder/aac";
            comp_ninbuf[cid] = 2;
            comp_noutbuf[cid] = 0;
            break;

        case XA_MIXER0:
            comp_format[cid].sample_rate = MIXER_SAMPLE_RATE;
            comp_format[cid].channels = MIXER_NUM_CH;
            comp_format[cid].pcm_width = MIXER_PCM_WIDTH;
            comp_setup[cid] = mixer_setup;
            comp_type[cid] = XAF_MIXER;
            g_comp_id[cid] = "mixer";
            comp_ninbuf[cid] = 0;
            comp_noutbuf[cid] = 0;
            break;

        default:
            FIO_PRINTF(stderr, "Check component type in comp_create_order table\n");
            return -1;
            break;
        }//switch()
    }//for(;i;)

    mem_handle = mem_init();

    xaf_adev_config_t adev_config;
    TST_CHK_API(xaf_adev_config_default_init(&adev_config), "xaf_adev_config_default_init");

    adev_config.pmem_malloc =  mem_malloc;
    adev_config.pmem_free =  mem_free;
#ifndef XA_DISABLE_EVENT
    adev_config.app_event_handler_cb =  xa_app_receive_events_cb;
#endif
    adev_config.audio_framework_buffer_size =  audio_frmwk_buf_size;
    adev_config.audio_component_buffer_size =  audio_comp_buf_size;
    TST_CHK_API(xaf_adev_open(&p_adev, &adev_config),  "xaf_adev_open");
    FIO_PRINTF(stdout, "Audio Device Ready\n");

#ifndef XA_DISABLE_EVENT
    xf_thread_t event_handler_thread;
    unsigned char event_handler_stack[STACK_SIZE];
    int event_handler_args[1] = {0}; // Dummy

    __xf_thread_create(&event_handler_thread, event_handler_entry, (void *)event_handler_args, "Event Handler Thread", event_handler_stack, STACK_SIZE, XAF_APP_THREADS_PRIORITY);
#endif    


    cid = XA_MIMO12_0;
    TST_CHK_API_COMP_CREATE(p_adev, &p_comp[cid], g_comp_id[cid], comp_ninbuf[cid], comp_noutbuf[cid], NULL, comp_type[cid], "xaf_comp_create");
    comp_state[cid] = COMP_CREATED; /*created*/
    TST_CHK_API(comp_setup[cid](p_comp[cid], &comp_format[cid], 2, comp_probe[cid], comp_probe_mask[cid]), "comp_setup");

    cid = XA_MIXER0;
    TST_CHK_API_COMP_CREATE(p_adev, &p_comp[cid], g_comp_id[cid], comp_ninbuf[cid], comp_noutbuf[cid], NULL, comp_type[cid], "xaf_comp_create");
    comp_state[cid] = COMP_CREATED; /*created*/
    TST_CHK_API(comp_setup[cid](p_comp[cid], &comp_format[cid], 2, comp_probe[cid], comp_probe_mask[cid]), "comp_setup");

    for (k = 0; k < 3; k++)
    {
        cid = comp_create_order[k];

        TST_CHK_API_COMP_CREATE(p_adev, &p_comp[cid], g_comp_id[cid], comp_ninbuf[cid], comp_noutbuf[cid], &p_comp_inbuf[cid][0], comp_type[cid], "xaf_comp_create");
        comp_state[cid] = COMP_CREATED; /*created*/
        TST_CHK_API(comp_setup[cid](p_comp[cid], &comp_format[cid], 2, comp_probe[cid], comp_probe_mask[cid]), "comp_setup");

        TST_CHK_API(xaf_comp_process(p_adev, p_comp[cid], NULL, 0, XAF_START_FLAG), "xaf_comp_process");

        for (i = 0; i < 2; i++)
        {
            TST_CHK_API(read_input(p_comp_inbuf[cid][i], buf_length, &read_length, p_input[cid], comp_type[cid]), "read_input");

            if (read_length)
                TST_CHK_API(xaf_comp_process(p_adev, p_comp[cid], p_comp_inbuf[cid][i], read_length, XAF_INPUT_READY_FLAG), "xaf_comp_process");
            else
            {    
                TST_CHK_API(xaf_comp_process(p_adev, p_comp[cid], NULL, 0, XAF_INPUT_OVER_FLAG), "xaf_comp_process");
                break;
            }
        }

        /* ...initialization loop */
        while (1)
        {
            TST_CHK_API(xaf_comp_get_status(p_adev, p_comp[cid], &comp_status, &dec_info[0]), "xaf_comp_get_status");

            if (comp_status == XAF_INIT_DONE || comp_status == XAF_EXEC_DONE) break;

            if (comp_status == XAF_NEED_INPUT)
            {
                void *p_buf = (void *)dec_info[0];
                int size = dec_info[1];

                TST_CHK_API(read_input(p_buf, size, &read_length, p_input[cid], comp_type[cid]), "read_input");

                if (read_length)
                    TST_CHK_API(xaf_comp_process(p_adev, p_comp[cid], p_buf, read_length, XAF_INPUT_READY_FLAG), "xaf_comp_process");
                else
                {    
                    TST_CHK_API(xaf_comp_process(p_adev, p_comp[cid], NULL, 0, XAF_INPUT_OVER_FLAG), "xaf_comp_process");
                    break;
                }
            }
        }

        if (comp_status != XAF_INIT_DONE)
        {
            FIO_PRINTF(stderr, "Failed to init\n");
            exit(-1);
        }
        else
        {
            FIO_PRINTF(stderr, "init done for comp_type=%d\n", comp_type[cid]);
        }

        TST_CHK_API(xaf_connect(p_comp[cid], 1, p_comp[XA_MIXER0], k, 4), "xaf_connect"); 
    }//for(;k;)

    TST_CHK_API(xaf_comp_process(p_adev, p_comp[XA_MIXER0], NULL, 0, XAF_START_FLAG), "xaf_comp_process");
    TST_CHK_API(xaf_comp_get_status(p_adev, p_comp[XA_MIXER0], &comp_status, &dec_info[0]), "xaf_comp_get_status");

    TST_CHK_API(xaf_connect(p_comp[XA_MIXER0], 4, p_comp[XA_MIMO12_0], 0, 4), "xaf_connect"); 
    TST_CHK_API(xaf_comp_process(p_adev, p_comp[XA_MIMO12_0], NULL, 0, XAF_START_FLAG), "xaf_comp_process");
    TST_CHK_API(xaf_comp_get_status(p_adev, p_comp[XA_MIMO12_0], &comp_status, &dec_info[0]), "xaf_comp_get_status");

    cid = XA_GAIN0;
    TST_CHK_API_COMP_CREATE(p_adev, &p_comp[cid], g_comp_id[cid], comp_ninbuf[cid], comp_noutbuf[cid], NULL, comp_type[cid], "xaf_comp_create");
    comp_state[cid] = COMP_CREATED; /*created*/
    TST_CHK_API(comp_setup[cid](p_comp[cid], &comp_format[cid], 3, comp_framesize[cid], comp_probe[cid], comp_probe_mask[cid]), "comp_setup");

    cid = XA_SRC_PP0;
    TST_CHK_API_COMP_CREATE(p_adev, &p_comp[cid], g_comp_id[cid], comp_ninbuf[cid], comp_noutbuf[cid], NULL, comp_type[cid], "xaf_comp_create");
    comp_state[cid] = COMP_CREATED; /*created*/
    TST_CHK_API(comp_setup[cid](p_comp[cid], &comp_format[cid], 3, out_sample_rate, comp_probe[cid], comp_probe_mask[cid]), "src_setup");

    TST_CHK_API(xaf_connect(p_comp[XA_MIMO12_0], 1, p_comp[XA_GAIN0], 0, 4), "xaf_connect"); 
    TST_CHK_API(xaf_connect(p_comp[XA_MIMO12_0], 2, p_comp[XA_SRC_PP0], 0, 4), "xaf_connect");

    TST_CHK_API(xaf_comp_process(p_adev, p_comp[XA_GAIN0], NULL, 0, XAF_START_FLAG), "xaf_comp_process");
    TST_CHK_API(xaf_comp_get_status(p_adev, p_comp[XA_GAIN0], &comp_status, &dec_info[0]), "xaf_comp_get_status");
    TST_CHK_API(xaf_comp_process(p_adev, p_comp[XA_SRC_PP0], NULL, 0, XAF_START_FLAG), "xaf_comp_process");
    TST_CHK_API(xaf_comp_get_status(p_adev, p_comp[XA_SRC_PP0], &comp_status, &dec_info[0]), "xaf_comp_get_status");

#ifdef XAF_PROFILE
    clk_start();
#endif

    for (k = 0; k < 4; k++)
    {
        cid = inp_thread_create_order[k];
        comp_thread_args[cid][0] = p_adev;
        comp_thread_args[cid][1] = p_comp[cid];
        comp_thread_args[cid][2] = p_input[k];
        comp_thread_args[cid][3] = NULL;
        comp_thread_args[cid][4] = &comp_type[cid];
        comp_thread_args[cid][5] = (void *)g_comp_id[cid];
        comp_thread_args[cid][6] = (void *)&inp_thread_create_order[k];
        if (k != 3)
        {
            __xf_thread_create(&comp_thread[cid], comp_process_entry, comp_thread_args[cid], "playback-usecase Thread", comp_stack[cid], STACK_SIZE, XAF_APP_THREADS_PRIORITY);
            FIO_PRINTF(stderr, "Created thread %d comp_type=%d\n", k, comp_type[cid]);
        }
    }

    for (k = 0; k < 2; k++)
    {
        cid = out_thread_create_order[k];
        comp_thread_args[cid][0] = p_adev;
        comp_thread_args[cid][1] = p_comp[cid];
        comp_thread_args[cid][2] = NULL;
        comp_thread_args[cid][3] = p_output[k];
        comp_thread_args[cid][4] = &comp_type[cid];
        comp_thread_args[cid][5] = (void *)g_comp_id[cid];
        comp_thread_args[cid][6] = (void *)&out_thread_create_order[k];
        __xf_thread_create(&comp_thread[cid], comp_process_entry, comp_thread_args[cid], "playback-usecase Thread", comp_stack[cid], STACK_SIZE, XAF_APP_THREADS_PRIORITY);
        FIO_PRINTF(stderr, "Created thread %d comp_type=%d\n", k, comp_type[cid]);
    }

#ifdef RUNTIME_ACTIONS
    void *threads[NUM_COMP_IN_GRAPH];
    int comp_nbufs[NUM_COMP_IN_GRAPH];

    for (k = 0; k < NUM_COMP_IN_GRAPH; k++)
    {
        comp_nbufs[k] = comp_ninbuf[k] + comp_noutbuf[k];
        threads[k] = &comp_thread[k];
    }

    for (k = 0; k < NUM_EXTRA_PROBE_THREADS; k++)
    {
        cid = extra_probe_threads_order[k];
        comp_thread_args[cid][0] = p_adev;
        comp_thread_args[cid][1] = p_comp[cid];
        comp_thread_args[cid][2] = NULL;
        comp_thread_args[cid][3] = NULL;
        comp_thread_args[cid][4] = &comp_type[cid];
        comp_thread_args[cid][5] = (void *)g_comp_id[cid];
        comp_thread_args[cid][6] = (void *)&extra_probe_threads_order[k];

        __xf_thread_init(&comp_thread[cid]); 
    }

    {
        int ret = execute_runtime_actions(runtime_params, p_adev, &p_comp[0], comp_nbufs, &threads[0], NUM_COMP_IN_GRAPH, &comp_thread_args[0][0], NUM_THREAD_ARGS, &comp_stack[0][0]);
        if (ret) return ret;
    }

#endif  //RUNTIME_ACTIONS

    int tot_io_threads = NUM_IO_THREADS;

    if (num_in_strms == 4)
    {
        tot_io_threads++;
    }


    for (k = 0; k < tot_io_threads; k++)
    {
        cid = thread_create_order[k];

        if (__xf_thread_get_state(&comp_thread[cid]) != XF_THREAD_STATE_INVALID)
        {
            __xf_thread_join(&comp_thread[cid], NULL);
            FIO_PRINTF(stdout, "component %d thread joined with exit code %x\n", cid, i);

        }
    }

    for (k = 0; k < (NUM_EXTRA_PROBE_THREADS); k++)
    {
        cid = extra_probe_threads_order[k];
        __xf_thread_join(&comp_thread[cid], NULL);
        FIO_PRINTF(stdout, "component %d thread joined with exit code %x\n", cid, i);
    }

#ifdef XAF_PROFILE
    compute_total_frmwrk_cycles();
    clk_stop();
#endif

    {
        /* collect memory stats before closing the device */
        WORD32 meminfo[5];
        if (xaf_get_mem_stats(p_adev, &meminfo[0]))
        {
            FIO_PRINTF(stdout, "Init is incomplete, reliable memory stats are unavailable.\n");
        }
        else
        {
            FIO_PRINTF(stderr, "Local Memory used by DSP Components, in bytes            : %8d of %8d\n", meminfo[0], adev_config.audio_component_buffer_size);
            FIO_PRINTF(stderr, "Shared Memory used by Components and Framework, in bytes : %8d of %8d\n", meminfo[1], adev_config.audio_framework_buffer_size);
            FIO_PRINTF(stderr, "Local Memory used by Framework, in bytes                 : %8d\n", meminfo[2]);
        }
    }
    /* ...exec done, clean-up */
    for (k = 0; k < tot_io_threads; k++)
    {
        cid = thread_create_order[k];
        __xf_thread_destroy(&comp_thread[cid]);
        FIO_PRINTF(stdout, "thread %d is deleted\n", cid);
    }

    for (k = 0; k < (NUM_EXTRA_PROBE_THREADS); k++)
    {
        cid = extra_probe_threads_order[k];
        __xf_thread_destroy(&comp_thread[cid]);
        FIO_PRINTF(stdout, "thread %d is deleted\n", cid);
    }


    for (k = 0; k < (NUM_COMP_IN_GRAPH); k++)
    {
        cid = comp_create_order[k];

        /* created component only be deleted and not needed to call delete again for already deleted component */
        if (comp_state[cid] == COMP_CREATED)
        {
            TST_CHK_API(xaf_comp_delete(p_comp[cid]), "xaf_comp_delete");
            FIO_PRINTF(stdout, "comp %d is deleted\n", cid);
        }

    }

#ifndef XA_DISABLE_EVENT
    extern UWORD32 g_event_handler_exit;
    g_event_handler_exit = 1;
    __xf_thread_join(&event_handler_thread, NULL);
    __xf_thread_destroy(&event_handler_thread);
    FIO_PRINTF(stdout, "Event handler thread joined with exit code %x\n", i);
#endif    

    TST_CHK_API(xaf_adev_close(p_adev, XAF_ADEV_NORMAL_CLOSE), "xaf_adev_close");
    FIO_PRINTF(stdout, "Audio device closed\n\n");

    mem_exit();
#ifndef XA_DISABLE_EVENT
    xa_app_free_event_list();
#endif

    dsp_comps_cycles = aac_dec_cycles + dec_cycles + mix_cycles + pcm_split_cycles + src_cycles;
    dsp_mcps += compute_comp_mcps(num_bytes_write, dsp_comps_cycles, comp_format[XA_GAIN0], &strm_duration);


    TST_CHK_API(print_mem_mcps_info(mem_handle, NUM_COMP_IN_GRAPH), "print_mem_mcps_info");

    for (i = 0; i < num_in_strms; i++)
    {
        if (p_input[i]) fio_fclose(p_input[i]);
    }

    for (i = 0; i < num_out_strms; i++)
    {
        if (p_output[i]) fio_fclose(p_output[i]);
    }

    fio_quit();
    
    /* ...deinitialize tracing facility */
    TRACE_DEINIT();

    return 0;
}

