/*
 * edma.c - EDMA driver
 */
#include "edma.h"

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

void edma_init(volatile void *edma_addr, u32 type,
	       struct nxp_edma_hw_tcd *tcd,
	       volatile void *dev_addr,
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
		}

		nbytes = burst * addr_width;
		attr = (EDMA_TCD_ATTR_SSIZE_16BIT | EDMA_TCD_ATTR_DSIZE_16BIT);
		iter = period_len / nbytes;

		last_sg = (u32)&tcd[(i+1)%period_count];

		edma_fill_tcd(&tcd[i], src_addr, dst_addr, attr, soff, nbytes, 0,
					iter, iter, doff, last_sg, 1, 0, 1, 0);
		dma_addr_next = dma_addr_next + period_len;
	}
}

void edma_irq_handler(volatile void * edma_addr) {

	unsigned int intr;

	intr = read32(edma_addr + EDMA_CH_INT);
	if (!intr)
		return ;
	write32(edma_addr + EDMA_CH_INT, 1);
	/*one buffer finished*/

}

void edma_start(volatile void * edma_addr, struct nxp_edma_hw_tcd * tcd,
		u32 is_rx) {
	u32 val;

	edma_set_tcd(edma_addr, &tcd[0]);

	val = read32(edma_addr + EDMA_CH_SBR);

	if (is_rx)
		val |= EDMA_CH_SBR_RD;
	else
		val |= EDMA_CH_SBR_WR;

	write32(edma_addr + EDMA_CH_SBR, val);

	val = read32(edma_addr + EDMA_CH_CSR);
	val |= EDMA_CH_CSR_ERQ;
	write32(edma_addr + EDMA_CH_CSR, val);

}

void edma_stop(volatile void * edma_addr) {
	u32 val;
	
	val = read32(edma_addr+ EDMA_CH_CSR);
	val |= ~EDMA_CH_CSR_ERQ;
	write32(edma_addr + EDMA_CH_CSR, val);
}

void edma_dump(volatile void * edma_addr) {
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

