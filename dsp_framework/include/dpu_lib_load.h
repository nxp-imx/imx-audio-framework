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

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************/

#ifndef _DPU_LIB_LOAD_
#define _DPU_LIB_LOAD_

#include "xa_type_def.h"
#include "xt_library_loader.h"
#include "loader_internal.h"
#include "debug.h"

enum pilib_status_t {
	lib_unloaded = 0,
	lib_loaded,
	lib_in_use
};

struct icm_xtlib_pil_info {
	xtlib_pil_info pil_info;
	UWORD32 lib_type;
};

struct xtlib_overlay_info {
	UWORD32 start_addr;
	UWORD32 codec_type;
};

struct dpu_lib_stat_t {
	UWORD32 type: 16;   /* codec type; e.g., 1: mp3dec */
	UWORD32 stat: 16;   /* loaded, init_done, active, closed */
	UWORD32 size_libtext;
	UWORD32 size_libdata;
	struct xtlib_overlay_info ol_inf_dspLib_dpu;
	xtlib_pil_info pil_inf_dspLib_dpu;
};

void *dpu_process_init_pi_lib(xtlib_pil_info *pil_info,
			      struct dpu_lib_stat_t *lib_stat,
			      UWORD32 byteswap);
void dpu_process_unload_pi_lib(struct dpu_lib_stat_t *lib_stat);

#endif
