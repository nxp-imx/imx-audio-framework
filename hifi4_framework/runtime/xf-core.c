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
 * xf-core.c
 *
 * DSP processing framework core
 *
 ******************************************************************************/

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "mydefs.h"
#include "xf-debug.h"
#include "xf-core.h"
#include "xf-shmem.h"


/* ...translate client-id into component handle */
static inline xf_component_t * xf_client_lookup(hifi_main_struct *hifi_config, u32 client)
{
    xf_cmap_link_t *link = &hifi_config->cmap[client];

    /* ...if link pointer is less than XF_CFG_MAX_CLIENTS, it is a free descriptor */
    return ((u32)link->c > XF_CFG_MAX_CLIENTS ? link->c : NULL);
}

/* ...allocate client-id */
static inline u32 xf_client_alloc(hifi_main_struct *hifi_config)
{
    u32     client = hifi_config->free;

    /* ...advance list head to next free id */
    (client < XF_CFG_MAX_CLIENTS ? hifi_config->free = hifi_config->cmap[client].next : 0);

    return client;
}

/* ...recycle client-id */
static inline void xf_client_free(hifi_main_struct *hifi_config, u32 client)
{
    /* ...put client into the head of the free id list */
    hifi_config->cmap[client].next = hifi_config->free, hifi_config->free = client;
    hifi_config->cmap[client].c = NULL;
}

/*******************************************************************************
 * Process commands to a proxy
 ******************************************************************************/

/* ...register new client */
static int xf_proxy_register(hifi_main_struct *hifi_config, xf_message_t *m)
{
    u32             src = XF_MSG_SRC(m->id);
    u32             client;
    xf_component_t *component;

    /* ...allocate new client-id */
    client = xf_client_alloc(hifi_config);
    if(client >= XF_CFG_MAX_CLIENTS)
    {
        LOG("No available clent to allocate\n");
        return -EBUSY;
    }

    /* ...create component via class factory */
    if ((component = (xf_component_t *)xf_component_factory(hifi_config, (xf_id_t)m->buffer, m->length)) == NULL)
    {
        LOG("Component creation failed\n");

        /* ...recycle client-id */
        xf_client_free(hifi_config, client);

        /* ...return generic out-of-memory code always */
        return -ENOMEM;
    }

    /* ...register component in a map */
    hifi_config->cmap[client].c = component;

    /* ...set component "default" port specification ("destination") */
    component->id = __XF_PORT_SPEC(0, client, 0);

    /* ...adjust session-id to include newly created component-id */
    m->id = __XF_MSG_ID(src, component->id);

    LOG3("registered client: %d, m-id: %x, (%s)\n", client, m->id, (xf_id_t)m->buffer);

    /* ...and return success to remote proxy (zero-length output) */
    xf_response_ok(m);

    return 0;
}

/* ...shared buffer allocation request */
static int xf_proxy_alloc(hifi_main_struct *hifi_config, xf_message_t *m)
{
    /* ...allocate shared memory buffer (system-specific function; may fail) */
	m->buffer = MEM_scratch_malloc(&hifi_config->scratch_mem_info, m->length);

    /* ...pass result to remote proxy (on success buffer is non-null) */
    xf_response(m);

    return 0;
}

/* ...shared buffer freeing request */
static int xf_proxy_free(hifi_main_struct *hifi_config, xf_message_t *m)
{
    /* ...pass buffer freeing request to system-specific function */
	MEM_scratch_mfree(&hifi_config->scratch_mem_info, m->buffer);

    /* ...return success to remote proxy (function never fails) */
    xf_response(m);

    return 0;
}

/* ...deal with suspend command */
static int xf_proxy_suspend(hifi_main_struct *hifi_config, xf_message_t *m)
{
    icm_header_t icm_msg;

    LOG("Process XF_SUSPEND command\n");

    /* ...disable shard message pool between DSP and AP core */
    XF_PROXY_WRITE(hifi_config, cmd_invalid, 1);
    XF_PROXY_WRITE(hifi_config, rsp_invalid, 1);

    /* ...save the data of hifi_main_struct structure */
    if(hifi_config->dpu_ext_msg.hifi_config_phys)
        memcpy((char *)(hifi_config->dpu_ext_msg.hifi_config_phys),
            hifi_config,
            sizeof(hifi_main_struct));

    /* ...return message back to the pool */
    xf_msg_pool_put(&hifi_config->pool, m);

    /* ...send ack to dsp driver */
    icm_msg.allbits = 0;
    icm_msg.intr = 1;
    icm_msg.msg  = XF_SUSPEND;
    icm_intr_send(icm_msg.allbits);

    return 0;
}

/* ...deal with resume command */
static int xf_proxy_resume(hifi_main_struct *hifi_config, xf_message_t *m)
{
    icm_header_t icm_msg;

    LOG("Process XF_RESUME command\n");

    /* ...return message back to the pool */
    xf_msg_pool_put(&hifi_config->pool, m);

    /* ...recover the data of hifi_main_struct structure */
    if(hifi_config->dpu_ext_msg.hifi_config_phys)
        memcpy(hifi_config,
            (char *)(hifi_config->dpu_ext_msg.hifi_config_phys),
            sizeof(hifi_main_struct));

    /* ...enable shard message pool between DSP and AP core */
    XF_PROXY_WRITE(hifi_config, cmd_invalid, 0);
    XF_PROXY_WRITE(hifi_config, rsp_invalid, 0);

    /* ...set is_interrupt flag */
    hifi_config->is_interrupt = 1;

    /* ...send ack to dsp driver */
    icm_msg.allbits = 0;
    icm_msg.intr = 1;
    icm_msg.msg  = XF_RESUME;
    icm_intr_send(icm_msg.allbits);

    return 0;
}

/* ...proxy command processing table */
static int (* const xf_proxy_cmd[])(hifi_main_struct *hifi_config, xf_message_t *) =
{
    [XF_OPCODE_TYPE(XF_REGISTER)] = xf_proxy_register,
    [XF_OPCODE_TYPE(XF_ALLOC)] = xf_proxy_alloc,
    [XF_OPCODE_TYPE(XF_FREE)] = xf_proxy_free,
    [XF_OPCODE_TYPE(XF_SUSPEND)] = xf_proxy_suspend,
    [XF_OPCODE_TYPE(XF_RESUME)] = xf_proxy_resume,
};

/* ...total number of commands supported */
#define XF_PROXY_CMD_NUM        (sizeof(xf_proxy_cmd) / sizeof(xf_proxy_cmd[0]))

/* ...process commands to a proxy */
static void xf_proxy_command(hifi_main_struct *hifi_config, xf_message_t *m)
{
    u32     opcode = m->opcode;
    int     res;

    /* ...dispatch command to proper hook */
    if (XF_OPCODE_TYPE(opcode) < XF_PROXY_CMD_NUM)
    {
        if ((res = xf_proxy_cmd[XF_OPCODE_TYPE(opcode)](hifi_config, m)) >= 0)
        {
            /* ...command processed successfully; do nothing */
            return;
        }
    }
    else
    {
        LOG1("invalid opcode: %x\n", opcode);
    }

    /* ...command processing failed; return generic failure response */
    xf_response_err(m);
}

static inline void xf_core_process(hifi_main_struct *hifi_config, xf_component_t *component)
{
    u32     id = component->id;

    /* ...client look-up successfull */
    LOG1("client[%u]::process\n", XF_PORT_CLIENT(id));

    /* ...call data-processing interface */
    if (component->entry(component, NULL) < 0)
    {
        LOG("execution error (ignored)\n");
    }
}

/* ...dispatch message queue execution */
static inline void xf_core_dispatch(hifi_main_struct *hifi_config, xf_message_t *m)
{
    u32             client;
    xf_component_t *component;

    /* ...do client-id/component lookup */
    if (XF_MSG_DST_PROXY(m->id))
    {
        /* ...process message addressed to proxy */
        xf_proxy_command(hifi_config, m);

        return;
    }

    /* ...message goes to local component */
    client = XF_MSG_DST_CLIENT(m->id);

    /* ...check if client is alive */
    if ((component = xf_client_lookup(hifi_config, client)) != NULL)
    {
        /* ...client look-up successfull */
        LOG3("client[%u]::cmd(id=%x, opcode=%x)\n", client, m->id, m->opcode);

        /* ...pass message to component entry point */
        if (component->entry(component, m) < 0)
        {
            /* ...call component destructor */
            if (component->exit(component, m) == 0)
            {
                /* ...component cleanup completed; recycle component-id */
                xf_client_free(hifi_config, client);
            }
        }
    }
    else
    {
        LOG2("Discard message id=%x - client %u not registered\n", m->id, client);

        /* ...complete message with general failure response */
        xf_response_err(m);
    }
}

/* ...initialize global framework data */
int xf_core_init(hifi_main_struct *hifi_config)
{
    xf_cmap_link_t     *link;
    u32                 i;

    /* ...create list of free client descriptors */
    for (link = &hifi_config->cmap[i = 0]; i < XF_CFG_MAX_CLIENTS; i++, link++)
    {
        link->next = i + 1;
    }

    /* ...set head of free clients list */
    hifi_config->free = 0;

    /* ...initialize scratch memory */
    MEM_scratch_init(&hifi_config->scratch_mem_info,
                     hifi_config->dpu_ext_msg.scratch_buf_phys,
                     hifi_config->dpu_ext_msg.scratch_buf_size);

    /* ...initialize local queue scheduler */
    xf_sched_init(&hifi_config->sched);

    /* ...initialize shared message pool between DSP and AP core */
    xf_shmem_init(hifi_config);

    /* ...set is_core_init after core has been initialized */
    hifi_config->is_core_init = 1;

    /* ...okay... it's all good */
    LOG("core initialized\n");

    return 0;
}

/* ...core executive loop function */
void xf_core_service(hifi_main_struct *hifi_config)
{
    u32             status;
    xf_message_t   *m;
    xf_task_t      *t;

    do
    {
        /* ...clear local status change */
        status = 0;

        /* ...send/receive messages to/from shared message pool between DSP and AP core */
        if(hifi_config->is_core_init)
            xf_shmem_process_queues(hifi_config);

        /* ...check if we have a backlog message placed into global command queue */
        while ((m = xf_msg_local_get(&hifi_config->queue)) != NULL)
        {
            /* ...dispatch message execution */
            xf_core_dispatch(hifi_config, m);

            /* ...set local status change */
            status = 1;
        }

        /* ...if scheduler queue is empty, break the loop */
        if (hifi_config->is_core_init && ((t = xf_sched_get(&hifi_config->sched)) != NULL))
        {
            /* ...data-processing execution (ignore internal errors) */
            xf_core_process(hifi_config, (xf_component_t *)t);

            /* ...set local status change */
            status = 1;
        }
    }
    while (status);
}

