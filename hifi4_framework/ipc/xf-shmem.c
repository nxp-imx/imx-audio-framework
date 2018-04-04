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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "xf-shmem.h"
#include "peripheral.h"
#include "dpu_lib_load.h"
#include <xtensa/config/system.h>

extern int xf_core_init(hifi_main_struct *hifi_config);

/* ...NULL-address specification */
#define XF_PROXY_NULL           (~0U)

/* ...invalid proxy address */
#define XF_PROXY_BADADDR        (hifi_config->dpu_ext_msg.scratch_buf_size)

/* ...local interface status change flag */
#define XF_PROXY_STATUS_LOCAL           (1 << 0)

/* ...remote status change notification flag */
#define XF_PROXY_STATUS_REMOTE          (1 << 1)


/* ...translate buffer address to shared proxy address */
static inline u32 xf_ipc_b2a(hifi_main_struct *hifi_config, void *b)
{
	void *start = (void *)hifi_config->dpu_ext_msg.scratch_buf_phys;

	if (b == NULL)
		return XF_PROXY_NULL;
	else if ((s32)(b - start) < hifi_config->dpu_ext_msg.scratch_buf_size)
		return (u32)(b - start);
	else
		return XF_PROXY_BADADDR;
}

/* ...translate shared proxy address to local pointer */
static inline void * xf_ipc_a2b(hifi_main_struct *hifi_config, u32 address)
{
	void *start = (void *)hifi_config->dpu_ext_msg.scratch_buf_phys;

	if (address < hifi_config->dpu_ext_msg.scratch_buf_size)
		return start + address;
	else if (address == XF_PROXY_NULL)
		return NULL;
	else
		return (void *)-1;
}

u32 icm_intr_send(u32 msg)
{
	mu_regs *base = (mu_regs*)MU_PADDR;

	mu_msg_send(base, 0, msg);

	return 0;
}

void xf_mu_init(hifi_main_struct *hifi_config)
{
	int i = 0;
	volatile int *mu = (volatile int *)MU_PADDR;
	mu_regs *base = (mu_regs*)MU_PADDR;
	icm_header_t icm_msg;
	xf_msg_pool_t *pool = &hifi_config->pool;

	/* ...initialize global message pool */
	pool->p = &hifi_config->icm_msg_que[0];
	for(pool->head = &pool->p[i = 0]; i < XF_CFG_MESSAGE_POOL_SIZE - 1; i++)
	{
		/* ...set message pointer to next message in the pool */
		xf_msg_pool_item(pool, i)->next = xf_msg_pool_item(pool, i + 1);
	}
	/* ...set tail of the list */
	xf_msg_pool_item(pool, i)->next = NULL;
	/* ...save pool size */
	pool->n = XF_CFG_MESSAGE_POOL_SIZE;

	mu_enableinterrupt_rx(base, 0);

	icm_msg.allbits = 0;
	icm_msg.intr = 1;
	icm_msg.msg  = ICM_CORE_READY;
	icm_intr_send(icm_msg.allbits);

	return;
}

void interrupt_handler_icm(void *arg)
{
	mu_regs *base = (mu_regs*)MU_PADDR;
	volatile icm_header_t recd_msg;
	hifi_main_struct *hifi_config = (hifi_main_struct *)arg;
	xf_message_t *m;
	u32 ext_msg_addr;
	u32 ext_msg_size = 0;
	u32 msg;

	mu_msg_receive(base, 0, &msg);
	recd_msg.allbits = msg;

	if (recd_msg.size == 8)
	{
		mu_msg_receive(base, 1, &ext_msg_addr);
		mu_msg_receive(base, 2, &ext_msg_size);
	}

	if (ICM_CORE_INIT == recd_msg.msg)
	{
		if(ext_msg_addr)
		{
			memcpy((u8 *)&hifi_config->dpu_ext_msg, (u8 *)ext_msg_addr, ext_msg_size);
			xf_core_init(hifi_config);
		}
	}
	else if(XF_SUSPEND == recd_msg.msg)
	{
		/* ...allocate message; the call should not fail */
		if((m = xf_msg_pool_get(&hifi_config->pool)) == NULL)
		{
			LOG("Error: ICM Queue full\n");
		}

		/* ...fill message parameters */
		m->id = __XF_MSG_ID(__XF_AP_PROXY(0), __XF_DSP_PROXY(0));
		m->opcode = XF_SUSPEND;
		m->length = 0;
		m->buffer = 0;
		m->ret = 0;

		/* ...and schedule message execution on proper core */
		xf_msg_submit(&hifi_config->queue, m);
	}
	else if(XF_RESUME == recd_msg.msg)
	{
		/* ...allocate message; the call should not fail */
		if((m = xf_msg_pool_get(&hifi_config->pool)) == NULL)
		{
			LOG("Error: ICM Queue full\n");
		}

		/* ...fill message parameters */
		m->id = __XF_MSG_ID(__XF_AP_PROXY(0), __XF_DSP_PROXY(0));
		m->opcode = XF_RESUME;
		m->length = 0;
		m->buffer = 0;
		m->ret = 0;

		/* ...and schedule message execution on proper core */
		xf_msg_submit(&hifi_config->queue, m);
	}
	else
	{
		;
	}

	hifi_config->is_interrupt = 1;

	return;
}

/* ...retrieve all incoming commands from shared memory ring-buffer */
static u32 xf_shmem_process_input(hifi_main_struct *hifi_config)
{
	hifi_main_struct *core = hifi_config;
	xf_message_t *m;
	volatile u32 read_idx, write_idx;
	u32 invalid;
	u32 status = 0;

	/* ...flush icache and dcache of dsp */
	xthal_dcache_all_unlock();
	xthal_dcache_all_invalidate();
	xthal_dcache_sync();
	xthal_icache_all_unlock();
	xthal_icache_all_invalidate();
	xthal_icache_sync();

	/* ...judge incoming command queue is valid or not */
	invalid = XF_PROXY_READ(core, cmd_invalid);
	if(invalid)
	{
		LOG("incoming command queue is invalid\n");
		return status;
	}

	/* ...get current value of write pointer */
	read_idx = XF_PROXY_READ(core, cmd_read_idx);
	write_idx = XF_PROXY_READ(core, cmd_write_idx);

	LOG2("Command queue: write = 0x%x / read = 0x%x\n", write_idx, read_idx);

	/* ...process all committed commands */
	while (!XF_QUEUE_EMPTY(read_idx, write_idx))
	{
		volatile xf_proxy_message_t* volatile command;

		/* ...allocate message; the call should not fail */
		if((m = xf_msg_pool_get(&hifi_config->pool)) == NULL)
		{
			LOG("Error: ICM Queue full\n");
			break;
		}

		/* ...if queue was full, set global proxy update flag */
		if (XF_QUEUE_FULL(read_idx, write_idx))
			status |= XF_PROXY_STATUS_REMOTE | XF_PROXY_STATUS_LOCAL;
		else
			status |= XF_PROXY_STATUS_LOCAL;

		/* ...get oldest not processed command */
		command = XF_PROXY_COMMAND(core, XF_QUEUE_IDX(read_idx));

		/*  ...synchronize memory contents */
		XF_PROXY_INVALIDATE(command, sizeof(*command));

		/* ...fill message parameters */
		m->id = command->session_id;
		m->opcode = command->opcode;
		m->length = command->length;
		m->buffer = xf_ipc_a2b(core, command->address);
		m->ret = command->ret;
		LOG4("ext_msg: [client: %x]:(%x,%x,%x)\n", XF_MSG_DST_CLIENT(m->id), m->id, m->opcode, m->length);

		/* ...invalidate message buffer contents as required - not here - tbd */
		(XF_OPCODE_CDATA(m->opcode) ? XF_PROXY_INVALIDATE(m->buffer, m->length) : 0);

		/* ...advance local reading index copy */
		read_idx = XF_QUEUE_ADVANCE_IDX(read_idx);

		/* ...update shadow copy of reading index */
		XF_PROXY_WRITE(core, cmd_read_idx, read_idx);

		/* ...and schedule message execution on proper core */
		xf_msg_submit(&hifi_config->queue, m);
	}

	return status;
}

/* ...send out all pending outgoing responses to the shared memory ring-buffer */
static u32 xf_shmem_process_output(hifi_main_struct *hifi_config)
{
	hifi_main_struct *core = hifi_config;
	mu_regs *base = (mu_regs*)MU_PADDR;
	icm_header_t dpu_icm;
	xf_message_t *m;
	volatile u32 read_idx, write_idx;
	u32 invalid;
	u32 status = 0;

	dpu_icm.allbits = 0;
	dpu_icm.intr = 1;
	dpu_icm.size = 8;

	/* ...judge response queue is valid or not */
	invalid = XF_PROXY_READ(core, rsp_invalid);
	if(invalid)
	{
		LOG("response  queue is invalid\n");
		return status;
	}

	/* ...get current value of peer read pointer */
	write_idx = XF_PROXY_READ(core, rsp_write_idx);
	read_idx = XF_PROXY_READ(core, rsp_read_idx);

	LOG2("Response queue: write = 0x%x / read = 0x%x\n", write_idx, read_idx);

	/* ...while we have response messages and there's space to write out one */
	while (!XF_QUEUE_FULL(read_idx, write_idx))
	{
		volatile xf_proxy_message_t* volatile response;

		/* ...remove message from internal queue */
		if ((m = xf_msg_proxy_get(&hifi_config->response)) == NULL)
			break;

		/* ...notify remote interface each time we send it a message (only if it was empty?) */
		status = XF_PROXY_STATUS_REMOTE | XF_PROXY_STATUS_LOCAL;

		/* ...flush message buffer contents to main memory as required - too late - different core - tbd */
		(XF_OPCODE_RDATA(m->opcode) ? XF_PROXY_FLUSH(m->buffer, m->length) : 0);

		/* ...find place in a queue for next response */
		response = XF_PROXY_RESPONSE(core, XF_QUEUE_IDX(write_idx));

		/* ...put the response message fields */
		response->session_id = m->id;
		response->opcode = m->opcode;
		response->length = m->length;
		response->address = xf_ipc_b2a(core, m->buffer);
		response->ret = m->ret;
		/* ...flush the content of the caches to main memory */
		XF_PROXY_FLUSH(response, sizeof(*response));

		LOG4("Response[client: %x]:(%x,%x,%x)\n", XF_MSG_SRC_CLIENT(m->id), m->id, m->opcode, m->length);

		/* ...return message back to the pool */
		xf_msg_pool_put(&hifi_config->pool, m);

		/* ...advance local writing index copy */
		write_idx = XF_QUEUE_ADVANCE_IDX(write_idx);

		/* ...update shared copy of queue write pointer */
		XF_PROXY_WRITE(core, rsp_write_idx, write_idx);

	}

	/* triger the mu interrupt when has responding message */
	if(status)
	{
		/* ...flush icache */
		xthal_icache_all_unlock();
		xthal_icache_all_invalidate();
		xthal_icache_sync();

		mu_msg_send(base, 0, dpu_icm.allbits);
	}

	return status;
}

/* ...process local/remote shared memory interface status change */
void xf_shmem_process_queues(hifi_main_struct *hifi_config)
{
	u32     status;

	do
	{
		/* ...send out pending response messages (frees message buffers, so do it first) */
		status = xf_shmem_process_output(hifi_config);

		/* ...receive and forward incoming command messages (allocates message buffers) */
		status |= xf_shmem_process_input(hifi_config);

	}while(status);
}

/* ...initialize shared memory interface (DSP side) */
void xf_shmem_init(hifi_main_struct *hifi_config)
{
	xf_shmem_data_t *shmem = (xf_shmem_data_t *)hifi_config->dpu_ext_msg.ext_msg_addr;

	hifi_config->shmem = (void *)shmem;
}
