/*
 * Copyright 2020 NXP
 *
 * This file wrap different dma, such as edma, sdma
 * and so on */
#include "wrap_dma.h"

int dmabuf_malloc(dma_t *dma, int size)
{
	if (!dma)
		return -1;
	xaf_malloc(&dma->p_dma_buf, size, 0);
	if (!dma->p_dma_buf)
		return -1;
	memset(dma->p_dma_buf, 0, size);
	return 0;
}

void *dmabuf_get(dma_t *dma)
{
	if (!dma || !dma->p_dma_buf)
		return NULL;
	return dma->p_dma_buf;
}

void dma_init(dma_t *dma)
{
	dma_config_t *dma_cfg = dma->dma_cfg;
#ifdef PLATF_8M
	sdma_pre_init(dma->p_dev, dma->p_dev_addr);

	sdma_init(dma->p_dev, DMA_MEM_TO_DEV, (void *)((UWORD32)dma_cfg->p_fe_dev_addr + dma_cfg->fe_dev_fifo_in_off), dma->p_dma_buf, 1,
			dma_cfg->period_len);
	sdma_init(dma->p_dev, DMA_DEV_TO_DEV, (void *)((UWORD32)dma_cfg->p_dev_addr + dma_cfg->dev_fifo_off),
			(void *)((UWORD32)dma_cfg->p_fe_dev_addr + dma_cfg->fe_dev_fifo_out_off),
			2, dma_cfg->period_len);
#else
	edma_init(dma_cfg->p_fe_dma_addr, DMA_MEM_TO_DEV, dma_cfg->p_fe_tcd_aligned,
		  dma_cfg->p_fe_dev_addr + dma_cfg->fe_dev_fifo_in_off, 0, dma->p_dma_buf,
		  dma_cfg->period_len,
		  2);
	edma_init(dma->p_dev_addr, DMA_DEV_TO_DEV, dma->p_tcd_aligned,
		  (UWORD32)dma_cfg->p_dev_addr + dma_cfg->dev_fifo_off,
		  dma_cfg->p_fe_dev_addr + dma_cfg->fe_dev_fifo_out_off, 0,
		  dma_cfg->period_len,
		  2);
#endif

}
void dma_start(dma_t *dma)
{
	dma_config_t *dma_cfg = dma->dma_cfg;
#ifdef PLATF_8M
	sdma_start(dma->p_dev, 1);
	sdma_start(dma->p_dev, 2);
#else
	edma_start(dma_cfg->p_fe_dma_addr, 0);
	edma_start(dma->p_dev_addr, 0);
#endif
}
void dma_stop(dma_t *dma)
{
	dma_config_t *dma_cfg = dma->dma_cfg;
#ifdef PLATF_8M
	sdma_stop(dma->p_dev, 1);
	sdma_stop(dma->p_dev, 2);
#else
	edma_stop(dma_cfg->p_dev_addr);
	edma_stop(dma_cfg->p_fe_dma_addr);
#endif
}
int dma_irq_handler(dma_t *dma)
{
	dma_config_t *dma_cfg = dma->dma_cfg;
#ifdef PLATF_8M
	return sdma_irq_handler(dma->p_dev, dma_cfg->period_len);
#else
	return edma_irq_handler(dma_cfg->p_fe_dma_addr);
#endif
}
void dma_suspend(dma_t *dma)
{
	dma_config_t *dma_cfg = dma->dma_cfg;
#ifdef PLATF_8M
	sdma_suspend(dma->p_dev);
#else
	edma_suspend(dma->p_dev_addr, dma->cache);
	edma_suspend(dma_cfg->p_fe_dma_addr, dma_cfg->fe_dma_cache);
#endif
}
void dma_resume(dma_t *dma)
{
	dma_config_t *dma_cfg = dma->dma_cfg;
#ifdef PLATF_8M
	sdma_resume(dma->p_dev);
#else
	edma_resume(dma->p_dev_addr, dma->cache);
	edma_resume(dma_cfg->p_fe_dma_addr, dma_cfg->fe_dma_cache);
#endif
}
void dma_config(dma_t *dma)
{
	dma_config_t *dma_cfg = dma->dma_cfg;
#ifdef PLATF_8M
#else
	edma_set_tcd(dma_cfg->p_fe_dma_addr, dma_cfg->p_fe_tcd_aligned);
	edma_set_tcd(dma->p_dev_addr, dma->p_tcd_aligned);
#endif
}
void dma_clearup(dma_t *dma)
{
#ifdef PLATF_8M
	if (dma->p_dev)
		sdma_clearup(dma->p_dev);
#endif
	return;
}
