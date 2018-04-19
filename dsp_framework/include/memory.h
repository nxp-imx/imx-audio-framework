//*****************************************************************
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

#ifndef __MEMORY_H_
#define __MEMORY_H_

#include "xf-types.h"

union header {
	struct {
		union header *ptr;
		unsigned long size;
	}s;
	char c[8];
};
typedef union header HEADER;

typedef struct {
	char *scratch_buf_ptr;
	long int scratch_total_size;
	long int scratch_remaining;
	HEADER *Base;
	HEADER *First;
	HEADER *Allocp;
	unsigned long Heapsize;
	unsigned long Availmem;
} dsp_mem_info;

void MEM_scratch_init(dsp_mem_info *mem_info, u32 ptr, u32 size);
void *MEM_scratch_malloc(dsp_mem_info *mem_info, int size);
void MEM_scratch_mfree(dsp_mem_info *mem_info, void *blk);

#endif


