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
static UA_ERROR_TYPE xa_base_preinit(struct XACodecBase *base)
{
	struct dsp_main_struct *dsp_config =
		(struct dsp_main_struct *)base->component.private_data;
	u32 size;
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	/* ...get API structure size */
	XA_API(base, XF_API_CMD_GET_API_SIZE, 0, &size);

	/* ...allocate memory for codec API structure (8-bytes aligned) */
	base->api = MEM_scratch_ua_malloc(size);
	memset(base->api, 0, size);

	/* ...codec pre-initialization */
	ret = XA_API(base,
		     XF_API_CMD_PRE_INIT,
		     base->codec_id,
		     (void *)dsp_config);
	if (ret != ACODEC_SUCCESS) {
		LOG2("Codec[%d] pre-initialization error, error = %d\n",
		     base->codec_id, ret);
		return ret;
	}

	LOG1("Codec[%d] pre-initialization completed\n",
	     base->codec_id);

	return ret;
}
/* ...initialization setup */
static UA_ERROR_TYPE xa_base_init(struct XACodecBase *base)
{
	UA_ERROR_TYPE ret;

	/* ...codec initialization */
	ret = XA_API(base, XF_API_CMD_INIT, 0, NULL);

	/* dedicate to Candence codec initialization*/
	if (base->codecinterface) {
		XA_API(base, XF_API_CMD_SET_PARAM, UNIA_CODEC_ID, &base->codec_id);

		ret = XA_API(base, XF_API_CMD_SET_PARAM, UNIA_CODEC_ENTRY_ADDR, (void *)&base->codecinterface);
		LOG3("Codec[%d] set codec entry[%p] completed, ret = %d\n", base->codec_id,
				base->codecinterface, ret);
	}
#ifdef DEBUG
	XA_API(base, XF_API_CMD_SET_PARAM, UNIA_FUNC_PRINT, NULL);
#endif

	LOG2("Codec[%d] initialization completed, ret = %d\n",
	     base->codec_id, ret);

	return ret;
}

/* ...post-initialization setup */
static UA_ERROR_TYPE xa_base_postinit(struct XACodecBase *base)
{
	UA_ERROR_TYPE ret;

	/* ...codec post-initialization */
	ret = XA_API(base, XF_API_CMD_POST_INIT, 0, NULL);

	LOG2("Codec[%d] post-initialization completed, ret = %d\n",
	     base->codec_id, ret);

	return ret;
}

/*******************************************************************************
 * Commands processing
 ******************************************************************************/

/* ...SET-PARAM processing (enabled in all states) */
UA_ERROR_TYPE xa_base_set_param(struct XACodecBase *base, struct xf_message *m)
{
	struct dsp_main_struct *dsp_config =
		(struct dsp_main_struct *)base->component.private_data;

	struct xf_set_param_msg *cmd = m->buffer;
	s32 command;
	data_t value;
	u32 n, i, j, pos;
	UA_ERROR_TYPE ret;

	/* ...check if we need to do initialization */
	if ((base->state & XA_BASE_FLAG_INIT) == 0) {
		/* ...do basic initialization */
		if (xa_base_init(base) != ACODEC_SUCCESS)
			return ACODEC_INIT_ERR;

		/* ...mark the codec static configuration is set */
		base->state ^= XA_BASE_FLAG_INIT;
	}

	/* ...calculate amount of parameters */
	n = m->length / sizeof(*cmd);

	/* ...send the collection of codec  parameters */
	for (i = 0; i < n; i++) {
		pos = 0;
		command = cmd[i].id;
		value = cmd[i].mixData;

		/* translate addr saved in param to local addr when special command */
		if (command == UNIA_CHAN_MAP_TABLE) {
			value.chan_map_tab.size = cmd[i].mixData.chan_map_tab.size;
			for(j = 0; j < value.chan_map_tab.size + 1; j++) {
				if (cmd[i].mixData.chan_map_tab.channel_table[j]) {
					value.chan_map_tab.channel_table[pos] = xf_ipc_a2b(dsp_config, cmd[i].mixData.chan_map_tab.channel_table[j]);
				}
				pos++;
			}
		}

		/* ...apply parameter; pass to codec-specific function */
		LOG2("set-param: [%d], %d\n", command, value.value);

		ret = XA_API(base, XF_API_CMD_SET_PARAM, command, &value);
		if (ret != ACODEC_SUCCESS) {
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
UA_ERROR_TYPE xa_base_get_param(struct XACodecBase *base, struct xf_message *m)
{
	struct xf_get_param_msg *cmd = m->buffer;
	u32 command;
	data_t value;
	u32 n, i;
	UA_ERROR_TYPE ret = 0;

	/* ...calculate amount of parameters */
	n = m->length / sizeof(*cmd);

	/* ...retrieve the collection of codec  parameters */
	for (i = 0; i < n; i++) {
		command = cmd[i].id;
		ret = XA_API(base, XF_API_CMD_GET_PARAM, command, &value);
		if (ret != ACODEC_SUCCESS) {
			m->ret = ret;
			break;
		}
		cmd[i].mixData = value;
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
static UA_ERROR_TYPE xa_base_process(struct XACodecBase *base)
{
	UA_ERROR_TYPE    ret;

	/* ...check if we need to do post-initialization */
	if ((base->state & XA_BASE_FLAG_POSTINIT) == 0) {
		/* ...check if we need to do initialization */
		if ((base->state & XA_BASE_FLAG_INIT) == 0) {
			/* ...do basic initialization */
			if (xa_base_init(base) != ACODEC_SUCCESS)
				return ACODEC_INIT_ERR;

			/* ...mark the codec static configuration is set */
			base->state ^= XA_BASE_FLAG_INIT;
		}

		/* ...do post-initialization step */
		ret = xa_base_postinit(base);
		if (ret != ACODEC_SUCCESS)
			return ACODEC_INIT_ERR;

		/* ...mark the codec static configuration is set */
		base->state ^= XA_BASE_FLAG_POSTINIT | XA_BASE_FLAG_EXECUTION;
	}

	/* ...clear internal scheduling flag */
	base->state &= ~XA_BASE_FLAG_SCHEDULE;

	/* ...codec-specific preprocessing (buffer maintenance) */
	ret = CODEC_API(base, preprocess);
	if (ret != ACODEC_SUCCESS)
		/* ...return non-fatal codec error */
		return ret;

	/* ...execution step */
	if (base->state & XA_BASE_FLAG_EXECUTION) {
		/* ...execute decoding process */
		ret = XA_API(base, XF_API_CMD_EXECUTE, 0, NULL);
	}

	/* ...codec-specific buffer post-processing */
	return CODEC_API(base, postprocess, ret);
}

/* ...message-processing function (component entry point) */
static int xa_base_command(struct xf_component *component, struct xf_message *m)
{
	struct XACodecBase  *base = (struct XACodecBase *)component;
	u32             cmd;

	/* ...invoke data-processing function if message is null */
	if (!m) {
		XF_CHK_ERR((xa_base_process(base) != ACODEC_SUCCESS),
			   XA_ERR_UNKNOWN);
		return 0;
	}

	/* ...process the command */
	LOG4("state[%X]:(%X, %d, %x\n",
	     base->state, m->opcode, m->length, m->buffer);

	/* ...bail out if this is forced termination command */
	cmd = XF_OPCODE_TYPE(m->opcode);
	if (cmd == XF_OPCODE_TYPE(XF_UNREGISTER)) {
		LOG1("force component[%d] termination\n", base->codec_id);
		return -1;
	}

	/* ...check opcode is valid */
	XF_CHK_ERR(cmd < base->command_num, XA_PARA_ERROR);

	/* ...and has a hook */
	XF_CHK_ERR(base->command[cmd] != NULL, XA_PARA_ERROR);

	/* ...pass control to specific command */
	XF_CHK_ERR((base->command[cmd](base, m) != ACODEC_SUCCESS), XA_ERR_UNKNOWN);

	/* ...execution completed successfully */
	return 0;
}

/*******************************************************************************
 * Base codec API
 ******************************************************************************/

/* ...data processing scheduling */
void xa_base_schedule(struct XACodecBase *base, u32 dts)
{
	struct dsp_main_struct *dsp_config =
		(struct dsp_main_struct *)base->component.private_data;

	if ((base->state & XA_BASE_FLAG_SCHEDULE) == 0) {
		/* ...schedule component task execution */
		xf_component_schedule(&dsp_config->sched,
				      &base->component,
				      dts);

		/* ...and put scheduling flag */
		base->state ^= XA_BASE_FLAG_SCHEDULE;
	} else {
		LOG1("codec[%d] processing pending\n", base->codec_id);
	}
}

/* ...cancel data processing */
void xa_base_cancel(struct XACodecBase *base)
{
	struct dsp_main_struct *dsp_config =
		(struct dsp_main_struct *)base->component.private_data;

	if (base->state & XA_BASE_FLAG_SCHEDULE) {
		/* ...cancel scheduled codec task */
		xf_component_cancel(&dsp_config->sched, &base->component);

		/* ...and clear scheduling flag */
		base->state ^= XA_BASE_FLAG_SCHEDULE;

		LOG1("codec[%d] processing cancelled\n", base->codec_id);
	}
}

/* ...base codec destructor */
void xa_base_destroy(struct XACodecBase *base)
{
	struct dsp_main_struct *dsp_config =
		(struct dsp_main_struct *)base->component.private_data;
	UA_ERROR_TYPE    ret;

	/* ...deallocate all resources */
	MEM_scratch_ua_mfree(base->api);

	/* ...destroy codec structure (and task) itself */
	MEM_scratch_ua_mfree(base);

	LOG1("codec[%d]: destroyed\n", base->codec_id);
}

/* ...generic codec initialization routine */
struct XACodecBase *xa_base_factory(struct dsp_main_struct *dsp_config,
				    u32 size,
				    xf_codec_func_t *process,
				    u32 type)
{
	struct XACodecBase  *base;

	/* ...make sure the size is sane */
	XF_CHK_ERR(size >= sizeof(struct XACodecBase), NULL);

	/* ...allocate local memory for codec structure */
	XF_CHK_ERR(base = MEM_scratch_ua_malloc(
					     size), NULL);

	/* ...reset codec memory */
	memset(base, 0, size);

	/* ...set low-level codec API function */
	base->process = process;

	/* ...set codec id */
	base->codec_id = type;

	/* ...set message processing function */
	base->component.entry = xa_base_command;

	/* ...set component private data pointer */
	base->component.private_data = (void *)dsp_config;

	/* ...do basic initialization */
	if (xa_base_preinit(base) != ACODEC_SUCCESS) {
		/* ...initialization failed for some reason; do cleanup */
		xa_base_destroy(base);

		return NULL;
	}

	return base;
}
