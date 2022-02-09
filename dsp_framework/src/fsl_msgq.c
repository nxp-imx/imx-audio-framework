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
 * xf-msgq.c
 *
 * System-specific IPC layer Implementation
 ******************************************************************************/

#define MODULE_TAG                      MSGQ

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "xaf-api.h"
#include "osal-msgq.h"
#include "mydefs.h"

/*******************************************************************************
 * Global Definitions
 ******************************************************************************/

extern struct dsp_main_struct *g_dsp;
extern XAF_ERR_CODE xaf_malloc(void **buf_ptr, int size, int id);

/* ...remote IPC is coherent for hostless xaf (say) */
#define XF_REMOTE_IPC_NON_COHERENT      0

/*******************************************************************************
 * Global abstractions
 ******************************************************************************/

/* ...prevent instructions reordering */
#define barrier()                           \
    __asm__ __volatile__("": : : "memory")

/* ...memory barrier */
#define XF_PROXY_BARRIER()                  \
    __asm__ __volatile__("memw": : : "memory")

/* ...memory invalidation */
#define XF_PROXY_INVALIDATE(buf, length)    \
    ({ if ((length)) { xthal_dcache_region_invalidate((buf), (length)); barrier(); } buf; })

/* ...memory flushing */
#define XF_PROXY_FLUSH(buf, length)         \
    ({ if ((length)) { barrier(); xthal_dcache_region_writeback((buf), (length)); XF_PROXY_BARRIER(); } buf; })


/*******************************************************************************
 * Local constants - tbd
 ******************************************************************************/

#define SEND_LOCAL_MSGQ_ENTRIES     16

/*******************************************************************************
 * Internal IPC API implementation
 ******************************************************************************/

int ipc_msgq_init(xf_msgq_t *cmdq, xf_msgq_t *respq, xf_event_t **msgq_event)
{

    *cmdq = *respq = NULL;

    __xf_lock(&(g_dsp->g_msgq_lock));
    
    if (g_dsp->g_ipc_msgq.init_done) 
    {
        *cmdq       = g_dsp->g_ipc_msgq.cmd_msgq;
        *respq      = g_dsp->g_ipc_msgq.resp_msgq;
        *msgq_event = &g_dsp->g_ipc_msgq.msgq_event;
        
        return 0;
    }

    g_dsp->g_ipc_msgq.cmd_msgq = __xf_msgq_create(SEND_MSGQ_ENTRIES, sizeof(xf_proxy_message_t));
    /* ...allocation mustn't fail on App Interface Layer */
    BUG(g_dsp->g_ipc_msgq.cmd_msgq == NULL, _x("Out-of-memeory"));


    g_dsp->g_ipc_msgq.resp_msgq = __xf_msgq_create(RECV_MSGQ_ENTRIES, sizeof(xf_proxy_message_t));
    /* ...allocation mustn't fail on App Interface Layer */
    BUG(g_dsp->g_ipc_msgq.resp_msgq == NULL, _x("Out-of-memeory"));

    __xf_event_init(&g_dsp->g_ipc_msgq.msgq_event, 0xffff);

    *cmdq       = g_dsp->g_ipc_msgq.cmd_msgq;
    *respq      = g_dsp->g_ipc_msgq.resp_msgq;
    *msgq_event = &g_dsp->g_ipc_msgq.msgq_event;

    g_dsp->g_ipc_msgq.init_done = 1;

    __xf_unlock(&(g_dsp->g_msgq_lock));

    return 0;
}

int ipc_msgq_delete(xf_msgq_t *cmdq, xf_msgq_t *respq)
{

    __xf_msgq_destroy(g_dsp->g_ipc_msgq.cmd_msgq);
    __xf_msgq_destroy(g_dsp->g_ipc_msgq.resp_msgq);
    __xf_event_destroy(&g_dsp->g_ipc_msgq.msgq_event);

    g_dsp->g_ipc_msgq.cmd_msgq   = NULL;
    g_dsp->g_ipc_msgq.resp_msgq  = NULL;
    g_dsp->g_ipc_msgq.init_done  = 0;

    *cmdq = *respq = NULL;

    return 0;
}

