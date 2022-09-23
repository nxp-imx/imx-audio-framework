/*
 * Copyright 2020 NXP
 *
 * sdma.c - SDMA driver
 */
#include <string.h>
#include <errno.h>

#include "fsl_dma.h"
#include "board.h"

#include "xa_type_def.h"

#include "types.h"
#include "sdma_script_code_imx7d_4_5.h"

#include "debug.h"

/* SDMA registers */
#define SDMA_H_C0PTR		0x000
#define SDMA_H_INTR		0x004
#define SDMA_H_STATSTOP		0x008
#define SDMA_H_START		0x00c
#define SDMA_H_EVTOVR		0x010
#define SDMA_H_DSPOVR		0x014
#define SDMA_H_HOSTOVR		0x018
#define SDMA_H_EVTPEND		0x01c
#define SDMA_H_DSPENBL		0x020
#define SDMA_H_RESET		0x024
#define SDMA_H_EVTERR		0x028
#define SDMA_H_INTRMSK		0x02c
#define SDMA_H_PSW		0x030
#define SDMA_H_EVTERRDBG	0x034
#define SDMA_H_CONFIG		0x038
#define SDMA_ONCE_ENB		0x040
#define SDMA_ONCE_DATA		0x044
#define SDMA_ONCE_INSTR		0x048
#define SDMA_ONCE_STAT		0x04c
#define SDMA_ONCE_CMD		0x050
#define SDMA_EVT_MIRROR		0x054
#define SDMA_ILLINSTADDR	0x058
#define SDMA_CHN0ADDR		0x05c
#define SDMA_ONCE_RTB		0x060
#define SDMA_XTRIG_CONF1	0x070
#define SDMA_XTRIG_CONF2	0x074
#define SDMA_CHNENBL0_IMX35	0x200
#define SDMA_CHNENBL0_IMX31	0x080
#define SDMA_CHNPRI_0		0x100
#define SDMA_DONE0_CONFIG	0x1000
#define SDMA_DONE0_CONFIG_DONE_SEL	0x7
#define SDMA_DONE0_CONFIG_DONE_DIS	0x6

/*
 * Buffer descriptor status values.
 */
#define BD_DONE  0x01
#define BD_WRAP  0x02
#define BD_CONT  0x04
#define BD_INTR  0x08
#define BD_RROR  0x10
#define BD_LAST  0x20
#define BD_EXTD  0x80

/*
 * Data Node descriptor status values.
 */
#define DND_END_OF_FRAME  0x80
#define DND_END_OF_XFER   0x40
#define DND_DONE          0x20
#define DND_UNUSED        0x01

/*
 * IPCV2 descriptor status values.
 */
#define BD_IPCV2_END_OF_FRAME  0x40

#define IPCV2_MAX_NODES        50
/*
 * Error bit set in the CCB status field by the SDMA,
 * in setbd routine, in case of a transfer error
 */
#define DATA_ERROR  0x10000000

/*
 * Buffer descriptor commands.
 */
#define C0_ADDR             0x01
#define C0_LOAD             0x02
#define C0_DUMP             0x03
#define C0_SETCTX           0x07
#define C0_GETCTX           0x03
#define C0_SETDM            0x01
#define C0_SETPM            0x04
#define C0_GETDM            0x02
#define C0_GETPM            0x08
/*
 * Change endianness indicator in the BD command field
 */
#define CHANGE_ENDIANNESS   0x80

/*
 *  p_2_p watermark_level description
 *	Bits		Name			Description
 *	0-7		Lower WML		Lower watermark level
 *	8		PS			1: Pad Swallowing
 *						0: No Pad Swallowing
 *	9		PA			1: Pad Adding
 *						0: No Pad Adding
 *	10		SPDIF			If this bit is set both source
 *						and destination are on SPBA
 *	11		Source Bit(SP)		1: Source on SPBA
 *						0: Source on AIPS
 *	12		Destination Bit(DP)	1: Destination on SPBA
 *						0: Destination on AIPS
 *	13-15		---------		MUST BE 0
 *	16-23		Higher WML		HWML
 *	24-27		N			Total number of samples after
 *						which Pad adding/Swallowing
 *						must be done. It must be odd.
 *	28		Lower WML Event(LWE)	SDMA events reg to check for
 *						LWML event mask
 *						0: LWE in EVENTS register
 *						1: LWE in EVENTS2 register
 *	29		Higher WML Event(HWE)	SDMA events reg to check for
 *						HWML event mask
 *						0: HWE in EVENTS register
 *						1: HWE in EVENTS2 register
 *	30		---------		MUST BE 0
 *	31		CONT			1: Amount of samples to be
 *						transferred is unknown and
 *						script will keep on
 *						transferring samples as long as
 *						both events are detected and
 *						script must be manually stopped
 *						by the application
 *						0: The amount of samples to be
 *						transferred is equal to the
 *						count field of mode word
 */
#define SDMA_WATERMARK_LEVEL_LWML	0xFF
#define SDMA_WATERMARK_LEVEL_PS		BIT(8)
#define SDMA_WATERMARK_LEVEL_PA		BIT(9)
#define SDMA_WATERMARK_LEVEL_SPDIF	BIT(10)
#define SDMA_WATERMARK_LEVEL_SP		BIT(11)
#define SDMA_WATERMARK_LEVEL_DP		BIT(12)
#define SDMA_WATERMARK_LEVEL_SD		BIT(13)
#define SDMA_WATERMARK_LEVEL_DD		BIT(14)
#define SDMA_WATERMARK_LEVEL_HWML	(0xFF << 16)
#define SDMA_WATERMARK_LEVEL_LWE	BIT(28)
#define SDMA_WATERMARK_LEVEL_HWE	BIT(29)
#define SDMA_WATERMARK_LEVEL_CONT	BIT(31)

#define SDMA_DMA_BUSWIDTHS	(BIT(DMA_SLAVE_BUSWIDTH_1_BYTE) | \
				 BIT(DMA_SLAVE_BUSWIDTH_2_BYTES) | \
				 BIT(DMA_SLAVE_BUSWIDTH_3_BYTES) | \
				 BIT(DMA_SLAVE_BUSWIDTH_4_BYTES))

#define SDMA_DMA_DIRECTIONS	(BIT(DMA_DEV_TO_MEM) | \
				 BIT(DMA_MEM_TO_DEV) | \
				 BIT(DMA_DEV_TO_DEV))

#define SDMA_WATERMARK_LEVEL_FIFOS_OFF	12
#define SDMA_WATERMARK_LEVEL_SW_DONE	BIT(23)
#define SDMA_WATERMARK_LEVEL_SW_DONE_SEL_OFF 24

#define IMX_DMA_SG_LOOP		BIT(0)


#define MAX_DMA_CHANNELS 32
#define MXC_SDMA_DEFAULT_PRIORITY 1
#define MXC_SDMA_MIN_PRIORITY 1
#define MXC_SDMA_MAX_PRIORITY 7


/*
 * 0x78(SDMA_XTRIG_CONF2+4)~0x100(SDMA_CHNPRI_O) registers are reserved and
 * can't be accessed. Skip these register touch in suspend/resume. Also below
 * two macros are only used on i.mx6sx.
 */
#define MXC_SDMA_RESERVED_REG (SDMA_CHNPRI_0 - SDMA_XTRIG_CONF2 - 4)
#define MXC_SDMA_SAVED_REG_NUM (((SDMA_CHNENBL0_IMX35 + 4 * 48) - \
				MXC_SDMA_RESERVED_REG) / 4)

/**
 * struct sdma_state_registers - SDMA context for a channel
 *
 * @pc:		program counter
 * @unused1:	unused
 * @t:		test bit: status of arithmetic & test instruction
 * @rpc:	return program counter
 * @unused0:	unused
 * @sf:		source fault while loading data
 * @spc:	loop start program counter
 * @unused2:	unused
 * @df:		destination fault while storing data
 * @epc:	loop end program counter
 * @lm:		loop mode
 */
struct sdma_state_registers {
	UWORD32 pc     :14;
	UWORD32 unused1: 1;
	UWORD32 t      : 1;
	UWORD32 rpc    :14;
	UWORD32 unused0: 1;
	UWORD32 sf     : 1;
	UWORD32 spc    :14;
	UWORD32 unused2: 1;
	UWORD32 df     : 1;
	UWORD32 epc    :14;
	UWORD32 lm     : 2;
} __attribute__ ((packed));

/*
 * Mode/Count of data node descriptors - IPCv2
 */
struct sdma_mode_count {
#define SDMA_BD_MAX_CNT	0xffff
	UWORD32 count   : 16; /* size of the buffer pointed by this BD */
	UWORD32 status  :  8; /* E,R,I,C,W,D status bits stored here */
	UWORD32 command :  8; /* command mostly used for channel 0 */
};

/*
 * Buffer descriptor
 */
struct sdma_buffer_descriptor {
	struct sdma_mode_count  mode;
	UWORD32 buffer_addr;	/* address of the buffer described */
	UWORD32 ext_buffer_addr;	/* extended buffer address */
} __attribute__ ((packed));

/**
 * struct sdma_channel_control - Channel control Block
 *
 * @current_bd_ptr:	current buffer descriptor processed
 * @base_bd_ptr:	first element of buffer descriptor array
 * @unused:		padding. The SDMA engine expects an array of 128 byte
 *			control blocks
 */
struct sdma_channel_control {
	UWORD32 current_bd_ptr;
	UWORD32 base_bd_ptr;
	UWORD32 unused[2];
} __attribute__ ((packed));


/**
 * struct sdma_context_data - sdma context specific to a channel
 *
 * @channel_state:	channel state bits
 * @gReg:		general registers
 * @mda:		burst dma destination address register
 * @msa:		burst dma source address register
 * @ms:			burst dma status register
 * @md:			burst dma data register
 * @pda:		peripheral dma destination address register
 * @psa:		peripheral dma source address register
 * @ps:			peripheral dma status register
 * @pd:			peripheral dma data register
 * @ca:			CRC polynomial register
 * @cs:			CRC accumulator register
 * @dda:		dedicated core destination address register
 * @dsa:		dedicated core source address register
 * @ds:			dedicated core status register
 * @dd:			dedicated core data register
 * @scratch0:		1st word of dedicated ram for context switch
 * @scratch1:		2nd word of dedicated ram for context switch
 * @scratch2:		3rd word of dedicated ram for context switch
 * @scratch3:		4th word of dedicated ram for context switch
 * @scratch4:		5th word of dedicated ram for context switch
 * @scratch5:		6th word of dedicated ram for context switch
 * @scratch6:		7th word of dedicated ram for context switch
 * @scratch7:		8th word of dedicated ram for context switch
 */
struct sdma_context_data {
	struct sdma_state_registers  channel_state;
	UWORD32  gReg[8];
	UWORD32  mda;
	UWORD32  msa;
	UWORD32  ms;
	UWORD32  md;
	UWORD32  pda;
	UWORD32  psa;
	UWORD32  ps;
	UWORD32  pd;
	UWORD32  ca;
	UWORD32  cs;
	UWORD32  dda;
	UWORD32  dsa;
	UWORD32  ds;
	UWORD32  dd;
	UWORD32  scratch0;
	UWORD32  scratch1;
	UWORD32  scratch2;
	UWORD32  scratch3;
	UWORD32  scratch4;
	UWORD32  scratch5;
	UWORD32  scratch6;
	UWORD32  scratch7;
} __attribute__ ((packed));

static __inline__ void
__clear_bit(unsigned long nr, volatile void * addr)
{
	int *m = ((int *) addr) + (nr >> 5);

	*m &= ~(1 << (nr & 31));
}

static inline void
__set_bit(unsigned long nr, volatile void * addr)
{
	int *m = ((int *) addr) + (nr >> 5);

	*m |= 1 << (nr & 31);
}

struct sdma_chan {
	dmac_t                          dma_chan;
	sdmac_cfg_t                     sdmac_cfg;

	/* internal config */
	int                             used;
	int                             channel_id;
	int                             bdnum;
	struct sdma_buffer_descriptor	*bd;
};

struct SDMA {
	dma_t				dma;

	volatile void                   *regs;
	int                             Int;
	struct sdma_channel_control	*ccb;
	struct sdma_context_data	*context;
	struct sdma_buffer_descriptor	*bd0;
	struct sdma_chan		chan_info[MAX_DMA_CHANNELS];
	UWORD32				save_regs[MXC_SDMA_SAVED_REG_NUM];
	u32				save_done0_regs[2];

	UWORD32				fw_loaded;
	UWORD32				is_on;
	int				init_done;
};

#define PAGE_SIZE 0x400
#define SDMA_H_CONFIG_DSPDMA	BIT(12) /* indicates if the DSPDMA is used */
#define SDMA_H_CONFIG_RTD_PINS	BIT(11) /* indicates if Real-Time Debug pins are enabled */
#define SDMA_H_CONFIG_ACR	BIT(4)  /* indicates if AHB freq /core freq = 2 or 1 */
#define SDMA_H_CONFIG_CSM	(3)       /* indicates which context switch mode is selected*/

#define BD_SIZE 0x80000

#define false 0
#define true 1

int sdma_get_para(struct dma_device *dma, dmac_t *dmac, int para, void *value)
{
	struct SDMA *sdma = (struct SDMA *)dma;
	if (!sdma)
		return -EINVAL;
	switch (para) {
	case GET_IRQ:
		*(int *)value = sdma->Int;
		break;
	default:
		break;
	}
	return 0;
}

static void sdma_enable_channel(struct SDMA *sdma, int channel)
{
	/* 1. Neither HSTART[i] bit can be set while
	 * the corresponding HE[i] bit is cleared.
	 * 2. When the Arm platform tries to set the HSTART[i] bit
	 * by writing a one (if the corresponding HE[i] bit is clear),
	 * the bit in the HSTART[i] register will remain cleared and
	 * the HE[i] bit will be set. */

	write32(sdma->regs + SDMA_H_START, BIT(channel));
}

/*
 * sdma_run_channel0 - run a channel and wait till it's done
 */
static int sdma_run_channel0(struct SDMA *sdma)
{
	unsigned int ret = 0, val, count;
	UWORD32 reg;

	sdma_enable_channel(sdma, 0);

	/* If dsp run on no cache mode, bd will updated on time,
	 * and it will cause dsp slower than before.
	 * If dsp run on cache mode, bd status can't updated on time.
	 * Thus suggest only detect bd0 status in debug mode. */
	while(1) {
		val = read32(sdma->regs + SDMA_H_STATSTOP);
		if (!(val & 1))
			break;
		for(count = 0; count < 99;)
			count++;
	}

	/* Set bits of CONFIG register with dynamic context switching */
	reg = read32(sdma->regs + SDMA_H_CONFIG);
	if ((reg & SDMA_H_CONFIG_CSM) == 0) {
		reg |= SDMA_H_CONFIG_CSM;
		write32(sdma->regs + SDMA_H_CONFIG, reg);
	}

	return ret;
}

static void sdma_channel_attach_bd(struct SDMA* sdma, int channel, int bdnum)
{
	sdma->chan_info[channel].bdnum = bdnum;
	if (!channel) {
		xaf_malloc((void **)&sdma->bd0, PAGE_SIZE, 0);
		memset(sdma->bd0, 0, PAGE_SIZE);
	}
	else {
		xaf_malloc((void **)&sdma->chan_info[channel].bd, sizeof(struct sdma_buffer_descriptor) * bdnum, 0);
		memset(sdma->chan_info[channel].bd, 0, sizeof(struct sdma_buffer_descriptor) * bdnum);
	}
}

static int sdma_request_channel0(struct SDMA *sdma)
{
	int ret = 0;
	int channel = 0;

	sdma_channel_attach_bd(sdma, 0, 1);

	if (!sdma->bd0) {
		ret = -1;
		goto out;
	}

	sdma->ccb[0].base_bd_ptr = (UWORD32)sdma->bd0;
	sdma->ccb[0].current_bd_ptr = (UWORD32)sdma->bd0;

	// set channel 0 priority
	write32(sdma->regs + SDMA_CHNPRI_0 + 4 * channel, MXC_SDMA_DEFAULT_PRIORITY);
	return 0;
out:

	return ret;
}

static int sdma_config_ownership(struct SDMA *sdma, int channel,
		int event_override, int mcu_override, int dsp_override)
{
	unsigned long evt, mcu, dsp;

	if (event_override && mcu_override && dsp_override)
		return -1;

	evt = read32(sdma->regs + SDMA_H_EVTOVR); //EO
	mcu = read32(sdma->regs + SDMA_H_HOSTOVR); //HO
	dsp = read32(sdma->regs + SDMA_H_DSPOVR); //DO

	if (dsp_override)
		__clear_bit(channel, &dsp);
	else
		__set_bit(channel, &dsp);

	if (event_override)
		__clear_bit(channel, &evt);
	else
		__set_bit(channel, &evt);

	if (mcu_override)
		__clear_bit(channel, &mcu);
	else
		__set_bit(channel, &mcu);

	write32(sdma->regs + SDMA_H_EVTOVR, evt);
	write32(sdma->regs + SDMA_H_HOSTOVR, mcu);
	write32(sdma->regs + SDMA_H_DSPOVR, dsp);

	return 0;
}

static int load_firmware(struct SDMA* sdma, void *buf, int size, int address)
{
	int val,reg;
	struct sdma_buffer_descriptor *bd0 = sdma->bd0;
	unsigned int *buf_phy;

	xaf_malloc((void **)&buf_phy, size, 0);
	if (!buf_phy)
		return -1;

	memcpy(buf_phy, buf, size);

	bd0->mode.command = C0_SETPM;
	bd0->mode.status = BD_DONE | BD_WRAP | BD_EXTD;
	bd0->mode.count = size / 2;
	bd0->buffer_addr = (int)buf_phy;
	bd0->ext_buffer_addr = address;

	sdma_run_channel0(sdma);

	xaf_free(buf_phy, 0);

	sdma->fw_loaded = true;
	sdma->is_on  = true;
	return 0;
}

int sdma_init(struct dma_device *dev)
{
	struct SDMA *sdma = (struct SDMA *)dev;
	unsigned int ccb_phys, bd0_phys;
	struct sdma_buffer_descriptor *bd0;
	unsigned int ccbsize;
	int count = 0, i, val;
	int channel = 0, reg;
	unsigned int reset;
	int ret;

	if (!sdma)
		return -1;

	if (sdma->init_done)
		return 0;
	/* reset */
	write32(sdma->regs + SDMA_H_RESET, 1);
	do {
		reset = read32(sdma->regs + SDMA_H_RESET);
	} while(reset == 1);
	/* Be sure SDMA has not started yet */
	write32(sdma->regs + SDMA_H_C0PTR, 0);

	ccbsize = MAX_DMA_CHANNELS * (sizeof(struct sdma_channel_control)
		+ sizeof(struct sdma_context_data));
	xaf_malloc((void **)&sdma->ccb, ccbsize, 0);
	if (!sdma->ccb) return -1;
	memset(sdma->ccb, 0, ccbsize);
	ccb_phys = (unsigned int)sdma->ccb;
	sdma->context = (void *)sdma->ccb + MAX_DMA_CHANNELS * (sizeof(struct sdma_channel_control));

	/* disable all channels */
	for (i = 0; i < 48; i++)
		write32(sdma->regs + SDMA_CHNENBL0_IMX35 + i * 4, 0);
	/* All channels have priority 0 */
	for (i = 0; i < MAX_DMA_CHANNELS; i++)
		write32(sdma->regs + SDMA_CHNPRI_0 + i * 4, 0);

	// request channel 0
	sdma_request_channel0(sdma);

	// config ownership channel 0
	sdma_config_ownership(sdma, 0, false, true, false);

	/* Set Command Channel (Channel Zero) */
	write32(sdma->regs + SDMA_CHN0ADDR, 0x4050);

	/* Set bits of CONFIG register ratio 1 */
	write32(sdma->regs + SDMA_H_CONFIG, SDMA_H_CONFIG_ACR);
	//write32(sdma->regs + SDMA_H_CONFIG, 0);

	write32(sdma->regs + SDMA_H_C0PTR, ccb_phys);

	//set channel 0 priority
	write32(sdma->regs + SDMA_CHNPRI_0, MXC_SDMA_MAX_PRIORITY);

	//load firmware
	ret = load_firmware(sdma, (void *)sdma_code, RAM_CODE_SIZE * sizeof(short), RAM_CODE_START_ADDR);
	if (ret)
		return -1;

	sdma->init_done = 1;

	LOG("sdma init\n");
	return 0;
}

static void sdma_event_enable(struct SDMA *sdma, int event, unsigned int channel, unsigned int done_cfg)
{
	unsigned int chnenbl;
	unsigned int val;

	if (event < 0)
		return;
	chnenbl = SDMA_CHNENBL0_IMX35 + 4 * event;
	val = read32(sdma->regs + chnenbl);
	__set_bit(channel, &val);
	write32(sdma->regs + chnenbl, val);

	if (done_cfg) {
		val = read32(sdma->regs + SDMA_DONE0_CONFIG);
		__set_bit(done_cfg, &val);
		write32(sdma->regs + SDMA_DONE0_CONFIG, val);
	}
}

static int sdma_disable_channel(struct SDMA *sdma, unsigned int channel)
{
	write32(sdma->regs + SDMA_H_STATSTOP, BIT(channel));
	return 0;
}

static int load_channel(dmac_t *dmac)
{
	struct SDMA *sdma = (struct SDMA *)dmac->dma_device;
	struct sdma_chan *sdmac = (struct sdma_chan *)dmac;
	sdmac_cfg_t *sdmac_cfg = &sdmac->sdmac_cfg;

	unsigned int done_cfg;
	struct sdma_buffer_descriptor *bd0 = sdma->bd0;
	struct sdma_buffer_descriptor *bd1;
	struct sdma_context_data *context;
	unsigned int context_phys;
	unsigned int script_addr;
	unsigned int r0, r1, r2, r6, r7;

	int type = dmac->direction;
	void *src_addr = dmac->src_addr, *dest_addr = dmac->dest_addr;
	int period_len = dmac->period_len;

	int channel = sdmac->channel_id;
	int ch_watermark = sdmac_cfg->watermark;
	int event = sdmac_cfg->events[0], event2 = sdmac_cfg->events[1];

	if (!sdma || !dest_addr || !src_addr)
		return -1;
	if (event < 0)
		return -1;
	r0 = r1 = r2 = r6 = r7 = 0;
	script_addr = 0;

	if (type == DMA_MEM_TO_DEV) {
		/* event 16: ASRC Context 0 receive DMA request */
		event = 16;
		script_addr = mcu_2_app_ADDR;
		r0 = 0;
		r1 = BIT(event);
		r2 = 0;
		r6 = (unsigned int)dest_addr;
		r7 = 0xc;
	}
	else if (type == DMA_DEV_TO_DEV) {
		/* event 5:  SAI-3 transmit DMA request
		 * event 17: ASRC Context 0 transmit DMA request */
		event = 17;
		event2 = 5;
		script_addr = p_2_p_ADDR;
		r0 = BIT(event2);
		r1 = BIT(event);
		r2 = (unsigned int)src_addr;
		r6 = (unsigned int)dest_addr;
		/*0-7 Lower WML Watermark Level
		8 PS 1 : Pad Swallowing
		0 : No Pad swallowing
		9 PA 1 : Pad Adding
		0 : No Pad swallowing
		10 SPDIF If this bit is set both source and destination are on SPBA
		11 Source Bit(SP) 1 : Source on SPBA
		0 : Source on AIPS
		12 Destination Bit (DP) 1 : Destination on SPBA
		0 : Destination on AIPS
		13-15 ------------ MUST BE 0
		16-23 Higher WML HWML
		24-27 N Total number of samples after which Pad adding/Swallowing must be
		done. It must be odd.
		28 Lower WML Event (LWE) SDMA events reg to check for LWML event mask
		0 : LWE in EVENTS register
		1 : LWE in EVENTS2 register
		29 Higher WML Event(HWE) SDMA events reg to check for HWML event mask
		0 : HWE in EVENTS register
		1 : HWE in EVENTS2 register
		30 ------------ MUST BE 0
		31 CONT 1 : Amount of samples to be transferred is unknown and script will keep
		on transferring samples as long as both events are detected and script
		must be manually stopped by the application
		0 : The amount of samples to be is equal to the count field of mode word
		*/
		r7 = 0x80061806;
	} else if (type == DMA_DEV_TO_MEM) {
		script_addr = sai_2_mcu_ADDR;
		r0 = 0;
		r1 = BIT(event);
		r2 = 0;
		r6 = (unsigned int)src_addr;
		r7 = ch_watermark;
	}
	/* set channel priority: 1 */
	write32(sdma->regs + SDMA_CHNPRI_0 + 4 * channel, 1);

	/* cfg for PDM, not used for audio */
	if (type == DMA_DEV_TO_MEM)
		done_cfg = 0x7;
	else
		done_cfg = 0x00;
	sdma_event_enable(sdma, event, channel, done_cfg);
	sdma_event_enable(sdma, event2, channel, done_cfg);

	if (type == DMA_MEM_TO_DEV) {
		sdma_channel_attach_bd(sdma, channel, 2);
		/* set 2 bds for transmite data */
		bd1 = sdmac->bd;
		bd1->mode.command = 2;
		bd1->mode.status = BD_DONE | BD_INTR | BD_CONT;
		bd1->mode.count = period_len;
		bd1->buffer_addr = (unsigned int)src_addr;

		bd1 = &bd1[1];
		bd1->mode.command = 2;
		bd1->mode.status = BD_DONE | BD_WRAP | BD_CONT + BD_INTR;
		bd1->mode.count = period_len;
		bd1->buffer_addr = (unsigned int)(src_addr + period_len);
	} else if (type == DMA_DEV_TO_DEV) {
		sdma_channel_attach_bd(sdma, channel, 1);
		bd1 = sdmac->bd;

		bd1->mode.command = 2;
		bd1->mode.status = BD_DONE | BD_WRAP | BD_CONT;
		bd1->mode.count = 64;
	} else if (type == DMA_DEV_TO_MEM) {
		sdma_channel_attach_bd(sdma, channel, 2);
		/* set 2 bds for transmite data */
		bd1 = sdmac->bd;
		bd1->mode.command = 0;
		bd1->mode.status = BD_DONE | BD_INTR | BD_CONT | BD_EXTD;
		bd1->mode.count = period_len;
		bd1->buffer_addr = (unsigned int)dest_addr;

		bd1 = &bd1[1];
		bd1->mode.command = 0;
		bd1->mode.status = BD_DONE | BD_INTR | BD_CONT | BD_EXTD | BD_WRAP;
		bd1->mode.count = period_len;
		bd1->buffer_addr = (unsigned int)(dest_addr + period_len);
	}

	sdma_disable_channel(sdma, channel);

	/* set starting channel triggered by DMA request event
	 * set EO = 0, HO , DO = 1 */
	sdma_config_ownership(sdma, channel, true, true, false);

	memset(sdma->context, 0, sizeof(*sdma->context));
	context = sdma->context;
	context_phys = (unsigned int)context;
	context->channel_state.pc = script_addr;
	context->gReg[0] = r0;
	context->gReg[1] = r1;
	context->gReg[2] = r2;
	context->gReg[6] = r6;
	context->gReg[7] = r7;

	bd0->mode.command = C0_SETDM;
	bd0->mode.status = BD_DONE | BD_WRAP | BD_EXTD;
	bd0->mode.count = sizeof(*context) / 4;
	bd0->buffer_addr = context_phys;
	bd0->ext_buffer_addr = 2048 + (sizeof(*context) / 4) * channel;
	sdma_run_channel0(sdma);

	sdma->ccb[channel].base_bd_ptr = (unsigned int)sdma->chan_info[channel].bd;
	sdma->ccb[channel].current_bd_ptr = (unsigned int)sdma->chan_info[channel].bd;

	return 0;
}

int sdma_chan_config(dmac_t *dma_chan, dmac_cfg_t *dmac_cfg)
{
	struct sdma_chan *sdmac = (struct sdma_chan *)dma_chan;
	sdmac_cfg_t *sdmac_cfg;

	dma_chan->direction = dmac_cfg->direction;
	dma_chan->src_addr = dmac_cfg->src_addr;
	dma_chan->dest_addr = dmac_cfg->dest_addr;
	dma_chan->src_maxbrust = dmac_cfg->src_maxbrust;
	dma_chan->dest_maxbrust = dmac_cfg->dest_maxbrust;
	dma_chan->src_width = dmac_cfg->src_width;
	dma_chan->dest_width = dmac_cfg->dest_width;
	dma_chan->period_len = dmac_cfg->period_len;
	dma_chan->callback = dmac_cfg->callback;
	dma_chan->comp = dmac_cfg->comp;

	if (dmac_cfg->peripheral_config)
		memcpy(&sdmac->sdmac_cfg, dmac_cfg->peripheral_config, dmac_cfg->peripheral_size);

	load_channel(dma_chan);
	LOG1("sdma ch[%d] config done\n", sdmac->channel_id);
	return 0;
}

int sdma_chan_start(dmac_t *dma_chan)
{
	struct sdma_chan *sdmac = (struct sdma_chan *)dma_chan;
	struct SDMA *sdma = dma_chan->dma_device;
	sdma_enable_channel(sdma, sdmac->channel_id);
	return 0;
}

int sdma_chan_stop(dmac_t *dma_chan)
{
	struct sdma_chan *sdmac = (struct sdma_chan *)dma_chan;
	struct SDMA *sdma = dma_chan->dma_device;
	sdma_disable_channel(sdma, sdmac->channel_id);
	return 0;
}

int find_last_bit_set(unsigned int val)
{
	int shift_bits;
	if (!val)
		return 0;

	shift_bits = 0;
	while (!(val & 0x1)) {
		val = val >> 1;
		shift_bits++;
	}

	return shift_bits;
}

int sdma_irq_handler(struct dma_device *dev)
{
	struct SDMA *sdma = (struct SDMA *)dev;
	unsigned int stat;
	stat = read32(sdma->regs + SDMA_H_INTR);
	write32(sdma->regs + SDMA_H_INTR, stat);

	/* channel 0 is special and not handled here, see run_channel0() */
	stat &= ~1;

	while (stat) {
		int channel = find_last_bit_set(stat);
		dmac_t *dma_ch = (dmac_t *)&sdma->chan_info[channel];

		//spin_lock(&sdmac->vc.lock);

		xthal_dcache_all_unlock();
		xthal_dcache_all_invalidate();
		xthal_dcache_sync();
		/* XXX: modify BD_DONE in here maybe cause sdma copy data
		 * that not updated yet. */
		sdma_change_bd_status(dma_ch);

		//spin_unlock(&sdmac->vc.lock);
		__clear_bit(channel, &stat);

		if (dma_ch->callback)
			dma_ch->callback(dma_ch->comp);
	}
	return 0;
}

static void sdma_reg_dump(struct SDMA* sdma)
{
	int i, count = 0;
	for (i = 0; i < MXC_SDMA_SAVED_REG_NUM;) {
		LOG2("sdma reg %d, %x\n", i, sdma->save_regs[i]);
		i++;
	}
}

static int sdma_save_restore_context(struct SDMA *sdma, UWORD32 save)
{
	struct sdma_context_data *context = sdma->context;
	struct sdma_buffer_descriptor *bd0 = sdma->bd0;
	unsigned long flags;
	int ret = 0;

	if (save)
		bd0->mode.command = C0_GETDM;
	else
		bd0->mode.command = C0_SETDM;

	bd0->mode.status = BD_DONE | BD_WRAP | BD_EXTD;
	bd0->mode.count = MAX_DMA_CHANNELS * sizeof(*context) / 4;
	bd0->buffer_addr = (unsigned int)sdma->context;
	bd0->ext_buffer_addr = 2048;
	ret = sdma_run_channel0(sdma);

	return ret;
}

int sdma_resume(struct dma_device *dev)
{
	LOG("sdma resume\n");
	struct SDMA *sdma = (struct SDMA *)dev;
	int ret, i;
	int channel = 0;

	if (sdma->is_on)
		return 0;
	/* Do nothing if mega/fast mix not turned off */
	if (read32(sdma->regs + SDMA_H_C0PTR)) {
		LOG("mix not turned off\n");
		return 0;
	}

	/* Firmware was lost, mark as "not ready" */
	sdma->fw_loaded = false;

	/* restore regs and load firmware */
	for (i = 0; i < MXC_SDMA_SAVED_REG_NUM; i++) {
		/*
		 * 0x78(SDMA_XTRIG_CONF2+4)~0x100(SDMA_CHNPRI_O) registers are
		 * reserved and can't be touched. Skip these regs.
		 */
		if (i > SDMA_XTRIG_CONF2 / 4)
			write32(sdma->regs + MXC_SDMA_RESERVED_REG + 4 * i, sdma->save_regs[i]);
		/* set static context switch  mode before channel0 running */
		else if (i == SDMA_H_CONFIG / 4)
			write32(sdma->regs + SDMA_H_CONFIG, sdma->save_regs[i] & ~SDMA_H_CONFIG_CSM);
		else
			write32(sdma->regs + 4 * i, sdma->save_regs[i]);
	}
	for (i = 0; i < 2; i++)
		write32(sdma->regs + SDMA_DONE0_CONFIG + 4 * i, sdma->save_done0_regs[i]);
	// set channel 0 priority
	write32(sdma->regs + SDMA_CHNPRI_0 + 4 * channel, MXC_SDMA_DEFAULT_PRIORITY);

	ret = load_firmware(sdma, (void *)sdma_code, RAM_CODE_SIZE * sizeof(short), RAM_CODE_START_ADDR);
	if (ret) {
		LOG("failed to get firmware\n");
		goto out;
	}

	ret = sdma_save_restore_context(sdma, false);
	if (ret) {
		LOG("restore context error!\n");
		goto out;
	}
out:
	return 0;
}

int sdma_suspend(struct dma_device *dev)
{
	LOG("sdma suspend\n");
	struct SDMA *sdma = (struct SDMA *)dev;
	UWORD32 ret;
	int i;

	if (!sdma->is_on)
		return 0;

	sdma->fw_loaded = false;
	sdma->is_on = false;

	ret = sdma_save_restore_context(sdma, true);
	if (ret) {
		LOG1("save context error %x\n", ret);
		return ret;
	}
	/* save regs */
	for (i = 0; i < MXC_SDMA_SAVED_REG_NUM; i++) {
		/*
		 * 0x78(SDMA_XTRIG_CONF2+4)~0x100(SDMA_CHNPRI_O) registers are
		 * reserved and can't be touched. Skip these regs.
		 */
		if (i > SDMA_XTRIG_CONF2 / 4)
			sdma->save_regs[i] = read32(sdma->regs +
							   MXC_SDMA_RESERVED_REG
							   + 4 * i);
		else
			sdma->save_regs[i] = read32(sdma->regs + 4 * i);
	}

	for (i = 0; i < 2; i++)
		sdma->save_done0_regs[i] = read32(sdma->regs + SDMA_DONE0_CONFIG + 4 * i);

	return 0;
}

int sdma_change_bd_status(dmac_t *dma_ch)
{
	struct sdma_chan *sdmac = (struct sdma_chan *)dma_ch;
	struct sdma_buffer_descriptor *bd;
	int bdnum, i, j = 0;

	if (!dma_ch)
		return 0;
	bd = sdmac->bd;
	bdnum = sdmac->bdnum;
	if (!bd)
		return 0;
	for (i = 0; i < bdnum; i++) {
		if (!(bd[i].mode.status & BD_DONE)) {
			bd[i].mode.status |= BD_DONE;
			bd[i].mode.count = dma_ch->period_len;
			j++;
		}
	}

	/*FIXME: return how many BD is consumed */
	return j;
}

void sdma_release(struct dma_device *dev)
{
	struct SDMA *sdma = (struct SDMA *)dev;
	int bdnum;
	struct sdma_buffer_descriptor *bd;
	int num;

	for (num = 0; num < 32; num++) {
		if (sdma->chan_info[num].used)
			return;
	}

	if (sdma->ccb) {
		xaf_free(sdma->ccb, 0);
		sdma->ccb = NULL;
	}
	if (sdma->bd0) {
		xaf_free(sdma->bd0, 0);
		sdma->bd0 = NULL;
	}

	for (num = 0; num < 32; num++) {
		if (sdma->chan_info[num].bd) {
			xaf_free(sdma->chan_info[num].bd, 0);
			sdma->chan_info[num].bd = NULL;
		}
	}
	xaf_free(sdma, 0);
	sdma = NULL;
}

dmac_t *request_sdma_chan(struct dma_device *dev, int dev_type)
{
	struct SDMA *sdma = (struct SDMA *)dev;
	struct sdma_chan *sdmac = NULL;
	/* channel0 is used internal */
	int ch_num = 1;
	while (ch_num < MAX_DMA_CHANNELS) {
		sdmac = &sdma->chan_info[ch_num];
		if (!sdmac->used) {
			sdmac->used = 1;
			sdmac->dma_chan.dma_device = dev;
			sdmac->channel_id = ch_num;
			break;
		}
		ch_num++;
	}
	if (ch_num >= MAX_DMA_CHANNELS)
		sdmac = NULL;
	return (dmac_t *)sdmac;
}

void release_sdma_chan(dmac_t *dma_chan)
{
	if (!dma_chan)
		return;
	struct sdma_chan *sdmac = (struct sdma_chan *)dma_chan;
	if (sdmac->bd)
		xaf_free(sdmac->bd, 0);
	memset(sdmac, 0, sizeof(struct sdma_chan));
}

dma_t *sdma_probe()
{
	struct SDMA *fsl_sdma = NULL;
	xaf_malloc((void **)&fsl_sdma, sizeof(struct SDMA), 0);
	if (!fsl_sdma)
		return NULL;
	memset(fsl_sdma, 0, sizeof(struct SDMA));

	fsl_sdma->regs             = (void *)SDMA_ADDR;
	fsl_sdma->Int              = SDMA_INT;

	fsl_sdma->dma.dma_type     = DMATYPE_SDMA;
	fsl_sdma->dma.get_para     = sdma_get_para;
	fsl_sdma->dma.init         = sdma_init;
	fsl_sdma->dma.release      = sdma_release;
	fsl_sdma->dma.irq_handler  = sdma_irq_handler;
	fsl_sdma->dma.suspend      = sdma_suspend;
	fsl_sdma->dma.resume       = sdma_resume;

	fsl_sdma->dma.request_dma_chan = request_sdma_chan;
	fsl_sdma->dma.release_dma_chan = release_sdma_chan;
	fsl_sdma->dma.chan_config  = sdma_chan_config;
	fsl_sdma->dma.chan_start   = sdma_chan_start;
	fsl_sdma->dma.chan_stop    = sdma_chan_stop;
	return (dma_t *)fsl_sdma;
}

