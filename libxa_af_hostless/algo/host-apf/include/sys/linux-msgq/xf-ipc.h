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
 * xf-ipc.h
 *
 * IPC layer for File I/O configuration
 *******************************************************************************/

#ifndef __XF_H
#error "xf-ipc.h mustn't be included directly"
#endif

/*******************************************************************************
 * Types definitions
 ******************************************************************************/

/* ...proxy IPC data */
typedef struct xf_proxy_ipc_data
{
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

        int                     rproc_id;

        /* ...pipe for asynchronous response delivery */
        int                     pipe[2];

}   xf_proxy_ipc_data_t;

/*******************************************************************************
 * Shared memory translation
 ******************************************************************************/

/* ...translate proxy shared address into local virtual address */
static inline void * xf_ipc_a2b(xf_proxy_ipc_data_t *ipc, UWORD32 address)
{
	return (address != 0) ? (unsigned char *)ipc->shmem + address : NULL;
}

/* ...translate local virtual address into shared proxy address */
static inline UWORD32 xf_ipc_b2a(xf_proxy_ipc_data_t *ipc, void *b)
{
	return (b != NULL) ? (UWORD32)(b - ipc->shmem) : 0;
}

/*******************************************************************************
 * Component inter-process communication
 ******************************************************************************/

typedef struct xf_ipc_data
{
    /* ...asynchronous response delivery pipe */
    int                 pipe[2];

}   xf_ipc_data_t;

/*******************************************************************************
 * Helpers for asynchronous response delivery
 ******************************************************************************/

extern int xf_proxy_ipc_response_put(xf_proxy_ipc_data_t *ipc, xf_proxy_msg_t *msg);
extern int xf_proxy_ipc_response_get(xf_proxy_ipc_data_t *ipc, xf_proxy_msg_t *msg);
extern int xf_ipc_response_put(xf_ipc_data_t *ipc, xf_user_msg_t *msg);
extern int xf_ipc_response_get(xf_ipc_data_t *ipc, xf_user_msg_t *msg);
extern int xf_ipc_data_init(xf_ipc_data_t *ipc);
extern int xf_ipc_data_destroy(xf_ipc_data_t *ipc);

/*******************************************************************************
* API functions
 ******************************************************************************/

/* ...send asynchronous command */
extern int  xf_ipc_send(xf_proxy_ipc_data_t *ipc, xf_proxy_msg_t *msg, void *b);

/* ...wait for response from DSP Interface Layer */
extern int  xf_ipc_wait(xf_proxy_ipc_data_t *ipc, UWORD32 timeout);

/* ...receive response from IPC layer */
extern int  xf_ipc_recv(xf_proxy_ipc_data_t *ipc, xf_proxy_msg_t *msg, void **b);

/* ...open proxy interface on proper DSP partition */
extern int  xf_ipc_open(xf_proxy_ipc_data_t *proxy, UWORD32 core);

/* ...close proxy handle */
extern void xf_ipc_close(xf_proxy_ipc_data_t *proxy, UWORD32 core);

/* ...set event to close proxy handle */
extern void xf_ipc_close_set_event(xf_proxy_ipc_data_t *ipc, UWORD32 core);

