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
 * xf-proxy.h
 *
 * Proxy commmand/response messages
 *
 *******************************************************************************/

#ifndef __XF_PROXY_H
#define __XF_PROXY_H

#include <pthread.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <poll.h>
#include "xf-types.h"
#include "xf-ipc.h"
#include "xf-opcode.h"
#include "xf-osal.h"
#include "xf-debug.h"


/* ...maximal number of clients supported by proxy */
#define XF_CFG_PROXY_MAX_CLIENTS        256

/* ...alignment for shared buffers */
#define XF_PROXY_ALIGNMENT              64

/* ...component string identifier */
typedef const char					*xf_id_t;

/* ...handle to proxy data */
typedef struct xf_proxy				xf_proxy_t;

/* ...handle to component data */
typedef struct xf_handle			xf_handle_t;

/* ...user-message */
typedef struct xf_user_msg          xf_user_msg_t;

/* ...proxy-message */
typedef struct xf_proxy_message     xf_proxy_msg_t;

/* ...buffer pool type */
typedef u32							xf_pool_type_t;

/* ...buffer pool */
typedef struct xf_pool				xf_pool_t;

/* ...individual buffer from pool */
typedef struct xf_buffer			xf_buffer_t;

/* ...thread handle definition */
typedef pthread_t					xf_thread_t;

/* ...response callback */
typedef void (*xf_response_cb)(xf_handle_t *h, xf_user_msg_t *msg);


/* ...buffer pool type */
enum xf_pool_type
{
	XF_POOL_AUX = 0,
	XF_POOL_INPUT = 1,
	XF_POOL_OUTPUT = 2
};

/*******************************************************************************
 * Types definitions
 ******************************************************************************/
/* ...need that at all? hope no */
struct xf_user_msg
{
	/* ...source component specification */
	u32             id;

	/* ...message opcode */
	u32             opcode;

	/* ...buffer length */
	u32             length;

	/* ...buffer pointer */
	void           *buffer;

	/* ...return message status */
	u32            ret;
};

/* ...command/response message */
struct xf_proxy_message
{
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

/* ...buffer link pointer */
typedef union xf_buffer_link
{
	/* ...pointer to next free buffer in a pool (for free buffer) */
	xf_buffer_t        *next;

	/* ...reference to a buffer pool (for allocated buffer) */
	xf_pool_t          *pool;
} xf_buffer_link_t;

/* ...buffer descriptor */
struct xf_buffer
{
	/* ...virtual address of contiguous buffer */
	void               *address;

	/* ...link pointer */
	xf_buffer_link_t    link;
};

/* ...buffer pool */
struct xf_pool
{
	/* ...reference to proxy data */
	xf_proxy_t         *proxy;

	/* ...length of individual buffer in a pool */
	u32                 length;

	/* ...number of buffers in a pool */
	u32                 number;

	/* ...pointer to pool memory */
	void               *p;

	/* ...pointer to first free buffer in a pool */
	xf_buffer_t        *free;

	/* ...individual buffers */
	xf_buffer_t         buffer[0];
};

/* ...free clients list */
typedef union xf_proxy_cmap_link
{
	/* ...index of next free client in the list */
	u32                     next;

	/* ...pointer to allocated component handle */
	xf_handle_t            *handle;

} xf_proxy_cmap_link_t;

/* ...proxy data structure */
struct xf_proxy
{
	/* ...platform-specific IPC data */
	xf_proxy_ipc_data_t     ipc;

	/* ...auxiliary buffer pool for clients */
	xf_pool_t              *aux;

	/* ...global proxy lock */
	xf_lock_t               lock;

	/* ...proxy thread handle */
	xf_thread_t             thread;

	/* ...proxy identifier (core of remote DSP hosting SHMEM interface) */
	u32                     core;

	/* ...client association map */
	xf_proxy_cmap_link_t    cmap[XF_CFG_PROXY_MAX_CLIENTS];
};


/*******************************************************************************
 * Auxiliary proxy helpers
 ******************************************************************************/
/* ...lock proxy data */
static inline void xf_proxy_lock(xf_proxy_t *proxy)
{
	__xf_lock(&proxy->lock);
}

/* ...unlock proxy data */
static inline void xf_proxy_unlock(xf_proxy_t *proxy)
{
	__xf_unlock(&proxy->lock);
}

/* ...translate proxy shared address into local virtual address */
static inline void * xf_proxy_a2b(xf_proxy_t *proxy, u32 address)
{
	return xf_ipc_a2b(&proxy->ipc, address);
}

/* ...translate local virtual address into shared proxy address */
static inline u32 xf_proxy_b2a(xf_proxy_t *proxy, void *b)
{
	return xf_ipc_b2a(&proxy->ipc, b);
}

/* ...submit asynchronous response message */
static inline int xf_proxy_response_put(xf_proxy_t *proxy, xf_proxy_msg_t *msg)
{
	return xf_proxy_ipc_response_put(&proxy->ipc, msg);
}

/* ...retrieve asynchronous response message */
static inline int xf_proxy_response_get(xf_proxy_t *proxy, xf_proxy_msg_t *msg)
{
	int fd;
	struct pollfd   pollfd;
	u32 timeout = TIMEOUT;
	int ret = 0;

	/* ...read pipe */
	fd = proxy->ipc.pipe[0];

	/* ...specify waiting set */
	pollfd.fd = fd;
	pollfd.events = POLLIN | POLLRDNORM;

	/* ...wait until there is a data in file */
	ret = poll(&pollfd, 1, timeout);
	if(ret > 0)
	    return xf_proxy_ipc_response_get(&proxy->ipc, msg);
	else if(ret < 0)
	    return ret;
	else
	    return -ETIMEDOUT;
}

/* ...accessor to buffer data */
static inline void * xf_buffer_data(xf_buffer_t *buffer)
{
	return buffer->address;
}

/* ...length of buffer data */
static inline size_t xf_buffer_length(xf_buffer_t *buffer)
{
	xf_pool_t *pool = buffer->link.pool;
	return (size_t)pool->length;
}

/*******************************************************************************
 * Component handle definition
 ******************************************************************************/
struct xf_handle
{
	/* ...platform-specific IPC data */
	xf_ipc_data_t           ipc;

	/* ...platform-specific IPC data, used for no-timely response message */
	xf_ipc_data_t           ipc_ack;

	/* ...reference to proxy data */
	xf_proxy_t             *proxy;

	/* ...component lock */
	xf_lock_t               lock;

	/* ...auxiliary control buffer for control transactions */
	xf_buffer_t            *aux;

	/* ...global client-id of the component */
	u32                     id;

	/* ...local client number (think about merging into "id" field - tbd) */
	u32                     client;

	/* ...response processing hook */
	xf_response_cb          response;
};

/*******************************************************************************
 * Auxiliary component helpers
 ******************************************************************************/
/* ...component client-id (global scope) */
static inline u32 xf_handle_id(xf_handle_t *handle)
{
	return handle->id;
}

/* ...pointer to auxiliary buffer */
static inline void * xf_handle_aux(xf_handle_t *handle)
{
	return xf_buffer_data(handle->aux);
}

/* ...put asynchronous response into local IPC */
static inline int xf_response_put(xf_handle_t *handle, xf_user_msg_t *msg)
{
	return xf_ipc_response_put(&handle->ipc, msg);
}

/* ...get asynchronous response from local IPC */
static inline int xf_response_get(xf_handle_t *handle, xf_user_msg_t *msg)
{
	int fd;
	struct pollfd   pollfd;
	u32 timeout = TIMEOUT;
	int ret = 0;

	/* ...read pipe */
	fd = handle->ipc.pipe[0];

	/* ...specify waiting set */
	pollfd.fd = fd;
	pollfd.events = POLLIN | POLLRDNORM;

	/* ...wait until there is a data in file */
	ret = poll(&pollfd, 1, timeout);
	if(ret > 0)
	    return xf_ipc_response_get(&handle->ipc, msg);
	else if(ret < 0)
	    return ret;
	else
	    return -ETIMEDOUT;
}

/* ...put asynchronous response into local IPC, used for no-timely response message */
static inline int xf_response_put_ack(xf_handle_t *handle, xf_user_msg_t *msg)
{
	return xf_ipc_response_put(&handle->ipc_ack, msg);
}

/* ...get asynchronous response from local IPC, used for no-timely response message */
static inline int xf_response_get_ack(xf_handle_t *handle, xf_user_msg_t *msg)
{
	int fd;
	struct pollfd   pollfd;
	u32 timeout = TIMEOUT;
	int ret = 0;

	/* ...read pipe */
	fd = handle->ipc_ack.pipe[0];

	/* ...specify waiting set */
	pollfd.fd = fd;
	pollfd.events = POLLIN | POLLRDNORM;

	/* ...wait until there is a data in file */
	ret = poll(&pollfd, 1, timeout);
	if(ret > 0)
	    return xf_ipc_response_get(&handle->ipc_ack, msg);
	else if(ret < 0)
	    return ret;
	else
	    return -ETIMEDOUT;
}


/*******************************************************************************
 * API functions
 ******************************************************************************/

/* ...send asynchronous command */
extern int  xf_ipc_send(xf_proxy_ipc_data_t *ipc, xf_proxy_msg_t *msg, void *b);

/* ...wait for response from remote proxy */
extern int  xf_ipc_wait(xf_proxy_ipc_data_t *ipc, u32 timeout);

/* ...receive response from IPC layer */
extern int  xf_ipc_recv(xf_proxy_ipc_data_t *ipc, xf_proxy_msg_t *msg, void **b);

/* ...open proxy interface on proper DSP partition */
extern int  xf_ipc_open(xf_proxy_ipc_data_t *proxy, u32 core);

/* ...close proxy handle */
extern void xf_ipc_close(xf_proxy_ipc_data_t *proxy, u32 core);

extern int xf_pool_alloc(xf_proxy_t *proxy, u32 number, u32 length, xf_pool_type_t type, xf_pool_t **pool);

extern int xf_pool_free(xf_pool_t *pool);

extern xf_buffer_t *xf_buffer_get(xf_pool_t *pool);

extern void xf_buffer_put(xf_buffer_t *buffer);

int xf_proxy_cmd_exec(xf_proxy_t *proxy, xf_user_msg_t *msg);

int xf_proxy_cmd(xf_proxy_t *proxy, xf_handle_t *handle, xf_user_msg_t *m);

int xf_proxy_init(xf_proxy_t *proxy, u32 core);

int xf_open(xf_proxy_t *proxy, xf_handle_t *handle, xf_id_t id, u32 core, xf_response_cb response);

void xf_close(xf_handle_t *handle);

int xf_command(xf_handle_t *handle, u32 port, u32 opcode, void *buffer, u32 length);

int xf_route(xf_handle_t *src, u32 src_port, xf_handle_t *dst, u32 dst_port, u32 num, u32 size, u32 align);

int xf_unroute(xf_handle_t *src, u32 src_port);

void xf_proxy_close(xf_proxy_t *proxy);

#endif
