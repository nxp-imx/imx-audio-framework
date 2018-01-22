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
 * icm_dpu.h
 *
 */

#ifndef _ICM_DPU_H_
#define _ICM_DPU_H_

#include "mytypes.h"

#define INT_NUMBER_ICM_INT(x)	(x)

/* Note: msg body will be available at pre-determined location. Receiver should
	read from that location.
*/

void icm_init(hifi_main_struct *hifi_config);

void interrupt_handler_icm(void *arg);

u32 icm_intr_send(u32 msg);

void icm_process(hifi_main_struct *hifi_config, u32 msg, u32 *pmsg_dpu);

void send_action_complete(hifi_main_struct *hifi_config, u32 msg_type, u32 ext_msg_siz, u8 *ext_msg);

void wake_on_ints(int level);

#endif /* _ICM_DPU_H_ */
