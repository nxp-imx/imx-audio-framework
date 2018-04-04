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

#ifndef _DPU_LIB_LOAD_
#define _DPU_LIB_LOAD_

#include "xf-types.h"
#include "xt_library_loader.h"
#include "system_address.h"
#include "loader_internal.h"
#include "xf-debug.h"

typedef enum {
	HIFI_CODEC_LIB =1,
	HIFI_CODEC_WRAP_LIB
} lib_type;

typedef enum {
	lib_unloaded = 0,
	lib_loaded,
	lib_in_use
} pilib_status_t;

typedef struct {
	xtlib_pil_info pil_info;
	u32 lib_type;
} icm_xtlib_pil_info;

typedef struct {
	u32 start_addr;
	u32 codec_type;
} xtlib_overlay_info;

typedef struct {
	u32 type: 16;   /* codec type; e.g., 1: mp3dec */
	u32 stat: 16;   /* loaded, init_done, active, closed */
	u32 size_libtext;
	u32 size_libdata;
	xtlib_overlay_info ol_inf_hifiLib_dpu;
	xtlib_pil_info pil_inf_hifiLib_dpu;
} dpu_lib_stat_t;

void *dpu_process_init_pi_lib(xtlib_pil_info *pil_info, dpu_lib_stat_t *lib_stat, u32 byteswap);
void dpu_process_unload_pi_lib(dpu_lib_stat_t *lib_stat);

#endif
