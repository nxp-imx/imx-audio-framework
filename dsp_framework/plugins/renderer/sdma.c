/*
 * sdma.c - SDMA driver
 */
#include "sdma.h"
#include "sdma_script_code_imx7d_4_5.h"

#include "xf-debug.h"

#define BIT(nr)			(1 << (nr))

#define PAGE_SIZE 0x4000
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

	write32(sdma->regs + SDMA_H_START, BIT(channel));
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

static int sdma_request_channel0(struct SDMA *sdma)
{
	int ret = 0;
	int channel = 0;

	sdma->bd0 = (struct sdma_buffer_descriptor *)MEM_scratch_ua_malloc(PAGE_SIZE);
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

static void sdma_event_enable(struct SDMA *sdma, unsigned int event, unsigned int channel, unsigned int sw_done, unsigned int done_cfg)
{
	unsigned int chnenbl;
	unsigned int val;

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

int sdma_init(struct SDMA * sdma, void *dest_addr, void *src_addr, int period_len) {
	unsigned int ret;
	unsigned int val;
	unsigned int channel = 1;
	unsigned int chnenbl;
	unsigned int event = 5;
	unsigned int done_cfg;
	struct sdma_buffer_descriptor *bd0 = sdma->bd0;
	struct sdma_buffer_descriptor *bd1;
	struct sdma_buffer_descriptor *bd2;
	struct sdma_context_data *context;
	unsigned int context_phys;

	LOG("sdma init start\n");
	/* set channel 1 priority: 1 */
	write32(sdma->regs + SDMA_CHNPRI_0 + 4 * channel, 1);

	/* cfg for PDM, not used for audio */
	done_cfg = 0x00;
	sdma_event_enable(sdma, event, channel, 0, done_cfg);
	bd1 = sdma->bd1 = (struct sdma_buffer_descriptor *)MEM_scratch_ua_malloc(sizeof(struct sdma_buffer_descriptor) * 2);
	bd2 = (struct sdma_buffer_descriptor *)&bd1[1];

	sdma_disable_channel(sdma, channel);

	/* set starting channel triggered by DMA request event
	 * set EO = 0, HO , DO = 1 */
	sdma_config_ownership(sdma, channel, true, true, false);

	memset(sdma->context, 0, sizeof(*sdma->context));
	context = sdma->context;
	context_phys = (unsigned int)context;
	context->channel_state.pc = mcu_2_app_ADDR;
	context->gReg[0] = 0;
	context->gReg[1] = 0x20;
	context->gReg[2] = 0;
	context->gReg[6] = (unsigned int)dest_addr;
	context->gReg[7] = 8;

	bd0->mode.command = C0_SETDM;
	bd0->mode.status = BD_DONE | BD_WRAP | BD_EXTD;
	bd0->mode.count = sizeof(*context) / 4;
	bd0->buffer_addr = context_phys;
	bd0->ext_buffer_addr = 2048 + (sizeof(*context) / 4) * channel;
	ret = sdma_run_channel0(sdma);

	/* set 2 bds for transmite data */
	bd1->mode.command = 2;
	bd1->mode.status = BD_DONE | BD_INTR | BD_CONT;
	bd1->mode.count = period_len;
	bd1->buffer_addr = (unsigned int)src_addr;

	bd2->mode.command = 2;
	bd2->mode.status = BD_DONE | BD_WRAP | BD_CONT + BD_INTR;
	bd2->mode.count = period_len;
	bd2->buffer_addr = (unsigned int)(src_addr + period_len);

	sdma->ccb[channel].base_bd_ptr = (unsigned int)sdma->bd1;
	sdma->ccb[channel].current_bd_ptr = (unsigned int)sdma->bd1;

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

void sdma_irq_handler(struct SDMA * sdma, int period_len) {

	unsigned long stat;
	stat = read32(sdma->regs + SDMA_H_INTR);
	write32(sdma->regs + SDMA_H_INTR, stat);

	/* channel 0 is special and not handled here, see run_channel0() */
	stat &= ~1;

	/* XXX: modify BD_DONE in here maybe cause sdma copy data
	 * that not updated yet. */
	sdma_change_bd_status(sdma, period_len);
	return;
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

void sdma_change_bd_status(struct SDMA* sdma, unsigned int period_len)
{
	struct sdma_buffer_descriptor *bd1 = sdma->bd1;
	struct sdma_buffer_descriptor *bd2;
	bd2 = (struct sdma_buffer_descriptor *)&bd1[1];

	if (!(bd1->mode.status & BD_DONE)) {
		bd1->mode.status |= BD_DONE;
		bd1->mode.count = period_len;
	}
	if (!(bd2->mode.status & BD_DONE)) {
		bd2->mode.status |= BD_DONE;
		bd2->mode.count = period_len;
	}
	return;
}
void sdma_clearup(struct SDMA* sdma)
{
	if (sdma->ccb)
		MEM_scratch_ua_mfree(sdma->ccb);
	if (sdma->bd0)
		MEM_scratch_ua_mfree(sdma->bd0);
	if (sdma->bd0)
		MEM_scratch_ua_mfree(sdma->bd1);
	return;
}
