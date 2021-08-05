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

struct rpmsg_endpoint_info {
	char name[32];
	uint32_t src;
	uint32_t dst;
};

#define RPMSG_CREATE_EPT_IOCTL  _IOW(0xb5, 0x1, struct rpmsg_endpoint_info)

#define RPMSG_DESTROY_EPT_IOCTL _IO(0xb5, 0x2)

#define EPT_NUM 2

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
	ret = write(fd, msg, sizeof(*msg));
	if (ret < 0)
		return -errno;
	/* ...communication mutex is still locked! */
	return 0;
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
	r = read(fd, &temp, sizeof(struct xf_proxy_message));
	if (r == sizeof(struct xf_proxy_message)) {
		msg->session_id = temp.session_id;
		msg->opcode = temp.opcode;
		msg->length = temp.length;

		if (msg->opcode == XF_SHMEM_INFO) {
			ipc->shmem_phys = temp.address;
			ipc->shmem_size = temp.length;

			/* ...map entire shared memory region (not too good - tbd) */
			XF_CHK_ERR((ipc->shmem = mmap(NULL,
						ipc->shmem_size,
						PROT_READ | PROT_WRITE,
						MAP_SHARED,
						ipc->fd_mem,
						temp.address & (~(0x1000 - 1)))) != MAP_FAILED, -errno);

			msg->address = temp.address;
			msg->ret = temp.ret;
		} else {

			*buffer = xf_ipc_a2b(ipc, temp.address);
			/* ...translate shared address into local pointer */
			XF_CHK_ERR((*buffer = xf_ipc_a2b(ipc, temp.address)) !=
					(void *)-1, -EBADFD);
			msg->address = temp.address;
			msg->ret = temp.ret;
		}

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
int file_write(char *path, char *str)
{
	int fd;
	ssize_t bytes_written;
	size_t str_sz;

	fd = open(path, O_WRONLY);
	if (fd == -1) {
		perror("Error");
		return -1;
	}
	str_sz = strlen(str);
	bytes_written = write(fd, str, str_sz);
	if (bytes_written != str_sz) {
		if (bytes_written == -1)
			perror("Error");
		close(fd);
		return -1;
	}

	if (-1 == close(fd)) {
		perror("Error");
		return -1;
	}
	return 0;
}

int file_read(char *path, char *str, size_t len)
{
	int fd;
	ssize_t bytes_read;
	size_t str_sz;

	fd = open(path, O_RDONLY);
	if (fd == -1) {
		perror("Error");
		return -1;
	}

	bytes_read = read(fd, str, len);
	if (bytes_read != len) {
		if (bytes_read == -1)
			perror("Error");
		close(fd);
		return 0;
	}

	if (-1 == close(fd)) {
		perror("Error");
		return -1;
	}

	return 0;
}

int xf_rproc_open(struct xf_proxy_ipc_data *ipc)
{
	struct rpmsg_endpoint_info eptinfo;
	char path_buf[512];
	char sbuf[32];
	int  found = 0;
	int rproc_flag = 0;
	int  i, j, fd, ret;

	ipc->rproc_id = -1;

	for (i = 0; i < 10; i++) {
		memset(path_buf, 0, 512);
		memset(sbuf, 0, 32);
		sprintf(path_buf, "/sys/class/remoteproc/remoteproc%u/name", i);

		if (access(path_buf, F_OK) < 0)
			continue;
		if (file_read(path_buf, sbuf, 32) < 0)
			continue;
		if (!strncmp(sbuf, "imx-dsp-rproc", 13)) {
			found = 1;
			ipc->rproc_id = i;
			break;
		}
	}

	if (!found)
		return -1;

	memset(path_buf, 0, 512);
	memset(sbuf, 0, 32);
	sprintf(path_buf, "/sys/class/remoteproc/remoteproc%u/state", ipc->rproc_id);
	if (file_read(path_buf, sbuf, 32) < 0)
		return -1;

	if (!strncmp(sbuf, "offline", 7)) {
		if (file_write(path_buf, "start") == 0)
			rproc_flag = 1;
	}

	/* wait /dev/rpmsgx ready or not */
	for (j = 0; j < 10000; j++) {
		for (i = 0; i < EPT_NUM; i++) {
			memset(path_buf, 0, 512);
			sprintf(path_buf, "/dev/rpmsg%d", i);
			if (access(path_buf, F_OK) >= 0)
				continue;
			else
				break;
		}

		if (i >= EPT_NUM)
			break;
		else
			usleep(10);
	}

	if ( j >= 10000 ) {
		printf("remote proc is not ready\n");
		goto err_virtio;
	}

	found = 0;
	for (i = 0; i < EPT_NUM; i++) {
		memset(path_buf, 0, 512);
		sprintf(path_buf, "/dev/rpmsg%d", i);
		ipc->fd = open(path_buf, O_RDWR | O_NONBLOCK);
		if (ipc->fd < 0)
			continue;
		found = 1;
		break;
	}

	if (!found){
		printf("Failed to open rpmsg.\n");
		goto err_virtio;
	}

	return 0;

err_virtio:
	if (rproc_flag) {
		memset(path_buf, 0, 512);
		sprintf(path_buf, "/sys/class/remoteproc/remoteproc%u/state", ipc->rproc_id);
		file_write(path_buf, "stop");
	}
	return -1;
}

int xf_rproc_close(struct xf_proxy_ipc_data *ipc)
{
	char path_buf[512];
	int  ret, i;
	int  fd[EPT_NUM];

	close(ipc->fd);

	for (i = 0; i < EPT_NUM; i++) {
		memset(path_buf, 0, 512);
		sprintf(path_buf, "/dev/rpmsg%d", i);
		fd[i] = open(path_buf, O_RDWR | O_NONBLOCK);
		if (fd[i] < 0)
			break;
		close(fd[i]);
	}
	/* all /dev/rpmsgX are free */
	if (i == EPT_NUM) {
		memset(path_buf, 0, 512);
		sprintf(path_buf, "/sys/class/remoteproc/remoteproc%u/state", ipc->rproc_id);

		if (file_write(path_buf, "stop") != 0)
			return -1;
	}

	return 0;
}

/* ...open proxy interface on proper DSP partition */
int xf_ipc_open(struct xf_proxy_ipc_data *ipc, u32 core)
{
	int ret, i;
	char path[20];

	/* ...open file handle */
	ret = xf_rproc_open(ipc);
	if (ret < 0)
		return ret;

	XF_CHK_ERR(ipc->fd >= 0, -errno);
	XF_CHK_ERR((ipc->fd_mem = open("/dev/mem", O_RDWR | O_SYNC)) >= 0, -errno);

	/* ...create pipe for asynchronous response delivery */
	XF_CHK_ERR(pipe(ipc->pipe) == 0, -errno);

	TRACE("proxy interface opened\n");

	return 0;
}

/* ...close proxy handle */
void xf_ipc_close(struct xf_proxy_ipc_data *ipc, u32 core)
{
	/* ...close asynchronous response delivery pipe */
	close(ipc->pipe[0]), close(ipc->pipe[1]);

	close(ipc->fd_mem);
	/* ...close proxy file handle */
	xf_rproc_close(ipc);

	TRACE("proxy interface closed\n");
}
