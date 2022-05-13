/*
* Copyright (c) 2015-2021 Cadence Design Systems Inc.
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
*/
#ifndef _OSAL_MSGQ_H
#define _OSAL_MSGQ_H

typedef struct {
    size_t  size;
} *xf_msgq_t;

/* ...open proxy interface on proper DSP partition */
static inline xf_msgq_t __xf_msgq_create(size_t n_items, size_t item_size)
{
    return NULL;
}

/* ...close proxy handle */
static inline void __xf_msgq_destroy(xf_msgq_t q)
{
}

static inline int __xf_msgq_send(xf_msgq_t q, const void *data, size_t sz)
{
	return 0;
}

#define MAXIMUM_TIMEOUT 10000

static inline int __xf_msgq_recv_blocking(xf_msgq_t q, void *data, size_t sz)
{
	return 0;
}

static inline int __xf_msgq_recv(xf_msgq_t q, void *data, size_t sz)
{
	return 0;
}

static inline int __xf_msgq_empty(xf_msgq_t q)
{
	return 0;
}

static inline int __xf_msgq_full(xf_msgq_t q)
{
	return 0;
}

#endif
