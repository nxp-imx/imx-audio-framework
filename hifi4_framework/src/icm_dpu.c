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

/*
 * icm_dpu.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "mytypes.h"
#include "mydefs.h"
#include <xtensa/config/system.h>
#include "system_address.h"
#include "xt_library_loader.h"
#include "peripheral.h"
#include "icm_dpu.h"
#include "dpu_lib_load.h"
#include "process.h"


void icm_init(hifi_main_struct *hifi_config)
{
	int i = 0;
	volatile int *mu = (volatile int *)MU_PADDR;
	mu_regs *base = (mu_regs*)MU_PADDR;
	icm_header_t icm_msg;

	hifi_config->icm_que_read_index = 0;
	hifi_config->icm_que_write_index = 0;
	hifi_config->icm_remaining_msgs = 0;

	for (i=0; i<MAX_MSGS_IN_QUEUE; i++)
	{
		hifi_config->icm_msg_que[i] = (u8 *)NULL;
	}

	mu_enableinterrupt_rx(base, 0);

	icm_msg.allbits = 0;
	icm_msg.intr = 1;
	icm_msg.msg  = ICM_CORE_READY;
	icm_intr_send(icm_msg.allbits);

	return;
}

int put_icm_to_que(hifi_main_struct *hifi_config, u32 msg)
{
	mu_regs *base = (mu_regs *)MU_PADDR;
	u8 *msg_ptr;
	icm_header_t recd_msg;
	u32 ext_msg_addr;
	u32 ext_msg_size = 0;
	volatile u32 *ext_msg_ptr;

	recd_msg.allbits = msg;

	if (MAX_MSGS_IN_QUEUE == hifi_config->icm_remaining_msgs)
	{
		LOG("Error: ICM Queue full\n");
		return -1;
	}

	if (ICM_PAUSE == recd_msg.msg)
	{
		int state = (CMD_ON == recd_msg.sub_msg) ? AUD_PAUSED : AUD_DECODING;
		LOG1("...Received Pause %d\n", recd_msg.sub_msg);
		return 0;
	}

	if (recd_msg.size == 8)
	{
		mu_msg_receive(base, 1, &ext_msg_addr);
		mu_msg_receive(base, 2, &ext_msg_size);
	}

	msg_ptr = (u8 *)malloc(4 + ext_msg_size);  // header + ext_msg.

	if (msg_ptr == NULL)
	{
		LOG("Error: malloc error\n");
		return -1;
	}
	hifi_config->icm_msg_que[hifi_config->icm_que_write_index] = msg_ptr;

	recd_msg.size = ext_msg_size;

	*(u32 *)msg_ptr = recd_msg.allbits;

	memcpy(msg_ptr + 4, (u8 *)ext_msg_addr, ext_msg_size);

	ext_msg_ptr = (volatile u32 *)ext_msg_addr;

	LOG2("ext... %x, %x\n", ext_msg_addr, ext_msg_size);

	{
	int i;
	for(i=0; i<ext_msg_size/4; i++)
		LOG1("ext_msg %x\n", ext_msg_ptr[i]);
	}

	hifi_config->icm_que_write_index = (hifi_config->icm_que_write_index + 1) % MAX_MSGS_IN_QUEUE;
	hifi_config->icm_remaining_msgs++;

	return 0;
}

u32 get_icm_from_que(hifi_main_struct *hifi_config, u8 *pext_msg)
{
	icm_header_t ret_hdr;
	u32 *pmsg;

	if (0 == hifi_config->icm_remaining_msgs)
		return 0;

	pmsg = (u32 *)hifi_config->icm_msg_que[hifi_config->icm_que_read_index];

	ret_hdr.allbits = pmsg[0];

	memcpy(pext_msg, (u8 *)pmsg, (4 + ret_hdr.size));
	free(pmsg);
	hifi_config->icm_msg_que[hifi_config->icm_que_read_index] = (u8 *)NULL;

	hifi_config->icm_que_read_index = (hifi_config->icm_que_read_index + 1) % MAX_MSGS_IN_QUEUE;
	hifi_config->icm_remaining_msgs--;

	return (ret_hdr.allbits);
}

u32 icm_intr_send(u32 msg)
{
	mu_regs *base = (mu_regs*)MU_PADDR;

	mu_msg_send(base, 0, msg);

	return XT_NO_ERROR;
}

u32 icm_intr_extended_send(hifi_main_struct *hifi_config, u32 msg, u8 *ext_msg)
{
	icm_header_t lmsg;
	mu_regs *base = (mu_regs*)MU_PADDR;
	int i=0;
	volatile u8 volatile *pmsg_apu = (u8 *)(hifi_config->dpu_ext_msg.ext_msg_addr);

	lmsg.allbits = msg;

	for (i=0; i < lmsg.size; i++)
	{
		pmsg_apu[i] = ext_msg[i];
	}

	mu_msg_send(base, 1, hifi_config->dpu_ext_msg.ext_msg_addr);
	mu_msg_send(base, 2, lmsg.size);

	lmsg.size = 8;

	mu_msg_send(base, 0, lmsg.allbits);

	return XT_NO_ERROR;
}

void send_action_complete(hifi_main_struct *hifi_config, u32 msg_type, u32 ext_msg_siz, u8 *ext_msg)
{
	icm_header_t dpu_icm;
	int i=0;
	volatile u8 volatile *pmsg_apu = (u8 *)(hifi_config->dpu_ext_msg.ext_msg_addr);
	mu_regs *base = (mu_regs*)MU_PADDR;

	dpu_icm.allbits = 0;
	dpu_icm.ack = 0;
	dpu_icm.intr = 1;
	dpu_icm.msg = ICM_DPU_ACTION_COMPLETE;
	dpu_icm.sub_msg = msg_type;
	dpu_icm.size = 8;		// if ext_msg is NULL, then ext_msg_siz is ret_val;

	if (NULL != ext_msg)
	{
		for (i=0; i < ext_msg_siz; i++)
		{
			pmsg_apu[i] = ext_msg[i];
		}
	}

	xthal_dcache_all_writeback();

	mu_msg_send(base, 1, hifi_config->dpu_ext_msg.ext_msg_addr);
	mu_msg_send(base, 2, ext_msg_siz);
	mu_msg_send(base, 0, dpu_icm.allbits);

	return;
}

void interrupt_handler_icm(void *arg) {

	mu_regs *base = (mu_regs*)MU_PADDR;
	volatile icm_header_t recd_msg;
	hifi_main_struct *hifi_config = (hifi_main_struct *)arg;
	int ret_val = 0;
	u32 msg;

	xthal_dcache_all_unlock();
	xthal_dcache_all_writeback();
	xthal_dcache_all_invalidate();
	xthal_dcache_sync();
	xthal_icache_all_unlock();
	xthal_icache_all_invalidate();
	xthal_icache_sync();

	mu_msg_receive(base, 0, &msg);

	recd_msg.allbits = msg;

	put_icm_to_que(hifi_config, msg);

	return;
}

void icm_process (hifi_main_struct *hifi_config, u32 msg, u32 *pmsg_dpu)
{
	HIFI_ERROR_TYPE ret_val = 0;
	int ret = 0;
	icm_header_t dpu_icm;
	u32 action_type;
	u32 i, id;

	dpu_icm.allbits = msg;

	if (0 == dpu_icm.allbits)
		return;
	action_type = dpu_icm.msg;

	switch (action_type)
	{
		case ICM_EXT_MSG_ADDR:
			{
				icm_dpu_ext_msg *pext_msg = (icm_dpu_ext_msg *)&pmsg_dpu[1];
				hifi_config->dpu_ext_msg.ext_msg_addr = pext_msg->ext_msg_addr;
				hifi_config->dpu_ext_msg.ext_msg_size = pext_msg->ext_msg_size;
				hifi_config->dpu_ext_msg.scratch_buf_phys = pext_msg->scratch_buf_phys;
				hifi_config->dpu_ext_msg.scratch_buf_size = pext_msg->scratch_buf_size;
				hifi_config->dpu_ext_msg.hifi_config_phys = pext_msg->hifi_config_phys;
				hifi_config->dpu_ext_msg.hifi_config_size = pext_msg->hifi_config_size;

				xthal_dcache_all_unlock();
				xthal_dcache_all_writeback();
				xthal_dcache_all_invalidate();
				xthal_dcache_sync();

				xthal_set_region_attribute((void *)(hifi_config->dpu_ext_msg.scratch_buf_phys),
											hifi_config->dpu_ext_msg.scratch_buf_size,
							                XCHAL_CA_WRITEBACK, 0);

				/* initialize scratch buffer */
				hifi_config->scratch_buf_ptr = (u8 *)hifi_config->dpu_ext_msg.scratch_buf_phys;
				hifi_config->scratch_total_size = hifi_config->dpu_ext_msg.scratch_buf_size;
				hifi_config->scratch_remaining = hifi_config->dpu_ext_msg.scratch_buf_size;

				/* initialize codec info */
				for(i = 0; i < MULTI_CODEC_NUM; i++)
					memset((void *)(&hifi_config->codec_info_client[i]), 0, sizeof(codec_info));

				LOG1("dcache attribute %x\n", xthal_get_dcacheattr());
				LOG1("icache attribute %x\n", xthal_get_icacheattr());
			}
			break;
		case ICM_PI_LIB_MEM_ALLOC:
			LOG1("Processing ICM msg 0x%08x\n", action_type);
			{
				icm_pilib_size_t *pext_msg = (icm_pilib_size_t *)&pmsg_dpu[1];
				icm_pilib_size_t resp_msg = {0,0,0};

				resp_msg.buffer_addr = (u32)MEM_scratch_malloc(hifi_config, pext_msg->buffer_size);
				if((u8 *)resp_msg.buffer_addr == NULL)
					resp_msg.ret = XA_INSUFFICIENT_MEM;

				send_action_complete(hifi_config, action_type, sizeof(icm_pilib_size_t), (u8 *)&resp_msg);
			}
			LOG("LIB_MEM_ALLOC complete\n");
			break;

		case ICM_PI_LIB_MEM_FREE:
			LOG1("Processing ICM msg 0x%08x\n", action_type);
			{
				icm_pilib_size_t *pext_msg = (icm_pilib_size_t *)&pmsg_dpu[1];
				icm_pilib_size_t resp_msg = {0,0,0};

				if(pext_msg->buffer_addr)
				{
					MEM_scratch_mfree(hifi_config, pext_msg->buffer_addr);
					resp_msg.buffer_addr = 0;
					resp_msg.ret = XA_SUCCESS;
				}

				send_action_complete(hifi_config, action_type, sizeof(icm_pilib_size_t), (u8 *)&resp_msg);
			}
			LOG("LIB_MEM_FREE complete\n");
			break;

		case ICM_PI_LIB_LOAD:
			LOG1("Processing ICM m 0x%08x\n", action_type);
			{
				icm_xtlib_pil_info *pext_msg = (icm_xtlib_pil_info *)&pmsg_dpu[1];
				icm_base_info_t resp_msg;
				id = pext_msg->process_id;

				codec_info *codec_info_ptr = &hifi_config->codec_info_client[id];

				ret_val = dpu_process_init_pi_lib(&pmsg_dpu[1], codec_info_ptr);	// passing the size of the extended message

				if(pext_msg->lib_type == HIFI_CODEC_LIB)
					codec_info_ptr->codecinterface = codec_info_ptr->lib_on_dpu.p_xa_process_api;
				else if(pext_msg->lib_type == HIFI_CODEC_WRAP_LIB)
					codec_info_ptr->codecwrapinterface = codec_info_ptr->lib_on_dpu.p_xa_process_api;

				LOG("LOAD_PI_LIB complete\n");
				resp_msg.ret = ret_val;
				send_action_complete(hifi_config, action_type, sizeof(icm_base_info_t), (u8 *)&resp_msg);
			}
			break;

		case ICM_PI_LIB_UNLOAD:
			LOG1("Processing ICM msg 0x%08x\n", action_type);
			{
				icm_base_info_t *pext_msg = (icm_base_info_t *)&pmsg_dpu[1];
				icm_base_info_t resp_msg;
				id = pext_msg->process_id;

				codec_info *codec_info_ptr = &hifi_config->codec_info_client[id];

				dpu_process_unload_pi_lib(codec_info_ptr);

				resp_msg.ret = ret_val;

				send_action_complete(hifi_config, action_type, sizeof(icm_base_info_t), (u8 *)&resp_msg);
				LOG("UNLOAD_PI_LIB complete\n");
			}
			break;

		case ICM_PI_LIB_INIT:
			LOG1("Processing ICM msg 0x%08x\n", action_type);
			{
				icm_base_info_t *pext_msg = (icm_base_info_t *)&pmsg_dpu[1];
				icm_base_info_t resp_msg = {0,};
				id = pext_msg->process_id;

				codec_info *codec_info_ptr = &hifi_config->codec_info_client[id];

				codec_info_ptr->codec_id = pext_msg->codec_id;
				ret_val = hifiCdc_init(codec_info_ptr, hifi_config);
				if (ret_val != XA_SUCCESS)
				{
					LOG("ICM_PI_LIB_INIT error\n");
				}
				resp_msg.ret = ret_val;

				send_action_complete(hifi_config, action_type, sizeof(icm_base_info_t), (u8 *)&resp_msg);
			}
			break;

		case ICM_OPEN:
			LOG1("Processing ICM msg 0x%08x\n", action_type);
			{
				icm_base_info_t *pext_msg = (icm_base_info_t *)&pmsg_dpu[1];
				icm_base_info_t resp_msg = {0,};
				id = pext_msg->process_id;

				codec_info *codec_info_ptr = &hifi_config->codec_info_client[id];

				ret_val = hifiCdc_open(codec_info_ptr);
				if (ret_val != XA_SUCCESS)
				{
					LOG("ICM_OPEN error\n");
				}
				resp_msg.ret = ret_val;

				send_action_complete(hifi_config, action_type, sizeof(icm_base_info_t), (u8 *)&resp_msg);
			}
			break;

		case ICM_EMPTY_THIS_BUFFER:
			{
				LOG1("Processing ICM msg 0x%08x\n", action_type);
				icm_cdc_iobuf_t *pext_msg = (icm_cdc_iobuf_t *)&pmsg_dpu[1];
				icm_cdc_iobuf_t *psysram_iobuf;
				id = pext_msg->base_info.process_id;

				codec_info *codec_info_ptr = &hifi_config->codec_info_client[id];
				psysram_iobuf = &(codec_info_ptr->sysram_io);

				ret_val = hifiCdc_process_emptybuf(pext_msg, codec_info_ptr);

				psysram_iobuf->base_info.ret = ret_val;

				send_action_complete(hifi_config, ICM_EMPTY_THIS_BUFFER, sizeof(icm_cdc_iobuf_t), (u8 *)psysram_iobuf);
			}
			break;

		case ICM_GET_PCM_PROP:
			{
				LOG1("Processing ICM msg 0x%08x\n", action_type);
				icm_pcm_prop_t *pext_msg = (icm_pcm_prop_t *)&pmsg_dpu[1];
				icm_pcm_prop_t resp_msg;
				id = pext_msg->base_info.process_id;

				codec_info *codec_info_ptr = &hifi_config->codec_info_client[id];

				memset(&resp_msg, 0, sizeof(icm_pcm_prop_t));

				ret_val = hifiCdc_getprop(&resp_msg, codec_info_ptr);
				if (ret_val != XT_NO_ERROR)
				{
					LOG("ICM_GET_PCM_PROP error\n");
				}
				resp_msg.base_info.ret = ret_val;
				send_action_complete(hifi_config, action_type, sizeof(icm_pcm_prop_t), (u8 *)&resp_msg);
			}
			break;

		case ICM_SET_PARA_CONFIG:
			{
				LOG1("Processing ICM msg 0x%08x\n", action_type);
				icm_prop_config *pext_msg = (icm_prop_config *)&pmsg_dpu[1];
				icm_prop_config resp_msg;
				id = pext_msg->base_info.process_id;

				codec_info *codec_info_ptr = &hifi_config->codec_info_client[id];

				memset(&resp_msg, 0, sizeof(icm_prop_config));

				ret_val = xa_hifi_cdc_set_param_config(pext_msg, codec_info_ptr);	// passing the size of the extended message
				if(ret_val != XT_NO_ERROR)
				{
					LOG("ICM_SET_PARA_CONFIG error\n");
				}

				resp_msg.base_info.ret = ret_val;
				send_action_complete(hifi_config, action_type, sizeof(icm_prop_config), (u8 *)&resp_msg);
			}
			break;

		case ICM_CLOSE:
			LOG1("Processing ICM msg 0x%08x\n", action_type);
			{
				icm_base_info_t *pext_msg = (icm_base_info_t *)&pmsg_dpu[1];
				icm_base_info_t resp_msg;
				id = pext_msg->process_id;

				codec_info *codec_info_ptr = &hifi_config->codec_info_client[id];

				ret_val = hifiCdc_close(codec_info_ptr);
				memset((void *)codec_info_ptr, 0, sizeof(codec_info));

				resp_msg.ret = ret_val;

				send_action_complete(hifi_config, action_type, sizeof(icm_base_info_t), (u8 *)&resp_msg);
				LOG("CODEC Close complete\n");
			}
			break;

		case ICM_RESET:
			{
				LOG1("Processing ICM msg 0x%08x\n", action_type);
				icm_base_info_t *pext_msg = (icm_base_info_t *)&pmsg_dpu[1];
				icm_base_info_t resp_msg = {0,0};
				id = pext_msg->process_id;

				codec_info *codec_info_ptr = &hifi_config->codec_info_client[id];

				ret_val = hifiCdc_reset(codec_info_ptr);

				resp_msg.ret = ret_val;
				send_action_complete(hifi_config, action_type, sizeof(icm_base_info_t), (u8 *)&resp_msg);
			}
			break;

		case ICM_CORE_EXIT:
			LOG1("Exiting per ICM msg 0x%08x\n", action_type);
			break;

		case ICM_PAUSE:
			break;
		case ICM_SUSPEND:
			/* store the hifi_config and DRAM0 and DRAM1*/
			LOG("ICM_SUSPEND\n");

			memcpy((char *)(hifi_config->dpu_ext_msg.hifi_config_phys),
				hifi_config,
				sizeof(hifi_main_struct));

			send_action_complete(hifi_config, action_type, 0, NULL);
			break;
		case ICM_RESUME:
			/* restore the hifi_config and DRAM0 and DRAM1*/
			LOG("ICM_RESUME\n");

			memcpy(hifi_config,
				(char *)(hifi_config->dpu_ext_msg.hifi_config_phys),
				sizeof(hifi_main_struct));

			send_action_complete(hifi_config, action_type, 0, NULL);
			break;
		default:
			break;
	}
	return;
}
