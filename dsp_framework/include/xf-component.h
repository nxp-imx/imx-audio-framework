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
 *******************************************************************************/

#ifndef __XF_COMPONENT_H
#define __XF_COMPONENT_H

#include "xf-sched.h"
#include "xf-msg.h"

/*******************************************************************************
 * Types definitions
 ******************************************************************************/

/* ...component literal identifier */
typedef const char * const xf_id_t;

/* ...component descriptor (base structure) */
typedef struct xf_component
{
    /* ...scheduler node */
    xf_task_t               task;

    /* ...component id */
    u32                     id;

    /* ...message-processing function */
    int                   (*entry)(struct xf_component *, xf_message_t *);

    /* ...component destructor function */
    int                   (*exit)(struct xf_component *, xf_message_t *);

    /* ...private data pointer */
    void                  *private_data;

}   xf_component_t;

/* ...component map entry */
typedef struct xf_cmap_link
{
	/* ...poiner to active client */
	xf_component_t     *c;

	/* ...index to a client in the list (values 0..XF_CFG_MAX_CLIENTS) */
	u32                 next;
} xf_cmap_link_t;

/*******************************************************************************
 * Helpers
 ******************************************************************************/

/* ...return core-id of the component */
static inline u32 xf_component_core(xf_component_t *component)
{
    return XF_PORT_CORE(component->id);
}

/* ...schedule component execution */
#define xf_component_schedule(sched, c, dts)                                \
({                                                                          \
    xf_sched_put(sched, &(c)->task, xf_sched_timestamp(sched) + (dts));     \
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
