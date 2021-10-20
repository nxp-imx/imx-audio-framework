/*****************************************************************
 * Copyright 2018-2020 NXP
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
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <pthread.h>
#include "id3_tag_decode.h"
#include "common.h"
#include "xaf-api.h"

#define ENABLE_ID3  1
#define ENABLE_BSAC_HEADER  1

#define COMP_CODEC		(1 << 0)
#define COMP_RENDER		(1 << 1)

typedef int WORD32;

struct AudioOption {
	/* General Options */
	int AudioFormat;
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

	/* Route audio format */
	int AudioFormatRoute;
	int comp_routed;
};

/* define global variables */
FILE *fd_dst;
FILE *fd_src;
u32 frame_count;
int log_of_perf;

void help_info(int ac, char *av[])
{
	printf("\n\n**************************************************\n");
	printf("* Test aplication for DSP\n");
	printf("* Options :\n\n");
	printf("          -f AudFormat  Audio Format(1-8)\n");
	printf("                        MP3        for 1\n");
	printf("                        AAC        for 2\n");
	printf("                        DAB        for 3\n");
	printf("                        MP2        for 4\n");
	printf("                        BSAC       for 5\n");
	printf("                        DRM        for 6\n");
	printf("                        SBCDEC     for 7\n");
	printf("                        SBCENC     for 8\n");
	printf("                        OGGDEC     for 9\n");
	printf("                        FSLMP3DEC  for 10\n");
	printf("                        FSLACCDEC  for 11\n");
	printf("                        FSLAC3DEC  for 13\n");
	printf("                        FSLDDPDEC  for 14\n");
	printf("                        FSLNBAMRDEC  for 15\n");
	printf("                        FSLWBAMRDEC  for 16\n");
	printf("          -i InFileNam  Input File Name\n");
	printf("          -o OutName    Output File Name\n");
	printf("          -s Samplerate Sampling Rate of Audio\n");
	printf("          -n Channel    Channel Number of Audio\n");
	printf("          -d Width      The Width of Samples\n");
	printf("          -r bitRate    Bit Rate\n");
	printf("          -t StreamType Stream Type\n");
	printf("                        0(STREAM_UNKNOWN)\n");
	printf("                        1(STREAM_ADTS)\n");
	printf("                        2(STREAM_ADIF)\n");
	printf("                        3(STREAM_RAW)\n");
	printf("                        4(STREAM_LATM)\n");
	printf("                        5(STREAM_LATM_OUTOFBAND_CONFIG)\n");
	printf("                        6(STREAM_LOAS)\n");
	printf("                        48(STREAM_DABPLUS_RAW_SIDEINFO)\n");
	printf("                        49(STREAM_DABPLUS)\n");
	printf("                        50(STREAM_BSAC_RAW)\n");
	printf("          -u Channel modes only for SBC_ENC\n");
	printf("                        0(CHMODE_MONO)\n");
	printf("                        1(CHMODE_DUAL)\n");
	printf("                        2(CHMODE_STEREO)\n");
	printf("                        3(CHMODE_JOINT)\n");
	printf("          -c Route AudFormat(1-8)\n");
	printf("                        MP3        for 1\n");
	printf("                        AAC        for 2\n");
	printf("                        DAB        for 3\n");
	printf("                        MP2        for 4\n");
	printf("                        BSAC       for 5\n");
	printf("                        DRM        for 6\n");
	printf("                        SBCDEC     for 7\n");
	printf("                        SBCENC     for 8\n");
	printf("                        RENDER_ESAI     for 32\n");
	printf("                        RENDER_SAI      for 33\n");
	printf("          -l Show the performance data\n");
	printf("**************************************************\n\n");
}

int GetParameter(int argc_t, char *argv_t[], struct AudioOption *pAOption)
{
	int i;
	char *pTmp = NULL, in[256];

	if (argc_t < 3) {
		help_info(argc_t, argv_t);
		goto Error;
	}
	for (i = 1; i < argc_t; i++) {
		pTmp = argv_t[i];
		if (*pTmp == '-') {
			strcpy(in, pTmp + 2);
			switch (pTmp[1]) {
			case 'f':
				pAOption->AudioFormat = atoi(in);
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
			case 'c':
				pAOption->AudioFormatRoute = atoi(in);
				pAOption->comp_routed = COMP_CODEC;
				if (pAOption->AudioFormatRoute >= RENDER_ESAI)
					pAOption->comp_routed = COMP_RENDER;
				break;
			case 'l':
				log_of_perf = 1;
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

void *comp_process_entry(void *arg)
{
	struct xaf_comp *p_comp = (struct xaf_comp *)arg;
	struct xaf_pipeline *p_pipe = p_comp->pipeline;
	struct xaf_info_s p_info;
	int size_read = INBUF_SIZE;
	int size;
	int ret = 0;
	struct timeval tv_begin = {0}, tv_end = {0};

	if (log_of_perf)
		gettimeofday(&tv_begin, NULL);

	do {
		/* ...wait until result is delivered */
		ret = xaf_comp_get_status(p_comp, &p_info);
		if (ret)
			goto Fail;

		if ((p_info.opcode == XF_FILL_THIS_BUFFER) &&
		    (p_info.buf == p_comp->outptr)) {
			TRACE("msg.ret = %x\n", p_info.ret);
			if (p_info.length) {
				u32 actual_size;

				frame_count++;

				if (log_of_perf) {
					/* Receive a buffer */
					gettimeofday(&tv_end, NULL);
					printf("time interval for %d frame %dus\n", frame_count,
					       (tv_end.tv_sec * 1000000 + tv_end.tv_usec) -
					       (tv_begin.tv_sec * 1000000 + tv_begin.tv_usec));
					tv_begin = tv_end;
				}

				actual_size = fwrite((void *)p_comp->outptr, 1,
						     p_info.length, fd_dst);
				TRACE("frame_count = %d, length = %d\n",
				      frame_count, p_info.length);
			}

			if (p_info.ret != XA_SUCCESS) {
				TRACE("error occur during decoding, err = 0x%x\n", p_info.ret);

				switch (p_comp->comp_type) {
				case CODEC_AAC_DEC:
					if (p_info.ret == XA_PROFILE_NOT_SUPPORT) {
						TRACE("error: aac_dec execute fatal unsupported feature!\n");
						ret = p_info.ret;
						goto Fail;
					} else if ((p_info.ret == XA_NOT_ENOUGH_DATA) ||
							(p_info.ret != XA_ERROR_STREAM) &&
							(p_info.ret != XA_ERR_UNKNOWN)) {
						/* ...issue asynchronous zero-length buffer
						 * to output port (port-id=1)
						 */
						ret = xaf_comp_process(p_comp,
								       p_comp->outptr,
								       OUTBUF_SIZE,
								       XF_FILL_THIS_BUFFER);
						continue;
					} else {
						ret = p_info.ret;
						goto Fail;
					}
					break;
				case CODEC_MP2_DEC:
				case CODEC_MP3_DEC:
				case CODEC_BSAC_DEC:
				case CODEC_DRM_DEC:
				case CODEC_DAB_DEC:
				case CODEC_SBC_DEC:
				case CODEC_SBC_ENC:
				case CODEC_FSL_OGG_DEC:
				case CODEC_FSL_MP3_DEC:
				case CODEC_FSL_AC3_DEC:
				case CODEC_FSL_DDP_DEC:
				case CODEC_FSL_NBAMR_DEC:
				case CODEC_FSL_WBAMR_DEC:
				case CODEC_OPUS_DEC:
					if ((p_info.ret == XA_NOT_ENOUGH_DATA) ||
					    (p_info.ret != XA_ERROR_STREAM) && (p_info.ret != XA_END_OF_STREAM)) {
						/* ...issue asynchronous zero-length
						 * buffer to output port (port-id=1)
						 */
						ret = xaf_comp_process(p_comp,
								       p_comp->outptr,
								       OUTBUF_SIZE,
								       XF_FILL_THIS_BUFFER);
						continue;
					} else {
						ret = p_info.ret;
						goto Fail;
					}
					break;
				case CODEC_FSL_AAC_DEC:
					if (p_info.ret == XA_PROFILE_NOT_SUPPORT) {
						TRACE("error: aac_dec execute fatal unsupported feature!\n");
						ret = p_info.ret;
						goto Fail;
					}
					else if (p_info.ret != XA_NOT_ENOUGH_DATA && p_info.ret != XA_CAPIBILITY_CHANGE
							&& p_info.ret != XA_ERROR_STREAM && p_info.ret != XA_END_OF_STREAM) {
						ret = p_info.ret;
						goto Fail;
					}
					else if ((p_info.ret == XA_NOT_ENOUGH_DATA) || p_info.ret == XA_CAPIBILITY_CHANGE ||
					    (p_info.ret != XA_ERROR_STREAM) && (p_info.ret != XA_END_OF_STREAM)) {
						/* ...issue asynchronous zero-length
						 * buffer to output port (port-id=1)
						 */
						ret = xaf_comp_process(p_comp,
								       p_comp->outptr,
								       OUTBUF_SIZE,
								       XF_FILL_THIS_BUFFER);
						continue;
					} else {
						ret = p_info.ret;
						goto Fail;
					}
					break;
				default:
					break;
				}
			}

			if ((p_info.length <= 0) && p_pipe->input_eos) {
				p_pipe->output_eos = 1;
				xaf_pipeline_send_eos(p_pipe);
			}

			if (!p_pipe->output_eos) {
				/* ...issue asynchronous zero-length buffer
				 * to output port (port-id=1)
				 */
				ret = xaf_comp_process(p_comp,
						       p_comp->outptr,
						       OUTBUF_SIZE,
						       XF_FILL_THIS_BUFFER);
			}
		} else {
			/* ...make sure response is expected */
			if ((p_info.opcode == XF_EMPTY_THIS_BUFFER) &&
			    (p_info.buf == p_comp->inptr)) {
				TRACE("Get XF_EMPTY_THIS_BUFFER response from dsp, msg.buffer = %lx\n", p_info.buf);
				size = fread(p_comp->inptr, 1, size_read, fd_src);
				TRACE("size_read = %d\n", size);
				if (size > 0) {
					ret = xaf_comp_process(p_comp,
							       p_comp->inptr,
							       size,
							       XF_EMPTY_THIS_BUFFER);
				} else {
					if (!p_pipe->input_eos) {
						ret = xaf_comp_process(p_comp,
								       NULL,
								       0,
								       XF_EMPTY_THIS_BUFFER);
						p_pipe->input_eos = 1;
					}
				}
			} else if ((p_info.opcode == XF_EMPTY_THIS_BUFFER) &&
					!p_info.buf)
				break;
			else if (p_info.opcode == XF_OUTPUT_EOS) {
				break;
			}
		}
	} while (1);

Fail:
	return (void *)(intptr_t)ret;
}

int main(int ac, char *av[])
{
	int err = 0;
	struct AudioOption   AOption;
	struct BSAC_Block_Params  BSAC_Params;
	int size, i;
	int filelen;
	int type;

#if ENABLE_ID3
	/* ID3 tag specific declarations */
	unsigned char id3_buf[128];
	unsigned char *pub_input_ptr;
	signed int id3_v1_found = 0, id3_v1_decoded = 0;
	id3v1_struct id3v1;
	signed int cur_pos;
	signed int id3_v2_found = 0, id3_v2_complete = 0;
	id3v2_struct id3v2;
	signed int i_bytes_consumed;
	signed int i_buff_size;
	signed int i_fread_bytes;
#endif

#ifdef TIME_PROFILE
	struct timeval StartTime, EndTime;
	unsigned long TotalDecTimeUs = 0, MaxDecTimeUs = 0;
	FILE *fp_mhz;
	FILE *fp_log;

#ifdef CPUFREQ
	int cpu_freq = CPUFREQ;
#else
	int cpu_freq = 1200;
#endif

	int samples = 0;
	int Peak_frame = 0;
	double curr = 0.0, Ave = 0.0, Peak = 0.0, performance = 0.0;
	double curr_dsp = 0.0, Ave_dsp = 0.0;
	double Peak_dsp = 0.0, performance_dsp = 0.0;

	fp_mhz = fopen("./audio_performance.txt", "a+");
	if (!fp_mhz) {
		printf("Couldn't open cycles file\n");
		return 1;
	}

	fp_log = fopen("./log.txt", "a+");
	if (!fp_log) {
		printf("Couldn't open cycles log file\n");
		return 1;
	}
#endif

	struct xaf_adev_s adev;
	struct xaf_pipeline pipeline;

	pthread_t thread[2];
	void *thread_ret[2];
	struct xaf_comp component[2];

	struct xf_set_param_msg s_param;
	struct xf_get_param_msg r_param[3];

	TRACE("Hi...\n");

	//Initialize Audio Option structure
	AOption.SampleRate = -1;
	AOption.Channel = -1;
	AOption.Width = -1;
	AOption.Bitrate = -1;
	AOption.streamtype = -1;
	AOption.channel_mode = -1;
	AOption.comp_routed = 0;

	if (GetParameter(ac, av, &AOption))
		return 1;

	TRACE("Audio Format type = %d\n", AOption.AudioFormat);
	type = AOption.AudioFormat;

	TRACE("Audio Format route type = %d\n", AOption.AudioFormatRoute);

	TRACE("infile: %s\n", AOption.InFileName);
	fd_src = fopen(AOption.InFileName, "rb");
	if (!fd_src) {
		TRACE("infile: %s open failed!\n", AOption.InFileName);
		return -ENOENT;
	}
	if (!(AOption.comp_routed & COMP_RENDER)) {
		TRACE("outfile: %s\n", AOption.OutFileName);
		fd_dst = fopen(AOption.OutFileName, "wb");
		if (!fd_dst) {
			TRACE("outfile: %s open failed!\n", AOption.OutFileName);
			return -ENOENT;
		}
	}

	/* ...open proxy */
	err = xaf_adev_open(&adev);
	if (err) {
		printf("open proxy error, err = %d\n", err);
		goto Fail2;
	}

	/* ...create pipeline */
	err = xaf_pipeline_create(&adev, &pipeline);
	if (err) {
		printf("create pipeline error, err = %d\n", err);
		goto Fail1;
	}

	/* ...create component */
	err = xaf_comp_create(&adev, &component[0], type);
	if (err) {
		printf("create component failed, type = %d, err = %d\n", type, err);
		goto Fail;
	}

	/* ...create routed component */
	if (AOption.comp_routed) {
		err = xaf_comp_create(&adev,
				      &component[1],
				      AOption.AudioFormatRoute);
		if (err) {
			printf("create component failed, type = %d, err = %d\n",
						AOption.AudioFormatRoute, err);
			goto Fail;
		}
	}

	/* ...add component into pipeline */
	err = xaf_comp_add(&pipeline, &component[0]);

	/* ...add routed component into pipeline */
	if (AOption.comp_routed)
		err = xaf_comp_add(&pipeline, &component[1]);

	if (AOption.comp_routed & COMP_RENDER) {
		/* set render parameters */
		s_param.id = XA_RENDERER_CONFIG_PARAM_SAMPLE_RATE;
		s_param.mixData.value = 48000;
		err = xaf_comp_set_config(&component[1], 1, &s_param);
		if (err) {
			printf("set param[cmd:0x%x|val:0x%x] error, err = %d\n",
				s_param.id, s_param.mixData.value, err);
			goto Fail;
		}

		s_param.id = XA_RENDERER_CONFIG_PARAM_CHANNELS;
		s_param.mixData.value = 2;
		err = xaf_comp_set_config(&component[1], 1, &s_param);
		if (err) {
			printf("set param[cmd:0x%x|val:0x%x] error, err = %d\n",
				s_param.id, s_param.mixData.value, err);
			goto Fail;
		}

		s_param.id = XA_RENDERER_CONFIG_PARAM_PCM_WIDTH;
		s_param.mixData.value = 16;
		err = xaf_comp_set_config(&component[1], 1, &s_param);
		if (err) {
			printf("set param[cmd:0x%x|val:0x%x] error, err = %d\n",
				s_param.id, s_param.mixData.value, err);
			goto Fail;
		}
	}

#if ENABLE_ID3
	/* ID3V1 handling */
	{
		fseek(fd_src, -128, SEEK_END);
		fread(id3_buf, 1, 128, fd_src);

		/* search for ID3V1 */
		id3_v1_found = search_id3_v1(id3_buf + 0);
		if (id3_v1_found) {
			TRACE("ID3V1 data :\n");

			/* if ID3V1 is found, decode ID3V1 */
			decode_id3_v1(id3_buf + 3, &id3v1);
			id3_v1_decoded = 1;

			TRACE("\n");
		}
		fseek(fd_src, 0, SEEK_SET);
	}

	/* ID3V2 handling */
	{
		signed int flag = 0;
		signed int continue_flag = 0;

		i_fread_bytes = fread(component[0].inptr,
				      sizeof(char), 0x1000, fd_src);

		/* search for ID3V2 */
		if (type != CODEC_FSL_MP3_DEC) {
			id3_v2_found =
				search_id3_v2((unsigned char *)component[0].inptr);

			if (id3_v2_found) {
				TRACE("ID3V2 data :\n");
				/* initialise the max fields */
				init_id3v2_field(&id3v2);

				while (!id3_v2_complete && id3_v2_found) {
					/* if ID3V2 is found, decode ID3V2 */
					id3_v2_complete = decode_id3_v2((const char *const)component[0].inptr,
									&id3v2, continue_flag, i_fread_bytes);

					if (!id3_v2_complete) {
						continue_flag = 1;
						i_bytes_consumed = id3v2.bytes_consumed;

						if (i_bytes_consumed < i_fread_bytes)
							xa_shift_input_buffer((char *)component[0].inptr,
										  i_fread_bytes, i_bytes_consumed);

						fseek(fd_src, i_bytes_consumed, SEEK_SET);

						pub_input_ptr = (unsigned char *)component[0].inptr;

						i_fread_bytes = fread(pub_input_ptr,
									  sizeof(unsigned char), 0x1000, fd_src);
						if (i_fread_bytes <= 0) {
							TRACE("ID3 Tag Decoding: End of file reached.\n");
							flag = 1;      /* failed */
							break;
						}
						i_buff_size = i_fread_bytes;
					}
				}

				if (id3_v2_complete) {
					TRACE("\n");

					i_bytes_consumed = id3v2.bytes_consumed;
					fseek(fd_src, i_bytes_consumed, SEEK_SET);

					pub_input_ptr = (unsigned char *)component[0].inptr;

					i_fread_bytes = fread(pub_input_ptr,
								  sizeof(unsigned char), 0x1000, fd_src);
					if (i_fread_bytes <= 0) {
						TRACE("ID3V2 tag decoding: end of file reached.\n");
						flag = 1;      /* failed */
					}

					i_buff_size = i_fread_bytes;
					i_bytes_consumed = 0;
				}
				if (flag)
					goto Fail;
			}
		}
	}
#endif

#if ENABLE_BSAC_HEADER
	if (type == CODEC_BSAC_DEC) {
		/* Parse the BSAC header */
		err = App_get_mp4forbsac_header(&BSAC_Params,
					(char *)component[0].inptr,
					i_fread_bytes);
		if (err == -1) {
			TRACE("Parse the BSAC header error!\n");
			goto Fail;
		}

		xa_shift_input_buffer((char *)component[0].inptr,
				      i_fread_bytes, 16);
		i_buff_size = i_fread_bytes - 16;
		pub_input_ptr = (unsigned char *)component[0].inptr +
			i_buff_size;
		i_fread_bytes = fread(pub_input_ptr,
				      sizeof(unsigned char), 16, fd_src);
		i_fread_bytes = i_fread_bytes + i_buff_size;

		AOption.SampleRate = BSAC_Params.sampleRate;
		AOption.Channel = BSAC_Params.scalOutNumChannels;
		AOption.Width = 16;
		/* raw bsac type */
		AOption.streamtype = XA_STREAM_BSAC_RAW;
	}
#endif

	if (AOption.SampleRate > 0) {
		s_param.id = XA_SAMPLERATE;
		s_param.mixData.value = AOption.SampleRate;
		err = xaf_comp_set_config(&component[0], 1, &s_param);
		if (err) {
			printf("set param[cmd:0x%x|val:0x%x] error, err = %d\n",
						s_param.id, s_param.mixData.value, err);
			goto Fail;
		}
	}

	if (AOption.Width > 0) {
		s_param.id = XA_DEPTH;
		s_param.mixData.value = AOption.Width;
		err = xaf_comp_set_config(&component[0], 1, &s_param);
		if (err) {
			printf("set param[cmd:0x%x|val:0x%x] error, err = %d\n",
						s_param.id, s_param.mixData.value, err);
			goto Fail;
		}
	}

	if (AOption.Channel > 0) {
		s_param.id = XA_CHANNEL;
		s_param.mixData.value = AOption.Channel;
		err = xaf_comp_set_config(&component[0], 1, &s_param);
		if (err) {
			printf("set param[cmd:0x%x|val:0x%x] error, err = %d\n",
						s_param.id, s_param.mixData.value, err);
			goto Fail;
		}
	}

	if (AOption.Bitrate > 0) {
		s_param.id = XA_BITRATE;
		s_param.mixData.value = AOption.Bitrate;
		err = xaf_comp_set_config(&component[0], 1, &s_param);
		if (err) {
			printf("set param[cmd:0x%x|val:0x%x] error, err = %d\n",
						s_param.id, s_param.mixData.value, err);
			goto Fail;
		}
	}

	if (AOption.streamtype >= 0) {
		s_param.id = XA_STREAM_TYPE;
		s_param.mixData.value = AOption.streamtype;
		err = xaf_comp_set_config(&component[0], 1, &s_param);
		if (err) {
			printf("set param[cmd:0x%x|val:0x%x] error, err = %d\n",
						s_param.id, s_param.mixData.value, err);
			goto Fail;
		}
	}

	if ((AOption.channel_mode >= 0) && (type == CODEC_SBC_ENC)) {
		s_param.id = XA_SBC_ENC_CHMODE;
		s_param.mixData.value = AOption.channel_mode;
		err = xaf_comp_set_config(&component[0], 1, &s_param);
		if (err) {
			printf("set param[cmd:0x%x|val:0x%x] error, err = %d\n",
						s_param.id, s_param.mixData.value, err);
			goto Fail;
		}
	}

	/* relax the standard validity checks for mp3_dec in default */
	if (type == CODEC_MP3_DEC) {
		s_param.id = XA_MP3_DEC_NONSTD_STRM_SUPPORT;
		s_param.mixData.value = 1;
		err = xaf_comp_set_config(&component[0], 1, &s_param);
		if (err) {
			printf("set param[cmd:0x%x|val:0x%x] error, err = %d\n",
						s_param.id, s_param.mixData.value, err);
			goto Fail;
		}
	}


	if (AOption.comp_routed & COMP_CODEC) {
		/* ...issue asynchronous buffer to routed
		 * output port (port-id=1)
		 */
		err = xaf_comp_process(&component[1],
				       component[1].outptr,
				       OUTBUF_SIZE,
				       XF_FILL_THIS_BUFFER);
		if (err) {
			printf("Failed to send XF_FILL_THIS_BUFFER cmd to routed output port, err = %d\n", err);
			goto Fail;
		}
	} else if (!AOption.comp_routed) {
		/* ...issue asynchronous buffer to output port (port-id=1) */
		err = xaf_comp_process(&component[0],
				       component[0].outptr,
				       OUTBUF_SIZE,
				       XF_FILL_THIS_BUFFER);
		if (err) {
			printf("Failed to send XF_FILL_THIS_BUFFER cmd to output port, err = %d\n", err);
			goto Fail;
		}
	}

	/* ...send input data to dsp */
	size = i_fread_bytes;
	err = xaf_comp_process(&component[0],
			       component[0].inptr,
			       size,
			       XF_EMPTY_THIS_BUFFER);
	if (err) {
		printf("Failed to send XF_EMPTY_THIS_BUFFER cmd to input port, err = %d\n", err);
		goto Fail;
	}

	/* ...connect component when routed */
	if (AOption.comp_routed) {
		err = xaf_connect(&component[0],
				  &component[1],
				  1,
				  OUTBUF_SIZE);
		if (err) {
			printf("Failed to connect component, err = %d\n", err);
			goto Fail;
		}
	}

	/* ...create thread to process component[0] message */
	pthread_create(&thread[0], 0,
		       comp_process_entry, &component[0]);

	/* ...create thread to process component[1] message */
	if (AOption.comp_routed & COMP_CODEC)
		pthread_create(&thread[1], 0,
			       comp_process_entry, &component[1]);

	/* ...wait component[0] thread end */
	pthread_join(thread[0], &thread_ret[0]);

	/* ...wait component[1] thread end */
	if (AOption.comp_routed & COMP_CODEC)
		pthread_join(thread[1], &thread_ret[1]);

	/* ...judge the return value of thread */
	if ((int)(intptr_t)(thread_ret[0]) == -ETIMEDOUT) {
		printf("thread response timeout\n");
		return -ETIMEDOUT;
	}
	if (AOption.comp_routed & COMP_CODEC) {
		if ((int)(intptr_t)(thread_ret[1]) == -ETIMEDOUT) {
			printf("thread response timeout\n");
			return -ETIMEDOUT;
		}
	}

	/* ...get paramter from component */
	r_param[0].id = XA_SAMPLERATE;
	r_param[1].id = XA_CHANNEL;
	r_param[2].id = XA_DEPTH;

	if (AOption.comp_routed) {
		err = xaf_comp_get_config(&component[1], 3, &r_param[0]);
		if (err) {
			printf("Fail to get routed component parameter, err = %d\n", err);
			goto Fail;
		}
	} else {
		err = xaf_comp_get_config(&component[0], 3, &r_param[0]);
		if (err) {
			printf("Fail to get component parameter, err = %d\n", err);
			goto Fail;
		}
	}
	TRACE("prop: samplerate = %d, channel = %d, width = %d\n",
	      r_param[0].mixData.value, r_param[1].mixData.value, r_param[2].mixData.value);

	/* ...disconnect component when routed */
	if (AOption.comp_routed) {
		err = xaf_disconnect(&component[0]);
		if (err) {
			printf("Fail to disconnect component, err = %d\n", err);
			goto Fail;
		}
	}

Fail:
	xaf_comp_delete(&component[0]);
	if (AOption.comp_routed)
		xaf_comp_delete(&component[1]);

Fail1:
	xaf_pipeline_delete(&pipeline);

Fail2:
	xaf_adev_close(&adev);

	if (fd_src)
		fclose(fd_src);
	if (fd_dst)
		fclose(fd_dst);

	return 0;
}
