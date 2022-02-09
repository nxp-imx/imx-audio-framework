// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2021 NXP

#include "mydefs.h"
#include "fsl_mu.h"

#ifdef PLATF_8M
struct mem_att dsp_mem_att[] = {
	/* DDR (Data) */
	{ 0x40000000, 0x40000000, 0x80000000, 0},
};

struct mem_cfg dsp_mem_cfg = {
	.att = dsp_mem_att,
	.att_size = 1,
};

struct mu_cfg dsp_mu_cfg = {
	.type   = MU_V1,
	.xTR    = 0x0,
	.xRR    = 0x10,
	.xSR    = {0x20, 0x20, 0x20, 0x20},
	.xCR    = {0x24, 0x24, 0x24, 0x24},
};

#else /* PLATF_8M */
#ifdef PLATF_8ULP
struct mem_att dsp_mem_att[] = {
	/* DDR (Data) */
	{ 0x0c000000, 0x80000000, 0x10000000, 0},
	{ 0x30000000, 0x90000000, 0x10000000, 0},
};

struct mem_cfg dsp_mem_cfg = {
	.att = dsp_mem_att,
	.att_size = 2,
};

struct mu_cfg dsp_mu_cfg = {
	.type   = MU_V2,
	.xTR    = 0x200,
	.xRR    = 0x280,
	.xSR    = {0xC, 0x118, 0x124, 0x12C},
	.xCR    = {0x110, 0x114, 0x120, 0x128},
};

#else /* !PLATF_8ULP && ! PLATF_8M -> (8QXP || 8QM)*/
struct mem_att dsp_mem_att[] = {
	/* DDR (Data) */
	{ 0x80000000, 0x80000000, 0x60000000, 0},
};

struct mem_cfg dsp_mem_cfg = {
	.att = dsp_mem_att,
	.att_size = 1,
};

struct mu_cfg dsp_mu_cfg = {
	.type   = MU_V1,
	.xTR    = 0x0,
	.xRR    = 0x10,
	.xSR    = {0x20, 0x20, 0x20, 0x20},
	.xCR    = {0x24, 0x24, 0x24, 0x24},
};

#endif /* PLATF_8ULP */
#endif /* PLATF_8M */
