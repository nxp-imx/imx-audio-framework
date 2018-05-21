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

#ifndef PI_LIBRARY_LOAD_H
#define PI_LIBRARY_LOAD_H

#include "xt_library_loader.h"
#include "loader_internal.h"

unsigned int
xtlib_pi_library_size_s(xtlib_packaged_library * library,
			xtlib_loader_globals * xtlib_globals);

unsigned int
xtlib_split_pi_library_size_s(xtlib_packaged_library *library,
			      unsigned int *code_size,
			      unsigned int *data_size,
			      xtlib_loader_globals *xtlib_globals);

void *
xtlib_load_pi_library_s(xtlib_packaged_library *library,
			void *destination_address,
			xtlib_pil_info *lib_info,
			xtlib_loader_globals *xtlib_globals);

void *
xtlib_load_pi_library_custom_fn_s(xtlib_packaged_library *library,
				  void *destination_address,
				  xtlib_pil_info *lib_info,
				  memcpy_func mcpy,
				  memset_func mset,
				  xtlib_loader_globals *xtlib_globals);

void *
xtlib_load_split_pi_library_s(xtlib_packaged_library *library,
			      void *destination_code_address,
			      void *destination_data_address,
			      xtlib_pil_info *lib_info,
			      xtlib_loader_globals *xtlib_globals);

void *
xtlib_load_split_pi_library_custom_fn_s(xtlib_packaged_library *library,
					void *destination_code_address,
					void *destination_data_address,
					xtlib_pil_info *lib_info,
					memcpy_func mcpy,
					memset_func mset,
					xtlib_loader_globals *xtlib_globals);

void *
xtlib_target_init_pi_library_s(xtlib_pil_info *lib_info,
			       xtlib_loader_globals *xtlib_globals);

void
xtlib_unload_pi_library(xtlib_pil_info *lib_info);

void *
xtlib_pi_library_debug_addr(xtlib_pil_info *lib_info);

xt_ptr
xtlib_host_load_pi_library_s(xtlib_packaged_library *library,
			     xt_ptr destination_address,
			     xtlib_pil_info *lib_info,
			     memcpy_func_ex mcpy_fn,
			     memset_func_ex mset_fn,
			     void *user,
			     xtlib_loader_globals *xtlib_globals);

xt_ptr
xtlib_host_load_split_pi_library_s(xtlib_packaged_library *library,
				   xt_ptr destination_code_address,
				   xt_ptr destination_data_address,
				   xtlib_pil_info *lib_info,
				   memcpy_func_ex mcpy_fn,
				   memset_func_ex mset_fn,
				   void *user,
				   xtlib_loader_globals *xtlib_globals);

#endif
