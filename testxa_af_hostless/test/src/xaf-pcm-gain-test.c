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
#include "xaf-utils-test.h"
#include "xaf-fio-test.h"

#define PRINT_USAGE FIO_PRINTF(stdout, "\nUsage: %s -infile:in_filename.pcm -outfile:out_filename.pcm [optional -pcm_width:16 (default) or 8 or 24 or 32 ]\n For pcm_width:24, input and output samples are in the form of MSB aligned 32 bits\n\n", argv[0]);

#define AUDIO_FRMWK_BUF_SIZE   (256 << 8)
#define AUDIO_COMP_BUF_SIZE    (1024 << 7)
#define NUM_COMP_IN_GRAPH       1

//component parameters
#define PCM_GAIN_SAMPLE_WIDTH   16
// supports 8,16,24,32-bit PCM

#define PCM_GAIN_NUM_CH         1
// supports upto 16 channels

#define PCM_GAIN_IDX_FOR_GAIN   1
//gain index range is 0 to 6 -> {0db, -6db, -12db, -18db, 6db, 12db, 18db}

#define PCM_GAIN_SAMPLE_RATE    44100

//following parameter is provided to simulate desired MHz load in PCM-GAIN for experiments
#define PCM_GAIN_BURN_CYCLES    0

unsigned int num_bytes_read, num_bytes_write;
extern int audio_frmwk_buf_size;
extern int audio_comp_buf_size;
double strm_duration;

int g_pcm_width=0;

#ifdef XAF_PROFILE
    extern long long tot_cycles, frmwk_cycles, fread_cycles, fwrite_cycles;
    extern long long dsp_comps_cycles, pcm_gain_cycles;
    extern double dsp_mcps;
#endif

/* Dummy unused functions */
XA_ERRORCODE xa_mp3_decoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_aac_decoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_mixer(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_mp3_encoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_src_pp_fx(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_renderer(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_capturer(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
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

static int pcm_gain_setup(void *p_comp)
{
    int param[12];
    int pcm_width;
    int num_ch = PCM_GAIN_NUM_CH;                 // supports upto 16 channels
    int sample_rate = PCM_GAIN_SAMPLE_RATE;
    int gain_idx = PCM_GAIN_IDX_FOR_GAIN;         //gain index range is 0 to 6 -> {0db, -6db, -12db, -18db, 6db, 12db, 18db}
    int frame_size = XAF_INBUF_SIZE;

    if(g_pcm_width == 0)
        pcm_width = PCM_GAIN_SAMPLE_WIDTH;        // supports 8,16,24,32-bit PCM
    else
        pcm_width = g_pcm_width;
    
    param[0] = XA_PCM_GAIN_CONFIG_PARAM_CHANNELS;
    param[1] = num_ch;
    param[2] = XA_PCM_GAIN_CONFIG_PARAM_SAMPLE_RATE;
    param[3] = sample_rate;
    param[4] = XA_PCM_GAIN_CONFIG_PARAM_PCM_WIDTH;
    param[5] = pcm_width;
    param[6] = XA_PCM_GAIN_CONFIG_PARAM_FRAME_SIZE;
    param[7] = frame_size;
    param[8] = XA_PCM_GAIN_CONFIG_PARAM_GAIN_FACTOR;
    param[9] = gain_idx;
    param[10]= XA_PCM_GAIN_BURN_ADDITIONAL_CYCLES;
    param[11]= PCM_GAIN_BURN_CYCLES; 

    return(xaf_comp_set_config(p_comp, 6, &param[0]));
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

void fio_quit()
{
    return;
}

int main_task(int argc, char **argv)
{

    void *p_adev = NULL;
    void *p_input, *p_output;
    xf_thread_t pcm_gain_thread;
    unsigned char pcm_gain_stack[STACK_SIZE];
    void * p_pcm_gain  = NULL;
    xaf_comp_status pcm_gain_status;
    int pcm_gain_info[4];
    char *filename_ptr;
    void *pcm_gain_thread_args[NUM_THREAD_ARGS];
    FILE *fp, *ofp;
    void *pcm_gain_inbuf[2];
    int buf_length = XAF_INBUF_SIZE;
    int read_length;
    int i;
    xaf_format_t pcm_gain_format;
    xf_id_t comp_id;
    int (*comp_setup)(void * p_comp);
    const char *ext;
    pUWORD8 ver_info[3] = {0,0,0};    //{ver,lib_rev,api_rev}
    unsigned short board_id = 0;
    int num_comp;
    mem_obj_t* mem_handle;
    xaf_comp_type comp_type;

#ifdef XAF_PROFILE
    frmwk_cycles = 0;
    fread_cycles = 0;
    fwrite_cycles = 0;
    dsp_comps_cycles = 0;
    pcm_gain_cycles = 0;
    tot_cycles = 0;
    num_bytes_read = 0;
    num_bytes_write = 0;
#endif


    memset(&pcm_gain_format, 0, sizeof(xaf_format_t));
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
    TST_CHK_API(print_verinfo(ver_info,(pUWORD8)"\'PCM Gain\'"), "print_verinfo");

    /* ...initialize tracing facility */
    TRACE_INIT("Xtensa Audio Framework - \'PCM Gain\' Sample App");

    /* ...check input arguments */
    if (argc < 3 || argc > 4)
    {
        PRINT_USAGE;
        return 0;
    }

    if(NULL != strstr(argv[1], "-infile:"))
    {
        filename_ptr = (char *)&(argv[1][8]);
        ext = strrchr(argv[1], '.');
        if((ext!=NULL))
        {
            ext++;
            if (!strcmp(ext, "pcm"))
            {
                comp_id    = "post-proc/pcm_gain";
                comp_setup = pcm_gain_setup;
            }
            else
            {
                FIO_PRINTF(stderr, "Unknown input file format '%s'\n", ext);
                exit(-1);
            }
        }
        else
        {
            FIO_PRINTF(stderr, "Failed to open infile\n");
            exit(-1);
        }

        /* ...open file */
        if ((fp = fio_fopen(filename_ptr, "rb")) == NULL)
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

    if(NULL != strstr(argv[2], "-outfile:"))
    {
        filename_ptr = (char *)&(argv[2][9]);

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
    if(argc == 4)
    {
        if(NULL != strstr(argv[3], "-pcm_width:" ))
        {
            char *pcm_width_ptr = (char *)&(argv[3][11]);
        
            if((*pcm_width_ptr) == '\0')
            {
                FIO_PRINTF(stderr, "Pcm width is not provide\n");
                exit(-1);
            }
            g_pcm_width = atoi(pcm_width_ptr);
        }
        else
        {
            PRINT_USAGE;
            return 0;
        }
    }
    p_input  = fp;
    p_output = ofp;

    mem_handle = mem_init();

    xaf_adev_config_t adev_config;
    TST_CHK_API(xaf_adev_config_default_init(&adev_config), "xaf_adev_config_default_init");

    adev_config.pmem_malloc =  mem_malloc;
    adev_config.pmem_free =  mem_free;
    adev_config.audio_framework_buffer_size =  audio_frmwk_buf_size;
    adev_config.audio_component_buffer_size =  audio_comp_buf_size;
    TST_CHK_API(xaf_adev_open(&p_adev, &adev_config),  "xaf_adev_open");

    FIO_PRINTF(stdout,"Audio Device Ready\n");

    /* ...create pcm gain component */
    comp_type = XAF_POST_PROC;
    TST_CHK_API_COMP_CREATE(p_adev, &p_pcm_gain, comp_id, 2, 1, &pcm_gain_inbuf[0], comp_type, "xaf_comp_create");
    TST_CHK_API(comp_setup(p_pcm_gain), "comp_setup");

    /* ...start pcm gain component */
    TST_CHK_API(xaf_comp_process(p_adev, p_pcm_gain, NULL, 0, XAF_START_FLAG),"xaf_comp_process");

    /* ...feed input to pcm gain component */
    for (i=0; i<2; i++)
    {
        TST_CHK_API(read_input(pcm_gain_inbuf[i], buf_length, &read_length, p_input, comp_type), "read_input");

        if (read_length)
            TST_CHK_API(xaf_comp_process(p_adev, p_pcm_gain, pcm_gain_inbuf[i], read_length, XAF_INPUT_READY_FLAG), "xaf_comp_process");
        else
        {    
            TST_CHK_API(xaf_comp_process(p_adev, p_pcm_gain, NULL, 0, XAF_INPUT_OVER_FLAG), "xaf_comp_process");
            break;
        }
    }

    /* ...initialization loop */
    while (1)
    {
        TST_CHK_API(xaf_comp_get_status(p_adev, p_pcm_gain, &pcm_gain_status, &pcm_gain_info[0]), "xaf_comp_get_status");

        if (pcm_gain_status == XAF_INIT_DONE || pcm_gain_status == XAF_EXEC_DONE) break;

        if (pcm_gain_status == XAF_NEED_INPUT)
        {
            void *p_buf = (void *) pcm_gain_info[0];
            int size    = pcm_gain_info[1];

            TST_CHK_API(read_input(p_buf, size, &read_length, p_input,comp_type), "read_input");

            if (read_length)
                TST_CHK_API(xaf_comp_process(p_adev, p_pcm_gain, p_buf, read_length, XAF_INPUT_READY_FLAG), "xaf_comp_process");
            else
            {    
                TST_CHK_API(xaf_comp_process(p_adev, p_pcm_gain, NULL, 0, XAF_INPUT_OVER_FLAG), "xaf_comp_process");
                break;
            }
        }
    }

    if (pcm_gain_status != XAF_INIT_DONE)
    {
        FIO_PRINTF(stderr, "Failed to init");
        exit(-1);
    }

    TST_CHK_API(get_comp_config(p_pcm_gain, &pcm_gain_format), "get_comp_config");

#ifdef XAF_PROFILE
    clk_start();

#endif

    pcm_gain_thread_args[0] = p_adev;
    pcm_gain_thread_args[1] = p_pcm_gain;
    pcm_gain_thread_args[2] = p_input;
    pcm_gain_thread_args[3] = p_output;
    pcm_gain_thread_args[4] = &comp_type;
    pcm_gain_thread_args[5] = (void *)comp_id;
    pcm_gain_thread_args[6] = (void *)&i;   //dummy

    /* ...init done, begin execution thread */
    __xf_thread_create(&pcm_gain_thread, comp_process_entry, &pcm_gain_thread_args[0], "Pcm Gain Thread", pcm_gain_stack, STACK_SIZE, XAF_APP_THREADS_PRIORITY);

    __xf_thread_join(&pcm_gain_thread, NULL);

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
    __xf_thread_destroy(&pcm_gain_thread);
    TST_CHK_API(xaf_comp_delete(p_pcm_gain), "xaf_comp_delete");
    TST_CHK_API(xaf_adev_close(p_adev, XAF_ADEV_NORMAL_CLOSE), "xaf_adev_close");
    FIO_PRINTF(stdout,"Audio device closed\n\n");

    mem_exit();

    dsp_comps_cycles = pcm_gain_cycles;

    /* ...For pcm_width=24, pcm samples are in the format MSB aligned 32 bits. Therefore MCPS has to be calculated considering pcm_width=32.*/
    if(pcm_gain_format.pcm_width == 24)
        pcm_gain_format.pcm_width = 32;

    dsp_mcps = compute_comp_mcps(num_bytes_write, pcm_gain_cycles, pcm_gain_format, &strm_duration);

    TST_CHK_API(print_mem_mcps_info(mem_handle, num_comp), "print_mem_mcps_info");

    if (fp)  fio_fclose(fp);
    if (ofp) fio_fclose(ofp);

    fio_quit();
    
    /* ...deinitialize tracing facility */
    TRACE_DEINIT();

    return 0;
}

