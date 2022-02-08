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

#include "audio/xa_mp3_enc_api.h"
#include "audio/xa-capturer-api.h"
#include "xaf-utils-test.h"
#include "xaf-fio-test.h"

#define PRINT_USAGE FIO_PRINTF(stdout, "\nUsage: %s  -outfile:out_filename.mp3  -samples:<samples to be captured(zero for endless capturing)> -frame_size:<optional:bytes per channel 128 to 2048>\n", argv[0]);\
    FIO_PRINTF(stdout, "\nNote: Capturer-plugin expects input file named 'capturer_in.pcm' to be present in the execution directory.\n\n");

#define AUDIO_FRMWK_BUF_SIZE   (256 << 8)

#define AUDIO_COMP_BUF_SIZE    (1024 << 7) + (1024 << 5)  + (1024 << 3)

#define NUM_COMP_IN_GRAPH       (2)

//encoder parameters
#define MP3_ENC_PCM_WIDTH       (16)
#define MP3_ENC_SAMPLE_RATE     (44100)
#define MP3_ENC_NUM_CH          (2)
#define CAPTURER_PCM_WIDTH       (16)
#define CAPTURER_SAMPLE_RATE     (44100)
#define CAPTURER_NUM_CH          (2)

unsigned int num_bytes_read, num_bytes_write;
extern int audio_frmwk_buf_size;
extern int audio_comp_buf_size;
double strm_duration;

#ifdef XAF_PROFILE
    extern long long tot_cycles, frmwk_cycles, fread_cycles, fwrite_cycles;
    extern long long dsp_comps_cycles, enc_cycles,capturer_cycles;
    extern double dsp_mcps;
#endif

/* Dummy unused functions */
XA_ERRORCODE xa_mp3_decoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_aac_decoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_mixer(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_pcm_gain(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_src_pp_fx(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_renderer(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_amr_wb_decoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_hotword_decoder(xa_codec_handle_t p_xa_module_obj, WORD32 i_cmd, WORD32 i_idx, pVOID pv_value){return 0;}
XA_ERRORCODE xa_vorbis_decoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_dummy_aec22(xa_codec_handle_t p_xa_module_obj, WORD32 i_cmd, WORD32 i_idx, pVOID pv_value) {return 0;}
XA_ERRORCODE xa_dummy_aec23(xa_codec_handle_t p_xa_module_obj, WORD32 i_cmd, WORD32 i_idx, pVOID pv_value) {return 0;}
XA_ERRORCODE xa_pcm_split(xa_codec_handle_t p_xa_module_obj, WORD32 i_cmd, WORD32 i_idx, pVOID pv_value) {return 0;}
XA_ERRORCODE xa_mimo_mix(xa_codec_handle_t p_xa_module_obj, WORD32 i_cmd, WORD32 i_idx, pVOID pv_value) {return 0;}
XA_ERRORCODE xa_dummy_wwd(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_dummy_hbuf(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_opus_encoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_dummy_wwd_msg(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_dummy_hbuf_msg(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}

/* ...Frame size per channel in bytes (8ms frame at 48 kHz with 2 bytes per sample) */
#define CAPTURER_FRAME_SIZE (48 * 8 * 2)
static int capturer_setup(void *p_capturer,xaf_format_t capturer_format,UWORD64 sample_end, WORD32 frame_size)
{
    int param[10];

    param[0] = XA_CAPTURER_CONFIG_PARAM_PCM_WIDTH;
    param[1] = capturer_format.pcm_width;
    param[2] = XA_CAPTURER_CONFIG_PARAM_CHANNELS;
    param[3] = capturer_format.channels;
    param[4] = XA_CAPTURER_CONFIG_PARAM_SAMPLE_RATE;
    param[5] = capturer_format.sample_rate;
    param[6] = XA_CAPTURER_CONFIG_PARAM_SAMPLE_END;
    param[7] = sample_end;
    param[8] = XA_CAPTURER_CONFIG_PARAM_FRAME_SIZE;
    param[9] = frame_size;

    fprintf(stderr, "capturer frame_size:%d\n", frame_size);

    return(xaf_comp_set_config(p_capturer, 5, &param[0]));
}
static int capturer_start_operation(void *p_capturer)
{
    int param[2];

    param[0] = XA_CAPTURER_CONFIG_PARAM_STATE;
    param[1] = XA_CAPTURER_STATE_START;

    return(xaf_comp_set_config(p_capturer, 1, &param[0]));
}

static int mp3_enc_setup(void *p_encoder,xaf_format_t encoder_format)
{
    int param[6];

    param[0] = XA_MP3ENC_CONFIG_PARAM_PCM_WDSZ;
    param[1] = encoder_format.pcm_width;
    param[2] = XA_MP3ENC_CONFIG_PARAM_SAMP_FREQ;
    param[3] = encoder_format.sample_rate;
    param[4] = XA_MP3ENC_CONFIG_PARAM_NUM_CHANNELS;
    param[5] = encoder_format.channels;

    return(xaf_comp_set_config(p_encoder, 3, &param[0]));
}


static int get_mp3_enc_get_config(void *p_comp, xaf_format_t *comp_format)
{
    int param[6];
    int ret;


    TST_CHK_PTR(p_comp, "get_mp3_enc_config");
    TST_CHK_PTR(comp_format, "get_mp3_enc_config");
    param[0] = XA_MP3ENC_CONFIG_PARAM_NUM_CHANNELS;
    param[2] = XA_MP3ENC_CONFIG_PARAM_PCM_WDSZ;
    param[4] = XA_MP3ENC_CONFIG_PARAM_SAMP_FREQ;
    ret = xaf_comp_get_config(p_comp, 3, &param[0]);
    if(ret < 0)
        return ret;

    comp_format->channels = param[1];
    comp_format->pcm_width = param[3];
    comp_format->sample_rate = param[5];

    return 0;
}

static int capturer_get_config(void *p_comp, xaf_format_t *comp_format)
{
    int param[8];
    int ret;


    TST_CHK_PTR(p_comp, "get_mp3_enc_config");
    TST_CHK_PTR(comp_format, "get_mp3_enc_config");
    param[0] = XA_CAPTURER_CONFIG_PARAM_CHANNELS;
    param[2] = XA_CAPTURER_CONFIG_PARAM_PCM_WIDTH;
    param[4] = XA_CAPTURER_CONFIG_PARAM_SAMPLE_RATE;
    param[6] = XA_CAPTURER_CONFIG_PARAM_BYTES_PRODUCED;
    ret = xaf_comp_get_config(p_comp, 4, &param[0]);
    if(ret < 0)
        return ret;
    comp_format->channels = param[1];
    comp_format->pcm_width = param[3];
    comp_format->sample_rate = param[5];
    comp_format->output_produced = (UWORD64) param[7];
    return 0;
}

void fio_quit()
{
    return;
}

int main_task(int argc, char **argv)
{

    void *p_adev = NULL;
    void *p_output = NULL;
    xf_thread_t mp3_encoder_thread;
    unsigned char mp3_encoder_stack[STACK_SIZE];
    void * p_mp3_encoder  = NULL;
    void * p_capturer  = NULL;
    xaf_comp_status mp3_encoder_status;
    xaf_comp_status capturer_status;
    int mp3_encoder_info[4];
    int capturer_info[4];
    char *filename_ptr;
    char *sample_cnt_ptr;
    void *mp3_encoder_thread_args[NUM_THREAD_ARGS];
    FILE *ofp = NULL;
    xaf_format_t encoder_format;
    xaf_format_t capturer_format;
    xf_id_t comp_id;
    const char *ext;
    pUWORD8 ver_info[3] = {0,0,0};    //{ver,lib_rev,api_rev}
    unsigned short board_id = 0;
    int num_comp;
    int ret = 0;
    int i = 0;
    mem_obj_t* mem_handle;
    xaf_comp_type comp_type;
    long long int sample_end_capturer = 0;
    double enc_mcps = 0;
    int capturer_frame_size = CAPTURER_FRAME_SIZE;

#ifdef XAF_PROFILE
    frmwk_cycles = 0;
    fread_cycles = 0;
    fwrite_cycles = 0;
    dsp_comps_cycles = 0;
    enc_cycles = 0;
    capturer_cycles = 0;
    tot_cycles = 0;
    num_bytes_read = 0;
    num_bytes_write = 0;
#endif

    memset(&encoder_format, 0, sizeof(xaf_format_t));
    memset(&capturer_format, 0, sizeof(xaf_format_t));

    audio_frmwk_buf_size = AUDIO_FRMWK_BUF_SIZE;
    audio_comp_buf_size = AUDIO_COMP_BUF_SIZE;
    num_comp = NUM_COMP_IN_GRAPH;

    // NOTE: set_wbna() should be called before any other dynamic
    // adjustment of the region attributes for cache.
    set_wbna(&argc, argv);

    /* ...start xos */
    board_id = start_rtos();

    /* ...get xaf version info*/
    TST_CHK_API(xaf_get_verinfo(ver_info), "xaf_get_verinfo");

    /* ...show xaf version info*/
    TST_CHK_API(print_verinfo(ver_info,(pUWORD8)"\'Capturer + Mp3 Encoder\'"), "print_verinfo");

    /* ...initialize tracing facility */
    TRACE_INIT("Xtensa Audio Framework - \'Capturer + Mp3 Encoder\' Sample App");

    /* ...check input arguments */
    for (i=0; i<(argc-1); i++)
    {
      if(NULL != strstr(argv[i+1], "-outfile:"))
      {
          filename_ptr = (char *)&(argv[i+1][9]);
          ext = strrchr(argv[i+1], '.');
	  	  if(ext!=NULL)
          {
	  		ext++;
	  		if (strcmp(ext, "mp3"))
	  		{
	  			FIO_PRINTF(stderr, "Unknown input file format '%s'\n", ext);
	  			exit(-1);
	  		}
	  	}
	  	else
          {
              FIO_PRINTF(stderr, "Failed to open outfile\n");
              exit(-1);
          }
      
          /* ...open file */
          if ((ofp = fio_fopen(filename_ptr, "wb")) == NULL)
          {
             FIO_PRINTF(stderr, "Failed to open '%s': %d\n", filename_ptr, errno);
             exit(-1);
          }
          p_output = ofp;
      }
      else if(NULL != strstr(argv[i+1], "-samples:"))
      {
          sample_cnt_ptr = (char *)&(argv[i+1][9]);
      
          if ((*sample_cnt_ptr) == '\0' )
          {
              FIO_PRINTF(stderr, "Samples to be produced is not entererd\n");
              exit(-1);
          }
          sample_end_capturer = atoi(sample_cnt_ptr);
      }
      else if(NULL != strstr(argv[i+1], "-frame_size:"))
      {
          char *frame_size_ptr = (char *)&(argv[i+1][12]);
          
          if ((*frame_size_ptr) == '\0' )
          {
              FIO_PRINTF(stderr, "Framesize is not provide\n");
              exit(-1);
          }
          capturer_frame_size = atoi(frame_size_ptr);
      }
      else
      {
          PRINT_USAGE;
          return 0;
      }
    }//for(;i<(argc-1);)

    if((argc < 2) || (p_output == NULL))
    {
        PRINT_USAGE;
        return 0;
    }

    mem_handle = mem_init();
    xaf_adev_config_t adev_config;
    TST_CHK_API(xaf_adev_config_default_init(&adev_config), "xaf_adev_config_default_init");

    adev_config.pmem_malloc =  mem_malloc;
    adev_config.pmem_free =  mem_free;
    adev_config.audio_framework_buffer_size =  audio_frmwk_buf_size;
    adev_config.audio_component_buffer_size =  audio_comp_buf_size;
    TST_CHK_API(xaf_adev_open(&p_adev, &adev_config),  "xaf_adev_open");
    FIO_PRINTF(stdout,"Audio Device Ready\n");

    /* ...create capturer component */
    comp_type = XAF_CAPTURER;
    capturer_format.sample_rate = CAPTURER_SAMPLE_RATE;
    capturer_format.channels = CAPTURER_NUM_CH;
    capturer_format.pcm_width = CAPTURER_PCM_WIDTH;
    TST_CHK_API_COMP_CREATE(p_adev, &p_capturer, "capturer", 0, 0, NULL, comp_type, "xaf_comp_create");
    TST_CHK_API(capturer_setup(p_capturer, capturer_format,sample_end_capturer, capturer_frame_size), "capturer_setup");
    /* ...start capturer component */
    TST_CHK_API(xaf_comp_process(p_adev, p_capturer, NULL, 0, XAF_START_FLAG),"xaf_comp_process");
    TST_CHK_API(xaf_comp_get_status(p_adev, p_capturer, &capturer_status, &capturer_info[0]), "xaf_comp_get_status");
    if (capturer_status != XAF_INIT_DONE)
    {
        FIO_PRINTF(stderr, "Failed to init");
        exit(-1);
    }

     /* ...create mp3 encoder component */
    encoder_format.sample_rate = MP3_ENC_SAMPLE_RATE;
    encoder_format.channels = MP3_ENC_NUM_CH;
    encoder_format.pcm_width = MP3_ENC_PCM_WIDTH;
    TST_CHK_API_COMP_CREATE(p_adev, &p_mp3_encoder, "audio-encoder/mp3", 0, 1, NULL, XAF_ENCODER, "xaf_comp_create");
    TST_CHK_API(mp3_enc_setup(p_mp3_encoder,encoder_format), "mp3_encoder_setup");
    TST_CHK_API(xaf_connect(p_capturer, 0, p_mp3_encoder, 0, 4), "xaf_connect");
    TST_CHK_API(capturer_start_operation(p_capturer), "capturer_start_operation");    

    TST_CHK_API(xaf_comp_process(p_adev, p_mp3_encoder, NULL, 0, XAF_START_FLAG), "xaf_comp_process");
    TST_CHK_API(xaf_comp_get_status(p_adev, p_mp3_encoder, &mp3_encoder_status, &mp3_encoder_info[0]), "xaf_comp_get_status");
    if (mp3_encoder_status != XAF_INIT_DONE)
    {
        FIO_PRINTF(stdout,"Failed to init \n");
        exit(-1);
    }
#ifdef XAF_PROFILE
    clk_start();

#endif
    comp_id="audio-encoder/mp3";
    comp_type = XAF_ENCODER;
    mp3_encoder_thread_args[0] = p_adev;
    mp3_encoder_thread_args[1] = p_mp3_encoder;
    mp3_encoder_thread_args[2] = NULL;
    mp3_encoder_thread_args[3] = p_output;
    mp3_encoder_thread_args[4] = &comp_type;
    mp3_encoder_thread_args[5] = (void *)comp_id;
    mp3_encoder_thread_args[6] = (void *)&i; //dummy
    ret = __xf_thread_create(&mp3_encoder_thread, comp_process_entry, &mp3_encoder_thread_args[0], "Mp3 encoder Thread", mp3_encoder_stack, STACK_SIZE, XAF_APP_THREADS_PRIORITY);
    if(ret != XOS_OK)
    {
        FIO_PRINTF(stdout,"Failed to create mp3 encoder thread  : %d\n", ret);
        exit(-1);
    }
    ret = __xf_thread_join(&mp3_encoder_thread, NULL);
    if(ret != XOS_OK)
    {
        FIO_PRINTF(stdout,"Decoder thread exit Failed : %d \n", ret);
        exit(-1);
    }


#ifdef XAF_PROFILE
   compute_total_frmwrk_cycles();
   clk_stop();
    //tot_cycles = clk_diff(frmwk_stop, frmwk_start);
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
    __xf_thread_destroy(&mp3_encoder_thread);
    TST_CHK_API(capturer_get_config(p_capturer, &capturer_format), "capturer get config");
    TST_CHK_API(get_mp3_enc_get_config(p_mp3_encoder, &encoder_format), "capturer get config");
    TST_CHK_API(xaf_comp_delete(p_mp3_encoder), "xaf_comp_delete");
    TST_CHK_API(xaf_comp_delete(p_capturer), "xaf_comp_delete");
    TST_CHK_API(xaf_adev_close(p_adev, XAF_ADEV_NORMAL_CLOSE), "xaf_adev_close");
    FIO_PRINTF(stdout,"Audio device closed\n\n");
    mem_exit();

    dsp_comps_cycles = enc_cycles + capturer_cycles;
    enc_mcps = compute_comp_mcps(capturer_format.output_produced, enc_cycles, encoder_format, &strm_duration);
    dsp_mcps = compute_comp_mcps(capturer_format.output_produced, capturer_cycles, capturer_format, &strm_duration);
    dsp_mcps += enc_mcps;

    TST_CHK_API(print_mem_mcps_info(mem_handle, num_comp), "print_mem_mcps_info");


    if (ofp) fio_fclose(ofp);

    fio_quit();

    /* ...deinitialize tracing facility */
    TRACE_DEINIT();

    return 0;
}

