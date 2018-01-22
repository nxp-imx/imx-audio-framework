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

#include "mytypes.h"
#include "mydefs.h"
#include "memory.h"
#include "peripheral.h"
#ifdef DEBUG
#include "printf.h"
#endif


HIFI_ERROR_TYPE hifiCdc_init(codec_info *codec_info_ptr, hifi_main_struct *hifi_config)
{
	HiFiCodecMemoryOps memops;
	HIFI_ERROR_TYPE ret = XA_SUCCESS;

	if(codec_info_ptr->codecwrapinterface == NULL)
		return XA_INIT_ERR;

	codec_info_ptr->codecwrapinterface(ACODEC_API_CREATE_CODEC, (void **)&codec_info_ptr->WrapFun.Create);
	codec_info_ptr->codecwrapinterface(ACODEC_API_DELETE_CODEC, (void **)&codec_info_ptr->WrapFun.Delete);
	codec_info_ptr->codecwrapinterface(ACODEC_API_INIT_CODEC, (void **)&codec_info_ptr->WrapFun.Init);
	codec_info_ptr->codecwrapinterface(ACODEC_API_RESET_CODEC, (void **)&codec_info_ptr->WrapFun.Reset);
	codec_info_ptr->codecwrapinterface(ACODEC_API_SET_PARAMETER, (void **)&codec_info_ptr->WrapFun.SetPara);
	codec_info_ptr->codecwrapinterface(ACODEC_API_GET_PARAMETER, (void **)&codec_info_ptr->WrapFun.GetPara);
	codec_info_ptr->codecwrapinterface(ACODEC_API_DECODE_FRAME, (void **)&codec_info_ptr->WrapFun.Process);
	codec_info_ptr->codecwrapinterface(ACODEC_API_GET_LAST_ERROR, (void **)&codec_info_ptr->WrapFun.GetLastError);

	memops.Malloc = (void *)MEM_scratch_malloc;
	memops.Free = (void *)MEM_scratch_mfree;
#ifdef DEBUG
	memops.hifi_printf = __hifi_printf;
#endif

	memops.p_xa_process_api = codec_info_ptr->codecinterface;
	memops.hifi_config = hifi_config;

	if(codec_info_ptr->WrapFun.Create == NULL)
		return XA_INIT_ERR;

	codec_info_ptr->pWrpHdl = codec_info_ptr->WrapFun.Create(&memops, codec_info_ptr->codec_id);
	if(codec_info_ptr->pWrpHdl == NULL) {
		LOG("Create codec error in codec wrapper\n");
		return XA_INIT_ERR;
	}

	return ret;
}

HIFI_ERROR_TYPE hifiCdc_open(codec_info *codec_info_ptr)
{
	HIFI_ERROR_TYPE ret;

	if(codec_info_ptr->WrapFun.Init == NULL)
		return XA_INIT_ERR;

	ret = codec_info_ptr->WrapFun.Init(codec_info_ptr->pWrpHdl);

	LOG1("ret = %d\n", ret);

	return ret;
}

HIFI_ERROR_TYPE xa_hifi_cdc_set_param_config(icm_prop_config *pext_msg, codec_info *codec_info_ptr)
{
	HIFI_ERROR_TYPE ret;
	HiFiCodecSetParameter param;

	param.cmd = pext_msg->cmd;
	param.val = pext_msg->val;

	LOG2("param.cmd = %x, param.val = %d\n", param.cmd, param.val);

	if(codec_info_ptr->WrapFun.SetPara == NULL)
		return XA_INIT_ERR;

	ret = codec_info_ptr->WrapFun.SetPara(codec_info_ptr->pWrpHdl, &param);

	return ret;
}

HIFI_ERROR_TYPE hifiCdc_getprop(icm_pcm_prop_t *pext_msg, codec_info *codec_info_ptr)
{
	HIFI_ERROR_TYPE ret;
	HiFiCodecGetParameter param;

	if(codec_info_ptr->WrapFun.GetPara == NULL)
		return XA_INIT_ERR;

	ret = codec_info_ptr->WrapFun.GetPara(codec_info_ptr->pWrpHdl, &param);

	pext_msg->bits = param.bits;
	pext_msg->channels = param.channels;
	pext_msg->pcmbytes = param.pcmbytes;
	pext_msg->sfreq = param.sfreq;
	pext_msg->consumed_bytes = param.consumed_bytes;
	pext_msg->cycles = param.cycles;

	return ret;
}

HIFI_ERROR_TYPE hifiCdc_process_emptybuf(icm_cdc_iobuf_t *pext_msg, codec_info *codec_info_ptr)
{
	HIFI_ERROR_TYPE ret;
	icm_cdc_iobuf_t *psysram_iobuf = &(codec_info_ptr->sysram_io);
	u32 inp_buf_ptr, inp_buf_size, offset;

	if(codec_info_ptr->WrapFun.Process == NULL)
		return XA_INIT_ERR;

	psysram_iobuf->inp_addr_sysram = pext_msg->inp_addr_sysram;
	psysram_iobuf->inp_buf_size_max = pext_msg->inp_buf_size_max;
	psysram_iobuf->inp_cur_offset = pext_msg->inp_cur_offset;

	psysram_iobuf->out_addr_sysram = pext_msg->out_addr_sysram;
	psysram_iobuf->out_buf_size_max = pext_msg->out_buf_size_max;
	psysram_iobuf->out_cur_offset = 0;

	codec_info_ptr->input_over = pext_msg->input_over;

	inp_buf_ptr = psysram_iobuf->inp_addr_sysram + psysram_iobuf->inp_cur_offset;
	inp_buf_size = psysram_iobuf->inp_buf_size_max - psysram_iobuf->inp_cur_offset;
	offset = 0;

	ret = codec_info_ptr->WrapFun.Process(codec_info_ptr->pWrpHdl,
						  (uint8 *)inp_buf_ptr,
						  inp_buf_size,
						  &offset,
						  (uint8 **)&psysram_iobuf->out_addr_sysram,
						  &psysram_iobuf->out_cur_offset,
						  codec_info_ptr->input_over);

	psysram_iobuf->inp_cur_offset = psysram_iobuf->inp_cur_offset + offset;

	LOG1("decode ret = %d\n", ret);

	return ret;
}

HIFI_ERROR_TYPE hifiCdc_close(codec_info *codec_info_ptr)
{
	HIFI_ERROR_TYPE ret;

	if(codec_info_ptr->WrapFun.Delete == NULL)
		return XA_INIT_ERR;

	ret = codec_info_ptr->WrapFun.Delete(codec_info_ptr->pWrpHdl);

	return ret;
}

HIFI_ERROR_TYPE hifiCdc_reset(codec_info *codec_info_ptr)
{
	HIFI_ERROR_TYPE ret;

	if(codec_info_ptr->WrapFun.Reset == NULL)
		return XA_INIT_ERR;

	ret = codec_info_ptr->WrapFun.Reset(codec_info_ptr->pWrpHdl);

	return ret;
}

