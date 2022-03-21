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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "types.h"
#include "memory.h"
#include "mydefs.h"
#include "board.h"
#include "debug.h"

#define ALLOC_ALIGNMENT 16

XAF_ERR_CODE xaf_malloc(void **buf_ptr, int size, int id)
{
	int obj_size = size + ALLOC_ALIGNMENT;
	int *ptr;

	ptr = xf_mem_alloc(obj_size, ALLOC_ALIGNMENT, 0, 0);
	if (!ptr) {
		LOG("malloc failed!\n");
		return XAF_MEMORY_ERR;
	}

	/* first word for store the size, which used for free */
	*ptr = obj_size;

	*buf_ptr = (void *)ptr + ALLOC_ALIGNMENT;

	return XAF_NO_ERR;
}

void xaf_free(void *buf_ptr, int id)
{
	if (!buf_ptr) {
		LOG("invailed memory pointer!\n");
		return;
	}

	int *ptr = buf_ptr - ALLOC_ALIGNMENT;
	int obj_size =  *ptr;

	xf_mem_free(ptr, obj_size, 0, 0);
}

void *xf_uniacodec_malloc(size_t size)
{
	int *buf_ptr;
	xaf_malloc((void **)&buf_ptr, size, 0);
	return buf_ptr;
}

void xf_uniacodec_free(void *buf_ptr)
{
	if (!buf_ptr)
		return;
	xaf_free(buf_ptr, 0);
}
