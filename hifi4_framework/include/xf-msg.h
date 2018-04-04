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
 * xf-msg.h
 *
 * Internal messages, and message queues.
 *
 *******************************************************************************/

#ifndef __XF_MSG_H
#define __XF_MSG_H

#include "xf-opcode.h"
#include "memory.h"

/*******************************************************************************
 * Types definitions
 ******************************************************************************/

/* ...forward declaration */
typedef struct xf_message   xf_message_t;

/* ...audio command/response message (internal to DSP processing framework) */
struct xf_message
{
    /* ...pointer to next item in the list */
    xf_message_t       *next;

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
typedef struct xf_msg_pool
{
    /* ...array of aligned messages */
    xf_message_t     *p;

    /* ...pointer to first free item in the pool */
    xf_message_t     *head;

    /* ...total size of the pool */
    u32                 n;

}   xf_msg_pool_t;

/* ...message accessor */
static inline xf_message_t * xf_msg_pool_item(xf_msg_pool_t *pool, u32 i)
{
    return (xf_message_t *) &pool->p[i];
}

/*******************************************************************************
 * Message queue data
 ******************************************************************************/

/* ...message queue (single-linked FIFO list) */
typedef struct  xf_msg_queue
{
    /* ...head of the queue */
    xf_message_t       *head;

    /* ...tail pointer */
    xf_message_t       *tail;

}   xf_msg_queue_t;

/*******************************************************************************
 * Message queue API
 ******************************************************************************/

/* ...initialize message queue */
static inline void  xf_msg_queue_init(xf_msg_queue_t *queue)
{
    queue->head = queue->tail = NULL;
}

/* ...push message in FIFO queue */
static inline int xf_msg_enqueue(xf_msg_queue_t *queue, xf_message_t *m)
{
    int     empty = (queue->head == NULL);

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
static inline xf_message_t * xf_msg_dequeue(xf_msg_queue_t *queue)
{
    xf_message_t   *m = queue->head;

    /* ...check if there is anything in the queue and dequeue it */
    if (m != NULL)
    {
        /* ...advance head to the next entry in the queue */
        if ((queue->head = m->next) == NULL)
            queue->tail = NULL;

        /* ...debug - wipe out next pointer */
        m->next = NULL;
    }

    return m;
}

/* ...test if message queue is empty */
static inline int xf_msg_queue_empty(xf_msg_queue_t *queue)
{
    return (queue->head == NULL);
}

/* ...get message queue head pointer */
static inline xf_message_t * xf_msg_queue_head(xf_msg_queue_t *queue)
{
    return queue->head;
}

/* ...check if message belongs to a pool */
static inline int xf_msg_from_pool(xf_msg_pool_t *pool, xf_message_t *m)
{
    return (u32)((xf_message_t *)m - pool->p) < pool->n;
}

/*******************************************************************************
 * Global message pool API
 ******************************************************************************/

/* ...submit message for execution on some DSP */
extern void xf_msg_submit(xf_msg_queue_t *queue, xf_message_t *m);

/* ...cancel local (scheduled on current core) message execution */
extern void xf_msg_cancel(xf_message_t *m);

/* ...complete message processing */
extern void xf_msg_complete(xf_message_t *m);

/* ...allocate message pool on specific core */
extern int  xf_msg_pool_init(xf_msg_pool_t *pool, u32 n, hifi_mem_info *mem_info);

/* ...allocate message from a pool (no concurrent access from other cores) */
extern xf_message_t * xf_msg_pool_get(xf_msg_pool_t *pool);

/* ...return message back to the pool (no concurrent access from other cores) */
extern void xf_msg_pool_put(xf_msg_pool_t *pool, xf_message_t *m);

/* ...destroy message pool */
extern void xf_msg_pool_destroy(xf_msg_pool_t *pool, hifi_mem_info *mem_info);

/* ...indicate whether pool of free messages is empty */
extern int  xf_message_pool_empty(void);

/* ...initialize global pool of messages */
extern void xf_message_pool_init(void);

/* ...retrieve message from local queue (protected from ISR) */
extern int xf_msg_local_put(xf_msg_queue_t *queue, xf_message_t *m);

/* ...retrieve message from local queue (protected from ISR) */
extern xf_message_t * xf_msg_local_get(xf_msg_queue_t *queue);

/* ...put message into proxy queue */
extern void xf_msg_proxy_put(xf_msg_queue_t *queue, xf_message_t *m);

/* ...retrieve message from proxy queue */
extern xf_message_t *xf_msg_proxy_get(xf_msg_queue_t *queue);

/* ...completion callback for message originating from remote proxy */
extern void xf_msg_proxy_complete(xf_msg_queue_t *queue, xf_message_t *m);

/*******************************************************************************
 * Auxiliary helpers
 ******************************************************************************/

/* ...send response message to caller */
static inline void xf_response(xf_message_t *m)
{
    xf_msg_complete(m);
}

/* ...send response message with output buffer */
static inline void xf_response_data(xf_message_t *m, u32 length)
{
    /* ...adjust message output buffer */
    m->length = length;

    /* ...return message to originator */
    xf_msg_complete(m);
}

/* ...send generic "ok" message (no data buffer) */
static inline void xf_response_ok(xf_message_t *m)
{
    /* ...adjust message output buffer */
    m->length = 0;

    /* ...return message to originator */
    xf_msg_complete(m);
}

/* ...send error-response message */
static inline void xf_response_err(xf_message_t *m)
{
    /* ...set generic error message */
    m->opcode = XF_UNREGISTER, m->length = 0;

    /* ...return message to originator */
    xf_msg_complete(m);
}

#endif
