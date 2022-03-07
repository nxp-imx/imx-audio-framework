/*
 * Copyright 2018-2020 NXP
 *
 * i2c.c - i2c driver
 */

#include "mydefs.h"
#include "board.h"
#include "io.h"
#include "i2c.h"
#include "debug.h"

struct imx_i2c_clk_pair {
	u16	div;
	u16	val;
};

#define NUM_DIV 50

static struct imx_i2c_clk_pair imx_i2c_clk_div[NUM_DIV] = {
	{ 22,   0x20 }, { 24,   0x21 }, { 26,   0x22 }, { 28,   0x23 },
	{ 30,   0x00 }, { 32,   0x24 }, { 36,   0x25 }, { 40,   0x26 },
	{ 42,   0x03 }, { 44,   0x27 }, { 48,   0x28 }, { 52,   0x05 },
	{ 56,   0x29 }, { 60,   0x06 }, { 64,   0x2A }, { 72,   0x2B },
	{ 80,   0x2C }, { 88,   0x09 }, { 96,   0x2D }, { 104,  0x0A },
	{ 112,  0x2E }, { 128,  0x2F }, { 144,  0x0C }, { 160,  0x30 },
	{ 192,  0x31 }, { 224,  0x32 }, { 240,  0x0F }, { 256,  0x33 },
	{ 288,  0x10 }, { 320,  0x34 }, { 384,  0x35 }, { 448,  0x36 },
	{ 480,  0x13 }, { 512,  0x37 }, { 576,  0x14 }, { 640,  0x38 },
	{ 768,  0x39 }, { 896,  0x3A }, { 960,  0x17 }, { 1024, 0x3B },
	{ 1152, 0x18 }, { 1280, 0x3C }, { 1536, 0x3D }, { 1792, 0x3E },
	{ 1920, 0x1B }, { 2048, 0x3F }, { 2304, 0x1C }, { 2560, 0x1D },
	{ 3072, 0x1E }, { 3840, 0x1F }
};

static void i2c_write(int reg, uint32_t val)
{
	write16((volatile unsigned short *)(I2C_ADDR + (reg << (OFFSET))), val);
}

static uint32_t i2c_read(int reg)
{
	return read16((volatile unsigned short *)(I2C_ADDR + (reg << (OFFSET))));
}

static void i2c_regdump()
{
	LOG1("I2C_IADR %x\n", i2c_read(IMX_I2C_IADR));
	LOG1("I2C_IFDR %x\n", i2c_read(IMX_I2C_IFDR));
	LOG1("I2C_I2CR %x\n", i2c_read(IMX_I2C_I2CR));
	LOG1("I2C_I2SR %x\n", i2c_read(IMX_I2C_I2SR));
	LOG1("I2C_I2DR %x\n\n", i2c_read(IMX_I2C_I2DR));
}

static void delay(int num)
{
	volatile int count = 0;
	volatile int temp = 0;

	for (; temp < num; temp++) {
		count = 0;
		while (count < 100)
			count++;
	}
}

static int i2c_clear_Int()
{
	uint32_t val = 0;
	uint32_t status;

	status = i2c_read(IMX_I2C_I2SR);
	while(!(status & I2SR_IIF)) {
		delay(50);
		status = i2c_read(IMX_I2C_I2SR);
	}
	val = status & (~I2SR_IIF);
	i2c_write(IMX_I2C_I2SR, val);
	status = i2c_read(IMX_I2C_I2SR);
	return 0;
}

static int i2c_imx_acked()
{
	delay(50);
	if (i2c_read(IMX_I2C_I2SR) & I2SR_RXAK) {
		LOG("I2C No ACK\n");
		return -ENXIO;  /* No ACK */
	}

	return 0;
}

static int i2c_get_ifdr()
{
	uint32_t div = I2C_CLK / I2C_BITRATE;
	int i = 0;

	if (div < imx_i2c_clk_div[0].div)
		i = 0;
	else if (div > imx_i2c_clk_div[NUM_DIV -1].div)
		i = NUM_DIV - 1;
	else
		for (i = 0; imx_i2c_clk_div[i].div < div; i++)
			;

	return imx_i2c_clk_div[i].div;
}

int i2c_transfer_data(uint32_t slave_addr, int reg, char data)
{
	int result = 0;
	uint32_t val;

	i2c_start();

	val = (slave_addr << 1) | WD;
	/* write 7-bit adress + R/W bit */
	i2c_write(IMX_I2C_I2DR, val);
	result = i2c_clear_Int();
	if (result)
		return result;
	result = i2c_imx_acked();
	if (result)
		goto fail;

	delay(50);
	/* write data */
	i2c_write(IMX_I2C_I2DR, reg);
	result = i2c_clear_Int();
	if (result)
		return result;
	result = i2c_imx_acked();
	if (result)
		goto fail;
	delay(50);

	i2c_write(IMX_I2C_I2DR, data);
	result = i2c_clear_Int();
	if (result)
		return result;
	result = i2c_imx_acked();
	if (result)
		goto fail;

fail:
	i2c_stop();

	return result;
}

void i2c_init()
{
}

void i2c_start()
{
	uint32_t val = 0;
	uint32_t i2c_busy = 0;
	int ifdr = i2c_get_ifdr();

	i2c_write(IMX_I2C_IFDR, ifdr);
	/* Enable I2C controller */
	i2c_write(IMX_I2C_I2SR, 0);
	i2c_write(IMX_I2C_I2CR, I2CR_IEN);

	/* Start singal */
	val = i2c_read(IMX_I2C_I2CR);
	val |= I2CR_MSTA;
	i2c_write(IMX_I2C_I2CR, val);

	val |= I2CR_MTX | I2CR_TXAK;
	val &= ~I2CR_DMAEN;
	i2c_write(IMX_I2C_I2CR, val);
}

void i2c_stop()
{
	uint32_t val = 0;

	/* Stop singal */
	val = i2c_read(IMX_I2C_I2CR);
	val &= ~(I2CR_MSTA);
	i2c_write(IMX_I2C_I2CR, val);

	/* need delay to wait generate stop singal */
	delay(50);
	/* Disable I2C controller */
	val = i2c_read(IMX_I2C_I2CR);
	val &= ~I2CR_IEN;
	i2c_write(IMX_I2C_I2CR, val);
	delay(50);
}

void i2c_reset()
{
	i2c_write(IMX_I2C_I2CR, ~I2CR_IEN);
	i2c_write(IMX_I2C_I2SR, 0);
}
