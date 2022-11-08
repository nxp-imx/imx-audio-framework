/*****************************************************************
 * Copyright 2018 NXP
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:

 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************/

#ifndef __LIBRARY_LOAD_H
#define __LIBRARY_LOAD_H

#include "xf.h"
#include "xaf-structs.h"

#define Elf32_Byte  unsigned char
#define xt_ptr      unsigned long
#define xt_int      int
#define xt_uint     unsigned int
#define xt_ulong    unsigned long

struct xtlib_packaged_library;

enum {
	XTLIB_NO_ERR = 0,
	XTLIB_NOT_ELF = 1,
	XTLIB_NOT_DYNAMIC = 2,
	XTLIB_NOT_STATIC = 3,
	XTLIB_NO_DYNAMIC_SEGMENT = 4,
	XTLIB_UNKNOWN_SYMBOL = 5,
	XTLIB_NOT_ALIGNED = 6,
	XTLIB_NOT_SPLITLOAD = 7,
	XTLIB_RELOCATION_ERR = 8
};

struct xtlib_loader_globals {
	int err;
	int byteswap;
};

struct xtlib_pil_info {
	xt_uint  dst_addr;
	xt_uint  src_offs;
	xt_uint  dst_data_addr;
	xt_uint  src_data_offs;
	xt_uint  start_sym;
	xt_uint  text_addr;
	xt_uint  init;
	xt_uint  fini;
	xt_uint  rel;
	xt_int  rela_count;
	xt_uint  hash;
	xt_uint  symtab;
	xt_uint  strtab;
	xt_int  align;
	xt_uint scratch_section_found;
	xt_ptr  scratch_addr;
};

struct icm_xtlib_pil_info {
	struct xtlib_pil_info pil_info;
	unsigned int lib_type;
};

struct lib_dnld_info_t {
	unsigned long pbuf_code;
	unsigned long pbuf_data;
	unsigned int size_code;
	unsigned int size_data;
	struct xtlib_pil_info *ppil_inf;
	unsigned int lib_on_dpu;    /* 0: not loaded, 1: loaded. */
};

struct lib_info {
	struct xtlib_pil_info  pil_info;
	struct xtlib_loader_globals	xtlib_globals;

	struct xf_pool	 *code_section_pool;
	struct xf_pool	 *data_section_pool;

	void		 *code_buf_virt;
	unsigned int code_buf_phys;
	unsigned int code_buf_size;
	void		 *data_buf_virt;
	unsigned int data_buf_phys;
	unsigned int data_buf_size;

	const char   *filename;
	unsigned int lib_type;
};

long xf_load_lib(xaf_comp_t *handle, struct lib_info *lib_info);
long xf_unload_lib(xaf_comp_t *handle, struct lib_info *lib_info);
XAF_ERR_CODE xaf_load_library(xaf_adev_t *p_adev, xaf_comp_t *p_comp, xf_id_t comp_id);

#endif
