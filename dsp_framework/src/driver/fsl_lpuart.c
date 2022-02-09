// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2021 NXP

#include "fsl_lpuart.h"
#include "board.h"

int lpuart_init(void)
{
	volatile int *lpuart1 = (volatile int *)LPUART_BASE;
	struct nxp_lpuart *base = (struct nxp_lpuart *)lpuart1;

	u32 tmp;
	u32 sbr, osr, baud_diff, tmp_osr, tmp_sbr, tmp_diff;
	u32 clk = UART_CLK_ROOT;
	u32 baudrate = BAUDRATE;

	tmp = base->ctrl;
	tmp &= ~(LPUART_CTRL_TE | LPUART_CTRL_RE);
	base->ctrl = tmp;
	base->modir = 0;
	base->fifo = ~(LPUART_FIFO_TXFE | LPUART_FIFO_RXFE);
	base->match = 0;

	baud_diff = BAUDRATE;
	osr = 0;
	sbr = 0;
	for (tmp_osr = 4; tmp_osr <= 32; tmp_osr++) {
		tmp_sbr = (clk / (baudrate * tmp_osr));
		if (tmp_sbr == 0)
			tmp_sbr = 1;

		tmp_diff = (clk / (tmp_osr * tmp_sbr));
		tmp_diff = tmp_diff - baudrate;

		if (tmp_diff > (baudrate - (clk / (tmp_osr * (tmp_sbr + 1))))) {
			tmp_diff = baudrate - (clk / (tmp_osr * (tmp_sbr + 1)));
			tmp_sbr++;
		}

		if (tmp_diff <= baud_diff) {
			baud_diff = tmp_diff;
			osr = tmp_osr;
			sbr = tmp_sbr;
		}
	}

	tmp = base->baud;

	if ((osr > 3) && (osr < 8))
		tmp |= LPUART_BAUD_BOTHEDGE_MASK;

	tmp &= ~LPUART_BAUD_OSR_MASK;
	tmp |= LPUART_BAUD_OSR(osr - 1);

	tmp &= ~LPUART_BAUD_SBR_MASK;
	tmp |= LPUART_BAUD_SBR(sbr);
	tmp &= ~(LPUART_BAUD_M10_MASK | LPUART_BAUD_SBNS_MASK);

	base->baud = tmp;

	tmp = base->ctrl;
	tmp &= ~(LPUART_CTRL_PE_MASK |
			LPUART_CTRL_PT_MASK  |
			LPUART_CTRL_M_MASK);
	base->ctrl = tmp;

	base->ctrl = LPUART_CTRL_RE | LPUART_CTRL_TE;

	return 0;
}

void lpuart_putc(const char c)
{
	volatile int *lpuart1 = (volatile int *)LPUART_BASE;
	struct nxp_lpuart *base = (struct nxp_lpuart *)lpuart1;

	if (c == '\n')
		lpuart_putc('\r');

	while (!(base->stat & LPUART_STAT_TDRE))
		;

	base->data = c;
}
