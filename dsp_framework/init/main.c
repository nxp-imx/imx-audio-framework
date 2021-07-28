/*****************************************************************
 * Copyright 2018-2020 NXP
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
#include "xf-shmem.h"
#include "xf-core.h"
#include "xf-hal.h"
#include "board.h"
#include "fsl_mu.h"

extern struct mem_cfg dsp_mem_cfg;
extern struct mu_cfg  dsp_mu_cfg;

static int rpmsg_callback(void *payload, uint32_t payload_len, uint32_t src, void *priv)
{
	struct rpmsg_ept_handle *ept_handle = (struct rpmsg_ept_handle *)priv;
	struct dsp_main_struct  *dsp_config = (struct dsp_main_struct *)ept_handle->priv;
	struct xf_proxy_message *recd_msg = payload;
	struct xf_message *m;
	u32 msg;

	if (payload_len != sizeof(struct xf_proxy_message)) {
		LOG("Error: message length\n");
		return RL_RELEASE;
	}

	if (ept_handle->peerAddr == RL_ADDR_ANY)
		ept_handle->peerAddr = src;

	/* ...allocate message; the call should not fail */
	m = xf_msg_pool_get(&dsp_config->pool);
	if (!m) {
		LOG("Error: ICM Queue full\n");
		return RL_RELEASE;
	}

	/* ...fill message parameters */
	/* use localAddr replace the 'core' id */
	m->id = recd_msg->session_id | ((ept_handle->localAddr & 0x3) - 1);
	m->opcode = recd_msg->opcode;
	m->length = recd_msg->length;
	m->buffer = xf_ipc_a2b(dsp_config, recd_msg->address);
	m->ret = recd_msg->ret;

	/* ...and schedule message execution on proper core */
	xf_msg_submit(&dsp_config->queue, m);

	return RL_RELEASE;
}

void dsp_global_msg_pool_init(struct dsp_main_struct *dsp_config)
{
	int i = 0;
	struct xf_msg_pool *pool = &dsp_config->pool;

	/* ...initialize global message pool */
	pool->p = &dsp_config->icm_msg_que[0];
	for (pool->head = &pool->p[i = 0]; i < XF_CFG_MESSAGE_POOL_SIZE - 1; i++) {
		/* ...set message pointer to next message in the pool */
		xf_msg_pool_item(pool, i)->next = xf_msg_pool_item(pool, i + 1);
	}
	/* ...set tail of the list */
	xf_msg_pool_item(pool, i)->next = NULL;
	/* ...save pool size */
	pool->n = XF_CFG_MESSAGE_POOL_SIZE;
}

/* ...wake on when interrupt happens */
void wake_on_ints(struct dsp_main_struct *dsp_config)
{
#if XCHAL_SW_VERSION >= 1404000
	xtos_interrupt_enable(INT_NUM_MU);
	xtos_set_intlevel(15);
#else
	_xtos_ints_on((1 << INT_NUM_MU));
	_xtos_set_intlevel(15);
#endif

	if (!dsp_config->is_interrupt)
		XT_WAITI(0);
#if XCHAL_SW_VERSION >= 1404000
	xtos_set_intlevel(0);
	xtos_interrupt_disable(INT_NUM_MU);
#else
	_xtos_set_intlevel(0);
	_xtos_ints_off((1 << INT_NUM_MU));
#endif

	dsp_config->is_interrupt = 0;
}

int main(void)
{
	int ret, i;
	struct dsp_main_struct *dsp = (struct dsp_main_struct *)GLOBAL_DSP_MEM_ADDR;

	/* set the cache attribute */
	xthal_set_icacheattr(I_CACHE_ATTRIBUTE);
	xthal_set_dcacheattr(D_CACHE_ATTRIBUTE);

	/* set the interrupt */
	xthal_set_intclear(-1);

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

		xf_core_init(dsp);

		dsp_global_msg_pool_init(dsp);

		dsp->rpmsg = rpmsg_lite_remote_init((void *)RPMSG_LITE_SRTM_SHMEM_BASE,
						    RPMSG_LITE_SRTM_LINK_ID,
						    RL_NO_FLAGS,
						    &dsp->rpmsg_ctxt);
		if (!dsp->rpmsg) {
			LOG("rpmsg init fail\n");
			goto exit;
		}

		/* ??? Send ready notification to A core */
		MU_txdb((void *)MUB_BASE, dsp->mu_cfg, RPMSG_MU_CHANNEL);

		/* wait A core to kick */
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

		}

	} else {
		/* only for resume */
		dsp->is_resume = 1;

		platform_init();

		MU_txdb((void *)MUB_BASE, dsp->mu_cfg, RPMSG_MU_CHANNEL);

		MU_EnableRxInterrupts((void *)MUB_BASE, dsp->mu_cfg, RPMSG_MU_CHANNEL);
	}

	for ( ; ; ) {
		/* ...dsp enter low power mode when no messages */
		wake_on_ints(dsp);

		/* ...service core event */
		xf_core_service(dsp);
	}

exit:
	LOG("DSP main process exit\n");
	return 0;
}
