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
 * xa-class-base.c
 *
 * Generic audio codec task implementation
 *
 ******************************************************************************/

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "xa-class-base.h"

/*******************************************************************************
 * Tracing configuration
 ******************************************************************************/

/*******************************************************************************
 * Internal functions definitions
 ******************************************************************************/

/* ...codec pre-initialization */
static HIFI_ERROR_TYPE xa_base_preinit(XACodecBase *base)
{
	hifi_main_struct *hifi_config = base->hifi_config;
	HiFiCodecMemoryOps memops;
	HIFI_ERROR_TYPE ret = XA_SUCCESS;

	if(base->codecwrapinterface == NULL)
	{
		LOG("base->codecwrapinterface Pointer is NULL\n");
		return XA_INIT_ERR;
	}

	base->codecwrapinterface(ACODEC_API_CREATE_CODEC, (void **)&base->WrapFun.Create);
	base->codecwrapinterface(ACODEC_API_DELETE_CODEC, (void **)&base->WrapFun.Delete);
	base->codecwrapinterface(ACODEC_API_INIT_CODEC, (void **)&base->WrapFun.Init);
	base->codecwrapinterface(ACODEC_API_RESET_CODEC, (void **)&base->WrapFun.Reset);
	base->codecwrapinterface(ACODEC_API_SET_PARAMETER, (void **)&base->WrapFun.SetPara);
	base->codecwrapinterface(ACODEC_API_GET_PARAMETER, (void **)&base->WrapFun.GetPara);
	base->codecwrapinterface(ACODEC_API_DECODE_FRAME, (void **)&base->WrapFun.Process);
	base->codecwrapinterface(ACODEC_API_GET_LAST_ERROR, (void **)&base->WrapFun.GetLastError);

	memops.Malloc = (void *)MEM_scratch_malloc;
	memops.Free = (void *)MEM_scratch_mfree;
#ifdef DEBUG
	memops.hifi_printf = NULL;
#endif

	memops.p_xa_process_api = base->codecinterface;
	memops.hifi_config = &hifi_config->scratch_mem_info;

	if(base->WrapFun.Create == NULL)
	{
		LOG("WrapFun.Create Pointer is NULL\n");
		return XA_INIT_ERR;
	}

	base->pWrpHdl = base->WrapFun.Create(&memops, base->codec_id);
	if(base->pWrpHdl == NULL) {
		LOG("Create codec error in codec wrapper\n");
		return XA_INIT_ERR;
	}

	LOG1("Codec[%d] pre-initialization completed\n", base->codec_id);

    return ret;
}

/* ...post-initialization setup */
static HIFI_ERROR_TYPE xa_base_postinit(XACodecBase *base)
{
	hifi_main_struct *hifi_config = base->hifi_config;
	HIFI_ERROR_TYPE ret;

	if(base->WrapFun.Init == NULL)
	{
		LOG("WrapFun.Init Pointer is NULL\n");
		return XA_INIT_ERR;
	}

	/* ...initialize codec memory */
	ret = base->WrapFun.Init(base->pWrpHdl);

	LOG2("Codec[%d] post-initialization completed, ret = %d\n", base->codec_id, ret);

	return ret;
}

/*******************************************************************************
 * Commands processing
 ******************************************************************************/

/* ...SET-PARAM processing (enabled in all states) */
HIFI_ERROR_TYPE xa_base_set_param(XACodecBase *base, xf_message_t *m)
{
	xf_set_param_msg_t     *cmd = m->buffer;
	HiFiCodecSetParameter param;
	u32 n, i;
	HIFI_ERROR_TYPE ret;

	/* ...check if we need to do pre-initialization */
	if ((base->state & XA_BASE_FLAG_PREINIT) == 0)
	{
		/* ...do basic initialization */
		if (xa_base_preinit(base) != XA_SUCCESS)
		{
		//	m->ret = XA_INIT_ERR;
		//	xf_response(m);

			/* ...initialization failed for some reason; do cleanup */
			xa_base_destroy(base);

			return XA_INIT_ERR;
		}

		/* ...mark the codec static configuration is set */
		base->state ^= XA_BASE_FLAG_PREINIT;
	}

	if(base->WrapFun.SetPara == NULL)
	{
		LOG("WrapFun.SetPara Pointer is NULL\n");
		m->ret = XA_INIT_ERR;
		xf_response(m);

		return XA_INIT_ERR;
	}

	/* ...calculate amount of parameters */
	n = m->length / sizeof(*cmd);

	/* ...send the collection of codec  parameters */
	for (i = 0; i < n; i++)
	{
		param.cmd = cmd[i].id;
		param.val = cmd[i].value;

		/* ...apply parameter; pass to codec-specific function */
		LOG2("set-param: [%d], %d\n", param.cmd, param.val);

		ret = base->WrapFun.SetPara(base->pWrpHdl, &param);
		if(ret != XA_SUCCESS)
		{
			m->ret = ret;
			break;
		}
	}

	LOG("Set param ok\n");
	/* ...complete message processing; output buffer is empty */
	xf_response_ok(m);

	return ret;
}

/* ...GET-PARAM message processing (enabled in all states) */
HIFI_ERROR_TYPE xa_base_get_param(XACodecBase *base, xf_message_t *m)
{
	HiFiCodecGetParameter param;
	xf_get_param_msg_t *cmd = m->buffer;
	u32 n, i;
	HIFI_ERROR_TYPE ret = 0;

	if(base->WrapFun.GetPara == NULL)
	{
		m->ret = XA_INIT_ERR;
		xf_response(m);

		return XA_INIT_ERR;
	}

	/* ...calculate amount of parameters */
	n = m->length / sizeof(*cmd);

    /* ...retrieve the collection of codec  parameters */
	ret = base->WrapFun.GetPara(base->pWrpHdl, &param);
	if(ret)
	{
		m->ret = XA_PARA_ERROR;
		xf_response(m);

		return XA_PARA_ERROR;
	}

	/* ...retrieve the collection of codec  parameters */
	for (i = 0; i < n; i++)
	{
		switch(cmd[i].id)
		{
			case XA_SAMPLERATE:
				cmd[i].value = param.sfreq;
				break;
			case XA_CHANNEL:
				cmd[i].value = param.channels;
				break;
			case XA_DEPTH:
				cmd[i].value = param.bits;
				break;
			case XA_CONSUMED_LENGTH:
				cmd[i].value = param.consumed_bytes;
				break;
			case XA_CONSUMED_CYCLES:
				cmd[i].value = param.cycles;
				break;
			default:
				cmd[i].value = 0;
				break;
		}
	}

	LOG1("Codec get parameter completed, ret = %d\n", ret);

	/* ...complete message specifying output buffer size */
	xf_response_data(m, n * sizeof(*cmd));

	return ret;
}

/*******************************************************************************
 * Command/data processing functions
 ******************************************************************************/

/* ...generic codec data processing */
static HIFI_ERROR_TYPE xa_base_process(XACodecBase *base)
{
	HIFI_ERROR_TYPE    ret;
	u32 consumed, produced;

	/* ...check if we need to do post-initialization */
	if ((base->state & XA_BASE_FLAG_POSTINIT) == 0)
	{
		/* ...check if we need to do pre-initialization */
		if ((base->state & XA_BASE_FLAG_PREINIT) == 0)
		{
			/* ...do basic initialization */
			if (xa_base_preinit(base) != XA_SUCCESS)
			{
				/* ...initialization failed for some reason; do cleanup */
				xa_base_destroy(base);
				LOG("Preinit failed\n");

				return XA_INIT_ERR;
			}

			/* ...mark the codec static configuration is set */
			base->state ^= XA_BASE_FLAG_PREINIT;
		}

		/* ...do post-initialization step */
		ret = xa_base_postinit(base);
		if(ret != XA_SUCCESS)
			return XA_INIT_ERR;

		/* ...mark the codec static configuration is set */
		base->state ^= XA_BASE_FLAG_POSTINIT | XA_BASE_FLAG_EXECUTION;
	}

	/* ...clear internal scheduling flag */
	base->state &= ~XA_BASE_FLAG_SCHEDULE;

	/* ...codec-specific preprocessing (buffer maintenance) */
	if ((ret = CODEC_API(base, preprocess)) != XA_SUCCESS)
	{
		/* ...return non-fatal codec error */
		return ret;
	}

	/* ...execution step */
	if (base->state & XA_BASE_FLAG_EXECUTION)
	{
		if ((ret = CODEC_API(base, process, &consumed, &produced)) != XA_SUCCESS)
		{
			/* ...return non-fatal codec error */
		//	return ret;
		}
	}

	/* ...codec-specific buffer post-processing */
	return CODEC_API(base, postprocess, consumed, produced, ret);
}

/* ...message-processing function (component entry point) */
static int xa_base_command(xf_component_t *component, xf_message_t *m)
{
    XACodecBase    *base = (XACodecBase *) component;
    u32             cmd;

    /* ...invoke data-processing function if message is null */
    if (m == NULL)
    {
        XF_CHK_ERR((XA_SUCCESS != xa_base_process(base)), XA_ERR_UNKNOWN);
        return 0;
    }

    /* ...process the command */
    LOG4("state[%X]:(%X, %d, %x\n", base->state, m->opcode, m->length, m->buffer);

    /* ...bail out if this is forced termination command (I do have a map; maybe I'd better have a hook? - tbd) */
    if ((cmd = XF_OPCODE_TYPE(m->opcode)) == XF_OPCODE_TYPE(XF_UNREGISTER))
    {
        LOG1("force component[%d] termination\n", base->codec_id);
        return -1;
    }

    /* ...check opcode is valid */
    XF_CHK_ERR(cmd < base->command_num, XA_PARA_ERROR);

    /* ...and has a hook */
    XF_CHK_ERR(base->command[cmd] != NULL, XA_PARA_ERROR);

    /* ...pass control to specific command */
    XF_CHK_ERR((XA_SUCCESS != base->command[cmd](base, m)), XA_ERR_UNKNOWN);

    /* ...execution completed successfully */
    return 0;
}

/*******************************************************************************
 * Base codec API
 ******************************************************************************/

/* ...data processing scheduling */
void xa_base_schedule(XACodecBase *base, u32 dts)
{
    hifi_main_struct *hifi_config = base->hifi_config;

    if ((base->state & XA_BASE_FLAG_SCHEDULE) == 0)
    {
        /* ...schedule component task execution */
        xf_component_schedule(&hifi_config->sched, &base->component, dts);

        /* ...and put scheduling flag */
        base->state ^= XA_BASE_FLAG_SCHEDULE;
    }
    else
    {
        LOG1("codec[%d] processing pending\n", base->codec_id);
    }
}

/* ...cancel data processing */
void xa_base_cancel(XACodecBase *base)
{
    hifi_main_struct *hifi_config = base->hifi_config;

    if (base->state & XA_BASE_FLAG_SCHEDULE)
    {
        /* ...cancel scheduled codec task */
        xf_component_cancel(&hifi_config->sched, &base->component);

        /* ...and clear scheduling flag */
        base->state ^= XA_BASE_FLAG_SCHEDULE;

        LOG1("codec[%d] processing cancelled\n", base->codec_id);
    }
}

/* ...base codec destructor */
void xa_base_destroy(XACodecBase *base)
{
    hifi_main_struct *hifi_config = base->hifi_config;
    HIFI_ERROR_TYPE    ret;

    /* ...destroy codec structure (and task) itself */
    MEM_scratch_mfree(&hifi_config->scratch_mem_info, base);

    LOG1("codec[%d]: destroyed\n", base->codec_id);
}

/* ...generic codec initialization routine */
XACodecBase * xa_base_factory(hifi_main_struct *hifi_config, u32 size, void *process)
{
    XACodecBase    *base;

    /* ...make sure the size is sane */
    XF_CHK_ERR(size >= sizeof(XACodecBase), NULL);

    /* ...allocate local memory for codec structure */
    XF_CHK_ERR(base = MEM_scratch_malloc(&hifi_config->scratch_mem_info, size), NULL);

    /* ...reset codec memory */
    memset(base, 0, size);

	/* ...set global structure pointer */
	base->hifi_config = hifi_config;

    /* ...set message processing function */
    base->component.entry = xa_base_command;

    return base;
}
