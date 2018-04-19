/*
***********************************************************************
 * Copyright 2009-2010 by Freescale Semiconductor, Inc.
 * Copyright 2018 NXP
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

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

***********************************************************************
*
*  History :
*  Date             Author              Version    Description
*
*  Aug,2009         B06543              0.1        Initial Version
*
*/

#ifndef _FSL_MMLAYER_TYPES_H
#define _FSL_MMLAYER_TYPES_H

#include <stdio.h>

#ifndef uint64
#ifdef WIN32
	typedef  unsigned __int64 uint64;
#else
	typedef  unsigned long long uint64;
#endif
#endif /*uint64*/


#ifndef int64
#ifdef WIN32
	typedef  __int64 int64;
#else
	typedef  long long int64;
#endif
#endif /*int64*/

typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
typedef int int32;
typedef short int16;
typedef char int8;

#ifndef bool
    #define bool int
#endif

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

#ifndef NULL
    #define NULL (void *)0
#endif
#endif /* _FSL_MMLAYER_TYPES_H */

