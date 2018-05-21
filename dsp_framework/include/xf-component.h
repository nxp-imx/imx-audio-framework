/*******************************************************************************
 * Copyright (C) 2017 Cadence Design Systems, Inc.
 * Copyright 2018 NXP
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to use this Software with Cadence processor cores only and
 * not with any other processors and platforms, subject to
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

 ******************************************************************************/

/*******************************************************************************
 * xf-component.h
 *
 * Xtensa processing framework component definition
 *
 ******************************************************************************/

#ifndef __XF_COMPONENT_H
#define __XF_COMPONENT_H

#include "xf-sched.h"
#include "xf-msg.h"

/*******************************************************************************
 * Types definitions
 ******************************************************************************/

/* ...component descriptor (base structure) */
struct xf_component {
	/* ...scheduler node */
	xf_task_t           task;

	/* ...component id */
	u32                 id;

	/* ...message-processing function */
	int                 (*entry)(struct xf_component *, struct xf_message *);

	/* ...component destructor function */
	int                 (*exit)(struct xf_component *, struct xf_message *);

	/* ...private data pointer */
	void                *private_data;

};

/* ...component map entry */
struct xf_cmap_link {
	/* ...poiner to active client */
	struct xf_component *c;

	/* ...index to a client in the list (values 0..XF_CFG_MAX_CLIENTS) */
	u32                 next;
};

/*******************************************************************************
 * Helpers
 ******************************************************************************/

/* ...return core-id of the component */
static inline u32 xf_component_core(struct xf_component *component)
{
	return XF_PORT_CORE(component->id);
}

/* ...schedule component execution */
#define xf_component_schedule(sched, c, dts)                            \
({                                                                      \
	xf_sched_put(sched, &(c)->task, xf_sched_timestamp(sched) + (dts)); \
})

/* ...cancel component execution */
#define xf_component_cancel(sched, c)                                   \
({                                                                      \
	xf_sched_cancel(sched, &(c)->task);                                 \
})

/*******************************************************************************
 * API functions
 ******************************************************************************/

#endif
