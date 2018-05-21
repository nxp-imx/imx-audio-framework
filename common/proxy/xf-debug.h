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

#ifndef __XF_DEBUG_H
#define __XF_DEBUG_H

#include <stdio.h>
#include <errno.h>

/*******************************************************************************
 * Auxiliary macros (put into "xf-types.h"?)
 ******************************************************************************/

#ifndef offset_of
#define offset_of(type, member)         \
	((int)(intptr_t)&(((const type *)(0))->member))
#endif

#ifndef container_of
#define container_of(ptr, type, member) \
	((type *)((void *)(ptr) - offset_of(type, member)))
#endif

/* ...check if non-zero value is a power-of-two */
#define xf_is_power_of_two(v)       (((v) & ((v) - 1)) == 0)

/*******************************************************************************
 * Bugchecks
 ******************************************************************************/
#define BUG(cond, fmt, ...)             (void)0

#ifdef DEBUG
#define  TRACE(...)  printf(__VA_ARGS__)
#else
#define  TRACE(...)  { }
#endif

/*******************************************************************************
 * Run-time error processing
 ******************************************************************************/

/* ...check the API call succeeds */
#define XF_CHK_API(cond)                                \
({                                                      \
	int __ret;                                          \
	__ret = (int)(cond);                                \
	if (__ret < 0) {                                    \
		TRACE("API error: %d\n", __ret);                \
		return __ret;                                   \
	}                                                   \
	__ret;                                              \
})

/* ...check the condition is true */
#define XF_CHK_ERR(cond, error)                 \
({                                              \
	intptr_t __ret;                             \
	__ret = (intptr_t)(cond);                   \
	if (!__ret) {                               \
		TRACE("check failed, error = %d\n", error);    \
		return error;                           \
	}                                           \
	(int)__ret;                                 \
})

#endif
