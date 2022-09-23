/*
 * Copyright 2022 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef   _FSL_DMA_H
#define   _FSL_DMA_H

#include <stddef.h>

#include "mydefs.h"

typedef enum {
	DMATYPE_SDMA = 0,
	DMATYPE_EDMA,
} dma_type_t;

typedef enum {
	DMA_MEM_TO_DEV = 0,
	DMA_DEV_TO_MEM = 1,
	DMA_DEV_TO_DEV = 2,
} dma_trans_dir_t;

typedef enum {
	GET_IRQ = 0,
} dma_para_t;

/* edma special */
typedef enum {
	DEV_IGNORE = 0,
	EDMA_ESAI_RX = 1,
	EDMA_ESAI_TX,
	EDMA_ASRC_RX,
	EDMA_ASRC_TX,
} edma_dev_type_t;

/* sdma special cfg */
struct sdma_audio_config {
	unsigned int src_fifo_num;
	unsigned int dst_fifo_num;
	unsigned int src_fifo_off;
	unsigned int dst_fifo_off;
	unsigned int words_per_fifo;
	unsigned int sw_done_sel;
};

typedef struct {
	unsigned int                    events[2];
	unsigned int                    watermark;
	struct sdma_audio_config        trans_cfg;
} sdmac_cfg_t;

typedef struct {
	dma_trans_dir_t  direction;
	void             *src_addr, *dest_addr;
	int              src_maxbrust, dest_maxbrust;
	int              src_width, dest_width;
	int              period_len;
	int              period_count;
	void             (*callback)(void *args);
	void             *comp;
	void             *peripheral_config;
	size_t           peripheral_size;
} dmac_cfg_t;

typedef struct {
	void             *dma_device;
	dma_trans_dir_t  direction;
	void             *src_addr, *dest_addr;
	int              src_maxbrust, dest_maxbrust;
	int              src_width, dest_width;
	int              period_len;
	int              period_count;
	void             (*callback)(void *args);
	void             *comp;
} dmac_t;

typedef struct dma_device {
	dma_type_t       dma_type;
	/* dma ops */
	int              (*init)(struct dma_device *dev);
	void             (*release)(struct dma_device *dev);
	int              (*irq_handler)(struct dma_device *dev);
	int              (*suspend)(struct dma_device *dev);
	int              (*resume)(struct dma_device *dev);
	int              (*get_para)(struct dma_device *dev, dmac_t *dmac, int para, void *value);

	dmac_t           *(*request_dma_chan)(struct dma_device *dev, int dev_type);
	void             (*release_dma_chan)(dmac_t *dma_chan);
	int              (*chan_config)(dmac_t *dma_chan, dmac_cfg_t *dma_trans_config);
	int              (*chan_start)(dmac_t *dma_chan);
	int              (*chan_stop)(dmac_t *dma_chan);
} dma_t;

void dma_probe(struct dsp_main_struct *dsp);

int dma_init(void *p_dma);
void dma_release(void *p_dma);
int dma_irq_handler(void *p_dma);
int dma_suspend(void *p_dma);
int dma_resume(void *p_dma);
int get_dma_para(void *p_dma, dmac_t *dma_ch, int para, void *vaule);

dmac_t *request_dma_chan(void *p_dma, int dev_type);
void release_dma_chan(dmac_t *p_dma_ch);
int dma_chan_start(dmac_t *p_dma_ch);
int dma_chan_stop(dmac_t *p_dma_ch);
int dma_chan_config(dmac_t *p_dma_ch, dmac_cfg_t *p_config);

#endif
