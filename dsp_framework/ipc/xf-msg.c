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
 * xf-msg.c
 *
 * Message/message pool handling
 *
 *****************************************************************************/

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "mydefs.h"
#include "xf-debug.h"
#include "xf-hal.h"

extern struct dsp_main_struct *dsp_global_data;

/*******************************************************************************
 * Entry points
 ******************************************************************************/

/* ...allocate message pool */
int xf_msg_pool_init(struct xf_msg_pool *pool,
		     u32 n,
		     struct dsp_mem_info *mem_info)
{
	u32 i;

	/* ...allocate shared memory from global pool */
	pool->p = MEM_scratch_ua_malloc(sizeof(*pool->p) * n);

	/* ...place all messages into single-liked list */
	for (pool->head = &pool->p[i = 0]; i < n - 1; i++) {
		/* ...set message pointer to next message in the pool */
		xf_msg_pool_item(pool, i)->next = xf_msg_pool_item(pool, i + 1);
	}

	/* ...set tail of the list */
	xf_msg_pool_item(pool, i)->next = NULL;

	/* ...save pool size */
	pool->n = n;

	return 0;
}

/* ...destroy memory pool */
void xf_msg_pool_destroy(struct xf_msg_pool *pool,
			 struct dsp_mem_info *mem_info)
{
	/* ...release pool memory (from shared local-IPC memory) */
	MEM_scratch_ua_mfree(pool->p);

	pool->p = NULL;
	pool->head = NULL;
	pool->n = 0;
}

/* ...allocate message from a pool (no concurrent access from other cores) */
struct xf_message *xf_msg_pool_get(struct xf_msg_pool *pool)
{
	struct xf_message  *_m;

	/* ...pop message from the head of the pool */
	XF_CHK_ERR(_m = pool->head, NULL);

	/* ...advance list head */
	pool->head = (struct xf_message *)(((struct xf_message *)_m)->next);

	/* ...debug - wipe out message "next" pointer */
	((struct xf_message *)_m)->next = NULL;

	/* ...return properly aligned message pointer */
	return (struct xf_message *)_m;
}

/* ...return message back to the pool (no concurrent access from other cores) */
void xf_msg_pool_put(struct xf_msg_pool *pool, struct xf_message *m)
{
	struct xf_message  *_m = (struct xf_message *)m;

	/* ...make sure the message is properly aligned object */
	BUG(!XF_IS_ALIGNED(_m), _x("Corrupted message pointer: %p"), _m);

	/* ...make sure it is returned to the same pool
	 * (need a length for that - tbd)
	 */
	BUG(!xf_msg_from_pool(pool, m) < 0,
	    _x("Bad pool/message: %p/%p"), pool->p, _m);

	/* ...place message into the head */
	m->next = (struct xf_message *)pool->head;

	/* ...advance pool head */
	pool->head = _m;
}

/* ...retrieve message from local queue (protected from ISR) */
int xf_msg_local_put(struct xf_msg_queue *queue, struct xf_message *m)
{
	int first;
	u32             status;

	status = xf_isr_disable(0);
	first = xf_msg_enqueue(queue, m);
	xf_isr_restore(0, status);

	return first;
}

/* ...retrieve message from local queue (protected from ISR) */
struct xf_message *xf_msg_local_get(struct xf_msg_queue *queue)
{
	struct xf_message *m;
	u32             status;

	status = xf_isr_disable(0);
	m = xf_msg_dequeue(queue);
	xf_isr_restore(0, status);

	return m;
}

/* ...put message into proxy queue */
void xf_msg_proxy_put(struct xf_msg_queue *queue, struct xf_message *m)
{
	int first;

	/* place a message in the queue */
	first = xf_msg_enqueue(queue, m);
}

/* ...retrieve message from proxy queue */
struct xf_message *xf_msg_proxy_get(struct xf_msg_queue *queue)
{
	struct xf_message *m;

	/* ...dequeue message from response queue */
	m = xf_msg_dequeue(queue);

	return m;
}

/* ...completion callback for message originating from remote proxy */
void xf_msg_proxy_complete(struct xf_msg_queue *queue, struct xf_message *m)
{
	/* ...place message into proxy response queue */
	xf_msg_proxy_put(queue, m);
}

/* ...submit message for instant execution on some core */
void xf_msg_submit(struct xf_msg_queue *queue, struct xf_message *m)
{
	xf_msg_local_put(queue, m);
}

/* ...complete message and pass response to a caller */
void xf_msg_complete(struct xf_message *m)
{
	struct dsp_main_struct *dsp_config = dsp_global_data;
	u32 src = XF_MSG_SRC(m->id);
	u32 dst = XF_MSG_DST(m->id);

	/* ...swap src/dst specifiers */
	m->id = __XF_MSG_ID(dst, src);

	/* ...check if message goes to remote IPC layer */
	if (XF_MSG_DST_PROXY(m->id)) {
		/* ...return message to proxy */
		xf_msg_proxy_complete(&dsp_config->response, m);
	} else {
		/* ...destination is within DSP cluster;
		 * check if that is a data buffer
		 */
		switch (m->opcode) {
		case XF_EMPTY_THIS_BUFFER:
			/* ...emptied buffer goes back to the output port */
			m->opcode = XF_FILL_THIS_BUFFER;
			break;
		case XF_FILL_THIS_BUFFER:
			/* ...filled buffer is passed to the input port */
			m->opcode = XF_EMPTY_THIS_BUFFER;
			break;
		}

		/* ...submit message for execution */
		xf_msg_submit(&dsp_config->queue, m);
	}
}
