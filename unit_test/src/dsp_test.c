//*****************************************************************
// Copyright 2018 NXP
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//*****************************************************************

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <math.h>
#include <string.h>
#include <malloc.h>
#include <sys/time.h>
#include <pthread.h>
#include <linux/types.h>
#include "mxc_hifi4.h"
#include "id3_tag_decode.h"
#include "common.h"


#define ENABLE_ID3  1
#define ENABLE_BSAC_HEADER  1

typedef int WORD32;


typedef struct
{
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
}AudioOption;

void help_info(int ac, char *av[])
{
	printf("\n\n**************************************************\n");
	printf("* Test aplication for HIFI\n");
	printf("* Options : \n\n");
	printf("          -f AudFormat  Audio Format(1-8)             \n");
	printf("                        MP3        for 1              \n");
	printf("                        AAC        for 2              \n");
	printf("                        DAB        for 3              \n");
	printf("                        MP2        for 4              \n");
	printf("                        BSAC       for 5              \n");
	printf("                        DRM        for 6              \n");
	printf("                        SBCDEC     for 7              \n");
	printf("                        SBCENC     for 8              \n");
	printf("          -i InFileNam  Input File Name               \n");
	printf("          -o OutName    Output File Name              \n");
	printf("          -s Samplerate Sampling Rate of Audio        \n");
	printf("          -n Channel    Channel Number of Audio       \n");
	printf("          -d Width      The Width of Samples          \n");
	printf("          -r bitRate    Bit Rate                      \n");
	printf("          -t StreamType Stream Type                   \n");
	printf("                        0(STREAM_UNKNOWN)             \n");
	printf("                        1(STREAM_ADTS)                \n");
	printf("                        2(STREAM_ADIF)                \n");
	printf("                        3(STREAM_RAW)                 \n");
	printf("                        4(STREAM_LATM)                \n");
	printf("                        5(STREAM_LATM_OUTOFBAND_CONFIG)\n");
	printf("                        6(STREAM_LOAS)                \n");
	printf("                        48(STREAM_DABPLUS_RAW_SIDEINFO)\n");
	printf("                        49(STREAM_DABPLUS)             \n");
	printf("                        50(STREAM_BSAC_RAW)            \n");
	printf("          -u Channel modes only for SBC_ENC           \n");
	printf("                        0(CHMODE_MONO)                \n");
	printf("                        1(CHMODE_DUAL)                \n");
	printf("                        2(CHMODE_STEREO)              \n");
	printf("                        3(CHMODE_JOINT)               \n");
	printf("**************************************************\n\n");
}

static void
write16_bits_lh(FILE *fp, WORD32 i)
{
	putc(i & 0xff, fp);
	putc((i >> 8) & 0xff, fp);
}

static void
write32_bits_lh(FILE *fp, WORD32 i)
{
	write16_bits_lh(fp, (WORD32)(i & 0xffffL));
	write16_bits_lh(fp, (WORD32)((i >> 16) & 0xffffL));
}

WORD32
write_wav_header (FILE *fp, /* file to write */
		WORD32 pcmbytes, /* total bytes in the wav file */
		WORD32 freq, /* sample rate */
		WORD32 channels, /* output channels */
		WORD32 bits /* bits per sample */)
{
	WORD32 bytes = (bits + 7) / 8;
	fwrite("RIFF", 1, 4, fp); /* label */
	write32_bits_lh(fp, pcmbytes + 44 - 8); /* length in bytes without header */
	fwrite("WAVEfmt ", 1, 8, fp); /* 2 labels */
	write32_bits_lh(fp, 2 + 2 + 4 + 4 + 2 + 2); /* length of PCM format decl area */
	write16_bits_lh(fp, 1); /* is pcm? */
	write16_bits_lh(fp, channels);
	write32_bits_lh(fp, freq);
	write32_bits_lh(fp, freq * channels * bytes); /* bps */
	write16_bits_lh(fp, channels * bytes);
	write16_bits_lh(fp, bits);
	fwrite("data", 1, 4, fp);
	write32_bits_lh(fp, pcmbytes);

	return (ferror(fp) ? -1 : 0);
}

int GetParameter(int argc_t, char *argv_t[], AudioOption *pAOption)
{
	int i;
	char *pTmp = NULL, in[256];
	if(argc_t < 3)
	{
		help_info(argc_t, argv_t);
		goto Error;
	}
	for(i = 1; i < argc_t; i++)
	{
		pTmp = argv_t[i];
		if(*pTmp == '-')
		{
			strcpy(in, pTmp+2);
			switch(pTmp[1])
			{
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
					pAOption->InFileName = pTmp+2;
					break;
				case 'o':
					pAOption->OutFileName = pTmp+2;
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

int main(int ac, char *av[])
{
	FILE *fd_dst = NULL;
	FILE *fd_src = NULL;
	FILE *fd_wav = NULL;
	int fd_hifi;
	int *input_buffer = NULL;
	int *output_buffer = NULL;
	int err = 0;
	struct decode_info   decode_info;
	struct prop_info     prop_info;
	struct binary_info   binary_info;
	struct binary_info   binary_info_wrap;
	struct prop_config   config;
	AudioOption   AOption;
	BSAC_Block_Params  BSAC_Params;
	int size_read;
	int size;
	int filelen;
	int type = CODEC_MP3_DEC;
	unsigned int frame_count = 0;
	unsigned int process_id = 0;

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
	unsigned long TotalDecTimeUs=0, MaxDecTimeUs=0;
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
	double curr_dsp = 0.0, Ave_dsp = 0.0, Peak_dsp = 0.0, performance_dsp = 0.0;

	fp_mhz = fopen("./audio_performance.txt","a+");
	if(fp_mhz == NULL)
	{
		printf("Couldn't open cycles file\n");
		return 1;
	}

	fp_log = fopen("./log.txt","a+");
	if(fp_log == NULL)
	{
		printf("Couldn't open cycles log file\n");
		return 1;
	}
#endif

	printf("Hi... \n");

	//Initialize Audio Option structure
	AOption.SampleRate = -1;
	AOption.Channel = -1;
	AOption.Width = -1;
	AOption.Bitrate = -1;
	AOption.streamtype = -1;
	AOption.channel_mode = -1;

	if(GetParameter(ac, av, &AOption))
	{
		return 1;
	}
	printf("Audio Format type = %d\n", AOption.AudioFormat);
	type = AOption.AudioFormat;

	fd_hifi = open("/dev/mxc_hifi4", O_RDWR);
	if (fd_hifi < 0)
	{
		printf("Unable to open device\n");
		return 1;
	}


	err = ioctl(fd_hifi, HIFI4_CLIENT_REGISTER, &process_id);
	if(err) {
		printf("no valiable client in dsp driver\n");
		goto Fail2;
	}

	binary_info.process_id = process_id;
	binary_info.lib_type = HIFI_CODEC_LIB;

	if (type == CODEC_MP3_DEC) {
		binary_info.type = CODEC_MP3_DEC;
		binary_info.file = "/usr/lib/imx-mm/audio-codec/hifi/lib_dsp_mp3_dec.so";
	} else if (type == CODEC_AAC_DEC) {
		binary_info.type = CODEC_AAC_DEC;
		binary_info.file = "/usr/lib/imx-mm/audio-codec/hifi/lib_dsp_aac_dec.so";
	} else if (type == CODEC_BSAC_DEC) {
		binary_info.type = CODEC_BSAC_DEC;
		binary_info.file = "/usr/lib/imx-mm/audio-codec/hifi/lib_dsp_bsac_dec.so";
	} else if (type == CODEC_DAB_DEC) {
		binary_info.type = CODEC_DAB_DEC;
		binary_info.file = "/usr/lib/imx-mm/audio-codec/hifi/lib_dsp_dabplus_dec.so";
	} else if (type == CODEC_MP2_DEC) {
		binary_info.type = CODEC_MP2_DEC;
		binary_info.file = "/usr/lib/imx-mm/audio-codec/hifi/lib_dsp_mp2_dec.so";
	} else if (type == CODEC_DRM_DEC) {
		binary_info.type = CODEC_DRM_DEC;
		binary_info.file = "/usr/lib/imx-mm/audio-codec/hifi/lib_dsp_drm_dec.so";
	} else if (type == CODEC_SBC_DEC) {
		binary_info.type = CODEC_SBC_DEC;
		binary_info.file = "/usr/lib/imx-mm/audio-codec/hifi/lib_dsp_sbc_dec.so";
	} else if (type == CODEC_SBC_ENC) {
		binary_info.type = CODEC_SBC_ENC;
		binary_info.file = "/usr/lib/imx-mm/audio-codec/hifi/lib_dsp_sbc_enc.so";
	}

	binary_info_wrap.type = binary_info.type;
	binary_info_wrap.file = "/usr/lib/imx-mm/audio-codec/hifi/lib_dsp_codec_wrap.so";
	binary_info_wrap.process_id = process_id;
	binary_info_wrap.lib_type = HIFI_CODEC_WRAP_LIB;

	printf("infile: %s\n", AOption.InFileName);
	fd_src = fopen(AOption.InFileName, "rb");
	if(fd_src == NULL)
	{
		printf("infile: %s open failed!\n", AOption.InFileName);
		goto Fail1;
	}
	printf("outfile: %s\n", AOption.OutFileName);
	fd_dst = fopen(AOption.OutFileName, "wb");
	if(fd_dst == NULL)
	{
		printf("outfile: %s open failed!\n", AOption.OutFileName);
		goto Fail1;
	}

	input_buffer  = malloc(4096);
	output_buffer = malloc(16384);
	printf("malloc is ok in input & output buffer\n");

	err = ioctl(fd_hifi, HIFI4_LOAD_CODEC, &binary_info_wrap);
	if(err)
	{
		printf("LOAD_CODEC Wrapper Failed\n");
		goto Fail1;
	}

	err = ioctl(fd_hifi, HIFI4_LOAD_CODEC, &binary_info);
	if(err)
	{
		printf("LOAD_CODEC Failed\n");
		goto Fail1;
	}

	err = ioctl(fd_hifi, HIFI4_INIT_CODEC, &process_id);
	if(err)
	{
		printf("INIT_CODEC Failed\n");
		goto Fail1;
	}

	memset(&config, 0, sizeof(struct prop_config));
	config.codec_id = type;
	config.process_id = process_id;

#if ENABLE_ID3
	/* ID3V1 handling */
	{
		fseek(fd_src, -128, SEEK_END);
		fread(id3_buf, 1, 128, fd_src);

		/* search for ID3V1 */
		id3_v1_found = search_id3_v1(id3_buf + 0);
		if (id3_v1_found)
		{
			printf("ID3V1 data : \n");

			/* if ID3V1 is found, decode ID3V1 */
			decode_id3_v1(id3_buf + 3, &id3v1);
			id3_v1_decoded = 1;

			printf("\n");
		}
		fseek(fd_src, 0, SEEK_SET);
	}

	/* ID3V2 handling */
	{
		signed int flag = 0;
		signed int continue_flag = 0;

		i_fread_bytes = fread(input_buffer, sizeof(char), 0x1000, fd_src);

		/* search for ID3V2 */
		id3_v2_found = search_id3_v2((unsigned char *)input_buffer);

		if (id3_v2_found)
		{
			printf("ID3V2 data : \n");
			/* initialise the max fields */
			init_id3v2_field(&id3v2);

			while (!id3_v2_complete && id3_v2_found)
			{
				/* if ID3V2 is found, decode ID3V2 */
				id3_v2_complete = decode_id3_v2((const char *const)input_buffer, &id3v2, continue_flag, i_fread_bytes);

				if (!id3_v2_complete)
				{
					continue_flag = 1;
					i_bytes_consumed = id3v2.bytes_consumed;

					if (i_bytes_consumed < i_fread_bytes)
						xa_shift_input_buffer((char *)input_buffer, i_fread_bytes, i_bytes_consumed);

					fseek(fd_src, i_bytes_consumed, SEEK_SET);

					pub_input_ptr = (unsigned char *)input_buffer;

					if((i_fread_bytes = fread(pub_input_ptr, sizeof(unsigned char), 0x1000, fd_src)) <= 0)
					{
						printf("ID3 Tag Decoding: End of file reached.\n");
						flag = 1;      /* failed */
						break;
					}
					i_buff_size = i_fread_bytes;
				}
			}

			if (id3_v2_complete)
			{
				printf("\n");

				i_bytes_consumed = id3v2.bytes_consumed;
				fseek(fd_src, i_bytes_consumed, SEEK_SET);

				pub_input_ptr = (unsigned char *)input_buffer;

				if((i_fread_bytes = fread(pub_input_ptr, sizeof(unsigned char), 0x1000, fd_src)) <= 0)
				{
					printf("ID3V2 tag decoding: end of file reached.\n");
					flag = 1;      /* failed */
				}

				i_buff_size = i_fread_bytes;
				i_bytes_consumed = 0;
			}
			if(flag)
			    goto Fail;
		}
	}
#endif

#if ENABLE_BSAC_HEADER
	if(type == CODEC_BSAC_DEC)
	{
		/* Parse the BSAC header */
		err = App_get_mp4forbsac_header(&BSAC_Params, (char *)input_buffer, i_fread_bytes);
		if(err == -1)
		{
			printf("Parse the BSAC header error!\n");
			goto Fail;
		}

		xa_shift_input_buffer((char *)input_buffer, i_fread_bytes, 16);
		i_buff_size = i_fread_bytes - 16;
		pub_input_ptr = (unsigned char *)input_buffer + i_buff_size;
		i_fread_bytes = fread(pub_input_ptr, sizeof(unsigned char), 16, fd_src);
		i_fread_bytes = i_fread_bytes + i_buff_size;

		AOption.SampleRate = BSAC_Params.sampleRate;
		AOption.Channel = BSAC_Params.scalOutNumChannels;
		AOption.Width = 16;
		AOption.streamtype = XA_STREAM_BSAC_RAW;         /* raw bsac type */
	}
#endif

	if(AOption.SampleRate > 0)
	{
		config.cmd = XA_SAMPLERATE;   //SAMPLINGRATE
		config.val = AOption.SampleRate;
		err = ioctl(fd_hifi, HIFI4_SET_CONFIG, &config);
	}

	if(AOption.Width > 0)
	{
		config.cmd = XA_DEPTH;    //PCM_WDSZ
		config.val = AOption.Width;
		err = ioctl(fd_hifi, HIFI4_SET_CONFIG, &config);
	}

	if(AOption.Channel > 0)
	{
		config.cmd = XA_CHANNEL;    //NUM_CHANNELS
		config.val = AOption.Channel;
		err = ioctl(fd_hifi, HIFI4_SET_CONFIG, &config);
	}

	if(AOption.Bitrate > 0)
	{
		config.cmd = XA_BITRATE;    //EXTERNALBITRATE
		config.val = AOption.Bitrate;
		err = ioctl(fd_hifi, HIFI4_SET_CONFIG, &config);
	}

	if(AOption.streamtype >= 0)
	{
		config.cmd = XA_STREAM_TYPE;    //EXTERNALBSFORMAT
		config.val = AOption.streamtype;
		err = ioctl(fd_hifi, HIFI4_SET_CONFIG, &config);
	}

	if((AOption.channel_mode >= 0) && (type == CODEC_SBC_ENC))
	{
		config.cmd = XA_SBC_ENC_CHMODE;   //CHMODE
		config.val = AOption.channel_mode;
		err = ioctl(fd_hifi, HIFI4_SET_CONFIG, &config);
	}

	/* relax the standard validity checks for mp3_dec in default */
	if(type == CODEC_MP3_DEC)
	{
		config.cmd = XA_MP3_DEC_NONSTD_STRM_SUPPORT;           //NONSTD_STRM_SUPPORT
		config.val = 1;
		err = ioctl(fd_hifi, HIFI4_SET_CONFIG, &config);
	}

	/* malloc buffer for dsp core lib and set parameters */
	err = ioctl(fd_hifi, HIFI4_CODEC_OPEN, &process_id);
	if(err)
	{
		printf("INIT_CODEC Failed\n");
		goto Fail;
	}

	memset(output_buffer, 0, 16384);

	size_read = 0x1000;

//	size = fread(input_buffer, 1, size_read, fd_src);
	size = i_fread_bytes;

	decode_info.in_buf_off = 0;
	decode_info.input_over = 0;
	decode_info.process_id = process_id;

	do {
		decode_info.in_buf_addr = input_buffer;
		decode_info.in_buf_size = size;
		decode_info.out_buf_addr = output_buffer;
		decode_info.out_buf_size = 16384;
		decode_info.out_buf_off = 0;

#ifdef TIME_PROFILE
		gettimeofday(&StartTime, NULL);
#endif
		err = ioctl(fd_hifi, HIFI4_DECODE_ONE_FRAME, &decode_info);
#ifdef TIME_PROFILE
		gettimeofday(&EndTime, NULL);
#endif

		fwrite(output_buffer, 1, decode_info.out_buf_off, fd_dst);

		if (decode_info.in_buf_off == decode_info.in_buf_size) {
			size = fread(input_buffer, 1, size_read, fd_src);
			decode_info.in_buf_off = 0;

			if(size == 0)
				decode_info.input_over = 1;
		}

		if(decode_info.out_buf_off > 0)
		{
			frame_count++;
			printf("loop count = %d\n", frame_count);

#ifdef TIME_PROFILE
			MaxDecTimeUs = (EndTime.tv_usec - StartTime.tv_usec) + (EndTime.tv_sec - StartTime.tv_sec)*1000000L;
			TotalDecTimeUs += MaxDecTimeUs;

			prop_info.process_id = process_id;
			err = ioctl(fd_hifi, HIFI4_GET_PCM_PROP, &prop_info);
			if(type == CODEC_SBC_ENC)
				samples = prop_info.consumed_bytes / (prop_info.channels * (prop_info.bits / 8));
			else
				samples = decode_info.out_buf_off / (prop_info.channels * (prop_info.bits / 8));

			curr = (double)MaxDecTimeUs * 0.000001 * cpu_freq * prop_info.samplerate/samples;
			curr_dsp = prop_info.cycles * 0.000001 * prop_info.samplerate/samples;
			performance += curr;
			performance_dsp += curr_dsp;
			Ave = performance / frame_count;
			Ave_dsp = performance_dsp / frame_count;

			if(Peak < curr)
			{
				Peak = curr;
				Peak_frame = frame_count;
			}
			if(Peak_dsp < curr_dsp)
			{
				Peak_dsp = curr_dsp;
			}

			fprintf(fp_log,"[A35]: Frame Count: %d Sample Rate: %d Samples: %d MCPS: %.2f Average: %.2f Peak: %.2f @[%d]\n", frame_count, prop_info.samplerate, samples, curr, Ave, Peak, Peak_frame);
			fprintf(fp_log,"[DSP]: Frame Count: %d Sample Rate: %d Samples: %d MCPS: %.2f Average: %.2f Peak: %.2f @[%d]\n", frame_count, prop_info.samplerate, samples,curr_dsp, Ave_dsp, Peak_dsp, Peak_frame);
#endif
		}

		if(err != XA_SUCCESS)
		{
			printf("error occur during decoding, err = 0x%x\n", err);

			switch(type)
			{
				case CODEC_AAC_DEC:
					if(err == XA_PROFILE_NOT_SUPPORT)
					{
						printf("error: aac_dec execute fatal unsupported feature!\n");
						goto Fail;
					}
					else if(err == XA_NOT_ENOUGH_DATA)
					{
						printf("error: aac_dec insufficient frame data\n");
						continue;
					}
					else
					{
						if(err == XA_ERROR_STREAM)
							goto Fail;
						else
							continue;
					}
					break;
				case CODEC_MP2_DEC:
				case CODEC_MP3_DEC:
					if(err == XA_NOT_ENOUGH_DATA)
					{
						printf("error: mp2_dec or mp3_dec insufficient frame data\n");
						continue;
					}
					else
					{
						if(err == XA_ERROR_STREAM)
							goto Fail;
						else
							continue;
					}
					break;
				case CODEC_BSAC_DEC:
					if(err == XA_NOT_ENOUGH_DATA)
					{
						printf("error: bsac_dec insufficient frame data\n");
						continue;
					}
					else
					{
						if(err == XA_ERROR_STREAM)
							goto Fail;
						else
							continue;
					}
					break;
				case CODEC_DRM_DEC:
					if(err == XA_NOT_ENOUGH_DATA)
					{
						printf("error: drm_dec insufficient frame data\n");
						continue;
					}
					else
					{
						if(err == XA_ERROR_STREAM)
							goto Fail;
						else
							continue;
					}
					break;
				case CODEC_DAB_DEC:
					if(err == XA_NOT_ENOUGH_DATA)
					{
						printf("error: dabplus_dec insufficient frame data\n");
						continue;
					}
					else
					{
						if(err == XA_ERROR_STREAM)
							goto Fail;
						else
							continue;
					}
					break;
				case CODEC_SBC_DEC:
					if(err == XA_NOT_ENOUGH_DATA)
					{
						printf("error: sbc_dec insufficient frame data\n");
						continue;
					}
					else
					{
						if(err == XA_ERROR_STREAM)
							goto Fail;
						else
							continue;
					}
					break;
				case CODEC_SBC_ENC:
					if(err == XA_NOT_ENOUGH_DATA)
					{
						printf("error: sbc_enc insufficient frame data\n");
						continue;
					}
					else
					{
						if(err == XA_ERROR_STREAM)
							goto Fail;
						else
							continue;
					}
					break;
				default:
					break;
			}
		}
		if((size <= 0) && (decode_info.out_buf_off <= 0))
			break;

	} while (1);

	prop_info.process_id = process_id;
	err = ioctl(fd_hifi, HIFI4_GET_PCM_PROP, &prop_info);

	printf("prop %d, %d, %d\n", prop_info.samplerate, prop_info.channels, prop_info.bits);

//	fclose(fd_src);
//	fclose(fd_dst);

#ifdef TIME_PROFILE
	fprintf(fp_mhz,"\n|\tPlat\t|\tAudio Format\t|\tPlat\t|\tSamplerate\t|\tCh\t|\tBiterate\t|\tBit\t|\tPerf(MIPS)\t|\tAudio file name\n");
	fprintf(fp_mhz, "|\t[A35]: \t|\tDSP Decoder \t|\tARM%d\t|\t%9d\t|\t%d\t|\t%8d\t|\t%d\t|\t%.8f\t|%s\n", 8, prop_info.samplerate, prop_info.channels, 0, prop_info.bits, Ave, AOption.InFileName);
	fprintf(fp_mhz, "|\t[DSP]: \t|\tDSP Decoder \t|\tARM%d\t|\t%9d\t|\t%d\t|\t%8d\t|\t%d\t|\t%.8f\t|%s\n", 8, prop_info.samplerate, prop_info.channels, 0, prop_info.bits, Ave_dsp, AOption.InFileName);

	fclose(fp_mhz);
	fclose(fp_log);
#endif

	fd_wav = fopen("./out.wav", "wb");
//	fd_dst = fopen(AOption.OutFileName, "rb");
	fseek(fd_dst, 0, SEEK_END);
	filelen = ftell(fd_dst);
	fseek(fd_dst, 0, SEEK_SET);
	write_wav_header(fd_wav, filelen, prop_info.samplerate, prop_info.channels, prop_info.bits);

	while ((size = fread(input_buffer, 1, 4096, fd_dst)) > 0 )
		fwrite(input_buffer, 1, size, fd_wav);


	fclose(fd_wav);

Fail:
	err = ioctl(fd_hifi, HIFI4_CODEC_CLOSE, &process_id);
Fail1:
	err = ioctl(fd_hifi, HIFI4_CLIENT_UNREGISTER, &process_id);
Fail2:
	if(fd_src)
		fclose(fd_src);
	if(fd_dst)
		fclose(fd_dst);

	close(fd_hifi);

	if(input_buffer)
		free(input_buffer);
	if(output_buffer)
		free(output_buffer);

	return 0;
}
