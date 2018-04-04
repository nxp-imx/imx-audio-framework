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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "memory.h"

/* for one block, its size is 8 bytes */
#define  ABLKSIZE  (sizeof(HEADER))
/* for every allocated buffer, it will have a header which type
 * is union header, so "+1" is used to allocate a header for this buffer
 */
#define  BTOU(nb)  ((((nb) + ABLKSIZE - 1) / ABLKSIZE) + 1)

void MEM_scratch_init(hifi_mem_info *mem_info, u32 ptr, u32 size)
{
	mem_info->scratch_buf_ptr = (u8 *)ptr;
	mem_info->scratch_total_size = size;
	mem_info->scratch_remaining = size;
}

/* sample implementation for memory allocation */
void *MEM_scratch_malloc(hifi_mem_info *mem_info, int nb)  // This always to allocates 8-byte aligned buffers
{
	register HEADER *p, *q;
	register unsigned long nu;
	hifi_mem_info *ptr;

	ptr = mem_info;

	if(nb == 0)
		return NULL;

	nb = (nb + 7) & ~7;

	/* because the heap memory has been divided into several blocks, one
	 * block's size is 8 bytes, so BTOU() is used to caculate the block
	 * number that nb bytes needs
	 */
	nu = BTOU(nb);

	if((q = ptr->Allocp) == NULL)
	{
		ptr->Heapsize = ptr->scratch_total_size;
		/* caculate all the available blocks when initializing the heap memory */
		ptr->Availmem = BTOU(ptr->Heapsize) - 2;

		/* create the free list of available blocks, ptr->Allocp alway points
		 * the free list's header node
		 */
		ptr->Base = (HEADER *)ptr->scratch_buf_ptr;
		ptr->Base->s.ptr = ptr->Allocp = q = ptr->Base;
		ptr->Base->s.size = 1;

		ptr->First = ptr->Base + 1;
		ptr->Base->s.ptr = ptr->First;
		ptr->First->s.ptr = ptr->Base;
		ptr->First->s.size = BTOU(ptr->Heapsize) - 2;
	}

	/* find an available node that can provided buffer in the free list */
	for(p = q->s.ptr; ; q = p, p = p->s.ptr)
	{
		if(p->s.size >= nu)
		{
			/* because every allocated buffer has a header, the header's
			 * size is one block, this place can ensure that every free node
			 * in free list includes at least 2 blocks.
			 * */
			if(p->s.size <= nu + 1)
			{
				q->s.ptr = p->s.ptr;
			}
			else
			{
				/* cut requested buffer from the high address of the free node in free list */
				p->s.size -= nu;
				p += p->s.size;
				p->s.size = nu;
			}

			p->s.ptr = p;
			ptr->Availmem -= p->s.size;
			p++;

			break;
		}

		/* no available node in free list can provide the requested buffer */
		if(p == ptr->Allocp)
		{
			p = NULL;
			break;
		}
	}
	if(p)
		memset((void *)p, 0, nb);

	return (void *)p;
}

/* Note: This function free up the memory allocations done using
 *  *	function MEM_heap_malloc */
void MEM_scratch_mfree(hifi_mem_info *mem_info, void *blk)
{
	register HEADER *p, *q;
	unsigned int i;
	hifi_mem_info *ptr;

	ptr = mem_info;

	if(blk == NULL)
		return;

	p = ((HEADER *)blk) - 1;

	/* check whether this blocks is allocated reasonably or not */
	if(p->s.ptr != p)
	{
		return;
	}
	ptr->Availmem += p->s.size;

	/* the freed buffer need to be inserted into the free list, so
	 * this place is used to find the inserting point.
	 */
	for(q = ptr->Allocp; !(p > q && p < q->s.ptr); q = q->s.ptr)
	{
		/* is the highest address of this circular free list */
		if(q >= q->s.ptr && (p > q || p < q->s.ptr))
			break;
	}

	/* check whether the freed buffer can be merged with the next node
	 * in free list or not.
	 */
	if(p + p->s.size == q->s.ptr)
	{
		p->s.size += q->s.ptr->s.size;
		p->s.ptr = q->s.ptr->s.ptr;
	}
	else
	{
		p->s.ptr = q->s.ptr;
	}

	/* check whether the freed buffer can be merged with the previous node
	 * in free list or not
	 */
	if((q + q->s.size == p) && (q != ptr->Base))
	{
		q->s.size += p->s.size;
		q->s.ptr = p->s.ptr;
	}
	else
	{
		q->s.ptr = p;
	}

	return;
}


