/*****************************************************************
 * Copyright 2018-2020 NXP
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

#include <string.h>
#include <xtensa/hal.h>
#include <xtensa/xtruntime.h>
#include <xtensa/tie/xt_interrupt.h>
#include <xtensa/config/core.h>
#include "xf-shmem.h"
#include "xf-core.h"
#include "xf-hal.h"

/*
 * Every 512M in 4GB space has dedicate cache attribute.
 * 1: write through
 * 2: cache bypass
 * 4: write back
 * F: invalid access
 */
#define I_CACHE_ATTRIBUTE  0x22242224     //write back mode
#define D_CACHE_ATTRIBUTE  0x22212221     //write through mode
#define INT_NUM_MU	7

/* ...define a global pointer, used in xf-msg.c */
struct dsp_main_struct *dsp_global_data;

/* ...wake on when interrupt happens */
void wake_on_ints(struct dsp_main_struct *dsp_config)
{
	_xtos_ints_on((1 << INT_NUM_MU) | (1 << INT_NUM_IRQSTR_DSP_6));

	_xtos_set_intlevel(15);
	if (!dsp_config->is_interrupt)
		XT_WAITI(0);
	_xtos_set_intlevel(0);

	_xtos_ints_off(1 << INT_NUM_MU);

	dsp_config->is_interrupt = 0;
}

int main(void)
{
	int ret;
	struct dsp_main_struct dsp_config;

	/* set the cache attribute */
	xthal_set_icacheattr(I_CACHE_ATTRIBUTE);
	xthal_set_dcacheattr(D_CACHE_ATTRIBUTE);

	memset(&dsp_config, 0, sizeof(dsp_config));
	dsp_global_data = &dsp_config;

	/* set the interrupt */
	xthal_set_intclear(-1);
	_xtos_set_interrupt_handler_arg(INT_NUM_MU,
					interrupt_handler_icm,
					&dsp_config);

#ifdef DEBUG
	enable_log();
#endif

	/* ...initialize MU */
	xf_mu_init(&dsp_config);

	LOG("DSP Start.....\n");

	for ( ; ; ) {
		/* ...dsp enter low power mode when no messages */
		wake_on_ints(&dsp_config);

		/* ...service core event */
		xf_core_service(&dsp_config);
	}

	LOG("DSP main process exit\n");
	return 0;
}
