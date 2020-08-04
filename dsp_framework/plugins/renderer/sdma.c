/*
 * Copyright 2020 NXP
 *
 * sdma.c - SDMA driver
 */
#include "sdma.h"
#include "sdma_script_code_imx7d_4_5.h"

#include "xf-debug.h"

#define BIT(nr)			(1 << (nr))

#define PAGE_SIZE 0x400
#define SDMA_H_CONFIG_DSPDMA	BIT(12) /* indicates if the DSPDMA is used */
#define SDMA_H_CONFIG_RTD_PINS	BIT(11) /* indicates if Real-Time Debug pins are enabled */
#define SDMA_H_CONFIG_ACR	BIT(4)  /* indicates if AHB freq /core freq = 2 or 1 */
#define SDMA_H_CONFIG_CSM	(3)       /* indicates which context switch mode is selected*/

#define MAX_DMA_CHANNELS 32
#define MXC_SDMA_DEFAULT_PRIORITY 1
#define MXC_SDMA_MIN_PRIORITY 1
#define MXC_SDMA_MAX_PRIORITY 7

#define BD_SIZE 0x80000

#define false 0
#define true 1


static void sdma_enable_channel(struct SDMA *sdma, int channel)
{
	/* 1. Neither HSTART[i] bit can be set while
	 * the corresponding HE[i] bit is cleared.
	 * 2. When the Arm platform tries to set the HSTART[i] bit
	 * by writing a one (if the corresponding HE[i] bit is clear),
	 * the bit in the HSTART[i] register will remain cleared and
	 * the HE[i] bit will be set. */

	write32_bit(sdma->regs + SDMA_H_START, BIT(channel), BIT(channel));
}

/*
 * sdma_run_channel0 - run a channel and wait till it's done
 */
static int sdma_run_channel0(struct SDMA *sdma)
{
	unsigned int ret = 0, val, count;
	u32 reg;

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
	if (!channel)
		sdma->bd0 = (struct sdma_buffer_descriptor *)MEM_scratch_ua_malloc(PAGE_SIZE);
	else
		sdma->chan_info[channel].bd = (struct sdma_buffer_descriptor *)MEM_scratch_ua_malloc(sizeof(struct sdma_buffer_descriptor) * bdnum);
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

	sdma->ccb[0].base_bd_ptr = (u32)sdma->bd0;
	sdma->ccb[0].current_bd_ptr = (u32)sdma->bd0;

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
	unsigned int *buf_phy = (unsigned int *)MEM_scratch_ua_malloc(size);

	memcpy(buf_phy, buf, size);

	bd0->mode.command = C0_SETPM;
	bd0->mode.status = BD_DONE | BD_WRAP | BD_EXTD;
	bd0->mode.count = size / 2;
	bd0->buffer_addr = (int)buf_phy;
	bd0->ext_buffer_addr = address;

	sdma_run_channel0(sdma);

	MEM_scratch_ua_mfree(buf_phy);

	sdma->fw_loaded = true;
	return 0;
}

int sdma_pre_init(struct SDMA *sdma, void *sdma_addr)
{
	unsigned int ccb_phys, bd0_phys;
	struct sdma_buffer_descriptor *bd0;
	unsigned int ccbsize;
	int count = 0, i, val;
	int channel = 0, reg;
	unsigned int reset;

	sdma->regs = (volatile void*)sdma_addr;
	/* reset */
	write32(sdma->regs + SDMA_H_RESET, 1);
	do {
		reset = read32(sdma->regs + SDMA_H_RESET);
	} while(reset == 1);
	/* Be sure SDMA has not started yet */
	write32(sdma->regs + SDMA_H_C0PTR, 0);

	ccbsize = MAX_DMA_CHANNELS * (sizeof(struct sdma_channel_control)
		+ sizeof(struct sdma_context_data));
	sdma->ccb = (struct sdma_channel_control *)MEM_scratch_ua_malloc(ccbsize);
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
	load_firmware(sdma, (void *)sdma_code, RAM_CODE_SIZE * sizeof(short), RAM_CODE_START_ADDR);

	LOG("sdma pre init\n");
	return 0;
}

static void sdma_event_enable(struct SDMA *sdma, int event, unsigned int channel, unsigned int sw_done, unsigned int done_cfg)
{
	unsigned int chnenbl;
	unsigned int val;

	if (event < 0)
		return;
	chnenbl = SDMA_CHNENBL0_IMX35 + 4 * event;
	val = read32(sdma->regs + chnenbl);
	__set_bit(channel, &val);
	write32(sdma->regs + chnenbl, val);

	/* Set SDMA_DONEx_CONFIG is sw_done enabled */
	if (sw_done) {
		val = read32(sdma->regs + SDMA_DONE0_CONFIG);
		__set_bit(done_cfg, &val);
		write32(sdma->regs + SDMA_DONE0_CONFIG, val);
	}
}
static int sdma_disable_channel(struct SDMA *sdma, unsigned int channel)
{
	write32_bit(sdma->regs + SDMA_H_STATSTOP, BIT(channel), BIT(channel));
	return 0;
}

int sdma_init(struct SDMA * sdma, int type, void *dest_addr, void *src_addr, int channel, int period_len) {
	unsigned int ret;
	unsigned int val;
	unsigned int chnenbl;
	int event, event2;
	unsigned int done_cfg;
	struct sdma_buffer_descriptor *bd0 = sdma->bd0;
	struct sdma_buffer_descriptor *bd1;
	struct sdma_context_data *context;
	unsigned int context_phys;
	unsigned int script_addr;
	unsigned int r0, r1, r2, r6, r7;

	event = -1;
	event2 = -1;

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
	}
	/* set channel priority: 1 */
	write32(sdma->regs + SDMA_CHNPRI_0 + 4 * channel, 1);

	/* cfg for PDM, not used for audio */
	done_cfg = 0x00;
	sdma_event_enable(sdma, event, channel, 0, done_cfg);
	sdma_event_enable(sdma, event2, channel, 0, done_cfg);

	if (type == DMA_MEM_TO_DEV) {
		sdma_channel_attach_bd(sdma, channel, 2);
		/* set 2 bds for transmite data */
		bd1 = sdma->chan_info[channel].bd;
		bd1->mode.command = 2;
		bd1->mode.status = BD_DONE | BD_INTR | BD_CONT;
		bd1->mode.count = period_len;
		bd1->buffer_addr = (unsigned int)src_addr;

		bd1 = &bd1[1];
		bd1->mode.command = 2;
		bd1->mode.status = BD_DONE | BD_WRAP | BD_CONT + BD_INTR;
		bd1->mode.count = period_len;
		bd1->buffer_addr = (unsigned int)(src_addr + period_len);
	} else {
		sdma_channel_attach_bd(sdma, channel, 1);
		bd1 = sdma->chan_info[channel].bd;

		bd1->mode.command = 2;
		bd1->mode.status = BD_DONE | BD_WRAP | BD_CONT;
		bd1->mode.count = 64;
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
	ret = sdma_run_channel0(sdma);

	sdma->ccb[channel].base_bd_ptr = (unsigned int)sdma->chan_info[channel].bd;
	sdma->ccb[channel].current_bd_ptr = (unsigned int)sdma->chan_info[channel].bd;

	return 0;
}

int sdma_start(struct SDMA* sdma, int channel)
{
	sdma_enable_channel(sdma, channel);
	return 0;
}

int sdma_stop(struct SDMA* sdma, int channel)
{
	sdma_disable_channel(sdma, channel);
	return 0;
}

int sdma_irq_handler(struct SDMA * sdma, int period_len) {

	unsigned long stat;
	stat = read32(sdma->regs + SDMA_H_INTR);
	write32(sdma->regs + SDMA_H_INTR, stat);

	/* channel 0 is special and not handled here, see run_channel0() */
	stat &= ~1;

	/* XXX: modify BD_DONE in here maybe cause sdma copy data
	 * that not updated yet. */
	return sdma_change_bd_status(sdma, period_len);
}

static void sdma_reg_dump(struct SDMA* sdma)
{
	int i, count = 0;
	for (i = 0; i < MXC_SDMA_SAVED_REG_NUM;) {
		LOG2("sdma reg %d, %x\n", i, sdma->save_regs[i]);
		i++;
	}
}

static int sdma_save_restore_context(struct SDMA *sdma, u32 save)
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

void sdma_resume(struct SDMA* sdma)
{
	LOG("sdma resume\n");
	int ret, i;
	int channel = 0;

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
	return;
}

void sdma_suspend(struct SDMA* sdma)
{
	LOG("sdma suspend\n");
	u32 ret;
	int i;

	ret = sdma_save_restore_context(sdma, true);
	if (ret) {
		LOG1("save context error %x\n", ret);
		return;
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
}

int sdma_change_bd_status(struct SDMA* sdma, unsigned int period_len)
{
	struct sdma_buffer_descriptor *bd;
	int chan_num, bdnum, i, j = 0;

	for (chan_num = 1; chan_num < 32; chan_num++) {
		bd = sdma->chan_info[chan_num].bd;
		bdnum = sdma->chan_info[chan_num].bdnum;
		if (!bd)
			continue;
		for (i = 0; i < bdnum; i++) {
			if (!(bd[i].mode.status & BD_DONE)) {
				bd[i].mode.status |= BD_DONE;
				bd[i].mode.count = period_len;
				j++;
			}
		}
	}

	/*FIXME: return how many BD is consumed */
	return j;
}
void sdma_clearup(struct SDMA* sdma)
{
	int bdnum;
	struct sdma_buffer_descriptor *bd;
	int num;

	if (sdma->ccb)
		MEM_scratch_ua_mfree(sdma->ccb);
	if (sdma->bd0)
		MEM_scratch_ua_mfree(sdma->bd0);

	for (num = 0; num < 32; num++) {
		if (sdma->chan_info[num].bd)
			MEM_scratch_ua_mfree(sdma->chan_info[num].bd);
	}
}
