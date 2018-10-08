/*
 * io.h - I/O abstraction header API
 */
#ifndef _IO_H
#define _IO_H

#include "mydefs.h"

void write16(volatile void * addr, u16 data);
u16 read16(volatile void * addr);
void write32(volatile void * addr, u32 data);
void write32_bit(volatile void * addr, u32 mask, u32 data);
u32 read32(volatile void * addr);

#endif
