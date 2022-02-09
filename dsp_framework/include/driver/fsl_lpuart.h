/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2021 NXP
 */
#ifndef _FSL_LPUART_H_
#define _FSL_LPUART_H_

#include "types.h"

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

#define BAUDRATE (115200)

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

#endif /* _FSL_LPUART_H_ */
