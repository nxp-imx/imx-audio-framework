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

#include <string.h>
#include <xtensa/hal.h>
#include <xtensa/xtruntime.h>
#include "mytypes.h"
#include "mydefs.h"
#include "icm_dpu.h"

#define CACHE_ATTRIBUTE  0x2224224F
#define INT_NUM_MU 	 7
#define MAX_BYTES_EXTMSG	64

int main()
{
	int ret;
	icm_header_t icm_msg;
	u8 picm_msg[4 + MAX_BYTES_EXTMSG];

	hifi_main_struct hifi_config;

	/* set the cache attribute */
	xthal_set_icacheattr(CACHE_ATTRIBUTE);
	xthal_set_dcacheattr(CACHE_ATTRIBUTE);

	memset(&hifi_config, 0, sizeof(hifi_config));

	/* set the interrupt */
	xthal_set_intclear(-1);
	_xtos_set_interrupt_handler_arg(INT_NUM_MU, interrupt_handler_icm, &hifi_config);
	_xtos_ints_on(1 << INT_NUM_MU);

#ifdef DEBUG
	enable_log();
#endif
	icm_init(&hifi_config);

	LOG("HIFI Start.....\n");

	while(1) {
		/***********************************
		* Get the msg from queue;
		*
		***********************************/
		icm_msg.allbits  = get_icm_from_que(&hifi_config, picm_msg);
		if (icm_msg.allbits == 0) {
			wake_on_ints(0);
		} else if (icm_msg.msg == ICM_CORE_EXIT) {

			LOG("Get ICM_CORE_EXIT command\n");
			icm_msg.allbits = 0;
			icm_msg.intr = 1;
			icm_msg.msg  = ICM_CORE_EXIT;
			icm_intr_send(icm_msg.allbits);
			break;
		}
		else {
			icm_process(&hifi_config, icm_msg.allbits,  (u32 *)picm_msg);

		}
	}

	LOG("HIFI main process exit\n");
	return 0;
}
