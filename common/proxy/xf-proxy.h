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
 ******************************************************************************/

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

/* ...buffer pool type */
typedef u32							xf_pool_type_t;

/* ...previous declaration of struct */
struct xf_buffer;
struct xf_pool;
struct xf_proxy;
struct xf_handle;

/* ...response callback */
typedef void (*xf_response_cb)(struct xf_handle *h, struct xf_user_msg *msg);

/* ...buffer pool type */
enum xf_pool_type {
	XF_POOL_AUX = 0,
	XF_POOL_INPUT = 1,
	XF_POOL_OUTPUT = 2
};

/*******************************************************************************
 * Types definitions
 ******************************************************************************/
/* ...buffer link pointer */
union xf_buffer_link {
	/* ...pointer to next free buffer in a pool (for free buffer) */
	struct xf_buffer   *next;

	/* ...reference to a buffer pool (for allocated buffer) */
	struct xf_pool     *pool;
};

/* ...buffer descriptor */
struct xf_buffer {
	/* ...virtual address of contiguous buffer */
	void               *address;

	/* ...link pointer */
	union xf_buffer_link    link;
};

/* ...buffer pool */
struct xf_pool {
	/* ...reference to proxy data */
	struct xf_proxy     *proxy;

	/* ...length of individual buffer in a pool */
	u32                 length;

	/* ...number of buffers in a pool */
	u32                 number;

	/* ...pointer to pool memory */
	void               *p;

	/* ...pointer to first free buffer in a pool */
	struct xf_buffer   *free;

	/* ...individual buffers */
	struct xf_buffer   buffer[0];
};

/* ...free clients list */
union xf_proxy_cmap_link {
	/* ...index of next free client in the list */
	u32                     next;

	/* ...pointer to allocated component handle */
	struct xf_handle        *handle;

};

/* ...proxy data structure */
struct xf_proxy {
	/* ...platform-specific IPC data */
	struct xf_proxy_ipc_data     ipc;

	/* ...auxiliary buffer pool for clients */
	struct xf_pool          *aux;

	/* ...global proxy lock */
	pthread_mutex_t         lock;

	/* ...proxy thread handle */
	pthread_t               thread;

	/* ...proxy identifier (core of remote DSP hosting SHMEM interface) */
	u32                     core;

	/* ...client association map */
	union xf_proxy_cmap_link    cmap[XF_CFG_PROXY_MAX_CLIENTS];
};

/*******************************************************************************
 * Auxiliary proxy helpers
 ******************************************************************************/
/* ...lock proxy data */
static inline void xf_proxy_lock(struct xf_proxy *proxy)
{
	__xf_lock(&proxy->lock);
}

/* ...unlock proxy data */
static inline void xf_proxy_unlock(struct xf_proxy *proxy)
{
	__xf_unlock(&proxy->lock);
}

/* ...translate proxy shared address into local virtual address */
static inline void *xf_proxy_a2b(struct xf_proxy *proxy, u32 address)
{
	return xf_ipc_a2b(&proxy->ipc, address);
}

/* ...translate local virtual address into shared proxy address */
static inline u32 xf_proxy_b2a(struct xf_proxy *proxy, void *b)
{
	return xf_ipc_b2a(&proxy->ipc, b);
}

/* ...submit asynchronous response message */
static inline int xf_proxy_response_put(struct xf_proxy *proxy,
					struct xf_proxy_message *msg)
{
	return xf_proxy_ipc_response_put(&proxy->ipc, msg);
}

/* ...retrieve asynchronous response message */
static inline int xf_proxy_response_get(struct xf_proxy *proxy,
					struct xf_proxy_message *msg)
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
	if (ret > 0)
		return xf_proxy_ipc_response_get(&proxy->ipc, msg);
	else if (ret < 0)
		return ret;
	else
		return -ETIMEDOUT;
}

/* ...accessor to buffer data */
static inline void *xf_buffer_data(struct xf_buffer *buffer)
{
	return buffer->address;
}

/* ...length of buffer data */
static inline size_t xf_buffer_length(struct xf_buffer *buffer)
{
	struct xf_pool *pool = buffer->link.pool;

	return (size_t)pool->length;
}

/*******************************************************************************
 * Component handle definition
 ******************************************************************************/
struct xf_handle {
	/* ...platform-specific IPC data */
	struct xf_ipc_data           ipc;

	/* ...platform-specific IPC data, used for no-timely response message */
	struct xf_ipc_data           ipc_ack;

	/* ...reference to proxy data */
	struct xf_proxy             *proxy;

	/* ...component lock */
	pthread_mutex_t        lock;

	/* ...auxiliary control buffer for control transactions */
	struct xf_buffer        *aux;

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
static inline u32 xf_handle_id(struct xf_handle *handle)
{
	return handle->id;
}

/* ...pointer to auxiliary buffer */
static inline void *xf_handle_aux(struct xf_handle *handle)
{
	return xf_buffer_data(handle->aux);
}

/* ...put asynchronous response into local IPC */
static inline int xf_response_put(struct xf_handle *handle,
				  struct xf_user_msg *msg)
{
	return xf_ipc_response_put(&handle->ipc, msg);
}

/* ...get asynchronous response from local IPC */
static inline int xf_response_get(struct xf_handle *handle,
				  struct xf_user_msg *msg)
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
	if (ret > 0)
		return xf_ipc_response_get(&handle->ipc, msg);
	else if (ret < 0)
		return ret;
	else
		return -ETIMEDOUT;
}

/* ...put asynchronous response into local IPC,
 * used for no-timely response message
 */
static inline int xf_response_put_ack(struct xf_handle *handle,
				      struct xf_user_msg *msg)
{
	return xf_ipc_response_put(&handle->ipc_ack, msg);
}

/* ...get asynchronous response from local IPC,
 * used for no-timely response message
 */
static inline int xf_response_get_ack(struct xf_handle *handle,
				      struct xf_user_msg *msg)
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
	if (ret > 0)
		return xf_ipc_response_get(&handle->ipc_ack, msg);
	else if (ret < 0)
		return ret;
	else
		return -ETIMEDOUT;
}

/* ...get asynchronous response message count,
 * used for no-timely response message
 */
static inline int xf_response_get_ack_count(struct xf_handle *handle)
{
	return xf_ipc_response_count(&handle->ipc_ack);
}

/*******************************************************************************
 * API functions
 ******************************************************************************/

/* ...send asynchronous command */
int  xf_ipc_send(struct xf_proxy_ipc_data *ipc,
		 struct xf_proxy_message *msg,
		 void *b);

/* ...wait for response from remote proxy */
int  xf_ipc_wait(struct xf_proxy_ipc_data *ipc, u32 timeout);

/* ...receive response from IPC layer */
int  xf_ipc_recv(struct xf_proxy_ipc_data *ipc,
		 struct xf_proxy_message *msg,
		 void **b);

/* ...open proxy interface on proper DSP partition */
int  xf_ipc_open(struct xf_proxy_ipc_data *proxy, u32 core);

/* ...close proxy handle */
void xf_ipc_close(struct xf_proxy_ipc_data *proxy, u32 core);

int xf_pool_alloc(struct xf_proxy *proxy,
		  u32 number,
		  u32 length,
		  xf_pool_type_t type,
		  struct xf_pool **pool);

int xf_pool_free(struct xf_pool *pool);

struct xf_buffer *xf_buffer_get(struct xf_pool *pool);

void xf_buffer_put(struct xf_buffer *buffer);

int xf_proxy_cmd_exec(struct xf_proxy *proxy, struct xf_user_msg *msg);

int xf_proxy_cmd(struct xf_proxy *proxy,
		 struct xf_handle *handle,
		 struct xf_user_msg *m);

int xf_proxy_init(struct xf_proxy *proxy, u32 core);

int xf_open(struct xf_proxy *proxy,
	    struct xf_handle *handle,
	    const char *id,
	    u32 core,
	    xf_response_cb response);

void xf_close(struct xf_handle *handle);

int xf_command(struct xf_handle *handle,
	       u32 port,
	       u32 opcode,
	       void *buffer,
	       u32 length);

int xf_route(struct xf_handle *src,
	     u32 src_port,
	     struct xf_handle *dst,
	     u32 dst_port,
	     u32 num,
	     u32 size,
	     u32 align);

int xf_unroute(struct xf_handle *src, u32 src_port);

void xf_proxy_close(struct xf_proxy *proxy);

#endif
