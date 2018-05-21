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
 * xf-fio.c
 *
 * System-specific IPC layer implementation for file I/O configuration
 ******************************************************************************/

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <poll.h>
#include <fcntl.h>
#include "xf-proxy.h"
#include "mxc_dsp.h"

/*******************************************************************************
 * Internal IPC API implementation
 ******************************************************************************/

/* ...pass command to remote DSP */
int xf_ipc_send(struct xf_proxy_ipc_data *ipc,
		struct xf_proxy_message *msg,
		void *b)
{
	int     fd = ipc->fd;
	int ret;

	/* ...pass message to kernel driver */
	ret = ioctl(fd, DSP_IPC_MSG_SEND, msg);

	/* ...communication mutex is still locked! */
	return ret;
}

/* ...wait for response availability */
int xf_ipc_wait(struct xf_proxy_ipc_data *ipc, u32 timeout)
{
	int             fd = ipc->fd;
	struct pollfd   pollfd;
	int ret;

	/* ...specify waiting set */
	pollfd.fd = fd;
	pollfd.events = POLLIN | POLLRDNORM;

	/* ...wait until there is a data in file */
	ret = poll(&pollfd, 1, timeout);
	if (ret < 0)
		return ret;
	else if (ret == 0)
		return -ETIMEDOUT;

	return 0;
}

/* ...read response from proxy */
int xf_ipc_recv(struct xf_proxy_ipc_data *ipc,
		struct xf_proxy_message *msg,
		void **buffer)
{
	int     fd = ipc->fd;
	int     r;
	struct xf_proxy_message temp;

	/* ...get message header from file */
	r = ioctl(fd, DSP_IPC_MSG_RECV, &temp);
	if (r == 0) {
		msg->session_id = temp.session_id;
		msg->opcode = temp.opcode;
		msg->length = temp.length;
		*buffer = xf_ipc_a2b(ipc, temp.address);
		/* ...translate shared address into local pointer */
		XF_CHK_ERR((*buffer = xf_ipc_a2b(ipc, temp.address)) !=
					(void *)-1, -EBADFD);
		msg->address = temp.address;
		msg->ret = temp.ret;

		/* ...return positive result indicating the message
		 * has been received.
		 */
		return sizeof(*msg);
	}

	/* ...if no response is available, return 0 result */
	return XF_CHK_API(errno == EAGAIN ? 0 : -errno);
}

/*******************************************************************************
 * Internal API functions implementation
 ******************************************************************************/

/* ...open proxy interface on proper DSP partition */
int xf_ipc_open(struct xf_proxy_ipc_data *ipc, u32 core)
{
	struct shmem_info mem_info;
	int ret;

	/* ...open file handle */
	XF_CHK_ERR((ipc->fd = open("/dev/mxc_hifi4", O_RDWR)) >= 0, -errno);

	/* ...pass shread memory core for this proxy instance */
	XF_CHK_ERR(ioctl(ipc->fd, DSP_CLIENT_REGISTER, 0) >= 0, -errno);

	/* ...create pipe for asynchronous response delivery */
	XF_CHK_ERR(pipe(ipc->pipe) == 0, -errno);

	/* ...get physical address and size of shared memory */
	ret = ioctl(ipc->fd, DSP_GET_SHMEM_INFO, &mem_info);
	if (ret) {
		TRACE("get physical address and size failed\n");
		return ret;
	}
	ipc->shmem_phys = mem_info.phys_addr;
	ipc->shmem_size = mem_info.size;

	/* ...map entire shared memory region (not too good - tbd) */
	XF_CHK_ERR((ipc->shmem = mmap(NULL,
				      ipc->shmem_size,
				      PROT_READ | PROT_WRITE,
				      MAP_SHARED,
				      ipc->fd,
				      0)) != MAP_FAILED, -errno);

	TRACE("proxy interface opened\n");

	return 0;
}

/* ...close proxy handle */
void xf_ipc_close(struct xf_proxy_ipc_data *ipc, u32 core)
{
	/* ...unmap shared memory region */
	(void)munmap(ipc->shmem, XF_CFG_REMOTE_IPC_POOL_SIZE);

	/* ...close asynchronous response delivery pipe */
	close(ipc->pipe[0]), close(ipc->pipe[1]);

	/* ...close proxy file handle */
	close(ipc->fd);

	TRACE("proxy interface closed\n");
}
