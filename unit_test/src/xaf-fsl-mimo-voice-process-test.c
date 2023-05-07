// Copyright 2023 NXP
// SPDX-License-Identifier: BSD-3-Clause

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "xa-pcm-gain-api.h"
#include "audio/xa-pcm-gain-api.h"
#include "audio/xa-renderer-api.h"
#include "audio/xa-capturer-api.h"
#include "xaf-utils-test.h"
#include "xaf-fio-test.h"
#include "fsl_unia.h"
#include "dsp_codec_interface.h"
#include "library_load.h"
#include "get_pcm_info.h"

#define AUDIO_FRMWK_BUF_SIZE   (256 << 8)
#define AUDIO_COMP_BUF_SIZE    (1024 << 7)

#define NUM_COMP_IN_GRAPH       2

//component parameters
#define MP3_DEC_PCM_WIDTH       16

#define CAPTURER_PCM_WIDTH       (32)
#define CAPTURER_SAMPLE_RATE     (48000)
#define CAPTURER_NUM_CH          (2)

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
	FIO_PRINTF(stdout, "* Voice Process Test aplication for DSP\n");
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
	FIO_PRINTF(stdout, "          -o Outfile    out_filename\n");
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
	FIO_PRINTF(stdout, "          -C codec type 1(WM8960)\n");
	FIO_PRINTF(stdout, "                        2(WM8962)\n");
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

static int renderer_setup(void *p_renderer,xaf_format_t renderer_format, uint32_t codec_type)
{
    int param[8];

    param[0] = XA_RENDERER_CONFIG_PARAM_PCM_WIDTH;
    param[1] = renderer_format.pcm_width;
    param[2] = XA_RENDERER_CONFIG_PARAM_CHANNELS;
    param[3] = renderer_format.channels;
    param[4] = XA_RENDERER_CONFIG_PARAM_SAMPLE_RATE;
    param[5] = renderer_format.sample_rate;
    param[6] = XA_RENDERER_CONFIG_PARAM_CODEC_TYPE;
    param[7] = codec_type;
    return(xaf_comp_set_config(p_renderer, 4, &param[0]));
}

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))

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

#define PCM_GAIN_IDX_FOR_GAIN   1

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

static int voiceprocess_setup(void *p_voiceprocess,xaf_format_t voiceprocess_format)
{
    int param[2];

    param[0] = UNIA_CHANNEL;
    param[1] = voiceprocess_format.channels;

    return(xaf_comp_set_config(p_voiceprocess, 1, &param[0]));

   return 0;
}

static int get_comp_config(void *p_comp, xaf_format_t *comp_format)
{
    int param[6];
    int ret;

    TST_CHK_PTR(p_comp, "get_comp_config");
    TST_CHK_PTR(comp_format, "get_comp_config");

    param[0] = XA_CODEC_CONFIG_PARAM_CHANNELS;
    param[2] = XA_CODEC_CONFIG_PARAM_PCM_WIDTH;
    param[4] = XA_CODEC_CONFIG_PARAM_SAMPLE_RATE;
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
	int captured;

	uint32_t codec_type;
};

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
			case 'C':
				pAOption->codec_type = atoi(in);
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

	if (AOption->CodecFormat != CODEC_PCM_GAIN) {
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
	} else {
		fprintf(stderr, "Audio Format not support!\n");
		return -1;
	}
	return 0;
}

int get_decoding_format(void *p_input, void *p_adev, xf_id_t dec_id, xaf_comp_type comp_type,
		int (*dec_setup)(void *p_comp), struct AudioOption *AOption,
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

	if (AOption->CodecFormat == CODEC_PCM_GAIN) {
		if (get_codec_pcm(p_input, dec_format) != 0)
			return -1;
		dec_format->pcm_width = 16;
		if (dec_format->channels == 0 || dec_format->channels > 2 || dec_format->sample_rate == 0) {
			printf("unsupport channel or samplerate!\n");
			return -1;
		}
		AOption->Channel = dec_format->channels;
		AOption->SampleRate = dec_format->sample_rate;
		AOption->Width = dec_format->pcm_width;
		printf("get samplerate %d, channel %d, width %d\n", dec_format->sample_rate, dec_format->channels, dec_format->pcm_width);
		return 0;
	}

	ofp = fio_fopen("/dev/null", "wb");
	p_output = ofp;
	/* ...create decoder component */
	TST_CHK_API_COMP_CREATE(p_adev, &p_decoder, dec_id, 2, 1, &dec_inbuf[0], comp_type, "xaf_comp_create");
#ifdef XA_FSL_UNIA_CODEC
	TST_CHK_API(xaf_load_library(p_adev, p_decoder, dec_id), "xaf_load_library");
#endif
	TST_CHK_API(dec_setup(p_decoder), "dec_setup");

	TST_CHK_API(AOption_setparam(p_decoder, AOption), "AOption setparam");

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
    void *p_capturer  = NULL;
    void *p_voiceprocess = NULL;
    void *p_pcm_gain  = NULL;
    void *p_input;
    void *p_output;
    xf_thread_t dec_thread;
    xf_thread_t pcm_gain_thread;
    unsigned char dec_stack[STACK_SIZE];
    unsigned char pcm_gain_stack[STACK_SIZE];
    xaf_comp_status dec_status;
    xaf_comp_status capturer_status;
    xaf_comp_status pcm_gain_status;
    long dec_info[4];
    int pcm_gain_info[4];
    int capturer_info[4];
    char *filename_ptr;
    void *dec_thread_args[NUM_THREAD_ARGS];
    void *pcm_gain_thread_args[NUM_THREAD_ARGS];
    FILE *fp;
    FILE *ofp;
    void *dec_inbuf[2];
    int buf_length = XAF_INBUF_SIZE;
    int read_length;
    int i;
    xaf_comp_type comp_type;
    long long int sample_end_capturer = -1;
    xf_id_t dec_id;
    int (*dec_setup)(void *p_comp);
    const char *ext;

    xaf_format_t dec_format;
    xaf_format_t renderer_format;
    uint32_t codec_type;
    xaf_format_t capturer_format;
    xaf_format_t pcm_gain_format;
    xaf_format_t voiceprocess_format;
    int num_comp;
    pUWORD8 ver_info[3] = {0,0,0};    //{ver,lib_rev,api_rev}
    unsigned short board_id = 0;
    mem_obj_t* mem_handle;

    struct AudioOption AOption;
    int ret_state;
    xf_id_t comp_id;

    int ret;
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
    memset(&pcm_gain_format, 0, sizeof(xaf_format_t));
    memset(&capturer_format, 0, sizeof(xaf_format_t));
    memset(&voiceprocess_format, 0, sizeof(xaf_format_t));

    audio_frmwk_buf_size = AUDIO_FRMWK_BUF_SIZE;
    audio_comp_buf_size = AUDIO_COMP_BUF_SIZE;
    num_comp = NUM_COMP_IN_GRAPH;

    // NOTE: set_wbna() should be called before any other dynamic
    // adjustment of the region attributes for cache.
    set_wbna(&argc, argv);

    /* ...start xos */
    board_id = start_rtos();

    /* ...initialize tracing facility */
    TRACE_INIT("Xtensa Audio Framework - \'Decoder + Renderer\' Sample App");

    //Initialize Audio Option structure
    AOption.SampleRate = -1;
    AOption.Channel = -1;
    AOption.Width = -1;
    AOption.Bitrate = -1;
    AOption.streamtype = -1;
    AOption.channel_mode = -1;

    AOption.codec_type = 0;

    if (GetParameter(argc, argv, &AOption)) {
        help_info();
        return 0;
    }

    if (AOption.codec_type < 0 || AOption.codec_type > 2) {
	fprintf(stderr, "Not support codec type %d\n", AOption.codec_type);
	return 0;
    }
    if (AOption.codec_type == 0)
	codec_type = 1;
    else
	codec_type = AOption.codec_type;

    /* ...open file */
    if ((fp = fio_fopen(AOption.InFileName, "rb")) == NULL)
    {
        FIO_PRINTF(stderr, "Failed to open '%s': %d\n", filename_ptr, errno);
        exit(-1);
    }
    p_input  = fp;

    ofp = fopen(AOption.OutFileName, "wb");
    if (!ofp) {
        FIO_PRINTF(stderr, "outfile: %s open failed!\n", AOption.OutFileName);
        return -ENOENT;
    }
    p_output = ofp;

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
    case CODEC_PCM_GAIN:
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
			dec_setup, &AOption, &dec_format);
    if (error != 0) {
        TST_CHK_API(xaf_adev_close(p_adev, XAF_ADEV_NORMAL_CLOSE), "xaf_adev_close");
        exit(-1);
    }

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

    /* ...create capturer component */
    comp_type = XAF_CAPTURER;
    capturer_format.sample_rate = CAPTURER_SAMPLE_RATE;
    capturer_format.channels = CAPTURER_NUM_CH;
    capturer_format.pcm_width = CAPTURER_PCM_WIDTH;
    TST_CHK_API_COMP_CREATE(p_adev, &p_capturer, "capturer", 0, 0, NULL, comp_type, "xaf_comp_create");
    TST_CHK_API(capturer_setup(p_capturer, capturer_format, sample_end_capturer), "capturer_setup");

    TST_CHK_API(xaf_comp_process(p_adev, p_capturer, NULL, 0, XAF_START_FLAG),"xaf_comp_process");

    TST_CHK_API(xaf_comp_get_status(p_adev, p_capturer, &capturer_status, &capturer_info[0]), "xaf_comp_get_status");

    if (capturer_status != XAF_INIT_DONE)
    {
        FIO_PRINTF(stderr, "Failed to init");
        exit(-1);
    }

    /* init voice seeker */
    voiceprocess_format.sample_rate = capturer_format.sample_rate;
    voiceprocess_format.channels = capturer_format.channels;
    voiceprocess_format.pcm_width = capturer_format.pcm_width;
    /* input port 0, 1
     * output port 2, 3
     */
    TST_CHK_API_COMP_CREATE(p_adev, &p_voiceprocess, "mimo-proc22/voiceprocess", 0, 0, NULL, XAF_MIMO_PROC_22, "xaf_comp_create");

    TST_CHK_API(voiceprocess_setup(p_voiceprocess, voiceprocess_format), "voiceprocess_setup");

    /* ...start voiceprocess component */
    TST_CHK_API(xaf_comp_process(p_adev, p_voiceprocess, NULL, 0, XAF_START_FLAG),"xaf_comp_process");

    TST_CHK_API(xaf_comp_get_status(p_adev, p_voiceprocess, &dec_status, &dec_info[0]), "xaf_comp_get_status");

    if (dec_status != XAF_INIT_DONE)
    {
        FIO_PRINTF(stderr, "Failed to init");
        exit(-1);
    }

    TST_CHK_API(xaf_load_library(p_adev, p_voiceprocess, "voice-process/dummy"), "xaf_load_library");

    /* decoder output port 1 connect to voice seeker input port 0 */
    TST_CHK_API(xaf_connect(p_decoder, 1, p_voiceprocess, 0, 4), "voiceprocess_connect");
    /* ...capturer port 0 is output port */
    TST_CHK_API(xaf_connect(p_capturer, 0, p_voiceprocess, 1, 8), "voiceprocess_connect");

    /* init render */
    renderer_format.sample_rate = dec_format.sample_rate;
    renderer_format.channels = dec_format.channels;
    renderer_format.pcm_width = dec_format.pcm_width;
    TST_CHK_API_COMP_CREATE(p_adev, &p_renderer, "renderer", 0, 0, NULL, XAF_RENDERER, "xaf_comp_create");
    TST_CHK_API(renderer_setup(p_renderer, renderer_format, codec_type), "renderer_setup");
    /* voice seeker output port 2 connect to render input port 0 */
    TST_CHK_API(xaf_connect(p_voiceprocess, 2, p_renderer, 0, 4), "renderer_connect");

    pcm_gain_format.sample_rate = PCM_GAIN_SAMPLE_RATE;
    pcm_gain_format.channels = PCM_GAIN_NUM_CH;
    pcm_gain_format.pcm_width = PCM_GAIN_SAMPLE_WIDTH;

     /* ...create pcm gain component */
    TST_CHK_API_COMP_CREATE(p_adev, &p_pcm_gain, "post-proc/pcm_gain", 0, 1, NULL, XAF_POST_PROC, "xaf_comp_create");
    TST_CHK_API(pcm_gain_setup(p_pcm_gain, pcm_gain_format), "pcm_gain_setup");

    TST_CHK_API(xaf_connect(p_voiceprocess, 3, p_pcm_gain, 0, 4), "xaf_connect");


    TST_CHK_API(xaf_comp_process(p_adev, p_renderer, NULL, 0, XAF_START_FLAG), "xaf_comp_process");
    TST_CHK_API(xaf_comp_get_status(p_adev, p_renderer, &dec_status, &dec_info[0]), "xaf_comp_get_status");

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

    comp_type = XAF_DECODER;
    dec_thread_args[0]= p_adev;
    dec_thread_args[1]= p_decoder;
    dec_thread_args[2]= p_input;
    dec_thread_args[3]= NULL;
    dec_thread_args[4]= &comp_type;
    dec_thread_args[5]= (void *)dec_id;
    dec_thread_args[6]= (void *)&i; //dummy

    /* ...init done, begin execution thread */
    __xf_thread_create(&dec_thread, comp_process_entry, &dec_thread_args[0], "Decoder Thread", dec_stack, STACK_SIZE, XAF_APP_THREADS_PRIORITY);

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

    ret = __xf_thread_join(&dec_thread, NULL);
    if(ret != 0)
    {
        FIO_PRINTF(stdout,"Decoder thread exit Failed : %d \n", ret);
        exit(-1);
    }
    __xf_thread_destroy(&dec_thread);

    ret = __xf_thread_join(&pcm_gain_thread, NULL);
    if(ret != 0)
    {
        FIO_PRINTF(stdout,"Pcm Gain thread exit Failed : %d \n", ret);
        exit(-1);
    }
    __xf_thread_destroy(&pcm_gain_thread);


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

    TST_CHK_API(xaf_comp_delete(p_renderer), "xaf_comp_delete");
    TST_CHK_API(xaf_comp_delete(p_pcm_gain), "xaf_comp_delete");
    TST_CHK_API(xaf_comp_delete(p_voiceprocess), "xaf_comp_delete");
    TST_CHK_API(xaf_comp_delete(p_decoder), "xaf_comp_delete");
    TST_CHK_API(xaf_comp_delete(p_capturer), "xaf_comp_delete");
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

