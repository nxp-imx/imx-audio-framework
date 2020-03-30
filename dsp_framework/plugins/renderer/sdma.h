#ifndef   _SDMA_H
#define   _SDMA_H
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



#include "sdma_script_code_imx7d_4_5.h"

/*
 * 0x78(SDMA_XTRIG_CONF2+4)~0x100(SDMA_CHNPRI_O) registers are reserved and
 * can't be accessed. Skip these register touch in suspend/resume. Also below
 * two macros are only used on i.mx6sx.
 */
#define MXC_SDMA_RESERVED_REG (SDMA_CHNPRI_0 - SDMA_XTRIG_CONF2 - 4)
#define MXC_SDMA_SAVED_REG_NUM (((SDMA_CHNENBL0_IMX35 + 4 * 48) - \
				MXC_SDMA_RESERVED_REG) / 4)

#define u32 unsigned int

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
	u32 pc     :14;
	u32 unused1: 1;
	u32 t      : 1;
	u32 rpc    :14;
	u32 unused0: 1;
	u32 sf     : 1;
	u32 spc    :14;
	u32 unused2: 1;
	u32 df     : 1;
	u32 epc    :14;
	u32 lm     : 2;
} __attribute__ ((packed));

/*
 * Mode/Count of data node descriptors - IPCv2
 */
struct sdma_mode_count {
#define SDMA_BD_MAX_CNT	0xffff
	u32 count   : 16; /* size of the buffer pointed by this BD */
	u32 status  :  8; /* E,R,I,C,W,D status bits stored here */
	u32 command :  8; /* command mostly used for channel 0 */
};

/*
 * Buffer descriptor
 */
struct sdma_buffer_descriptor {
	struct sdma_mode_count  mode;
	u32 buffer_addr;	/* address of the buffer described */
	u32 ext_buffer_addr;	/* extended buffer address */
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
	u32 current_bd_ptr;
	u32 base_bd_ptr;
	u32 unused[2];
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
	u32  gReg[8];
	u32  mda;
	u32  msa;
	u32  ms;
	u32  md;
	u32  pda;
	u32  psa;
	u32  ps;
	u32  pd;
	u32  ca;
	u32  cs;
	u32  dda;
	u32  dsa;
	u32  ds;
	u32  dd;
	u32  scratch0;
	u32  scratch1;
	u32  scratch2;
	u32  scratch3;
	u32  scratch4;
	u32  scratch5;
	u32  scratch6;
	u32  scratch7;
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

struct SDMA {
	//regs base addr
	volatile void			*regs;

	struct sdma_channel_control	*ccb;
	struct sdma_context_data	*context;
	struct sdma_buffer_descriptor	*bd0;
	struct sdma_buffer_descriptor	*bd1;

	u32				save_regs[MXC_SDMA_SAVED_REG_NUM];
	u32				fw_loaded;

};

int sdma_pre_init(struct SDMA* sdma, void *sdma_addr);
int sdma_init(struct SDMA *sdma, void *dest_addr, void *src_addr, int perioid_len);
int sdma_start(struct SDMA *sdma, int channel);
int sdma_stop(struct SDMA *sdma, int channel);
void sdma_irq_handler(struct SDMA * sdma, int period_len);
void sdma_resume(struct SDMA* sdma);
void sdma_suspend(struct SDMA* sdma);
void sdma_clearup(struct SDMA* sdma);
void sdma_change_bd_status(struct SDMA* sdma, unsigned int perioid_len);

#endif
