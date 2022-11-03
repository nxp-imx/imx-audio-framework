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

#ifndef __MY_DEFS_H
#define __MY_DEFS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xf-dp.h"
#include "memory.h"
#include "rpmsg_lite.h"
#include "rpmsg_ns.h"
#include "fsl_mu.h"

struct icm_dpu_ext_msg {
	u32     scratch_buf_phys;
	u32     scratch_buf_size;
	u32     dsp_config_phys;
	u32     dsp_config_size;
	u32	dsp_board_type;
};

struct rpmsg_ept_handle {
	u32 localAddr;                 /*!< RPMsg local endpoint address */
	u32 peerAddr;                  /*!< RPMsg peer endpoint address */
	void *priv;                    /* Pointer for dsp_main_struct */
};

/* Max supported ISR counts */
#define ISR_COUNT (12U) /* Change for multiple remote cores */
/*!
 * Structure to keep track of registered ISR's.
 */
struct isr_info {
	void *data;
};

struct mem_att {
	u32 da; /* device address (From Cortex M4 view)*/
	u32 sa; /* system bus address */
	u32 size; /* size of reg range */
	int flags;
};

struct mem_cfg {
	struct mem_att *att;
	int    att_size;
};

/*
 * Only support one instance
 */
#define EPT_NUM        0x2
struct dsp_main_struct {
	xf_dsp_t 				xf_dsp;
	ipc_msgq_t				g_ipc_msgq;
	xf_lock_t				g_msgq_lock;

	u32 					is_core_init;

	struct icm_dpu_ext_msg			dpu_ext_msg;

	struct rpmsg_lite_instance		*rpmsg;
	struct rpmsg_lite_instance		rpmsg_ctxt;

	struct rpmsg_lite_endpoint		*ept[EPT_NUM];
	struct rpmsg_lite_ept_static_context	ept_ctx[EPT_NUM];
	struct rpmsg_ept_handle			ept_handle[EPT_NUM];
	struct isr_info				isr_table[ISR_COUNT];
	struct mem_cfg				*mem_cfg;
	struct mu_cfg				*mu_cfg;
	void					*platform_lock;

	void					*dma_device;

	void					*micfil;
};

struct dsp_main_struct* get_main_struct();

#endif
