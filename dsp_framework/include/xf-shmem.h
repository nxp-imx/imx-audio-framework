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
 * xf-shmem.h
 *
 * Definitions for Xtensa SHMEM configuration
 *
 *******************************************************************************/

#ifndef __XF_SHMEM_H
#define __XF_SHMEM_H

#include "mydefs.h"
#include "xf-proxy.h"
#include <xtensa/config/system.h>

/*******************************************************************************
 * Memory structures
 ******************************************************************************/

/* ...data managed by host CPU (remote) - in case of shunt it is a IPC layer */
struct xf_proxy_host_data
{
    /* ...command queue */
    xf_proxy_message_t      command[XF_PROXY_MESSAGE_QUEUE_LENGTH];

    /* ...writing index into command queue */
    u32                     cmd_write_idx;

    /* ...reading index for response queue */
    u32                     rsp_read_idx;

    /* ...indicate command queue is valid or not */
    u32						cmd_invalid;
}/*   __attribute__((__packed__, __aligned__(XF_PROXY_ALIGNMENT)))*/;

/* ...data managed by DSP (local) */
struct xf_proxy_dsp_data
{
    /* ...response queue */
    xf_proxy_message_t      response[XF_PROXY_MESSAGE_QUEUE_LENGTH];

    /* ...writing index into response queue */
    u32                     rsp_write_idx;

    /* ...reading index for command queue */
    u32                     cmd_read_idx;

    /* ...indicate response queue is valid or not */
    u32						rsp_invalid;
}/*   __attribute__((__packed__, __aligned__(XF_PROXY_ALIGNMENT)))*/;

/* ...shared memory data */
typedef struct xf_shmem_data 
{
    /* ...outgoing data (maintained by host CPU (remote side)) */
    volatile struct xf_proxy_host_data   remote/*      __xf_shmem__*/;
    
    /* ...ingoing data (maintained by DSP (local side)) */
    volatile struct xf_proxy_dsp_data    local/*       __xf_shmem__*/;

}   xf_shmem_data_t;

/*******************************************************************************
 * Shared memory accessors
 ******************************************************************************/

/* ...shared memory pointer for a core */
#define XF_SHMEM_DATA(core)                         \
    ((xf_shmem_data_t *)core->shmem)
#if 1
/* ...prevent instructions reordering */
#define barrier()                                   \
    __asm__ __volatile__("": : : "memory")

/* ...memory barrier */
#define XF_PROXY_BARRIER()                          \
	__asm__ __volatile__("memw": : : "memory")
#endif
/* ...memory invalidation */
#define XF_PROXY_INVALIDATE(buf, length)    \
	({ if ((length)) { /*xthal_dcache_region_invalidate((buf), (length)); barrier()*/; } buf; })

/* ...memory flushing */
#define XF_PROXY_FLUSH(buf, length)         \
	({ if ((length)) { /*barrier(); xthal_dcache_region_writeback((buf), (length)); XF_PROXY_BARRIER()*/; } buf; })

/* ...atomic reading */
#define XF_PROXY_READ_ATOMIC(var)                   \
    ({ XF_PROXY_INVALIDATE(&(var), sizeof(var)); (var); })

/* ...atomic writing */
#define XF_PROXY_WRITE_ATOMIC(var, value)           \
    ({(var) = (value); XF_PROXY_FLUSH(&(var), sizeof(var)); (value); })

/* ...accessors */
#define XF_PROXY_READ(core, field)                  \
    __XF_PROXY_READ_##field(XF_SHMEM_DATA(core))

#define XF_PROXY_WRITE(core, field, v)              \
    __XF_PROXY_WRITE_##field(XF_SHMEM_DATA(core), (v))

/* ...individual fields accessors */
#define __XF_PROXY_READ_cmd_write_idx(proxy)        \
    XF_PROXY_READ_ATOMIC(proxy->remote.cmd_write_idx)

#define __XF_PROXY_READ_cmd_read_idx(proxy)         \
    proxy->local.cmd_read_idx

#define __XF_PROXY_READ_cmd_invalid(proxy)        \
	XF_PROXY_READ_ATOMIC(proxy->remote.cmd_invalid)

#define __XF_PROXY_READ_rsp_write_idx(proxy)        \
    proxy->local.rsp_write_idx

#define __XF_PROXY_READ_rsp_read_idx(proxy)         \
    XF_PROXY_READ_ATOMIC(proxy->remote.rsp_read_idx)

#define __XF_PROXY_READ_rsp_invalid(proxy)         \
    XF_PROXY_READ_ATOMIC(proxy->local.rsp_invalid)

/* ...individual fields accessors */
#define __XF_PROXY_WRITE_cmd_write_idx(proxy, v)    \
    XF_PROXY_WRITE_ATOMIC(proxy->remote.cmd_write_idx, v)

#define __XF_PROXY_WRITE_cmd_read_idx(proxy, v)     \
    XF_PROXY_WRITE_ATOMIC(proxy->local.cmd_read_idx, v)

#define __XF_PROXY_WRITE_cmd_invalid(proxy, v)     \
    XF_PROXY_WRITE_ATOMIC(proxy->remote.cmd_invalid, v)

#define __XF_PROXY_WRITE_rsp_read_idx(proxy, v)     \
    XF_PROXY_WRITE_ATOMIC(proxy->remote.rsp_read_idx, v)

#define __XF_PROXY_WRITE_rsp_write_idx(proxy, v)    \
    XF_PROXY_WRITE_ATOMIC(proxy->local.rsp_write_idx, v)

#define __XF_PROXY_WRITE_rsp_invalid(proxy, v)    \
    XF_PROXY_WRITE_ATOMIC(proxy->local.rsp_invalid, v)

/* ...command buffer accessor */
#define XF_PROXY_COMMAND(core, idx)                 \
    (&XF_SHMEM_DATA((core))->remote.command[(idx)])

/* ...response buffer accessor */
#define XF_PROXY_RESPONSE(core, idx)                \
    (&XF_SHMEM_DATA((core))->local.response[(idx)])


/*******************************************************************************
 * API functions
 ******************************************************************************/

/* ...process shared memory interface on given DSP core */
void xf_shmem_process_queues(dsp_main_struct *dsp_config);

/* ...initialize shared memory interface (DSP side) */
void xf_shmem_init(dsp_main_struct *dsp_config);

void interrupt_handler_icm(void *arg);

u32 icm_intr_send(u32 msg);

#endif
