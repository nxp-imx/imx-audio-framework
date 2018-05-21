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

/* ...allocate 6 bits for client number per core */
#define XF_CFG_MAX_CLIENTS             (1 << 6)

union icm_header_t {
	struct {
		/* intr = 1 when sending msg */
		u32 msg     : 6;
		/* sub_msg will have ICM_MSG when
		 * msg=ICM_XXX_ACTION_COMPLETE
		 */
		u32 sub_msg : 6;
		// reserved
		u32 rsvd    : 3;
		u32 intr    : 1;
		/* =size in bytes (excluding header) to follow
		 * when intr=1, =response message when ack=1
		 */
		u32 size    : 15;
		u32 ack     : 1;
	};
	u32 allbits;
};

enum icm_action_t {
	ICM_CORE_READY = 1,
	ICM_CORE_INIT,
	ICM_CORE_EXIT,
};

struct icm_dpu_ext_msg {
	u32     ext_msg_addr;
	u32     ext_msg_size;
	u32     scratch_buf_phys;
	u32     scratch_buf_size;
	u32     dsp_config_phys;
	u32     dsp_config_size;
};

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
	struct icm_dpu_ext_msg dpu_ext_msg;

	struct dsp_mem_info scratch_mem_info;

};

#endif
