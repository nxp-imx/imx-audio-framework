/*******************************************************************************
* Copyright (C) 2017 Cadence Design Systems, Inc.
* Copyright 2018-2020 NXP
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
 * xa-class-renderer.c
 *
 * Generic audio renderer component class
 ******************************************************************************/

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "xa-class-base.h"
#include "xf-io.h"
#include "xf-hal.h"
#include "audio/xa-renderer-api.h"

/*******************************************************************************
 * Data structures
 ******************************************************************************/

/* ...renderer data */
struct XARenderer
{
	/***************************************************************************
	* Control data
	**************************************************************************/

	/* ...generic audio codec data */
	struct XACodecBase       base;

	/* ...input port */
	struct xf_input_port     input;

	/* ...buffer completion hook */
	xa_renderer_cb_t         cdata;

	/* ...output ready message */
	struct xf_message        msg;

	/***************************************************************************
	* Run-time configuration parameters
	**************************************************************************/

	/* ...time conversion factor (input byte "duration" in timebase units) */
	u32                      factor;

	/* ...internal message scheduling flag (shared with interrupt) */
	u32                      schedule;
};

/*******************************************************************************
 * Renderee flags
 ******************************************************************************/

/* ...rendering is performed */
#define XA_RENDERER_FLAG_RUNNING        __XA_BASE_FLAG(1 << 0)

/* ...renderer is idle and produces silence */
#define XA_RENDERER_FLAG_SILENCE        __XA_BASE_FLAG(1 << 1)

/* ...ouput data is ready */
#define XA_RENDERER_FLAG_OUTPUT_READY   __XA_BASE_FLAG(1 << 2)

/*******************************************************************************
 * Internal helpers
 ******************************************************************************/

/*******************************************************************************
 * Commands handlers
 ******************************************************************************/

/* ...EMPTY-THIS-BUFFER command processing */
static UA_ERROR_TYPE xa_renderer_empty_this_buffer(struct XACodecBase *base, struct xf_message *m)
{
	struct XARenderer *renderer = (struct XARenderer *) base;

	/* ...make sure the port is valid (what about multi-channel renderer?) */
	XF_CHK_ERR(XF_MSG_DST_PORT(m->id) == 0, ACODEC_PARA_ERROR);

	/* ...command is allowed only in "postinit" state */
	/* XF_CHK_ERR(base->state & XA_BASE_FLAG_POSTINIT, ACODEC_PARA_ERROR); */

	LOG2("received buffer [%p]:%u\n", m->buffer, m->length);

	/* ...put message into input port */
	if (xf_input_port_put(&renderer->input, m) && (base->state & XA_RENDERER_FLAG_OUTPUT_READY))
	{
		/* ...force data processing instantly */
		xa_base_schedule(base, 0);
	}

	return ACODEC_SUCCESS;
}

/* ...FILL-THIS-BUFFER command processing */
static UA_ERROR_TYPE xa_renderer_fill_this_buffer(struct XACodecBase *base, struct xf_message *m)
{
	struct XARenderer *renderer = (struct XARenderer *) base;

	/* ...make sure message is our internal one */
	XF_CHK_ERR(m == &renderer->msg, ACODEC_PARA_ERROR);

	/* ...atomically clear callback message scheduling flag */
	xf_atomic_clear(&renderer->schedule, 1);

	/* ...check if output port flag was not set */
	if ((base->state & XA_RENDERER_FLAG_OUTPUT_READY) == 0)
	{
		/* ...indicate ouput is ready */
		base->state ^= XA_RENDERER_FLAG_OUTPUT_READY;

		/* ...check if we have pending input */
		if (xf_input_port_ready(&renderer->input))
		{
			/* ...force data processing instantly */
			xa_base_schedule(base, 0);
		}
	}

	return ACODEC_SUCCESS;
}

/* ...FLUSH command processing */
static UA_ERROR_TYPE xa_renderer_flush(struct XACodecBase *base, struct xf_message *m)
{
	struct XARenderer *renderer = (struct XARenderer *) base;

	/* ...command is allowed only in "execution" state - not necessarily - tbd*/
	/* XF_CHK_ERR(base->state & XA_BASE_FLAG_EXECUTION, ACODEC_PARA_ERROR); */

	/* ...ensure input parameter length is zero */
	/* XF_CHK_ERR(m->length == 0, ACODEC_PARA_ERROR); */

	/* ...flush command must be addressed to input port */
	XF_CHK_ERR(XF_MSG_DST_PORT(m->id) == 0, ACODEC_PARA_ERROR);

	/* ...cancel data processing if needed */
	xa_base_cancel(base);

	/* ...input port flushing; purge content of input buffer */
	xf_input_port_purge(&renderer->input);

	/* ...pass response to caller */
	xf_response(m);

	return ACODEC_SUCCESS;
}

/*******************************************************************************
 * Completion callback - shall be a separate IRQ-safe code
 ******************************************************************************/

/* ...this code runs from interrupt handler; we need to protect data somehow */
static void xa_renderer_callback(xa_renderer_cb_t *cdata, u32 i_idx)
{
	struct XARenderer *renderer = container_of(cdata, struct XARenderer, cdata);
	struct XACodecBase    *base = (struct XACodecBase *) renderer;
	struct dsp_main_struct *dsp_config =
		(struct dsp_main_struct *)base->component.private_data;

	LOG("xa_renderer_callback\n");

	if (xf_atomic_test_and_set(&renderer->schedule, 1))
	{
		/* ...set internal scheduling message */
		renderer->msg.id = __XF_MSG_ID(0, ((struct xf_component *)renderer)->id);
		renderer->msg.opcode = XF_FILL_THIS_BUFFER;
		renderer->msg.length = 0;
		renderer->msg.buffer = NULL;

		xf_msg_submit(&dsp_config->queue, &renderer->msg);
	}
}

/*******************************************************************************
 * Codec API implementation
 ******************************************************************************/

/* ...buffers handling */
static UA_ERROR_TYPE xa_renderer_memtab(struct XACodecBase *base, u32 size, u32 align)
{
	struct XARenderer *renderer = (struct XARenderer *)base;
	struct dsp_main_struct *dsp_config =
		(struct dsp_main_struct *)base->component.private_data;

	/* ...only "input" buffers are supported */

	/* ...input buffer allocation; make sure input port index is sane */
	//    XF_CHK_ERR(idx == 0, XA_API_FATAL_INVALID_CMD_TYPE);

	/* ...create input port for a track */
	XF_CHK_ERR(xf_input_port_init(&renderer->input, size, align, &dsp_config->scratch_mem_info) == 0, ACODEC_PARA_ERROR);

	/* ...well, we want to use buffers without copying them into interim buffer */
	LOG1("renderer input port created - size=%u\n", size);

	/* ...set input port buffer if needed */
	(size ? XA_API(base, XF_API_CMD_SET_INPUT_PTR, 0, renderer->input.buffer) : 0);

	/* ...mark renderer output buffer is ready */
	base->state |= XA_RENDERER_FLAG_OUTPUT_READY;

	return ACODEC_SUCCESS;
}

/* ...preprocessing function */
static UA_ERROR_TYPE xa_renderer_preprocess(struct XACodecBase *base)
{
	struct XARenderer     *renderer = (struct XARenderer *) base;
	struct dsp_main_struct *dsp_config =
		(struct dsp_main_struct *)base->component.private_data;
	u32  filled = 0;

	LOG("renderer preprocess\n");
	/* ...check current execution stage */
	if (base->state & XA_BASE_FLAG_RUNTIME_INIT)
	{
		/* ...no special processing in runtime initialization stage */
		return ACODEC_SUCCESS;
	}

	/* ...submit input buffer to the renderer */
	if (xf_input_port_bypass(&renderer->input))
	{
		void   *input;

		/* ...in-place buffers used */
		if ((input = xf_input_port_data(&renderer->input)) != NULL)
		{
			/* ...set input buffer pointer */
			XA_API(base, XF_API_CMD_SET_INPUT_PTR, 0, input);

			/* ..retrieve number of bytes */
			filled = xf_input_port_length(&renderer->input);
		}
		else if (!xf_input_port_done(&renderer->input))
		{
			/* ...no input data available; do nothing */
			return ACODEC_NOT_ENOUGH_DATA;
		}
		else
		{
			/* ...input port is done; buffer is empty */
			filled = 0;
		}
	}
	else
	{
		/* ...port is in non-bypass mode; try to fill internal buffer */
		if (!xf_input_port_fill(&renderer->input))
		{
			/* ...insufficient input data */
//			xf_msg_submit(&dsp_config->queue, &renderer->msg);
//			return ACODEC_SUCCESS;
		}
//		else
//		{
			/* ...retrieve number of bytes put in buffer */
			filled = xf_input_port_level(&renderer->input);
//		}
		if (!renderer->input.remaining)
			xf_input_port_complete(&renderer->input);
	}

	/* ...set total number of bytes we have in buffer */
	XA_API(base, XF_API_CMD_SET_INPUT_BYTES, 0, &filled);

	/* ...check if input stream is over */
	if (xf_input_port_done(&renderer->input))
	{
		/* ...pass input-over command to plugin */
		XA_API(base, XF_API_CMD_INPUT_OVER, 0, NULL);
		/* ...input stream is over; complete pending zero-length message */
		xf_input_port_purge(&renderer->input);

		LOG(("renderer operation is over\n"));
	}

	return ACODEC_SUCCESS;
}

/* ...postprocessing function */
static UA_ERROR_TYPE xa_renderer_postprocess(struct XACodecBase *base, u32 ret)
{
	struct XARenderer     *renderer = (struct XARenderer *) base;
	u32             consumed;
	u32             i = 0;

	LOG1("renderer postprocess %d\n", ret);
	if (ret)
	{
		if (base->state & XA_BASE_FLAG_RUNTIME_INIT)
		{
			/* ...processing is done while in runtime initialization state (can't be - tbd) */
			BUG(1, _x("breakpoint"));
		}
		else if (base->state & XA_BASE_FLAG_EXECUTION)
		{
			/* ...runtime initialization is done */
//            XA_CHK(xa_renderer_prepare_runtime(renderer));

			/* ...reschedule execution instantly (both input- and output-ready conditions are set) */
			xa_base_schedule(base, 0);
			return ACODEC_SUCCESS;
		}
		else
		{
			/* ...renderer operation is completed (can't be - tbd) */
			BUG(1, _x("breakpoint"));
		}
	}

	/* ...get total amount of consumed bytes */
	XA_API(base, XF_API_CMD_GET_CONSUMED_BYTES, i, &consumed);

	/* ...input buffer maintenance; consume that amount from input port and check for end-of-stream condition */
	if (consumed)
	{
		/* ...consume bytes from input buffer */
		xf_input_port_consume(&renderer->input, consumed);

		/* ...reschedule execution if we have pending input */
		if (xf_input_port_ready(&renderer->input))
		{
			/* ...schedule execution with respect to urgency  */
			xa_base_schedule(base, consumed * renderer->factor);
		}
	}
	else
	{
		/* ...failed to put anything; clear OUTPUT-READY condition */
		base->state &= ~XA_RENDERER_FLAG_OUTPUT_READY;
	}

	return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xa_renderer_suspend(struct XACodecBase *base, struct xf_message *m) {

	XA_API(base, XF_API_CMD_SUSPEND, 0, NULL);

	return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xa_renderer_resume(struct XACodecBase *base, struct xf_message *m) {

	XA_API(base, XF_API_CMD_RESUME, 0, NULL);

	return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xa_renderer_pause(struct XACodecBase *base, struct xf_message *m) {

	XF_CHK_ERR((base->state & XA_BASE_FLAG_POSTINIT), ACODEC_PARA_ERROR);

	base->pause_state = base->state;

	XA_API(base, XF_API_CMD_PAUSE, 0, NULL);

	if (base->state & XA_BASE_FLAG_SCHEDULE)
		xa_base_cancel(base);

	LOG2("renderer[%d] pause, state: %d\n", base->codec_id, base->state);
	return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xa_renderer_pause_release(struct XACodecBase *base, struct xf_message *m) {

	XF_CHK_ERR((base->state & XA_BASE_FLAG_POSTINIT), ACODEC_PARA_ERROR);

	XA_API(base, XF_API_CMD_PAUSE_RELEASE, 0, NULL);

	if (base->pause_state & XA_BASE_FLAG_SCHEDULE)
		xa_base_schedule(base, 0);

	LOG2("renderer[%d] pause release, state: %d\n", base->codec_id, base->state);
	return ACODEC_SUCCESS;
}
/*******************************************************************************
 * Command-processing function
 ******************************************************************************/

/* ...command hooks */
static UA_ERROR_TYPE (* const xa_renderer_cmd[])(struct XACodecBase *, struct xf_message *) =
{
	/* ...set-parameter - actually, same as in generic case */
	[XF_OPCODE_TYPE(XF_SET_PARAM)] = xa_base_set_param,
	[XF_OPCODE_TYPE(XF_GET_PARAM)] = xa_base_get_param,

	/* ...input buffers processing */
	[XF_OPCODE_TYPE(XF_EMPTY_THIS_BUFFER)] = xa_renderer_empty_this_buffer,
	[XF_OPCODE_TYPE(XF_FILL_THIS_BUFFER)] = xa_renderer_fill_this_buffer,
	[XF_OPCODE_TYPE(XF_FLUSH)]  = xa_renderer_flush,

	[XF_OPCODE_TYPE(XF_RESUME)]  = xa_renderer_resume,
	[XF_OPCODE_TYPE(XF_SUSPEND)]  = xa_renderer_suspend,
	[XF_OPCODE_TYPE(XF_PAUSE)]  = xa_renderer_pause,
	[XF_OPCODE_TYPE(XF_PAUSE_RELEASE)]  = xa_renderer_pause_release,
};

/* ...total number of commands supported */
#define XA_RENDERER_CMD_NUM         (sizeof(xa_renderer_cmd) / sizeof(xa_renderer_cmd[0]))

/*******************************************************************************
 * Entry points
 ******************************************************************************/

/* ...renderer termination-state command processor */
static int xa_renderer_terminate(struct xf_component *component, struct xf_message *m)
{
	struct XARenderer *renderer = (struct XARenderer *) component;

	/* ...check if we received internal message */
	if (m == &renderer->msg)
	{
		/* ...callback execution completed; complete operation */
		return -1;
	}
	else if (m->opcode == XF_UNREGISTER)
	{
		/* ...ignore subsequent unregister command/response */
		return 0;
	}
	else
	{
		/* ...everything else is responded with generic failure */
		xf_response_err(m);
		return 0;
	}
}

/* ...renderer class destructor */
static int xa_renderer_destroy(struct xf_component *component, struct xf_message *m)
{
	struct XARenderer *renderer = (struct XARenderer *) component;
	u32    core = xf_component_core(component);
	struct dsp_main_struct *dsp_config =
		(struct dsp_main_struct *)component->private_data;

	/* ...destroy input port */
	xf_input_port_destroy(&renderer->input, &dsp_config->scratch_mem_info);

	/* ...cleanup renderer used mem */
	XA_API(&renderer->base, XF_API_CMD_CLEANUP, 0, NULL);

	/* ...destroy base object */
	xa_base_destroy(&renderer->base);

	LOG1(("renderer[%p] destroyed\n"), renderer);

	/* ...indicate the component is destroyed */
	return 0;
}

/* ...renderer class first-stage destructor */
static int xa_renderer_cleanup(struct xf_component *component, struct xf_message *m)
{
	struct XARenderer     *renderer = (struct XARenderer *) component;
	struct XACodecBase    *base = (struct XACodecBase *) renderer;
	u32  state = XA_RENDERER_STATE_IDLE;

	/* ...complete message with error result code */
	xf_response_err(m);

	/* ...cancel component task execution if needed */
	xa_base_cancel(base);

	/* ...stop hardware renderer if it's running */
	XA_API(base, XF_API_CMD_SET_PARAM, XA_RENDERER_CONFIG_PARAM_STATE, &state);

	/* ...purge input port */
	xf_input_port_purge(&renderer->input);

	/* ...check if we have internal message scheduled */
	if (xf_atomic_test_and_clear(&renderer->schedule, 1))
	{
		/* ...wait until callback message is returned */
		component->entry = xa_renderer_terminate;
		component->exit = xa_renderer_destroy;

		LOG1(("renderer[%p] cleanup sequence started\n"), renderer);

		/* ...indicate that second stage is required */
		return 1;
	}
	else
	{
		/* ...callback is not scheduled; destroy renderer */
		return xa_renderer_destroy(component, NULL);
	}
}

/* ...renderer class factory */
struct xf_component * xa_renderer_factory(struct dsp_main_struct *dsp_config,
                                                   xf_codec_func_t * process,
                                                   u32 type)
{
	struct XARenderer *renderer;

	/* ...construct generic audio component */
	XF_CHK_ERR(renderer = (struct XARenderer *)xa_base_factory(dsp_config,
								sizeof(*renderer),
								process,
								type),
								NULL);

	/* ...set generic codec API */
	renderer->base.memtab = xa_renderer_memtab;
	renderer->base.preprocess = xa_renderer_preprocess;
	renderer->base.postprocess = xa_renderer_postprocess;

	/* ...set message-processing table */
	renderer->base.command = xa_renderer_cmd;
	renderer->base.command_num = XA_RENDERER_CMD_NUM;

	/* ...set component destructor hook */
	renderer->base.component.exit = xa_renderer_cleanup;

	/* ...set notification callback data */
	renderer->cdata.cb = xa_renderer_callback;

	/* ...pass buffer completion callback to the component */
	XA_API(&renderer->base, XF_API_CMD_SET_PARAM, XA_RENDERER_CONFIG_PARAM_CB, &renderer->cdata);

	CODEC_API(&renderer->base, memtab, 4096, 0);

	LOG1(("Renderer[%p] created\n"), renderer);

	/* ...return handle to component */
	return (struct xf_component *) renderer;
}
