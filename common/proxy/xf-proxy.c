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
 *
 ******************************************************************************/

/*******************************************************************************
 * xf-proxy.c
 *
 * Xtensa audio processing framework; application side
 ******************************************************************************/

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "xf-proxy.h"
#include "xf-ipc.h"
#include "xf-opcode.h"
#include "xf-osal.h"

/*******************************************************************************
 * Internal functions definitions
 ******************************************************************************/

/* ...execute proxy command synchronously */
int xf_proxy_cmd_exec(struct xf_proxy *proxy, struct xf_user_msg *msg)
{
	struct xf_proxy_message  m;

	/* ...send command to remote proxy */
	m.session_id = msg->id;
	m.opcode = msg->opcode;
	m.length = msg->length;
	m.ret = msg->ret;

	/* ...translate address */
	XF_CHK_ERR((m.address = xf_proxy_b2a(proxy, msg->buffer)) !=
				proxy->ipc.shmem_size, -EINVAL);

	/* ...pass command to remote proxy */
	XF_CHK_API(xf_ipc_send(&proxy->ipc, &m, msg->buffer));

	/* ...wait for response reception indication from proxy thread */
	XF_CHK_API(xf_proxy_response_get(proxy, &m));

	/* ...copy parameters */
	msg->id = m.session_id;
	msg->opcode = m.opcode;
	msg->length = m.length;
	msg->ret = m.ret;

	/* ...translate address back to virtual space */
	if (msg->opcode != XF_SHMEM_INFO) {
		XF_CHK_ERR((msg->buffer = xf_proxy_a2b(proxy, m.address)) !=
			   (void *)-1, -EBADFD);
	}

	TRACE("proxy[%p]: command done: [%08x:%p:%u:%u]\n",
	      proxy,
	      msg->opcode,
	      msg->buffer,
	      msg->length,
	      msg->ret);

	return 0;
}

/* ...pass command to remote DSP */
int xf_proxy_cmd(struct xf_proxy *proxy,
		 struct xf_handle *handle,
		 struct xf_user_msg *m)
{
	struct xf_proxy_message  msg;

	/* ...set session-id of the message */
	msg.session_id = __XF_MSG_ID(__XF_AP_CLIENT(0, handle->client), m->id);
	msg.opcode = m->opcode;
	msg.length = m->length;
	msg.ret = m->ret;

	/* ...translate buffer pointer to shared address */
	XF_CHK_ERR((msg.address = xf_proxy_b2a(proxy, m->buffer)) !=
				proxy->ipc.shmem_size, -EINVAL);

	/* ...submit command message to IPC layer */
	return XF_CHK_API(xf_ipc_send(&proxy->ipc, &msg, m->buffer));
}

/* ...allocate local client-id number */
static inline u32 xf_client_alloc(struct xf_proxy *proxy,
				  struct xf_handle *handle)
{
	u32     client;

	client = proxy->cmap[0].next;
	if (client) {
		/* ...pop client from free clients list */
		proxy->cmap[0].next = proxy->cmap[client].next;

		/* ...put client handle into association map */
		handle->client = client, proxy->cmap[client].handle = handle;
	}

	return client;
}

/* ...recycle local client-id number */
static inline void xf_client_free(struct xf_proxy *proxy,
				  struct xf_handle *handle)
{
	u32     client = handle->client;

	/* ...push client into head of the free clients list */
	proxy->cmap[client].next = proxy->cmap[0].next;

	/* ...adjust head of free clients */
	proxy->cmap[0].next = client;
}

/* ...lookup client basing on its local id */
static inline struct xf_handle *xf_client_lookup(struct xf_proxy *proxy,
						 u32 client)
{
	/* ...client index must be in proper range */
	BUG(client >= XF_CFG_PROXY_MAX_CLIENTS,
	    "Invalid client index: %u", client);

	/* ...check if client index is small */
	if (proxy->cmap[client].next < XF_CFG_PROXY_MAX_CLIENTS)
		return NULL;
	else
		return proxy->cmap[client].handle;
}

/* ...create new client on remote core */
static inline int xf_client_register(struct xf_proxy *proxy,
				     struct xf_handle *handle,
				     const char *id,
				     u32 core)
{
	void *b = xf_handle_aux(handle);
	struct xf_user_msg   msg;

	/* ...set session-id: source is local proxy,
	 * destination is remote proxy
	 */
	msg.id = __XF_MSG_ID(__XF_AP_PROXY(0), __XF_DSP_PROXY(0));
	msg.opcode = XF_REGISTER;
	msg.buffer = b;
	msg.length = strlen(id) + 1;
	msg.ret = 0;

	/* ...copy component identifier */
	memcpy(b, (void *)id, xf_buffer_length(handle->aux));

	/* ...execute command synchronously */
	XF_CHK_API(xf_proxy_cmd_exec(proxy, &msg));

	/* ...check operation is successfull */
	XF_CHK_ERR(msg.opcode == XF_REGISTER, -EFAULT);

	/* ...save received component global client-id */
	handle->id = XF_MSG_SRC(msg.id);

	TRACE("[%p]=[%s:%u:%u]\n",
	      handle,
	      id,
	      XF_PORT_CORE(handle->id),
	      XF_PORT_CLIENT(handle->id));

	return 0;
}

/* ...unregister client from remote proxy */
static inline int xf_client_unregister(struct xf_proxy *proxy,
				       struct xf_handle *handle)
{
	struct xf_user_msg msg;

	/* ...make sure the client is consistent */
	BUG(proxy->cmap[handle->client].handle != handle,
	    "Invalid handle: %p", handle);

	/* ...set message parameters */
	msg.id = __XF_MSG_ID(__XF_AP_PROXY(0), handle->id);
	msg.opcode = XF_UNREGISTER;
	msg.buffer = NULL;
	msg.length = 0;
	msg.ret = 0;

	/* ...synchronously execute command on remote proxy */
	XF_CHK_API(xf_proxy_cmd_exec(proxy, &msg));

	/* ...opcode must be XF_UNREGISTER - tbd */
	BUG(msg.opcode != XF_UNREGISTER, "Invalid opcode: %X", msg.opcode);

	TRACE("%p[%u:%u] unregistered\n",
	      handle,
		  XF_PORT_CORE(handle->id),
		  XF_PORT_CLIENT(handle->id));

	return 0;
}

/* ...allocate shared buffer */
static inline int xf_proxy_buffer_alloc(struct xf_proxy *proxy,
					u32 length,
					void **buffer)
{
	struct xf_user_msg   msg;

	/* ...prepare command parameters */
	msg.id = __XF_MSG_ID(__XF_AP_PROXY(0), __XF_DSP_PROXY(0));
	msg.opcode = XF_ALLOC;
	msg.length = length;
	msg.buffer = NULL;
	msg.ret = 0;

	/* ...synchronously execute command on remote proxy */
	XF_CHK_API(xf_proxy_cmd_exec(proxy, &msg));

	/* ...check if response is valid */
	XF_CHK_ERR(msg.opcode == XF_ALLOC, -EBADFD);

	/* ...check if allocation is successful */
	XF_CHK_ERR(msg.buffer != NULL, -ENOMEM);

	/* ...save output parameter */
	*buffer = msg.buffer;

	TRACE("proxy: allocated [%p:%u]\n", *buffer, length);

	return 0;
}

/* ...free shared AP-DSP memory */
static inline int xf_proxy_buffer_free(struct xf_proxy *proxy,
				       void *buffer,
				       u32 length)
{
	struct xf_user_msg   msg;

	/* ...prepare command parameters */
	msg.id = __XF_MSG_ID(__XF_AP_PROXY(0), __XF_DSP_PROXY(0));
	msg.opcode = XF_FREE;
	msg.length = length;
	msg.buffer = buffer;
	msg.ret = 0;

	/* ...synchronously execute command on remote proxy */
	XF_CHK_API(xf_proxy_cmd_exec(proxy, &msg));

	/* ...check if response is valid */
	XF_CHK_ERR(msg.opcode == XF_FREE, -EBADFD);

	TRACE("proxy: free [%p:%u]\n", buffer, length);

	return 0;
}

/*******************************************************************************
 * Proxy interface asynchronous receiving thread
 ******************************************************************************/

static void *xf_proxy_thread(void *arg)
{
	struct xf_proxy     *proxy = arg;
	struct xf_handle    *client;
	int             r;

	TRACE("xf_proxy_thread start\n");

	/* ...start polling thread */
	while (1) {
		struct xf_proxy_message  m;
		struct xf_user_msg   msg;

		/* wait for response from remote proxy */
		r = xf_ipc_wait(&proxy->ipc, -1);
		if (r < 0)
			break;

		/* ...retrieve all responses received */
		while ((r = xf_ipc_recv(&proxy->ipc, &m, &msg.buffer)) ==
					sizeof(m)) {
			/* ...make sure we have proper core identifier
			 * of SHMEM interface
			 */
			BUG(XF_MSG_DST_CORE(m.id) != proxy->core,
			    "Invalid session-id: %X", m.session_id);

			/* ...make sure translation is successful */
			BUG(msg.buffer == (void *)-1,
			    "Invalid buffer address: %08x", m.address);

			/* ...retrieve information fields */
			msg.id = XF_MSG_SRC(m.session_id);
			msg.opcode = m.opcode;
			msg.length = m.length;
			msg.ret = m.ret;

			TRACE("R[%08x]:(%08x,%u,%08x,%u)\n",
			      m.session_id,
			      m.opcode,
			      m.length,
			      m.address,
			      m.ret);

			/* ...lookup component basing on destination
			 * port specification
			 */
			if (XF_AP_CLIENT(m.session_id) == 0) {
				/* ...put proxy response to local IPC queue */
				xf_proxy_response_put(proxy, &m);
			} else if ((client = xf_client_lookup(proxy,
					XF_AP_CLIENT(m.session_id))) != NULL) {
				/* ...client is found; invoke its
				 * response callback (must be non-blocking)
				 */
				client->response(client, &msg);
			} else {
				/* ...client has been disconnected already;
				 * drop message
				 */
				TRACE("Client look-up failed - drop message\n");
			}
		}

		/* ...if result code is negative; terminate thread operation */
		if (r < 0) {
			TRACE("abnormal proxy[%p] thread termination: %d\n",
			      proxy, r);
			break;
		}
	}

	TRACE("IPC proxy[%p] thread terminated: %d\n", proxy, r);

	return (void *)(intptr_t)r;
}

/*******************************************************************************
 * DSP proxy API
 ******************************************************************************/

void sighand(int signo)
{
	pthread_exit(NULL);
}

/* ...open DSP proxy */
int xf_proxy_init(struct xf_proxy *proxy, u32 core)
{
	u32             i;
	int             r;
	int                     rc;
	struct sigaction        actions;
	struct xf_user_msg   msg;

	/* ...initialize proxy lock */
	__xf_lock_init(&proxy->lock);

	memset(&actions, 0, sizeof(actions));
	sigemptyset(&actions.sa_mask);
	actions.sa_flags = 0;
	actions.sa_handler = sighand;

	/* set the handle function of SIGUSR1 */
	rc = sigaction(SIGUSR1, &actions, NULL);

	/* ...open proxy IPC interface */
	XF_CHK_API(xf_ipc_open(&proxy->ipc, 0));

	/* ...save proxy core - hmm, too much core identifiers - tbd */
	proxy->core = 0;

	/* ...line-up all clients into single-linked list */
	for (i = 0; i < XF_CFG_PROXY_MAX_CLIENTS - 1; i++)
		proxy->cmap[i].next = i + 1;

	/* ...tail of the list points back to head (list terminator) */
	proxy->cmap[i].next = 0;

	/* ...initialize thread attributes (joinable, with minimal stack) */
	r = __xf_thread_create(&proxy->thread, xf_proxy_thread, proxy);
	if (r < 0) {
		TRACE("Failed to create polling thread: %d\n", r);
		xf_ipc_close(&proxy->ipc, 0);
		return r;
	}

	TRACE("proxy [%p] opened\n", proxy);

	/* ...prepare command parameters */
	msg.id = __XF_MSG_ID(__XF_AP_PROXY(0), __XF_DSP_PROXY(0));
	msg.opcode = XF_SHMEM_INFO;
	msg.length = 0;
	msg.buffer = 0;
	msg.ret = 0;

	XF_CHK_API(xf_proxy_cmd_exec(proxy, &msg));

	TRACE("Got shmem info\n", proxy);
	return 0;
}

/* ...close proxy handle */
void xf_proxy_close(struct xf_proxy *proxy)
{
	u32     core = proxy->core;

	/* ...unmap shared memory region */
	(void)munmap(&proxy->ipc.shmem, proxy->ipc.shmem_size);

	/* ...terminate proxy thread */
	__xf_thread_destroy(&proxy->thread);

	/* ...close proxy IPC interface */
	xf_ipc_close(&proxy->ipc, core);

	TRACE("proxy [%p] closed\n", proxy);
}

/*******************************************************************************
 * DSP component API
 ******************************************************************************/

/* ...open component handle */
int xf_open(struct xf_proxy *proxy,
	    struct xf_handle *handle,
	    const char *id,
	    u32 core,
	    xf_response_cb response)
{
	int     r;

	/* ...initialize component handle lock */
	__xf_lock_init(&handle->lock);

	/* ...retrieve auxiliary control buffer from proxy - need I */
	XF_CHK_ERR(handle->aux = xf_buffer_get(proxy->aux), -EBUSY);

	/* ...initialize IPC data */
	XF_CHK_API(xf_ipc_data_init(&handle->ipc));

	/* ...initialize IPC data, used for no-timely response message */
	XF_CHK_API(xf_ipc_data_init(&handle->ipc_ack));

	/* ...register client in interlocked fashion */
	xf_proxy_lock(proxy);

	/* ...allocate local client */
	if (xf_client_alloc(proxy, handle) == 0) {
		TRACE("client allocation failed\n");
		r = -EBUSY;
	} else if ((r = xf_client_register(proxy, handle, id, 0)) < 0) {
		TRACE("client registering failed\n");
		xf_client_free(proxy, handle);
	}

	xf_proxy_unlock(proxy);

	/* ...if failed, release buffer handle */
	if (r < 0) {
		/* ...operation failed; return buffer back to proxy pool */
		xf_buffer_put(handle->aux), handle->aux = NULL;
	} else {
		/* ...operation completed successfully; assign handle data */
		handle->response = response;
		handle->proxy = proxy;

		TRACE("component[%p]:(id=%s) created\n", handle, id);
	}

	return XF_CHK_API(r);
}

/* ...close component handle */
void xf_close(struct xf_handle *handle)
{
	struct xf_proxy *proxy = handle->proxy;

	/* ...do I need to take component lock here? guess no - tbd */

	/* ...buffers and stuff? - tbd */

	/* ...acquire global proxy lock */
	xf_proxy_lock(proxy);

	/* ...unregister component from remote DSP proxy (ignore result code) */
	(void)xf_client_unregister(proxy, handle);

	/* ...recycle client-id afterwards */
	xf_client_free(proxy, handle);

	/* ...release global proxy lock */
	xf_proxy_unlock(proxy);

	/* ...destroy IPC data */
	xf_ipc_data_destroy(&handle->ipc);

	/* ...destroy IPC data, used for no-timely response message */
	xf_ipc_data_destroy(&handle->ipc_ack);

	/* ...clear handle data */
	xf_buffer_put(handle->aux), handle->aux = NULL;

	/* ...wipe out proxy pointer */
	handle->proxy = NULL;

	TRACE("component[%p] destroyed\n", handle);
}

/* ...port binding function */
int xf_route(struct xf_handle *src,
	     u32 src_port,
	     struct xf_handle *dst,
	     u32 dst_port,
	     u32 num,
	     u32 size,
	     u32 align)
{
	struct xf_proxy        *proxy = src->proxy;
	struct xf_buffer       *b;
	struct xf_route_port_msg *m;
	struct xf_user_msg     msg;

	/* ...sanity checks - proxy pointers are same */
	XF_CHK_ERR(proxy == dst->proxy, -EINVAL);

	/* ...buffer data is sane */
	XF_CHK_ERR(num && size && xf_is_power_of_two(align), -EINVAL);

	/* ...get control buffer */
	XF_CHK_ERR(b = xf_buffer_get(proxy->aux), -EBUSY);

	/* ...get message buffer */
	m = xf_buffer_data(b);

	/* ...fill-in message parameters */
	m->src = __XF_PORT_SPEC2(src->id, src_port);
	m->dst = __XF_PORT_SPEC2(dst->id, dst_port);
	m->alloc_number = num;
	m->alloc_size = size;
	m->alloc_align = align;

	/* ...set command parameters */
	msg.id = __XF_MSG_ID(__XF_AP_PROXY(0),
				__XF_PORT_SPEC2(src->id, src_port));
	msg.opcode = XF_ROUTE;
	msg.length = sizeof(*m);
	msg.buffer = m;
	msg.ret = 0;

	/* ...synchronously execute command on remote DSP */
	XF_CHK_API(xf_proxy_cmd_exec(proxy, &msg));

	/* ...return buffer to proxy */
	xf_buffer_put(b);

	/* ...check result is successfull */
	XF_CHK_ERR(msg.opcode == XF_ROUTE, -ENOMEM);

	/* ...port binding completed */
	TRACE("[%p]:%u bound to [%p]:%u\n", src, src_port, dst, dst_port);

	return 0;
}

/* ...port unbinding function */
int xf_unroute(struct xf_handle *src, u32 src_port)
{
	struct xf_proxy        *proxy = src->proxy;
	struct xf_buffer       *b;
	struct xf_unroute_port_msg  *m;
	struct xf_user_msg     msg;
	int                     r;

	/* ...get control buffer */
	XF_CHK_ERR(b = xf_buffer_get(proxy->aux), -EBUSY);

	/* ...get message buffer */
	m = xf_buffer_data(b);

	/* ...fill-in message parameters */
	m->src = __XF_PORT_SPEC2(src->id, src_port);

	/* ...set command parameters */
	msg.id = __XF_MSG_ID(__XF_AP_PROXY(proxy->core),
				__XF_PORT_SPEC2(src->id, src_port));
	msg.opcode = XF_UNROUTE;
	msg.length = sizeof(*m);
	msg.buffer = m;
	msg.ret = 0;

	/* ...synchronously execute command on remote DSP */
	r = xf_proxy_cmd_exec(proxy, &msg);
	if (r) {
		TRACE("Command failed: %d\n", r);
		goto out;
	} else if (msg.opcode != XF_UNROUTE) {
		TRACE("Port unbinding failed\n");
		r = -EBADFD;
		goto out;
	}

	/* ...port binding completed */
	TRACE("[%p]:%u unbound\n", src, src_port);

out:
	/* ...return buffer to proxy */
	xf_buffer_put(b);

	return r;
}

/* ...send a command message to component */
int xf_command(struct xf_handle *handle,
	       u32 port,
	       u32 opcode,
	       void *buffer,
	       u32 length)
{
	struct xf_proxy *proxy = handle->proxy;
	struct xf_proxy_message  msg;

	/* ...fill-in message parameters */
	msg.session_id = __XF_MSG_ID(__XF_AP_CLIENT(0, handle->client),
				__XF_PORT_SPEC2(handle->id, port));
	msg.opcode = opcode;
	msg.length = length;
	XF_CHK_ERR((msg.address = xf_proxy_b2a(proxy, buffer)) !=
				proxy->ipc.shmem_size, -EINVAL);
	msg.ret = 0;

	TRACE("[%p]:[%08x]:(%08x,%u,%p)\n",
	      handle,
		  msg.session_id,
		  opcode,
		  length,
		  buffer);

	/* ...pass command to IPC layer */
	return XF_CHK_API(xf_ipc_send(&proxy->ipc, &msg, buffer));
}

/*******************************************************************************
 * Buffer pool API
 ******************************************************************************/

/* ...allocate buffer pool */
int xf_pool_alloc(struct xf_proxy *proxy,
		  u32 number,
		  u32 length,
		  xf_pool_type_t type,
		  struct xf_pool **pool)
{
	struct xf_pool      *p;
	struct xf_buffer    *b;
	void           *data;
	int             r;

	/* ...basic sanity checks; number of buffers is positive */
	XF_CHK_ERR(number > 0, -EINVAL);

	/* ...get properly aligned buffer length */
	length = (length + XF_PROXY_ALIGNMENT - 1) & ~(XF_PROXY_ALIGNMENT - 1);

	/* ...allocate data structure */
	XF_CHK_ERR(p = malloc(offset_of(struct xf_pool, buffer) +
				number * sizeof(struct xf_buffer)), -ENOMEM);

	/* ...issue memory pool allocation request to remote DSP */
	xf_proxy_lock(proxy);
	r = xf_proxy_buffer_alloc(proxy, number * length, &p->p);
	xf_proxy_unlock(proxy);

	/* ...if operation is failed, do cleanup */
	if (r < 0) {
		TRACE("failed to allocate buffer: %d\n", r);
		free(p);
		return r;
	}

	/* ...set pool parameters */
	p->number = number, p->length = length;
	p->proxy = proxy;

	/* ...create individual buffers and link them into free list */
	for (p->free = b = &p->buffer[0], data = p->p; number > 0;
				number--, b++) {
		/* ...set address of the buffer (no length there) */
		b->address = data;

		/* ...file buffer into the free list */
		b->link.next = b + 1;

		/* ...advance data pointer in contiguous buffer */
		data += length;
	}

	/* ...terminate list of buffers (not too good - tbd) */
	b[-1].link.next = NULL;

	TRACE("[%p]: pool[%p] created: %u * %u\n",
	      proxy,
		  p,
		  p->number,
		  p->length);

	/* ...return buffer pointer */
	*pool = p;

	return 0;
}

/* ...buffer pool destruction */
int xf_pool_free(struct xf_pool *pool)
{
	struct xf_proxy     *proxy;

	/* ...basic sanity checks; pool is positive */
	XF_CHK_ERR(pool != NULL, -EINVAL);

	/* ...get proxy pointer */
	XF_CHK_ERR((proxy = pool->proxy) != NULL, -EINVAL);

	/* ...use global proxy lock for pool operations protection */
	xf_proxy_lock(proxy);

	/* ...release allocated buffer on remote DSP */
	xf_proxy_buffer_free(proxy, pool->p, pool->length * pool->number);

	/* ...release global proxy lock */
	xf_proxy_unlock(proxy);

	/* ...deallocate pool structure itself */
	free(pool);

	TRACE("[%p]::pool[%p] destroyed\n", proxy, pool);

	return 0;
}

/* ...get new buffer from a pool */
struct xf_buffer *xf_buffer_get(struct xf_pool *pool)
{
	struct xf_buffer    *b;

	/* ...use global proxy lock for pool operations protection */
	xf_proxy_lock(pool->proxy);

	/* ...take buffer from a head of the free list */
	b = pool->free;
	if (b) {
		/* ...advance free list head */
		pool->free = b->link.next, b->link.pool = pool;

		TRACE("pool[%p]::get[%p]\n", pool, b);
	}

	xf_proxy_unlock(pool->proxy);

	return b;
}

/* ...return buffer back to pool */
void xf_buffer_put(struct xf_buffer *buffer)
{
	struct xf_pool  *pool = buffer->link.pool;

	/* ...use global proxy lock for pool operations protection */
	xf_proxy_lock(pool->proxy);

	/* ...put buffer back to a pool */
	buffer->link.next = pool->free, pool->free = buffer;

	TRACE("pool[%p]::put[%p]\n", pool, buffer);

	xf_proxy_unlock(pool->proxy);
}
