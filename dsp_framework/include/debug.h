/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2021 NXP
 */

#ifndef __DEBUG_H
#define __DEBUG_H


#ifdef DEBUG
#define dsp_printf(...) __dsp_printf(__VA_ARGS__)
#define LOG(msg)                    { dsp_printf(msg);                    }
#define LOG1(msg, arg1)             { dsp_printf(msg, arg1);              }
#define LOG2(msg, arg1, arg2)       { dsp_printf(msg, arg1, arg2);        }
#define LOG3(msg, arg1, arg2, arg3) { dsp_printf(msg, arg1, arg2, arg3);  }
#define LOG4(msg, arg1, arg2, arg3, arg4) { dsp_printf(msg, arg1, arg2, arg3, arg4); }
#else
#define dsp_printf(...)
#define LOG(msg)                    { }
#define LOG1(msg, arg1)             { }
#define LOG2(msg, arg1, arg2)       { }
#define LOG3(msg, arg1, arg2, arg3) { }
#define LOG4(msg, arg1, arg2, arg3, arg4) { }
#endif

#endif
