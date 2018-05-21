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

/******************************************************************************
 * xf-msg.h
 *
 * Internal messages, and message queues.
 *
 *****************************************************************************/

#ifndef __XF_MSG_H
#define __XF_MSG_H

#include "xf-opcode.h"
#include "memory.h"

/*******************************************************************************
 * Types definitions
 ******************************************************************************/

/* ...audio command/response message (internal to DSP processing framework) */
struct xf_message {
	/* ...pointer to next item in the list */
	struct xf_message   *next;

	/* ...shmem session_id */
	u32                 id;

	/* ...operation code */
	u32                 opcode;

	/* ...length of attached message buffer */
	u32                 length;

	/* ...message buffer (translated virtual address) */
	void               *buffer;

	/* ...return message status */
	u32                ret;
};

/* ...message pool definition */
struct xf_msg_pool {
	/* ...array of aligned messages */
	struct xf_message     *p;

	/* ...pointer to first free item in the pool */
	struct xf_message     *head;

	/* ...total size of the pool */
	u32                 n;

};

/* ...message accessor */
static inline struct xf_message *xf_msg_pool_item(
					struct xf_msg_pool *pool, u32 i)
{
	return (struct xf_message *)&pool->p[i];
}

/*******************************************************************************
 * Message queue data
 ******************************************************************************/

/* ...message queue (single-linked FIFO list) */
struct  xf_msg_queue {
	/* ...head of the queue */
	struct xf_message       *head;

	/* ...tail pointer */
	struct xf_message       *tail;

};

/*******************************************************************************
 * Message queue API
 ******************************************************************************/

/* ...initialize message queue */
static inline void  xf_msg_queue_init(struct  xf_msg_queue *queue)
{
	queue->head = NULL;
	queue->tail = NULL;
}

/* ...push message in FIFO queue */
static inline int xf_msg_enqueue(struct  xf_msg_queue *queue,
				 struct xf_message *m)
{
	int empty = (queue->head == NULL);

	/* ...set list terminating pointer */
	m->next = NULL;

	if (empty)
		queue->head = m;
	else
		queue->tail->next = m;

	/* ...advance tail pointer */
	queue->tail = m;

	/* ...return emptiness status */
	return empty;
}

/* ...retrieve (pop) next message from FIFO queue */
static inline struct xf_message *xf_msg_dequeue(struct xf_msg_queue *queue)
{
	struct xf_message *m = queue->head;

	/* ...check if there is anything in the queue and dequeue it */
	if (m) {
		/* ...advance head to the next entry in the queue */
		queue->head = m->next;
		if (!queue->head)
			queue->tail = NULL;

		/* ...debug - wipe out next pointer */
		m->next = NULL;
	}

	return m;
}

/* ...test if message queue is empty */
static inline int xf_msg_queue_empty(struct xf_msg_queue *queue)
{
	return (queue->head == NULL);
}

/* ...get message queue head pointer */
static inline struct xf_message *xf_msg_queue_head(struct xf_msg_queue *queue)
{
	return queue->head;
}

/* ...check if message belongs to a pool */
static inline int xf_msg_from_pool(struct xf_msg_pool *pool,
				   struct xf_message *m)
{
	return (u32)((struct xf_message *)m - pool->p) < pool->n;
}

/*******************************************************************************
 * Global message pool API
 ******************************************************************************/

/* ...submit message for execution on some DSP */
void xf_msg_submit(struct xf_msg_queue *queue, struct xf_message *m);

/* ...cancel local (scheduled on current core) message execution */
void xf_msg_cancel(struct xf_message *m);

/* ...complete message processing */
void xf_msg_complete(struct xf_message *m);

/* ...allocate message pool on specific core */
int xf_msg_pool_init(struct xf_msg_pool *pool,
		     u32 n,
		     struct dsp_mem_info *mem_info);

/* ...allocate message from a pool (no concurrent access from other cores) */
struct xf_message *xf_msg_pool_get(struct xf_msg_pool *pool);

/* ...return message back to the pool (no concurrent access from other cores) */
void xf_msg_pool_put(struct xf_msg_pool *pool, struct xf_message *m);

/* ...destroy message pool */
void xf_msg_pool_destroy(struct xf_msg_pool *pool,
			 struct dsp_mem_info *mem_info);

/* ...indicate whether pool of free messages is empty */
int  xf_message_pool_empty(void);

/* ...initialize global pool of messages */
void xf_message_pool_init(void);

/* ...retrieve message from local queue (protected from ISR) */
int xf_msg_local_put(struct xf_msg_queue *queue, struct xf_message *m);

/* ...retrieve message from local queue (protected from ISR) */
struct xf_message *xf_msg_local_get(struct xf_msg_queue *queue);

/* ...put message into proxy queue */
void xf_msg_proxy_put(struct xf_msg_queue *queue, struct xf_message *m);

/* ...retrieve message from proxy queue */
struct xf_message *xf_msg_proxy_get(struct xf_msg_queue *queue);

/* ...completion callback for message originating from remote proxy */
void xf_msg_proxy_complete(struct xf_msg_queue *queue, struct xf_message *m);

/*******************************************************************************
 * Auxiliary helpers
 ******************************************************************************/

/* ...send response message to caller */
static inline void xf_response(struct xf_message *m)
{
	xf_msg_complete(m);
}

/* ...send response message with output buffer */
static inline void xf_response_data(struct xf_message *m, u32 length)
{
	/* ...adjust message output buffer */
	m->length = length;

	/* ...return message to originator */
	xf_msg_complete(m);
}

/* ...send generic "ok" message (no data buffer) */
static inline void xf_response_ok(struct xf_message *m)
{
	/* ...adjust message output buffer */
	m->length = 0;

	/* ...return message to originator */
	xf_msg_complete(m);
}

/* ...send error-response message */
static inline void xf_response_err(struct xf_message *m)
{
	/* ...set generic error message */
	m->opcode = XF_UNREGISTER, m->length = 0;

	/* ...return message to originator */
	xf_msg_complete(m);
}

#endif
