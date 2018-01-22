//*****************************************************************
// Copyright (c) 2017 Cadence Design Systems, Inc.
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "xt_library_loader.h"
#include "system_address.h"

#include "mydefs.h"
#include "mytypes.h"
#include "icm_dpu.h"

typedef enum {
	DTYP_DPUSW = 0,
	DTYP_DPULIB_MP3DEC,
	DTYP_DPULIB_CUSTOM,
	DTYP_AUD_MP3DAT = 16,
	DTYP_AUD_AACDAT
} sram_data_type_t;

typedef struct {
	int  data_type;		// dpu_sw, mp3dec lib, ..., mp3data, aacdata
	unsigned char *ppkg_in_sram;		// address in System Ram
	int  data_size;		// size of this data
} dpu_data_t;


/* TODO: These two should be passed from APU, may be as part of ICM or sharing common offsets */
//volatile xtlib_pil_info pil_inf_hifiLib_dpu __attribute__((section(".sram.data")));
//volatile dpu_data_t dpu_block[3] __attribute__((section(".sram.data")));

typedef enum {
	lib_unloaded = 0,
	lib_loaded,
	lib_in_use
} pilib_status_t;


int dpu_process_init_pi_lib(unsigned int *extmsg, codec_info *codec_info_ptr)
{
	u32 ret_val;
	u32 (*plib_entry_func)() =0;
	icm_header_t dpu_icm;
	xtlib_pil_info *pil_info = (xtlib_pil_info *)extmsg;
	void *pg_xa_process_api;

//	memcpy((char *)&lib_on_dpu.ol_inf_hifiLib_dpu, (char *)extmsg, sizeof(xtlib_overlay_info));
//	plib_entry_func = (u32 (*)())lib_on_dpu.ol_inf_hifiLib_dpu.start_addr;
//	LOG1("library start addr %x\n",lib_on_dpu.ol_inf_hifiLib_dpu.start_addr);
//	lib_on_dpu.type = lib_on_dpu.ol_inf_hifiLib_dpu.codec_type;

	memcpy((char *)&(codec_info_ptr->lib_on_dpu.pil_inf_hifiLib_dpu), (char *)extmsg, sizeof(xtlib_pil_info));
	plib_entry_func = (void *)xtlib_target_init_pi_library_s(&(codec_info_ptr->lib_on_dpu.pil_inf_hifiLib_dpu), &codec_info_ptr->xtlib_globals);
	LOG1("xtlib_target_init_pi_library, %x\n",plib_entry_func);

	// 	get the codec_api_function
	pg_xa_process_api = (void *)(plib_entry_func)();

	codec_info_ptr->lib_on_dpu.stat = lib_loaded;
	codec_info_ptr->lib_on_dpu.p_xa_process_api = pg_xa_process_api;

	return 0;
}
#if 0
int check_libload_status(int lib_type, codec_info *codec_info_ptr)
{
	int ret_val = 0;
	hifiCdc_inst_t *inst = codec_info_ptr->inst;

	if (lib_type != inst->lib_on_dpu.type)
		ret_val = -1;
	else
	{
		if (inst->lib_on_dpu.stat == lib_unloaded)
			ret_val = -1;
	}
	return ret_val;
}
#endif
void dpu_process_unload_pi_lib(codec_info *codec_info_ptr)
{
	/*
		When the library is no longer needed, call the following function on the target processor before
		releasing the memory allocated for the library:
		void xtlib_unload_pi_library (xtlib_pil_info * lib_info);
	 */
	xtlib_unload_pi_library (&(codec_info_ptr->lib_on_dpu.pil_inf_hifiLib_dpu));
	memset((char *)&(codec_info_ptr->lib_on_dpu.pil_inf_hifiLib_dpu), 0, sizeof(xtlib_pil_info));
	codec_info_ptr->lib_on_dpu.stat = lib_unloaded;

	LOG("PIL library unloaded \n");
	return;
}
#if 0
void get_mem_for_pilib(icm_pilib_size_t *plib_sz_info, icm_pilib_alloc_resp_t *plib_minfo, codec_info *codec_info_ptr)
{
	hifiCdc_inst_t *inst = codec_info_ptr->inst;

	plib_minfo->codec_type = plib_sz_info->codec_type;

	{
		inst->lib_on_dpu.type = plib_sz_info->codec_type;
		inst->lib_on_dpu.stat = lib_unloaded;
	}

	return;
}
#endif
