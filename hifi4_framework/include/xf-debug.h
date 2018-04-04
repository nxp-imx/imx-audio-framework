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

/*******************************************************************************
 * xf-debug.h
 *
 * Debugging interface for Xtensa Audio DSP codec server
 *
 *******************************************************************************/

#ifndef __XF_DEBUG_H
#define __XF_DEBUG_H

#include <stdio.h>
#include "printf.h"

#define TIMESTAMP
#define COREINFO

#ifdef DEBUG
#define hifi_printf(...) __hifi_printf(__VA_ARGS__)
#define LOG(msg)                    { TIMESTAMP; COREINFO; hifi_printf(msg);                    }
#define LOG1(msg, arg1)             { TIMESTAMP; COREINFO; hifi_printf(msg, arg1);              }
#define LOG2(msg, arg1, arg2)       { TIMESTAMP; COREINFO; hifi_printf(msg, arg1, arg2);        }
#define LOG3(msg, arg1, arg2, arg3) { TIMESTAMP; COREINFO; hifi_printf(msg, arg1, arg2, arg3);  }
#define LOG4(msg, arg1, arg2, arg3, arg4) { TIMESTAMP; COREINFO; hifi_printf(msg, arg1, arg2, arg3, arg4); }
#else
#define hifi_printf(...)
#define LOG(msg)                    { }
#define LOG1(msg, arg1)             { }
#define LOG2(msg, arg1, arg2)       { }
#define LOG3(msg, arg1, arg2, arg3) { }
#define LOG4(msg, arg1, arg2, arg3, arg4) { }
#endif

/*******************************************************************************
 * Auxiliary macros (put into "xf-types.h"?)
 ******************************************************************************/

#ifndef offset_of
#define offset_of(type, member)         \
    ((int)&(((const type *)(0))->member))
#endif

#ifndef container_of
#define container_of(ptr, type, member) \
    ((type *)((void *)(ptr) - offset_of(type, member)))
#endif

#define BUG(cond, fmt, ...)             (void)0

/*******************************************************************************
 * Run-time error processing
 ******************************************************************************/

/* ...check the API call succeeds */
#define XF_CHK_API(cond)                                \
({                                                      \
    int __ret;                                          \
                                                        \
    if ((__ret = (int)(cond)) < 0)                      \
    {                                                   \
        LOG1("API error: %d\n", __ret);                 \
        return __ret;                                   \
    }                                                   \
    __ret;                                              \
})

/* ...check the condition is true */
#define XF_CHK_ERR(cond, error)                         \
({                                                      \
    int __ret;                                          \
                                                        \
    if (!(__ret = (int)(cond)))                         \
    {                                                   \
    }                                                   \
    __ret;                                              \
})

#endif
