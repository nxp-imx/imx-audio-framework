/*
 * Copyright 2020 NXP
 */

#ifndef   _WRAP_DMA_H
#define   _WRAP_DMA_H

#include "mydefs.h"
#include "xf-debug.h"
#include "edma.h"
#include "sdma.h"

typedef enum {
	SDMA = 0,
	EDMA,
} dma_type_t;

typedef struct {
	/* fe dev */
	void           *p_fe_dev;
	void           *p_fe_dev_addr;
	void           *p_fe_dma_addr;
	void           *p_fe_dma_buffer;
	UWORD32        fe_dma_cache[40];
	UWORD32        fe_dev_dma_int;
	void           *p_fe_tcd;
	void           *p_fe_tcd_aligned;
	UWORD32        fe_dev_fifo_off;
	UWORD32        fe_dev_fifo_in_off;
	UWORD32        fe_dev_fifo_out_off;
	/* dev */
	void           *p_dev;
	void           *p_dev_addr;
	void           *p_dma_addr;
	void           *p_dma_buffer;
	UWORD32        dma_cache[40];
	UWORD32        dev_dma_int;
	void           *p_tcd;
	void           *p_tcd_aligned;
	UWORD32        dev_fifo_off;
	UWORD32        dev_fifo_in_off;
	UWORD32        dev_fifo_out_off;

	WORD32         period_len;
} dma_config_t;

typedef struct {
	void           *p_dev;
	/* dev phy addr */
	void           *p_dev_addr;
	/* ping-pong buffer */
	void           *p_dma_buf;
	dma_type_t     type;
	UWORD32        cache[40];
	UWORD32        dev_int;
	void           *p_tcd;
	void           *p_tcd_aligned;

	dma_config_t   *dma_cfg;
} dma_t;

int  dmabuf_malloc(dma_t *dma, int size);
void *dmabuf_get(dma_t *dma);
void dma_init(dma_t *dma);
void dma_start(dma_t *dma);
void dma_stop(dma_t *dma);
int  dma_irq_handler(dma_t *dma);
void dma_suspend(dma_t *dma);
void dma_resume(dma_t *dma);
void dma_config(dma_t *dma);
void dma_clearup(dma_t *dma);

#endif
