/*****************************************************************
 * Copyright 2022 NXP
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
 *
 *****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "audio/xa-microspeech-frontend-api.h"
#include "audio/xa-microspeech-inference-api.h"
#include "xaf-utils-test.h"
#include "xaf-fio-test.h"

#define INPUT_PCM_WIDTH             (16)
#define INPUT_SAMPLE_RATE           (16000)
#define INPUT_CHANNEL_NUM           (1)
#define INPUT_FRAME_SIZE            (16 * 2 * 20) // 20 ms

extern const unsigned int g_yes_1000ms_audio_data_size;
extern const int16_t g_yes_1000ms_audio_data[];
extern const unsigned int g_no_1000ms_audio_data_size;
extern const int16_t g_no_1000ms_audio_data[];

unsigned int num_bytes_read, num_bytes_write;

static int16_t input_buffer[INPUT_SAMPLE_RATE * 2];

struct InputStream {
    bool isFile;
    FILE* handle;
    uint8_t* buffer;
    int size;
    int position;
};

static int read_stream(struct InputStream* p_input, uint8_t* p_buffer, int size)
{
    if (p_input->size < (p_input->position + 1) + size)
    {
        size = p_input->size - p_input->position;
    }

    if (p_input->isFile)
    {
        size = fread(p_buffer, sizeof(uint8_t), size, p_input->handle);
        p_input->position = ftell(p_input->handle);
    }
    else
    {
        memcpy(p_buffer, &p_input->buffer[p_input->position], size);
        p_input->position += size;
    }

    return size;
}

static int frontend_process(void *arg)
{
    void *p_adev, *p_frontend;
    xaf_comp_status comp_status;
    long comp_info[4];
    void * (*arg_arr)[10];
    xaf_comp_type comp_type;
    int error;
    struct InputStream* p_input;
    void* p_inbuf;

    arg_arr = arg;
    p_adev = (*arg_arr)[0];
    p_frontend = (*arg_arr)[1];
    p_input = (*arg_arr)[2];
    p_inbuf = (*arg_arr)[3];

    TST_CHK_API(xaf_comp_process(p_adev, p_frontend, NULL, 0, XAF_EXEC_FLAG), "xaf_comp_process");

    while (1)
    {
        TST_CHK_API(xaf_comp_get_status(p_adev, p_frontend, &comp_status, &comp_info[0]), "xaf_comp_get_status");

        if (comp_status == XAF_EXEC_DONE) break;

        if (comp_status == XAF_NEED_INPUT)
        {
            void *p_buf = (void *) comp_info[0]; 
            long size    = comp_info[1];

            if (!p_buf)
            {
                p_buf = p_inbuf;
                size = INPUT_FRAME_SIZE;
            }

            if ((size = read_stream(p_input, p_buf, INPUT_FRAME_SIZE)) > 0)
                TST_CHK_API(xaf_comp_process(p_adev, p_frontend, p_buf, size, XAF_INPUT_READY_FLAG), "xaf_comp_process");
            else
                TST_CHK_API(xaf_comp_process(p_adev, p_frontend, NULL, 0, XAF_INPUT_OVER_FLAG), "xaf_comp_process");
        }
    }

    return 0;
}

static int inference_process(void *arg)
{
    void *p_adev, *p_inference;
    void *p_input, *p_output;
    xaf_comp_status comp_status;
    long comp_info[4];
    void * (*arg_arr)[10];
    xaf_comp_type comp_type;
    int error;

    arg_arr = arg;
    p_adev = (*arg_arr)[0];
    p_inference = (*arg_arr)[1];

    TST_CHK_API(xaf_comp_process(p_adev, p_inference, NULL, 0, XAF_EXEC_FLAG), "xaf_comp_process");

    while (1)
    {
        TST_CHK_API(xaf_comp_get_status(p_adev, p_inference, &comp_status, &comp_info[0]), "xaf_comp_get_status");

        if (comp_status == XAF_EXEC_DONE) break;

        if (comp_status == XAF_OUTPUT_READY)
        {
            const char* labels[4] = {"SILENCE", "UNKNOWN", "YES", "NO"};
            void *p_buf = (void *)comp_info[0];
            long size = comp_info[1];

            int *pOut = (int*)p_buf;
            if (pOut[0])
                FIO_PRINTF(stdout, "Heard \"%s\" with %d%% score\n", labels[pOut[1]], (pOut[2] * 100) / 255);

            TST_CHK_API(xaf_comp_process(p_adev, p_inference, (void *)comp_info[0], comp_info[1], XAF_NEED_OUTPUT_FLAG), "xaf_comp_process");
        }
    }

    return 0;
}

static int run_microspeech()
{
    void *p_adev = NULL;
    void *p_inference = NULL;    
    void *p_frontend = NULL;    
    xaf_comp_config_t comp_config;
    xaf_comp_status comp_status;
    long comp_info[4];
    void *comp_inbuf[XAF_MAX_INBUFS];
    int i;    
    xaf_comp_type comp_type;

    pUWORD8 ver_info[3] = {0,0,0};    //{ver,lib_rev,api_rev}

    /* ...get xaf version info*/
    TST_CHK_API(xaf_get_verinfo(ver_info), "xaf_get_verinfo");

    /* ...show xaf version info*/
    TST_CHK_API(print_verinfo(ver_info, (pUWORD8)"\'TensorFlow Lite Micro keyword detection\'"), "print_verinfo");

    /* ...initialize tracing facility */
    TRACE_INIT("Xtensa Audio Framework - TensorFlow Lite Micro keyword detection");

    for (i = 0; i < 16000; i++)
    {
        input_buffer[i] = g_yes_1000ms_audio_data[i];
        input_buffer[i + INPUT_SAMPLE_RATE] = g_no_1000ms_audio_data[i];
    }

    struct InputStream input = {
        .isFile = false,
        .buffer = (uint8_t*)input_buffer,
        .size = (g_yes_1000ms_audio_data_size + g_no_1000ms_audio_data_size) * sizeof(int16_t),
        .position = 0,
    };

    xaf_adev_config_t adev_config;
    TST_CHK_API(xaf_adev_config_default_init(&adev_config), "xaf_adev_config_default_init");

    adev_config.pmem_malloc =  mem_malloc;
    adev_config.pmem_free =  mem_free;
    adev_config.audio_framework_buffer_size =  256 << 8;
    adev_config.audio_component_buffer_size =  1024 << 7;
    TST_CHK_API(xaf_adev_open(&p_adev, &adev_config),  "xaf_adev_open");

    FIO_PRINTF(stdout, "Audio Device Ready\n");

    /* ...create frontend component */
    comp_type = XAF_POST_PROC;
    TST_CHK_API(xaf_comp_config_default_init(&comp_config), "xaf_comp_config_default_init");
    comp_config.comp_id = "post-proc/microspeech_fe";
    comp_config.comp_type = comp_type;
    comp_config.num_input_buffers = 1;
    comp_config.num_output_buffers = 0;
    comp_config.pp_inbuf = (pVOID (*)[XAF_MAX_INBUFS])&comp_inbuf[0];
    TST_CHK_API(xaf_comp_create(p_adev, &p_frontend, &comp_config), "xaf_comp_create");

    {
        int32_t param[][2] = {
            {
                XA_MICROSPEECH_FE_CONFIG_PARAM_CHANNELS,
                INPUT_CHANNEL_NUM,
            }, {
                XA_MICROSPEECH_FE_CONFIG_PARAM_SAMPLE_RATE,
                INPUT_SAMPLE_RATE,
            }, {
                XA_MICROSPEECH_FE_CONFIG_PARAM_PCM_WIDTH,
                INPUT_PCM_WIDTH,
            },
        };

        xaf_comp_set_config(p_frontend, 3, (pWORD32)&param[0]);
    }

    /* ...create inference component */
    comp_type = XAF_POST_PROC;
    TST_CHK_API(xaf_comp_config_default_init(&comp_config), "xaf_comp_config_default_init");
    comp_config.comp_id = "post-proc/microspeech_inference";
    comp_config.comp_type = comp_type;
    comp_config.num_input_buffers = 0;
    comp_config.num_output_buffers = 1;
    TST_CHK_API(xaf_comp_create(p_adev, &p_inference, &comp_config), "xaf_comp_create");

    {
        int32_t param[][2] = {
            {
                XA_MICROSPEECH_INFERENCE_CONFIG_PARAM_CHANNELS,
                INPUT_CHANNEL_NUM,
            }, {
                XA_MICROSPEECH_INFERENCE_CONFIG_PARAM_SAMPLE_RATE,
                INPUT_SAMPLE_RATE,
            }, {
                XA_MICROSPEECH_INFERENCE_CONFIG_PARAM_PCM_WIDTH,
                INPUT_PCM_WIDTH,
            },
        };

        xaf_comp_set_config(p_inference, 3, (pWORD32)&param[0]);
    }

    /* ...start decoder component */            
    TST_CHK_API(xaf_comp_process(p_adev, p_frontend, NULL, 0, XAF_START_FLAG), "xaf_comp_process");

    TST_CHK_API(xaf_comp_get_status(p_adev, p_frontend, &comp_status, &comp_info[0]), "xaf_comp_get_status");

    if (comp_status != XAF_INIT_DONE)
    {
        FIO_PRINTF(stderr, "Failed to init");
        exit(-1);
    }

    FIO_PRINTF(stdout, "Frontend component initialized\n");

    TST_CHK_API(xaf_connect(p_frontend, 1, p_inference, 0, 4), "xaf_connect");

    FIO_PRINTF(stdout, "Components connected\n");

    TST_CHK_API(xaf_comp_process(p_adev, p_inference, NULL, 0, XAF_START_FLAG), "xaf_comp_process");

    TST_CHK_API(xaf_comp_get_status(p_adev, p_inference, &comp_status, &comp_info[0]), "xaf_comp_get_status");

    if (comp_status != XAF_INIT_DONE)
    {
        FIO_PRINTF(stderr, "Failed to init");
        exit(-1);
    }

    FIO_PRINTF(stdout, "Inference component initialized\n");

    xf_thread_t inference_thread;
    void* inference_thread_args[] = {
        p_adev,
        p_inference,
    };
    __xf_thread_create(&inference_thread, (xf_entry_t*)inference_process, inference_thread_args, "Inference Thread", NULL, STACK_SIZE, XAF_APP_THREADS_PRIORITY);

    xf_thread_t frontend_thread;
    void* frontend_thread_args[] = {
        p_adev,
        p_frontend,
        &input,
        comp_inbuf[0],
    };
    __xf_thread_create(&frontend_thread, (xf_entry_t*)frontend_process, frontend_thread_args, "Frontend Thread", NULL, STACK_SIZE, XAF_APP_THREADS_PRIORITY);

    __xf_thread_join(&frontend_thread, NULL);
    __xf_thread_join(&inference_thread, NULL);

    __xf_thread_destroy(&frontend_thread);
    __xf_thread_destroy(&inference_thread);

    TST_CHK_API(xaf_comp_delete(p_frontend), "xaf_comp_delete");
    TST_CHK_API(xaf_comp_delete(p_inference), "xaf_comp_delete");
    TST_CHK_API(xaf_adev_close(p_adev, XAF_ADEV_NORMAL_CLOSE), "xaf_adev_close");

    FIO_PRINTF(stdout, "Audio device closed\n\n");

    return 0;
}

int main_task(int argc, char *argv[])
{
    unsigned short board_id = 0;

    // NOTE: set_wbna() should be called before any other dynamic
    // adjustment of the region attributes for cache.
    set_wbna(&argc, argv);

    /* ...start xos */
    board_id = start_rtos();

    if (run_microspeech() == XAF_INVALIDVAL_ERR)
    {
        FIO_PRINTF(stderr, "Error: Failed to initialize TensorFlow Lite Micro Xtensa Audio Framework components! "
            "Please, make sure to backup hifi4.bin and replace it by hifi4_tflm_*.bin in the /lib/firmware/imx/dsp directory.\n");
    }

    return 0;
}
