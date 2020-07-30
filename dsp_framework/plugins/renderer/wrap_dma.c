/*
 * Copyright 2020 NXP
 *
 * This file wrap different xdma, such as edma, sdma
 * and so on */
#include "wrap_dma.h"
void xdma_init(struct XADevRenderer *d)
{
#ifdef PLATF_8M
	sdma_pre_init(d->sdma, d->sdma_addr);

	sdma_init(d->sdma, DMA_MEM_TO_DEV, d->fe_dev_fifo_in_off, d->dma_buf, 1,
			d->frame_size * d->sample_size);
	sdma_init(d->sdma, DMA_DEV_TO_DEV, d->dev_addr + d->dev_fifo_off, d->fe_dev_fifo_out_off,
			2, d->frame_size * d->sample_size);
#else
	edma_init(d->fe_edma_addr, DMA_MEM_TO_DEV, d->fe_tcd_align32,
		  d->fe_dev_addr + d->fe_dev_fifo_in_off, 0, d->dma_buf,
		  d->frame_size * d->sample_size,
		  2);
	edma_init(d->edma_addr, DMA_DEV_TO_DEV, d->tcd_align32,
		  d->dev_addr + d->dev_fifo_off,
		  d->fe_dev_addr + d->fe_dev_fifo_out_off, 0,
		  d->frame_size * d->sample_size,
		  2);
#endif

}
void xdma_start(struct XADevRenderer *d)
{
#ifdef PLATF_8M
	sdma_start(d->sdma, 1);
	sdma_start(d->sdma, 2);
#else
	edma_start(d->fe_edma_addr, 0);
	edma_start(d->edma_addr, 0);
#endif
}
void xdma_stop(struct XADevRenderer *d)
{
#ifdef PLATF_8M
	sdma_stop(d->sdma, 1);
	sdma_stop(d->sdma, 2);
#else
	edma_stop(d->edma_addr);
	edma_stop(d->fe_edma_addr);
#endif
}
void xdma_irq_handler(struct XADevRenderer *d)
{
#ifdef PLATF_8M
	sdma_irq_handler(d->sdma, d->sample_size * d->frame_size);
#else
	edma_irq_handler(d->fe_edma_addr);
#endif
}
void xdma_suspend(struct XADevRenderer *d)
{
#ifdef PLATF_8M
	sdma_suspend(d->sdma);
#else
	edma_suspend(d->edma_addr, d->edma_cache);
	edma_suspend(d->fe_edma_addr, d->fe_edma_cache);
#endif
}
void xdma_resume(struct XADevRenderer *d)
{
#ifdef PLATF_8M
	sdma_resume(d->sdma);
#else
	edma_resume(d->edma_addr, d->edma_cache);
	edma_resume(d->fe_edma_addr, d->fe_edma_cache);
#endif
}
void xdma_config(struct XADevRenderer *d)
{
#ifdef PLATF_8M
#else
	edma_set_tcd(d->fe_edma_addr, d->fe_tcd_align32);
	edma_set_tcd(d->edma_addr, d->tcd_align32);
#endif
}
void xdma_clearup(struct XADevRenderer *d)
{
#ifdef PLAFT_8M
	sdma_clearup(d->sdma);
#endif
	return;
}
