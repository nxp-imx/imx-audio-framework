/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2021 NXP
 */

#ifndef _BOARD_H_
#define _BOARD_H_

enum {
	DSP_IMX8QXP_TYPE = 0,
	DSP_IMX8QM_TYPE,
	DSP_IMX8MP_TYPE,
	DSP_IMX8ULP_TYPE,
};

/*
 * Memory allocation for reserved memory:
 * We alway reserve 32M memory from DRAM
 * The DRAM reserved memory is split into three parts currently.
 * The front part is used to keep the dsp firmware, the other part is
 * considered as scratch memory for dsp framework.
 *
 *---------------------------------------------------------------------------
 *| Offset                |  Size    |   Usage                              |
 *---------------------------------------------------------------------------
 *| 0x0 ~ 0xEFFFFF        |  15M     |   Code memory of firmware            |
 *---------------------------------------------------------------------------
 *| 0xF00000 ~ 0xFFFFFF   |  1M      |   Message buffer + Globle dsp struct |
 *---------------------------------------------------------------------------
 *| 0x1000000 ~ 0x1FFFFFF |  16M     |   Scratch memory                     |
 *---------------------------------------------------------------------------
 */

/* Cache definition
 * Every 512M in 4GB space has dedicate cache attribute.
 * 1: write through
 * 2: cache bypass
 * 4: write back
 * F: invalid access
 */

#define FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL (1)
#define RESUME_FLAG (0x1A2B3C4D)

#define RP_MBOX_SUSPEND_SYSTEM  (0xFF11)
#define RP_MBOX_SUSPEND_ACK     (0xFF12)
#define RP_MBOX_RESUME_SYSTEM   (0xFF13)
#define RP_MBOX_RESUME_ACK      (0xFF14)

/* PLATF_8M */
#ifdef PLATF_8M

#define BOARD_TYPE (DSP_IMX8MP_TYPE)

#define I_CACHE_ATTRIBUTE      0x22242224     //write back mode
#define D_CACHE_ATTRIBUTE      0x22212221     //write through mode
#define INT_NUM_MU             7
#define MU_PADDR               0x30E70000

#define VDEV0_VRING_SA_BASE   (0x942f0000U)
#define VDEV0_VRING_DA_BASE   (0x942f0000U)
#define VDEV0_VRING_SIZE      (0x00008000U)
#define VDEV0_VRING_NUM       (0x00000002U)

#define RESERVED_MEM_ADDR     (0x92400000U)
#define RESERVED_MEM_SIZE     (0x02000000U)
#define GLOBAL_DSP_MEM_ADDR   (RESERVED_MEM_ADDR + 0xF00000U)
#define GLOBAL_DSP_MEM_SIZE   (0x100000U)
#define SCRATCH_MEM_ADDR      (RESERVED_MEM_ADDR + 0x1000000U)
#define SCRATCH_MEM_SIZE      (0xef0000U)

#define RPMSG_LITE_SRTM_SHMEM_BASE  (VDEV0_VRING_DA_BASE)
#define RPMSG_LITE_SRTM_LINK_ID     (0U)

#define MUB_BASE              (MU_PADDR)
#define SYSTEM_CLOCK          (800000000UL)

#define UART_BASE             (0x30a60000)
#define UART_CLK_ROOT         (24000000)

#else /* PLATF_8M */
#ifdef PLATF_8ULP

#define BOARD_TYPE (DSP_IMX8ULP_TYPE)

#define I_CACHE_ATTRIBUTE      0x22222224     //write back mode
#define D_CACHE_ATTRIBUTE      0x22222221     //write through mode
#define INT_NUM_MU             15
#define MU_PADDR               0x2DA20000

/* remapping  0x8def0000U   -    0x19ef0000 */
#define VDEV0_VRING_SA_BASE   (0x8fef0000U)
#define VDEV0_VRING_DA_BASE   (0x1bef0000U)
#define VDEV0_VRING_SIZE      (0x00008000U)
#define VDEV0_VRING_NUM       (0x00000002U)

/* remapping  0x8e000000U   -    0x1a000000 */
#define RESERVED_MEM_ADDR     (0x1a000000U)
#define RESERVED_MEM_SIZE     (0x02000000U)
#define GLOBAL_DSP_MEM_ADDR   (RESERVED_MEM_ADDR + 0xF00000U)
#define GLOBAL_DSP_MEM_SIZE   (0x100000U)
#define SCRATCH_MEM_ADDR      (RESERVED_MEM_ADDR + 0x1000000U)
#define SCRATCH_MEM_SIZE      (0xef0000U)

#define RPMSG_LITE_SRTM_SHMEM_BASE  (VDEV0_VRING_DA_BASE)
#define RPMSG_LITE_SRTM_LINK_ID     (0U)

#define MUB_BASE              (MU_PADDR)
#define SYSTEM_CLOCK          (528000000UL)

/*lpuart6 for debug */
#define LPUART_BASE           (0x29860000)
#define UART_CLK_ROOT         (48000000)

#else /* !PLATF_8ULP && ! PLATF_8M  (8QXP || 8QM)*/

#define BOARD_TYPE (DSP_IMX8QXP_TYPE)

#define I_CACHE_ATTRIBUTE      0x22242224     //write back mode
#define D_CACHE_ATTRIBUTE      0x22212221     //write through mode
#define INT_NUM_MU             7
#define MU_PADDR               0x5D310000

#define VDEV0_VRING_SA_BASE   (0x942f0000U)
#define VDEV0_VRING_DA_BASE   (0x942f0000U)
#define VDEV0_VRING_SIZE      (0x00008000U)
#define VDEV0_VRING_NUM       (0x00000002U)

#define RESERVED_MEM_ADDR     (0x92400000U)
#define RESERVED_MEM_SIZE     (0x02000000U)
#define GLOBAL_DSP_MEM_ADDR   (RESERVED_MEM_ADDR + 0xF00000U)
#define GLOBAL_DSP_MEM_SIZE   (0x100000U)
#define SCRATCH_MEM_ADDR      (RESERVED_MEM_ADDR + 0x1000000U)
#define SCRATCH_MEM_SIZE      (0xef0000U)

#define RPMSG_LITE_SRTM_SHMEM_BASE  (VDEV0_VRING_DA_BASE)
#define RPMSG_LITE_SRTM_LINK_ID     (0U)

#define MUB_BASE              (MU_PADDR)
#define SYSTEM_CLOCK          (600000000UL)

#define LPUART_BASE           (0x5a090000)
#define UART_CLK_ROOT         (80000000)

#endif /*PLATF_8ULP */
#endif /*PLATF_8M */

#endif /* _BOARD_H_ */
