// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2021 NXP

#include "board.h"
#include "fsl_mu.h"

void MU_Init(void *base)
{
}

void MU_Deinit(void *base)
{
}

int MU_txdb(void *base, struct mu_cfg *cfg, uint32_t ch_idx)
{
	u32 val;

	val = read32(base + cfg->xCR[MU_GCR]);
	val |= MU_xCR_GIRn(cfg->type, ch_idx);
	write32(base + cfg->xCR[MU_GCR], val);

	return 0;
}

void MU_SendMsg(void *base, struct mu_cfg *cfg, uint32_t ch_idx, uint32_t msg)
{
	write32(base + cfg->xTR + ch_idx * 4, msg);
}

uint32_t MU_ReceiveMsgNonBlocking(void *base, struct mu_cfg *cfg, uint32_t ch_idx)
{
	return read32(base + cfg->xRR + ch_idx * 4);
}

void MU_EnableRxInterrupts(void *base, struct mu_cfg *cfg, uint32_t ch_idx)
{
	u32 val;

	val = read32(base + cfg->xCR[MU_RCR]);
	val |= MU_xCR_RIEn(cfg->type, ch_idx);
	write32(base + cfg->xCR[MU_RCR], val);
}

void MU_DisableRxInterrupts(void *base, struct mu_cfg *cfg, uint32_t ch_idx)
{
	u32 val;

	val = read32(base + cfg->xCR[MU_RCR]);
	val &= ~MU_xCR_RIEn(cfg->type, ch_idx);
	write32(base + cfg->xCR[MU_RCR], val);
}

uint32_t MU_GetRxStatusFlags(void *base, struct mu_cfg *cfg, uint32_t ch_idx)
{
	return read32(base + cfg->xSR[MU_RSR]) & MU_xSR_RFn(cfg->type, ch_idx);
}
