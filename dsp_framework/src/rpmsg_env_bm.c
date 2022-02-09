// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2021 NXP

/**************************************************************************
 * FILE NAME
 *
 *       rpmsg_env_bm.c
 *
 *
 * DESCRIPTION
 *
 *       This file is Bare Metal Implementation of env layer for OpenAMP.
 *
 *
 **************************************************************************/

#include "rpmsg_env.h"
#include "rpmsg_platform.h"
#include "virtqueue.h"
#include "rpmsg_compiler.h"
#include "board.h"
#include "mydefs.h"

#include <stdlib.h>
#include <string.h>

#if defined(RL_USE_ENVIRONMENT_CONTEXT) && (RL_USE_ENVIRONMENT_CONTEXT == 1)
#error "This RPMsg-Lite port requires RL_USE_ENVIRONMENT_CONTEXT set to 0"
#endif

/*!
 * env_init
 *
 * Initializes OS/BM environment.
 *
 */
int32_t env_init(void)
{
	struct dsp_main_struct *dsp = (struct dsp_main_struct *)GLOBAL_DSP_MEM_ADDR;

	// first call
	(void)memset(dsp->isr_table, 0, sizeof(struct isr_info));

	return platform_init();
}

/*!
 * env_deinit
 *
 * Uninitializes OS/BM environment.
 *
 * @returns Execution status
 */
int32_t env_deinit(void)
{
	return platform_deinit();
}

/*!
 * env_allocate_memory - implementation
 *
 * @param size
 */
void *env_allocate_memory(uint32_t size)
{
	return malloc(size);
}

/*!
 * env_free_memory - implementation
 *
 * @param ptr
 */
void env_free_memory(void *ptr)
{
	if (ptr != ((void *)0))
		free(ptr);
}

/*!
 *
 * env_memset - implementation
 *
 * @param ptr
 * @param value
 * @param size
 */
void env_memset(void *ptr, int32_t value, uint32_t size)
{
	(void)memset(ptr, value, size);
}

/*!
 *
 * env_memcpy - implementation
 *
 * @param dst
 * @param src
 * @param len
 */
void env_memcpy(void *dst, void const *src, uint32_t len)
{
	(void)memcpy(dst, src, len);
}

/*!
 *
 * env_strcmp - implementation
 *
 * @param dst
 * @param src
 */

int32_t env_strcmp(const char *dst, const char *src)
{
	return strcmp(dst, src);
}

/*!
 *
 * env_strncpy - implementation
 *
 * @param dest
 * @param src
 * @param len
 */
void env_strncpy(char *dest, const char *src, uint32_t len)
{
	(void)strncpy(dest, src, len);
}

/*!
 *
 * env_strncmp - implementation
 *
 * @param dest
 * @param src
 * @param len
 */
int32_t env_strncmp(char *dest, const char *src, uint32_t len)
{
	return strncmp(dest, src, len);
}

/*!
 *
 * env_mb - implementation
 *
 */
void env_mb(void)
{
	MEM_BARRIER();
}

/*!
 * env_rmb - implementation
 */
void env_rmb(void)
{
	MEM_BARRIER();
}

/*!
 * env_wmb - implementation
 */
void env_wmb(void)
{
	MEM_BARRIER();
}

/*!
 * env_map_vatopa - implementation
 *
 * @param address
 */
uint32_t env_map_vatopa(void *address)
{
	return platform_vatopa(address);
}

/*!
 * env_map_patova - implementation
 *
 * @param address
 */
void *env_map_patova(uint32_t address)
{
	return platform_patova(address);
}

/*!
 * env_create_mutex
 *
 * Creates a mutex with the given initial count.
 *
 */
#if defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1)
int32_t env_create_mutex(void **lock, int32_t count, void *context)
#else
int32_t env_create_mutex(void **lock, int32_t count)
#endif
{
	/* make the mutex pointer point to itself
	 * this marks the mutex handle as initialized.
	 */
	*lock = lock;
	return 0;
}

/*!
 * env_delete_mutex
 *
 * Deletes the given lock
 *
 */
void env_delete_mutex(void *lock)
{
}

/*!
 * env_lock_mutex
 *
 * Tries to acquire the lock, if lock is not available then call to
 * this function will suspend.
 */
void env_lock_mutex(void *lock)
{
	/* No mutex needed for RPMsg-Lite in BM environment,
	 * since the API is not shared with ISR context.
	 */
}

/*!
 * env_unlock_mutex
 *
 * Releases the given lock.
 */
void env_unlock_mutex(void *lock)
{
	/* No mutex needed for RPMsg-Lite in BM environment,
	 * since the API is not shared with ISR context.
	 */
}

/*!
 * env_sleep_msec
 *
 * Suspends the calling thread for given time , in msecs.
 */
void env_sleep_msec(uint32_t num_msec)
{
	platform_time_delay(num_msec);
}

/*!
 * env_register_isr
 *
 * Registers interrupt handler data for the given interrupt vector.
 *
 * @param vector_id - virtual interrupt vector number
 * @param data      - interrupt handler data (virtqueue)
 */
void env_register_isr(uint32_t vector_id, void *data)
{
	struct dsp_main_struct *dsp = (struct dsp_main_struct *)GLOBAL_DSP_MEM_ADDR;

	RL_ASSERT(vector_id < ISR_COUNT);
	if (vector_id < ISR_COUNT)
		dsp->isr_table[vector_id].data = data;
}

/*!
 * env_unregister_isr
 *
 * Unregisters interrupt handler data for the given interrupt vector.
 *
 * @param vector_id - virtual interrupt vector number
 */
void env_unregister_isr(uint32_t vector_id)
{
	struct dsp_main_struct *dsp = (struct dsp_main_struct *)GLOBAL_DSP_MEM_ADDR;

	RL_ASSERT(vector_id < ISR_COUNT);
	if (vector_id < ISR_COUNT)
		dsp->isr_table[vector_id].data = ((void *)0);
}

/*!
 * env_enable_interrupt
 *
 * Enables the given interrupt
 *
 * @param vector_id   - virtual interrupt vector number
 */

void env_enable_interrupt(uint32_t vector_id)
{
	(void)platform_interrupt_enable(vector_id);
}

/*!
 * env_disable_interrupt
 *
 * Disables the given interrupt
 *
 * @param vector_id   - virtual interrupt vector number
 */

void env_disable_interrupt(uint32_t vector_id)
{
	(void)platform_interrupt_disable(vector_id);
}

/*!
 * env_map_memory
 *
 * Enables memory mapping for given memory region.
 *
 * @param pa   - physical address of memory
 * @param va   - logical address of memory
 * @param size - memory size
 * param flags - flags for cache/uncached  and access type
 */

void env_map_memory(uint32_t pa, uint32_t va, uint32_t size, uint32_t flags)
{
	platform_map_mem_region(va, pa, size, flags);
}

/*!
 * env_disable_cache
 *
 * Disables system caches.
 *
 */

void env_disable_cache(void)
{
	platform_cache_all_flush_invalidate();
	platform_cache_disable();
}

/*========================================================= */
/* Util data / functions for BM */

void env_isr(uint32_t vector)
{
	struct dsp_main_struct *dsp = (struct dsp_main_struct *)GLOBAL_DSP_MEM_ADDR;
	struct isr_info *info;

	RL_ASSERT(vector < ISR_COUNT);
	if (vector < ISR_COUNT) {
		info = &dsp->isr_table[vector];
		platform_cache_all_flush_invalidate();
		virtqueue_notification((struct virtqueue *)info->data);
	}
}
