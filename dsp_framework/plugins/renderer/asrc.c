/*
 * ASRC driver
 *
 * Copyright 2018 NXP
 */
#include "mydefs.h"
#include "asrc.h"

void asrc_init(volatile void * asrc_addr, int mode, int channels, int rate,
	       int width, int mclk_rate) {
	unsigned long ipg_rate;
	int reg, retry = 10;
	int clk_index, ideal;

	/* initialize ASRC */

	/* Halt ASRC internal FP when input FIFO needs data for pair A, B, C */
	write32(asrc_addr + REG_ASRCTR, ASRCTR_ASRCEN);

	/* Disable interrupt by default */
	write32(asrc_addr + REG_ASRIER, 0x0);

	/* Apply recommended settings for parameters from Reference Manual */
	write32(asrc_addr + REG_ASRPM1, 0x7fffff);
	write32(asrc_addr + REG_ASRPM2, 0x255555);
	write32(asrc_addr + REG_ASRPM3, 0xff7280);
	write32(asrc_addr + REG_ASRPM4, 0xff7280);
	write32(asrc_addr + REG_ASRPM5, 0xff7280);

	/* Base address for task queue FIFO. Set to 0x7C */
	write32_bit(asrc_addr + REG_ASRTFR1,
			   ASRTFR1_TF_BASE_MASK, ASRTFR1_TF_BASE(0xfc));

	ipg_rate = 150000000;
	/* Set the period of the 76KHz and 56KHz sampling clocks based on
	 * the ASRC processing clock.
	 */
	write32(asrc_addr + REG_ASR76K, ipg_rate / 76000);
	write32(asrc_addr + REG_ASR56K, ipg_rate / 56000);

	/* configure ASRC */
	if (rate == 48000) {
		clk_index = 0;
		ideal = 0;
	} else {
		clk_index = 15;
		ideal = 1;
	}

	/* 2 channels, pair A */
	write32_bit(asrc_addr + REG_ASRCNCR,
		    ASRCNCR_ANCi_MASK(0, 4),
		    ASRCNCR_ANCi(0, 2, 4));

	/* automatic selection for processing mode */
	write32_bit(asrc_addr + REG_ASRCTR, ASRCTR_ATSi_MASK(0),
		    ASRCTR_ATS(0));

	/* use internal measured ratio */
	write32_bit(asrc_addr + REG_ASRCTR,
		    ASRCTR_USRi_MASK(0) | ASRCTR_IDRi_MASK(0),
		    ASRCTR_USR(0));

	/* set the input and output clock sources */
	write32_bit(asrc_addr + REG_ASRCSR,
		    ASRCSR_AICSi_MASK(0) | ASRCSR_AOCSi_MASK(0),
		    ASRCSR_AICS(0, clk_index) | ASRCSR_AOCS(0, 0));

	/* input clock divisors */
	write32_bit(asrc_addr + REG_ASRCDR(0),
		    ASRCDRi_AOCPi_MASK(0) | ASRCDRi_AICPi_MASK(0) |
		    ASRCDRi_AOCDi_MASK(0) | ASRCDRi_AICDi_MASK(0),
		    ASRCDRi_AOCP(0, 63) | ASRCDRi_AICP(0, 63));

	/* implement word_width configurations */
	write32_bit(asrc_addr + REG_ASRMCR1(0),
		     ASRMCR1i_OW16_MASK | ASRMCR1i_IWD_MASK,
		     ASRMCR1i_OW16(1) | ASRMCR1i_IWD(1));

	/* enable buffer stall */
	write32_bit(asrc_addr + REG_ASRMCR(0), REG_ASRMCR(0),
		    ASRMCRi_BUFSTALLi_MASK, ASRMCRi_BUFSTALLi);

	/* set default thresholds for input and output fifo */
	write32_bit(asrc_addr + REG_ASRMCR(0),
		   ASRMCRi_EXTTHRSHi_MASK |
		   ASRMCRi_INFIFO_THRESHOLD_MASK |
		   ASRMCRi_OUTFIFO_THRESHOLD_MASK,
		   ASRMCRi_EXTTHRSHi |
		   ASRMCRi_INFIFO_THRESHOLD(32) |
		   ASRMCRi_OUTFIFO_THRESHOLD(32));

	if (!ideal)
		return;

	/* clear ASTSx bit to use Ideal Ratio mode */
	write32_bit(asrc_addr + REG_ASRCTR, ASRCTR_ATSi_MASK(0), 0);

	/* enable Ideal Ratio mode */
	write32_bit(asrc_addr + REG_ASRCTR,
		    ASRCTR_IDRi_MASK(0) | ASRCTR_USRi_MASK(0),
		    ASRCTR_IDR(0) | ASRCTR_USR(0));

	/* apply configurations for pre- and post-processing */
	write32_bit(asrc_addr + REG_ASRCFG,
		    ASRCFG_PREMODi_MASK(0) | ASRCFG_POSTMODi_MASK(0),
		    ASRCFG_PREMOD(0, 0) | ASRCFG_POSTMOD(0, 1));

	/* set ideal ratio = 44100/48000 */
	write32(asrc_addr + REG_ASRIDRL(0), 0x00ACCCCC);
	write32(asrc_addr + REG_ASRIDRH(0), 0x00000003);
}

void asrc_start(volatile void *asrc_addr, int tx) {
	int reg, retry = 10, i;
	int index = 0;

	/* Enable the current pair */
	write32_bit(asrc_addr + REG_ASRCTR, ASRCTR_ASRCEi_MASK(index),
		    ASRCTR_ASRCE(index));

	/* Wait for status of initialization */
	do {
		//sleep(50);
		reg = read32(asrc_addr + REG_ASRCFG);
		reg &= ASRCFG_INIRQi_MASK(index);
	} while (!reg && --retry);

	/* Make the input fifo to ASRC STALL level */
	reg = read32(asrc_addr, REG_ASRCNCR);
	for (i = 0; i < 2 * 4; i++)
		write32(asrc_addr + REG_ASRDI(index), 0);

	/* Enable overload interrupt */
	write32(asrc_addr + REG_ASRIER, ASRIER_AOLIE);
}

void asrc_stop(volatile void *asrc_addr, int tx) {
	int index = 0;
	/* stop the current pair */
	write32_bit(asrc_addr + REG_ASRCTR, ASRCTR_ASRCEi_MASK(index), 0);
}

void asrc_irq_handler(volatile void *asrc_addr) {
	/* clear overload error */
	write32(asrc_addr + REG_ASRSTR, ASRSTR_AOLE);
}

void asrc_dump(volatile void *asrc_addr) { }

