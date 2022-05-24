#include <stdio.h>
#include <alsa/asoundlib.h>
#include "xaf-utils-test.h"

#define COMPOSE_ID(a,b,c,d)	((a) | ((b)<<8) | ((c)<<16) | ((d)<<24))

#define WAV_RIFF		COMPOSE_ID('R','I','F','F')
#define WAV_RIFX		COMPOSE_ID('R','I','F','X')
#define WAV_WAVE		COMPOSE_ID('W','A','V','E')
#define WAV_FMT			COMPOSE_ID('f','m','t',' ')
#define WAV_DATA		COMPOSE_ID('d','a','t','a')

/* WAVE fmt block constants from Microsoft mmreg.h header */
#define WAV_FMT_PCM             0x0001
#define WAV_FMT_IEEE_FLOAT      0x0003
#define WAV_FMT_DOLBY_AC3_SPDIF 0x0092
#define WAV_FMT_EXTENSIBLE      0xfffe

/* Used with WAV_FMT_EXTENSIBLE format */
#define WAV_GUID_TAG		"\x00\x00\x00\x00\x10\x00\x80\x00\x00\xAA\x00\x38\x9B\x71"

typedef struct {
	u_int magic;		/* 'RIFF' */
	u_int length;		/* filelen */
	u_int type;		/* 'WAVE' */
} WaveHeader;

typedef struct {
	u_short format;		/* see WAV_FMT_* */
	u_short channels;
	u_int sample_fq;	/* frequence of sample */
	u_int byte_p_sec;
	u_short byte_p_spl;	/* samplesize; 1 or 2 bytes */
	u_short bit_p_spl;	/* 8, 12 or 16 bit */
} WaveFmtBody;

typedef struct {
	WaveFmtBody format;
	u_short ext_size;
	u_short bit_p_spl;
	u_int channel_mask;
	u_short guid_format;	/* WAV_FMT_* */
	u_char guid_tag[14];	/* WAV_GUID_TAG */
} WaveFmtExtensibleBody;

typedef struct {
	u_int type;		/* 'data' */
	u_int length;		/* samplecount */
} WaveChunkHeader;

static int parse_wav_header(FILE *file, unsigned int *num_channels, unsigned int *sample_rate,
		      unsigned int *format, unsigned int *data_length) {
	WaveHeader wave_header;
	WaveChunkHeader chunk_header;
	WaveFmtBody fmt_body;
	int more_chunks = 1;
	int ret;

	ret = fread(&wave_header, sizeof(WaveHeader), 1, file);
	if ((wave_header.magic != WAV_RIFF) ||
	    (wave_header.type != WAV_WAVE)) {
		fprintf(stderr, "Error: it is not a riff/wave file\n");
		return -1;
	}

	do {
		ret = fread(&chunk_header, sizeof(WaveChunkHeader), 1, file);
		switch (chunk_header.type) {
		case WAV_FMT:
			ret = fread(&fmt_body, sizeof(WaveFmtBody), 1, file);
			/* If the format header is larger, skip the rest */
			if (chunk_header.length > sizeof(WaveFmtBody))
				fseek(file, chunk_header.length - sizeof(WaveFmtBody), SEEK_CUR);

			*num_channels = fmt_body.channels;
			*sample_rate = fmt_body.sample_fq;

			switch (fmt_body.bit_p_spl) {
			case 8:
				*format = SND_PCM_FORMAT_U8;
				break;
			case 16:
				*format = SND_PCM_FORMAT_S16_LE;
				break;
			case 24:
				switch (fmt_body.byte_p_spl / fmt_body.channels) {
				case 3:
					*format = SND_PCM_FORMAT_S24_3LE;
					break;
				case 4:
					*format = SND_PCM_FORMAT_S24_LE;
					break;
				default:
					fprintf(stderr, "format error\n");
					return -1;
				}
				break;
			case 32:
				if (fmt_body.format == WAV_FMT_PCM) {
					*format = SND_PCM_FORMAT_S32_LE;
				} else if (fmt_body.format == WAV_FMT_IEEE_FLOAT) {
					*format = SND_PCM_FORMAT_FLOAT_LE;
				}
				break;
			default:
				fprintf(stderr, "format error\n");
				return -1;
			}
			break;
		case WAV_DATA:
			/* Stop looking for chunks */
			more_chunks = 0;
			*data_length = chunk_header.length;
			break;
		default:
			/* Unknown chunk, skip bytes */
			fseek(file, chunk_header.length, SEEK_CUR);
		}
	} while (more_chunks);

	return 0;
}

int get_codec_pcm(FILE *file, xaf_format_t *dec_format)
{
	unsigned int channels, rate, format;
	int size, data_length, offset;

	fseek(file, 0L, SEEK_END);
	size = ftell(file);
	fseek(file, 0L, SEEK_SET);

	if (parse_wav_header(file, &channels, &rate, &format, &data_length) == -1) {
		fclose(file);
		return -1;
	}

	offset = ftell(file);
	if (data_length + offset != size)
		return -1;

	dec_format->channels = channels;
	dec_format->sample_rate = rate;
	if (format != SND_PCM_FORMAT_S16_LE) {
		fprintf(stderr, "unsupport pcm format %d\n", format);
		fclose(file);
		return -1;
	}
	return 0;
}
