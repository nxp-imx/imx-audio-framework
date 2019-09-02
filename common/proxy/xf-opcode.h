/*******************************************************************************
 * Copyright (C) 2017 Cadence Design Systems, Inc.
 * Copyright 2018 NXP
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to use this Software with Cadence processor cores only and
 * not with any other processors and platforms, subject to
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

 ******************************************************************************/

/*******************************************************************************
 * xf-opcode.h
 *
 * Xtensa audio processing framework. Message API
 *
 ******************************************************************************/

#ifndef __XF_OPCODE_H
#define __XF_OPCODE_H

#include "xf-types.h"

/*******************************************************************************
 * Message routing composition - move somewhere else - tbd
 ******************************************************************************/

/* ...adjust IPC client of message going from user-space */
#define XF_MSG_AP_FROM_USER(id, client) \
	(((id) & ~(0xF << 2)) | (client))

/* ...wipe out IPC client from message going to user-space */
#define XF_MSG_AP_TO_USER(id)           \
	((id) & ~(0xF << 18))

/* ...port specification (12 bits) */
#define __XF_PORT_SPEC(core, id, port)  ((core) | ((id) << 2) | ((port) << 8))
#define __XF_PORT_SPEC2(id, port)       ((id) | ((port) << 8))
#define XF_PORT_CORE(spec)              ((spec) & 0x3)
#define XF_PORT_CLIENT(spec)            (((spec) >> 2) & 0x3F)
#define XF_PORT_ID(spec)                (((spec) >> 8) & 0xF)

/* ...message id contains source and destination ports specification */
#define __XF_MSG_ID(src, dst)   (((src) & 0xFFFF) | (((dst) & 0xFFFF) << 16))
#define XF_MSG_SRC(id)          (((id) >> 0) & 0xFFFF)
#define XF_MSG_SRC_CORE(id)     (((id) >> 0) & 0x3)
#define XF_MSG_SRC_CLIENT(id)   (((id) >> 2) & 0x3F)
#define XF_MSG_SRC_ID(id)       (((id) >> 0) & 0xFF)
#define XF_MSG_SRC_PORT(id)     (((id) >> 8) & 0xF)
#define XF_MSG_SRC_PROXY(id)    (((id) >> 15) & 0x1)
#define XF_MSG_DST(id)          (((id) >> 16) & 0xFFFF)
#define XF_MSG_DST_CORE(id)     (((id) >> 16) & 0x3)
#define XF_MSG_DST_CLIENT(id)   (((id) >> 18) & 0x3F)
#define XF_MSG_DST_ID(id)       (((id) >> 16) & 0xFF)
#define XF_MSG_DST_PORT(id)     (((id) >> 24) & 0xF)
#define XF_MSG_DST_PROXY(id)    (((id) >> 31) & 0x1)

/* ...special treatment of AP-proxy destination field */
#define XF_AP_IPC_CLIENT(id)            (((id) >> 18) & 0xF)
#define XF_AP_CLIENT(id)                (((id) >> 22) & 0x1FF)
#define __XF_AP_PROXY(core)             ((core) | 0x8000)
#define __XF_DSP_PROXY(core)            ((core) | 0x8000)
#define __XF_AP_CLIENT(core, client)    ((core) | ((client) << 6) | 0x8000)

/*******************************************************************************
 * Opcode composition
 ******************************************************************************/

/* ...opcode composition with command/response data tags */
#define __XF_OPCODE(c, r, op)   (((c) << 31) | ((r) << 30) | ((op) & 0x3F))

/* ...accessors */
#define XF_OPCODE_CDATA(opcode)         ((opcode) & (1 << 31))
#define XF_OPCODE_RDATA(opcode)         ((opcode) & (1 << 30))
#define XF_OPCODE_TYPE(opcode)          ((opcode) & (0x3F))

/*******************************************************************************
 * Opcode types
 ******************************************************************************/

/* ...unregister client */
#define XF_UNREGISTER                   __XF_OPCODE(0, 0, 0)

/* ...register client at proxy */
#define XF_REGISTER                     __XF_OPCODE(1, 0, 1)

/* ...port routing command */
#define XF_ROUTE                        __XF_OPCODE(1, 0, 2)

/* ...port unrouting command */
#define XF_UNROUTE                      __XF_OPCODE(1, 0, 3)

/* ...shared buffer allocation */
#define XF_ALLOC                        __XF_OPCODE(0, 0, 4)

/* ...shared buffer freeing */
#define XF_FREE                         __XF_OPCODE(0, 0, 5)

/* ...set component parameters */
#define XF_SET_PARAM                    __XF_OPCODE(1, 0, 6)

/* ...get component parameters */
#define XF_GET_PARAM                    __XF_OPCODE(1, 1, 7)

/* ...input buffer reception */
#define XF_EMPTY_THIS_BUFFER            __XF_OPCODE(1, 0, 8)

/* ...output buffer reception */
#define XF_FILL_THIS_BUFFER             __XF_OPCODE(0, 1, 9)

/* ...flush specific port */
#define XF_FLUSH                        __XF_OPCODE(0, 0, 10)

/* ...start component operation */
#define XF_START                        __XF_OPCODE(0, 0, 11)

/* ...stop component operation */
#define XF_STOP                         __XF_OPCODE(0, 0, 12)

/* ...pause component operation */
#define XF_PAUSE                        __XF_OPCODE(0, 0, 13)

/* ...resume component operation */
#define XF_RESUME                       __XF_OPCODE(0, 0, 14)

/* ...resume component operation */
#define XF_SUSPEND                      __XF_OPCODE(0, 0, 15)

/* ...load lib for component operation */
#define XF_LOAD_LIB                     __XF_OPCODE(0, 0, 16)

/* ...unload lib for component operation */
#define XF_UNLOAD_LIB                   __XF_OPCODE(0, 0, 17)

/* ...component output eos operation */
#define XF_OUTPUT_EOS                   __XF_OPCODE(0, 0, 18)

/* ...total amount of supported decoder commands */
#define __XF_OP_NUM                     19

/*******************************************************************************
 * XF_START message definition
 ******************************************************************************/

struct __attribute__((__packed__)) xf_start_msg {
	/* ...effective sample rate */
	u32             sample_rate;

	/* ...number of channels */
	u32             channels;

	/* ...sample width */
	u32             pcm_width;

	/* ...minimal size of intput buffer */
	u32             input_length;

	/* ...size of output buffer */
	u32             output_length;

};

/*******************************************************************************
 * bascial message
 ******************************************************************************/
typedef union DATA {
	u32                 value;

	struct {
		u32 size;
		u32 channel_table[10];
	} chan_map_tab;

	struct {
		u32 samplerate;
		u32 width;
		u32 depth;
		u32 channels;
		u32 endian;
		u32 interleave;
		u32 layout[12];
		u32 chan_pos_set;  // indicate if channel position is set outside or use codec default
	} outputFormat;
} data_t;

/*******************************************************************************
 * XF_GET_PARAM message
 ******************************************************************************/

/* ...message body (command/response) */
struct __attribute__((__packed__)) xf_get_param_msg {
		/* ...array of parameters requested */
		u32                 id;

		/* ...array of parameters values */
		data_t              mixData;

};

/* ...length of the XF_GET_PARAM command/response */
#define XF_GET_PARAM_CMD_LEN(params)    (sizeof(u32) * (params))
#define XF_GET_PARAM_RSP_LEN(params)    (sizeof(u32) * (params))

/*******************************************************************************
 * XF_SET_PARAM message
 ******************************************************************************/

/* ...component initialization parameter */
struct __attribute__ ((__packed__)) xf_set_param_msg {
	/* ...index of parameter passed to SET_CONFIG_PARAM call */
	u32                 id;

	/* ...value of parameter */
	data_t              mixData;
};

/* ...length of the command message */
#define XF_SET_PARAM_CMD_LEN(params)      \
			(sizeof(struct xf_set_param_msg) * (params))

/*******************************************************************************
 * XF_ROUTE definition
 ******************************************************************************/

/* ...port routing command */
struct __attribute__((__packed__)) xf_route_port_msg {
	/* ...source port specification */
	u32                 src;

	/* ...destination port specification */
	u32                 dst;

	/* ...number of buffers to allocate */
	u32                 alloc_number;

	/* ...length of buffer to allocate */
	u32                 alloc_size;

	/* ...alignment restriction for a buffer */
	u32                 alloc_align;

};

/*******************************************************************************
 * XF_UNROUTE definition
 ******************************************************************************/

/* ...port unrouting command */
struct __attribute__((__packed__)) xf_unroute_port_msg {
	/* ...source port specification */
	u32                 src;

	/* ...destination port specification */
	u32                 dst;

};

#endif
