/*****************************************************************
 * Copyright 2018-2021 NXP
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

#include <string.h>
#include <xtensa/hal.h>
#include <xtensa/xtruntime.h>
#include <xtensa/tie/xt_interrupt.h>
#include <xtensa/config/core.h>
#include "rpmsg_lite.h"
#include "board.h"
#include "fsl_mu.h"
#include "mydefs.h"
#include "debug.h"

/* ...NULL-address specification */
#define XF_PROXY_NULL           (~0U)
/* ...invalid proxy address */
#define XF_PROXY_BADADDR        (dsp_config->dpu_ext_msg.scratch_buf_size)

extern struct mem_cfg dsp_mem_cfg;
extern struct mu_cfg  dsp_mu_cfg;

xf_dsp_t *xf_g_dsp;
struct dsp_main_struct *g_dsp;

struct dsp_main_struct *get_main_struct()
{
	return g_dsp;
}

int xf_ipc_deinit(UWORD32 core)
{
	xf_core_data_t  *cd = XF_CORE_DATA(core);

	XF_CHK_API(xf_mm_deinit(&cd->shared_pool));

	return 0;
}

/* ...system-specific IPC layer initialization */
int xf_ipc_init(UWORD32 core)
{
	xf_core_data_t  *cd = XF_CORE_DATA(core);
	xf_core_ro_data_t  *ro = XF_CORE_RO_DATA(core);

	xf_shmem_data_t *shmem = (xf_shmem_data_t *) xf_g_dsp->xf_ap_shmem_buffer;

	/* ...initialize pointer to shared memory */
	cd->shmem = (xf_shmem_handle_t *)shmem;

	/* ...global memory pool initialization */
	XF_CHK_API(xf_mm_init(&cd->shared_pool, (shmem->buffer),
			      (xf_g_dsp->xf_ap_shmem_buffer_size - (sizeof(xf_shmem_data_t) - XF_CFG_REMOTE_IPC_POOL_SIZE))));

	/* ...open xos-msgq interface */
	XF_CHK_API(ipc_msgq_init(&ro->ipc.cmd_msgq, &ro->ipc.resp_msgq, &ro->ipc.msgq_event));

	return 0;
}

static int rpmsg_callback(void *payload, uint32_t payload_len, uint32_t src, void *priv)
{
	struct rpmsg_ept_handle *ept_handle = (struct rpmsg_ept_handle *)priv;
	struct dsp_main_struct  *dsp_config = (struct dsp_main_struct *)ept_handle->priv;
	struct xf_proxy_message *recd_msg = payload;
	xf_core_ro_data_t *ro;
	xf_proxy_message_t msg;
	u32 core = 0;

	ro = XF_CORE_RO_DATA(core);

	if (payload_len != sizeof(struct xf_proxy_message)) {
		LOG("Error: message length\n");
		return RL_RELEASE;
	}

	if (ept_handle->peerAddr == RL_ADDR_ANY)
		ept_handle->peerAddr = src;

	/* ...fill message parameters */
	/* use localAddr replace the 'core' id */
	msg.session_id = recd_msg->session_id;
	msg.opcode     = recd_msg->opcode;
	msg.length     = recd_msg->length;
	msg.address    = recd_msg->address;

	/* We only support 2 instances, so use BIT(14) in
	 * session_id for this.
	 * BIT(14) == 0: first instance
	 * BIT(14) == 1: second instance
	 * this bit is only used for return message to user
	 * space.
	 */
	if (ept_handle->localAddr == 2)
		msg.session_id |= BIT(14);
	LOG2("cmd.... %x, %x\n", msg.opcode, msg.length);
	/* ...pass message to xos message queue */
	/* ??? should be called in thread context */
	__xf_msgq_send(ro->ipc.cmd_msgq, &msg, sizeof(msg));
	__xf_event_set(ro->ipc.msgq_event, CMD_MSGQ_READY);

	return RL_RELEASE;
}

void rpmsg_response(UWORD32 core)
{
	xf_proxy_message_t msg;
	xf_core_ro_data_t *ro;
	xf_msgq_t resp_msgq;
	u32 ept_idx;
	int ret;

	ro = XF_CORE_RO_DATA(core);
	resp_msgq = ro->ipc.resp_msgq;

	while(!__xf_msgq_empty(resp_msgq)) {
		ret = __xf_msgq_recv(resp_msgq, &msg, sizeof(msg));
		if(ret != XAF_NO_ERR)
			return;

		/* Use BIT(30) for instance distinguish
		 * because src and dst has been swapped.
		 * so BIT(30) is the BIT(14) in src.
		 */
		if (msg.session_id & BIT(30)) {
			ept_idx = 1;
			msg.session_id &= ~BIT(30);
		} else
			ept_idx = 0;
		LOG3("resp... %x, %x, %x\n", msg.session_id, msg.opcode, msg.length);

		rpmsg_lite_send(g_dsp->rpmsg,
				g_dsp->ept[ept_idx],
				g_dsp->ept_handle[ept_idx].peerAddr,
				(char *)&msg,
				sizeof(xf_proxy_message_t),
				RL_DONT_BLOCK);
	}
}

int main(void)
{
	int ret, i;
	int core;
	struct dsp_main_struct *dsp;

	/* set the cache attribute */
	xthal_set_icacheattr(I_CACHE_ATTRIBUTE);
	xthal_set_dcacheattr(D_CACHE_ATTRIBUTE);

	/* set the interrupt */
	xthal_set_intclear(-1);

	dsp = (struct dsp_main_struct *)GLOBAL_DSP_MEM_ADDR;
	xf_g_dsp = &dsp->xf_dsp;
	g_dsp = dsp;
	core = 0;

	/* This is needed by xos, otherwise interrupt is not triggered */
	xos_start_main("main", 7, 0);
#ifdef DEBUG
	enable_log();
#endif
	/* ...initialize MU */
	MU_Init((void *)MUB_BASE);

	LOG("DSP Start.....\n");

	if (!dsp->is_core_init) {
		dsp->dpu_ext_msg.scratch_buf_phys = SCRATCH_MEM_ADDR;
		dsp->dpu_ext_msg.scratch_buf_size = SCRATCH_MEM_SIZE;
		dsp->dpu_ext_msg.dsp_config_phys  = GLOBAL_DSP_MEM_ADDR;
		dsp->dpu_ext_msg.dsp_config_size  = GLOBAL_DSP_MEM_SIZE;
		dsp->dpu_ext_msg.dsp_board_type   = BOARD_TYPE;
		dsp->mem_cfg                      = &dsp_mem_cfg;
		dsp->mu_cfg                       = &dsp_mu_cfg;

		/* ??? Allocate local pool */
		xf_g_dsp->xf_ap_shmem_buffer       = (UWORD8 *)SCRATCH_MEM_ADDR;
		xf_g_dsp->xf_ap_shmem_buffer_size  = SCRATCH_MEM_SIZE - 0x6F0000;
		/* ??? Reserved 0xF0000 size for local */
		xf_g_dsp->xf_dsp_local_buffer      = (UWORD8 *)(SCRATCH_MEM_ADDR + SCRATCH_MEM_SIZE - 0x6F0000);
		xf_g_dsp->xf_dsp_local_buffer_size = 0x6F0000;

		XF_CHK_API(xf_mm_init(&(xf_g_dsp->xf_core_data[0]).local_pool, xf_g_dsp->xf_dsp_local_buffer, xf_g_dsp->xf_dsp_local_buffer_size));
		 __xf_lock_init(&(g_dsp->g_msgq_lock));
		xf_core_init(core); /* ->xf_ipc_init() */
		dsp->is_core_init = 1;

		for (i = 0; i < XAF_MAX_WORKER_THREADS; i++) {
			xf_g_dsp->xf_core_data[0].worker_thread_scratch_size[i] = 1024*4*16;
		}

		dsp->rpmsg = rpmsg_lite_remote_init((void *)RPMSG_LITE_SRTM_SHMEM_BASE,
						    RPMSG_LITE_SRTM_LINK_ID,
						    RL_NO_FLAGS,
						    &dsp->rpmsg_ctxt);
		if (!dsp->rpmsg) {
			LOG("rpmsg init fail\n");
			goto exit;
		}

		/* Send ready notification to A core */
		MU_txdb((void *)MUB_BASE, dsp->mu_cfg, RPMSG_MU_CHANNEL);

		/* Wait A core to kick */
		while (!rpmsg_lite_is_link_up(dsp->rpmsg))
			env_sleep_msec(1);

		LOG("rpmsg linked.....\n");

		for (i = 0; i < EPT_NUM; i++) {
			dsp->ept_handle[i].priv = dsp;
			dsp->ept_handle[i].peerAddr = RL_ADDR_ANY;
			dsp->ept[i] = rpmsg_lite_create_ept(dsp->rpmsg, RL_ADDR_ANY,
							    (rl_ept_rx_cb_t)rpmsg_callback,
							    (void *)&dsp->ept_handle[i],
							    &dsp->ept_ctx[i]);
			if (!dsp->ept[i]) {
				LOG1("create ept %d fail\n", i);
				goto exit;
			}
			dsp->ept_handle[i].localAddr = dsp->ept[i]->addr;

			/* rpmsg-raw is defined in kernel */
			ret = rpmsg_ns_announce(dsp->rpmsg, dsp->ept[i], "rpmsg-raw",
						(uint32_t)RL_NS_CREATE);
			if (ret != RL_SUCCESS) {
				LOG1("ns %d fail\n", i);
				goto exit;
			}
		}

	} else {
		xf_proxy_message_t msg;
		xf_core_ro_data_t *ro;

		platform_init();

		MU_EnableRxInterrupts((void *)MUB_BASE, dsp->mu_cfg, RPMSG_MU_CHANNEL);
		xos_interrupt_enable(INT_NUM_MU);

		MU_txdb((void *)MUB_BASE, dsp->mu_cfg, RPMSG_MU_CHANNEL);

		ro = XF_CORE_RO_DATA(core);

		/* add resume message to msgq */
		msg.session_id = __XF_MSG_ID(__XF_AP_PROXY(0), __XF_DSP_PROXY(0));
		msg.opcode     = XF_SUSPEND_RESUME;
		msg.length     = 0;
		msg.address    = 0;

		__xf_msgq_send(ro->ipc.cmd_msgq, &msg, sizeof(msg));
		__xf_event_init(ro->ipc.msgq_event, 0xffff);
		__xf_event_set(ro->ipc.msgq_event, CMD_MSGQ_READY);
	}

	for ( ; ; ) {
		xf_ipi_wait(core);

		/* ...service core event */
		xf_core_service(core);

	}

	xf_core_deinit(core);

exit:
	LOG("DSP main process exit\n");
	return 0;
}
