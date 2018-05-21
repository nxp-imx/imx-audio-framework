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

 *****************************************************************************/

/*******************************************************************************
 * xf-proxy.h
 *
 * Proxy commmand/response messages
 *
 ******************************************************************************/

#ifndef __XF_PROXY_H
#define __XF_PROXY_H

#include "xf-types.h"

/*******************************************************************************
 * Types definitions
 ******************************************************************************/
/* ...command/response message */
struct __attribute__((__packed__)) xf_proxy_message {
	/* ...session ID */
	u32                 session_id;

	/* ...proxy API command/reponse code */
	u32                 opcode;

	/* ...length of attached buffer */
	u32                 length;

	/* ...physical address of message buffer */
	u32                 address;

	/* ...return message status */
	u32                 ret;

};

/*******************************************************************************
 * Ring buffer support
 ******************************************************************************/

/* ...total length of shared memory queue (for commands and responses) */
#define XF_PROXY_MESSAGE_QUEUE_LENGTH   (1 << 6)

/* ...index mask */
#define XF_PROXY_MESSAGE_QUEUE_MASK     0x3F

/* ...ring-buffer index */
#define __XF_QUEUE_IDX(idx, counter)    \
	    (((idx) & XF_PROXY_MESSAGE_QUEUE_MASK) | ((counter) << 16))

/* ...retrieve ring-buffer index */
#define XF_QUEUE_IDX(idx)               \
	    ((idx) & XF_PROXY_MESSAGE_QUEUE_MASK)

#define XF_QUEUE_ADVANCE_IDX(idx)       \
	    (((idx) + 0x10001) & (0xFFFF0000 | XF_PROXY_MESSAGE_QUEUE_MASK))

/* ...test if ring buffer is empty */
#define XF_QUEUE_EMPTY(read, write)     \
	    ((read) == (write))

/* ...test if ring buffer is full */
#define XF_QUEUE_FULL(read, write)      \
	    ((write) == (read) + (XF_PROXY_MESSAGE_QUEUE_LENGTH << 16))

#endif
