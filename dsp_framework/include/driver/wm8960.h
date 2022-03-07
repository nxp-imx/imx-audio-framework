/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef _WM8960_H
#define _WM8960_H

/* CODEC register space */

#define CODEC_CACHEREGNUM 	56

#define CODEC_LINVOL		0x0
#define CODEC_RINVOL		0x1
#define CODEC_LOUT1		0x2
#define CODEC_ROUT1		0x3
#define CODEC_CLOCK1		0x4
#define CODEC_DACCTL1		0x5
#define CODEC_DACCTL2		0x6
#define CODEC_IFACE1		0x7
#define CODEC_CLOCK2		0x8
#define CODEC_IFACE2		0x9
#define CODEC_LDAC		0xa
#define CODEC_RDAC		0xb

#define CODEC_RESET		0xf
#define CODEC_3D		0x10
#define CODEC_ALC1		0x11
#define CODEC_ALC2		0x12
#define CODEC_ALC3		0x13
#define CODEC_NOISEG		0x14
#define CODEC_LADC		0x15
#define CODEC_RADC		0x16
#define CODEC_ADDCTL1		0x17
#define CODEC_ADDCTL2		0x18
#define CODEC_POWER1		0x19
#define CODEC_POWER2		0x1a
#define CODEC_ADDCTL3		0x1b
#define CODEC_APOP1		0x1c
#define CODEC_APOP2		0x1d

#define CODEC_LINPATH		0x20
#define CODEC_RINPATH		0x21
#define CODEC_LOUTMIX		0x22

#define CODEC_ROUTMIX		0x25
#define CODEC_MONOMIX1		0x26
#define CODEC_MONOMIX2		0x27
#define CODEC_LOUT2		0x28
#define CODEC_ROUT2		0x29
#define CODEC_MONO		0x2a
#define CODEC_INBMIX1		0x2b
#define CODEC_INBMIX2		0x2c
#define CODEC_BYPASS1		0x2d
#define CODEC_BYPASS2		0x2e
#define CODEC_POWER3		0x2f
#define CODEC_ADDCTL4		0x30
#define CODEC_CLASSD1		0x31

#define CODEC_CLASSD3		0x33
#define CODEC_PLL1		0x34
#define CODEC_PLL2		0x35
#define CODEC_PLL3		0x36
#define CODEC_PLL4		0x37

/*
 * CODEC Clock dividers
 */
#define CODEC_SYSCLKDIV 		0
#define CODEC_DACDIV			1
#define CODEC_OPCLKDIV			2
#define CODEC_DCLKDIV			3
#define CODEC_TOCLKSEL			4

#define CODEC_SYSCLK_DIV_1		(0 << 1)
#define CODEC_SYSCLK_DIV_2		(2 << 1)

#define CODEC_SYSCLK_MCLK		(0 << 0)
#define CODEC_SYSCLK_PLL		(1 << 0)
#define CODEC_SYSCLK_AUTO		(2 << 0)

#define CODEC_DAC_DIV_1		(0 << 3)
#define CODEC_DAC_DIV_1_5		(1 << 3)
#define CODEC_DAC_DIV_2		(2 << 3)
#define CODEC_DAC_DIV_3		(3 << 3)
#define CODEC_DAC_DIV_4		(4 << 3)
#define CODEC_DAC_DIV_5_5		(5 << 3)
#define CODEC_DAC_DIV_6		(6 << 3)

#define CODEC_DCLK_DIV_1_5		(0 << 6)
#define CODEC_DCLK_DIV_2		(1 << 6)
#define CODEC_DCLK_DIV_3		(2 << 6)
#define CODEC_DCLK_DIV_4		(3 << 6)
#define CODEC_DCLK_DIV_6		(4 << 6)
#define CODEC_DCLK_DIV_8		(5 << 6)
#define CODEC_DCLK_DIV_12		(6 << 6)
#define CODEC_DCLK_DIV_16		(7 << 6)

#define CODEC_TOCLK_F19		(0 << 1)
#define CODEC_TOCLK_F21		(1 << 1)

#define CODEC_OPCLK_DIV_1		(0 << 0)
#define CODEC_OPCLK_DIV_2		(1 << 0)
#define CODEC_OPCLK_DIV_3		(2 << 0)
#define CODEC_OPCLK_DIV_4		(3 << 0)
#define CODEC_OPCLK_DIV_5_5		(4 << 0)
#define CODEC_OPCLK_DIV_6		(5 << 0)

#define IMX8MP_BUS_CODEC_ADDR		0x1A
#define BUS_CODEC_ADDR			IMX8MP_BUS_CODEC_ADDR

void WM8960_dump();
void WM8960_Init();
int WM8960_WriteReg(uint8_t reg, uint16_t val);
int WM8960_ReadReg(uint8_t reg, uint16_t *val);
int WM8960_ModifyReg(uint8_t reg, uint16_t mask, uint16_t val);

#endif /* CODEC HEADER */
