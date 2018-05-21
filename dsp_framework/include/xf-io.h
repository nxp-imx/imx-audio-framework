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
 * xf-io.h
 *
 * Input/output data ports
 *
 ******************************************************************************/

#ifndef __XF_IO_H
#define __XF_IO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xf-msg.h"
#include "memory.h"

/*******************************************************************************
 * Types definitions
 ******************************************************************************/

/* ...input port with interim buffer */
struct xf_input_port {
	/* ...message queue */
	struct  xf_msg_queue    queue;

	/* ...internal contiguous buffer to store incoming data */
	void                   *buffer;

	/* ...size of internal buffer */
	u32                     length;

	/* ...current writing position in the buffer */
	u32                     filled;

	/* ...interim pointer to input message buffer */
	void                   *access;

	/* ...remaining length of current input message */
	u32                     remaining;

	/* ...execution flags */
	u32                     flags;

};

/*******************************************************************************
 * Input port flags
 ******************************************************************************/

/* ...data enabled */
#define XF_INPUT_FLAG_CREATED           (1 << 0)

/* ...data enabled */
#define XF_INPUT_FLAG_ENABLED           (1 << 1)

/* ...end-of-stream condition */
#define XF_INPUT_FLAG_EOS               (1 << 2)

/* ...stream completed */
#define XF_INPUT_FLAG_DONE              (1 << 3)

/* ...stream purging sequence */
#define XF_INPUT_FLAG_PURGING           (1 << 4)

/* ...base input port flags mask */
#define __XF_INPUT_FLAGS(flags)         ((flags) & ((1 << 5) - 1))

/* ...custom input port flag */
#define __XF_INPUT_FLAG(f)              ((f) << 5)

/*******************************************************************************
 * Helpers
 ******************************************************************************/

/* ...test if input port is created */
static inline int xf_input_port_created(struct xf_input_port *port)
{
	return (port->flags & XF_INPUT_FLAG_CREATED);
}

/* ...check if input port is ready (has pending message) */
static inline int xf_input_port_ready(struct xf_input_port *port)
{
	return (xf_msg_queue_head(&port->queue) != NULL);
}

/* ...test if input port entered end-of-stream condition */
static inline int xf_input_port_done(struct xf_input_port *port)
{
	return (port->flags & XF_INPUT_FLAG_DONE);
}

/* ...check if port is in bypass mode */
static inline int xf_input_port_bypass(struct xf_input_port *port)
{
	return port->buffer == NULL;
}

/* ...bypass port only: check if there is a data available */
static inline void *xf_input_port_data(struct xf_input_port *port)
{
	return port->access;
}

/* ...bypass port only: get remaining length of current message */
static inline u32 xf_input_port_length(struct xf_input_port *port)
{
	return port->remaining;
}

/* ...non-bypass port only: get current fill level */
static inline u32 xf_input_port_level(struct xf_input_port *port)
{
	return port->filled;
}

/*******************************************************************************
 * Output port data
 ******************************************************************************/

struct xf_output_port {
	/* ...pending message queue */
	struct  xf_msg_queue    queue;

	/* ...message pool */
	struct xf_msg_pool      pool;

	/* ...saved port unrouting message */
	struct xf_message       *unroute;

	/* ...internal buffer pointer to store incoming data */
	void                   *buffer;

	/* ...length of output buffer */
	u32                     length;

	/* ...output port flags */
	u32                     flags;

};

/*******************************************************************************
 * Output port flags
 ******************************************************************************/

/* ...port is created */
#define XF_OUTPUT_FLAG_CREATED          (1 << 0)

/* ...port is routed */
#define XF_OUTPUT_FLAG_ROUTED           (1 << 1)

/* ...data enabled */
#define XF_OUTPUT_FLAG_ENABLED          (1 << 2)

/* ...stream completed */
#define XF_OUTPUT_FLAG_DONE             (1 << 3)

/* ...flushing sequence is on-going */
#define XF_OUTPUT_FLAG_FLUSHING         (1 << 4)

/* ...port is idle - no outstanding messages */
#define XF_OUTPUT_FLAG_IDLE             (1 << 5)
/* ...port is being unrouted */
#define XF_OUTPUT_FLAG_UNROUTING        (1 << 6)

/* ...base output port flags accessor */
#define __XF_OUTPUT_FLAGS(flags)        ((flags) & ((1 << 7) - 1))

/* ...custom output port flag */
#define __XF_OUTPUT_FLAG(f)             ((f) << 7)

/*******************************************************************************
 * Helpers
 ******************************************************************************/

/* ...test if input port is created */
static inline int xf_output_port_created(struct xf_output_port *port)
{
	return (port->flags & XF_OUTPUT_FLAG_CREATED);
}

/* ...check if port is routed */
static inline int xf_output_port_routed(struct xf_output_port *port)
{
	return ((port->flags & XF_OUTPUT_FLAG_ROUTED) != 0);
}

/* ...check if port unrouting sequence is ongoing */
static inline int xf_output_port_unrouting(struct xf_output_port *port)
{
	return ((port->flags & XF_OUTPUT_FLAG_UNROUTING) != 0);
}

/* ...check if port is idle (owns all data buffers) */
static inline int xf_output_port_idle(struct xf_output_port *port)
{
	return ((port->flags & XF_OUTPUT_FLAG_IDLE) != 0);
}

/* ...check if port is ready (has output buffers - better use flags - tbd) */
static inline int xf_output_port_ready(struct xf_output_port *port)
{
#ifdef XAF_ENABLE_NON_HIKEY
	return (xf_msg_queue_head(&port->queue) &&
		!xf_output_port_unrouting(port));
#else
	return (xf_msg_queue_head(&port->queue) != NULL);
#endif
}

/* ...output port flow-control message accessor */
static inline struct xf_message *xf_output_port_control_msg(
					struct xf_output_port *port)
{
	return xf_msg_pool_item(&port->pool, 0);
}

/*******************************************************************************
 * Input port API
 ******************************************************************************/

/* ...initialize input port structure */
int  xf_input_port_init(struct xf_input_port *port,
			u32 size,
			u32 align,
			struct dsp_mem_info *mem_info);

/* ...put message into input port queue */
int  xf_input_port_put(struct xf_input_port *port, struct xf_message *m);
#ifndef XAF_ENABLE_NON_HIKEY
/* ...initiate eos */
int xf_input_port_eos(struct xf_input_port *port);
#endif
/* ...fill-in required amount of data into input port buffer */
int  xf_input_port_fill(struct xf_input_port *port);

/* ...pad input buffer with given value */
void xf_input_port_pad(struct xf_input_port *port, u8 pad);

/* ...consume bytes from input buffer */
void xf_input_port_consume(struct xf_input_port *port, u32 n);

/* ...purge input port queue */
void xf_input_port_purge(struct xf_input_port *port);

/* ...save flow-control message for propagated input port purging sequence */
void xf_input_port_control_save(struct xf_input_port *port, struct xf_message *m);

/* ...complete input port purging sequence */
void xf_input_port_purge_done(struct xf_input_port *port);

/* ...destroy input port data */
void xf_input_port_destroy(struct xf_input_port *port,
			   struct dsp_mem_info *mem_info);

/*******************************************************************************
 * Output port API
 ******************************************************************************/

/* ...initialize output port structure */
int  xf_output_port_init(struct xf_output_port *port, u32 size);

/* ...put next message to the output port */
int  xf_output_port_put(struct xf_output_port *port, struct xf_message *m);

/* ...get data buffer of output message */
void *xf_output_port_data(struct xf_output_port *port);

/* ...route output port */
int xf_output_port_route(struct xf_output_port *port,
			 u32 id,
			 u32 n,
			 u32 length,
			 u32 align,
			 struct dsp_mem_info *mem_info);

/* ...unroute output port */
void xf_output_port_unroute(struct xf_output_port *port,
			    struct dsp_mem_info *mem_info);
#ifdef XAF_ENABLE_NON_HIKEY
/* ...start output port unrouting sequence */
void xf_output_port_unroute_start(struct xf_output_port *port, struct xf_message *m);

/* ...complete port unrouting sequence */
void xf_output_port_unroute_done(struct xf_output_port *port);
#endif
/* ...produce output message marking amount of bytes produced */
int  xf_output_port_produce(struct xf_output_port *port, u32 n, u32 ret);

/* ...purge output port */
void xf_output_port_purge(struct xf_output_port *port);

/* ...flush output port and return non-zero result if sequence is over */
int xf_output_port_flush(struct xf_output_port *port, u32 opcode);

/* ...fill-in required amount of data into output port buffer */
int xf_output_port_fill(struct xf_output_port *port);

/* ...complete flushing sequence */
void xf_output_port_flush_done(struct xf_output_port *port);

/* ...destroy output port data */
void xf_output_port_destroy(struct xf_output_port *port,
			    struct dsp_mem_info *mem_info);

#endif
