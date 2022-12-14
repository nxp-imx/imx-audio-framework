// Copyright 2022 NXP
// SPDX-License-Identifier: BSD-3-Clause

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "io.h"
#include "board.h"
#include "micfil.h"
#include "debug.h"

#define MICFIL_REG_NUM			(0xA8/4 + 1)
/* MICFIL Register Map */
#define REG_MICFIL_CTRL1		0x00
#define REG_MICFIL_CTRL2		0x04
#define REG_MICFIL_STAT			0x08
#define REG_MICFIL_FIFO_CTRL		0x10
#define REG_MICFIL_FIFO_STAT		0x14
#define REG_MICFIL_DATACH0		0x24
#define REG_MICFIL_DATACH1		0x28
#define REG_MICFIL_DATACH2		0x2C
#define REG_MICFIL_DATACH3		0x30
#define REG_MICFIL_DATACH4		0x34
#define REG_MICFIL_DATACH5		0x38
#define REG_MICFIL_DATACH6		0x3C
#define REG_MICFIL_DATACH7		0x40
#define REG_MICFIL_DC_CTRL		0x64
#define REG_MICFIL_OUT_CTRL		0x74
#define REG_MICFIL_OUT_STAT		0x7C
#define REG_MICFIL_VAD0_CTRL1		0x90
#define REG_MICFIL_VAD0_CTRL2		0x94
#define REG_MICFIL_VAD0_STAT		0x98
#define REG_MICFIL_VAD0_SCONFIG		0x9C
#define REG_MICFIL_VAD0_NCONFIG		0xA0
#define REG_MICFIL_VAD0_NDATA		0xA4
#define REG_MICFIL_VAD0_ZCD		0xA8

/* MICFIL Control Register 1 -- REG_MICFILL_CTRL1 0x00 */
#define MICFIL_CTRL1_MDIS_SHIFT		31
#define MICFIL_CTRL1_MDIS_MASK		BIT(MICFIL_CTRL1_MDIS_SHIFT)
#define MICFIL_CTRL1_MDIS		BIT(MICFIL_CTRL1_MDIS_SHIFT)
#define MICFIL_CTRL1_DOZEN_SHIFT	30
#define MICFIL_CTRL1_DOZEN_MASK		BIT(MICFIL_CTRL1_DOZEN_SHIFT)
#define MICFIL_CTRL1_DOZEN		BIT(MICFIL_CTRL1_DOZEN_SHIFT)
#define MICFIL_CTRL1_PDMIEN_SHIFT	29
#define MICFIL_CTRL1_PDMIEN_MASK	BIT(MICFIL_CTRL1_PDMIEN_SHIFT)
#define MICFIL_CTRL1_PDMIEN		BIT(MICFIL_CTRL1_PDMIEN_SHIFT)
#define MICFIL_CTRL1_DBG_SHIFT		28
#define MICFIL_CTRL1_DBG_MASK		BIT(MICFIL_CTRL1_DBG_SHIFT)
#define MICFIL_CTRL1_DBG		BIT(MICFIL_CTRL1_DBG_SHIFT)
#define MICFIL_CTRL1_SRES_SHIFT		27
#define MICFIL_CTRL1_SRES_MASK		BIT(MICFIL_CTRL1_SRES_SHIFT)
#define MICFIL_CTRL1_SRES		BIT(MICFIL_CTRL1_SRES_SHIFT)
#define MICFIL_CTRL1_DBGE_SHIFT		26
#define MICFIL_CTRL1_DBGE_MASK		BIT(MICFIL_CTRL1_DBGE_SHIFT)
#define MICFIL_CTRL1_DBGE		BIT(MICFIL_CTRL1_DBGE_SHIFT)
#define MICFIL_CTRL1_DISEL_SHIFT	24
#define MICFIL_CTRL1_DISEL_WIDTH	2
#define MICFIL_CTRL1_DISEL_MASK		((BIT(MICFIL_CTRL1_DISEL_WIDTH) - 1) \
					 << MICFIL_CTRL1_DISEL_SHIFT)
#define MICFIL_CTRL1_DISEL(v)		(((v) << MICFIL_CTRL1_DISEL_SHIFT) \
					 & MICFIL_CTRL1_DISEL_MASK)
#define MICFIL_CTRL1_ERREN_SHIFT	23
#define MICFIL_CTRL1_ERREN_MASK		BIT(MICFIL_CTRL1_ERREN_SHIFT)
#define MICFIL_CTRL1_ERREN		BIT(MICFIL_CTRL1_ERREN_SHIFT)
#define MICFIL_CTRL1_CHEN_SHIFT		0
#define MICFIL_CTRL1_CHEN_WIDTH		8
#define MICFIL_CTRL1_CHEN_MASK(x)	(BIT(x) << MICFIL_CTRL1_CHEN_SHIFT)
#define MICFIL_CTRL1_CHEN(x)		(MICFIL_CTRL1_CHEN_MASK(x))

/* MICFIL Control Register 2 -- REG_MICFILL_CTRL2 0x04 */
#define MICFIL_CTRL2_QSEL_SHIFT		25
#define MICFIL_CTRL2_QSEL_WIDTH		3
#define MICFIL_CTRL2_QSEL_MASK		((BIT(MICFIL_CTRL2_QSEL_WIDTH) - 1) \
					 << MICFIL_CTRL2_QSEL_SHIFT)
#define MICFIL_HIGH_QUALITY		BIT(MICFIL_CTRL2_QSEL_SHIFT)
#define MICFIL_MEDIUM_QUALITY		(0 << MICFIL_CTRL2_QSEL_SHIFT)
#define MICFIL_LOW_QUALITY		(7 << MICFIL_CTRL2_QSEL_SHIFT)
#define MICFIL_VLOW0_QUALITY		(6 << MICFIL_CTRL2_QSEL_SHIFT)
#define MICFIL_VLOW1_QUALITY		(5 << MICFIL_CTRL2_QSEL_SHIFT)
#define MICFIL_VLOW2_QUALITY		(4 << MICFIL_CTRL2_QSEL_SHIFT)

#define MICFIL_CTRL2_CICOSR_SHIFT	16
#define MICFIL_CTRL2_CICOSR_WIDTH	4
#define MICFIL_CTRL2_CICOSR_MASK	((BIT(MICFIL_CTRL2_CICOSR_WIDTH) - 1) \
					 << MICFIL_CTRL2_CICOSR_SHIFT)
#define MICFIL_CTRL2_CICOSR(v)		(((v) << MICFIL_CTRL2_CICOSR_SHIFT) \
					 & MICFIL_CTRL2_CICOSR_MASK)
#define MICFIL_CTRL2_CLKDIV_SHIFT	0
#define MICFIL_CTRL2_CLKDIV_WIDTH	8
#define MICFIL_CTRL2_CLKDIV_MASK	((BIT(MICFIL_CTRL2_CLKDIV_WIDTH) - 1) \
					 << MICFIL_CTRL2_CLKDIV_SHIFT)
#define MICFIL_CTRL2_CLKDIV(v)		(((v) << MICFIL_CTRL2_CLKDIV_SHIFT) \
					 & MICFIL_CTRL2_CLKDIV_MASK)

/* MICFIL Status Register -- REG_MICFIL_STAT 0x08 */
#define MICFIL_STAT_BSY_FIL_SHIFT	31
#define MICFIL_STAT_BSY_FIL_MASK	BIT(MICFIL_STAT_BSY_FIL_SHIFT)
#define MICFIL_STAT_BSY_FIL		BIT(MICFIL_STAT_BSY_FIL_SHIFT)
#define MICFIL_STAT_FIR_RDY_SHIFT	30
#define MICFIL_STAT_FIR_RDY_MASK	BIT(MICFIL_STAT_FIR_RDY_SHIFT)
#define MICFIL_STAT_FIR_RDY		BIT(MICFIL_STAT_FIR_RDY_SHIFT)
#define MICFIL_STAT_LOWFREQF_SHIFT	29
#define MICFIL_STAT_LOWFREQF_MASK	BIT(MICFIL_STAT_LOWFREQF_SHIFT)
#define MICFIL_STAT_LOWFREQF		BIT(MICFIL_STAT_LOWFREQF_SHIFT)
#define MICFIL_STAT_CHXF_SHIFT(v)	(v)
#define MICFIL_STAT_CHXF_MASK(v)	BIT(MICFIL_STAT_CHXF_SHIFT(v))
#define MICFIL_STAT_CHXF(v)		BIT(MICFIL_STAT_CHXF_SHIFT(v))

/* MICFIL FIFO Control Register -- REG_MICFIL_FIFO_CTRL 0x10 */
#define MICFIL_FIFO_CTRL_FIFOWMK_SHIFT	0
#define MICFIL_FIFO_CTRL_FIFOWMK_WIDTH	3
#define MICFIL_FIFO_CTRL_FIFOWMK_MASK	((BIT(MICFIL_FIFO_CTRL_FIFOWMK_WIDTH) - 1) \
					 << MICFIL_FIFO_CTRL_FIFOWMK_SHIFT)
#define MICFIL_FIFO_CTRL_FIFOWMK(v)	(((v) << MICFIL_FIFO_CTRL_FIFOWMK_SHIFT) \
					 & MICFIL_FIFO_CTRL_FIFOWMK_MASK)

/* MICFIL FIFO Status Register -- REG_MICFIL_FIFO_STAT 0x14 */
#define MICFIL_FIFO_STAT_FIFOX_OVER_SHIFT(v)	(v)
#define MICFIL_FIFO_STAT_FIFOX_OVER_MASK(v)	BIT(MICFIL_FIFO_STAT_FIFOX_OVER_SHIFT(v))
#define MICFIL_FIFO_STAT_FIFOX_UNDER_SHIFT(v)	((v) + 8)
#define MICFIL_FIFO_STAT_FIFOX_UNDER_MASK(v)	BIT(MICFIL_FIFO_STAT_FIFOX_UNDER_SHIFT(v))

/* MICFIL DC Remover Control Register -- REG_MICFIL_DC_CTRL */
#define MICFIL_DC_CTRL_SHIFT		0
#define MICFIL_DC_CTRL_MASK		0xFFFF
#define MICFIL_DC_CTRL_WIDTH		2
#define MICFIL_DC_CHX_SHIFT(v)		(2 * (v))
#define MICFIL_DC_CHX_MASK(v)		((BIT(MICFIL_DC_CTRL_WIDTH) - 1) \
					 << MICFIL_DC_CHX_SHIFT(v))
#define MICFIL_DC_MODE(v1, v2)		(((v1) << MICFIL_DC_CHX_SHIFT(v2)) \
					 & MICFIL_DC_CHX_MASK(v2))
#define MICFIL_DC_CUTOFF_21HZ		0
#define MICFIL_DC_CUTOFF_83HZ		1
#define MICFIL_DC_CUTOFF_152Hz		2
#define MICFIL_DC_BYPASS			3

/* MICFIL Output Control Register */
#define MICFIL_OUTGAIN_CHX_SHIFT(v)	(4 * (v))

/* Constants */
#define MICFIL_OUTPUT_CHANNELS		8
#define MICFIL_CTRL2_OSR_DEFAULT	(0 << MICFIL_CTRL2_CICOSR_SHIFT)

#ifdef PLATF_8M
#define MICFIL_FIFO_DEPTH		32
#else
#define MICFIL_FIFO_DEPTH		8
#endif

#define CLK_8K_FREQ			24576000
#define CLK_11K_FREQ			22579200
#define MICFIL_CLK_ROOT			CLK_8K_FREQ

// Only support rates that santify (rate % 8k = 0) cause fixed clk
#define MICFIL_NUM_RATES		7
const int micfil_constraint_rates[MICFIL_NUM_RATES] = { 8000, 16000, 32000, 48000 };

#define CICOSR_DEFAULT			0
#define OSR_DEFAULT			16

struct fsl_micfil {
	void			*base_addr;
	int			dc_remover;
	int			Int;
	int			dma_event_id;
	int			sample_rate;
	int			channel;
	int			channel_gain[MICFIL_OUTPUT_CHANNELS];
	int			width;
	int			quality;
	unsigned int		cache[MICFIL_REG_NUM];
};

static inline int64_t clk_get_rate(struct fsl_micfil *micfil)
{
	return MICFIL_CLK_ROOT;
}

static int rate_support(int rate)
{
	int i;
	int ret = 0;

	for (i = 0; i < MICFIL_NUM_RATES; i++) {
		if (rate == micfil_constraint_rates[i]) {
			ret = 1;
			break;
		}
	}
	return ret;
}

/* returned pdm_clk * Kfactor */
static int get_pdm_clk(struct fsl_micfil *micfil, unsigned int rate)
{
	int qsel, osr;
	int pdm_clk = 0;

	unsigned int ctrl2_reg = read32(micfil->base_addr + REG_MICFIL_CTRL2);
	osr = 16 - ((ctrl2_reg & MICFIL_CTRL2_CICOSR_MASK)
		    >> MICFIL_CTRL2_CICOSR_SHIFT);
	qsel = ctrl2_reg & MICFIL_CTRL2_QSEL_MASK;

	if (!rate || !osr)
		return ERROR;

	switch (qsel) {
	case MICFIL_HIGH_QUALITY:
		pdm_clk = rate * 8 * osr / 2; /* kfactor = 0.5 */
		break;
	case MICFIL_MEDIUM_QUALITY:
	case MICFIL_VLOW0_QUALITY:
		pdm_clk = rate * 4 * osr * 1; /* kfactor = 1 */
		break;
	case MICFIL_LOW_QUALITY:
	case MICFIL_VLOW1_QUALITY:
		pdm_clk = rate * 2 * osr * 2; /* kfactor = 2 */
		break;
	case MICFIL_VLOW2_QUALITY:
		pdm_clk = rate * osr * 4; /* kfactor = 4 */
		break;
	default:
		LOG("can't get pdm clk cause wrong quality!\n");
		break;
	}

	return pdm_clk;
}

static int get_clk_div(struct fsl_micfil *micfil, unsigned int rate)
{
	unsigned int ctrl2_reg;
	int64_t mclk_rate;
	int clk_div;

	ctrl2_reg = read32(micfil->base_addr + REG_MICFIL_CTRL2);

	mclk_rate = clk_get_rate(micfil);

	clk_div = mclk_rate / (get_pdm_clk(micfil, rate) * 2);

	return clk_div;
}

static int set_quality(struct fsl_micfil *micfil)
{
	int ret = OK;
	int quality = micfil->quality;

	switch (quality) {
	case HIGH_QUALITY:
		write32_bit(micfil->base_addr + REG_MICFIL_CTRL2, MICFIL_CTRL2_QSEL_MASK, MICFIL_HIGH_QUALITY);
		break;
	case MEDIUM_QUALITY:
		write32_bit(micfil->base_addr + REG_MICFIL_CTRL2, MICFIL_CTRL2_QSEL_MASK, MICFIL_MEDIUM_QUALITY);
		break;
	case LOW_QUALITY:
		write32_bit(micfil->base_addr + REG_MICFIL_CTRL2, MICFIL_CTRL2_QSEL_MASK, MICFIL_LOW_QUALITY);
		break;
	case VLOW0_QUALITY:
		write32_bit(micfil->base_addr + REG_MICFIL_CTRL2, MICFIL_CTRL2_QSEL_MASK, MICFIL_VLOW0_QUALITY);
		break;
	case VLOW1_QUALITY:
		write32_bit(micfil->base_addr + REG_MICFIL_CTRL2, MICFIL_CTRL2_QSEL_MASK, MICFIL_VLOW1_QUALITY);
		break;
	case VLOW2_QUALITY:
		write32_bit(micfil->base_addr + REG_MICFIL_CTRL2, MICFIL_CTRL2_QSEL_MASK, MICFIL_VLOW2_QUALITY);
		break;
	default:
		LOG("Set quality error!\n");
		ret = ERROR;
		break;
	}

	return ret;
}

static int set_chan_gain(struct fsl_micfil *micfil, int ch_idx)
{
	int ret = OK;
	int i;

	unsigned int out_ctrl_reg = read32(micfil->base_addr + REG_MICFIL_OUT_CTRL);
	out_ctrl_reg |= (micfil->channel_gain[ch_idx]) << (ch_idx * 4);

	write32(micfil->base_addr + REG_MICFIL_OUT_CTRL, out_ctrl_reg);

	return ret;
}

static int set_dc_remover(struct fsl_micfil *micfil)
{
	/* TODO: update dc remover for all channels */
	return 0;
}

static int enable_channel(struct fsl_micfil *micfil, int channels)
{
	/* enable channels */
	write32_bit(micfil->base_addr + REG_MICFIL_CTRL1, 0xFF, ((1 << channels) - 1));

	return OK;
}

static int set_watermark(struct fsl_micfil *micfil)
{
	int ret = OK;
	return ret;
}

static int set_clock_params(struct fsl_micfil *micfil, int out_rate)
{
	int clk_div;

	/* use fixed mclk CLK_8K_FREQ */

	/* set CICOSR */
	write32_bit(micfil->base_addr + REG_MICFIL_CTRL2,
				 MICFIL_CTRL2_CICOSR_MASK,
				 MICFIL_CTRL2_OSR_DEFAULT);

	/* set CLK_DIV */
	clk_div = get_clk_div(micfil, out_rate);
	if (clk_div < 0)
		return -EINVAL;

	write32_bit(micfil->base_addr + REG_MICFIL_CTRL2,
				 MICFIL_CTRL2_CLKDIV_MASK, clk_div);

	return OK;
}

static int micfil_reset(struct fsl_micfil *micfil)
{
	write32_bit(micfil->base_addr + REG_MICFIL_CTRL1,
				 MICFIL_CTRL1_MDIS_MASK, 0);

	write32_bit(micfil->base_addr + REG_MICFIL_CTRL1,
				 MICFIL_CTRL1_SRES_MASK,
				 MICFIL_CTRL1_SRES);

	/* w1c */
	write32_bit(micfil->base_addr + REG_MICFIL_STAT, 0xFF, 0xFF);

	return OK;
}

static bool micfil_writeable_reg(struct fsl_micfil *micfil, unsigned int reg)
{
	switch (reg) {
	case REG_MICFIL_CTRL1:
	case REG_MICFIL_CTRL2:
	case REG_MICFIL_STAT:		/* Write 1 to Clear */
	case REG_MICFIL_FIFO_CTRL:
	case REG_MICFIL_FIFO_STAT:	/* Write 1 to Clear */
	case REG_MICFIL_DC_CTRL:
	case REG_MICFIL_OUT_CTRL:
	case REG_MICFIL_OUT_STAT:	/* Write 1 to Clear */
	case REG_MICFIL_VAD0_CTRL1:
	case REG_MICFIL_VAD0_CTRL2:
	case REG_MICFIL_VAD0_STAT:	/* Write 1 to Clear */
	case REG_MICFIL_VAD0_SCONFIG:
	case REG_MICFIL_VAD0_NCONFIG:
	case REG_MICFIL_VAD0_ZCD:
		return true;
	default:
		return false;
	}
}

static bool micfil_readable_reg(struct fsl_micfil *micfil, unsigned int reg)
{
	switch (reg) {
	case REG_MICFIL_CTRL1:
	case REG_MICFIL_CTRL2:
	case REG_MICFIL_STAT:
	case REG_MICFIL_FIFO_CTRL:
	case REG_MICFIL_FIFO_STAT:
	case REG_MICFIL_DATACH0:
	case REG_MICFIL_DATACH1:
	case REG_MICFIL_DATACH2:
	case REG_MICFIL_DATACH3:
	case REG_MICFIL_DATACH4:
	case REG_MICFIL_DATACH5:
	case REG_MICFIL_DATACH6:
	case REG_MICFIL_DATACH7:
	case REG_MICFIL_DC_CTRL:
	case REG_MICFIL_OUT_CTRL:
	case REG_MICFIL_OUT_STAT:
	case REG_MICFIL_VAD0_CTRL1:
	case REG_MICFIL_VAD0_CTRL2:
	case REG_MICFIL_VAD0_STAT:
	case REG_MICFIL_VAD0_SCONFIG:
	case REG_MICFIL_VAD0_NCONFIG:
	case REG_MICFIL_VAD0_NDATA:
	case REG_MICFIL_VAD0_ZCD:
		return true;
	default:
		return false;
	}
}

void micfil_probe(struct dsp_main_struct *dsp)
{
	int board_type = BOARD_TYPE;
	struct fsl_micfil *micfil = NULL;

	if (dsp->micfil)
		return;

	if (board_type == DSP_IMX8MP_TYPE) {
		xaf_malloc((void **)&micfil, sizeof(struct fsl_micfil), 0);
		if (!micfil)
			return;
		memset(micfil, 0, sizeof(struct fsl_micfil));

		micfil->base_addr = (void *)MICFIL_ADDR;
		micfil->Int = MICFIL_INT;
		micfil->dma_event_id = SDMA_MICFIL_EVENT;
	}

	dsp->micfil = (void *)micfil;
}

int micfil_get_dma_event_id(void *p_micfil)
{
	if (!p_micfil) {
		LOG("micfil is null\n");
		return ERROR;
	}
	struct fsl_micfil *micfil = (struct fsl_micfil*)p_micfil;
	return micfil->dma_event_id;
}

int micfil_get_irqnum(void *p_micfil)
{
	if (!p_micfil) {
		LOG("micfil is null\n");
		return ERROR;
	}
	struct fsl_micfil *micfil = (struct fsl_micfil*)p_micfil;
	return micfil->Int;
}

void *micfil_get_datach0_addr(void *p_micfil)
{
	if (!p_micfil) {
		LOG("micfil is null\n");
		return 0;
	}
	struct fsl_micfil *micfil = (struct fsl_micfil*)p_micfil;
	return (void *)(micfil->base_addr + REG_MICFIL_DATACH0);
}

int micfil_init(void *p_micfil)
{
	int ret = OK;
	int rate, ch, ch_idx;
	struct fsl_micfil *micfil;
	int i, val;

	if (!p_micfil) {
		LOG("micfil is null\n");
		return ERROR;
	}
	micfil = (struct fsl_micfil*)p_micfil;

	/* Disable the module */
	write32_bit(micfil->base_addr + REG_MICFIL_CTRL1, MICFIL_CTRL1_PDMIEN_MASK, 0);

	if (!micfil->quality)
		micfil->quality = VLOW0_QUALITY;
	ret = set_quality(micfil);

	/* set default gain to a */
	write32(micfil->base_addr + REG_MICFIL_OUT_CTRL, 0x22222222);
	for (i = 0; i < MICFIL_OUTPUT_CHANNELS; i++)
		micfil->channel_gain[i] = 0xA;

	/* set DC Remover in bypass mode*/
	val = 0;
	for (i = 0; i < MICFIL_OUTPUT_CHANNELS; i++)
		val |= MICFIL_DC_MODE(MICFIL_DC_BYPASS, i);
	write32_bit(micfil->base_addr + REG_MICFIL_DC_CTRL, MICFIL_DC_CTRL_MASK, val);
	micfil->dc_remover = MICFIL_DC_BYPASS;

	/* FIFO Watermark Control - FIFOWMK*/
	val = MICFIL_FIFO_CTRL_FIFOWMK(MICFIL_FIFO_DEPTH) - 1;
	write32_bit(micfil->base_addr + REG_MICFIL_FIFO_CTRL, MICFIL_FIFO_CTRL_FIFOWMK_MASK, val);

	if (!micfil->channel)
		micfil->channel = 2;
	ret = enable_channel(micfil, micfil->channel);
	if (ret) {
		LOG("failed to enable channels\n");
		return ERROR;
	}

	if (!micfil->sample_rate)
		micfil->sample_rate = 48000;
	ret = set_clock_params(micfil, micfil->sample_rate);
	if (ret) {
		LOG("failed to set clk\n");
		return ERROR;
	}

	return OK;
}

int micfil_release(void *p_micfil)
{
	if (p_micfil) {
		xaf_free(p_micfil, 0);
		p_micfil = NULL;
	}
	return 0;
}

int micfil_set_param(void *p_micfil, int i_idx, unsigned int *p_vaule)
{
	if (!p_micfil) {
		LOG("micfil is null\n");
		return ERROR;
	}
	struct fsl_micfil *micfil = (struct fsl_micfil*)p_micfil;

	switch (i_idx) {
	case MICFIL_SAMPLE_RATE:
		if (!rate_support(*p_vaule))
			goto Err;
		micfil->sample_rate = (int)*p_vaule;
		break;
	case MICFIL_CHANNEL:
		micfil->channel = (int)*p_vaule;
		break;
	case MICFIL_WIDTH:
		micfil->width = (int)*p_vaule;
		break;
	case MICFIL_QUALITY:
		micfil->quality = (int)*p_vaule;
		break;
	default:
		break;
	}

	return OK;
Err:
	return ERROR;
}

int micfil_get_param(void *p_micfil, int i_idx, unsigned int *p_vaule)
{
	/* TODO */
	return OK;
}

int micfil_start(void *p_micfil)
{
	int ret;
	if (!p_micfil) {
		LOG("micfil is null\n");
		return ERROR;
	}
	struct fsl_micfil *micfil = (struct fsl_micfil*)p_micfil;

	ret = micfil_reset(micfil);
	if (ret) {
		LOG("failed to soft reset\n");
		return ERROR;
	}

	/* DMA Interrupt Selection - DISEL bits
	 * 00 - DMA and IRQ disabled
	 * 01 - DMA req enabled
	 * 10 - IRQ enabled
	 * 11 - reserved
	 */
	write32_bit(micfil->base_addr + REG_MICFIL_CTRL1,
				 MICFIL_CTRL1_DISEL_MASK,
				 (1 << MICFIL_CTRL1_DISEL_SHIFT));

	/* Enable the module */
	write32_bit(micfil->base_addr + REG_MICFIL_CTRL1,
				 MICFIL_CTRL1_PDMIEN_MASK,
				 MICFIL_CTRL1_PDMIEN);

	return OK;
}

int micfil_stop(void *p_micfil)
{
	if (!p_micfil) {
		LOG("micfil is null\n");
		return ERROR;
	}
	struct fsl_micfil *micfil = (struct fsl_micfil*)p_micfil;

	/* Disable the module */
	write32_bit(micfil->base_addr + REG_MICFIL_CTRL1, MICFIL_CTRL1_PDMIEN_MASK, 0);

	write32_bit(micfil->base_addr + REG_MICFIL_CTRL1, MICFIL_CTRL1_DISEL_MASK,
				 (0 << MICFIL_CTRL1_DISEL_SHIFT));

	return OK;
}

void micfil_irq_handler(void *p_micfil)
{
	LOG("micfil irq handler\n");
}

void micfil_suspend(void *p_micfil)
{
	LOG("micfil suspend\n");
	int reg_idx;
	if (!p_micfil) {
		LOG("micfil is null\n");
		return;
	}
	struct fsl_micfil *micfil = (struct fsl_micfil*)p_micfil;

	for (reg_idx = 0; reg_idx < MICFIL_REG_NUM; reg_idx++) {
		if (micfil_readable_reg(micfil, reg_idx * 4))
			micfil->cache[reg_idx] = read32(micfil->base_addr + reg_idx * 4);
	}
	return;
}

void micfil_resume(void *p_micfil)
{
	int reg_idx;
	if (!p_micfil) {
		LOG("micfil is null\n");
		return;
	}
	struct fsl_micfil *micfil = (struct fsl_micfil*)p_micfil;

	for (reg_idx = 0; reg_idx < MICFIL_REG_NUM; reg_idx++) {
		if (micfil_writeable_reg(micfil, reg_idx * 4))
			write32(micfil->base_addr + reg_idx * 4, micfil->cache[reg_idx]);
	}
	return;
}

void micfil_dump(void *p_micfil)
{
	int reg_idx;
	if (!p_micfil) {
		LOG("micfil is null\n");
		return;
	}
	struct fsl_micfil *micfil = (struct fsl_micfil*)p_micfil;

	for (reg_idx = 0; reg_idx < MICFIL_REG_NUM; reg_idx++) {
		if (micfil_readable_reg(micfil, reg_idx))
			LOG2("micfil reg[%x]: %x\n", reg_idx * 4, read32(micfil->base_addr + reg_idx * 4));
	}
}

