// SPDX-License-Identifier: BSD-3-Clause
/*
 * io.c - I/O abstraction layer
 *
 * Copyright 2018 NXP
 */
#include "io.h"

void write16(volatile void * addr, u16 data) {
	*(volatile unsigned short *)(addr) = (data);
}

u16 read16(volatile void * addr) {
	u16 read_data = *(volatile unsigned short *)(addr);
	return read_data;
}

void write32(volatile void * addr, u32 data) {
	*(volatile unsigned int *)(addr) = (data);
}

void write32_bit(volatile void * addr, u32 mask, u32 data) {
	u32 read_data = *(volatile unsigned int *)(addr);
	read_data = (read_data & (~mask)) | data;
	*(volatile unsigned int *)(addr) = (read_data);
}

u32 read32(volatile void * addr) {
	u32 read_data = *(volatile unsigned int *)(addr);
	return read_data;
}
