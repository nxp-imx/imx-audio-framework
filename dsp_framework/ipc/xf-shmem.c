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

 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <xtensa/config/system.h>
#include "xf-shmem.h"
#include "peripheral.h"
#include "dpu_lib_load.h"
#include "board.h"

extern int xf_core_init(struct dsp_main_struct *dsp_config);
extern struct dsp_mem_info *RESERVED_mem_info;

/* ...NULL-address specification */
#define XF_PROXY_NULL           (~0U)

/* ...invalid proxy address */
#define XF_PROXY_BADADDR        (dsp_config->dpu_ext_msg.scratch_buf_size)

/* ...local interface status change flag */
#define XF_PROXY_STATUS_LOCAL           (1 << 0)

/* ...remote status change notification flag */
#define XF_PROXY_STATUS_REMOTE          (1 << 1)

/* ...translate buffer address to shared proxy address */
static inline u32 xf_ipc_b2a(struct dsp_main_struct *dsp_config, void *b)
{
	void *start = (void *)dsp_config->dpu_ext_msg.scratch_buf_phys;

	if (!b)
		return XF_PROXY_NULL;
	else if ((s32)(b - start) < dsp_config->dpu_ext_msg.scratch_buf_size)
		return (u32)(b - start);
#ifdef PLATF_8MP_LPA
	else if ((s32)(b - (void *)RESERVED_mem_info->scratch_buf_ptr) < RESERVED_mem_info->scratch_total_size)
		return (u32)(b - (void *)RESERVED_mem_info->scratch_buf_ptr + dsp_config->dpu_ext_msg.scratch_buf_size);
#endif
	else
		return XF_PROXY_BADADDR;
}

/* ...translate shared proxy address to local pointer */
void *xf_ipc_a2b(struct dsp_main_struct *dsp_config, u32 address)
{
	void *start = (void *)dsp_config->dpu_ext_msg.scratch_buf_phys;

	if (address < dsp_config->dpu_ext_msg.scratch_buf_size)
		return start + address;
#ifdef PLATF_8MP_LPA
	else if (address < RESERVED_mem_info->scratch_total_size + dsp_config->dpu_ext_msg.scratch_buf_size)
		return RESERVED_mem_info->scratch_buf_ptr + address - dsp_config->dpu_ext_msg.scratch_buf_size;
#endif
	else if (address == XF_PROXY_NULL)
		return NULL;
	else
		return (void *)-1;
}

/* ...retrieve all incoming commands from shared memory ring-buffer */
static u32 xf_shmem_process_input(struct dsp_main_struct *dsp_config)
{
	struct xf_message *m;

	if (dsp_config->is_resume) {
		/* ...allocate message; the call should not fail */
		m = xf_msg_pool_get(&dsp_config->pool);
		if (!m) {
			LOG("Error: ICM Queue full\n");
			return 0;
		}

		/* ...fill message parameters */
		/* use localAddr replace the 'core' id */
		m->id = __XF_MSG_ID(__XF_AP_PROXY(0), __XF_DSP_PROXY(0));
		m->opcode = XF_RESUME;
		m->length = 0;
		m->buffer = 0;
		m->ret = 0;

		dsp_config->is_resume = 0;

		/* ...and schedule message execution on proper core */
		xf_msg_submit(&dsp_config->queue, m);
	}

	env_isr(1);

	if (dsp_config->is_suspend) {
		/* ...allocate message; the call should not fail */
		m = xf_msg_pool_get(&dsp_config->pool);
		if (!m) {
			LOG("Error: ICM Queue full\n");
			return 0;
		}

		/* ...fill message parameters */
		/* use localAddr replace the 'core' id */
		m->id = __XF_MSG_ID(__XF_AP_PROXY(0), __XF_DSP_PROXY(0));
		m->opcode = XF_SUSPEND;
		m->length = 0;
		m->buffer = 0;
		m->ret = 0;

		dsp_config->is_suspend = 0;

		/* ...and schedule message execution on proper core */
		xf_msg_submit(&dsp_config->queue, m);
	}

	return 0;
}

/* ...send out all pending outgoing responses
 * to the shared memory ring-buffer
 */
static u32 xf_shmem_process_output(struct dsp_main_struct *dsp_config)
{
	struct dsp_main_struct *core = dsp_config;
	struct xf_proxy_message resp;
	struct xf_proxy_message *response = &resp;
	struct xf_message *m;
	u32 status = 0;
	u32 ept_idx;

	/* ...remove message from internal queue */
	m = xf_msg_proxy_get(&dsp_config->response);
	if (!m)
		return 0;

	/* ...notify remote interface each time we send
	 * it a message (only if it was empty?)
	 */
	status = XF_PROXY_STATUS_REMOTE | XF_PROXY_STATUS_LOCAL;

	/* ...flush message buffer contents to main memory
	 * as required - too late - different core - tbd
	 */
	(XF_OPCODE_RDATA(m->opcode) ? XF_PROXY_FLUSH(m->buffer, m->length) : 0);

	/* ...put the response message fields */
	response->session_id = m->id;
	response->opcode = m->opcode;
	response->length = m->length;
	if (m->opcode == XF_SHMEM_INFO)
		response->address = (u32)m->buffer;
	else
		response->address = xf_ipc_b2a(core, m->buffer);
	response->ret = m->ret;
	/* ...flush the content of the caches to main memory */
	XF_PROXY_FLUSH(response, sizeof(*response));

	LOG4("Response[client: %x]:(%x,%x,%x)\n",
	     XF_MSG_SRC_CLIENT(m->id), m->id, m->opcode, m->length);

	/* ...return message back to the pool */
	xf_msg_pool_put(&dsp_config->pool, m);

	ept_idx = XF_MSG_DST_CORE(response->session_id);
	rpmsg_lite_send(dsp_config->rpmsg,
			dsp_config->ept[ept_idx],
			dsp_config->ept_handle[ept_idx].peerAddr,
			(char *)response,
			sizeof(struct xf_proxy_message),
			RL_DONT_BLOCK);

	return status;
}

/* ...process local/remote shared memory interface status change */
void xf_shmem_process_queues(struct dsp_main_struct *dsp_config)
{
	u32     status;

	do {
		/* ...send out pending response messages
		 * (frees message buffers, so do it first)
		 */
		status = xf_shmem_process_output(dsp_config);

		/* ...receive and forward incoming command messages
		 * (allocates message buffers)
		 */
		status |= xf_shmem_process_input(dsp_config);

	} while (status);
}

/* ...initialize shared memory interface (DSP side) */
void xf_shmem_init(struct dsp_main_struct *dsp_config)
{

}
