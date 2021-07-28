/*****************************************************************
 * Copyright 2018 NXP
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
 *
 *****************************************************************/

#ifndef __MY_DEFS_H
#define __MY_DEFS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xf-msg.h"
#include "xf-opcode.h"
#include "xf-sched.h"
#include "xf-component.h"
#include "memory.h"
#include "rpmsg_lite.h"
#include "rpmsg_ns.h"
#include "fsl_mu.h"

/* ...allocate 6 bits for client number per core */
#define XF_CFG_MAX_CLIENTS             (1 << 6)

struct icm_dpu_ext_msg {
	u32     scratch_buf_phys;
	u32     scratch_buf_size;
	u32     dsp_config_phys;
	u32     dsp_config_size;
	u32	dsp_board_type;
};

struct rpmsg_ept_handle {
	u32 localAddr;                 /*!< RPMsg local endpoint address */
	u32 peerAddr;                  /*!< RPMsg peer endpoint address */
	void *priv;                    /* Pointer for dsp_main_struct */
};

/* Max supported ISR counts */
#define ISR_COUNT (12U) /* Change for multiple remote cores */
/*!
 * Structure to keep track of registered ISR's.
 */
struct isr_info {
	void *data;
};

struct mem_att {
	u32 da; /* device address (From Cortex M4 view)*/
	u32 sa; /* system bus address */
	u32 size; /* size of reg range */
	int flags;
};

struct mem_cfg {
	struct mem_att *att;
	int    att_size;
};

/*
 * Only support one instance
 */
#define EPT_NUM        0x2
#define XF_CFG_MESSAGE_POOL_SIZE  256
struct dsp_main_struct {
	/* shared memory message pool data */
	struct xf_message icm_msg_que[XF_CFG_MESSAGE_POOL_SIZE];
	struct xf_msg_pool pool;

	/* ...scheduler queue (sorted by execution timestamp) */
	xf_sched_t sched;

	/* ...command/response queue for communication
	 * within local core (including ISRs)
	 */
	struct  xf_msg_queue queue;

	/* ...message queue containing responses to remote proxy */
	struct  xf_msg_queue response;

	/* ...per-core component mapping */
	struct xf_cmap_link cmap[XF_CFG_MAX_CLIENTS];

	/* ...index of first free client */
	u32 free;

	/* ...opaque system-specific shared memory data handle */
	volatile void *shmem;

	u32 is_core_init;
	u32 is_interrupt;
	u32 is_suspend;
	u32 is_resume;

	struct icm_dpu_ext_msg dpu_ext_msg;
	struct dsp_mem_info scratch_mem_info;

	struct rpmsg_lite_instance *rpmsg;
	struct rpmsg_lite_instance rpmsg_ctxt;

	struct rpmsg_lite_endpoint *ept[EPT_NUM];
	struct rpmsg_lite_ept_static_context ept_ctx[EPT_NUM];
	struct rpmsg_ept_handle ept_handle[EPT_NUM];
	struct isr_info isr_table[ISR_COUNT];
	struct mem_cfg *mem_cfg;
	struct mu_cfg *mu_cfg;
	void *platform_lock;
};

#endif
