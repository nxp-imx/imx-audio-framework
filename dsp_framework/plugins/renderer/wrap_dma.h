#ifndef   _WRAP_DMA_H
#define   _WRAP_DMA_H

#include "mydefs.h"
#include "xf-debug.h"
#include "edma.h"
#include "sdma.h"
#include "dsp_renderer_interface.h"

void xdma_init(struct XADevRenderer *d);
void xdma_start(struct XADevRenderer *d);
void xdma_stop(struct XADevRenderer *d);
void xdma_irq_handler(struct XADevRenderer *d);
void xdma_suspend(struct XADevRenderer *d);
void xdma_resume(struct XADevRenderer *d);
void xdma_config(struct XADevRenderer *d);
void xdma_clearup(struct XADevRenderer *d);
#endif
