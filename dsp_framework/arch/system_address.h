/*****************************************************************
 * Copyright (c) 2017 Cadence Design Systems, Inc.
 * Copyright 2018 NXP
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************/

#ifndef _SYSTEMADDRESS_H_
#define _SYSTEMADDRESS_H_

#include <stddef.h>
#include <xtensa/config/core.h>
#include <xtensa/config/system.h>

/**********************************
 * System Memory Map
 **********************************/
#define XSHAL_MU5_SIDEB_BYPASS_PADDR  0x5D290000
#define XSHAL_MU6_SIDEB_BYPASS_PADDR  0x5D2A0000
#define XSHAL_MU7_SIDEB_BYPASS_PADDR  0x5D2B0000
#define XSHAL_MU8_SIDEB_BYPASS_PADDR  0x5D2C0000
#define XSHAL_MU9_SIDEB_BYPASS_PADDR  0x5D2D0000
#define XSHAL_MU10_SIDEB_BYPASS_PADDR 0x5D2E0000
#define XSHAL_MU11_SIDEB_BYPASS_PADDR 0x5D2F0000
#define XSHAL_MU12_SIDEB_BYPASS_PADDR 0x5D300000
#define XSHAL_MU13_SIDEB_BYPASS_PADDR 0x5D310000
#define XSHAL_MU13_SIDEB_BYPASS_PADDR_8MP 0x30E70000
#define XSHAL_MU3_SIDEB_BYPASS_PADDR_8ULP 0x2DA20000

#ifdef PLATF_8M
#define MU_PADDR  XSHAL_MU13_SIDEB_BYPASS_PADDR_8MP
#else
#ifdef PLATF_8ULP
#define MU_PADDR  XSHAL_MU3_SIDEB_BYPASS_PADDR_8ULP
#else
#define MU_PADDR  XSHAL_MU13_SIDEB_BYPASS_PADDR
#endif
#endif

#define  XSHAL_MU5_SIDEB_BTR0  (MU_PADDR + 0x00)
#define  XSHAL_MU5_SIDEB_BTR1  (MU_PADDR + 0x04)
#define  XSHAL_MU5_SIDEB_BTR2  (MU_PADDR + 0x08)
#define  XSHAL_MU5_SIDEB_BTR3  (MU_PADDR + 0x0C)
#define  XSHAL_MU5_SIDEB_BRR0  (MU_PADDR + 0x10)
#define  XSHAL_MU5_SIDEB_BRR1  (MU_PADDR + 0x14)
#define  XSHAL_MU5_SIDEB_BRR2  (MU_PADDR + 0x18)
#define  XSHAL_MU5_SIDEB_BRR3  (MU_PADDR + 0x1C)
#define  XSHAL_MU5_SIDEB_BSR   (MU_PADDR + 0x20)
#define  XSHAL_MU5_SIDEB_BCR   (MU_PADDR + 0x24)

/* Destination is DPU's local DataRAMs from where DPU processes input data
 * blocks. In this example, the macros XCHAL_DATARAM(0/1)_VADDR are same on
 * both APU and DPU.  If DPU's DataRAM area is mapped to different addresses
 * then modify the macros DEST(0/1)_ADDR accordingly.
 */
#define DEST0_ADDR (XCHAL_DATARAM0_VADDR + XCHAL_DATARAM0_SIZE / 2)	// 0x3FFE 0000 + 0x0001 0000
#define DEST1_ADDR (XCHAL_DATARAM1_VADDR + XCHAL_DATARAM1_SIZE / 2)	// 0x3FFC 0000 + 0x0001 0000

#if XSHAL_SIMIO_BYPASS_VADDR1
/* Start of an address space, which is safe to use for external components in Simulations  */
/* For the HiFi 2 core in RD4 release XSHAL_SIMIO_BYPASS_VADDR is set to 0xC0000000       */
/* See file system.h in the <cores_name build dir>/xtensa-elf/arch/include/xtensa/config   */

#define GMEM_VADDR XSHAL_SIMIO_BYPASS_VADDR

#define DMA_NOTIFY_ADDR_APU			(GMEM_VADDR + 0x00)	  // RW by APU_DMAC: 4 Byte: DMA Notification address
#define INT_ICM_APU_RX_MMIO_ADDR	(GMEM_VADDR + 0x04)	  // RW by APU, DPU; 0xC0000004
#define INT_ICM_DPU_RX_MMIO_ADDR	(GMEM_VADDR + 0x08)	  // RW by APU, DPU; 0xC0000008
#define	DPU_SYS_IF_REG				(GMEM_VADDR + 0x0C)	  // RW by APU;  	0xC000000C
#define DMA_NOTIFY_ADDR_DPU			(GMEM_VADDR + 0x10)	  // RW by DPU_DMAC: 4 Byte: DMA Notification address
#define DMAC_RESOURCE_LOCK			(GMEM_VADDR + 0x14)	  // DMAC Lock

#define ICMINT_APU_RESOURCE_LOCK	(GMEM_VADDR + 0x18)	  // APU's icm interrupt
#define ICMINT_DPU_RESOURCE_LOCK	(GMEM_VADDR + 0x1C)	  // DPU's icm interrupt

#define DMA_REQ_ADDR_APU			(GMEM_VADDR + 0x1000) // RW by APU: DMA Request register set
#define DMA_DESC_ADDR_APU			(GMEM_VADDR + 0x1100) // RW by APU: DMA Descriptors

#define DATA_SRC_ADDR 				(GMEM_VADDR + 0x2000) // RW by APU: For DMA example code.

#define ICM_MSG_BASE_ADDR_DPU		(GMEM_VADDR + 0x3000)
#define ICM_MSG_BASE_ADDR_APU		(GMEM_VADDR + 0x4000)

/* Destination is DPU's local DataRAMs from where DPU processes input data
 * blocks. In this example, the macros XCHAL_DATARAM(0/1)_VADDR are same on
 * both APU and DPU.  If DPU's DataRAM area is mapped to different addresses
 * then modify the macros DEST(0/1)_ADDR accordingly.
 */
#define DEST0_ADDR (XCHAL_DATARAM0_VADDR + XCHAL_DATARAM0_SIZE / 2)	// 0x3FFE 0000 + 0x0001 0000
#define DEST1_ADDR (XCHAL_DATARAM1_VADDR + XCHAL_DATARAM1_SIZE / 2)	// 0x3FFC 0000 + 0x0001 0000
#endif

#ifndef DPU_LOCAL_MEM_DIRECT_MAP
	// Following are re-mapped to DPU's local memories by Inbound PIF Arbiter.
	// These macros should be used by APU and DMAC only.
	#define DPU_IRAM0_BASE_ADDR		0xA0040000		//	0x40000000 - 0x4001FFFF
	#define DPU_DRAM0_BASE_ADDR		0xA0020000		//	0x3FFE0000 - 0x3FFFFFFF
	#define DPU_DRAM1_BASE_ADDR		0xA0000000		//	0x3FFC0000 - 0x3FFDFFFF
#else
	// Following are directly map to DPU's local memories by Inbound PIF Arbiter.
	// This approach only works with identical cores
	#define DPU_IRAM0_BASE_ADDR		XCHAL_INSTRAM0_VADDR	//	0x40000000 - 0x4001FFFF
	#define DPU_DRAM0_BASE_ADDR		XCHAL_DATARAM0_VADDR	//	0x3FFE0000 - 0x3FFFFFFF
	#define DPU_DRAM1_BASE_ADDR		XCHAL_DATARAM1_VADDR	//	0x3FFC0000 - 0x3FFDFFFF
#endif

// These two mapped to DEST0_ADDR and DEST1_ADDR in Inbound PIF Arbiter.
#define DPU_IRAM0_LIB_CODE_ADDR (DPU_IRAM0_BASE_ADDR + XCHAL_INSTRAM0_SIZE / 2) // DPU's 0x4001_0000, 64K
#define DPU_IRAM0_MAIN_CODE_ADDR (DPU_IRAM0_BASE_ADDR)	// DPU's 0x4000_0000, 64K
#define DPU_DRAM0_LIB_DATA_ADDR (DPU_DRAM0_BASE_ADDR + XCHAL_DATARAM0_SIZE / 2)	//	DPU's 0x3FFF_0000, 64K
#define DPU_DRAM0_MAIN_DATA_ADDR (DPU_DRAM0_BASE_ADDR)	//	DPU's 0x3FFE_0000, 64K
#define DPU_DRAM1_IO_ADDR1 (DPU_DRAM1_BASE_ADDR + XCHAL_DATARAM1_SIZE / 2)	//	DPU's 0x3FFD_0000, 64K
#define DPU_DRAM1_IO_ADDR0 (DPU_DRAM1_BASE_ADDR)	//	DPU's 0x3FFC_0000, 64K

#define SYSRAM_BASE_ADDR	XSHAL_RAM_VADDR		/* 0x6000_0000 */
#define SYSRAM_SIZE			XSHAL_RAM_VSIZE		/* 0x0400_0000 = 64 MB */

/* With Sim-local LSP, STACK, Heap, .bss sections will be on local memories.
 *  sram19 : C : 0x60000400 - 0x63ffffff : .sram.rodata .sram.literal .sram.text .sram.data .sram.bss;
 *
 *  With Sim LSP, STACK, HEAP are on System RAM.
 *  sram19 : C : 0x60000400 - 0x63ffffff :  STACK :  HEAP : .sram.rodata .rodata .sram.literal .literal .sram.text .text .sram.data .data .sram.bss .bss;
 *
 * This application will use the System RAM (64 MB) as follows
 * 0x6000_0000 to 0x603f_ffff:  4 MB for default .sram.rodata .sram.literal .sram.text .sram.data .sram.bss;
 * 0x6040_0000 to 0x607f_ffff:  4 MB for Library images
 * 0x6080_0000 to 0x617f_ffff: 16 MB for Compressed Audio content
 * 0x6180_0000 to 0x63ff_ffff: 40 MB for Decoded (PCM) Audio
 *
 */

#define SYSRAM_PART_SIZE	(SYSRAM_SIZE / 16)		//  4 MB

#define SIZE_DPU_LIBS_SEG	(SYSRAM_PART_SIZE)		//  4 MB
#define SIZE_AUD_FILS		(SYSRAM_PART_SIZE * 4)	// 16 MB
#define SIZE_AUD_FILS_PCM	(SYSRAM_PART_SIZE * 10)	// 40 MB

#define BASE_DPU_LIBS_SEG	(SYSRAM_BASE_ADDR + SYSRAM_PART_SIZE)   // 0x6040_0000, 4 MB
#define	BASE_AUD_FILS_SEG	(BASE_DPU_LIBS_SEG + SIZE_DPU_LIBS_SEG) // 0x6080_0000,16 MB
#define BASE_PCM_FILS_SEG	(BASE_AUD_FILS_SEG + SIZE_AUD_FILS)     // 0x6180_0000,40 MB

#endif  /* _SYSTEMADDRESS_H_ */
