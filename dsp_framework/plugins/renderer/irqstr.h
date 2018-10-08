/*
 * irqstr.h - IRQ steer header
 *
 * Copyright 2018 NXP
 */
#ifndef _IRQSTR_H
#define _IRQSTR_H

#define IRQSTEER_CHnCTL       0x0
#define IRQSTEER_CHnCTL_CH(n) BIT(n)
#define IRQSTEER_CHnMASK(n)   ((n) + 4)
#define IRQ_TO_MASK_OFFSET(n)  ((15 - ((n) / 32)) * 4)
#define IRQ_TO_MASK_SHIFT(n)   ((n) % 32)
#define IRQSTEER_CHnSTATUS(n)   ((n) + 0x84)

void irqstr_init(volatile void * irqstr_addr, int dev_Int, int dma_Int);
void irqstr_start(volatile void * irqstr_addr);
void irqstr_stop(volatile void * irqstr_addr);

#if 0
void irqstr_handler(struct dsp_main_struct  * hifi_config);
#endif

void irqstr_dump(volatile void * irqstr_addr);

#endif /* _IRQSTR_H */
