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

#ifndef _DSP_WRAP_H_
#define _DSP_WRAP_H_

#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
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
#include <malloc.h>
#include <sys/time.h>
#include <pthread.h>
#include <linux/types.h>

#include "library_load.h"
#include "xf-types.h"
#include "xaf-api.h"
#include "xaf-mem.h"
#include "osal-msgq.h"
#include "xa_error_standards.h"

#include "fsl_unia.h"
#include "dsp_codec_interface.h"

/* ...size of auxiliary pool for communication with DSP */
#define XA_AUX_POOL_SIZE                32

/* ...length of auxiliary pool messages */
#define XA_AUX_POOL_MSG_LENGTH          128

/* ...number of max input buffers */
#define INBUF_SIZE                      4096
/* Enlarge outbuf size for receive wma10 output */
#define OUTBUF_SIZE                     8192*3*8*2
/* size of parameter buffer for pass complex parameter */
#define PARAM_SIZE                      120

#define N_ELEMENTS(arr)	(sizeof(arr) / sizeof((arr)[0]))

#define _STR(s)     #s
#define STR(s)      _STR(s)

/* Used to malloc inner buffer in create() function*/
#define AACD_6CH_FRAME_MAXLEN            (6 * 768)
#define MP3D_INPUT_BUF_PUSH_SIZE         (2048 * 4)

#define MP3D_FRAME_SIZE                  576
#define AAC_FRAME_SIZE                   1024

/* ANSI color print */
#define COLOR_RED       31
#define COLOR_GREEN     32
#define COLOR_YELLOW    33
#define COLOR_BLUE      34
#define COLOR_PURPLE    35
#define COLOR_CYAN      36

#define COLORFUL_STR(color, format, ...)\
	"\33[1;" STR(color) "m" format "\33[0m", ##__VA_ARGS__

#define YELLOW_STR(format, ...) COLORFUL_STR(COLOR_YELLOW, format, ##__VA_ARGS__)
#define RED_STR(format, ...)    COLORFUL_STR(COLOR_RED, format, ##__VA_ARGS__)
#define GREEN_STR(format, ...)  COLORFUL_STR(COLOR_GREEN, format, ##__VA_ARGS__)
#define BLUE_STR(format, ...)   COLORFUL_STR(COLOR_BLUE, format, ##__VA_ARGS__)
#define PURPLE_STR(format, ...) COLORFUL_STR(COLOR_PURPLE, format, ##__VA_ARGS__)
#define CYAN_STR(format, ...)   COLORFUL_STR(COLOR_CYAN, format, ##__VA_ARGS__)

static UWORD32 aacd_1channel_layout[] = {
	/* FC */
	UA_CHANNEL_FRONT_CENTER,
};

static UWORD32 aacd_2channel_layout[] = {
	/* FL,FR */
	UA_CHANNEL_FRONT_LEFT,
	UA_CHANNEL_FRONT_RIGHT,
};

static UWORD32 aacd_3channel_layout[] = {
	/* FC,FL,FR */
	UA_CHANNEL_FRONT_CENTER,
	UA_CHANNEL_FRONT_LEFT,
	UA_CHANNEL_FRONT_RIGHT,
};

static UWORD32 aacd_4channel_layout[] = {
	/* FC,FCL,FCR,BC */
	UA_CHANNEL_FRONT_CENTER,
	UA_CHANNEL_FRONT_LEFT_CENTER,
	UA_CHANNEL_FRONT_RIGHT_CENTER,
	UA_CHANNEL_REAR_CENTER
};

static UWORD32 aacd_5channel_layout[] = {
	/* FC,FL,FR,BL,BR */
	UA_CHANNEL_FRONT_CENTER,
	UA_CHANNEL_FRONT_LEFT,
	UA_CHANNEL_FRONT_RIGHT,
	UA_CHANNEL_REAR_LEFT,
	UA_CHANNEL_REAR_RIGHT
};

static UWORD32 aacd_6channel_layout[] = {
	/* FC,FL,FR,BL,BR,LFE */
	UA_CHANNEL_FRONT_CENTER,
	UA_CHANNEL_FRONT_LEFT,
	UA_CHANNEL_FRONT_RIGHT,
	UA_CHANNEL_REAR_LEFT,
	UA_CHANNEL_REAR_RIGHT,
	UA_CHANNEL_LFE
};

static UWORD32 aacd_8channel_layout[] = {
	/* FC,FCL,FCR,SL,SR,BL,BR,LFE */
	UA_CHANNEL_FRONT_CENTER,
	UA_CHANNEL_FRONT_LEFT_CENTER,
	UA_CHANNEL_FRONT_RIGHT_CENTER,
	UA_CHANNEL_SIDE_LEFT,
	UA_CHANNEL_SIDE_RIGHT,
	UA_CHANNEL_REAR_LEFT,
	UA_CHANNEL_REAR_RIGHT,
	UA_CHANNEL_LFE
};

static UWORD32 *aacd_channel_layouts[] = {
	NULL,
	aacd_1channel_layout, // 1
	aacd_2channel_layout, // 2
	aacd_3channel_layout,
	aacd_4channel_layout,
	aacd_5channel_layout,
	aacd_6channel_layout,
	NULL,
	aacd_8channel_layout,
};

/* wma channel layouts */
static UWORD32 wma10d_1channel_layout[] = {
  /* FC */
    UA_CHANNEL_FRONT_CENTER,
};

static UWORD32 wma10d_2channel_layout[] = {
  /* FL,FR */
  UA_CHANNEL_FRONT_LEFT,
  UA_CHANNEL_FRONT_RIGHT,
};

static UWORD32 wma10d_3channel_layout[] = {
  /* FL,FR,FC */
  UA_CHANNEL_FRONT_LEFT,
  UA_CHANNEL_FRONT_RIGHT,
  UA_CHANNEL_FRONT_CENTER,
};

static UWORD32 wma10d_4channel_layout[] = {
  /* FL,FR,BL,BR */
  UA_CHANNEL_FRONT_LEFT_CENTER,
  UA_CHANNEL_FRONT_RIGHT_CENTER,
  UA_CHANNEL_REAR_LEFT,
  UA_CHANNEL_REAR_RIGHT
};

static UWORD32 wma10d_5channel_layout[] = {
/* FL,FR,FC,BL,BR */
  UA_CHANNEL_FRONT_LEFT,
  UA_CHANNEL_FRONT_RIGHT,
  UA_CHANNEL_FRONT_CENTER,
  UA_CHANNEL_REAR_LEFT,
  UA_CHANNEL_REAR_RIGHT
};

static UWORD32 wma10d_6channel_layout[] = {
/* FL,FR,FC,LFE,BL,BR */
  UA_CHANNEL_FRONT_LEFT,
  UA_CHANNEL_FRONT_RIGHT,
  UA_CHANNEL_FRONT_CENTER,
  UA_CHANNEL_LFE,
  UA_CHANNEL_REAR_LEFT,
  UA_CHANNEL_REAR_RIGHT,
};

static UWORD32 wma10d_7channel_layout[] = {
/* FL,FR,FC,LFE,BL,BR,BC */
  UA_CHANNEL_FRONT_LEFT,
  UA_CHANNEL_FRONT_RIGHT,
  UA_CHANNEL_FRONT_CENTER,
  UA_CHANNEL_LFE,
  UA_CHANNEL_REAR_LEFT,
  UA_CHANNEL_REAR_RIGHT,
  UA_CHANNEL_REAR_CENTER,
};

static UWORD32 wma10d_8channel_layout[] = {
/* FL,FR,FC,LFE,BL,BR,SL,SR  */
  UA_CHANNEL_FRONT_LEFT_CENTER,
  UA_CHANNEL_FRONT_RIGHT_CENTER,
  UA_CHANNEL_FRONT_CENTER,
  UA_CHANNEL_LFE,
  UA_CHANNEL_REAR_LEFT,
  UA_CHANNEL_REAR_RIGHT,
  UA_CHANNEL_SIDE_LEFT,
  UA_CHANNEL_SIDE_RIGHT,
};

static UWORD32 * wma10d_channel_layouts[] = {
    NULL,
    wma10d_1channel_layout, // 1
    wma10d_2channel_layout, // 2
    wma10d_3channel_layout,
    wma10d_4channel_layout,
    wma10d_5channel_layout,
    wma10d_6channel_layout,
    wma10d_7channel_layout,
    wma10d_8channel_layout,
};


enum ChipCode {
	CC_UNKNOW = 0,
	MX8ULP,
	CC_OTHERS
};

struct ChipInfo {
	enum ChipCode code;
	char name[30];
};

struct lastErr {
	WORD32 ErrType;
	char *ErrMsg;
};

struct innerBuf {
	UWORD8 *data;               /* inner buffer */
	UWORD32 inner_size;          /* current inner buffer size */
	UWORD32 inner_offset;        /* offset of inner buffer */
	UWORD32 buf_size;            /* total buf size */
	UWORD32 threshold;           /* threshold of the input buffer*/
};

struct DSP_Handle {
	UniACodecMemoryOps sMemOps;

	UWORD32 samplerate;
	UWORD32 channels;
	UWORD32 depth;
	bool framed;
	UWORD32 SampleBits;
	bool downmix;
	UniACodecParameterBuffer codecData;
	UniAcodecOutputPCMFormat outputFormat;
	char *codcDesc;
	char codec_name[256];
	WORD32 bitrate;

	STREAM_TYPE stream_type;

	UWORD32 mpeg_layer;
	UWORD32 mpeg_version;

	struct innerBuf  inner_buf;
	bool  initialized;
	WORD32 last_err;
	WORD32 frame_nb;
	UWORD32 consumed_length;
	CHAN_TABLE chan_map_tab;
	UWORD32 layout_bak[UA_CHANNEL_MAX];
	bool ID3flag;
	UWORD32 tagsize;

/****************DSP******************/
	xaf_adev_t *p_adev;
	xaf_adev_config_t adev_config;
	xaf_comp_t *p_comp;
	xaf_comp_status comp_status;
	WORD32 saved_comp_status;

	AUDIOFORMAT audio_type;
	WORD32 codec_type;
	UWORD8 *dsp_out_buf;
	UWORD32 outbuf_alloc_size;
	UWORD32 last_output_size;
	bool memory_allocated;
	bool depth_is_set;
	bool input_over;
	bool inptr_busy, outptr_busy;
	bool codecdata_copy;
	bool codecdata_ignored;
	unsigned int codecoffset;
	int blockalign;
	int offset_copy;
};

UA_ERROR_TYPE InputBufHandle(struct innerBuf *inner_buf,
			     UWORD8 *InputBuf,
			     UWORD32 InputSize,
			     UWORD32 *offset,
			     bool framed);
UA_ERROR_TYPE ResetInnerBuf(struct innerBuf *inner_buf,
			    UWORD32 buf_size,
			    UWORD32 threshold);
UA_ERROR_TYPE SetDefaultFeature(UniACodec_Handle pua_handle);
void cancel_unused_channel_data(UWORD8 *data_in, WORD32 length, WORD32 depth);

int comp_process(UniACodec_Handle pua_handle,
		 UWORD8 *input,
		 UWORD32 in_size,
		 UWORD32 *in_off,
		 UWORD8 *output,
		 UWORD32 *out_size);

int comp_flush_msg(UniACodec_Handle pua_handle);

const char *DSPDecVersionInfo();
UniACodec_Handle DSPDecCreate(UniACodecMemoryOps *memOps, AUDIOFORMAT type);
UA_ERROR_TYPE DSPDecDelete(UniACodec_Handle pua_handle);
UA_ERROR_TYPE DSPDecReset(UniACodec_Handle pua_handle);
UA_ERROR_TYPE DSPDecSetPara(UniACodec_Handle pua_handle,
			    UA_ParaType ParaType,
			    UniACodecParameter *parameter);
UA_ERROR_TYPE DSPDecGetPara(UniACodec_Handle pua_handle,
			    UA_ParaType ParaType,
			    UniACodecParameter *parameter);
UA_ERROR_TYPE DSPDecFrameDecode(UniACodec_Handle pua_handle,
				UWORD8 *InputBuf,
				UWORD32 InputSize,
				UWORD32 *offset,
				UWORD8 **OutputBuf,
				UWORD32 *OutputSize);
char *DSPDecLastErr(UniACodec_Handle pua_handle);

#endif //_DSP_WRAP_H_
