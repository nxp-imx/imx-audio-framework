//*****************************************************************
// Copyright (c) 2017 Cadence Design Systems, Inc.
// Copyright 2018 NXP
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//*****************************************************************

/*
 * mydefs.h
 *
 */

#ifndef _MYDEFS_H_
#define _MYDEFS_H_

#include <xtensa/config/core.h>
#include <time.h>

#if 0
#define TIMESTAMP hifi_printf("%d: ", (int)clock());
#define COREINFO  hifi_printf("Core_%x %s: ", my_prid, XCHAL_CORE_ID);
#endif

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

#endif /* _MYDEFS_H_ */
