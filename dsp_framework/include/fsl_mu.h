/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2021 NXP
 */
#ifndef _FSL_MU_H_
#define _FSL_MU_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "xf-types.h"

#define BIT(x) (1 << x)

enum mu_type {
	MU_V1,
	MU_V2,
};

enum mu_ch_type {
	MU_TX,         /* Tx */
	MU_RX,         /* Rx */
	MU_TXDB,       /* Tx doorbell */
	MU_RXDB,       /* Rx doorbell */
};

enum mu_xcr {
	MU_GIER,
	MU_GCR,
	MU_TCR,
	MU_RCR,
};

enum mu_xsr {
	MU_SR,
	MU_GSR,
	MU_TSR,
	MU_RSR,
};

struct mu_cfg {
	enum mu_type type;
	u32     xTR;            /* Tx Reg0 */
	u32     xRR;            /* Rx Reg0 */
	u32     xSR[4];         /* Status Reg */
	u32     xCR[4];         /* Control Reg*/
};

#define MU_xSR_GIPn(type, x) (type & MU_V2 ? BIT(x) : BIT(28 + (3 - (x))))
#define MU_xSR_RFn(type, x)  (type & MU_V2 ? BIT(x) : BIT(24 + (3 - (x))))
#define MU_xSR_TEn(type, x)  (type & MU_V2 ? BIT(x) : BIT(20 + (3 - (x))))

/* General Purpose Interrupt Enable */
#define MU_xCR_GIEn(type, x) (type & MU_V2 ? BIT(x) : BIT(28 + (3 - (x))))
/* Receive Interrupt Enable */
#define MU_xCR_RIEn(type, x) (type & MU_V2 ? BIT(x) : BIT(24 + (3 - (x))))
/* Transmit Interrupt Enable */
#define MU_xCR_TIEn(type, x) (type & MU_V2 ? BIT(x) : BIT(20 + (3 - (x))))
/* General Purpose Interrupt Request */
#define MU_xCR_GIRn(type, x) (type & MU_V2 ? BIT(x) : BIT(16 + (3 - (x))))

void MU_Init(void *base);
void MU_Deinit(void *base);
int MU_txdb(void *base, struct mu_cfg *cfg, uint32_t ch_idx);
void MU_SendMsg(void *base, struct mu_cfg *cfg, uint32_t ch_idx, uint32_t msg);
uint32_t MU_ReceiveMsgNonBlocking(void *base, struct mu_cfg *cfg, uint32_t ch_idx);
void MU_EnableInterrupts(void *base, struct mu_cfg *cfg, uint32_t ch_idx);
void MU_DisableInterrupts(void *base, struct mu_cfg *cfg, uint32_t ch_idx);

#endif /* _FSL_MU_H_*/
