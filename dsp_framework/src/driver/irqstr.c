/*
 * irqstr.c - IRQ steer driver
 *
 * Copyright 2018-2020 NXP
 */
#include "mydefs.h"
#include "irqstr.h"
#include "debug.h"

void irqstr_init(volatile void *irqstr_addr, int dev_Int, int dma_Int) {
#ifndef PLATF_8M
	write32_bit(irqstr_addr + IRQSTEER_CHnMASK(IRQ_TO_MASK_OFFSET(dev_Int + 32)),
			1 << IRQ_TO_MASK_SHIFT(dev_Int + 32),
			1 << IRQ_TO_MASK_SHIFT(dev_Int + 32));
	write32_bit(irqstr_addr + IRQSTEER_CHnMASK(IRQ_TO_MASK_OFFSET(dma_Int + 32)),
			1 << IRQ_TO_MASK_SHIFT(dma_Int + 32),
			1 << IRQ_TO_MASK_SHIFT(dma_Int + 32));
#endif
}

void irqstr_start(volatile void *irqstr_addr, int dev_Int, int dma_Int) {
#ifndef PLATF_8M
	write32(irqstr_addr + IRQSTEER_CHnCTL, 0x1);
#else
	write32_bit(irqstr_addr + IRQSTEER_CHnMASK(IRQ_TO_MASK_OFFSET(dev_Int + 32)),
			1 << IRQ_TO_MASK_SHIFT(dev_Int + 32),
			1 << IRQ_TO_MASK_SHIFT(dev_Int + 32));
	write32_bit(irqstr_addr + IRQSTEER_CHnMASK(IRQ_TO_MASK_OFFSET(dma_Int + 32)),
			1 << IRQ_TO_MASK_SHIFT(dma_Int + 32),
			1 << IRQ_TO_MASK_SHIFT(dma_Int + 32));
#endif
}

void irqstr_stop(volatile void *irqstr_addr, int dev_Int, int dma_Int) {
#ifndef PLATF_8M
	write32(irqstr_addr + IRQSTEER_CHnCTL, 0x0);
#else
	write32_bit(irqstr_addr + IRQSTEER_CHnMASK(IRQ_TO_MASK_OFFSET(dev_Int + 32)),
			1 << IRQ_TO_MASK_SHIFT(dev_Int + 32),
			0);
	write32_bit(irqstr_addr + IRQSTEER_CHnMASK(IRQ_TO_MASK_OFFSET(dma_Int + 32)),
			1 << IRQ_TO_MASK_SHIFT(dma_Int + 32),
			0);
#endif
}

#if 0
void irqstr_handler(struct dsp_main_struct *hifi_config) {
	u32 status;
	void *sai_addr = hifi_config->tx.sai_addr;
	void *edma_addr_tx = hifi_config->tx.edma_addr;
	void *edma_addr_rx = hifi_config->rx.edma_addr;
	void *irqstr_addr = hifi_config->irqstr_addr;

	status = read32(irqstr_addr + IRQSTEER_CHnSTATUS(IRQ_TO_MASK_OFFSET(314+32)));

	if (status &  (1 << IRQ_TO_MASK_SHIFT(314+32)))
		return sai_irq_handler(sai_addr);

	if (status &  (1 << IRQ_TO_MASK_SHIFT(315+32))) {
		edma_irq_handler(edma_addr_tx);
		edma_irq_handler(edma_addr_rx);
	}

	return;
}
#endif

void irqstr_dump(volatile void *irqstr_addr) {
	int i;

	LOG1("IRQSTEER_CHnCTL %x\n", read32(irqstr_addr + IRQSTEER_CHnCTL));
	for (i = 0; i < 16; i ++)
		LOG1("IRQSTEER_CHnMASK %x\n", read32(irqstr_addr + IRQSTEER_CHnMASK(i*4)));

	for (i = 0; i < 16; i ++)
		LOG1("IRQSTEER_CHnSTATUS %x\n", read32(irqstr_addr + IRQSTEER_CHnSTATUS(i*4)));

	return;
}
