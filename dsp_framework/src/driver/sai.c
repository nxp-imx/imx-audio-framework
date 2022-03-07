/*
 * Copyright 2020 NXP
 *
 * sai.c - SAI driver
 */

#include "mydefs.h"
#include "sai.h"
#include "edma.h"
#include "irqstr.h"
#include "io.h"
#include "debug.h"

#ifdef PLATF_8M
#define offset 8
#else
#define offset 0
#endif

void sai_init(volatile void * sai_addr,  int mode, int channel, int rate,
	      int width, int mclk_rate) {

	/* reset */
	write32(sai_addr + FSL_SAI_TCSR(offset), FSL_SAI_CSR_SR);
	write32(sai_addr + FSL_SAI_RCSR(offset), FSL_SAI_CSR_SR);
	write32(sai_addr + FSL_SAI_TCSR(offset), 0);
	write32(sai_addr + FSL_SAI_RCSR(offset), 0);

#ifdef PLATF_8M
	write32(sai_addr + FSL_SAI_MCTL, FSL_SAI_MCTL_MCLK_EN);
#endif

	write32(sai_addr + FSL_SAI_TCR1(offset), 32);
	write32(sai_addr + FSL_SAI_RCR1(offset), 32);

	write32(sai_addr + FSL_SAI_TCR2(offset), FSL_SAI_CR2_MSEL_MCLK1);
	write32(sai_addr + FSL_SAI_RCR2(offset), FSL_SAI_CR2_MSEL_MCLK1);

	write32_bit(sai_addr + FSL_SAI_TCR2(offset),
		    FSL_SAI_CR2_BCP|FSL_SAI_CR2_BCD_MSTR,
		    FSL_SAI_CR2_BCP|FSL_SAI_CR2_BCD_MSTR);
	write32_bit(sai_addr + FSL_SAI_RCR2(offset),
		    FSL_SAI_CR2_BCP|FSL_SAI_CR2_BCD_MSTR,
		    FSL_SAI_CR2_BCP|FSL_SAI_CR2_BCD_MSTR);

	write32_bit(sai_addr + FSL_SAI_TCR4(offset),
		    FSL_SAI_CR4_MF|FSL_SAI_CR4_FSE|FSL_SAI_CR4_FSP|FSL_SAI_CR4_FSD_MSTR,
		    FSL_SAI_CR4_MF|FSL_SAI_CR4_FSE|FSL_SAI_CR4_FSP|FSL_SAI_CR4_FSD_MSTR);
	write32_bit(sai_addr + FSL_SAI_RCR4(offset),
		    FSL_SAI_CR4_MF|FSL_SAI_CR4_FSE|FSL_SAI_CR4_FSP|FSL_SAI_CR4_FSD_MSTR,
		    FSL_SAI_CR4_MF|FSL_SAI_CR4_FSE|FSL_SAI_CR4_FSP|FSL_SAI_CR4_FSD_MSTR);

#ifdef PLATF_8M
	write32_bit(sai_addr + FSL_SAI_TCR2(offset), FSL_SAI_CR2_DIV_MASK, 3);
#else
	write32_bit(sai_addr + FSL_SAI_TCR2(offset), FSL_SAI_CR2_DIV_MASK, 7);
#endif
	write32_bit(sai_addr + FSL_SAI_RCR2(offset), FSL_SAI_CR2_DIV_MASK, 7);
	
	write32_bit(sai_addr + FSL_SAI_TCR3(offset), FSL_SAI_CR3_TRCE_MASK, 1 << 16);
	write32_bit(sai_addr + FSL_SAI_RCR3(offset), FSL_SAI_CR3_TRCE_MASK, 1 << 16);

	write32_bit(sai_addr + FSL_SAI_TCR4(offset), FSL_SAI_CR4_SYWD_MASK, FSL_SAI_CR4_SYWD(16));
	write32_bit(sai_addr + FSL_SAI_TCR4(offset), FSL_SAI_CR4_FRSZ_MASK, FSL_SAI_CR4_FRSZ(2));

	write32_bit(sai_addr + FSL_SAI_RCR4(offset), FSL_SAI_CR4_SYWD_MASK, FSL_SAI_CR4_SYWD(16));
	write32_bit(sai_addr + FSL_SAI_RCR4(offset), FSL_SAI_CR4_FRSZ_MASK, FSL_SAI_CR4_FRSZ(2));

	write32_bit(sai_addr + FSL_SAI_TCR5(offset),
		    FSL_SAI_CR5_WNW_MASK|FSL_SAI_CR5_W0W_MASK|FSL_SAI_CR5_FBT_MASK,
		    FSL_SAI_CR5_WNW(16)|FSL_SAI_CR5_W0W(16)|FSL_SAI_CR5_FBT(15));
	write32_bit(sai_addr + FSL_SAI_RCR5(offset),
		    FSL_SAI_CR5_WNW_MASK|FSL_SAI_CR5_W0W_MASK|FSL_SAI_CR5_FBT_MASK,
		    FSL_SAI_CR5_WNW(16)|FSL_SAI_CR5_W0W(16)|FSL_SAI_CR5_FBT(15));

	if (channel > 1)
		write32(sai_addr + FSL_SAI_TMR, 0xFFFFFFFC);
	else
		write32(sai_addr + FSL_SAI_TMR, 0xFFFFFFFE);
	write32(sai_addr + FSL_SAI_RMR, 0xFFFFFFFC);
}

#define FSL_SAI_FLAGS  (FSL_SAI_CSR_SEIE | FSL_SAI_CSR_FEIE)

void sai_irq_handler(volatile void * sai_addr) {

	u32 tcsr = *(volatile unsigned int *)(sai_addr + FSL_SAI_TCSR(offset));
	u32 mask = (FSL_SAI_FLAGS >> FSL_SAI_CSR_xIE_SHIFT) << FSL_SAI_CSR_xF_SHIFT;
	u32 flags = tcsr & mask;
	u32 rcsr;

	if(!flags)
		goto irq_rx;

	if (flags & FSL_SAI_CSR_FEF)
		tcsr |= FSL_SAI_CSR_FR;

	flags &= FSL_SAI_CSR_xF_W_MASK;
	tcsr &= ~FSL_SAI_CSR_xF_MASK;

	if(flags)
		write32(sai_addr + FSL_SAI_TCSR(offset), flags | tcsr);

irq_rx:
	rcsr = *(volatile unsigned int *)(sai_addr + FSL_SAI_RCSR(offset));
	flags = rcsr & mask;

	if (!flags)
		goto out;

	if (flags & FSL_SAI_CSR_FEF)
		rcsr |= FSL_SAI_CSR_FR;

	flags &= FSL_SAI_CSR_xF_W_MASK;
	tcsr &= ~FSL_SAI_CSR_xF_MASK;

	if(flags)
		write32(sai_addr + FSL_SAI_RCSR(offset), flags | tcsr);

out:
	return;
}

void sai_start(volatile void * sai_addr, int tx) {

	write32_bit(sai_addr + FSL_SAI_TCR2(offset), FSL_SAI_CR2_SYNC, 0);
	write32_bit(sai_addr + FSL_SAI_RCR2(offset), FSL_SAI_CR2_SYNC, FSL_SAI_CR2_SYNC);

	if (tx) {
		write32_bit(sai_addr + FSL_SAI_TCSR(offset),
			    FSL_SAI_CSR_FRDE, FSL_SAI_CSR_FRDE);
		write32_bit(sai_addr + FSL_SAI_TCSR(offset),
			    FSL_SAI_CSR_TERE, FSL_SAI_CSR_TERE);
		write32_bit(sai_addr + FSL_SAI_TCSR(offset),
			    FSL_SAI_CSR_SE, FSL_SAI_CSR_SE);
		write32_bit(sai_addr + FSL_SAI_TCSR(offset),
			    FSL_SAI_CSR_xIE_MASK, FSL_SAI_FLAGS);
	} else {
		write32_bit(sai_addr + FSL_SAI_RCSR(offset),
			    FSL_SAI_CSR_FRDE, FSL_SAI_CSR_FRDE);
		write32_bit(sai_addr + FSL_SAI_TCSR(offset),
			    FSL_SAI_CSR_TERE, FSL_SAI_CSR_TERE);
		write32_bit(sai_addr + FSL_SAI_RCSR(offset),
			    FSL_SAI_CSR_TERE, FSL_SAI_CSR_TERE);
		write32_bit(sai_addr + FSL_SAI_RCSR(offset),
			    FSL_SAI_CSR_SE, FSL_SAI_CSR_SE);
		write32_bit(sai_addr + FSL_SAI_RCSR(offset),
			    FSL_SAI_CSR_xIE_MASK, FSL_SAI_FLAGS);
	}

	return;
}

void sai_stop(volatile void * sai_addr, int tx) {
	u32 xcsr;
	u32 count = 100;

	if (tx) {
		write32_bit(sai_addr + FSL_SAI_TCSR(offset), FSL_SAI_CSR_FRDE, 0);
		write32_bit(sai_addr + FSL_SAI_TCSR(offset), FSL_SAI_CSR_xIE_MASK, 0);
	} else {
		write32_bit(sai_addr + FSL_SAI_RCSR(offset), FSL_SAI_CSR_FRDE, 0);
		write32_bit(sai_addr + FSL_SAI_RCSR(offset), FSL_SAI_CSR_xIE_MASK, 0);
	}

	xcsr = read32(sai_addr + FSL_SAI_xCSR(!tx, 0));

	if (!(xcsr & FSL_SAI_CSR_FRDE)) {
		write32_bit(sai_addr + FSL_SAI_TCSR(offset), FSL_SAI_CSR_TERE, 0);
		write32_bit(sai_addr + FSL_SAI_RCSR(offset), FSL_SAI_CSR_TERE, 0);

		do {
			//sleep(100);
			xcsr = read32(sai_addr + FSL_SAI_xCSR(tx, 0));
		} while (--count && xcsr & FSL_SAI_CSR_TERE);

		write32_bit(sai_addr + FSL_SAI_TCSR(offset), FSL_SAI_CSR_FR, FSL_SAI_CSR_FR);
		write32_bit(sai_addr + FSL_SAI_RCSR(offset), FSL_SAI_CSR_FR, FSL_SAI_CSR_FR);

		/* master mode */
		write32_bit(sai_addr + FSL_SAI_TCSR(offset), FSL_SAI_CSR_SR, FSL_SAI_CSR_SR);
		write32_bit(sai_addr + FSL_SAI_RCSR(offset), FSL_SAI_CSR_SR, FSL_SAI_CSR_SR);
		write32_bit(sai_addr + FSL_SAI_TCSR(offset), FSL_SAI_CSR_SR, 0);
		write32_bit(sai_addr + FSL_SAI_RCSR(offset), FSL_SAI_CSR_SR, 0);
	}
}

void sai_dump(volatile void * sai_addr) {

	LOG1("SAI_TCSR %x\n", read32(sai_addr + FSL_SAI_TCSR(offset)));
	LOG1("SAI_TCR1 %x\n", read32(sai_addr + FSL_SAI_TCR1(offset)));
	LOG1("SAI_TCR2 %x\n", read32(sai_addr + FSL_SAI_TCR2(offset)));
	LOG1("SAI_TCR3 %x\n", read32(sai_addr + FSL_SAI_TCR3(offset)));
	LOG1("SAI_TCR4 %x\n", read32(sai_addr + FSL_SAI_TCR4(offset)));
	LOG1("SAI_TCR5 %x\n", read32(sai_addr + FSL_SAI_TCR5(offset)));
	LOG1("SAI_TFR0 %x\n", read32(sai_addr + FSL_SAI_TFR0));
	LOG1("SAI_TMR %x\n", read32(sai_addr + FSL_SAI_TMR));

	LOG1("SAI_RCSR %x\n", read32(sai_addr + FSL_SAI_RCSR(offset)));
	LOG1("SAI_RCR1 %x\n", read32(sai_addr + FSL_SAI_RCR1(offset)));
	LOG1("SAI_RCR2 %x\n", read32(sai_addr + FSL_SAI_RCR2(offset)));
	LOG1("SAI_RCR3 %x\n", read32(sai_addr + FSL_SAI_RCR3(offset)));
	LOG1("SAI_RCR4 %x\n", read32(sai_addr + FSL_SAI_RCR4(offset)));
	LOG1("SAI_RCR5 %x\n", read32(sai_addr + FSL_SAI_RCR5(offset)));
	LOG1("SAI_RDR0 %x\n", read32(sai_addr + FSL_SAI_RDR0));
	LOG1("SAI_RFR0 %x\n", read32(sai_addr + FSL_SAI_RFR0));
	LOG1("SAI_RMR %x\n", read32(sai_addr + FSL_SAI_RMR));

	return;
}
void sai_suspend(volatile void *sai_addr,  u32 *cache_addr) {

	LOG("sai suspend\n");
	cache_addr[0] = read32(sai_addr + FSL_SAI_TCSR(offset));
	cache_addr[1] = read32(sai_addr + FSL_SAI_TCR1(offset));
	cache_addr[2] = read32(sai_addr + FSL_SAI_TCR2(offset));
	cache_addr[3] = read32(sai_addr + FSL_SAI_TCR3(offset));
	cache_addr[4] = read32(sai_addr + FSL_SAI_TCR4(offset));
	cache_addr[5] = read32(sai_addr + FSL_SAI_TCR5(offset));
	cache_addr[6] = read32(sai_addr + FSL_SAI_TMR);
	cache_addr[7] = read32(sai_addr + FSL_SAI_RCSR(offset));
	cache_addr[8] = read32(sai_addr + FSL_SAI_RCR1(offset));
	cache_addr[9] = read32(sai_addr + FSL_SAI_RCR2(offset));
	cache_addr[10] = read32(sai_addr + FSL_SAI_RCR3(offset));
	cache_addr[11] = read32(sai_addr + FSL_SAI_RCR4(offset));
	cache_addr[12] = read32(sai_addr + FSL_SAI_RCR5(offset));
	cache_addr[13] = read32(sai_addr + FSL_SAI_RMR);
#ifdef PLATF_8M
	cache_addr[14] = read32(sai_addr + FSL_SAI_MCTL);
#endif

}

void sai_resume(volatile void *sai_addr, u32 * cache_addr) {

	LOG("sai resume\n");
	write32(sai_addr + FSL_SAI_TCR1(offset),   cache_addr[1]);
	write32(sai_addr + FSL_SAI_TCR2(offset),   cache_addr[2]);
	write32(sai_addr + FSL_SAI_TCR3(offset),   cache_addr[3]);
	write32(sai_addr + FSL_SAI_TCR4(offset),   cache_addr[4]);
	write32(sai_addr + FSL_SAI_TCR5(offset),   cache_addr[5]);
	write32(sai_addr + FSL_SAI_TMR,       cache_addr[6]);
	write32(sai_addr + FSL_SAI_TCSR(offset),   cache_addr[0]);
	write32(sai_addr + FSL_SAI_RCR1(offset),   cache_addr[8]);
	write32(sai_addr + FSL_SAI_RCR2(offset),   cache_addr[9]);
	write32(sai_addr + FSL_SAI_RCR3(offset),   cache_addr[10]);
	write32(sai_addr + FSL_SAI_RCR4(offset),   cache_addr[11]);
	write32(sai_addr + FSL_SAI_RCR5(offset),   cache_addr[12]);
	write32(sai_addr + FSL_SAI_RMR,       cache_addr[13]);
	write32(sai_addr + FSL_SAI_RCSR(offset),   cache_addr[7]);
#ifdef PLATF_8M
	write32(sai_addr + FSL_SAI_MCTL,      cache_addr[14]);
#endif
}
