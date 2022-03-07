/*
 * Copyright 2018-2020 NXP
 */

#ifndef _DSP_RENDER_INTERFACE_H_
#define _DSP_RENDER_INTERFACE_H_

#include "board.h"

#ifdef PLATF_8M

#define DEV_ADDR			SAI_MP_ADDR
#define DEV_INT				SAI_MP_INT_NUM
#define DEV_FIFO_OFF			FSL_SAI_TDR0

#define DMA_INT				-1
#define DMA_ADDR			SDMA3_ADDR

#define FE_DMA_INT			SDMA3_INT_NUM
#define FE_DMA_ADDR			NULL
#define FE_DEV_INT			SAI_MP_INT_NUM
#define FE_DEV_ADDR			EASRC_MP_ADDR
#define FE_DEV_FIFO_IN_OFF		REG_EASRC_WRFIFO(0)
#define FE_DEV_FIFO_OUT_OFF		REG_EASRC_RDFIFO(0)

#define IRQ_STR_ADDR			IRQSTR_MP_ADDR

#else /* PLATF_8M */
#ifdef PLATF_8ULP

#define DEV_ADDR			-1
#define DEV_INT				-1
#define DEV_FIFO_OFF			-1

#define DMA_INT				-1
#define DMA_ADDR			-1

#define FE_DMA_INT			-1
#define FE_DMA_ADDR			-1
#define FE_DEV_INT			-1
#define FE_DEV_ADDR			-1
#define FE_DEV_FIFO_IN_OFF		-1
#define FE_DEV_FIFO_OUT_OFF		-1

#define IRQ_STR_ADDR			-1

#else /* !PLATF_8ULP && ! PLATF_8M  (8QXP || 8QM)*/

#define DEV_ADDR			ESAI_ADDR
#define DEV_INT				ESAI_INT_NUM
#define DEV_FIFO_OFF			REG_ESAI_ETDR

#define DMA_INT				EDMA_ESAI_INT_NUM
#define DMA_ADDR			EDMA_ADDR_ESAI_TX

#define FE_DMA_INT			EDMA_ASRC_INT_NUM
#define FE_DMA_ADDR			EDMA_ADDR_ASRC_RXA
#define FE_DEV_INT			ASRC_INT_NUM
#define FE_DEV_ADDR			ASRC_ADDR
#define FE_DEV_FIFO_IN_OFF		REG_ASRDIA
#define FE_DEV_FIFO_OUT_OFF		REG_ASRDOA

#if PLATF_8QXP
#define IRQ_STR_ADDR			IRQSTR_QXP_ADDR
#else
#define IRQ_STR_ADDR			IRQSTR_QM_ADDR
#endif

#endif
#endif /* END OF PLATF_8M */

#include "sai.h"
#include "esai.h"
#include "asrc.h"
#include "easrc.h"
#include "irqstr.h"
#include "wrap_dma.h"
#include "wm8960.h"

#endif
