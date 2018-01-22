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
 * intr_utils.c
 */

#include <stdio.h>
#include <xtensa/config/core.h>
#include <xtensa/xtruntime.h>
#include <xtensa/tie/xt_interrupt.h>
#include "mydefs.h"

#define TIMER0_INT_MASK (1 << XCHAL_TIMER0_INTERRUPT)
#define TIMER0_INTERVAL 0x00000800

#if XCHAL_NUM_TIMERS < 1
#error This OS requires at least one internal timer.
#endif


void wake_on_ints(int level)
{
	XT_WAITI(0);
	return;
}

void interrupt_handler_timer0 (void) {
	/* doesn't need to do anything in particular,
	 but turn off the timer interrupt anyhow    */
	_xtos_ints_off(TIMER0_INT_MASK);
	return;
}

void sleep(unsigned int ui_cycles)
{
	LOG1("waiting %d cycles\n", ui_cycles);
	xthal_set_ccompare(0, xthal_get_ccount() + ui_cycles);

	// See Sys Software Ref Manual table 3.7 for more info.
	_xtos_ints_on(TIMER0_INT_MASK);
	do
	{
		XT_WAITI(0);  // 0 so there is no masking
	} while (!(xthal_get_interrupt() & TIMER0_INT_MASK));
	// to make sure wakeup was due to this timer interrupt

	return;
}
