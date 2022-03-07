/*
 * Copyright 2018-2020 NXP
 *
 * wm8960.c - wm8960 driver
 */

#include "mydefs.h"
#include "io.h"
#include "i2c.h"
#include "wm8960.h"
#include "debug.h"

uint32_t reg_cache[CODEC_CACHEREGNUM] = {
	/* default val */
	0x0097, 0x0097, 0x0000, 0x0000, 0x0000, 0x0008, 0x0000, 0x000a, 0x01c0, 0x0000, 0x00ff, 0x00ff, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x007b, 0x0100, 0x0032, 0x0000, 0x00c3, 0x00c3, 0x01c0, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0100, 0x0100, 0x0050, 0x0050, 0x0050, 0x0050, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0040, 0x0000, 0x0000, 0x0050, 0x0050, 0x0000, 0x0002, 0x0037, 0x004d, 0x0080, 0x0008, 0x0031, 0x0026, 0x00e9,
};

void WM8960_dump()
{
	int i;
	for (i = 0; i < CODEC_CACHEREGNUM; i++)
		LOG2("wm8960 reg[%x]: %x\n", i, reg_cache[i]);
}

void WM8960_Init()
{
	WM8960_WriteReg(CODEC_RESET, 0x00);

	/* ===== Enable headphone jack detect ===== */
	/* HPSEL: JD3 */
	WM8960_ModifyReg(CODEC_ADDCTL4, 3<<2, 3<<2);
	/* HPSWEN:1, HPSWPOL:0 */
        WM8960_ModifyReg(CODEC_ADDCTL2, 3<<5, 2<<5);
	/* TOCLKSEL:1, TOEN:1 */
	WM8960_ModifyReg(CODEC_ADDCTL1, 3, 3);

	/* CLKSEL:0(MCLK), SYSCLKDIV:00(1),
	 * DACDIV:000(SYSCLK/(1*256))
	 */
	WM8960_WriteReg(CODEC_CLOCK1, 0x0);

	/* MS:0(SLAVE MODE), WL:00(16bit), FORMAT:10(I2S) */
	WM8960_WriteReg(CODEC_IFACE1, 0x02);

	/* VMIDSEL:01(For playback/record), VREF:1 */
	WM8960_WriteReg(CODEC_POWER1, 0xC0);
	/* DACR:1, DACL:1, LOUT1:1, ROUT1:1, SPKL:1, SPLR:1 */
	WM8960_WriteReg(CODEC_POWER2, 0x1F8);
	/* LOMIX:1, ROMIX:1 */
	WM8960_WriteReg(CODEC_POWER3, 0xC);

	/* DIGITAL DAC VOLUME CONTROL */
	WM8960_WriteReg(CODEC_LDAC, 0x1EB);
	WM8960_WriteReg(CODEC_RDAC, 0x1EB);

	/* DACMU:0(un mute) */
	WM8960_WriteReg(CODEC_DACCTL1, 0x0);
	WM8960_WriteReg(CODEC_LINVOL, 0x123);
	WM8960_WriteReg(CODEC_RINVOL, 0x123);

	/* DMONOMIX:0(Stereo) */
        WM8960_ModifyReg(CODEC_ADDCTL1, 1<<4, 0);

	/* LD2LO:1 */
	WM8960_ModifyReg(CODEC_LOUTMIX, 1<<8, 1<<8);
	WM8960_ModifyReg(CODEC_ROUTMIX, 1<<8, 1<<8);

	/* OUT1VU:1(Update left and right channel gains)
	 * LO1ZC:0(Left zero cross enable)
	 * LOUT1VOL:6e(LOUT1 Volume)
	 * */
	WM8960_WriteReg(CODEC_LOUT1, 0x16e);
	WM8960_WriteReg(CODEC_ROUT1, 0x16e);
}

/* A control word consists of 16 bits. The first 7 bits (B15 to B9) are address bits
 * that select which control register is accessed. The remaining 9 bits (B8 to B0) are
 * data bits.
 */
int WM8960_WriteReg(uint8_t reg, uint16_t val)
{
	int ret;
	uint8_t ctrl_data;
	uint8_t ctrl_data2;
	uint32_t slave_addr = BUS_CODEC_ADDR;
	/* Copy data to cache */
	reg_cache[reg] = val;

	/* 7bit addr + 1bit data */
	ctrl_data = (reg << 1) | ((val & 0x01ff) >> 8);

	/* remain 8bit data */
	ctrl_data2 = val & 0x00ff;

	ret = i2c_transfer_data(slave_addr, ctrl_data, ctrl_data2);
	return ret;
}

int WM8960_ReadReg(uint8_t reg, uint16_t *val)
{
	if (reg >= CODEC_CACHEREGNUM)
	{
		return -1;
	}

	*val = reg_cache[reg];

	return 0;
}

int WM8960_ModifyReg(uint8_t reg, uint16_t mask, uint16_t val)
{
	int ret   = 0;
	uint16_t reg_val = 0;

	WM8960_ReadReg(reg, &reg_val);

	reg_val &= (uint16_t)~mask;
	reg_val |= val;

	ret = WM8960_WriteReg(reg, reg_val);

	return ret;
}
