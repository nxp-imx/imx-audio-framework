/*
 * Copyright 2018-2020 NXP
 *
 * edma.c - EDMA driver
 */
#include "fsl_dma.h"

#include "board.h"
#include "mydefs.h"

#include "debug.h"

#define EDMA_CH_CSR                     0x00
#define EDMA_CH_ES                      0x04
#define EDMA_CH_INT                     0x08
#define EDMA_CH_SBR                     0x0C
#define EDMA_CH_PRI                     0x10
#define EDMA_TCD_SADDR                  0x20
#define EDMA_TCD_SOFF                   0x24
#define EDMA_TCD_ATTR                   0x26
#define EDMA_TCD_NBYTES                 0x28
#define EDMA_TCD_SLAST                  0x2C
#define EDMA_TCD_DADDR                  0x30
#define EDMA_TCD_DOFF                   0x34
#define EDMA_TCD_CITER_ELINK            0x36
#define EDMA_TCD_CITER                  0x36
#define EDMA_TCD_DLAST_SGA              0x38
#define EDMA_TCD_CSR                    0x3C
#define EDMA_TCD_BITER_ELINK            0x3E
#define EDMA_TCD_BITER                  0x3E

#define EDMA_CH_SBR_RD                  BIT(22)
#define EDMA_CH_SBR_WR                  BIT(21)
#define EDMA_CH_CSR_ERQ                 BIT(0)
#define EDMA_CH_CSR_EARQ                BIT(1)
#define EDMA_CH_CSR_EEI                 BIT(2)
#define EDMA_CH_CSR_DONE                BIT(30)
#define EDMA_CH_CSR_ACTIVE              BIT(31)

#define EDMA_TCD_ATTR_DSIZE(x)          (((x) & 0x0007))
#define EDMA_TCD_ATTR_DMOD(x)           (((x) & 0x001F) << 3)
#define EDMA_TCD_ATTR_SSIZE(x)          (((x) & 0x0007) << 8)
#define EDMA_TCD_ATTR_SMOD(x)           (((x) & 0x001F) << 11)
#define EDMA_TCD_ATTR_SSIZE_8BIT        (0x0000)
#define EDMA_TCD_ATTR_SSIZE_16BIT       (0x0100)
#define EDMA_TCD_ATTR_SSIZE_32BIT       (0x0200)
#define EDMA_TCD_ATTR_SSIZE_64BIT       (0x0300)
#define EDMA_TCD_ATTR_SSIZE_16BYTE      (0x0400)
#define EDMA_TCD_ATTR_SSIZE_32BYTE      (0x0500)
#define EDMA_TCD_ATTR_SSIZE_64BYTE      (0x0600)
#define EDMA_TCD_ATTR_DSIZE_8BIT        (0x0000)
#define EDMA_TCD_ATTR_DSIZE_16BIT       (0x0001)
#define EDMA_TCD_ATTR_DSIZE_32BIT       (0x0002)
#define EDMA_TCD_ATTR_DSIZE_64BIT       (0x0003)
#define EDMA_TCD_ATTR_DSIZE_16BYTE      (0x0004)
#define EDMA_TCD_ATTR_DSIZE_32BYTE      (0x0005)
#define EDMA_TCD_ATTR_DSIZE_64BYTE      (0x0006)

#define EDMA_TCD_SOFF_SOFF(x)           (x)
#define EDMA_TCD_NBYTES_NBYTES(x)       (x)
#define EDMA_TCD_NBYTES_MLOFF(x)        (x << 10)
#define EDMA_TCD_NBYTES_DMLOE           (1 << 30)
#define EDMA_TCD_NBYTES_SMLOE           (1 << 31)
#define EDMA_TCD_SLAST_SLAST(x)         (x)
#define EDMA_TCD_DADDR_DADDR(x)         (x)
#define EDMA_TCD_CITER_CITER(x)         ((x) & 0x7FFF)
#define EDMA_TCD_DOFF_DOFF(x)           (x)
#define EDMA_TCD_DLAST_SGA_DLAST_SGA(x) (x)
#define EDMA_TCD_BITER_BITER(x)         ((x) & 0x7FFF)

#define EDMA_TCD_CSR_START              BIT(0)
#define EDMA_TCD_CSR_INT_MAJOR          BIT(1)
#define EDMA_TCD_CSR_INT_HALF           BIT(2)
#define EDMA_TCD_CSR_D_REQ              BIT(3)
#define EDMA_TCD_CSR_E_SG               BIT(4)
#define EDMA_TCD_CSR_E_LINK             BIT(5)
#define EDMA_TCD_CSR_ACTIVE             BIT(6)
#define EDMA_TCD_CSR_DONE               BIT(7)

#define MAX_PERIOD_COUNT  8

#define MAX_EDMA_CHANNELS 32

typedef struct nxp_edma_hw_tcd {
	u32  saddr;
	u16  soff;
	u16  attr;
	u32  nbytes;
	u32  slast;
	u32  daddr;
	u16  doff;
	u16  citer;
	u32  dlast_sga;
	u16  csr;
	u16  biter;
}__attribute__ ((aligned(32))) tcd_t;

typedef struct {
	dmac_t              dmac;
	int                 chan_id;
	int                 busy;
	tcd_t               *tcd;
	tcd_t               *tcd_align32;

	void                *membase;
	int                 Int;
	/* fixed in probe state */
	edma_dev_type_t     dev;
	unsigned int        cache[16];
} edmac_t;

typedef struct {
	dma_t               dma;
	edmac_t             edmac[MAX_EDMA_CHANNELS];
	int                 init_done;
} edma_t;

typedef struct {
	void                *membase;
	int                 Int;
	int                 dev;
} edma_chcfg_t;

/* only define dsp used */
static const edma_chcfg_t edmac_cfg[] = {
	{ (void *)EDMA_ADDR_ESAI_TX,  EDMA_ESAI_INT_NUM, EDMA_ESAI_TX },
	{ (void *)EDMA_ADDR_ESAI_RX,  EDMA_ESAI_INT_NUM, EDMA_ESAI_RX },
	{ (void *)EDMA_ADDR_ASRC_RXA, EDMA_ASRC_INT_NUM, EDMA_ASRC_RX },
	{ (void *)EDMA_ADDR_ASRC_TXA, EDMA_ASRC_INT_NUM, EDMA_ASRC_TX },
};

void edma_fill_tcd(struct nxp_edma_hw_tcd *tcd, u32 src_addr, u32 dst_addr,
		   u16 attr, u16 soff, u32 nbytes, u32 slast, u16 citer,
		   u16 biter, u16 doff, u32 dlast_sga, u32 major_int,
		   u32 disable_req, u32 enable_sg, u32 is_rx) {
	u16 csr = 0;

	tcd->saddr = src_addr;
	tcd->daddr = dst_addr;

	tcd->attr = attr;
	tcd->soff = soff;

	tcd->nbytes = nbytes;
	tcd->slast  = slast;

	tcd->citer  = citer;
	tcd->doff   = doff;

	tcd->dlast_sga = dlast_sga;
	tcd->biter     = biter;

	if (major_int)
		csr |= EDMA_TCD_CSR_INT_MAJOR;

	if (disable_req)
		csr |= EDMA_TCD_CSR_D_REQ;

	if (enable_sg)
		csr |= EDMA_TCD_CSR_E_SG;

	if (is_rx)
		csr |= EDMA_TCD_CSR_ACTIVE;

	tcd->csr = csr;
}

void edma_set_tcd(volatile void * edma_addr, struct nxp_edma_hw_tcd *tcd) {

	write32(edma_addr + EDMA_TCD_CSR,  0);
	write32(edma_addr + EDMA_TCD_SADDR, tcd->saddr);
	write32(edma_addr + EDMA_TCD_DADDR, tcd->daddr);
	write16(edma_addr + EDMA_TCD_ATTR, tcd->attr);
	write16(edma_addr + EDMA_TCD_SOFF, tcd->soff);
	write32(edma_addr + EDMA_TCD_NBYTES, tcd->nbytes);
	write32(edma_addr + EDMA_TCD_SLAST, tcd->slast);
	write16(edma_addr + EDMA_TCD_CITER, tcd->citer);
	write16(edma_addr + EDMA_TCD_BITER, tcd->biter);
	write16(edma_addr + EDMA_TCD_DOFF, tcd->doff);

	write32(edma_addr + EDMA_TCD_DLAST_SGA, tcd->dlast_sga);
	
	if ((EDMA_TCD_CSR_E_SG | tcd->csr) &&
		(EDMA_CH_CSR_DONE | read32(edma_addr + EDMA_CH_CSR)))
		write32(edma_addr + EDMA_CH_CSR, EDMA_CH_CSR_DONE);

	write16(edma_addr + EDMA_TCD_CSR, tcd->csr);
}

void edma_init_tcd(struct nxp_edma_hw_tcd *tcd, u32 type,
	       volatile void *dev_addr,
	       volatile void *dev2_addr,
	       volatile void *dma_buf_addr,
	       u32 period_size, u32 period_count) {
	u32 dma_addr = (u32)dma_buf_addr;
	u32 period_len = period_size;
	u32 buf_len = period_len * period_count;
	u32 dma_addr_next = dma_addr;
	u32 src_addr, dst_addr;
	u32 soff, doff, nbytes, attr, iter, last_sg;
	u32 burst=4, addr_width=2;
	int i;

	nbytes = burst * addr_width;
	attr = (EDMA_TCD_ATTR_SSIZE_16BIT | EDMA_TCD_ATTR_DSIZE_16BIT);
	iter = period_len / nbytes;

	for ( i = 0; i < period_count; i ++ ) {
		if (dma_addr_next >= dma_addr + buf_len)
			dma_addr_next = dma_addr;
		if (type == DMA_MEM_TO_DEV) {
			src_addr = dma_addr_next;
			dst_addr = (u32)dev_addr;
			soff = 2;
			doff = 0;
		} else if (type == DMA_DEV_TO_MEM) {
			src_addr = (u32)dev_addr;
			dst_addr = dma_addr_next;
			soff = 0;
			doff = 2;
		} else {
			/* DMA_DEV_TO_DEV */
			src_addr = (u32)dev2_addr;
			dst_addr = (u32)dev_addr;
			soff = 0;
			doff = 0;
		}

		last_sg = (u32)&tcd[(i+1)%period_count];

		edma_fill_tcd(&tcd[i], src_addr, dst_addr, attr, soff, nbytes,
			      0, iter, iter, doff, last_sg, 1, 0, 1, 0);
		dma_addr_next = dma_addr_next + period_len;
	}
}

static void edma_chan_suspend(volatile void *edma_addr,  u32 *cache_addr)
{
	cache_addr[0] = read32(edma_addr + EDMA_CH_CSR);
	cache_addr[1] = read32(edma_addr + EDMA_CH_ES);
	cache_addr[2] = read32(edma_addr + EDMA_CH_INT);
	cache_addr[3] = read32(edma_addr + EDMA_CH_SBR);
	cache_addr[4] = read32(edma_addr + EDMA_CH_PRI);
	cache_addr[5] = read32(edma_addr + EDMA_TCD_SADDR);
	cache_addr[6] = read16(edma_addr + EDMA_TCD_SOFF);
	cache_addr[7] = read16(edma_addr + EDMA_TCD_ATTR);
	cache_addr[8] = read32(edma_addr + EDMA_TCD_NBYTES);
	cache_addr[9] = read32(edma_addr + EDMA_TCD_SLAST);
	cache_addr[10] = read32(edma_addr + EDMA_TCD_DADDR);
	cache_addr[11] = read16(edma_addr + EDMA_TCD_DOFF);
	cache_addr[12] = read16(edma_addr + EDMA_TCD_CITER);
	cache_addr[13] = read32(edma_addr + EDMA_TCD_DLAST_SGA);
	cache_addr[14] = read16(edma_addr + EDMA_TCD_CSR);
	cache_addr[15] = read16(edma_addr + EDMA_TCD_BITER);
}

static void edma_chan_resume(volatile void *edma_addr,  u32 *cache_addr)
{
	write32(edma_addr + EDMA_CH_CSR,     cache_addr[0]);
	write32(edma_addr + EDMA_CH_SBR,     cache_addr[3]);
	write32(edma_addr + EDMA_TCD_SADDR,  cache_addr[5]);
	write16(edma_addr + EDMA_TCD_SOFF,   cache_addr[6]);
	write16(edma_addr + EDMA_TCD_ATTR,   cache_addr[7]);
	write32(edma_addr + EDMA_TCD_NBYTES, cache_addr[8]);
	write32(edma_addr + EDMA_TCD_SLAST,  cache_addr[9]);
	write32(edma_addr + EDMA_TCD_DADDR,  cache_addr[10]);
	write16(edma_addr + EDMA_TCD_DOFF,   cache_addr[11]);
	write16(edma_addr + EDMA_TCD_CITER,  cache_addr[12]);
	write32(edma_addr + EDMA_TCD_DLAST_SGA,  cache_addr[13]);
	write16(edma_addr + EDMA_TCD_CSR,    cache_addr[14]);
	write16(edma_addr + EDMA_TCD_BITER,  cache_addr[15]);
}

static void edma_chan_dump(volatile void * edma_addr)
{
	LOG1("EDMA_CH_CSR %x\n",  read32(edma_addr + EDMA_CH_CSR));
	LOG1("EDMA_CH_ES %x\n",   read32(edma_addr + EDMA_CH_ES));
	LOG1("EDMA_CH_INT %x\n",  read32(edma_addr + EDMA_CH_INT));
	LOG1("EDMA_CH_SBR %x\n",  read32(edma_addr + EDMA_CH_SBR));
	LOG1("EDMA_CH_PRI %x\n",  read32(edma_addr + EDMA_CH_PRI));

	LOG1("EDMA_TCD_SADDR %x\n",  read32(edma_addr + EDMA_TCD_SADDR));
	LOG1("EDMA_TCD_SOFF %x\n",   read16(edma_addr + EDMA_TCD_SOFF));
	LOG1("EDMA_TCD_ATTR %x\n",   read16(edma_addr + EDMA_TCD_ATTR));
	LOG1("EDMA_TCD_NBYTES %x\n", read32(edma_addr + EDMA_TCD_NBYTES));
	LOG1("EDMA_TCD_SLAST %x\n",  read32(edma_addr + EDMA_TCD_SLAST));

	LOG1("EDMA_TCD_DADDR %x\n", read32(edma_addr + EDMA_TCD_DADDR));
	LOG1("EDMA_TCD_DOFF %x\n",  read16(edma_addr + EDMA_TCD_DOFF));

	LOG1("EDMA_TCD_CITER %x\n",     read16(edma_addr + EDMA_TCD_CITER));
	LOG1("EDMA_TCD_DLAST_SGA %x\n", read32(edma_addr + EDMA_TCD_DLAST_SGA));

	LOG1("EDMA_TCD_CSR %x\n",   read16(edma_addr + EDMA_TCD_CSR));
	LOG1("EDMA_TCD_BITER %x\n", read16(edma_addr + EDMA_TCD_BITER));

	return;
}

int edma_get_para(struct dma_device *dev, dmac_t *dmac, int para, void *value)
{
	edma_t *edma = NULL;
	edmac_t *edmac = NULL;
	if (!dev || !dmac)
		return -EINVAL;
	edma = (edma_t *)dev;
	edmac = (edmac_t *)dmac;

	switch(para) {
	case GET_IRQ:
		*(int *)value = edmac->Int;
		break;
	default:
		*(int *)value = 0;
		break;
	}

	return 0;
}

int edma_init(struct dma_device *dev)
{
	edma_t *edma = NULL;
	if (!dev)
		return -EINVAL;
	edma = (edma_t *)dev;

	if (!edma->init_done)
		edma->init_done = 1;

	return 0;
}

void edma_release(struct dma_device *dev)
{
	edma_t *edma = NULL;
	int idx;
	if (!dev)
		return;

	edma = (edma_t *)dev;

	for (idx = 0; idx < MAX_EDMA_CHANNELS; idx++) {
		edmac_t *edmac = &edma->edmac[idx];
		if (edmac->busy)
			return;
	}

	edma->init_done = 0;

	xaf_free(edma, 0);
	edma = NULL;
}

int edma_irq_handler(struct dma_device *dev)
{
	edma_t *edma = NULL;
	unsigned int intr;
	int idx;
	if (!dev)
		return -EINVAL;

	edma = (edma_t *)dev;

	for (idx = 0; idx < MAX_EDMA_CHANNELS; idx++) {
		edmac_t *edmac = &edma->edmac[idx];
		dmac_t *dmac = (dmac_t *)edmac;
		if (edmac->busy) {
			intr = read32(edmac->membase + EDMA_CH_INT);
			if (!intr)
				continue;
			write32(edmac->membase + EDMA_CH_INT, 1);
			if (dmac->callback)
				dmac->callback(dmac->comp);
		}
	}

	return 0;
}

int edma_suspend(struct dma_device *dev)
{
	edma_t *edma = NULL;
	int idx;
	if (!dev)
		return -EINVAL;

	edma = (edma_t *)dev;

	for (idx = 0; idx < MAX_EDMA_CHANNELS; idx++) {
		edmac_t *edmac = &edma->edmac[idx];
		if (edmac->busy) {
			edma_chan_suspend(edmac->membase, &edmac->cache[0]);
		}
	}

	return 0;
}

int edma_resume(struct dma_device *dev)
{
	edma_t *edma = NULL;
	int idx;
	if (!dev)
		return -EINVAL;

	edma = (edma_t *)dev;

	for (idx = 0; idx < MAX_EDMA_CHANNELS; idx++) {
		edmac_t *edmac = &edma->edmac[idx];
		if (edmac->busy) {
			edma_chan_resume(edmac->membase, &edmac->cache[0]);
		}
	}

	return 0;
}

dmac_t *request_edma_chan(struct dma_device *dev, int dev_type)
{
	edma_t *edma = NULL;
	edmac_t *edmac = NULL;
	int ch_num = 0;
	if (!dev)
		return NULL;

	edma = (edma_t *)dev;

	while (ch_num < MAX_EDMA_CHANNELS) {
		edmac = &edma->edmac[ch_num];
		if (!edmac->busy && (edmac->dev == dev_type)) {
			edmac->busy = 1;
			edmac->dmac.dma_device = dev;
			break;
		}
		ch_num++;
	}
	if (ch_num >= MAX_EDMA_CHANNELS)
		edmac = NULL;
	return (dmac_t *)edmac;
}

void release_edma_chan(dmac_t *dma_chan)
{
	edmac_t *edmac = (edmac_t *)dma_chan;
	if (!edmac)
		return;

	if (edmac->tcd) {
		xaf_free(edmac->tcd, 0);
		edmac->tcd_align32 = NULL;
	}
	memset(edmac, 0, sizeof(edmac_t));
}

int edma_chan_config(dmac_t *dma_chan, dmac_cfg_t *dmac_cfg)
{
	edmac_t *edmac = NULL;
	if (!dma_chan)
		return -EINVAL;

	dma_chan->direction = dmac_cfg->direction;
	dma_chan->src_addr = dmac_cfg->src_addr;
	dma_chan->dest_addr = dmac_cfg->dest_addr;
	dma_chan->src_maxbrust = dmac_cfg->src_maxbrust;
	dma_chan->dest_maxbrust = dmac_cfg->dest_maxbrust;
	dma_chan->src_width = dmac_cfg->src_width;
	dma_chan->dest_width = dmac_cfg->dest_width;
	dma_chan->period_len = dmac_cfg->period_len;
	dma_chan->period_count = dmac_cfg->period_count;
	dma_chan->callback = dmac_cfg->callback;
	dma_chan->comp = dmac_cfg->comp;

	edmac = (edmac_t *)dma_chan;

	if (!edmac->tcd) {
		xaf_malloc((void **)&edmac->tcd, MAX_PERIOD_COUNT * sizeof(tcd_t) + 32, 0);
		edmac->tcd_align32 = (void *)(((u32)edmac->tcd + 31) & ~31);
	}
	if (!edmac->tcd)
		return -ENOMEM;

	if (dma_chan->direction == DMA_DEV_TO_MEM) {
		edma_init_tcd(edmac->tcd_align32, dma_chan->direction, dma_chan->src_addr,
				0, dma_chan->dest_addr, dma_chan->period_len,
				dma_chan->period_count);
	} else if (dma_chan->direction == DMA_MEM_TO_DEV) {
		edma_init_tcd(edmac->tcd_align32, dma_chan->direction, dma_chan->dest_addr,
				0, dma_chan->src_addr, dma_chan->period_len,
				dma_chan->period_count);
	} else {
		/* dev to dev */
		edma_init_tcd(edmac->tcd_align32, dma_chan->direction, dma_chan->dest_addr,
				dma_chan->src_addr, 0, dma_chan->period_len,
				dma_chan->period_count);
	}

	edma_set_tcd(edmac->membase, edmac->tcd_align32);

	return 0;
}

int edma_chan_start(dmac_t *dma_chan)
{
	edmac_t *edmac = NULL;
	u32 val;
	if (!dma_chan)
		return -EINVAL;

	edmac = (edmac_t *)dma_chan;

	val = read32(edmac->membase + EDMA_CH_SBR);

	switch (edmac->dev) {
	case EDMA_ESAI_RX:
	case EDMA_ASRC_RX:
		val |= EDMA_CH_SBR_RD;
		break;
	case EDMA_ESAI_TX:
	case EDMA_ASRC_TX:
		val |= EDMA_CH_SBR_WR;
		break;
	default:
		break;
	}

	write32(edmac->membase + EDMA_CH_SBR, val);

	val = read32(edmac->membase + EDMA_CH_CSR);
	val |= EDMA_CH_CSR_ERQ;
	write32(edmac->membase + EDMA_CH_CSR, val);

	return 0;
}

int edma_chan_stop(dmac_t *dma_chan)
{
	edmac_t *edmac = NULL;
	u32 val;
	if (!dma_chan)
		return -EINVAL;
	edmac = (edmac_t *)dma_chan;

	val = read32(edmac->membase + EDMA_CH_CSR);
	val &= ~EDMA_CH_CSR_ERQ;
	write32(edmac->membase + EDMA_CH_CSR, val);

	return 0;
}

int edma_chan_probe(edma_t *edma)
{
	int idx;
	edmac_t *edmac = NULL;

	for (idx = 0; idx < MAX_EDMA_CHANNELS; idx++) {
		edmac = &edma->edmac[idx];
		edmac->chan_id = idx;
	}

	for (idx = 0; idx < sizeof(edmac_cfg)/sizeof(edma_chcfg_t); idx++) {
		edmac = &edma->edmac[idx];
		edmac->membase = edmac_cfg[idx].membase;
		edmac->Int = edmac_cfg[idx].Int;
		edmac->dev = edmac_cfg[idx].dev;
	}

	return 0;
}

dma_t *edma_probe()
{
	edma_t *fsl_edma = NULL;
	xaf_malloc((void **)&fsl_edma, sizeof(edma_t), 0);
	if (!fsl_edma)
		return NULL;
	memset(fsl_edma, 0, sizeof(edma_t));

	fsl_edma->dma.dma_type     = DMATYPE_EDMA;
	fsl_edma->dma.get_para     = edma_get_para;
	fsl_edma->dma.init         = edma_init;
	fsl_edma->dma.release      = edma_release;
	fsl_edma->dma.irq_handler  = edma_irq_handler;
	fsl_edma->dma.suspend      = edma_suspend;
	fsl_edma->dma.resume       = edma_resume;

	fsl_edma->dma.request_dma_chan = request_edma_chan;
	fsl_edma->dma.release_dma_chan = release_edma_chan;
	fsl_edma->dma.chan_config  = edma_chan_config;
	fsl_edma->dma.chan_start   = edma_chan_start;
	fsl_edma->dma.chan_stop    = edma_chan_stop;

	edma_chan_probe(fsl_edma);

	return (dma_t *)fsl_edma;
}
