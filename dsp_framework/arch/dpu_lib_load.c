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
#include "dpu_lib_load.h"


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
//volatile xtlib_pil_info pil_inf_dspLib_dpu __attribute__((section(".sram.data")));
//volatile dpu_data_t dpu_block[3] __attribute__((section(".sram.data")));

void *dpu_process_init_pi_lib(xtlib_pil_info *pil_info, dpu_lib_stat_t *lib_stat, u32 byteswap)
{
	xtlib_loader_globals xtlib_globals = {0,0};
	u32 (*plib_entry_func)() =0;
	void *pg_xa_process_api;

	xtlib_globals.byteswap = byteswap;

	memcpy((char *)&(lib_stat->pil_inf_dspLib_dpu), (char *)pil_info, sizeof(xtlib_pil_info));
	plib_entry_func = (void *)xtlib_target_init_pi_library_s(&(lib_stat->pil_inf_dspLib_dpu), &xtlib_globals);
	LOG1("xtlib_target_init_pi_library, %x\n",plib_entry_func);

	//get the codec_api_function
	pg_xa_process_api = (void *)(plib_entry_func)();
	if(!pg_xa_process_api)
	{
		lib_stat->stat = lib_unloaded;
		return NULL;
	}

	lib_stat->stat = lib_loaded;

	return pg_xa_process_api;
}
#if 0
int check_libload_status(int lib_type, codec_info *codec_info_ptr)
{
	int ret_val = 0;
	dspCdc_inst_t *inst = codec_info_ptr->inst;

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
void dpu_process_unload_pi_lib(dpu_lib_stat_t *lib_stat)
{
	/*
	    When the library is no longer needed, call the following function on the target processor before
	    releasing the memory allocated for the library:
	    void xtlib_unload_pi_library (xtlib_pil_info * lib_info);
	 */
//	xtlib_unload_pi_library (&(lib_stat->pil_inf_dspLib_dpu));
	memset((char *)&(lib_stat->pil_inf_dspLib_dpu), 0, sizeof(xtlib_pil_info));
	lib_stat->stat = lib_unloaded;

	LOG("PIL library unloaded \n");
	return;
}
#if 0
void get_mem_for_pilib(icm_pilib_size_t *plib_sz_info, icm_pilib_alloc_resp_t *plib_minfo, codec_info *codec_info_ptr)
{
	dspCdc_inst_t *inst = codec_info_ptr->inst;

	plib_minfo->codec_type = plib_sz_info->codec_type;

	{
		inst->lib_on_dpu.type = plib_sz_info->codec_type;
		inst->lib_on_dpu.stat = lib_unloaded;
	}

	return;
}
#endif
