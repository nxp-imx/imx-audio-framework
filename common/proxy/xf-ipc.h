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

#ifndef __XF_IPC_H
#define __XF_IPC_H

#include "xf-types.h"
#include "xf-osal.h"

/* ...NULL-address specification */
#define XF_PROXY_NULL       (~0U)

/* ...response timeout, 10s */
#define TIMEOUT   10000

/*******************************************************************************
 * Types definitions
 ******************************************************************************/
/* ...need that at all? hope no */
struct xf_user_msg {
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
struct xf_proxy_message {
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

/* ...proxy IPC data */
struct xf_proxy_ipc_data {
	/* ...shared memory buffer pointer */
	void                   *shmem;

	/* ...shared memory physical address */
	unsigned int           shmem_phys;

	/* ...shared memory size */
	unsigned int           shmem_size;

	/* ...file descriptor */
	int                     fd;

	/* ...file descriptor "/dev/mem" */
	int                     fd_mem;

	/* ...file descriptor "/dev/rpmsg_ctrl" */
	int                     fd_ctrl;

	int                     rproc_id;

	/* ...pipe for asynchronous response delivery */
	int                     pipe[2];

};

/*******************************************************************************
 * Helpers for asynchronous response delivery
 ******************************************************************************/

#define xf_proxy_ipc_response_put(ipc, msg) \
	(write((ipc)->pipe[1],                  \
		   (msg),                           \
		   sizeof(*(msg))) == sizeof(*(msg)) ? 0 : -errno)

#define xf_proxy_ipc_response_get(ipc, msg) \
	(read((ipc)->pipe[0],                   \
		  (msg),                            \
		  sizeof(*(msg))) == sizeof(*(msg)) ? 0 : -errno)

/*******************************************************************************
 * Shared memory translation
 ******************************************************************************/

/* ...translate proxy shared address into local virtual address */
static inline void *xf_ipc_a2b(struct xf_proxy_ipc_data *ipc, u32 address)
{
	if (address < ipc->shmem_size)
		return (unsigned char *)ipc->shmem + address;
	else if (address == XF_PROXY_NULL)
		return NULL;
	else
		return (void *)(-1);
}

/* ...translate local virtual address into shared proxy address */
static inline u32 xf_ipc_b2a(struct xf_proxy_ipc_data *ipc, void *b)
{
	u32     a;

	if (!b)
		return XF_PROXY_NULL;

	a = (u32)((u8 *)b - (u8 *)ipc->shmem);
	if (a < ipc->shmem_size)
		return a;
	else
		return ipc->shmem_size;
}

/*******************************************************************************
 * Component inter-process communication
 ******************************************************************************/

struct xf_ipc_data {
	/* ...asynchronous response delivery pipe */
	int                 pipe[2];

	/* ...message count in pipe */
	int                 count;

	/* ...message count lock */
	pthread_mutex_t     lock;

};

/*******************************************************************************
 * Helpers for asynchronous response delivery
 ******************************************************************************/

static inline int xf_ipc_response_put(struct xf_ipc_data *ipc,
				      struct xf_user_msg *msg)
{
	__xf_lock(&ipc->lock);
	ipc->count++;
	__xf_unlock(&ipc->lock);

	if (write(ipc->pipe[1], msg, sizeof(*msg)) == sizeof(*msg))
		return 0;
	else
		return -errno;
}

static inline int xf_ipc_response_get(struct xf_ipc_data *ipc,
				      struct xf_user_msg *msg)
{
	if (read(ipc->pipe[0], msg, sizeof(*msg)) == sizeof(*msg)) {
		__xf_lock(&ipc->lock);
		ipc->count--;
		__xf_unlock(&ipc->lock);

		return 0;
	}

	return -errno;
}

static inline int xf_ipc_response_count(struct xf_ipc_data *ipc)
{
	int count = 0;

	__xf_lock(&ipc->lock);
	count = ipc->count;
	__xf_unlock(&ipc->lock);

	return count;
}

static inline int xf_ipc_data_init(struct xf_ipc_data *ipc)
{
	/* ...initialize pipe */
	if (pipe(ipc->pipe) == 0)
		return 0;
	else
		return -errno;

	/* ...initialize message count */
	ipc->count = 0;

	/* ...lock initialization */
	__xf_lock_init(&ipc->lock);
}

static inline void xf_ipc_data_destroy(struct xf_ipc_data *ipc)
{
	close(ipc->pipe[0]);
	close(ipc->pipe[1]);

	ipc->count = 0;
}

#endif
