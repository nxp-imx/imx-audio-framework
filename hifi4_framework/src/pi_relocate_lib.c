/*
Copyright (c) 2014 Cadence Design Systems Inc.
Copyright 2018 NXP

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/



#include <xtensa/config/core.h>
#include <xtensa/config/defs.h>
#include "elf.h"
#include "xt_library_loader.h"
#include "loader_internal.h"
#include "mydefs.h"
#define BPW                (8*sizeof(Elf32_Word))
#define DIV_ROUNDUP(a, b)  (((a) + (b) - 1)/(b))

/* We cannot allow FLIXing of loads and stores to IRAM. This may
   cause runtime error if FLIX formats allow placing this operations
   into a slot other than slot number zero.
   Using assembly for this operations prevents bundling of loads and
   stores but we still benefit from bundling other ops.
*/

static inline unsigned int
iram_load (void *ptr)
{
  unsigned int t;
  asm volatile("l32i %0, %1, 0" : "=r"(t) : "r"(ptr) : "memory");
  return t;
}

static inline void
iram_store (void *ptr, unsigned int value)
{
  asm volatile("s32i %0, %1, 0" :: "r"(value), "r"(ptr) : "memory");
}

/* Extract word from {lo, hi} pair at byte offset 'ofs'. This is equivalent
   of unaligned load from aligned memory region {lo, hi}. */

static inline unsigned int
extract (unsigned int lo, unsigned int hi, int ofs)
{
#ifdef __XTENSA_EB__
  XT_SSA8B(ofs);
  return XT_SRC(lo, hi);
#else
  XT_SSA8L(ofs);
  return XT_SRC(hi, lo);
#endif
}

/* Sets word at byte offset 'ofs' in the pair {lo, hi} to 'value'. This is equivalent
   of unaligned store into aligned memory region {lo, hi}. */

static inline void
combine (unsigned int *lo, unsigned int *hi, unsigned int value, int ofs)
{
  unsigned int x0, x1;
#ifdef __XTENSA_EB__
  XT_SSA8B(ofs);
  x0 = XT_SRC(*lo, *lo);
  x1 = XT_SRC(*hi, *hi);
  XT_SSA8L(ofs);
  *lo = XT_SRC(x0, value);
  *hi = XT_SRC(value, x1);
#else
  XT_SSA8L(ofs);
  x0 = XT_SRC(*lo, *lo);
  x1 = XT_SRC(*hi, *hi);
  XT_SSA8B(ofs);
  *lo = XT_SRC(value, x0);
  *hi = XT_SRC(x1, value);
#endif
}

#if XCHAL_HAVE_CONST16

/* Helper functions for copying instructions between IRAM and word aligned buffer.
 */

/* copy_in copies 'bits' number of bits starting from word aligned pointer 'src' at
   byte offset 'src_ofs' into word-aligned buffer 'buf'.
   The actual number of bytes copied is higher, this helps to write this buffer back
   without need to read data from IRAM again. The actual 'src':'src_pos' data starts
   from word dst[1]. Th word dst[0] holds data from the start of 'src' up to 'src_pos'
   byte.
*/
static inline void
copy_in (Elf32_Word *dst, Elf32_Word *src, int src_ofs, int bits)
{
  Elf32_Word copied;
  Elf32_Word count = DIV_ROUNDUP(bits+8*src_ofs, BPW);

  if (src_ofs == 0) { /* special case - source is word-aligned */
    dst++;
    for (copied=0; copied<count; copied++)
      *dst++ = iram_load (src++);
  } else {
    Elf32_Word last = 0;
    Elf32_Word curr;
#ifdef __XTENSA_EB__
    XT_SSA8B(src_ofs);
    for (copied=0; copied<count; copied ++) {
      *dst++ = XT_SRC(last, (curr = iram_load (src++)));
      last = curr;
    }
#else
    XT_SSA8L(src_ofs);
    for (copied=0; copied<count; copied ++) {
      *dst++ = XT_SRC((curr = iram_load (src++)), last);
      last = curr;
    }
#endif
    *dst = XT_SRC (last, last);
  }
}

/* copy_out writes data buffered using copy_in back to IRAM.
   'src_ofs' and 'bits' has to be the same as in copy_in call
   but 'dst' now is the 'src' from copy_in and 'src' in copy_out
   is the 'dst' from copy_in. */

static inline void
copy_out (Elf32_Word *dst, Elf32_Word *src, int src_ofs, int bits)
{
  Elf32_Word copied;
  Elf32_Word count = DIV_ROUNDUP(bits+8*src_ofs, BPW);

  if (src_ofs == 0) { /* special case - desctionation is word-aligned */
    src++;
    for (copied=0; copied<count; copied ++)
      iram_store (dst++, *src++);
  } else {
    Elf32_Word last = *src++;
    Elf32_Word curr;
#ifdef __XTENSA_EB__
    XT_SSA8L(src_ofs);
    for (copied=0; copied<count; copied ++) {
      iram_store (dst++, XT_SRC(last, (curr = *src++)));
      last = curr;
    }
#else
    XT_SSA8B(src_ofs);
    for (copied=0; copied<count; copied ++) {
      iram_store (dst++, XT_SRC((curr = *src++), last));
      last = curr;
    }
#endif
  }
}


/* including xtimm_ functions */

#define uint32_t Elf32_Word

#include <xtensa/config/core-const16w.inc>

static int
relocate_op (char *ptr, int slot, Elf32_Word value)
{
  Elf32_Word format_length, a_ofs, mem_lo, mem_hi, insn;
  unsigned *a_ptr;

  a_ofs = (unsigned)ptr % sizeof(unsigned);
  a_ptr = (unsigned *)((unsigned)ptr - a_ofs);

  mem_lo = iram_load (a_ptr);
#ifdef __XTENSA_EB__
  insn = mem_lo << (8*a_ofs);
#else
  insn = mem_lo >> (8*a_ofs);
#endif

  format_length = xtimm_get_format_length (insn);
  if (format_length == 0)
    return XTLIB_RELOCATION_ERR;

  if (format_length <= 32) {
    if (format_length + 8*a_ofs <= 32) {
      if (!xtimm_fmt_small_immediate_field_set (&insn, slot, value))
	return XTLIB_RELOCATION_ERR;
      combine (&mem_lo, &mem_hi, insn, a_ofs);
      iram_store(a_ptr, mem_lo);
    } else {
      mem_hi = iram_load (a_ptr + 1);
      insn = extract (mem_lo, mem_hi, a_ofs);
      if (!xtimm_fmt_small_immediate_field_set (&insn, slot, value))
	return XTLIB_RELOCATION_ERR;
      combine (&mem_lo, &mem_hi, insn, a_ofs);
      iram_store(a_ptr, mem_lo);
      iram_store(a_ptr+1, mem_hi);
    }
  } else {
    Elf32_Word buf[DIV_ROUNDUP(XTIMM_MAX_FORMAT_LENGTH, BPW)+2];
    copy_in (buf, a_ptr, a_ofs, format_length);
    if (!xtimm_fmt_large_immediate_field_set (&buf[1], slot, value))
      return XTLIB_RELOCATION_ERR;
    copy_out (a_ptr, buf, a_ofs, format_length);
  }
  return XTLIB_NO_ERR;
}

#endif /* XCHAL_HAVE_CONST16 */

static Elf32_Addr
reloc_addr (const xtlib_pil_info * lib_info, Elf32_Addr addr)
{
  return ((addr >= (Elf32_Addr)lib_info->src_data_offs)
	  ? ((Elf32_Addr)lib_info->dst_data_addr - lib_info->src_data_offs)
	  : ((Elf32_Addr)lib_info->dst_addr - lib_info->src_offs))
    + addr;
}

int
xtlib_relocate_pi_lib (xtlib_pil_info * lib_info, xtlib_loader_globals *xtlib_globals)
{
  int i;
  Elf32_Rela * relocations = (Elf32_Rela *) lib_info->rel;

  for (i = 0; i < lib_info->rela_count; i++) {
    Elf32_Rela * rela = &relocations[i];
    Elf32_Word r_type;

    if (ELF32_R_SYM(rela->r_info) != STN_UNDEF) {
      xtlib_globals->err = XTLIB_UNKNOWN_SYMBOL;
      return XTLIB_UNKNOWN_SYMBOL;
    }

    r_type = ELF32_R_TYPE(rela->r_info);

#if XCHAL_HAVE_CONST16
    /* instruction specific relocation, at the moment only const16 expected  */
    if (r_type >= R_XTENSA_SLOT0_OP && r_type <= R_XTENSA_SLOT14_OP) {
      int status = relocate_op ((void *)reloc_addr (lib_info, rela->r_offset),
				(r_type - R_XTENSA_SLOT0_OP),
				reloc_addr (lib_info, rela->r_addend));
      if (status != XTLIB_NO_ERR)
       return status;
    } else
    if (r_type >= R_XTENSA_SLOT0_ALT && r_type <= R_XTENSA_SLOT14_ALT) {
      int status = relocate_op ((void *)reloc_addr (lib_info, rela->r_offset),
				(r_type - R_XTENSA_SLOT0_ALT),
				reloc_addr (lib_info, rela->r_addend) >> 16);
      if (status != XTLIB_NO_ERR)
       return status;
    } else
#endif
    if (r_type == R_XTENSA_RELATIVE) {
      Elf32_Word *addr = (Elf32_Word*)reloc_addr (lib_info, rela->r_offset);
      Elf32_Word a_ofs = (Elf32_Word)addr % 4;
      // check if it's 4 byte aligned, C++ exception tables may have
      // unaligned relocations
      if (a_ofs == 0) {
          iram_store (addr, reloc_addr (lib_info, iram_load (addr) + rela->r_addend));
      } else {
          Elf32_Word hi, lo, val;
          Elf32_Word *a_ptr = (Elf32_Word*)((Elf32_Word)addr - a_ofs);
          lo = iram_load (a_ptr);
          hi = iram_load (a_ptr+1);
          val = extract (lo, hi, a_ofs);
          val = reloc_addr (lib_info, val + rela->r_addend);
          combine (&lo, &hi, val, a_ofs);
          iram_store (a_ptr, lo);
          iram_store (a_ptr+1, hi);
      }
    } else if (r_type == R_XTENSA_NONE) {
    } else
      return XTLIB_RELOCATION_ERR;
  }
  return XTLIB_NO_ERR;
}
