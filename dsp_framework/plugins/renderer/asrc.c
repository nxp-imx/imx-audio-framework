/*
 * ASRC driver
 *
 * Copyright 2018-2020 NXP
 */
#include "mydefs.h"
#include "asrc.h"

static int proc_autosel(int Fsin, int Fsout, int *pre_proc, int *post_proc)
{
	int det_out_op2_cond;
	int det_out_op0_cond;
	det_out_op2_cond = (((Fsin * 15 > Fsout * 16) & (Fsout < 56000)) |
					((Fsin > 56000) & (Fsout < 56000)));
	det_out_op0_cond = (Fsin * 23 < Fsout * 8);

	/*
	 * Not supported case: Tsout>16.125*Tsin, and Tsout>8.125*Tsin.
	 */
	if (Fsin * 8 > 129 * Fsout)
		*pre_proc = 5;
	else if (Fsin * 8 > 65 * Fsout)
		*pre_proc = 4;
	else if (Fsin * 8 > 33 * Fsout)
		*pre_proc = 2;
	else if (Fsin * 8 > 15 * Fsout) {
		if (Fsin > 152000)
			*pre_proc = 2;
		else
			*pre_proc = 1;
	} else if (Fsin < 76000)
		*pre_proc = 0;
	else if (Fsin > 152000)
		*pre_proc = 2;
	else
		*pre_proc = 1;

	if (det_out_op2_cond)
		*post_proc = 2;
	else if (det_out_op0_cond)
		*post_proc = 0;
	else
		*post_proc = 1;

	if (*pre_proc == 4 || *pre_proc == 5)
		return -1;
	return 0;
}

#define IDEAL_RATIO_DECIMAL_DEPTH 26

int asrc_set_ideal_ratio(volatile void * asrc_addr, int index,
				    int inrate, int outrate)
{
	unsigned long ratio;
	int i;

	if (!outrate)
		return -1;

	/* Calculate the intergal part of the ratio */
	ratio = (inrate / outrate) << IDEAL_RATIO_DECIMAL_DEPTH;

	/* ... and then the 26 depth decimal part */
	inrate %= outrate;

	for (i = 1; i <= IDEAL_RATIO_DECIMAL_DEPTH; i++) {
		inrate <<= 1;

		if (inrate < outrate)
			continue;

		ratio |= 1 << (IDEAL_RATIO_DECIMAL_DEPTH - i);
		inrate -= outrate;

		if (!inrate)
			break;
	}

	write32(asrc_addr + REG_ASRIDRL(index), ratio);
	write32(asrc_addr + REG_ASRIDRH(index), ratio >> 24);

	return 0;
}



void asrc_init(volatile void * asrc_addr, int mode, int channels, int rate,
	       int width, int mclk_rate) {
	unsigned long ipg_rate;
	int reg, retry = 10;
	int clk_index, ideal;
	int pre_proc, post_proc;

	if (asrc_addr == NULL)
		return;
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

	clk_index = 15;
	ideal = 1;

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

	proc_autosel(rate, 48000, &pre_proc, &post_proc);

	/* apply configurations for pre- and post-processing */
	write32_bit(asrc_addr + REG_ASRCFG,
		    ASRCFG_PREMODi_MASK(0) | ASRCFG_POSTMODi_MASK(0),
		    ASRCFG_PREMOD(0, pre_proc) | ASRCFG_POSTMOD(0, post_proc));

	asrc_set_ideal_ratio(asrc_addr, 0, rate, 48000);
}

void asrc_start(volatile void *asrc_addr, int tx) {
	int reg, retry = 10, i;
	int index = 0;

	if (asrc_addr == NULL)
		return;
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
	if (asrc_addr == NULL)
		return;
	/* stop the current pair */
	write32_bit(asrc_addr + REG_ASRCTR, ASRCTR_ASRCEi_MASK(index), 0);
}

void asrc_irq_handler(volatile void *asrc_addr) {
	/* clear overload error */
	if (asrc_addr == NULL)
		return;
	write32(asrc_addr + REG_ASRSTR, ASRSTR_AOLE);
}

void asrc_suspend(volatile void *asrc_addr,  u32 *cache_addr) {
	if (asrc_addr == NULL)
		return;

	cache_addr[0] = read32(asrc_addr + REG_ASRCTR);
	cache_addr[1] = read32(asrc_addr + REG_ASRIER);
	cache_addr[2] = read32(asrc_addr + REG_ASRCNCR);
	cache_addr[3] = read32(asrc_addr + REG_ASRCFG);
	cache_addr[4] = read32(asrc_addr + REG_ASRCSR);
	cache_addr[5] = read32(asrc_addr + REG_ASRCDR1);
	cache_addr[6] = read32(asrc_addr + REG_ASRCDR2);
	cache_addr[7] = read32(asrc_addr + REG_ASRSTR);
	cache_addr[8] = read32(asrc_addr + REG_ASRPM1);
	cache_addr[9] = read32(asrc_addr + REG_ASRPM2);
	cache_addr[10] = read32(asrc_addr + REG_ASRPM3);
	cache_addr[11] = read32(asrc_addr + REG_ASRPM4);
	cache_addr[12] = read32(asrc_addr + REG_ASRPM5);
	cache_addr[13] = read32(asrc_addr + REG_ASRTFR1);
	cache_addr[14] = read32(asrc_addr + REG_ASRCCR);
	cache_addr[18] = read32(asrc_addr + REG_ASRIDRHA);
	cache_addr[19] = read32(asrc_addr + REG_ASRIDRLA);
	cache_addr[20] = read32(asrc_addr + REG_ASRIDRHB);
	cache_addr[21] = read32(asrc_addr + REG_ASRIDRLB);
	cache_addr[22] = read32(asrc_addr + REG_ASRIDRHC);
	cache_addr[23] = read32(asrc_addr + REG_ASRIDRLC);
	cache_addr[24] = read32(asrc_addr + REG_ASR76K);
	cache_addr[25] = read32(asrc_addr + REG_ASR56K);
	cache_addr[26] = read32(asrc_addr + REG_ASRMCRA);
	cache_addr[27] = read32(asrc_addr + REG_ASRFSTA);
	cache_addr[28] = read32(asrc_addr + REG_ASRMCRB);
	cache_addr[29] = read32(asrc_addr + REG_ASRFSTB);
	cache_addr[30] = read32(asrc_addr + REG_ASRMCRC);
	cache_addr[31] = read32(asrc_addr + REG_ASRFSTC);
	cache_addr[32] = read32(asrc_addr + REG_ASRMCR1A);
	cache_addr[33] = read32(asrc_addr + REG_ASRMCR1B);
	cache_addr[34] = read32(asrc_addr + REG_ASRMCR1C);
}

void asrc_resume(volatile void *asrc_addr,  u32 *cache_addr) {
	if (asrc_addr == NULL)
		return;

	write32(asrc_addr + REG_ASRCTR,  cache_addr[0]);
	write32(asrc_addr + REG_ASRIER,  cache_addr[1]);
	write32(asrc_addr + REG_ASRCNCR, cache_addr[2]);
	write32(asrc_addr + REG_ASRCFG,  cache_addr[3]);
	write32(asrc_addr + REG_ASRCSR,  cache_addr[4]);
	write32(asrc_addr + REG_ASRCDR1, cache_addr[5]);
	write32(asrc_addr + REG_ASRCDR2, cache_addr[6]);
	write32(asrc_addr + REG_ASRSTR,  cache_addr[7]);
	write32(asrc_addr + REG_ASRPM1,  cache_addr[8]);
	write32(asrc_addr + REG_ASRPM2,  cache_addr[9]);
	write32(asrc_addr + REG_ASRPM3,  cache_addr[10]);
	write32(asrc_addr + REG_ASRPM4,  cache_addr[11]);
	write32(asrc_addr + REG_ASRPM5,  cache_addr[12]);
	write32(asrc_addr + REG_ASRTFR1, cache_addr[13]);
	write32(asrc_addr + REG_ASRCCR,  cache_addr[14]);
	write32(asrc_addr + REG_ASRIDRHA, cache_addr[18]);
	write32(asrc_addr + REG_ASRIDRLA, cache_addr[19]);
	write32(asrc_addr + REG_ASRIDRHB, cache_addr[20]);
	write32(asrc_addr + REG_ASRIDRLB, cache_addr[21]);
	write32(asrc_addr + REG_ASRIDRHC, cache_addr[22]);
	write32(asrc_addr + REG_ASRIDRLC, cache_addr[23]);
	write32(asrc_addr + REG_ASR76K,  cache_addr[24]);
	write32(asrc_addr + REG_ASR56K,  cache_addr[25]);
	write32(asrc_addr + REG_ASRMCRA, cache_addr[26]);
	write32(asrc_addr + REG_ASRMCRB, cache_addr[28]);
	write32(asrc_addr + REG_ASRMCRC, cache_addr[30]);
	write32(asrc_addr + REG_ASRMCR1A, cache_addr[32]);
	write32(asrc_addr + REG_ASRMCR1B, cache_addr[33]);
	write32(asrc_addr + REG_ASRMCR1C, cache_addr[34]);
}

void asrc_dump(volatile void *asrc_addr) {

	if (asrc_addr == NULL)
		return;
}

int asrc_hw_params(volatile void *asrc_addr, int channels, int rate, int format, volatile void *context)
{
	return 0;
}
