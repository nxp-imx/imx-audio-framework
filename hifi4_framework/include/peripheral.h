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

#ifndef PERIPHERAL_HEADER_H
#define PERIPHERAL_HEADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "mytypes.h"

#define MU_CR_NMI_MASK                           0x8u
#define MU_CR_GIRn_MASK                          0xF0000u
#define MU_CR_GIRn_NMI_MASK			(MU_CR_GIRn_MASK | MU_CR_NMI_MASK)

#define MU_SR_RF0_MASK				(1U<<27U)
#define MU_SR_TE0_MASK				(1U<<23U)
#define MU_CR_RIE0_MASK				(1U<<27U)

typedef struct {
	volatile u32		MU_TR[4];
	volatile const  u32	MU_RR[4];
	volatile u32		MU_SR; 
	volatile u32		MU_CR;
} mu_regs;


void mu_enableinterrupt_rx(mu_regs *regs, u32 idx);
void mu_msg_receive(mu_regs *regs, u32 regidx, u32 *msg);
void mu_msg_send(mu_regs *regs, u32 regidx, u32 msg);

#define LPUART_STAT_TDRE		(1 << 23)
#define LPUART_FIFO_TXFE		0x80
#define LPUART_FIFO_RXFE		0x40

#define LPUART_BAUD_SBR_MASK                     (0x1FFFU)
#define LPUART_BAUD_SBR_SHIFT                    (0U)
#define LPUART_BAUD_SBR(x)                       (((u32)(((u32)(x)) << LPUART_BAUD_SBR_SHIFT)) & LPUART_BAUD_SBR_MASK)
#define LPUART_BAUD_SBNS_MASK                    (0x2000U)
#define LPUART_BAUD_SBNS_SHIFT                   (13U)
#define LPUART_BAUD_SBNS(x)                      (((u32)(((u32)(x)) << LPUART_BAUD_SBNS_SHIFT)) & LPUART_BAUD_SBNS_MASK)
#define LPUART_BAUD_BOTHEDGE_MASK                (0x20000U)
#define LPUART_BAUD_BOTHEDGE_SHIFT               (17U)
#define LPUART_BAUD_BOTHEDGE(x)                  (((u32)(((u32)(x)) << LPUART_BAUD_BOTHEDGE_SHIFT)) & LPUART_BAUD_BOTHEDGE_MASK)
#define LPUART_BAUD_OSR_MASK                     (0x1F000000U)
#define LPUART_BAUD_OSR_SHIFT                    (24U)
#define LPUART_BAUD_OSR(x)                       (((u32)(((u32)(x)) << LPUART_BAUD_OSR_SHIFT)) & LPUART_BAUD_OSR_MASK)
#define LPUART_BAUD_M10_MASK                     (0x20000000U)
#define LPUART_BAUD_M10_SHIFT                    (29U)
#define LPUART_BAUD_M10(x)                       (((u32)(((u32)(x)) << LPUART_BAUD_M10_SHIFT)) & LPUART_BAUD_M10_MASK)


#define LPUART_CTRL_TE				(1 << 19)
#define LPUART_CTRL_RE				(1 << 18)
#define LPUART_CTRL_PT_MASK			(0x1U)
#define LPUART_CTRL_PE_MASK			(0x2U)
#define LPUART_CTRL_M_MASK			(0x10U)


struct nxp_lpuart {
        volatile u32 verid;
        volatile u32 param;
        volatile u32 global;
        volatile u32 pincfg;
        volatile u32 baud;
        volatile u32 stat;
        volatile u32 ctrl;
        volatile u32 data;
        volatile u32 match;
        volatile u32 modir;
        volatile u32 fifo;
        volatile u32 water;
};

#define UART_CLK_ROOT (80000000)
#define BAUDRATE (115200)
#define LPUART_BASE  (0x5a090000)


void hifi_putc(const char c);
void hifi_puts(const char *s);

#ifdef DEBUG
int enable_log();
#endif


#endif
