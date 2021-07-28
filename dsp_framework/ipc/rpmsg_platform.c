// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2021 NXP

#include <xtensa/config/core.h>
#include <xtensa/xos.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "board.h"
#include "fsl_mu.h"
#include "rpmsg_platform.h"
#include "rpmsg_env.h"
#include "mydefs.h"

#if defined(RL_USE_ENVIRONMENT_CONTEXT) && (RL_USE_ENVIRONMENT_CONTEXT == 1)
#error "This RPMsg-Lite port requires RL_USE_ENVIRONMENT_CONTEXT set to 0"
#endif

void MU_B_IRQHandler(void *arg)
{
	struct dsp_main_struct *dsp = (struct dsp_main_struct *)GLOBAL_DSP_MEM_ADDR;
	uint32_t msg;

	dsp->is_interrupt = 1;

	if (MU_GetRxStatusFlags((void *)MUB_BASE, dsp->mu_cfg, RPMSG_MU_CHANNEL) != 0UL) {
		/* Read message from RX register. */
		msg = MU_ReceiveMsgNonBlocking((void *)MUB_BASE, dsp->mu_cfg, RPMSG_MU_CHANNEL);
		/*AP -> DSP:  rx->tx*/
		if (msg == 0)
			env_isr(0);

		if (msg == RP_MBOX_SUSPEND_SYSTEM)
			dsp->is_suspend = 1;
	}
}

int32_t platform_init_interrupt(uint32_t vector_id, void *isr_data)
{
	struct dsp_main_struct *dsp = (struct dsp_main_struct *)GLOBAL_DSP_MEM_ADDR;

	/* Register ISR to environment layer */
	env_register_isr(vector_id, isr_data);

	env_lock_mutex(dsp->platform_lock);

	MU_EnableRxInterrupts((void *)MUB_BASE, dsp->mu_cfg, RPMSG_MU_CHANNEL);

	env_unlock_mutex(dsp->platform_lock);

	return 0;
}

int32_t platform_deinit_interrupt(uint32_t vector_id)
{
	struct dsp_main_struct *dsp = (struct dsp_main_struct *)GLOBAL_DSP_MEM_ADDR;

	/* Prepare the MU Hardware */
	env_lock_mutex(dsp->platform_lock);

	MU_DisableRxInterrupts((void *)MUB_BASE, dsp->mu_cfg, RPMSG_MU_CHANNEL);

	/* Unregister ISR from environment layer */
	env_unregister_isr(vector_id);

	env_unlock_mutex(dsp->platform_lock);

	return 0;
}

void platform_notify(uint32_t vector_id)
{
	struct dsp_main_struct *dsp = (struct dsp_main_struct *)GLOBAL_DSP_MEM_ADDR;
	uint32_t msg = (uint32_t)(vector_id);

	env_lock_mutex(dsp->platform_lock);
	MU_SendMsg((void *)MUB_BASE, dsp->mu_cfg, RPMSG_MU_CHANNEL, msg);
	env_unlock_mutex(dsp->platform_lock);
}

/**
 * platform_time_delay
 *
 * @param num_msec Delay time in ms.
 *
 * This is not an accurate delay, it ensures at least num_msec passed when return.
 */
void platform_time_delay(uint32_t num_msec)
{
	uint32_t loop;

	/* Calculate the CPU loops to delay, each loop has approx. 6 cycles */
	loop = SYSTEM_CLOCK / 6U / 1000U * num_msec;

	while (loop > 0U) {
		asm("nop");
		loop--;
	}
}

/**
 * platform_in_isr
 *
 * Return whether CPU is processing IRQ
 *
 * @return True for IRQ, false otherwise.
 *
 */
int32_t platform_in_isr(void)
{
	return (xthal_get_interrupt() & xthal_get_intenable());
}

/**
 * platform_interrupt_enable
 *
 * Enable peripheral-related interrupt
 *
 * @param vector_id Virtual vector ID that needs to be converted to IRQ number
 *
 * @return vector_id Return value is never checked.
 *
 */
int32_t platform_interrupt_enable(uint32_t vector_id)
{
#if XCHAL_SW_VERSION >= 1404000
	xtos_interrupt_enable(INT_NUM_MU);
#else
	_xtos_ints_on((1 << INT_NUM_MU));
#endif

	return ((int32_t)vector_id);
}

/**
 * platform_interrupt_disable
 *
 * Disable peripheral-related interrupt.
 *
 * @param vector_id Virtual vector ID that needs to be converted to IRQ number
 *
 * @return vector_id Return value is never checked.
 *
 */
int32_t platform_interrupt_disable(uint32_t vector_id)
{
#if XCHAL_SW_VERSION >= 1404000
	xtos_interrupt_disable(INT_NUM_MU);
#else
	_xtos_ints_off((1 << INT_NUM_MU));
#endif

	return ((int32_t)vector_id);
}

/**
 * platform_map_mem_region
 *
 * Dummy implementation
 *
 */
void platform_map_mem_region(uint32_t vrt_addr, uint32_t phy_addr, uint32_t size, uint32_t flags)
{
}

/**
 * platform_cache_all_flush_invalidate
 *
 * Dummy implementation
 *
 */
void platform_cache_all_flush_invalidate(void)
{
	xthal_dcache_all_unlock();
	xthal_dcache_all_invalidate();
	xthal_dcache_sync();
	xthal_icache_all_unlock();
	xthal_icache_all_invalidate();
	xthal_icache_sync();
}

/**
 * platform_cache_disable
 *
 * Dummy implementation
 *
 */
void platform_cache_disable(void)
{
}

/**
 * platform_vatopa
 *
 * Dummy implementation
 *
 */
uint32_t platform_vatopa(void *addr)
{
	struct dsp_main_struct *dsp = (struct dsp_main_struct *)GLOBAL_DSP_MEM_ADDR;
	struct mem_cfg *cfg = dsp->mem_cfg;
	uint32_t da = (uint32_t)addr;
	uint32_t i, sys;

	/* parse address translation table */
	for (i = 0; i < cfg->att_size; i++) {
		struct mem_att *att = &cfg->att[i];

		if (da >= att->da && da < att->da + att->size) {
			unsigned int offset = da - att->da;

			sys = att->sa + offset;
			return (uint32_t)(char *)sys;
		}
	}

	return ((uint32_t)(char *)addr);
}

/**
 * platform_patova
 *
 * Dummy implementation
 *
 */
void *platform_patova(uint32_t addr)
{
	struct dsp_main_struct *dsp = (struct dsp_main_struct *)GLOBAL_DSP_MEM_ADDR;
	struct mem_cfg *cfg = dsp->mem_cfg;
	uint32_t i, da;

	/* parse address translation table */
	for (i = 0; i < cfg->att_size; i++) {
		struct mem_att *att = &cfg->att[i];

		if (addr >= att->sa && addr < att->sa + att->size) {
			unsigned int offset = addr - att->sa;

			da = att->da + offset;
			return (void *)(char *)da;
		}
	}

	return ((void *)(char *)addr);
}

/**
 * platform_init
 *
 * platform/environment init
 */
int32_t platform_init(void)
{
	struct dsp_main_struct *dsp = (struct dsp_main_struct *)GLOBAL_DSP_MEM_ADDR;

	/* Create lock used in multi-instanced RPMsg */
	if (env_create_mutex(&dsp->platform_lock, 1) != 0)
		return -1;

	/* Register interrupt handler for MU_B on HiFi4 */
#if XCHAL_SW_VERSION >= 1404000
	xtos_set_interrupt_handler(INT_NUM_MU, MU_B_IRQHandler, GLOBAL_DSP_MEM_ADDR, NULL);
#else
	_xtos_set_interrupt_handler_arg(INT_NUM_MU, MU_B_IRQHandler, GLOBAL_DSP_MEM_ADDR);
#endif

	return 0;
}

/**
 * platform_deinit
 *
 * platform/environment deinit process
 */
int32_t platform_deinit(void)
{
	struct dsp_main_struct *dsp = (struct dsp_main_struct *)GLOBAL_DSP_MEM_ADDR;

#if XCHAL_SW_VERSION >= 1404000
	xtos_set_interrupt_handler(INT_NUM_MU, ((void *)0), NULL, NULL);
#else
	_xtos_set_interrupt_handler_arg(INT_NUM_MU, ((void *)0), NULL);
#endif

	/* Delete lock used in multi-instanced RPMsg */
	env_delete_mutex(dsp->platform_lock);
	dsp->platform_lock = ((void *)0);

	return 0;
}
