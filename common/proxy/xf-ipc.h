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

/* ...size of the shared memory pool (in bytes) */
#define  XF_CFG_REMOTE_IPC_POOL_SIZE  0xFFFFFF

/* ...NULL-address specification */
#define XF_PROXY_NULL       (~0U)

/* ...invalid proxy address */
#define XF_PROXY_BADADDR    XF_CFG_REMOTE_IPC_POOL_SIZE

/* ...response timeout, 10s */
#define TIMEOUT   10000

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

    /* ...pipe for asynchronous response delivery */
    int                     pipe[2];

}   xf_proxy_ipc_data_t;

/*******************************************************************************
 * Helpers for asynchronous response delivery
 ******************************************************************************/

#define xf_proxy_ipc_response_put(ipc, msg) \
    (write((ipc)->pipe[1], (msg), sizeof(*(msg))) == sizeof(*(msg)) ? 0 : -errno)

#define xf_proxy_ipc_response_get(ipc, msg) \
    (read((ipc)->pipe[0], (msg), sizeof(*(msg))) == sizeof(*(msg)) ? 0 : -errno)

/*******************************************************************************
 * Shared memory translation
 ******************************************************************************/

/* ...translate proxy shared address into local virtual address */
static inline void * xf_ipc_a2b(xf_proxy_ipc_data_t *ipc, u32 address)
{
    if (address < XF_CFG_REMOTE_IPC_POOL_SIZE)
        return (unsigned char *) ipc->shmem + address;
    else if (address == XF_PROXY_NULL)
        return NULL;
    else
        return (void *) -1;
}

/* ...translate local virtual address into shared proxy address */
static inline u32 xf_ipc_b2a(xf_proxy_ipc_data_t *ipc, void *b)
{
    u32     a;

    if (b == NULL)
        return XF_PROXY_NULL;
    if ((a = (u32)((u8 *)b - (u8 *)ipc->shmem)) < XF_CFG_REMOTE_IPC_POOL_SIZE)
        return a;
    else
        return XF_PROXY_BADADDR;
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

#define xf_ipc_response_put(ipc, msg)       \
    (write((ipc)->pipe[1], (msg), sizeof(*(msg))) == sizeof(*(msg)) ? 0 : -errno)

#define xf_ipc_response_get(ipc, msg)       \
    (read((ipc)->pipe[0], (msg), sizeof(*(msg))) == sizeof(*(msg)) ? 0 : -errno)

#define xf_ipc_data_init(ipc)               \
    (pipe((ipc)->pipe) == 0 ? 0 : -errno)

#define xf_ipc_data_destroy(ipc)            \
    (close((ipc)->pipe[0]), close((ipc)->pipe[1]))

#endif
