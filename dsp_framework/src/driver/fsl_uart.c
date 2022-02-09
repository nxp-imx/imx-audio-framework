// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2021 NXP

#include "fsl_uart.h"
#include "board.h"

void uart_putc(const char c)
{
	volatile char *uart_base = (volatile char *)UART_BASE;
	unsigned int status;
	unsigned int count = 0;

	if (c == '\n')
		uart_putc('\r');

	/* drain */
	do {
		status = read32(uart_base + USR1);
		count++;
	} while (count <= 600);

	write32(uart_base + URTX0, c);
}

int uart_init(void)
{
	volatile char *uart_base = (volatile char *)UART_BASE;

	/* enable UART */
	write32(uart_base + UCR1, 0x0001);

	write32(uart_base + UCR2, 0x5027);

	/* Set UCR3[RXDMUXSEL] = 1. */
	write32(uart_base + UCR3, 0x0084);

	write32(uart_base + UCR4, 0x4002);

	/* Set internal clock divider = 1 */
	write32(uart_base + UFCR, 0x0A81);

	write32(uart_base + UBIR, 0x3E7);
	write32(uart_base + UBMR, 0x32DC);

	write32(uart_base + UCR1, 0x2201);
	write32(uart_base + UMCR, 0x0000);

	return 0;
}

