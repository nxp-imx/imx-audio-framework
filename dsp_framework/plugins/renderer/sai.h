/*******************************************************************************
 * sai.c
 *
 ******************************************************************************/

#ifndef   _SAI_H
#define   _SAI_H

#define FSL_SAI_TCSR(offset) (0x00 + offset) /* SAI Transmit Control */
#define FSL_SAI_TCR1(offset) (0x04 + offset) /* SAI Transmit Configuration 1 */
#define FSL_SAI_TCR2(offset) (0x08 + offset) /* SAI Transmit Configuration 2 */
#define FSL_SAI_TCR3(offset) (0x0c + offset) /* SAI Transmit Configuration 3 */
#define FSL_SAI_TCR4(offset) (0x10 + offset) /* SAI Transmit Configuration 4 */
#define FSL_SAI_TCR5(offset) (0x14 + offset) /* SAI Transmit Configuration 5 */
#define FSL_SAI_TDR0    0x20 /* SAI Transmit Data */
#define FSL_SAI_TDR1    0x24 /* SAI Transmit Data */
#define FSL_SAI_TFR0    0x40 /* SAI Transmit FIFO */
#define FSL_SAI_TFR1    0x44 /* SAI Transmit FIFO */
#define FSL_SAI_TMR     0x60 /* SAI Transmit Mask */
#define FSL_SAI_TTCTL	0x70 /* SAI Transmit Timestamp Control Register */
#define FSL_SAI_TTCTN	0x74 /* SAI Transmit Timestamp Counter Register */
#define FSL_SAI_TBCTN	0x78 /* SAI Transmit Bit Counter Register */
#define FSL_SAI_TTCAP	0x7C /* SAI Transmit Timestamp Capture */
#define FSL_SAI_RCSR(offset) (0x80 + offset) /* SAI Receive Control */
#define FSL_SAI_RCR1(offset) (0x84 + offset) /* SAI Receive Configuration 1 */
#define FSL_SAI_RCR2(offset) (0x88 + offset) /* SAI Receive Configuration 2 */
#define FSL_SAI_RCR3(offset) (0x8c + offset) /* SAI Receive Configuration 3 */
#define FSL_SAI_RCR4(offset) (0x90 + offset) /* SAI Receive Configuration 4 */
#define FSL_SAI_RCR5(offset) (0x94 + offset) /* SAI Receive Configuration 5 */
#define FSL_SAI_RDR0    0xa0 /* SAI Receive Data */
#define FSL_SAI_RDR1    0xa4 /* SAI Receive Data */
#define FSL_SAI_RFR0    0xc0 /* SAI Receive FIFO */
#define FSL_SAI_RFR1    0xc4 /* SAI Receive FIFO */
#define FSL_SAI_RFR     0xc0 /* SAI Receive FIFO */
#define FSL_SAI_RMR     0xe0 /* SAI Receive Mask */
#define FSL_SAI_RTCTL	0xf0 /* SAI Receive Timestamp Control Register */
#define FSL_SAI_RTCTN	0xf4 /* SAI Receive Timestamp Counter Register */
#define FSL_SAI_RBCTN	0xf8 /* SAI Receive Bit Counter Register */
#define FSL_SAI_RTCAP	0xfc /* SAI Receive Timestamp Capture */
#define FSL_SAI_MCTL	0x100 /* SAI MCLK Control Register */
#define FSL_SAI_MDIV	0x104 /* SAI MCLK Divide Register */

#define FSL_SAI_xCSR(tx, off)   (tx ? FSL_SAI_TCSR(off) : FSL_SAI_RCSR(off))
#define FSL_SAI_xCR1(tx, off)   (tx ? FSL_SAI_TCR1(off) : FSL_SAI_RCR1(off))
#define FSL_SAI_xCR2(tx, off)   (tx ? FSL_SAI_TCR2(off) : FSL_SAI_RCR2(off))
#define FSL_SAI_xCR3(tx, off)   (tx ? FSL_SAI_TCR3(off) : FSL_SAI_RCR3(off))
#define FSL_SAI_xCR4(tx, off)   (tx ? FSL_SAI_TCR4(off) : FSL_SAI_RCR4(off))
#define FSL_SAI_xCR5(tx, off)   (tx ? FSL_SAI_TCR5(off) : FSL_SAI_RCR5(off))
#define FSL_SAI_xDR(tx)         (tx ? FSL_SAI_TDR : FSL_SAI_RDR)
#define FSL_SAI_xFR(tx)         (tx ? FSL_SAI_TFR : FSL_SAI_RFR)
#define FSL_SAI_xMR(tx)         (tx ? FSL_SAI_TMR : FSL_SAI_RMR)

/* SAI Transmit/Receive Control Register */
#define FSL_SAI_CSR_TERE        BIT(31)
#define FSL_SAI_CSR_SE          BIT(30)
#define FSL_SAI_CSR_FR          BIT(25)
#define FSL_SAI_CSR_SR          BIT(24)
#define FSL_SAI_CSR_xF_SHIFT    16
#define FSL_SAI_CSR_xF_W_SHIFT  18
#define FSL_SAI_CSR_xF_MASK     (0x1f << FSL_SAI_CSR_xF_SHIFT)
#define FSL_SAI_CSR_xF_W_MASK   (0x7 << FSL_SAI_CSR_xF_W_SHIFT)
#define FSL_SAI_CSR_WSF         BIT(20)
#define FSL_SAI_CSR_SEF         BIT(19)
#define FSL_SAI_CSR_FEF         BIT(18)
#define FSL_SAI_CSR_FWF         BIT(17)
#define FSL_SAI_CSR_FRF         BIT(16)
#define FSL_SAI_CSR_xIE_SHIFT   8
#define FSL_SAI_CSR_xIE_MASK    (0x1f << FSL_SAI_CSR_xIE_SHIFT)
#define FSL_SAI_CSR_WSIE        BIT(12)
#define FSL_SAI_CSR_SEIE        BIT(11)
#define FSL_SAI_CSR_FEIE        BIT(10)
#define FSL_SAI_CSR_FWIE        BIT(9)
#define FSL_SAI_CSR_FRIE        BIT(8)
#define FSL_SAI_CSR_FRDE        BIT(0)

/* SAI Transmit and Receive Configuration 1 Register */
#define FSL_SAI_CR1_RFW_MASK    0x1f

/* SAI Transmit and Receive Configuration 2 Register */
#define FSL_SAI_CR2_SYNC        BIT(30)
#define FSL_SAI_CR2_MSEL_MASK   (0x3 << 26)
#define FSL_SAI_CR2_MSEL_BUS    0
#define FSL_SAI_CR2_MSEL_MCLK1  BIT(26)
#define FSL_SAI_CR2_MSEL_MCLK2  BIT(27)
#define FSL_SAI_CR2_MSEL_MCLK3  (BIT(26) | BIT(27))
#define FSL_SAI_CR2_MSEL(ID)    ((ID) << 26)
#define FSL_SAI_CR2_BCP         BIT(25)
#define FSL_SAI_CR2_BCD_MSTR    BIT(24)
#define FSL_SAI_CR2_DIV_MASK    0xff


/* SAI Transmit and Receive Configuration 3 Register */
#define FSL_SAI_CR3_TRCE_MASK   (0xff << 16)
#define FSL_SAI_CR3_TRCE(x)     (x << 16)
#define FSL_SAI_CR3_WDFL(x)     (x)
#define FSL_SAI_CR3_WDFL_MASK   0x1f

/* SAI Transmit and Receive Configuration 4 Register */

#define FSL_SAI_CR4_FCONT       BIT(28)
#define FSL_SAI_CR4_FCOMB_SHIFT BIT(26)
#define FSL_SAI_CR4_FCOMB_SOFT  BIT(27)
#define FSL_SAI_CR4_FCOMB_MASK  (0x3 << 26)
#define FSL_SAI_CR4_FPACK_8     (0x2 << 24)
#define FSL_SAI_CR4_FPACK_16    (0x3 << 24)
#define FSL_SAI_CR4_FRSZ(x)     (((x) - 1) << 16)
#define FSL_SAI_CR4_FRSZ_MASK   (0x1f << 16)
#define FSL_SAI_CR4_SYWD(x)     (((x) - 1) << 8)
#define FSL_SAI_CR4_SYWD_MASK   (0x1f << 8)
#define FSL_SAI_CR4_MF          BIT(4)
#define FSL_SAI_CR4_FSE         BIT(3)
#define FSL_SAI_CR4_FSP         BIT(1)
#define FSL_SAI_CR4_FSD_MSTR    BIT(0)

/* SAI Transmit and Receive Configuration 5 Register */
#define FSL_SAI_CR5_WNW(x)      (((x) - 1) << 24)
#define FSL_SAI_CR5_WNW_MASK    (0x1f << 24)
#define FSL_SAI_CR5_W0W(x)      (((x) - 1) << 16)
#define FSL_SAI_CR5_W0W_MASK    (0x1f << 16)
#define FSL_SAI_CR5_FBT(x)      ((x) << 8)
#define FSL_SAI_CR5_FBT_MASK    (0x1f << 8)

/* SAI MCLK Control Register */
#define FSL_SAI_MCTL_MCLK_EN	BIT(30)	/* MCLK Enable */
#define FSL_SAI_MCTL_MSEL_MASK	(0x3 << 24)
#define FSL_SAI_MCTL_MSEL(ID)   ((ID) << 24)
#define FSL_SAI_MCTL_MSEL_BUS	0
#define FSL_SAI_MCTL_MSEL_MCLK1	BIT(24)
#define FSL_SAI_MCTL_MSEL_MCLK2	BIT(25)
#define FSL_SAI_MCTL_MSEL_MCLK3	(BIT(24) | BIT(25))
#define FSL_SAI_MCTL_DIV_EN	BIT(23)
#define FSL_SAI_MCTL_DIV_MASK	0xFF

void sai_init(volatile void * sai_addr, int mode, int channel, int rate, int width, int mclk_rate);
void sai_start(volatile void * sai_addr, int tx);
void sai_stop(volatile void * sai_addr, int tx);
void sai_dump(volatile void * sai_addr);
void sai_irq_handler(volatile void * sai_addr);
void sai_suspend(volatile void *sai_addr,  u32 *cache_addr);
void sai_resume(volatile void *sai_addr, u32 * cache_addr);

#endif
