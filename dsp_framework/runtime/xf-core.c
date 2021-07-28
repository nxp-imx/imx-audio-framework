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

 *****************************************************************************/

/******************************************************************************
 * xf-core.c
 *
 * DSP processing framework core
 *
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/

#include "mydefs.h"
#include "xf-debug.h"
#include "xf-core.h"
#include "xf-shmem.h"
#include "board.h"

/* ...translate client-id into component handle */
static inline struct xf_component *xf_client_lookup(
			struct dsp_main_struct *dsp_config, u32 client)
{
	struct xf_cmap_link *link = &dsp_config->cmap[client];

	/* ...if link pointer is less than XF_CFG_MAX_CLIENTS,
	 * it is a free descriptor
	 */
	return ((u32)link->c > XF_CFG_MAX_CLIENTS ? link->c : NULL);
}

/* ...allocate client-id */
static inline u32 xf_client_alloc(struct dsp_main_struct *dsp_config)
{
	u32     client = dsp_config->free;

	/* ...advance list head to next free id */
	(client < XF_CFG_MAX_CLIENTS ? dsp_config->free =
		dsp_config->cmap[client].next : 0);

	return client;
}

/* ...recycle client-id */
static inline void xf_client_free(struct dsp_main_struct *dsp_config,
				  u32 client)
{
	/* ...put client into the head of the free id list */
	dsp_config->cmap[client].next = dsp_config->free;
	dsp_config->free = client;
	dsp_config->cmap[client].c = NULL;
}

/******************************************************************************
 * Process commands to a proxy
 *****************************************************************************/

/* ...register new client */
static int xf_proxy_register(struct dsp_main_struct *dsp_config,
			     struct xf_message *m)
{
	u32             src = XF_MSG_SRC(m->id);
	u32             client;
	struct xf_component *component;

	/* ...allocate new client-id */
	client = xf_client_alloc(dsp_config);
	if (client >= XF_CFG_MAX_CLIENTS) {
		LOG("No available clent to allocate\n");
		return -EBUSY;
	}

	/* ...create component via class factory */
	component = (struct xf_component *)xf_component_factory(dsp_config,
					(const char *)m->buffer, m->length);
	if (!component) {
		LOG("Component creation failed\n");

		/* ...recycle client-id */
		xf_client_free(dsp_config, client);

		/* ...return generic out-of-memory code always */
		return -ENOMEM;
	}

	/* ...register component in a map */
	dsp_config->cmap[client].c = component;

	/* ...set component "default" port specification ("destination") */
	component->id = __XF_PORT_SPEC(0, client, 0);

	/* ...adjust session-id to include newly created component-id */
	m->id = __XF_MSG_ID(src, component->id);

	LOG3("registered client: %d, m-id: %x, (%s)\n",
	     client, m->id, (const char *)m->buffer);

	/* ...and return success to remote proxy (zero-length output) */
	xf_response_ok(m);

	return 0;
}

/* ...shared buffer allocation request */
static int xf_proxy_alloc(struct dsp_main_struct *dsp_config,
			  struct xf_message *m)
{
	/* ...allocate shared memory buffer
	 * (system-specific function; may fail)
	 */
	m->buffer = MEM_scratch_malloc(&dsp_config->scratch_mem_info,
				m->length);

	/* ...pass result to remote proxy (on success buffer is non-null) */
	xf_response(m);

	return 0;
}

/* ...shared buffer freeing request */
static int xf_proxy_free(struct dsp_main_struct *dsp_config,
			 struct xf_message *m)
{
	/* ...pass buffer freeing request to system-specific function */
	MEM_scratch_mfree(&dsp_config->scratch_mem_info, m->buffer);

	/* ...return success to remote proxy (function never fails) */
	xf_response(m);

	return 0;
}

/* ...deal with suspend command */
static int xf_proxy_suspend(struct dsp_main_struct *dsp_config,
			    struct xf_message *m)
{
	struct xf_cmap_link     *link;
	struct xf_component *component;
	u32                 i;

	LOG("Process XF_SUSPEND command\n");

	/* ...call suspend of each component */
	for (link = &dsp_config->cmap[i = 0]; i < XF_CFG_MAX_CLIENTS; i++, link++) {
		if (link->c != NULL) {
			component = link->c;

			component->entry(component, m);
		}
	}

	/* ...return message back to the pool */
	xf_msg_pool_put(&dsp_config->pool, m);

	/* send message back*/
	platform_notify(RP_MBOX_SUSPEND_ACK);

	return 0;
}

/* ...deal with resume command */
static int xf_proxy_resume(struct dsp_main_struct *dsp_config,
			   struct xf_message *m)
{
	struct xf_cmap_link     *link;
	struct xf_component *component;
	struct xf_message *m_tmp;
	u32                 i;

	LOG("Process XF_RESUME command\n");

	/* ...return message back to the pool */
	xf_msg_pool_put(&dsp_config->pool, m);

	/* ...recover the data of dsp_main_struct structure */

	m_tmp = xf_msg_pool_get(&dsp_config->pool);
	if (!m_tmp) {
		LOG("Error: ICM Queue full\n");
		return -ENOMEM;
	}
	/* ...fill message parameters */
	m_tmp->id = __XF_MSG_ID(__XF_AP_PROXY(0), __XF_DSP_PROXY(0));
	m_tmp->opcode = XF_RESUME;
	m_tmp->length = 0;
	m_tmp->buffer = 0;
	m_tmp->ret = 0;

	/* ...call resume of each component */
	for (link = &dsp_config->cmap[i = 0]; i < XF_CFG_MAX_CLIENTS; i++, link++) {
		if (link->c != NULL) {
			component = link->c;
			component->entry(component, m_tmp);
		}
	}

	xf_msg_pool_put(&dsp_config->pool, m_tmp);

	/* no message reply to A core */

	return 0;
}

/* ...deal with resume command */
static int xf_proxy_pause(struct dsp_main_struct *dsp_config,
			   struct xf_message *m)
{
	struct xf_cmap_link     *link;
	struct xf_component *component;
	u32                 i;

	LOG("Process XF_PAUSE command\n");

	/* TBD:  Should be for instance, one instance is paused, another one
	 * still can run.
	 */
	/* ...call pause of each component */
	for (link = &dsp_config->cmap[i = 0]; i < XF_CFG_MAX_CLIENTS; i++, link++) {
		if (link->c != NULL) {
			component = link->c;

			component->entry(component, m);
		}
	}

	/* ...return message back to the pool */
	xf_msg_pool_put(&dsp_config->pool, m);

	xf_response(m);

	return 0;
}

/* ...deal with pause release command */
static int xf_proxy_pause_release(struct dsp_main_struct *dsp_config,
				  struct xf_message *m)
{
	struct xf_cmap_link     *link;
	struct xf_component *component;
	u32                 i;

	LOG("Process XF_PAUSE_RELEASE command\n");

	/* ...call resume of each component */

	/* TBD:  Should be for instance, one instance is paused, another one
	 * still can run.
	 */
	for (link = &dsp_config->cmap[i = 0]; i < XF_CFG_MAX_CLIENTS; i++, link++) {
		if (link->c != NULL) {
			component = link->c;
			component->entry(component, m);
		}
	}

	/* ...return message back to the pool */
	xf_msg_pool_put(&dsp_config->pool, m);

	xf_response(m);

	return 0;
}

static int xf_proxy_shmem_info(struct dsp_main_struct *dsp_config,
			       struct xf_message *m)
{
	LOG("GET SHMEM INFO\n");

	m->buffer = (void *)env_map_vatopa((void *)dsp_config->dpu_ext_msg.scratch_buf_phys);
	m->length = dsp_config->dpu_ext_msg.scratch_buf_size;

	xf_response(m);

	return 0;
}

/* ...proxy command processing table */
static int (* const xf_proxy_cmd[])(struct dsp_main_struct *dsp_config, struct xf_message *) = {
	[XF_OPCODE_TYPE(XF_REGISTER)] = xf_proxy_register,
	[XF_OPCODE_TYPE(XF_ALLOC)] = xf_proxy_alloc,
	[XF_OPCODE_TYPE(XF_FREE)] = xf_proxy_free,
	[XF_OPCODE_TYPE(XF_SUSPEND)] = xf_proxy_suspend,
	[XF_OPCODE_TYPE(XF_RESUME)] = xf_proxy_resume,
	[XF_OPCODE_TYPE(XF_PAUSE)] = xf_proxy_pause,
	[XF_OPCODE_TYPE(XF_PAUSE_RELEASE)] = xf_proxy_pause_release,
	[XF_OPCODE_TYPE(XF_SHMEM_INFO)] = xf_proxy_shmem_info,
};

/* ...total number of commands supported */
#define XF_PROXY_CMD_NUM (sizeof(xf_proxy_cmd) / sizeof(xf_proxy_cmd[0]))

/* ...process commands to a proxy */
static void xf_proxy_command(struct dsp_main_struct *dsp_config,
			     struct xf_message *m)
{
	u32     opcode = m->opcode;
	int     res;

	/* ...dispatch command to proper hook */
	if (XF_OPCODE_TYPE(opcode) < XF_PROXY_CMD_NUM) {
		res = xf_proxy_cmd[XF_OPCODE_TYPE(opcode)](dsp_config, m);
		if (res >= 0) {
			/* ...command processed successfully; do nothing */
			return;
		}
	} else {
		LOG1("invalid opcode: %x\n", opcode);
	}

	/* ...command processing failed; return generic failure response */
	xf_response_err(m);
}

static inline void xf_core_process(struct dsp_main_struct *dsp_config,
				   struct xf_component *component)
{
	u32     id = component->id;

	/* ...client look-up successfull */
	LOG1("client[%u]::process\n", XF_PORT_CLIENT(id));

	/* ...call data-processing interface */
	if (component->entry(component, NULL) < 0)
		LOG("execution error (ignored)\n");
}

/* ...dispatch message queue execution */
static inline void xf_core_dispatch(struct dsp_main_struct *dsp_config,
				    struct xf_message *m)
{
	u32             client;
	struct xf_component *component;

	/* ...do client-id/component lookup */
	if (XF_MSG_DST_PROXY(m->id)) {
		/* ...process message addressed to proxy */
		xf_proxy_command(dsp_config, m);

		return;
	}

	/* ...message goes to local component */
	client = XF_MSG_DST_CLIENT(m->id);

	/* ...check if client is alive */
	component = xf_client_lookup(dsp_config, client);
	if (component) {
		/* ...client look-up successfull */
		LOG3("client[%u]::cmd(id=%x, opcode=%x)\n",
		     client, m->id, m->opcode);

		/* ...pass message to component entry point */
		if (component->entry(component, m) < 0) {
			/* ...call component destructor */
			if (component->exit(component, m) == 0) {
				/* ...component cleanup completed;
				 * recycle component-id
				 */
				xf_client_free(dsp_config, client);
			}
		}
	} else {
		LOG2("Discard message id=%x - client %u not registered\n",
		     m->id, client);

		/* ...complete message with general failure response */
		xf_response_err(m);
	}
}

/* ...initialize global framework data */
int xf_core_init(struct dsp_main_struct *dsp_config)
{
	struct xf_cmap_link     *link;
	u32                 i;

	/* ...create list of free client descriptors */
	for (link = &dsp_config->cmap[i = 0]; i < XF_CFG_MAX_CLIENTS; i++, link++)
		link->next = i + 1;

	/* ...set head of free clients list */
	dsp_config->free = 0;

	/* ...initialize scratch memory */
	MEM_scratch_init(&dsp_config->scratch_mem_info,
			 dsp_config->dpu_ext_msg.scratch_buf_phys,
			 dsp_config->dpu_ext_msg.scratch_buf_size);

	/* ...initialize local queue scheduler */
	xf_sched_init(&dsp_config->sched);

	/* ...initialize shared message pool between DSP and AP core */
	xf_shmem_init(dsp_config);

	/* ...set is_core_init after core has been initialized */
	dsp_config->is_core_init = 1;

	/* ...okay... it's all good */
	LOG("core initialized\n");

	return 0;
}

/* ...core executive loop function */
void xf_core_service(struct dsp_main_struct *dsp_config)
{
	u32             status;
	struct xf_message   *m;
	xf_task_t      *t;

	do {
		/* ...clear local status change */
		status = 0;

		/* ...send/receive messages to/from shared message
		 * pool between DSP and AP core
		 */
		if (dsp_config->is_core_init)
			xf_shmem_process_queues(dsp_config);

		/* ...check if we have a backlog message placed
		 * into global command queue
		 */
		while ((m = xf_msg_local_get(&dsp_config->queue)) != NULL) {
			/* ...dispatch message execution */
			xf_core_dispatch(dsp_config, m);

			/* ...set local status change */
			status = 1;
			/*
			 * Break the loop when SUSPEND for we have store the
			 * env, after resume,  the framework will be
			 * reconfigured to same env.
			 */
			if (m->opcode == XF_SUSPEND) {
				status = 0;
				break;
			}
		}

		/* ...if scheduler queue is empty, break the loop */
		t = xf_sched_get(&dsp_config->sched);
		if (dsp_config->is_core_init && t) {
			/* ...data-processing execution
			 * (ignore internal errors)
			 */
			xf_core_process(dsp_config, (struct xf_component *)t);

			/* ...set local status change */
			status = 1;
		}
	} while (status);
}

