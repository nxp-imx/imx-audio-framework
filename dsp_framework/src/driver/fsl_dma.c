// Copyright 2022 NXP
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>

#include "board.h"

#include "fsl_dma.h"

#include "sdma.h"

void dma_probe(struct dsp_main_struct *dsp)
{
	void *dma = NULL;
	unsigned int board_type = BOARD_TYPE;

	if (dsp->dma_device)
		return;

	if (board_type == DSP_IMX8MP_TYPE)
		dma = (void *)sdma_probe();
	else
		dma = (void *)edma_probe();

	dsp->dma_device = dma;
}

int get_dma_para(void *p_dma, dmac_t *p_dma_ch, int para, void *vaule)
{
	dma_t *dma = (dma_t *)p_dma;
	if (!dma)
		return -EINVAL;
	return dma->get_para(dma, p_dma_ch, para, vaule);
}

int dma_init(void *p_dma)
{
	dma_t *dma = (dma_t *)p_dma;
	if (!dma)
		return -EINVAL;
	return dma->init(dma);
}

void dma_release(void *p_dma)
{
	dma_t *dma = (dma_t *)p_dma;
	if (!dma)
		return;
	dma->release(dma);
}

int dma_irq_handler(void *p_dma)
{
	dma_t *dma = (dma_t *)p_dma;
	if (!dma)
		return -EINVAL;
	return dma->irq_handler(dma);
}

int dma_suspend(void *p_dma)
{
	dma_t *dma = (dma_t *)p_dma;
	if (!dma)
		return -EINVAL;
	return dma->suspend(dma);
}

int dma_resume(void *p_dma)
{
	dma_t *dma = (dma_t *)p_dma;
	if (!dma)
		return -EINVAL;
	return dma->resume(dma);
}

dmac_t *request_dma_chan(void *p_dma, int dev_type)
{
	dmac_t *dma_ch = NULL;
	dma_t *dma = (dma_t *)p_dma;
	if (!dma)
		return NULL;
	dma_ch = (dmac_t *)dma->request_dma_chan(dma, dev_type);
	return dma_ch;
}

void release_dma_chan(dmac_t *p_dma_ch)
{
	if (!p_dma_ch)
		return;
	dma_t *dma = (dma_t *)p_dma_ch->dma_device;
	if (!dma)
		return;
	dma->release_dma_chan(p_dma_ch);
}

int dma_chan_start(dmac_t *p_dma_ch)
{
	if (!p_dma_ch)
		return -EINVAL;
	dma_t *dma = (dma_t *)p_dma_ch->dma_device;
	if (!dma)
		return -EINVAL;
	return dma->chan_start(p_dma_ch);
}

int dma_chan_stop(dmac_t *p_dma_ch)
{
	if (!p_dma_ch)
		return -EINVAL;
	dma_t *dma = (dma_t *)p_dma_ch->dma_device;
	if (!dma)
		return -EINVAL;
	return dma->chan_stop(p_dma_ch);
}

int dma_chan_config(dmac_t *p_dma_ch, dmac_cfg_t *p_config)
{
	if (!p_dma_ch || !p_config)
		return -EINVAL;
	dma_t *dma = (dma_t *)p_dma_ch->dma_device;
	if (!dma)
		return -EINVAL;
	return dma->chan_config(p_dma_ch, p_config);
}

