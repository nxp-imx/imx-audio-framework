/*
 * Copyright (c) 2012-2013 by Tensilica Inc. ALL RIGHTS RESERVED.
 * Copyright 2018 NXP
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "elf.h"
#include "xt_library_loader.h"
#include "loader_internal.h"

#ifdef __XTENSA__
#include <xtensa/hal.h>
#endif

int
xtlib_verify_magic(Elf32_Ehdr *header, xtlib_loader_globals *xtlib_globals)
{
	Elf32_Byte magic_no;

	magic_no =  header->e_ident[EI_MAG0];
	if (magic_no != 0x7f)
		return -1;

	magic_no = header->e_ident[EI_MAG1];
	if (magic_no != 'E')
		return -1;

	magic_no = header->e_ident[EI_MAG2];
	if (magic_no != 'L')
		return -1;

	magic_no = header->e_ident[EI_MAG3];
	if (magic_no != 'F')
		return -1;

	if (header->e_ident[EI_CLASS] != ELFCLASS32)
		return -1;

	{
		/* determine byte order  */
		union {
		short s;
		char c[sizeof(short)];
	} u;

	u.s = 1;

	if (header->e_ident[EI_DATA] == ELFDATA2LSB)
		xtlib_globals->byteswap = u.c[sizeof(short) - 1] == 1;
	else if (header->e_ident[EI_DATA] == ELFDATA2MSB)
		xtlib_globals->byteswap = u.c[0] == 1;
	else
		return -1;
	}

	return 0;
}

void
xtlib_load_seg(Elf32_Phdr *pheader, void *src_addr, xt_ptr dst_addr,
	       memcpy_func_ex mcpy, memset_func_ex mset, void *user,
	       xtlib_loader_globals *xtlib_globals)
{
	Elf32_Word bytes_to_copy = xtlib_host_word(pheader->p_filesz, xtlib_globals);
	Elf32_Word bytes_to_zero = xtlib_host_word(pheader->p_memsz, xtlib_globals) - bytes_to_copy;

	xt_ptr zero_addr = dst_addr + bytes_to_copy;

	if (bytes_to_copy > 0) {
		mcpy(dst_addr, src_addr, bytes_to_copy, user);

#ifdef __XTENSA__
		if (pheader->p_flags & PF_X) {
			xthal_dcache_region_writeback(dst_addr, bytes_to_copy);
			xthal_icache_region_invalidate(dst_addr, bytes_to_copy);
		}
#endif
	}

	if (bytes_to_zero > 0)
		mset(zero_addr, 0, bytes_to_zero, user);
}

unsigned int
xtlib_error(xtlib_loader_globals *xtlib_globals)
{
	return xtlib_globals->err;
}

Elf32_Half
xtlib_host_half(Elf32_Half v, xtlib_loader_globals *xtlib_globals)
{
	return (xtlib_globals->byteswap) ? (v >> 8) | (v << 8) : v;
}

Elf32_Word
xtlib_host_word(Elf32_Word v, xtlib_loader_globals *xtlib_globals)
{
	if (xtlib_globals->byteswap) {
		v = ((v & 0x00FF00FF) << 8) | ((v & 0xFF00FF00) >> 8);
		v = (v >> 16) | (v << 16);
	}
	return v;
}

#ifdef __XTENSA__

void
xtlib_sync(void)
{
	xthal_dcache_sync();
	xthal_icache_sync();
#if XCHAL_HAVE_LOOPS
	asm __volatile__ ("movi a7, 0\n wsr.lcount a7");
#endif
}

xt_ptr
xtlib_user_memcpy(xt_ptr dest, const void *src, unsigned int n, void *user)
{
	return ((user_funcs *)user)->mcpy(dest, src, n);
}

xt_ptr
xtlib_user_memset(xt_ptr s, int c, unsigned int n, void *user)
{
	return ((user_funcs *)user)->mset(s, c, n);
}

#endif /* __XTENSA__ */
