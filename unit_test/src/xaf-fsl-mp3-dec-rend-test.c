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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "audio/xa-renderer-api.h"
#include "xaf-utils-test.h"
#include "xaf-fio-test.h"
#include "fsl_unia.h"
#include "dsp_codec_interface.h"
#include "library_load.h"

#define PRINT_USAGE FIO_PRINTF(stdout, "\nUsage: %s -infile:filename.mp3 \n\n", argv[0]);
#define AUDIO_FRMWK_BUF_SIZE   (256 << 8)
#define AUDIO_COMP_BUF_SIZE    (1024 << 7)

#define NUM_COMP_IN_GRAPH       2

//component parameters
#define MP3_DEC_PCM_WIDTH       16

unsigned int num_bytes_read, num_bytes_write;
extern int audio_frmwk_buf_size;
extern int audio_comp_buf_size;
double strm_duration;

#ifdef XAF_PROFILE
    extern long long tot_cycles, frmwk_cycles, fread_cycles, fwrite_cycles;
    extern long long dsp_comps_cycles, dec_cycles,renderer_cycles;
    extern double dsp_mcps;
#endif

/* Dummy unused functions */
XA_ERRORCODE xa_aac_decoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_mixer(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_pcm_gain(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_mp3_encoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_src_pp_fx(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_capturer(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_amr_wb_decoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_hotword_decoder(xa_codec_handle_t p_xa_module_obj, WORD32 i_cmd, WORD32 i_idx, pVOID pv_value){return 0;}
XA_ERRORCODE xa_vorbis_decoder(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_dummy_aec22(xa_codec_handle_t p_xa_module_obj, WORD32 i_cmd, WORD32 i_idx, pVOID pv_value) {return 0;}
XA_ERRORCODE xa_dummy_aec23(xa_codec_handle_t p_xa_module_obj, WORD32 i_cmd, WORD32 i_idx, pVOID pv_value) {return 0;}
XA_ERRORCODE xa_pcm_split(xa_codec_handle_t p_xa_module_obj, WORD32 i_cmd, WORD32 i_idx, pVOID pv_value) {return 0;}
XA_ERRORCODE xa_mimo_mix(xa_codec_handle_t p_xa_module_obj, WORD32 i_cmd, WORD32 i_idx, pVOID pv_value) {return 0;}
XA_ERRORCODE xa_dummy_wwd(xa_codec_handle_t p_xa_module_obj, WORD32 i_cmd, WORD32 i_idx, pVOID pv_value) {return 0;}
XA_ERRORCODE xa_dummy_hbuf(xa_codec_handle_t p_xa_module_obj, WORD32 i_cmd, WORD32 i_idx, pVOID pv_value) {return 0;}
XA_ERRORCODE xa_opus_encoder(xa_codec_handle_t p_xa_module_obj, WORD32 i_cmd, WORD32 i_idx, pVOID pv_value) {return 0;}
XA_ERRORCODE xa_dummy_wwd_msg(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}
XA_ERRORCODE xa_dummy_hbuf_msg(xa_codec_handle_t var1, WORD32 var2, WORD32 var3, pVOID var4){return 0;}

void help_info()
{
	FIO_PRINTF(stdout, "\n\n**************************************************\n");
	FIO_PRINTF(stdout, "* Renderer Test aplication for DSP\n");
	FIO_PRINTF(stdout, "* Options :\n\n");
	FIO_PRINTF(stdout, "          -f AudFormat  Audio Format(1-16)\n");
	FIO_PRINTF(stdout, "                        MP3        for 1\n");
	FIO_PRINTF(stdout, "                        AAC        for 2\n");
	FIO_PRINTF(stdout, "                        DAB        for 3\n");
	FIO_PRINTF(stdout, "                        MP2        for 4\n");
	FIO_PRINTF(stdout, "                        BSAC       for 5\n");
	FIO_PRINTF(stdout, "                        DRM        for 6\n");
	FIO_PRINTF(stdout, "                        SBCDEC     for 7\n");
	FIO_PRINTF(stdout, "                        SBCENC     for 8\n");
	FIO_PRINTF(stdout, "                        FSLOGGDEC  for 9\n");
	FIO_PRINTF(stdout, "                        FSLMP3DEC  for 10\n");
	FIO_PRINTF(stdout, "                        FSLACCDEC  for 11\n");
	FIO_PRINTF(stdout, "                        FSLAC3DEC  for 13\n");
	FIO_PRINTF(stdout, "                        FSLDDPDEC  for 14\n");
	FIO_PRINTF(stdout, "                        FSLNBAMRDEC  for 15\n");
	FIO_PRINTF(stdout, "                        FSLWBAMRDEC  for 16\n");
	FIO_PRINTF(stdout, "                        FSLWMADEC    for 17\n");
	FIO_PRINTF(stdout, "          -i InFileNam  Input File Name\n");
	FIO_PRINTF(stdout, "          -s Samplerate Sampling Rate of Audio\n");
	FIO_PRINTF(stdout, "          -n Channel    Channel Number of Audio\n");
	FIO_PRINTF(stdout, "          -d Width      The Width of Samples\n");
	FIO_PRINTF(stdout, "          -r bitRate    Bit Rate\n");
	FIO_PRINTF(stdout, "          -t StreamType Stream Type\n");
	FIO_PRINTF(stdout, "                        0(STREAM_UNKNOWN)\n");
	FIO_PRINTF(stdout, "                        1(STREAM_ADTS)\n");
	FIO_PRINTF(stdout, "                        2(STREAM_ADIF)\n");
	FIO_PRINTF(stdout, "                        3(STREAM_RAW)\n");
	FIO_PRINTF(stdout, "                        4(STREAM_LATM)\n");
	FIO_PRINTF(stdout, "                        5(STREAM_LATM_OUTOFBAND_CONFIG)\n");
	FIO_PRINTF(stdout, "                        6(STREAM_LOAS)\n");
	FIO_PRINTF(stdout, "                        48(STREAM_DABPLUS_RAW_SIDEINFO)\n");
	FIO_PRINTF(stdout, "                        49(STREAM_DABPLUS)\n");
	FIO_PRINTF(stdout, "                        50(STREAM_BSAC_RAW)\n");
	FIO_PRINTF(stdout, "          -u Channel modes only for SBC_ENC\n");
	FIO_PRINTF(stdout, "                        0(CHMODE_MONO)\n");
	FIO_PRINTF(stdout, "                        1(CHMODE_DUAL)\n");
	FIO_PRINTF(stdout, "                        2(CHMODE_STEREO)\n");
	FIO_PRINTF(stdout, "                        3(CHMODE_JOINT)\n");
	FIO_PRINTF(stdout, "Must set correct parameters for stream\n");
	FIO_PRINTF(stdout, "**************************************************\n\n");
}
static int mp3_setup(void *p_decoder)
{
    int param[2];

    param[0] = UNIA_DEPTH;
    param[1] = 16;

    return(xaf_comp_set_config(p_decoder, 1, &param[0]));
}

static int dummy_setup(void *p_decoder)
{
	return XAF_NO_ERR;
}

static int renderer_setup(void *p_renderer,xaf_format_t renderer_format)
{
    int param[6];

    param[0] = XA_RENDERER_CONFIG_PARAM_PCM_WIDTH;
    param[1] = renderer_format.pcm_width;
    param[2] = XA_RENDERER_CONFIG_PARAM_CHANNELS;
    param[3] = renderer_format.channels;
    param[4] = XA_RENDERER_CONFIG_PARAM_SAMPLE_RATE;
    param[5] = renderer_format.sample_rate;
    return(xaf_comp_set_config(p_renderer, 3, &param[0]));
}


static int get_comp_config(void *p_comp, xaf_format_t *comp_format)
{
    int param[6];
    int ret;


    TST_CHK_PTR(p_comp, "get_comp_config");
    TST_CHK_PTR(comp_format, "get_comp_config");

    param[0] = UNIA_CHANNEL;
    param[2] = UNIA_DEPTH;
    param[4] = UNIA_SAMPLERATE;

    ret = xaf_comp_get_config(p_comp, 3, &param[0]);
    if(ret < 0)
        return ret;

    comp_format->channels = param[1];
    comp_format->pcm_width = param[3];
    comp_format->sample_rate = param[5];

    return 0;
}

static int renderer_get_config(void *p_comp, xaf_format_t *comp_format)
{
    int param[8];
    int ret;
    
    
    TST_CHK_PTR(p_comp, "get_renderer_config");
    TST_CHK_PTR(comp_format, "get_renderer_config");
    param[0] = XA_RENDERER_CONFIG_PARAM_CHANNELS;
    param[2] = XA_RENDERER_CONFIG_PARAM_PCM_WIDTH;
    param[4] = XA_RENDERER_CONFIG_PARAM_SAMPLE_RATE;    
    param[6] = XA_RENDERER_CONFIG_PARAM_BYTES_PRODUCED;
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

struct AudioOption {
	/* General Options */
	int CodecFormat;
	int SampleRate;
	int Channel;
	int Width;
	int Bitrate;
	int streamtype;

	/* Dedicate for SBC ENC */
	int channel_mode;

	/* Input & Output File Name */
	char *InFileName;
	char *OutFileName;
};

int GetParameter(int argc_t, char *argv_t[], struct AudioOption *pAOption)
{
	int i;
	char *pTmp = NULL, in[256];

	if (argc_t < 3) {
		goto Error;
	}
	for (i = 1; i < argc_t; i++) {
		pTmp = argv_t[i];
		if (*pTmp == '-') {
			strcpy(in, pTmp + 2);
			switch (pTmp[1]) {
			case 'f':
				pAOption->CodecFormat = atoi(in);
				break;
			case 'r':
				pAOption->Bitrate = atoi(in);
				break;
			case 's':
				pAOption->SampleRate = atoi(in);
				break;
			case 'n':
				pAOption->Channel = atoi(in);
				break;
			case 'd':
				pAOption->Width = atoi(in);
				break;
			case 't':
				pAOption->streamtype = atoi(in);
				break;
			case 'i':
				pAOption->InFileName = pTmp + 2;
				break;
			case 'o':
				pAOption->OutFileName = pTmp + 2;
				break;
			case 'u':
				pAOption->channel_mode = atoi(in);
				break;
			default:
				goto Error;
			}
		}
	}
	return 0;

Error:
	return 1;
}

int AOption_setparam(void *p_decoder, void *p_AOption)
{
	struct AudioOption *AOption = p_AOption;
	int param[2];

	if (AOption->SampleRate > 0) {
		param[0] = UNIA_SAMPLERATE;
		param[1] = AOption->SampleRate;
		xaf_comp_set_config(p_decoder, 1, &param[0]);
	}
	if (AOption->Channel > 0) {
		param[0] = UNIA_CHANNEL;
		param[1] = AOption->Channel;
		xaf_comp_set_config(p_decoder, 1, &param[0]);
	}
	if (AOption->Width > 0) {
		param[0] = UNIA_DEPTH;
		param[1] = AOption->Width;
		xaf_comp_set_config(p_decoder, 1, &param[0]);
	}
	if (AOption->Bitrate > 0) {
		param[0] = UNIA_BITRATE;
		param[1] = AOption->Bitrate;
		xaf_comp_set_config(p_decoder, 1, &param[0]);
	}
	if (AOption->streamtype > 0) {
		param[0] = UNIA_STREAM_TYPE;
		param[1] = AOption->streamtype;
		xaf_comp_set_config(p_decoder, 1, &param[0]);
	}
	if (AOption->channel_mode > 0) {
		param[0] = UNIA_SBC_ENC_CHMODE;
		param[1] = AOption->channel_mode;
		xaf_comp_set_config(p_decoder, 1, &param[0]);
	}
	return 0;
}

int get_decoding_format(void *p_input, void *p_adev, xf_id_t dec_id, xaf_comp_type comp_type,
		int (*dec_setup)(void *p_comp), struct AudioOption AOption,
		xaf_format_t *dec_format)
{
	void *p_output;
	FILE *ofp;
	void *p_decoder = NULL;
	void *dec_inbuf[2];
	int buf_length = XAF_INBUF_SIZE;
	int read_length;

	int i;
	int error = 0;
	long comp_info[4];
	int input_over = 0;
	xaf_comp_status comp_status;

	ofp = fio_fopen("/dev/null", "wb");
	p_output = ofp;
	/* ...create decoder component */
	TST_CHK_API_COMP_CREATE(p_adev, &p_decoder, dec_id, 2, 1, &dec_inbuf[0], comp_type, "xaf_comp_create");
#ifdef XA_FSL_UNIA_CODEC
	TST_CHK_API(xaf_load_library(p_adev, p_decoder, dec_id), "xaf_load_library");
#endif
	TST_CHK_API(dec_setup(p_decoder), "dec_setup");

	TST_CHK_API(AOption_setparam(p_decoder, &AOption), "AOption setparam");

	/* ...start decoder component */
	TST_CHK_API(xaf_comp_process(p_adev, p_decoder, NULL, 0, XAF_START_FLAG), "xaf_comp_process");

	/* ...feed input to decoder component */
	for (i=0; i<2; i++)
	{
		TST_CHK_API(read_input(dec_inbuf[i], buf_length, &read_length, p_input, comp_type), "read_input");

		if (read_length)
			TST_CHK_API(xaf_comp_process(p_adev, p_decoder, dec_inbuf[i], read_length, XAF_INPUT_READY_FLAG), "xaf_comp_process");
		else {
			TST_CHK_API(xaf_comp_process(p_adev, p_decoder, NULL, 0, XAF_INPUT_OVER_FLAG), "xaf_comp_process");
		break;
		}
	}

	/* ...initialization loop */
	while (1)
	{
		TST_CHK_API(xaf_comp_get_status(p_adev, p_decoder, &comp_status, &comp_info[0]), "xaf_comp_get_status");

		if (comp_status == XAF_INIT_DONE || comp_status == XAF_EXEC_DONE) break;

		if (comp_status == XAF_NEED_INPUT)
		{
			void *p_buf = (long *) comp_info[0];
			long size    = (long)comp_info[1];

			TST_CHK_API(read_input(p_buf, size, &read_length, p_input, comp_type), "read_input");

			if (read_length)
				TST_CHK_API(xaf_comp_process(p_adev, p_decoder, p_buf, read_length, XAF_INPUT_READY_FLAG), "xaf_comp_process");
			else {
				TST_CHK_API(xaf_comp_process(p_adev, p_decoder, NULL, 0, XAF_INPUT_OVER_FLAG), "xaf_comp_process");
				break;
			}
		}
	}

	if (comp_status != XAF_INIT_DONE)
	{
		FIO_PRINTF(stderr, "Failed to init");
		if (ofp) fio_fclose(ofp);
		exit(-1);
	}

	TST_CHK_API(xaf_comp_process(NULL, p_decoder, NULL, 0, XAF_EXEC_FLAG), "xaf_comp_process");

	while (comp_status != XAF_OUTPUT_READY) {
		error = xaf_comp_get_status(NULL, p_decoder, &comp_status, &comp_info[0]);

		if (comp_status == XAF_EXEC_DONE) break;

		if (comp_status == XAF_NEED_INPUT && !input_over) {
			read_length = 0;
			void *p_buf = (void *)comp_info[0];
			long size = (int)comp_info[1];
			TST_CHK_API(read_input(p_buf, size, &read_length, p_input, comp_type), "read_input");
			if (read_length)
				TST_CHK_API(xaf_comp_process(NULL, p_decoder, (void *)comp_info[0], read_length, XAF_INPUT_READY_FLAG), "xaf_comp_process");
			else {
				TST_CHK_API(xaf_comp_process(NULL, p_decoder, NULL, 0, XAF_INPUT_OVER_FLAG), "xaf_comp_process");
				input_over = 1;
			}
		}
	}
	TST_CHK_API(get_comp_config(p_decoder, dec_format), "get_comp_config");

	TST_CHK_API(xaf_comp_delete(p_decoder), "xaf_comp_delete");

	if (dec_format->channels == 0 || dec_format->channels > 2 || dec_format->pcm_width != 16 || dec_format->sample_rate == 0) {
		printf("decoding format not support in render!\n");
		if (ofp) fio_fclose(ofp);
		TST_CHK_API(xaf_adev_close(p_adev, XAF_ADEV_NORMAL_CLOSE), "xaf_adev_close");
		return -1;
	}

	if (ofp) fio_fclose(ofp);

	printf("get samplerate %d, channel %d, width %d\n", dec_format->sample_rate, dec_format->channels, dec_format->pcm_width);

	return 0;
}

int main_task(int argc, char **argv)
{

    void *p_adev = NULL;
    void *p_decoder = NULL;
    void *p_renderer = NULL;
    void *p_input;
    xf_thread_t dec_thread;
    unsigned char dec_stack[STACK_SIZE];
    xaf_comp_status dec_status;
    long dec_info[4];
    char *filename_ptr;
    void *dec_thread_args[NUM_THREAD_ARGS];
    FILE *fp;
    void *dec_inbuf[2];
    int buf_length = XAF_INBUF_SIZE;
    int read_length;
    int i;
    xaf_comp_type comp_type;
    xf_id_t dec_id;
    int (*dec_setup)(void *p_comp);
    const char *ext;

    xaf_format_t dec_format;
    xaf_format_t renderer_format;
    int num_comp;
    pUWORD8 ver_info[3] = {0,0,0};    //{ver,lib_rev,api_rev}
    unsigned short board_id = 0;
    mem_obj_t* mem_handle;

    struct AudioOption AOption;

#ifdef XAF_PROFILE
    frmwk_cycles = 0;
    fread_cycles = 0;
    fwrite_cycles = 0;
    dec_cycles = 0;
    dsp_comps_cycles = 0;
    tot_cycles = 0;
    num_bytes_read = 0;
    num_bytes_write = 0;
    renderer_cycles = 0;
#endif

    memset(&dec_format, 0, sizeof(xaf_format_t));
    memset(&renderer_format, 0, sizeof(xaf_format_t));

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
    TST_CHK_API(print_verinfo(ver_info,(pUWORD8)"\'Decoder + Renderer\'"), "print_verinfo");

    /* ...initialize tracing facility */
    TRACE_INIT("Xtensa Audio Framework - \'Decoder + Renderer\' Sample App");

    //Initialize Audio Option structure
    AOption.SampleRate = -1;
    AOption.Channel = -1;
    AOption.Width = -1;
    AOption.Bitrate = -1;
    AOption.streamtype = -1;
    AOption.channel_mode = -1;

    if (GetParameter(argc, argv, &AOption)) {
        help_info();
        return 0;
    }

    /* ...open file */
    if ((fp = fio_fopen(AOption.InFileName, "rb")) == NULL)
    {
        FIO_PRINTF(stderr, "Failed to open '%s': %d\n", filename_ptr, errno);
        exit(-1);
    }
    p_input  = fp;

    switch (AOption.CodecFormat) {
    case CODEC_MP3_DEC:
        dec_id    = "audio-decoder/mp3";
        dec_setup = mp3_setup;
        break;
    case CODEC_AAC_DEC:
        dec_id    = "audio-decoder/aac";
        dec_setup = dummy_setup;
        break;
    case CODEC_DAB_DEC:
        dec_id    = "audio-decoder/dabplus";
        dec_setup = dummy_setup;
        break;
    case CODEC_MP2_DEC:
        dec_id    = "audio-decoder/mp2";
        dec_setup = dummy_setup;
        break;
    case CODEC_BSAC_DEC:
        dec_id    = "audio-decoder/bsac";
        dec_setup = dummy_setup;
        break;
    case CODEC_DRM_DEC:
        dec_id    = "audio-decoder/drm";
        dec_setup = dummy_setup;
        break;
    case CODEC_SBC_DEC:
        dec_id    = "audio-decoder/sbc";
        dec_setup = dummy_setup;
        break;
    case CODEC_SBC_ENC:
        dec_id    = "audio-encoder/sbc";
        dec_setup = dummy_setup;
        break;
    case CODEC_FSL_OGG_DEC:
        dec_id    = "audio-decoder/fsl-ogg";
        dec_setup = dummy_setup;
        break;
    case CODEC_FSL_MP3_DEC:
        dec_id    = "audio-decoder/fsl-mp3";
        dec_setup = dummy_setup;
        break;
    case CODEC_FSL_AAC_DEC:
        dec_id    = "audio-decoder/fsl-aac";
        dec_setup = dummy_setup;
        break;
    case CODEC_FSL_AAC_PLUS_DEC:
        dec_id    = "audio-decoder/fsl-aacplus";
        dec_setup = dummy_setup;
        break;
    case CODEC_FSL_AC3_DEC:
        dec_id    = "audio-decoder/fsl-ac3";
        dec_setup = dummy_setup;
        break;
    case CODEC_FSL_DDP_DEC:
        dec_id    = "audio-decoder/fsl-ddp";
        dec_setup = dummy_setup;
        break;
    case CODEC_FSL_NBAMR_DEC:
        dec_id    = "audio-decoder/fsl-nbamr";
        dec_setup = dummy_setup;
        break;
    case CODEC_FSL_WBAMR_DEC:
        dec_id    = "audio-decoder/fsl-wbamr";
        dec_setup = dummy_setup;
        break;
    case CODEC_FSL_WMA_DEC:
        dec_id    = "audio-decoder/fsl-wma";
        dec_setup = dummy_setup;
        break;
    case CODEC_OPUS_DEC:
        dec_id    = "audio-decoder/opus";
        dec_setup = dummy_setup;
        break;
    case CODEC_PCM_DEC:
        dec_id    = "audio-decoder/pcm";
        dec_setup = dummy_setup;
        break;
    default:
        return 1;
    }

    xaf_adev_config_t adev_config;
    TST_CHK_API(xaf_adev_config_default_init(&adev_config), "xaf_adev_config_default_init");

    mem_handle = mem_init(&adev_config);

    adev_config.pmem_malloc =  mem_malloc;
    adev_config.pmem_free =  mem_free;
    adev_config.audio_framework_buffer_size =  audio_frmwk_buf_size;
    adev_config.audio_component_buffer_size =  audio_comp_buf_size;
    TST_CHK_API(xaf_adev_open(&p_adev, &adev_config),  "xaf_adev_open");

    FIO_PRINTF(stdout, "Audio Device Ready\n");

    int error = get_decoding_format(p_input, p_adev, dec_id, comp_type,
			dec_setup, AOption, &dec_format);
    if (error != 0)
	    exit(-1);

    fseek(fp, 0L, SEEK_SET);

    /* ...create decoder component */
    comp_type = XAF_DECODER;

    TST_CHK_API_COMP_CREATE(p_adev, &p_decoder, dec_id, 2, 0, &dec_inbuf[0], comp_type, "xaf_comp_create");
#ifdef XA_FSL_UNIA_CODEC
    TST_CHK_API(xaf_load_library(p_adev, p_decoder, dec_id), "xaf_load_library");
#endif
    TST_CHK_API(dec_setup(p_decoder), "dec_setup");

    TST_CHK_API(AOption_setparam(p_decoder, &AOption), "AOption setparam");

    /* ...start decoder component */
    TST_CHK_API(xaf_comp_process(p_adev, p_decoder, NULL, 0, XAF_START_FLAG), "xaf_comp_process");

    /* ...feed input to decoder component */
    for (i=0; i<2; i++)
    {
        TST_CHK_API(read_input(dec_inbuf[i], buf_length, &read_length, p_input, comp_type), "read_input");

        if (read_length)
            TST_CHK_API(xaf_comp_process(p_adev, p_decoder, dec_inbuf[i], read_length, XAF_INPUT_READY_FLAG), "xaf_comp_process");
        else
        {    
            TST_CHK_API(xaf_comp_process(p_adev, p_decoder, NULL, 0, XAF_INPUT_OVER_FLAG), "xaf_comp_process");
            break;
        }
    }

    /* ...initialization loop */
    while (1)
    {
        TST_CHK_API(xaf_comp_get_status(p_adev, p_decoder, &dec_status, &dec_info[0]), "xaf_comp_get_status");

        if (dec_status == XAF_INIT_DONE || dec_status == XAF_EXEC_DONE) break;

        if (dec_status == XAF_NEED_INPUT)
        {
            void *p_buf = (long *) dec_info[0];
            long size    = (long)dec_info[1];

            TST_CHK_API(read_input(p_buf, size, &read_length, p_input, comp_type), "read_input");

            if (read_length)
                TST_CHK_API(xaf_comp_process(p_adev, p_decoder, p_buf, read_length, XAF_INPUT_READY_FLAG), "xaf_comp_process");
            else
            {    
                TST_CHK_API(xaf_comp_process(p_adev, p_decoder, NULL, 0, XAF_INPUT_OVER_FLAG), "xaf_comp_process");
                break;
            }
        }
    }

    if (dec_status != XAF_INIT_DONE)
    {
        FIO_PRINTF(stderr, "Failed to init");
        exit(-1);
    }

    renderer_format.sample_rate = dec_format.sample_rate;
    renderer_format.channels = dec_format.channels;
    renderer_format.pcm_width = dec_format.pcm_width;
    TST_CHK_API_COMP_CREATE(p_adev, &p_renderer, "renderer", 0, 0, NULL, XAF_RENDERER, "xaf_comp_create");
    TST_CHK_API(renderer_setup(p_renderer, renderer_format), "renderer_setup");
    TST_CHK_API(xaf_connect(p_decoder, 1, p_renderer, 0, 4), "renderer_connect");
    TST_CHK_API(xaf_comp_process(p_adev, p_renderer, NULL, 0, XAF_START_FLAG), "xaf_comp_process");
    TST_CHK_API(xaf_comp_get_status(p_adev, p_renderer, &dec_status, &dec_info[0]), "xaf_comp_get_status");




#ifdef XAF_PROFILE
    clk_start();
    
#endif

    dec_thread_args[0]= p_adev;
    dec_thread_args[1]= p_decoder;
    dec_thread_args[2]= p_input;
    dec_thread_args[3]= NULL;
    dec_thread_args[4]= &comp_type;
    dec_thread_args[5]= (void *)dec_id;
    dec_thread_args[6]= (void *)&i; //dummy

    /* ...init done, begin execution thread */
    __xf_thread_create(&dec_thread, comp_process_entry, &dec_thread_args[0], "Decoder Thread", dec_stack, STACK_SIZE, XAF_APP_THREADS_PRIORITY);

    __xf_thread_join(&dec_thread, NULL);
   
#ifdef XAF_PROFILE
    compute_total_frmwrk_cycles();
    clk_stop();
    
#endif

    TST_CHK_API(renderer_get_config(p_renderer, &renderer_format), "renderer get config");
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
    __xf_thread_destroy(&dec_thread);
    TST_CHK_API(xaf_comp_delete(p_renderer), "xaf_comp_delete");
    TST_CHK_API(xaf_comp_delete(p_decoder), "xaf_comp_delete");
    TST_CHK_API(xaf_adev_close(p_adev, XAF_ADEV_NORMAL_CLOSE), "xaf_adev_close");
    FIO_PRINTF(stdout,"Audio device closed\n\n");

    mem_exit(mem_handle);

#ifdef XAF_PROFILE
    dsp_comps_cycles = dec_cycles + renderer_cycles;

    dsp_mcps = compute_comp_mcps(renderer_format.output_produced, dec_cycles, dec_format, &strm_duration);
    dsp_mcps += compute_comp_mcps(renderer_format.output_produced, renderer_cycles, renderer_format, &strm_duration);
    TST_CHK_API(print_mem_mcps_info(mem_handle, num_comp), "print_mem_mcps_info");
#endif

    if (fp)  fio_fclose(fp);
    
    fio_quit();
    
    /* ...deinitialize tracing facility */
    TRACE_DEINIT();
    
    return 0;
}

