/*
 * Copyright 2018-2020 NXP
 */

#include "mydefs.h"
#include "esai.h"
#include "debug.h"


/***********************************************************
mode: 0- slave mode,  1- master mode
channel: the channel number
rate:    the sample rate
format:  the sample format, default is S16_LE
mclk_rate: the rate of the source clock
************************************************************/
void esai_init(volatile void * esai_addr, int mode, int channels, int rate,
	       int width, int mclk_rate) {

	write32(esai_addr + REG_ESAI_ECR, ESAI_ECR_ERST);
	write32(esai_addr + REG_ESAI_ECR, ESAI_ECR_ESAIEN);
	write32(esai_addr + REG_ESAI_SAICR, 0);

	write32_bit(esai_addr + REG_ESAI_TCCR,  ESAI_xCCR_xDC_MASK,
		    ESAI_xCCR_xDC(2));
	write32_bit(esai_addr + REG_ESAI_TCCR,
		    ESAI_xCCR_xCKP | ESAI_xCCR_xHCKP | ESAI_xCCR_xHCKD |
		    ESAI_xCCR_xFSP | ESAI_xCCR_xFSD | ESAI_xCCR_xCKD,
		    ESAI_xCCR_xCKP | ESAI_xCCR_xHCKP | ESAI_xCCR_xHCKD |
		    ESAI_xCCR_xFSD | ESAI_xCCR_xCKD);

	write32_bit(esai_addr + REG_ESAI_TCR,
		    ESAI_xCR_xFSL | ESAI_xCR_xFSR | ESAI_xCR_xWA, 0);

	if (channels > 1)
		write32_bit(esai_addr + REG_ESAI_TCR,
			    ESAI_xCR_xMOD_MASK, ESAI_xCR_xMOD_NETWORK);
	else
		write32_bit(esai_addr + REG_ESAI_TCR, ESAI_xCR_xMOD_MASK, 0);

	write32_bit(esai_addr + REG_ESAI_TCR,
				ESAI_xCR_xSWS_MASK | ESAI_xCR_PADC,
				ESAI_xCR_xSWS(32, 16) | ESAI_xCR_PADC);

	write32_bit(esai_addr + REG_ESAI_PRRC,
				ESAI_PRRC_PDC_MASK,
				ESAI_PRRC_PDC(ESAI_GPIO));
        write32_bit(esai_addr + REG_ESAI_PCRC,
				ESAI_PCRC_PC_MASK,
				ESAI_PCRC_PC(ESAI_GPIO));

        write32(esai_addr + REG_ESAI_TSMA, 0);
        write32(esai_addr + REG_ESAI_TSMB, 0);
	write32(esai_addr + REG_ESAI_RSMA, 0);
	write32(esai_addr + REG_ESAI_RSMB, 0);

	write32_bit(esai_addr + REG_ESAI_ECR, ESAI_ECR_ETI, ESAI_ECR_ETI);

	write32_bit(esai_addr + REG_ESAI_TFCR,
				ESAI_xFCR_xFR_MASK,
				ESAI_xFCR_xFR);

	write32_bit(esai_addr + REG_ESAI_TFCR,
				ESAI_xFCR_xFR_MASK | ESAI_xFCR_xWA_MASK | ESAI_xFCR_xFWM_MASK | ESAI_xFCR_TE_MASK | ESAI_xFCR_TIEN,
				ESAI_xFCR_xWA(16) | ESAI_xFCR_xFWM(64) | ESAI_xFCR_TE(1) | ESAI_xFCR_TIEN);

	write32_bit(esai_addr + REG_ESAI_TCCR, ESAI_xCCR_xFP_MASK, ESAI_xCCR_xFP(8));
	write32_bit(esai_addr + REG_ESAI_TCCR, ESAI_xCCR_xPSR_MASK, ESAI_xCCR_xPSR_BYPASS);

}


void esai_start(volatile void * esai_addr, int tx) {

	write32_bit(esai_addr + REG_ESAI_xFCR(tx), ESAI_xFCR_xFEN_MASK, ESAI_xFCR_xFEN);
	write32_bit(esai_addr + REG_ESAI_xCR(tx),
				tx ? ESAI_xCR_TE_MASK : ESAI_xCR_RE_MASK,
				tx ? ESAI_xCR_TE(1) : ESAI_xCR_RE(1));
	write32_bit(esai_addr + REG_ESAI_xSMB(tx),
				ESAI_xSMB_xS_MASK, ESAI_xSMB_xS(0x3));
	write32_bit(esai_addr + REG_ESAI_xSMA(tx),
				ESAI_xSMA_xS_MASK, ESAI_xSMA_xS(0x3));
}

void esai_stop(volatile void * esai_addr, int tx) {

	write32_bit(esai_addr + REG_ESAI_xCR(tx),
				tx ? ESAI_xCR_TE_MASK : ESAI_xCR_RE_MASK, 0);
	write32_bit(esai_addr + REG_ESAI_xSMA(tx),
				ESAI_xSMA_xS_MASK, 0);
        write32_bit(esai_addr + REG_ESAI_xSMB(tx),
				ESAI_xSMB_xS_MASK, 0);
	write32_bit(esai_addr + REG_ESAI_xFCR(tx),
				ESAI_xFCR_xFR | ESAI_xFCR_xFEN,
				ESAI_xFCR_xFR);
	write32_bit(esai_addr + REG_ESAI_xFCR(tx),
				ESAI_xFCR_xFR, 0);
}

void esai_suspend(volatile void *esai_addr,  u32 *cache_addr) {

	cache_addr[0] = read32(esai_addr + REG_ESAI_ECR);
	cache_addr[1] = read32(esai_addr + REG_ESAI_ESR);
	cache_addr[2] = read32(esai_addr + REG_ESAI_TFCR);
	cache_addr[3] = read32(esai_addr + REG_ESAI_TFSR);
	cache_addr[4] = read32(esai_addr + REG_ESAI_RFCR);
	cache_addr[5] = read32(esai_addr + REG_ESAI_RFSR);
	cache_addr[6] = read32(esai_addr + REG_ESAI_SAISR);
	cache_addr[7] = read32(esai_addr + REG_ESAI_SAICR);
	cache_addr[8] = read32(esai_addr + REG_ESAI_TCR);
	cache_addr[9] = read32(esai_addr + REG_ESAI_TCCR);
	cache_addr[10] = read32(esai_addr + REG_ESAI_RCR);
	cache_addr[11] = read32(esai_addr + REG_ESAI_RCCR);
	cache_addr[12] = read32(esai_addr + REG_ESAI_TSMA);
	cache_addr[13] = read32(esai_addr + REG_ESAI_TSMB);
	cache_addr[14] = read32(esai_addr + REG_ESAI_RSMA);
	cache_addr[15] = read32(esai_addr + REG_ESAI_RSMB);
	cache_addr[16] = read32(esai_addr + REG_ESAI_PRRC);
	cache_addr[17] = read32(esai_addr + REG_ESAI_PCRC);
}

void esai_resume(volatile void *esai_addr, u32 * cache_addr) {

	write32(esai_addr + REG_ESAI_ECR,   cache_addr[0]);
	write32(esai_addr + REG_ESAI_TFCR,  cache_addr[2]);
	write32(esai_addr + REG_ESAI_RFCR,  cache_addr[4]);
	write32(esai_addr + REG_ESAI_SAICR, cache_addr[7]);
	write32(esai_addr + REG_ESAI_TCR,   cache_addr[8]);
	write32(esai_addr + REG_ESAI_TCCR,  cache_addr[9]);
	write32(esai_addr + REG_ESAI_RCR,   cache_addr[10]);
	write32(esai_addr + REG_ESAI_RCCR,  cache_addr[11]);
	write32(esai_addr + REG_ESAI_TSMA,  cache_addr[12]);
	write32(esai_addr + REG_ESAI_TSMB,  cache_addr[13]);
	write32(esai_addr + REG_ESAI_RSMA,  cache_addr[14]);
	write32(esai_addr + REG_ESAI_RSMB,  cache_addr[15]);
	write32(esai_addr + REG_ESAI_PRRC,  cache_addr[16]);
	write32(esai_addr + REG_ESAI_PCRC,  cache_addr[17]);
	write32_bit(esai_addr + REG_ESAI_TFCR,
				ESAI_xFCR_xFR_MASK,
				ESAI_xFCR_xFR);
	write32_bit(esai_addr + REG_ESAI_TFCR,
				ESAI_xFCR_xFR_MASK,
				0);
}

void esai_irq_handler(volatile void * esai_addr) {

}


void esai_dump(volatile void * esai_addr) {
	LOG1("ESAI_ECR %x\n", read32(esai_addr + REG_ESAI_ECR));
	LOG1("ESAI_ESR %x\n", read32(esai_addr + REG_ESAI_ESR));
	LOG1("ESAI_TFCR %x\n", read32(esai_addr + REG_ESAI_TFCR));
	LOG1("ESAI_TFSR %x\n", read32(esai_addr + REG_ESAI_TFSR));
	LOG1("ESAI_RFCR %x\n", read32(esai_addr + REG_ESAI_RFCR));
	LOG1("ESAI_RFSR %x\n", read32(esai_addr + REG_ESAI_RFSR));
	LOG1("ESAI_SAISR %x\n", read32(esai_addr + REG_ESAI_SAISR));
	LOG1("ESAI_SAICR %x\n", read32(esai_addr + REG_ESAI_SAICR));
	LOG1("ESAI_TCR %x\n",   read32(esai_addr + REG_ESAI_TCR));
	LOG1("ESAI_TCCR %x\n",  read32(esai_addr + REG_ESAI_TCCR));
	LOG1("ESAI_RCR %x\n",   read32(esai_addr + REG_ESAI_RCR));
	LOG1("ESAI_RCCR %x\n",  read32(esai_addr + REG_ESAI_RCCR));

	LOG1("ESAI_TSMA %x\n",  read32(esai_addr + REG_ESAI_TSMA));
	LOG1("ESAI_TSMB %x\n",  read32(esai_addr + REG_ESAI_TSMB));
	LOG1("ESAI_RSMA %x\n",  read32(esai_addr + REG_ESAI_RSMA));
	LOG1("ESAI_RSMB %x\n",  read32(esai_addr + REG_ESAI_RSMB));
	LOG1("ESAI_PRRC %x\n",  read32(esai_addr + REG_ESAI_PRRC));
	LOG1("ESAI_PCRC %x\n",  read32(esai_addr + REG_ESAI_PCRC));
}

