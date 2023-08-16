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

#include "audio/xa-pcm-gain-api.h"
#include "audio/xa-capturer-api.h"
#include "xaf-utils-test.h"
#include "xaf-fio-test.h"

#define PRINT_USAGE FIO_PRINTF(stdout, "\nUsage: %s  -outfile:out_filename.pcm -samples:<samples-per-channel to be captured(zero for endless capturing)> -rate:<optional>\n", argv[0]);

#define AUDIO_FRMWK_BUF_SIZE   (256 << 8)
#define AUDIO_COMP_BUF_SIZE    (1024 << 8)

#define NUM_COMP_IN_GRAPH       2

//component parameters
#define PCM_GAIN_SAMPLE_WIDTH   32
// supports only 16-bit PCM

#define PCM_GAIN_NUM_CH         2
// supports 1 and 2 channels only

#define PCM_GAIN_IDX_FOR_GAIN   1
//gain index range is 0 to 6 -> {0db, -6db, -12db, -18db, 6db, 12db, 18db}

#define PCM_GAIN_SAMPLE_RATE    48000
// micfil only support generate 32bit data */
#define CAPTURER_PCM_WIDTH       (32)
#define CAPTURER_SAMPLE_RATE     (48000)
#define CAPTURER_NUM_CH          (2)

unsigned int num_bytes_read, num_bytes_write;
extern int audio_frmwk_buf_size;
extern int audio_comp_buf_size;
double strm_duration;

#ifdef XAF_PROFILE
    extern int tot_cycles, frmwk_cycles, fread_cycles, fwrite_cycles;
    extern int dsp_comps_cycles, pcm_gain_cycles,capturer_cycles;
    extern double dsp_mcps;
#endif

/* Dummy unused functions */
XA_ERRORCODE xa_mp3_decoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_aac_decoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_mixer(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_mp3_encoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
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

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))

static int pcm_gain_setup(void *p_comp,xaf_format_t comp_format)
{
    int gain_idx = PCM_GAIN_IDX_FOR_GAIN;         //gain index range is 0 to 6 -> {0db, -6db, -12db, -18db, 6db, 12db, 18db}
    int frame_size = 32 * comp_format.channels * comp_format.sample_rate * comp_format.pcm_width / 8 / 1000;
    int param[][2] = {
        {
            XA_PCM_GAIN_CONFIG_PARAM_CHANNELS,
            comp_format.channels,
        }, {
            XA_PCM_GAIN_CONFIG_PARAM_SAMPLE_RATE,
            comp_format.sample_rate,
        }, {
            XA_PCM_GAIN_CONFIG_PARAM_PCM_WIDTH,
            comp_format.pcm_width,
        }, {
            XA_PCM_GAIN_CONFIG_PARAM_FRAME_SIZE,
            frame_size,
        }, {
            XA_PCM_GAIN_CONFIG_PARAM_GAIN_FACTOR,
            gain_idx,
        }, {
            XAF_COMP_CONFIG_PARAM_PRIORITY,
            0,
        },
    };

    fprintf(stderr, "%s: frame_size = %d\n", __func__, frame_size);

    return(xaf_comp_set_config(p_comp, ARRAY_SIZE(param), param[0]));
}


#define XA_CAPTURER_FRAME_SIZE    (1024)

static int capturer_setup(void *p_capturer,xaf_format_t capturer_format,UWORD64 sample_end)
{
    int param[][2] = {
        {
            XA_CAPTURER_CONFIG_PARAM_PCM_WIDTH,
            capturer_format.pcm_width,
        }, {
            XA_CAPTURER_CONFIG_PARAM_CHANNELS,
            capturer_format.channels,
        }, {
            XA_CAPTURER_CONFIG_PARAM_SAMPLE_RATE,
            capturer_format.sample_rate,
        }, {
            XA_CAPTURER_CONFIG_PARAM_SAMPLE_END,
            sample_end,
        }, {
            XAF_COMP_CONFIG_PARAM_PRIORITY,
            1,
        },
    };

    return(xaf_comp_set_config(p_capturer, ARRAY_SIZE(param), param[0]));
}

static int capturer_start_operation(void *p_capturer)
{
    int param[][2] = {
        {
            XA_CAPTURER_CONFIG_PARAM_STATE,
            XA_CAPTURER_STATE_START,
        },
    };

    return(xaf_comp_set_config(p_capturer, ARRAY_SIZE(param), param[0]));
}

static int get_comp_config(void *p_comp, xaf_format_t *comp_format)
{
    int param[6];
    int ret;


    TST_CHK_PTR(p_comp, "get_comp_config");
    TST_CHK_PTR(comp_format, "get_comp_config");

    param[0] = XA_PCM_GAIN_CONFIG_PARAM_CHANNELS;
    param[2] = XA_PCM_GAIN_CONFIG_PARAM_PCM_WIDTH;
    param[4] = XA_PCM_GAIN_CONFIG_PARAM_SAMPLE_RATE;

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
    void *p_output;
    xf_thread_t pcm_gain_thread;
    unsigned char pcm_gain_stack[STACK_SIZE];
    void * p_pcm_gain  = NULL;
    void * p_capturer  = NULL;
    xaf_comp_status pcm_gain_status;
    xaf_comp_status capturer_status;
    int pcm_gain_info[4];
    int capturer_info[4];
    char *filename_ptr;
    char *sample_cnt_ptr;
    char *sample_rate_ptr;
    char *channel_ptr;
    void *pcm_gain_thread_args[NUM_THREAD_ARGS];
    FILE *ofp = NULL;
    int i;
    xaf_format_t pcm_gain_format;
    xaf_format_t capturer_format;
    const char *ext;
    pUWORD8 ver_info[3] = {0,0,0};    //{ver,lib_rev,api_rev}
    unsigned short board_id = 0;
    int num_comp;
    int ret = 0;
    mem_obj_t* mem_handle;
    xaf_comp_type comp_type;
    long long int sammple_end_capturer = 0;
    unsigned int capturer_sample_rate = 0;
    unsigned int capturer_channel = 0;
    double pcm_gain_mcps = 0;
    xf_id_t comp_id;

#ifdef XAF_PROFILE
    frmwk_cycles = 0;
    fread_cycles = 0;
    fwrite_cycles = 0;
    dsp_comps_cycles = 0;
    pcm_gain_cycles = 0;
    capturer_cycles = 0;
    tot_cycles = 0;
    num_bytes_read = 0;
    num_bytes_write = 0;
#endif

    memset(&pcm_gain_format, 0, sizeof(xaf_format_t));
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
    TST_CHK_API(print_verinfo(ver_info,(pUWORD8)"\'Capturer + PCM Gain\'"), "print_verinfo");

    /* ...initialize tracing facility */
    TRACE_INIT("Xtensa Audio Framework - \'Capturer + PCM Gain\' Sample App");

    /* ...check input arguments */
    if ((argc != 3) && (argc != 4) && (argc != 5))
    {
        PRINT_USAGE;
        return 0;
    }

    if(NULL != strstr(argv[1], "-outfile:"))
    {
        filename_ptr = (char *)&(argv[1][9]);
        ext = strrchr(argv[1], '.');
        if(ext!=NULL)
        {
            ext++;
            if (strcmp(ext, "pcm"))
            {

                /*comp_id_pcm_gain    = "post-proc/pcm_gain";
                comp_setup = pcm_gain_setup;*/
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
    }
    else
    {
        PRINT_USAGE;
        return 0;
    }
    if(NULL != strstr(argv[2], "-samples:"))
    {
        sample_cnt_ptr = (char *)&(argv[2][9]);

        if ((*sample_cnt_ptr) == '\0' )
        {
            FIO_PRINTF(stderr, "samples-per-channel to be produced is not entererd\n");
            exit(-1);
        }
        sammple_end_capturer = atoi(sample_cnt_ptr);


    }
    else
    {
        PRINT_USAGE;
        return 0;
    }
    if((argc > 3) && NULL != strstr(argv[3], "-rate:"))
    {
        sample_rate_ptr = (char *)&(argv[3][6]);

        capturer_sample_rate = atoi(sample_rate_ptr);

        if (capturer_sample_rate < 8000 || capturer_sample_rate > 48000) {
            FIO_PRINTF(stderr, "capturer samples-rate is not support\n");
            exit(-1);
        }
    }
    else
        capturer_sample_rate = CAPTURER_SAMPLE_RATE;

    if((argc > 4) && (NULL != strstr(argv[4], "-channel:")))
    {
        channel_ptr = (char *)&(argv[4][9]);

        capturer_channel = atoi(channel_ptr);

        if (capturer_channel < 1 || capturer_channel > 2) {
            FIO_PRINTF(stderr, "capturer channel is not support\n");
            exit(-1);
        }
    }
    else
        capturer_channel = CAPTURER_NUM_CH;


    p_output = ofp;
    xaf_adev_config_t adev_config;
    TST_CHK_API(xaf_adev_config_default_init(&adev_config), "xaf_adev_config_default_init");

    mem_handle = mem_init(&adev_config);

    adev_config.pmem_malloc =  mem_malloc;
    adev_config.pmem_free =  mem_free;
    adev_config.audio_framework_buffer_size =  audio_frmwk_buf_size;
    adev_config.audio_component_buffer_size =  audio_comp_buf_size;
    TST_CHK_API(xaf_adev_open(&p_adev, &adev_config),  "xaf_adev_open");
    FIO_PRINTF(stdout,"Audio Device Ready\n");

    TST_CHK_API(xaf_adev_set_priorities(p_adev, 2, 3, 2), "xaf_adev_set_priorities");

    /* ...create capturer component */
    comp_type = XAF_CAPTURER;
    capturer_format.sample_rate = capturer_sample_rate;
    capturer_format.channels = capturer_channel;
    capturer_format.pcm_width = CAPTURER_PCM_WIDTH;
    TST_CHK_API_COMP_CREATE(p_adev, &p_capturer, "capturer", 0, 0, NULL, comp_type, "xaf_comp_create");
    TST_CHK_API(capturer_setup(p_capturer, capturer_format,sammple_end_capturer), "capturer_setup");

    pcm_gain_format.sample_rate = PCM_GAIN_SAMPLE_RATE;
    pcm_gain_format.channels = PCM_GAIN_NUM_CH;
    pcm_gain_format.pcm_width = PCM_GAIN_SAMPLE_WIDTH;

     /* ...create pcm gain component */
    TST_CHK_API_COMP_CREATE(p_adev, &p_pcm_gain, "post-proc/pcm_gain", 0, 1, NULL, XAF_POST_PROC, "xaf_comp_create");
    TST_CHK_API(pcm_gain_setup(p_pcm_gain,pcm_gain_format), "pcm_gain_setup");

    /* ...start capturer component */
    TST_CHK_API(xaf_comp_process(p_adev, p_capturer, NULL, 0, XAF_START_FLAG),"xaf_comp_process");

    TST_CHK_API(xaf_comp_get_status(p_adev, p_capturer, &capturer_status, &capturer_info[0]), "xaf_comp_get_status");

    if (capturer_status != XAF_INIT_DONE)
    {
        FIO_PRINTF(stderr, "Failed to init");
        exit(-1);
    }
    TST_CHK_API(xaf_connect(p_capturer, 0, p_pcm_gain, 0, 6), "xaf_connect");

    TST_CHK_API(capturer_start_operation(p_capturer), "capturer_start_operation");    

    TST_CHK_API(xaf_comp_process(p_adev, p_pcm_gain, NULL, 0, XAF_START_FLAG), "xaf_comp_process");

    TST_CHK_API(xaf_comp_get_status(p_adev, p_pcm_gain, &pcm_gain_status, &pcm_gain_info[0]), "xaf_comp_get_status");
    if (pcm_gain_status != XAF_INIT_DONE)
    {
        FIO_PRINTF(stdout,"Failed to init \n");
        exit(-1);
    }
#ifdef XAF_PROFILE
    clk_start();

#endif
    comp_id="post-proc/pcm_gain";
    comp_type = XAF_POST_PROC;
    pcm_gain_thread_args[0] = p_adev;
    pcm_gain_thread_args[1] = p_pcm_gain;
    pcm_gain_thread_args[2] = NULL;
    pcm_gain_thread_args[3] = p_output;
    pcm_gain_thread_args[4] = &comp_type;
    pcm_gain_thread_args[5] = (void *)comp_id;
    pcm_gain_thread_args[6] = (void *)&i;
    ret = __xf_thread_create(&pcm_gain_thread, comp_process_entry, &pcm_gain_thread_args[0], "Pcm gain Thread", pcm_gain_stack, STACK_SIZE, XAF_APP_THREADS_PRIORITY);
    if(ret != 0)
    {
        FIO_PRINTF(stdout,"Failed to create PCM gain thread  : %d\n", ret);
        exit(-1);
    }
    ret = __xf_thread_join(&pcm_gain_thread, NULL);

    if(ret != 0)
    {
        FIO_PRINTF(stdout,"Decoder thread exit Failed : %d \n", ret);
        exit(-1);
    }

    __xf_thread_destroy(&pcm_gain_thread);

#ifdef XAF_PROFILE
    compute_total_frmwrk_cycles();
    clk_stop();
#endif

    TST_CHK_API(capturer_get_config(p_capturer, &capturer_format), "capturer get config");
    TST_CHK_API(get_comp_config(p_pcm_gain, &pcm_gain_format), "capturer get config");

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
    TST_CHK_API(xaf_comp_delete(p_pcm_gain), "xaf_comp_delete");
    TST_CHK_API(xaf_comp_delete(p_capturer), "xaf_comp_delete");
    TST_CHK_API(xaf_adev_close(p_adev, XAF_ADEV_NORMAL_CLOSE), "xaf_adev_close");
    FIO_PRINTF(stdout,"Audio device closed\n\n");
    mem_exit(mem_handle);

#ifdef XAF_PROFILE
    dsp_comps_cycles = pcm_gain_cycles + capturer_cycles;
    dsp_mcps = compute_comp_mcps(capturer_format.output_produced, capturer_cycles, capturer_format, &strm_duration);

    pcm_gain_mcps = compute_comp_mcps(capturer_format.output_produced, pcm_gain_cycles, pcm_gain_format, &strm_duration);

    dsp_mcps += pcm_gain_mcps;
#endif

    TST_CHK_API(print_mem_mcps_info(mem_handle, num_comp), "print_mem_mcps_info");


    if (ofp) fio_fclose(ofp);

    fio_quit();

    /* ...deinitialize tracing facility */
    TRACE_DEINIT();

    return 0;
}


