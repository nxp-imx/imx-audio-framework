// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021 NXP.
 *
 * This file populates resource table for BM remote
 * for use by the Linux Master
 */

#include "board.h"
#include "rsc_table.h"
#include "rpmsg_lite.h"
#include <string.h>

#define NUM_VRINGS 0x02

/* Place resource table in special ELF section */
__attribute__((section(".resource_table")))

const struct remote_resource_table resources = {
	/* Version */
	1,

	/* NUmber of table entries */
	NO_RESOURCE_ENTRIES,
	/* reserved fields */
	{
		0,
		0,
	},

	/* Offsets of rsc entries */
	{
		offsetof(struct remote_resource_table, user_vdev),
	},

	/* SRTM virtio device entry */
	{
		RSC_VDEV,
		7,
		0,
		RSC_VDEV_FEATURE_NS,
		0,
		0,
		0,
		NUM_VRINGS,
		{0, 0},
	},

	/* Vring rsc entry - part of vdev rsc entry */
	{VDEV0_VRING_DA_BASE, VRING_ALIGN, RL_BUFFER_COUNT, 0, 0},
	{VDEV0_VRING_DA_BASE + VRING_SIZE, VRING_ALIGN, RL_BUFFER_COUNT, 1, 0},
};
