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
 * xf-types.h
 *
 * Platform-specific typedefs
 *
 ******************************************************************************/

#ifndef __XF_TYPES_H
#define __XF_TYPES_H

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include <stdio.h>
#include <errno.h>

/*******************************************************************************
 * Primitive types
 ******************************************************************************/

typedef unsigned int    u32;
typedef signed int      s32;
typedef unsigned short  u16;
typedef signed short    s16;
typedef unsigned char   u8;
typedef signed char     s8;

#define BIT(x) (1 << (x))

#endif
