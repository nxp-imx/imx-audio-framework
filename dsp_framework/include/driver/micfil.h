/*
 * Copyright 2022 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MICFIL_H
#define MICFIL_H

#include "mydefs.h"
#include "fsl_unia.h"

#define ERROR (-1)
#define OK     0

enum micfil_quality {
	HIGH_QUALITY,
	MEDIUM_QUALITY,
	LOW_QUALITY,
	VLOW0_QUALITY,
	VLOW1_QUALITY,
	VLOW2_QUALITY
};

void micfil_probe(struct dsp_main_struct *dsp);

int micfil_get_dma_event_id(void *p_micfil);
int micfil_get_irqnum(void *p_micfil);
void *micfil_get_datach0_addr(void *p_micfil);

int micfil_init(void *p_micfil);
int micfil_release(void *p_micfil);
int micfil_set_param(void *p_micfil, int i_idx, unsigned int *p_vaule);
int micfil_get_param(void *p_micfil, int i_idx, unsigned int *p_vaule);
int micfil_start(void *p_micfil);
int micfil_stop(void *p_micfil);
void micfil_irq_handler(void *p_micfil);
void micfil_suspend(void *p_micfil);
void micfil_resume(void *p_micfil);

void micfil_dump(void *p_micfil);

#endif
