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

#include "xf-types.h"
#include "xf-debug.h"

#include "memory.h"
#include "mydefs.h"
#include "board.h"

/* for one block, its size is 8 bytes */
#define  ABLKSIZE  (sizeof(union header))
/* for every allocated buffer, it will have a header which type
 * is union header, so "+1" is used to allocate a header for this buffer
 */
#define  BTOU(nb)  ((((nb) + ABLKSIZE - 1) / ABLKSIZE) + 1)

#define OCRAM_A_OFFSET (0x30000)
#define OCRAM_A_PTR    (0x3b700000+OCRAM_A_OFFSET)
#define OCRAM_A_SIZE   (256*1024-OCRAM_A_OFFSET)
#define OCRAM_A_END    (OCRAM_A_PTR + OCRAM_A_SIZE)
#define RESERVED_OFFSET (0x0)
#define RESERVED_PTR    (0x92400000+RESERVED_OFFSET)
#define RESERVED_SIZE   (0x2000000-RESERVED_OFFSET)
#define RESERVED_END    (RESERVED_PTR + RESERVED_SIZE)

struct dsp_mem_info OCRAM_A_mem_info_t;
struct dsp_mem_info *OCRAM_A_mem_info;
struct dsp_mem_info RESERVED_mem_info_t;
struct dsp_mem_info *RESERVED_mem_info;

void MEM_scratch_init(struct dsp_mem_info *mem_info, u32 ptr, u32 size)
{
	mem_info->scratch_buf_ptr = (u8 *)ptr;
	mem_info->scratch_total_size = size;
	mem_info->scratch_remaining = size;

#ifdef PLATF_8MP_LPA
	OCRAM_A_mem_info = &OCRAM_A_mem_info_t;
	OCRAM_A_mem_info->scratch_buf_ptr = (char *)OCRAM_A_PTR;
	OCRAM_A_mem_info->scratch_total_size = OCRAM_A_SIZE;
	OCRAM_A_mem_info->scratch_remaining = OCRAM_A_SIZE;

	RESERVED_mem_info = &RESERVED_mem_info_t;
	RESERVED_mem_info->scratch_buf_ptr = (char *)RESERVED_PTR;
	RESERVED_mem_info->scratch_total_size = RESERVED_SIZE;
	RESERVED_mem_info->scratch_remaining = RESERVED_SIZE;
#endif
}

void *MEM_scratch_ua_malloc(int size)
{
	struct dsp_main_struct *dsp = (struct dsp_main_struct *)GLOBAL_DSP_MEM_ADDR;

#ifdef PLATF_8MP_LPA
	return MEM_scratch_malloc(OCRAM_A_mem_info, size);
#else
	return MEM_scratch_malloc(&dsp->scratch_mem_info, size);
#endif
}
/* sample implementation for memory allocation,
 * This always to allocates 8-byte aligned buffers
 */
void *MEM_scratch_malloc(struct dsp_mem_info *mem_info, int nb)
{
	register union header *p, *q;
	register unsigned long nu;
	struct dsp_mem_info *ptr;

	ptr = mem_info;

#ifdef PLATF_8MP_LPA
	if (nb > ptr->Availmem*8-8)
		ptr = RESERVED_mem_info;
#endif

	if (nb == 0)
		return NULL;

	nb = (nb + 7) & ~7;

	/* because the heap memory has been divided into several blocks, one
	 * block's size is 8 bytes, so BTOU() is used to caculate the block
	 * number that nb bytes needs
	 */
	nu = BTOU(nb);

	q = ptr->Allocp;
	if (!q) {
		ptr->Heapsize = ptr->scratch_total_size;
		/* caculate all the available blocks
		 * when initializing the heap memory
		 */
		ptr->Availmem = BTOU(ptr->Heapsize) - 2;

		/* create the free list of available blocks,
		 * ptr->Allocp alway points the free list's header node
		 */
		ptr->Base = (union header *)ptr->scratch_buf_ptr;
		ptr->Base->s.ptr = ptr->Base;
		ptr->Allocp = ptr->Base;
		q = ptr->Base;
		ptr->Base->s.size = 1;

		ptr->First = ptr->Base + 1;
		ptr->Base->s.ptr = ptr->First;
		ptr->First->s.ptr = ptr->Base;
		ptr->First->s.size = BTOU(ptr->Heapsize) - 2;
	}

	/* find an available node that can provided buffer in the free list */
	for (p = q->s.ptr; ; q = p, p = p->s.ptr) {
		if (p->s.size >= nu) {
			/* because every allocated buffer has a header,
			 * the header's size is one block, this place
			 * can ensure that every free node in free list
			 * includes at least 2 blocks.
			 */
			if (p->s.size <= nu + 1) {
				q->s.ptr = p->s.ptr;
			} else {
				/* cut requested buffer from the high address
				 * of the free node in free list
				 */
				p->s.size -= nu;
				p += p->s.size;
				p->s.size = nu;
			}

			p->s.ptr = p;
			ptr->Availmem -= p->s.size;
			p++;

			break;
		}

		/* no available node in free list can
		 * provide the requested buffer
		 */
		if (p == ptr->Allocp) {
			p = NULL;
			break;
		}
	}
	if (p)
		memset((void *)p, 0, nb);

	LOG3("alloc size out: %p %d avail mem: %d\n", (void *)p, nb, ptr->Availmem*8);

	return (void *)p;
}

void MEM_scratch_ua_mfree(void *ptr)
{
	struct dsp_main_struct *dsp = (struct dsp_main_struct *)GLOBAL_DSP_MEM_ADDR;
#ifdef PLATF_8MP_LPA
	MEM_scratch_mfree(OCRAM_A_mem_info, ptr);
#else
	MEM_scratch_mfree(&dsp->scratch_mem_info, ptr);
#endif
}
/* Note: This function free up the memory allocations done using
 * function MEM_heap_malloc
 */
void MEM_scratch_mfree(struct dsp_mem_info *mem_info, void *blk)
{
	register union header *p, *q;
	unsigned int i;
	struct dsp_mem_info *ptr;

	ptr = mem_info;

	if (!blk)
		return;

	p = ((union header *)blk) - 1;

	/* check whether this blocks is allocated reasonably or not */
	if (p->s.ptr != p)
		return;

#ifdef PLATF_8MP_LPA
	if (p >= RESERVED_PTR)
		ptr = RESERVED_mem_info;
#endif

	ptr->Availmem += p->s.size;

	LOG3("free size: %p %d   %d\n",p, ptr->Availmem*8, p->s.size*8);

	/* the freed buffer need to be inserted into the free list, so
	 * this place is used to find the inserting point.
	 */
	for (q = ptr->Allocp; !(p > q && p < q->s.ptr); q = q->s.ptr) {
		/* is the highest address of this circular free list */
		if (q >= q->s.ptr && (p > q || p < q->s.ptr))
			break;
	}

	/* check whether the freed buffer can be merged with the next node
	 * in free list or not.
	 */
	if (p + p->s.size == q->s.ptr) {
		p->s.size += q->s.ptr->s.size;
		p->s.ptr = q->s.ptr->s.ptr;
	} else {
		p->s.ptr = q->s.ptr;
	}

	/* check whether the freed buffer can be merged with the previous node
	 * in free list or not
	 */
	if ((q + q->s.size == p) && (q != ptr->Base)) {
		q->s.size += p->s.size;
		q->s.ptr = p->s.ptr;
	} else {
		q->s.ptr = p;
	}
}
