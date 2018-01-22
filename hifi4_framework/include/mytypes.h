//*****************************************************************
// Copyright (c) 2017 Cadence Design Systems, Inc.
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

/*
 * mytypes.h
 *
 */

#ifndef _MYTYPES_H_
#define _MYTYPES_H_

#include "xt_library_loader.h"
#include "system_address.h"
#include "loader_internal.h"
#include "hifi_codec_interface.h"

#define HF_SUCCESS	0
#define HF_ERROR	1

/* external buffer
 *  --------------------------------------------------------------------------
 *  |     name                        |   size       |    description     |
 * ----------------------------------------------------------------------------------
 *  |     scratch buffer for malloc   |   0xffffff   |  For MEM_scratch_malloc()
 * ----------------------------------------------------------------------------------
 *  |     global structure            |   4096       |  For store hifi config structure
 * ----------------------------------------------------------------------------------
 */

#define MULTI_CODEC_NUM     	5

#define INTERFACE_HEADER_SIZE  44
#define INTERFACE_MAX_CONFIG_SIZE  64   /* max size of the interface header extension in bytes */

#define ICM_INTR_MASK ((1 < 15))
#define ICM_IACK_MASK ((1 < 31))

#define MYMAX(x,y) 	((x) > (y) ? (x) : (y))
#define MYMIN(x,y) 	((x) < (y) ? (x) : (y))

typedef unsigned char u8;
typedef char s8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed int s32;


union header {
	struct {
		union header *ptr;
		unsigned long size;
	}s;
	char c[8];
};
typedef union header HEADER;

typedef union {
	struct {
	u32 msg		: 6;		// intr = 1 when sending msg.
	u32 sub_msg : 6;		// sub_msg will have ICM_MSG when msg=ICM_XXX_ACTION_COMPLETE
	u32 rsvd 	: 3;		// reserved
	u32 intr	: 1;
	u32 size 	: 15;		// =size in bytes (excluding header) to follow when intr=1, =response message when ack=1
	u32 ack 	: 1;
	};
	u32 allbits;
} icm_header_t;

typedef enum {
	HIFI_CODEC_LIB =1,
	HIFI_CODEC_WRAP_LIB
}lib_type;

typedef enum {
	ICM_CORE_READY = 1,
	ICM_PI_LIB_MEM_ALLOC,
	ICM_PI_LIB_MEM_FREE,
	ICM_PI_LIB_INIT,
	ICM_PI_LIB_LOAD,
	ICM_PI_LIB_UNLOAD,

	ICM_DPU_ACTION_COMPLETE,
	ICM_APU_ACTION_COMPLETE,

	ICM_OPEN,
	ICM_EMPTY_THIS_BUFFER,
	ICM_FILL_THIS_BUFFER,
	ICM_PAUSE,
	ICM_CLOSE,

	ICM_GET_PCM_PROP,
	ICM_SET_PARA_CONFIG,

	ICM_CORE_EXIT,
	ICM_EXT_MSG_ADDR,
	ICM_RESET,
	ICM_SUSPEND,
	ICM_RESUME,
} icm_action_t;

typedef enum {
	AUD_IDLE = 0,
	AUD_STOPPED,
	AUD_DECODING,
	AUD_PAUSED
} aud_status_t;

#define CMD_ON	1
#define CMD_OFF	0

typedef enum {
	XT_NO_ERROR = 0,
	XT_ERROR1,
	XT_ERROR2,
	XT_ERROR_FATAL=-1
} xt_error_t;

typedef struct {
	u32 buffer_addr;
	u32 buffer_size;
	s32 ret;
} icm_pilib_size_t;

typedef struct {
        u32     ext_msg_addr;
        u32     ext_msg_size;
        u32     scratch_buf_phys;
        u32     scratch_buf_size;
        u32     hifi_config_phys;
        u32     hifi_config_size;
} icm_dpu_ext_msg;

typedef struct {
	u32 process_id;			// non-zero indicates success;
	u32 codec_id;           /*codec id*/
	s32 ret;                // executed status of function;
} icm_base_info_t;

typedef struct {
	icm_base_info_t base_info;

	u32 inp_addr_sysram;	// init by APU
	u32 inp_buf_size_max;	// init by APU
	u32 inp_cur_offset;		// init by APU, updated by DPU
	u32 out_addr_sysram;	// init by APU
	u32 out_buf_size_max;   // init by APU
	u32 out_cur_offset;		// init by APU, updated by DPU
	u32 input_over;         // indicate external stream is over;
} icm_cdc_iobuf_t;

typedef struct {
	icm_base_info_t base_info;
	u32 cmd;        /*set parameter command value*/
	u32 val;        /*set parameter value*/
} icm_prop_config;

typedef struct {
	icm_base_info_t base_info;

	u32 pcmbytes; 	/* total bytes in the wav file */
	u32 sfreq; 		/* sample rate */
	u32 channels;	/* output channels */
	u32 bits;		/* bits per sample */
	u32 consumed_bytes;
	u32 cycles;
} icm_pcm_prop_t;

typedef struct {
	xtlib_pil_info pil_info;

	u32 process_id;
	u32 lib_type;
} icm_xtlib_pil_info;

typedef struct {
	u32 start_addr;
	u32 codec_type;
}xtlib_overlay_info;

typedef struct {
	u32 type: 16;	/* codec type; e.g., 1: mp3dec */
	u32 stat: 16;	/* loaded, init_done, active, closed */
	u32 size_libtext;
	u32 size_libdata;
	xtlib_overlay_info ol_inf_hifiLib_dpu;
	xtlib_pil_info pil_inf_hifiLib_dpu;
	void *p_xa_process_api;
}dpu_lib_stat_t;

typedef struct {
	UniACodecVersionInfo    VersionInfo;
	UniACodecCreate         Create;
	UniACodecDelete         Delete;
	UniACodecInit           Init;
	UniACodecReset          Reset;
	UniACodecSetParameter   SetPara;
	UniACodecGetParameter   GetPara;
	UniACodec_decode_frame  Process;
	UniACodec_get_last_error    GetLastError;
}sCodecFun;

typedef struct codec_info_t {

	struct codec_info_t *list;

	u32 process_id;            /* process identifier */
	u32 codec_id;              /* codec identifier */
	u32 status;

	u32 input_over;
	icm_cdc_iobuf_t sysram_io;

	icm_dpu_ext_msg *dpu_ext_msg;
	dpu_lib_stat_t lib_on_dpu;
	xtlib_loader_globals xtlib_globals;

	tUniACodecQueryInterface codecwrapinterface;
	void *codecinterface;
	HiFiCodec_Handle pWrpHdl;
	sCodecFun WrapFun;

} codec_info;


#define MAX_MSGS_IN_QUEUE   4
typedef struct {
	u8 *icm_msg_que[MAX_MSGS_IN_QUEUE];
	u32 icm_que_read_index;
	u32 icm_que_write_index;
	volatile u32 icm_remaining_msgs;
	icm_dpu_ext_msg dpu_ext_msg;

	char *scratch_buf_ptr;
	long int scratch_total_size;
	long int scratch_remaining;
	HEADER *Base;
	HEADER *First;
	HEADER *Allocp;
	unsigned long Heapsize;
	unsigned long Availmem;

	codec_info codec_info_client[MULTI_CODEC_NUM];

} hifi_main_struct;

#endif /* _MYTYPES_H_ */
