/*
* Copyright (c) 2015-2021 Cadence Design Systems Inc.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
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
*/
/*******************************************************************************
 *
 * System-specific IPC layer Implementation
 ******************************************************************************/

#define MODULE_TAG                      IPC

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/poll.h>

#include "xf.h"
#include "xaf-api.h"
#include "xaf-structs.h"
#include "osal-msgq.h"

/*******************************************************************************
 * Global Definitions
 ******************************************************************************/

#define TIMEOUT   10000

/*******************************************************************************
 * Global abstractions
 ******************************************************************************/

struct dma_heap_allocation_data {
        __u64 len;
        __u32 fd;
        __u32 fd_flags;
        __u64 heap_flags;
};

#define DMA_HEAP_IOC_MAGIC              'H'

#define DMA_HEAP_IOCTL_ALLOC    _IOWR(DMA_HEAP_IOC_MAGIC, 0x0,\
                                      struct dma_heap_allocation_data)

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
int xf_ipc_send(xf_proxy_ipc_data_t *ipc, xf_proxy_msg_t *msg, void *b)
{
	int     fd = ipc->fd;
	int ret;

	TRACE(CMD, _b("C[%08x]:(%x,%08x,%u)"), msg->id, msg->opcode, msg->address, msg->length);

	/* ...pass message to kernel driver */
	ret = write(fd, msg, sizeof(*msg));
	if (ret < 0)
		return -errno;
	/* ...communication mutex is still locked! */
	return 0;
}

/* ...wait for response availability */
int xf_ipc_wait(xf_proxy_ipc_data_t *ipc, UWORD32 timeout)
{
	int             fd = ipc->fd;
	struct pollfd   pollfd;
	int ret;

	/* ...specify waiting set */
	pollfd.fd = fd;
	pollfd.events = POLLIN | POLLRDNORM;

	/* ...wait until there is a data in file */
	ret = poll(&pollfd, 1, -1);
	if (ret < 0)
		return ret;
	else if (ret == 0)
		return -ETIMEDOUT;

	return 0;
}

/* ...read response from proxy */
int xf_ipc_recv(xf_proxy_ipc_data_t *ipc, xf_proxy_msg_t *msg, void **buffer)
{
	int     fd = ipc->fd;
	int     r;
	xf_proxy_msg_t temp;

	/* ...get message header from file */
	r = read(fd, &temp, sizeof(xf_proxy_msg_t));
	if (r == sizeof(xf_proxy_msg_t)) {
		msg->id = temp.id;
		msg->opcode = temp.opcode;
		msg->length = temp.length;
		msg->address = temp.address;

		TRACE(RSP, _b("R[%08x]:(%x,%u,%08x)"), msg->id, msg->opcode, msg->length, msg->address);

		/* ...translate shared address into local pointer */
		XF_CHK_ERR((*buffer = xf_ipc_a2b(ipc, temp.address)) !=
				(void *)-1, -EBADFD);

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

int xf_dma_buf_open(struct xf_proxy_ipc_data *ipc) {

	struct dma_heap_allocation_data heap_data;
	int fd;
	int ret;

	fd = open("/dev/dma_heap/dsp", O_RDWR | O_SYNC);
	if (fd < 0) {
		printf("open dma heap for dsp fail\n");
		return -1;
	}

	/* size need to match with firmware */
	heap_data.len = 0xEF0000;
	heap_data.fd_flags = O_RDWR | O_CLOEXEC;
	heap_data.heap_flags = 0;
	heap_data.fd = 0;

	ret = ioctl(fd, DMA_HEAP_IOCTL_ALLOC, &heap_data);
	if (ret < 0) {
		printf("ioctl DMA_HEAP_IOCTL_ALLOC fail %d\n", ret);
		close(fd);
		return -1;
	}

	ipc->fd_mem = heap_data.fd;
	ipc->shmem_size = heap_data.len;

	ipc->shmem = mmap(NULL, ipc->shmem_size,
			  PROT_READ | PROT_WRITE, MAP_SHARED,
			  ipc->fd_mem, 0);
	if (ipc->shmem == MAP_FAILED) {
		printf("mmap fail %d\n", -errno);
		close(fd);
		close(ipc->fd_mem);
		return -1;
	}

	close(fd);
	return 0;
}

int xf_dma_buf_close(struct xf_proxy_ipc_data *ipc) {
	/* ...unmap shared memory region */
	(void)munmap(ipc->shmem, ipc->shmem_size);
	close(ipc->fd_mem);

	return 0;
}

void sighand(int signo)
{
	pthread_exit(NULL);
}

/* ...open proxy interface on proper DSP partition */
int xf_ipc_open(struct xf_proxy_ipc_data *ipc, UWORD32 core)
{
	int ret, i;
	char path[20];
	struct sigaction actions;

	memset(&actions, 0, sizeof(actions));
	sigemptyset(&actions.sa_mask);
	actions.sa_flags = 0;
	actions.sa_handler = sighand;

	/* set the handle function of SIGUSR1 */
	sigaction(SIGUSR1, &actions, NULL);

	/* ...open file handle */
	ret = xf_rproc_open(ipc);
	if (ret < 0)
		return ret;

	ret = xf_dma_buf_open(ipc);
	if (ret < 0) {
		xf_rproc_close(ipc);
		return ret;
	}

	/* ...create pipe for asynchronous response delivery */
	XF_CHK_ERR(pipe(ipc->pipe) == 0, -errno);

	TRACE(INFO,_b("proxy interface opened\n"));

	return 0;
}

/* ...close proxy handle */
void xf_ipc_close(struct xf_proxy_ipc_data *ipc, UWORD32 core)
{
	/* ...close asynchronous response delivery pipe */
	close(ipc->pipe[0]), close(ipc->pipe[1]);

	xf_dma_buf_close(ipc);
	/* ...close proxy file handle */
	xf_rproc_close(ipc);

	TRACE(INFO, _b("proxy interface closed\n"));
}

/*******************************************************************************
 * Internal API functions implementation
 ******************************************************************************/

/* ...set event to close proxy handle */
void xf_ipc_close_set_event(xf_proxy_ipc_data_t *ipc, UWORD32 core)
{
}

/*******************************************************************************
 * Local component message queue API implementation
 ******************************************************************************/

int xf_ipc_data_init(xf_ipc_data_t *ipc)
{
	/* ...initialize pipe */
	if (pipe(ipc->pipe) == 0)
		return 0;
	else
		return -errno;

	return 0;
}

int xf_ipc_data_destroy(xf_ipc_data_t *ipc)
{
	close(ipc->pipe[0]);
	close(ipc->pipe[1]);

	return 0;
}

int xf_ipc_response_put(xf_ipc_data_t *ipc, struct xf_user_msg *msg)
{
        if (write(ipc->pipe[1], msg, sizeof(*msg)) == sizeof(*msg))
                return 0;
        else
                return -errno;
}

int xf_ipc_response_get(xf_ipc_data_t *ipc, struct xf_user_msg *msg)
{
	int fd;
	struct pollfd   pollfd;
	UWORD32 timeout = TIMEOUT;
	int ret = 0;

	/* ...read pipe */
	fd = ipc->pipe[0];

	/* ...specify waiting set */
	pollfd.fd = fd;
	pollfd.events = POLLIN | POLLRDNORM;

	/* ...wait until there is a data in file */
	ret = poll(&pollfd, 1, timeout);
	if (ret > 0) {
		if (read(ipc->pipe[0], msg, sizeof(*msg)) == sizeof(*msg))
			return 0;
		return -errno;
	} else if (ret < 0)
		return ret;
	else
		return -ETIMEDOUT;
}


/*******************************************************************************
 * Helpers for asynchronous response delivery
 ******************************************************************************/
int xf_proxy_ipc_response_put(xf_proxy_ipc_data_t *ipc, xf_proxy_msg_t *msg)
{
	int ret;

	ret = write((ipc)->pipe[1], (msg), sizeof(*(msg)));
	if (ret == sizeof(*(msg)))
		return 0;
	else
		return -errno;
}

int xf_proxy_ipc_response_get(xf_proxy_ipc_data_t *ipc, xf_proxy_msg_t *msg)
{
        int fd;
        struct pollfd   pollfd;
        UWORD32 timeout = TIMEOUT;
        int ret = 0;

        /* ...read pipe */
        fd = ipc->pipe[0];

        /* ...specify waiting set */
        pollfd.fd = fd;
        pollfd.events = POLLIN | POLLRDNORM;

        /* ...wait until there is a data in file */
        ret = poll(&pollfd, 1, timeout);
        if (ret > 0) {
		ret = read((ipc)->pipe[0], (msg), sizeof(*(msg)));
		if (ret == sizeof(*(msg)))
			return 0;
		else
			return  -errno;
	}
        else if (ret < 0)
                return ret;
        else
                return -ETIMEDOUT;
}
