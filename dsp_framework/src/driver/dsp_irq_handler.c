// Copyright 2022 NXP
// SPDX-License-Identifier: BSD-3-Clause

#include "mydefs.h"
#include "io.h"
#include "hardware.h"

#include "debug.h"

void xa_hw_comp_isr()
{
	struct dsp_main_struct *dsp;
	s32     avail;
	u32     status;
	u32     num;

	if (IRQ_STR_ADDR == 0)
		return;
	dsp = get_main_struct();

	/* process hw interrupt if dev exist */
	if (SAI_INT != 0) {
		status = read32((void *)IRQ_STR_ADDR + IRQSTEER_CHnSTATUS(IRQ_TO_MASK_OFFSET(SAI_INT + 32)));
		if (status &  (1 << IRQ_TO_MASK_SHIFT(SAI_INT + 32)))
			sai_irq_handler((void *)SAI_ADDR);
	}

	if (ESAI_INT != 0) {
		status = read32((void *)IRQ_STR_ADDR + IRQSTEER_CHnSTATUS(IRQ_TO_MASK_OFFSET(ESAI_INT + 32)));
		if (status &  (1 << IRQ_TO_MASK_SHIFT(ESAI_INT + 32)))
			esai_irq_handler((void *)ESAI_ADDR);
	}

	if (ASRC_INT != 0) {
		status = read32((void *)IRQ_STR_ADDR + IRQSTEER_CHnSTATUS(IRQ_TO_MASK_OFFSET(ASRC_INT + 32)));
		if (status &  (1 << IRQ_TO_MASK_SHIFT(ASRC_INT + 32)))
			asrc_irq_handler((void *)ASRC_ADDR);
	}

	if (EASRC_INT != 0) {
		status = read32((void *)IRQ_STR_ADDR + IRQSTEER_CHnSTATUS(IRQ_TO_MASK_OFFSET(EASRC_INT + 32)));
		if (status &  (1 << IRQ_TO_MASK_SHIFT(EASRC_INT + 32)))
			easrc_irq_handler((void *)EASRC_ADDR);
	}

	if (MICFIL_INT != 0) {
		void *micfil = dsp->micfil;
		status = read32((void *)IRQ_STR_ADDR + IRQSTEER_CHnSTATUS(IRQ_TO_MASK_OFFSET(MICFIL_INT + 32)));
		if (status &  (1 << IRQ_TO_MASK_SHIFT(MICFIL_INT + 32)))
			micfil_irq_handler(micfil);
	}

	if (SDMA_INT != 0) {
		dma_t *dma = dsp->dma_device;
		status = read32((void *)IRQ_STR_ADDR + IRQSTEER_CHnSTATUS(IRQ_TO_MASK_OFFSET(SDMA_INT + 32)));
		if (status &  (1 << IRQ_TO_MASK_SHIFT(SDMA_INT + 32)))
			dma_irq_handler(dma);
	}

	if (EDMA_ESAI_INT_NUM != 0) {
		dma_t *dma = dsp->dma_device;
		status = read32((void *)IRQ_STR_ADDR + IRQSTEER_CHnSTATUS(IRQ_TO_MASK_OFFSET(EDMA_ESAI_INT_NUM + 32)));
		if (status &  (1 << IRQ_TO_MASK_SHIFT(EDMA_ESAI_INT_NUM + 32)))
			dma_irq_handler(dma);
	}
	if (EDMA_ASRC_INT_NUM != 0) {
		dma_t *dma = dsp->dma_device;
		status = read32((void *)IRQ_STR_ADDR + IRQSTEER_CHnSTATUS(IRQ_TO_MASK_OFFSET(EDMA_ASRC_INT_NUM + 32)));
		if (status &  (1 << IRQ_TO_MASK_SHIFT(EDMA_ASRC_INT_NUM + 32)))
			dma_irq_handler(dma);
	}
}

