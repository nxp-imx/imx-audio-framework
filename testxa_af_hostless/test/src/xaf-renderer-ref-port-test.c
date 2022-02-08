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

#include "audio/xa-aec23-api.h"
#include "audio/xa-pcm-gain-api.h"
#include "audio/xa-renderer-api.h"
#include "xaf-utils-test.h"
#include "xaf-fio-test.h"

#define TESTBENCH_USAGE FIO_PRINTF(stdout, "\nUsage: %s -infile:<>.pcm -infile:<>.pcm -outfile:<>.pcm -outfile:<>.pcm\n", argv[0]);\
    FIO_PRINTF(stdout, "\nNote: Renderer-plugin writes output to the file named 'renderer_out.pcm' in the execution directory.\n");\
    FIO_PRINTF(stdout, "\nNote: This testbench does not support runtime commands for the 'Renderer' component.\n\n");

#define AUDIO_FRMWK_BUF_SIZE   (256 << 12)
#define AUDIO_COMP_BUF_SIZE    (1024 << 12)

enum{
  XA_COMP=-1,
  XA_GAIN0 = 0,
  XA_RENDERER0 = 1,
  XA_AEC23_0 = 2,
  XA_GAIN1 = 3,
  XA_GAIN2 = 4,
  XA_GAIN3 = 5,
  NUM_COMP_IN_GRAPH,
};

#define NUM_INP_THREADS   2
#define NUM_OUT_THREADS   2
#define NUM_THREADS	      (NUM_INP_THREADS + NUM_OUT_THREADS)

const int comp_create_order[] = {XA_GAIN0, XA_RENDERER0, XA_GAIN1, XA_AEC23_0, XA_GAIN2, XA_GAIN3}; const int comp_thread_order[] = {XA_GAIN0, XA_GAIN1, XA_GAIN2, XA_GAIN3};

char comp_string[NUM_COMP_IN_GRAPH * 15] = {"PCMGAIN0:0, RENDERER:1, AEC23:2, PCMGAIN1:3, PCMGAIN2:4, PCMGAIN3:5"};
#define COMPONENT_STRING FIO_PRINTF(stdout, "(Component:Component_ID) : %s\n\n", comp_string);

#define RUNTIME_ACTIONS 1
#ifdef RUNTIME_ACTIONS
#define PRINT_USAGE \
	TESTBENCH_USAGE;\
	COMPONENT_STRING;\
	runtime_param_usage(); \
	/*runtime_param_reconnect_usage();*/\
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
    extern long long dsp_comps_cycles, aec23_cycles, capturer_cycles, renderer_cycles ,mixer_cycles, pcm_gain_cycles;
    extern double dsp_mcps;
#endif

/* Dummy unused functions */
XA_ERRORCODE xa_mp3_decoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_aac_decoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_mixer(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_mp3_encoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_src_pp_fx(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
//XA_ERRORCODE xa_renderer(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_capturer(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_amr_wb_decoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_hotword_decoder(xa_codec_handle_t p_xa_module_obj, WORD32 i_cmd, WORD32 i_idx, pVOID pv_value){return 0;}
XA_ERRORCODE xa_vorbis_decoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
//XA_ERRORCODE xa_pcm_gain(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_dummy_aec22(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
//XA_ERRORCODE xa_dummy_aec23(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_pcm_split(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_mimo_mix(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_dummy_wwd(xa_codec_handle_t p_xa_module_obj, WORD32 i_cmd, WORD32 i_idx, pVOID pv_value) {return 0;}
XA_ERRORCODE xa_dummy_hbuf(xa_codec_handle_t p_xa_module_obj, WORD32 i_cmd, WORD32 i_idx, pVOID pv_value) {return 0;}
XA_ERRORCODE xa_opus_encoder(xa_codec_handle_t p_xa_module_obj, WORD32 i_cmd, WORD32 i_idx, pVOID pv_value) {return 0;}
XA_ERRORCODE xa_dummy_wwd_msg(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_dummy_hbuf_msg(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}

#define MAX_INP_STRMS           2
#define MAX_OUT_STRMS           2
#define MIN_INP_STRMS           2

#define RENDERER_SAMPLE_RATE	16000
#define RENDERER_NUM_CH		    2
#define RENDERER_PCM_WIDTH	    16
#define RENDERER_FRAME_SIZE    (1024)
static int renderer_setup(void *p_comp, xaf_format_t *p_format, int nvar_args, ...)
{
#define RENDERER_NUM_SET_PARAMS	4
    int param[RENDERER_NUM_SET_PARAMS*2];
    int frame_size = RENDERER_FRAME_SIZE;

    va_list varg_list;
    va_start(varg_list, nvar_args);

    frame_size = va_arg(varg_list, int);

    param[0*2+0] = XA_RENDERER_CONFIG_PARAM_PCM_WIDTH;
    param[0*2+1] = p_format->pcm_width;
    param[1*2+0] = XA_RENDERER_CONFIG_PARAM_CHANNELS;
    param[1*2+1] = p_format->channels;
    param[2*2+0] = XA_RENDERER_CONFIG_PARAM_SAMPLE_RATE;
    param[2*2+1] = p_format->sample_rate;
    param[3*2+0] = XA_RENDERER_CONFIG_PARAM_FRAME_SIZE;
    param[3*2+1] = frame_size;

    fprintf(stderr, "renderer:frame_size=%d\n", frame_size);
    va_end(varg_list);

    return(xaf_comp_set_config(p_comp, RENDERER_NUM_SET_PARAMS, &param[0]));
}

#define AEC23_SAMPLE_RATE         16000
#define AEC23_NUM_IN_CH           2
#define AEC23_PCM_WIDTH           16

static int aec23_setup(void *p_comp, xaf_format_t *p_format, int nvar_args, ...)
{
#define AEC23_NUM_SET_PARAMS	3
    int param[AEC23_NUM_SET_PARAMS*2 + 2];
    int probe_enabled;
    va_list varg_list;

    va_start(varg_list, nvar_args);

    param[0*2+0] = XA_AEC23_CONFIG_PARAM_CHANNELS;
    param[0*2+1] = p_format->channels;          
    param[1*2+0] = XA_AEC23_CONFIG_PARAM_SAMPLE_RATE;
    param[1*2+1] = p_format->sample_rate;
    param[2*2+0] = XA_AEC23_CONFIG_PARAM_PCM_WIDTH;
    param[2*2+1] = p_format->pcm_width;

    va_arg(varg_list, int); //framesize is unused dummy
    probe_enabled = va_arg(varg_list, int);
    if(probe_enabled)
    {
      param[3*2+0] = XAF_COMP_CONFIG_PARAM_PROBE_ENABLE;
      param[3*2+1] = va_arg(varg_list, int);
    
      fprintf(stderr, "AEC23_SETUP: PROBE_ENABLED\n");
    }
    va_end(varg_list);

    return(xaf_comp_set_config(p_comp, AEC23_NUM_SET_PARAMS + probe_enabled, &param[0]));
}

#define PCM_GAIN_SAMPLE_WIDTH   16
#define PCM_GAIN_NUM_CH         2
#define PCM_GAIN_SAMPLE_RATE    16000
#define PCM_GAIN_IDX_FOR_GAIN   0

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


#if 0 /* unused function */
static int get_comp_config(void *p_comp, xaf_format_t *comp_format)
{
    int param[6];
    int ret;


    TST_CHK_PTR(p_comp, "get_comp_config");
    TST_CHK_PTR(comp_format, "get_comp_config");

    param[0] = XA_MIXER_CONFIG_PARAM_CHANNELS;
    param[2] = XA_MIXER_CONFIG_PARAM_PCM_WIDTH;
    param[4] = XA_MIXER_CONFIG_PARAM_SAMPLE_RATE;


    ret = xaf_comp_get_config(p_comp, 3, &param[0]);
    if(ret < 0)
        return ret;

    comp_format->channels = param[1];
    comp_format->pcm_width = param[3];
    comp_format->sample_rate = param[5];

    return 0;
}
#endif

void fio_quit()
{
    return;
}

int main_task(int argc, char **argv)
{
    void *p_adev = NULL;
    xf_thread_t comp_thread[NUM_COMP_IN_GRAPH];
    unsigned char comp_stack[NUM_COMP_IN_GRAPH][STACK_SIZE];
    void * p_comp[NUM_COMP_IN_GRAPH];
    xaf_comp_status comp_status;
    int dec_info[4];
    void *p_input[MAX_INP_STRMS], *p_output[MAX_OUT_STRMS];
    void *comp_input[NUM_COMP_IN_GRAPH][MAX_INP_STRMS], *comp_output[NUM_COMP_IN_GRAPH][MAX_OUT_STRMS];
    xf_id_t comp_id[NUM_COMP_IN_GRAPH];
    void *comp_thread_args[NUM_COMP_IN_GRAPH][NUM_THREAD_ARGS];
    FILE *fp, *ofp;
    void *comp_inbuf[2];
    void *source_comp_inbuf[NUM_INP_THREADS][2];
    int buf_length = XAF_INBUF_SIZE;
    int read_length;
    int i, j, k, cid;
    int (*comp_setup[NUM_COMP_IN_GRAPH])(void * p_comp, xaf_format_t *, int, ...);
    const char *ext;
    pUWORD8 ver_info[3] = {0,0,0};    //{ver,lib_rev,api_rev}
    unsigned short board_id = 0;
    mem_obj_t* mem_handle;
    xaf_comp_type comp_type[NUM_COMP_IN_GRAPH];
    int num_in_strms = 0;
    int num_out_strms = 0;
    xaf_format_t comp_format[NUM_COMP_IN_GRAPH];
    int comp_ninbuf[NUM_COMP_IN_GRAPH];
    int comp_noutbuf[NUM_COMP_IN_GRAPH];
    int comp_framesize[NUM_COMP_IN_GRAPH];
    int frame_size_cmdl[MAX_INP_STRMS];
    int nframe_size_cmdl = 0;
    int comp_probe[NUM_COMP_IN_GRAPH];
    int comp_probe_mask[NUM_COMP_IN_GRAPH];
#ifdef RUNTIME_ACTIONS
    void *runtime_params;
#endif

#ifdef XAF_PROFILE
    frmwk_cycles = 0;
    fread_cycles = 0;
    fwrite_cycles = 0;
    dsp_comps_cycles = 0;
    aec23_cycles = 0;
    tot_cycles = 0;
    num_bytes_read = 0;
    num_bytes_write = 0;
    pcm_gain_cycles = 0;
    renderer_cycles =0;
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
    TST_CHK_API(print_verinfo(ver_info,(pUWORD8)"\'renderer-output as aec23-reference-input\'"), "print_verinfo");

    /* ...initialize tracing facility */
    TRACE_INIT("Xtensa Audio Framework - /'renderer-output as aec23-reference-input/' Sample App");

    /* ...check input arguments */
#if 0
    if (argc < (1+MIN_INP_STRMS) || argc > (1+MAX_INP_STRMS+MAX_OUT_STRMS))
    {
        PRINT_USAGE;
        return 0;
    }
#endif

    for (j=0; j<NUM_COMP_IN_GRAPH; j++)
    {
      comp_probe[j] = 0;
      comp_probe_mask[j] = 0;
    }

    for (j=0; j<MAX_INP_STRMS; j++)
    {
      p_input[j] = NULL;
      frame_size_cmdl[j] = 0;
    }

    for (j=0; j<MAX_OUT_STRMS; j++)
      p_output[j] = NULL;

    for (i=0; i<(argc-1); i++)
    {
        char *filename_ptr;
        // Open input files
        if(NULL != strstr(argv[i+1], "-infile:"))
        {
            filename_ptr = (char *)&(argv[i+1][8]);
            ext = strrchr(argv[i+1], '.');
            ext++;
        
            /* ...open file */
            if ((fp = fio_fopen(filename_ptr, "rb")) == NULL)
            {
               FIO_PRINTF(stderr, "Failed to open '%s': %d\n", argv[i+1], errno);
               exit(-1);
            }
	    else
               FIO_PRINTF(stderr, "opened infile:'%s' i=%d\n", argv[i+1], i);
            p_input[num_in_strms] = fp;
            num_in_strms++;
        }
        // Open output file
        else if(NULL != strstr(argv[i+1], "-outfile:"))
        {
            filename_ptr = (char *)&(argv[i+1][9]);

            if ((ofp = fio_fopen(filename_ptr, "wb")) == NULL)
            {
               FIO_PRINTF(stderr, "Failed to open '%s': %d\n", filename_ptr, errno);
               exit(-1);
            }
            else
               FIO_PRINTF(stderr, "opened outfile:'%s'\n", argv[i+1]);
    	    p_output[num_out_strms] = ofp;
            num_out_strms++;
        }
        else if(NULL != strstr(argv[i+1], "-frame_size:"))
        {
            char *frame_size_ptr = (char *)&(argv[i+1][12]);
            
            if ((*frame_size_ptr) == '\0' )
            {
                FIO_PRINTF(stderr, "Framesize is not provide\n");
                exit(-1);
            }
            frame_size_cmdl[nframe_size_cmdl] = atoi(frame_size_ptr);
            printf("frame_size=%d\n", frame_size_cmdl[nframe_size_cmdl]);
            nframe_size_cmdl++;
        }
        else if ( (NULL != strstr(argv[i+1], "-C:")) || (NULL != strstr(argv[i+1], "-D:")))
        {
            continue; //parsed in utils as part of runtime actions
        }
        else if ( (NULL != strstr(argv[i+1], "-pr:")) || (NULL != strstr(argv[i+1], "-probe:")) )
        {
            continue; //parsed in utils as part of runtime actions
        }
        else if (NULL != strstr(argv[i+1], "-probe-cfg:"))
        {
            char *token;
            char *ptr1 = NULL;
            char *pr_string;
            int  port_num;

            pr_string = (char *)&(argv[i+1][11]);
                    
            token = strtok_r(pr_string, ",", &ptr1);    //Component ID
        
            cid = atoi(token); 
            if ( (cid < 0) || (cid >= NUM_COMP_IN_GRAPH) )
            {
                fprintf(stderr, "\n\nProbe-Config-Parse: Invalid component ID. Allowed range: 0-%d\n", NUM_COMP_IN_GRAPH-1);
                return -1;
            }
        
            comp_probe[cid] = 1;
                        
            for( token = strtok_r(NULL, ",", &ptr1); token != NULL; token = strtok_r(NULL, ",", &ptr1) )
            {    
                FILE *fp;
                char probefname[64];
                port_num = atoi(token);
                if ( (port_num < 0 ) || ( port_num > 7))
                {
                    fprintf(stderr, "Invalid port number %d. Allowed range: 0-7\n", port_num );
                    return -1;
                }        
                sprintf(probefname, "comp%d_port%d.bin", cid, port_num);
                /* to avoid appending the probe output to the previous data */
                fp = fopen(probefname, "wb");
                fclose(fp);
                comp_probe_mask[cid] |=  XAF_PORT_MASK( port_num );    
            }             
        }
        else
        {
            PRINT_USAGE;
            return 0;
        }
    }//for(;i;)

    if(num_in_strms == 0 || ofp == NULL)
    {
        PRINT_USAGE;
        exit(-1);
    }

#ifdef RUNTIME_ACTIONS
    {
        int ret = parse_runtime_params(&runtime_params, argc-1, argv, NUM_COMP_IN_GRAPH);
        if (ret) return ret;
    }    
#endif

    mem_handle = mem_init();

    xaf_adev_config_t adev_config;
    TST_CHK_API(xaf_adev_config_default_init(&adev_config), "xaf_adev_config_default_init");

    adev_config.pmem_malloc =  mem_malloc;
    adev_config.pmem_free =  mem_free;
    adev_config.audio_framework_buffer_size =  audio_frmwk_buf_size;
    adev_config.audio_component_buffer_size =  audio_comp_buf_size;
    TST_CHK_API(xaf_adev_open(&p_adev, &adev_config),  "xaf_adev_open");
    FIO_PRINTF(stdout,"Audio Device Ready\n");

    for (i=0; i<NUM_COMP_IN_GRAPH; i++)
    {
	    cid = comp_create_order[i];
    	p_comp[cid] = NULL;
    	memset(&comp_format[cid], 0, sizeof(xaf_format_t));
	    comp_setup[cid] = NULL;
    	comp_type[cid] = -1;
        comp_id[cid]    = NULL;
    	comp_ninbuf[cid] = 0;
	    comp_noutbuf[cid] = 0;
    	comp_framesize[cid] = XAF_INBUF_SIZE; /* XAF_INBUF_SIZE:4096 */

    	for (j=0; j<MAX_INP_STRMS; j++)
	      comp_input[cid][j] = NULL;
    	for (j=0; j<MAX_OUT_STRMS; j++)
    	  comp_output[cid][j] = NULL;

		switch(cid)
		{
		  case XA_RENDERER0:
                comp_format[cid].sample_rate = RENDERER_SAMPLE_RATE;
                comp_format[cid].channels    = RENDERER_NUM_CH;
                comp_format[cid].pcm_width   = RENDERER_PCM_WIDTH;
                comp_setup[cid] = renderer_setup;
                comp_type[cid] = XAF_RENDERER;
                comp_id[cid]    = "renderer";
        	    comp_framesize[cid] = RENDERER_FRAME_SIZE; 
                if(frame_size_cmdl[cid])
        	        comp_framesize[cid] = frame_size_cmdl[cid];
          break;
        
		  case XA_GAIN0:
        	    //comp_format[cid].sample_rate = PCM_GAIN_SAMPLE_RATE;
        	    comp_format[cid].sample_rate = RENDERER_SAMPLE_RATE;
        	    comp_format[cid].channels    = PCM_GAIN_NUM_CH;
        	    comp_format[cid].pcm_width   = PCM_GAIN_SAMPLE_WIDTH;
        	    comp_setup[cid] = pcm_gain_setup;
        	    comp_type[cid] = XAF_POST_PROC;
                comp_id[cid]    = "post-proc/pcm_gain";
		        comp_ninbuf[cid]  = 2;
		        comp_input[cid][0] = p_input[0];
                if(frame_size_cmdl[cid])
        	        comp_framesize[cid] = frame_size_cmdl[cid]; /* XAF_INBUF_SIZE:4096 */
                fprintf(stderr,"GAIN0 frame_size:%d\n", comp_framesize[cid]);
		  break;
    
		  case XA_GAIN1:
        	    comp_format[cid].sample_rate = PCM_GAIN_SAMPLE_RATE;
        	    comp_format[cid].channels    = PCM_GAIN_NUM_CH;
        	    comp_format[cid].pcm_width   = PCM_GAIN_SAMPLE_WIDTH;
        	    comp_setup[cid] = pcm_gain_setup;
        	    comp_type[cid] = XAF_POST_PROC;
                comp_id[cid]    = "post-proc/pcm_gain";
		        comp_ninbuf[cid]  = 2;
		        comp_input[cid][0] = p_input[1];
		  break;
    
		  case XA_AEC23_0:
        	    comp_format[cid].sample_rate = AEC23_SAMPLE_RATE;
        	    comp_format[cid].channels    = AEC23_NUM_IN_CH;
        	    comp_format[cid].pcm_width   = AEC23_PCM_WIDTH;
        	    comp_setup[cid] = aec23_setup;
        	    comp_type[cid] = XAF_MIMO_PROC_23;
                comp_id[cid]    = "mimo-proc23/aec23";
		  break;
    
		  case XA_GAIN2:
        	    comp_format[cid].sample_rate = PCM_GAIN_SAMPLE_RATE;
        	    comp_format[cid].channels    = PCM_GAIN_NUM_CH;
        	    comp_format[cid].pcm_width   = PCM_GAIN_SAMPLE_WIDTH;
        	    comp_setup[cid] = pcm_gain_setup;
        	    comp_type[cid] = XAF_POST_PROC;
                comp_id[cid]    = "post-proc/pcm_gain";
		        comp_noutbuf[cid]  = 1;
		        comp_output[cid][0] = p_output[0];
		  break;
    
		  case XA_GAIN3:
        	    comp_format[cid].sample_rate = PCM_GAIN_SAMPLE_RATE;
        	    comp_format[cid].channels    = PCM_GAIN_NUM_CH;
        	    comp_format[cid].pcm_width   = PCM_GAIN_SAMPLE_WIDTH;
        	    comp_setup[cid] = pcm_gain_setup;
        	    comp_type[cid] = XAF_POST_PROC;
                comp_id[cid]    = "post-proc/pcm_gain";
		        comp_noutbuf[cid]  = 1;
		        comp_output[cid][0] = p_output[1];
		  break;
    
		  default:
                FIO_PRINTF(stderr, "Check component type in comp_create_order table\n");
		    return -1;
		  break;
		}//switch()
    }//for(;i;)

    cid = XA_GAIN2;
    TST_CHK_API_COMP_CREATE(p_adev, &p_comp[cid], comp_id[cid], comp_ninbuf[cid], comp_noutbuf[cid], &comp_inbuf[0], comp_type[cid], "xaf_comp_create");
    FIO_PRINTF(stderr, "CREATE done for GAIN2 cid:%d\n", cid);
    TST_CHK_API(comp_setup[cid](p_comp[cid], &comp_format[cid], 3, comp_framesize[cid], comp_probe[cid], comp_probe_mask[cid]), "comp_setup");
    FIO_PRINTF(stderr, "SETUP done for GAIN2 cid:%d\n", cid);

    cid = XA_GAIN3;
    TST_CHK_API_COMP_CREATE(p_adev, &p_comp[cid], comp_id[cid], comp_ninbuf[cid], comp_noutbuf[cid], &comp_inbuf[0], comp_type[cid], "xaf_comp_create");
    FIO_PRINTF(stderr, "CREATE done for GAIN2 cid:%d\n", cid);
    TST_CHK_API(comp_setup[cid](p_comp[cid], &comp_format[cid], 3, comp_framesize[cid], comp_probe[cid], comp_probe_mask[cid]), "comp_setup");
    FIO_PRINTF(stderr, "SETUP done for GAIN2 cid:%d\n", cid);

    for(k=0; k<NUM_INP_THREADS;k++)
    {
    	cid = comp_thread_order[k];
    	TST_CHK_API_COMP_CREATE(p_adev, &p_comp[cid], comp_id[cid], comp_ninbuf[cid], comp_noutbuf[cid], &source_comp_inbuf[k][0], comp_type[cid], "xaf_comp_create");
        TST_CHK_API(comp_setup[cid](p_comp[cid], &comp_format[cid], 3, comp_framesize[cid], comp_probe[cid], comp_probe_mask[cid]), "comp_setup");

        TST_CHK_API(xaf_comp_process(p_adev, p_comp[cid], NULL, 0, XAF_START_FLAG), "xaf_comp_process");
        
#if 0    
        for (i=0; i<2; i++)
        {
            TST_CHK_API(read_input(comp_inbuf[i], buf_length, &read_length, comp_input[cid][0], comp_type[cid]), "read_input");
        
            if (read_length)
                TST_CHK_API(xaf_comp_process(p_adev, p_comp[cid], comp_inbuf[i], read_length, XAF_INPUT_READY_FLAG), "xaf_comp_process");
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
                void *p_buf = (void *) dec_info[0];
                int size    = dec_info[1];
        
                TST_CHK_API(read_input(p_buf, size, &read_length, comp_input[cid][0], comp_type[cid]), "read_input");
        
                if (read_length)
                    TST_CHK_API(xaf_comp_process(p_adev, p_comp[cid], p_buf, read_length, XAF_INPUT_READY_FLAG), "xaf_comp_process");
                else
                {    
                    TST_CHK_API(xaf_comp_process(p_adev, p_comp[cid], NULL, 0, XAF_INPUT_OVER_FLAG), "xaf_comp_process");
                    break;
                }
            }
        }
#endif    
   
        /* ...non XAF_DECODER type components do not need initial input data to initialize */
        TST_CHK_API(xaf_comp_get_status(p_adev, p_comp[cid], &comp_status, &dec_info[0]), "xaf_comp_get_status");
        
        if (comp_status != XAF_INIT_DONE)
        {
            FIO_PRINTF(stderr, "Failed to init\n");
            exit(-1);
        }
    	else
        {
            FIO_PRINTF(stderr, "init done for cid=%d or comp_type=%d\n", cid, comp_type[cid]);
	    }
    }//for(;k;)

    cid = XA_RENDERER0;
    TST_CHK_API_COMP_CREATE(p_adev, &p_comp[cid], comp_id[cid], comp_ninbuf[cid], comp_noutbuf[cid], &comp_inbuf[0], comp_type[cid], "xaf_comp_create");
    FIO_PRINTF(stderr, "CREATE done for RENDERER0 cid:%d\n", cid);
    TST_CHK_API(comp_setup[cid](p_comp[cid], &comp_format[cid], 1, comp_framesize[cid]), "comp_setup");
    FIO_PRINTF(stderr, "SETUP done for RENDERER0 cid:%d\n", cid);

    TST_CHK_API(xaf_connect(p_comp[XA_GAIN0], 1, p_comp[XA_RENDERER0], 0, 2), "xaf_connect");
    FIO_PRINTF(stderr, "connect done for GAIN0->RENDERER0\n");

    TST_CHK_API(xaf_comp_process(p_adev, p_comp[XA_RENDERER0], NULL, 0, XAF_START_FLAG), "xaf_comp_process");
    TST_CHK_API(xaf_comp_get_status(p_adev, p_comp[XA_RENDERER0], &comp_status, &dec_info[0]), "xaf_comp_get_status");
    FIO_PRINTF(stderr, "START done for RENDERER0\n");

    cid = XA_AEC23_0;
    TST_CHK_API_COMP_CREATE(p_adev, &p_comp[cid], comp_id[cid], comp_ninbuf[cid], comp_noutbuf[cid], &comp_inbuf[0], comp_type[cid], "xaf_comp_create");
    FIO_PRINTF(stderr, "CREATE done for AEC23_0 cid:%d\n", cid);
    TST_CHK_API(comp_setup[cid](p_comp[cid], &comp_format[cid], 3, comp_framesize[cid], comp_probe[cid], comp_probe_mask[cid]), "comp_setup non-input");
    FIO_PRINTF(stderr, "SETUP done for AEC23_0 cid:%d\n", cid);

    /* ...connect GAIN1 to AEC23's Mic port */
    TST_CHK_API(xaf_connect(p_comp[XA_GAIN1], 1, p_comp[XA_AEC23_0], 0, 2), "xaf_connect");
    FIO_PRINTF(stderr, "connect done for GAIN1->AEC23_0 port-0\n");

    /* ...connect RENDERER0 to AEC23's reference/feedback port */
    TST_CHK_API(xaf_connect(p_comp[XA_RENDERER0], 1, p_comp[XA_AEC23_0], 1, 2), "xaf_connect");
    FIO_PRINTF(stderr, "connect done for RENDERER0->AEC23_0 port-1\n");

    TST_CHK_API(xaf_comp_process(p_adev, p_comp[XA_AEC23_0], NULL, 0, XAF_START_FLAG), "xaf_comp_process");
    FIO_PRINTF(stderr, "START done for AEC23_0\n");

    TST_CHK_API(xaf_comp_get_status(p_adev, p_comp[XA_AEC23_0], &comp_status, &dec_info[0]), "xaf_comp_get_status");
    if (comp_status != XAF_INIT_DONE)
    {
        FIO_PRINTF(stderr, "Failed to init Mixer");
        exit(-1);
    }

    TST_CHK_API(xaf_connect(p_comp[XA_AEC23_0], 2, p_comp[XA_GAIN2], 0, 4), "xaf_connect");
    FIO_PRINTF(stderr, "connect done for AEC23->GAIN2\n");

    TST_CHK_API(xaf_connect(p_comp[XA_AEC23_0], 3, p_comp[XA_GAIN3], 0, 4), "xaf_connect");
    FIO_PRINTF(stderr, "connect done for AEC23->GAIN3\n");

    TST_CHK_API(xaf_comp_process(p_adev, p_comp[XA_GAIN2], NULL, 0, XAF_START_FLAG), "xaf_comp_process");
    TST_CHK_API(xaf_comp_get_status(p_adev, p_comp[XA_GAIN2], &comp_status, &dec_info[0]), "xaf_comp_get_status");
    FIO_PRINTF(stderr, "START done for GAIN2\n");

    TST_CHK_API(xaf_comp_process(p_adev, p_comp[XA_GAIN3], NULL, 0, XAF_START_FLAG), "xaf_comp_process");
    TST_CHK_API(xaf_comp_get_status(p_adev, p_comp[XA_GAIN3], &comp_status, &dec_info[0]), "xaf_comp_get_status");
    FIO_PRINTF(stderr, "START done for GAIN3\n");

    /* ...start feeding input data to source components after the pipeline is setup */
    for(k=0; k<NUM_INP_THREADS;k++)
    {
    	cid = comp_thread_order[k];
    
        for (i=0; i<2; i++)
        {
            TST_CHK_API(read_input(source_comp_inbuf[k][i], buf_length, &read_length, comp_input[cid][0], comp_type[cid]), "read_input");
        
            if (read_length)
                TST_CHK_API(xaf_comp_process(p_adev, p_comp[cid], source_comp_inbuf[k][i], read_length, XAF_INPUT_READY_FLAG), "xaf_comp_process");
            else
            {    
                TST_CHK_API(xaf_comp_process(p_adev, p_comp[cid], NULL, 0, XAF_INPUT_OVER_FLAG), "xaf_comp_process");
                break;
            }
        }
    }
 
#ifdef XAF_PROFILE
    clk_start();
#endif

    for(k=0; k<(NUM_THREADS);k++)
    {
      cid = comp_thread_order[k];
      comp_thread_args[cid][0] = p_adev;
      comp_thread_args[cid][1] = p_comp[cid];
      comp_thread_args[cid][2] = comp_input[cid][0];
      comp_thread_args[cid][3] = comp_output[cid][0];
      comp_thread_args[cid][4] = &comp_type[cid];
      comp_thread_args[cid][5] = (void *)comp_id[cid];
      comp_thread_args[cid][6] = (void *)&comp_thread_order[k];
      FIO_PRINTF(stderr, "For thread %d comp_type=%d...p_input=%p p_output=%p\n", k, comp_type[cid], comp_input[cid][0], comp_output[cid][0]);
      __xf_thread_create(&comp_thread[cid], comp_process_entry, comp_thread_args[cid], "ref-path-sync Thread", comp_stack[cid], STACK_SIZE, XAF_APP_THREADS_PRIORITY);
      FIO_PRINTF(stderr, "Created thread %d comp_type=%d\n", k, comp_type[cid]);
    }

#ifdef RUNTIME_ACTIONS
    void *threads[NUM_COMP_IN_GRAPH];
    int comp_nbufs[NUM_COMP_IN_GRAPH];

    for(k=0; k < NUM_COMP_IN_GRAPH; k++)
    {
        int edge_comp_flag = 0;
        comp_nbufs[k] = comp_ninbuf[k] + comp_noutbuf[k];
        threads[k] = &comp_thread[k];

        cid = comp_create_order[k];
        for(i=0; i<NUM_THREADS; i++)
        {
            if(cid == comp_thread_order[i])
            { 
                edge_comp_flag = 1;
                break;
            }
        }
        if(edge_comp_flag) continue; //not to overwrite arguments of already created thread for edge components

        comp_thread_args[cid][0] = p_adev;
        comp_thread_args[cid][1] = p_comp[cid];
        comp_thread_args[cid][2] = comp_input[cid][0];
        comp_thread_args[cid][3] = comp_output[cid][0];
        comp_thread_args[cid][4] = &comp_type[cid];
        comp_thread_args[cid][5] = (void *)comp_id[cid];
        comp_thread_args[cid][6] = (void *)&comp_create_order[k];

        __xf_thread_init(&comp_thread[cid]);
    }

    {
    int ret = execute_runtime_actions(runtime_params, p_adev, &p_comp[0], comp_nbufs, &threads[0], NUM_COMP_IN_GRAPH, &comp_thread_args[0][0], NUM_THREAD_ARGS, &comp_stack[0][0]);
    if (ret) return ret;
    }
#endif  //RUNTIME_ACTIONS

    for(k=0; k<(NUM_THREADS);k++)
    {
        cid = comp_thread_order[k];
    	__xf_thread_join(&comp_thread[cid], NULL);
    	FIO_PRINTF(stdout,"component %d thread joined with exit code %x\n", cid, i);
    }

#ifdef XAF_PROFILE
   compute_total_frmwrk_cycles();
   clk_stop();
#endif

    {
        /* collect memory stats before closing the device */
        WORD32 meminfo[5];
        if(xaf_get_mem_stats(p_adev, &meminfo[0]))
        {
            FIO_PRINTF(stdout,"Init is incomplete, reliable memory stats are unavailable.\n");
        }
        else
        {
            FIO_PRINTF(stderr,"Local Memory used by DSP Components, in bytes            : %8d of %8d\n", meminfo[0], adev_config.audio_component_buffer_size);
            FIO_PRINTF(stderr,"Shared Memory used by Components and Framework, in bytes : %8d of %8d\n", meminfo[1], adev_config.audio_framework_buffer_size);
            FIO_PRINTF(stderr,"Local Memory used by Framework, in bytes                 : %8d\n", meminfo[2]);
        }
    }
    /* ...exec done, clean-up */
    for(k=0; k<(NUM_THREADS);k++)
    {
      cid = comp_thread_order[k];
      __xf_thread_destroy(&comp_thread[cid]);
    }

    for(k=0; k<(NUM_COMP_IN_GRAPH);k++)
      TST_CHK_API(xaf_comp_delete(p_comp[k]), "xaf_comp_delete");

    TST_CHK_API(xaf_adev_close(p_adev, XAF_ADEV_NORMAL_CLOSE), "xaf_adev_close");
    FIO_PRINTF(stdout,"Audio device closed\n\n");

    mem_exit();

    dsp_comps_cycles = pcm_gain_cycles + renderer_cycles;

    dsp_mcps = compute_comp_mcps(num_bytes_write, dsp_comps_cycles, comp_format[XA_AEC23_0], &strm_duration);

    TST_CHK_API(print_mem_mcps_info(mem_handle, NUM_COMP_IN_GRAPH), "print_mem_mcps_info");

    for (i=0; i<num_in_strms; i++)
    {
        if (p_input[i]) fio_fclose(p_input[i]);
    }

    for (i=0; i<num_out_strms; i++)
    {
        if (p_output[i]) fio_fclose(p_output[i]);
    }

    fio_quit();
    
    /* ...deinitialize tracing facility */
    TRACE_DEINIT();

    return 0;
}

