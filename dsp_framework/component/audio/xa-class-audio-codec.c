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
 * xa-class-audio-codec.c
 *
 * Generic audio codec task implementation
 *
 ******************************************************************************/

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "xa-class-base.h"
#include "xf-io.h"

/*******************************************************************************
 * Tracing configuration
 ******************************************************************************/

/*******************************************************************************
 * Internal functions definitions
 ******************************************************************************/

struct XAAudioCodec {
	/*********************************
	 * Control data
	 ********************************/

	/* ...generic audio codec data */
	struct XACodecBase      base;

	/* ...input port data */
	struct xf_input_port    input;

	/* ...output port data */
	struct xf_output_port   output;

	/* ...input port index */
	u32                  in_idx;

	/* ...output port index */
	u32                  out_idx;

	/************************************
	 * Run-time configuration parameters
	 ***********************************/

	/* ...sample size in bytes */
	u32                     sample_size;

	/* ...audio sample duration */
	u32                     factor;

	/* ...total number of produced audio frames since last reset */
	u32                     produced;

	/* ...indicate input stream is over */
	u32                     input_over;

	/* loading library info of codec wrap */
	struct dpu_lib_stat_t lib_codec_wrap_stat;

	/* loading library info of codec */
	struct dpu_lib_stat_t lib_codec_stat;

};

/*******************************************************************************
 * Auxiliary codec execution flags
 ******************************************************************************/

/* ...input port setup condition */
#define XA_CODEC_FLAG_INPUT_SETUP       __XA_BASE_FLAG(1 << 0)

/* ...output port setup condition */
#define XA_CODEC_FLAG_OUTPUT_SETUP      __XA_BASE_FLAG(1 << 1)

#ifdef PLATF_8MP_LPA
#define INPUT_BUFFER_LEN (32*1024)
/* ...SRC GPR10, indicate ATF control power, 0xD for LPA, 0x1D for LPA with */
/* ...DRAM. 0x0 for DSP idle */
#define FLAG_ADDRESS     0x30390098
#define FLAG_IDLE        0x0
#define FLAG_LPA         0xD
#define FLAG_LPA_DRAM    0x1D
#define DSP_IDLE         {write32(FLAG_ADDRESS, FLAG_IDLE);}
#define DSP_LPA          {write32(FLAG_ADDRESS, FLAG_LPA);}
#define DSP_LPA_DRAM     {write32(FLAG_ADDRESS, FLAG_LPA_DRAM);}
#define input_sync 0
#else
#define INPUT_BUFFER_LEN (4096)
#define DSP_IDLE         {}
#define DSP_LPA          {}
#define DSP_LPA_DRAM     {}
#define input_sync 1
#endif

#define INPUT_SYNC 1

/*******************************************************************************
 * Data processing scheduling
 ******************************************************************************/

/*******************************************************************************
 * Commands processing
 ******************************************************************************/

/* ...XF_LOAD_LIB command processing */
static UA_ERROR_TYPE xa_codec_lib_load(struct XACodecBase *base,
					struct xf_message *m)
{
	struct XAAudioCodec *codec = (struct XAAudioCodec *)base;
	struct icm_xtlib_pil_info  *cmd = m->buffer;
	struct dpu_lib_stat_t *lib_stat;
	void *lib_interface;

	switch(cmd->lib_type) {
	case DSP_CODEC_LIB:
		lib_stat = &codec->lib_codec_stat;
		break;
	case DSP_CODEC_WRAP_LIB:
		lib_stat = &codec->lib_codec_wrap_stat;
		break;
	default:
		LOG("Unknown lib type\n");
		m->ret = ACODEC_INIT_ERR;
		xf_response(m);

		return ACODEC_INIT_ERR;
	}

	lib_interface = dpu_process_init_pi_lib(&cmd->pil_info, lib_stat, 0);
	if (!lib_interface) {
		LOG2("lib load error: lib_type = %d, lib_entry = 0x%x\n",
		     cmd->lib_type, lib_interface);
		m->ret = ACODEC_INIT_ERR;
		xf_response(m);
		return ACODEC_INIT_ERR;
	}

	/* save codec entry addr */
	if (cmd->lib_type == DSP_CODEC_LIB)
		base->codecinterface = lib_interface;

	/* ...set codec lib handle if codec is loaded as library*/
	XA_API(base,
	       XF_API_CMD_SET_LIB_ENTRY,
	       cmd->lib_type,
	       lib_interface);

	LOG2("load lib success: lib_type = %d, lib_entry = %x\n",
	     cmd->lib_type,
	     lib_interface);
	xf_response_ok(m);

	return ACODEC_SUCCESS;
}

/* ...XF_UNLOAD_LIB command processing */
static UA_ERROR_TYPE xa_codec_lib_unload(struct XACodecBase *base,
					  struct xf_message *m)
{
	struct XAAudioCodec *codec = (struct XAAudioCodec *)base;
	struct icm_xtlib_pil_info  *cmd = m->buffer;
	struct dpu_lib_stat_t *lib_stat;
	int do_cleanup = 0;

	switch(cmd->lib_type) {
	case DSP_CODEC_LIB:
		lib_stat = &codec->lib_codec_stat;
		break;
	case DSP_CODEC_WRAP_LIB:
		lib_stat = &codec->lib_codec_wrap_stat;
		do_cleanup = 1;
		break;
	default:
		/* ... just don't care about stray unloads */
		lib_stat = NULL;
		LOG("Unknown lib type\n");
	}

	if (lib_stat && lib_stat->stat == lib_loaded) {
		/* ...destory codec resources */
		if (do_cleanup)
			XA_API(base, XF_API_CMD_CLEANUP, 0, NULL);

		dpu_process_unload_pi_lib(lib_stat);
	}

	LOG("unload lib successfully\n");
	xf_response_ok(m);

	return ACODEC_SUCCESS;
}

/* ...EMPTY-THIS-BUFFER command processing */
static UA_ERROR_TYPE xa_codec_empty_this_buffer(struct XACodecBase *base,
						 struct xf_message *m)
{
	struct XAAudioCodec *codec = (struct XAAudioCodec *)base;

	/* ...make sure the port is sane */
	XF_CHK_ERR(XF_MSG_DST_PORT(m->id) == 0, ACODEC_PARA_ERROR);

	/* ...command is allowed only in post-init state */
	/*XF_CHK_ERR(base->state & XA_BASE_FLAG_POSTINIT, ACODEC_PARA_ERROR);*/

	/* ...put message into input queue */
	if (xf_input_port_put(&codec->input, m)) {
		/* ...restart stream if it is in completed state */
		if (base->state & XA_BASE_FLAG_COMPLETED) {
			/* ...reset execution stage */
			base->state = XA_BASE_FLAG_POSTINIT |
				XA_BASE_FLAG_EXECUTION;

			/* ...reset produced samples counter */
			codec->produced = 0;
		}

		/* ...codec must be in one of these states */
		/* XF_CHK_ERR(base->state & (XA_BASE_FLAG_RUNTIME_INIT |
					XA_BASE_FLAG_EXECUTION), ACODEC_PARA_ERROR); */

		/* ...schedule data processing if output is ready */
		if (xf_output_port_ready(&codec->output))
			xa_base_schedule(base, 0);
	}

	LOG2("Received input buffer [%x]:%x\n", m->buffer, m->length);

	return ACODEC_SUCCESS;
}

/* ...FILL-THIS-BUFFER command processing */
static UA_ERROR_TYPE xa_codec_fill_this_buffer(struct XACodecBase *base,
						struct xf_message *m)
{
	struct XAAudioCodec *codec = (struct XAAudioCodec *)base;

	/* ...make sure the port is sane */
	/* XF_CHK_ERR(XF_MSG_DST_PORT(m->id) == 1, ACODEC_PARA_ERROR); */

	/* ...command is allowed only in postinit state */
	/* XF_CHK_ERR(base->state & XA_BASE_FLAG_POSTINIT, ACODEC_PARA_ERROR);*/

	/* ...special handling of zero-length buffer */
	if (base->state & XA_BASE_FLAG_RUNTIME_INIT) {
		/* ...message must be zero-length */
		BUG(m->length != 0,
		    _x("Invalid message length: %u"), m->length);
	} else if (m == xf_output_port_control_msg(&codec->output)) {
		/* ...end-of-stream processing indication received */
		BUG((base->state & XA_BASE_FLAG_COMPLETED) == 0,
		    _x("invalid state: %x"), base->state);

		/* ... mark flushing sequence is done */
		xf_output_port_flush_done(&codec->output);

		/* ...complete pending zero-length input buffer */
		xf_input_port_purge(&codec->input);

		LOG1("codec[%d] playback completed\n", base->codec_id);

		/* ...playback is over */
		return ACODEC_SUCCESS;
	} else if ((base->state & XA_BASE_FLAG_COMPLETED) &&
				!xf_output_port_routed(&codec->output)) {
		/* ...return message arrived from application immediately */
		xf_response_ok(m);

		return ACODEC_SUCCESS;
	} else {
		LOG2("Received output buffer [%x]:%x\n", m->buffer, m->length);

		/* ...adjust message length (may be shorter than original) */
		m->length = codec->output.length;
	}

	/* ...place message into output port */
	if (xf_output_port_put(&codec->output, m)) {
		/* ...codec must be in one of these states */
		XF_CHK_ERR(base->state & (XA_BASE_FLAG_RUNTIME_INIT |
					XA_BASE_FLAG_EXECUTION), ACODEC_PARA_ERROR);

		if (xf_input_port_ready(&codec->input) || xf_input_port_level(&codec->input)) {
			/* ...schedule data processing instantly */
			xa_base_schedule(base, 0);
		}
	}

	return ACODEC_SUCCESS;
}

/* ...output port routing */
static UA_ERROR_TYPE xa_codec_port_route(struct XACodecBase *base,
					  struct xf_message *m)
{
	struct dsp_main_struct *dsp_config =
		(struct dsp_main_struct *)base->component.private_data;
	struct XAAudioCodec *codec = (struct XAAudioCodec *)base;
	struct xf_route_port_msg *cmd = m->buffer;
	struct xf_output_port  *port = &codec->output;
	u32                     src = XF_MSG_DST(m->id);
	u32                     dst = cmd->dst;

	/* ...command is allowed only in "postinit" state */
	/* XF_CHK_ERR(base->state & XA_BASE_FLAG_POSTINIT, ACODEC_PARA_ERROR);*/

	/* ...make sure output port is addressed */
	/* XF_CHK_ERR(XF_MSG_DST_PORT(m->id) == 1, ACODEC_PARA_ERROR);*/

	/* ...make sure port is not routed yet */
	XF_CHK_ERR(!xf_output_port_routed(port), ACODEC_PARA_ERROR);

	/* ...route output port - allocate queue */
	XF_CHK_ERR(xf_output_port_route(port,
					__XF_MSG_ID(dst, src),
					cmd->alloc_number,
					cmd->alloc_size,
					cmd->alloc_align,
					&dsp_config->scratch_mem_info) == 0,
					ACODEC_INSUFFICIENT_MEM);

	if ((xf_input_port_ready(&codec->input)))
		xa_base_schedule(base, 0);

	/* ...pass success result to caller */
	xf_response_ok(m);

	return ACODEC_SUCCESS;
}

/* ...port unroute command */
static UA_ERROR_TYPE xa_codec_port_unroute(struct XACodecBase *base,
					    struct xf_message *m)
{
	struct dsp_main_struct *dsp_config =
		(struct dsp_main_struct *)base->component.private_data;
	struct XAAudioCodec *codec = (struct XAAudioCodec *)base;

	/* ...command is allowed only in "postinit" state */
	XF_CHK_ERR(base->state & XA_BASE_FLAG_POSTINIT, ACODEC_PARA_ERROR);

	/* ...make sure output port is addressed */
	XF_CHK_ERR(XF_MSG_DST_PORT(m->id) == 1, ACODEC_PARA_ERROR);

	/* ...cancel any pending processing */
	xa_base_cancel(base);

	/* ...clear output-port-setup condition */
	base->state &= ~XA_CODEC_FLAG_OUTPUT_SETUP;

	/* ...pass flush command down the graph */
	if (xf_output_port_flush(&codec->output, XF_FLUSH)) {
		LOG("port is idle; instantly unroute\n");

		/* ...flushing sequence is not needed;
		 * command may be satisfied instantly
		 */
		xf_output_port_unroute(&codec->output,
				       &dsp_config->scratch_mem_info);

		/* ...pass response to the proxy */
		xf_response_ok(m);
	} else {
		LOG("port is busy; propagate unroute command\n");

		/* ...flushing sequence is started; save flow-control message */
		xf_output_port_unroute_start(&codec->output, m);
	}

	return ACODEC_SUCCESS;
}

/* ...FLUSH command processing */
static UA_ERROR_TYPE xa_codec_flush(struct XACodecBase *base, struct xf_message *m)
{
	struct XAAudioCodec *codec = (struct XAAudioCodec *)base;
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	/* ...command is allowed only in "postinit" state */
	/* XF_CHK_ERR(base->state & XA_BASE_FLAG_POSTINIT, ACODEC_PARA_ERROR); */

	/* ...ensure input parameter length is zero */
	/* XF_CHK_ERR(m->length == 0, ACODEC_PARA_ERROR); */

	LOG("flush command received\n");

	/* ...flush command must be addressed to input port */
	if (XF_MSG_DST_PORT(m->id) == 0) {
		/* ...cancel data processing message if needed */
		xa_base_cancel(base);

		/* ...input port flushing; purge content of input buffer */
		xf_input_port_purge(&codec->input);

		/* ...clear input-ready condition */
		base->state &= ~XA_CODEC_FLAG_INPUT_SETUP;

		/* ...reset execution runtime */
		XA_API(base, XF_API_CMD_RUNTIME_INIT, 0, NULL);

		/* ...reset produced samples counter */
		codec->produced = 0;

		/* ...propagate flushing command to output port */
		if (xf_output_port_flush(&codec->output, XF_FLUSH)) {
			/* ...flushing sequence is not needed;
			 * satisfy command instantly
			 */
			xf_response(m);
		} else {
			/* ...flushing sequence is started;
			 * save flow-control message at input port
			 */
			xf_input_port_control_save(&codec->input, m);
		}
	} else if (xf_output_port_unrouting(&codec->output)) {
		/* ...flushing during port unrouting;
		 * complete unroute sequence
		 */
		xf_output_port_unroute_done(&codec->output);

		LOG("port is unrouted\n");
	} else {
		/* ...output port flush command/response;
		 * check if the port is routed
		 */
		if (!xf_output_port_routed(&codec->output)) {
			/* ...complete all queued messages */
			xf_output_port_flush(&codec->output, XF_FLUSH);

			/* ...and pass response to flushing command */
			xf_response(m);
		} else {
			/* ...response to flushing command received */
			BUG(m != xf_output_port_control_msg(&codec->output),
			    _x("invalid message: %p"), m);

			/* ...mark flushing sequence is completed */
			xf_output_port_flush_done(&codec->output);

			/* ...complete original flow-control command */
			xf_input_port_purge_done(&codec->input);
		}

		/* ...clear output-setup condition */
		base->state &= ~XA_CODEC_FLAG_OUTPUT_SETUP;
	}

	return ACODEC_SUCCESS;
}

/*******************************************************************************
 * Generic codec API
 ******************************************************************************/

/* ...memory buffer handling */
static UA_ERROR_TYPE xa_codec_memtab(struct XACodecBase *base,
				      u32 size,
				      u32 align)
{
	struct XAAudioCodec *codec = (struct XAAudioCodec *)base;
	struct dsp_main_struct *dsp_config =
		(struct dsp_main_struct *)base->component.private_data;
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	/* ...input port specification; allocate internal buffer */
	XF_CHK_ERR(xf_input_port_init(&codec->input, size, align,
				      &dsp_config->scratch_mem_info) == 0,
				      ACODEC_INSUFFICIENT_MEM);

	/* ...initialize output port queue (no allocation here yet) */
	XF_CHK_ERR(xf_output_port_init(&codec->output, 16384) == 0,
		   ACODEC_INSUFFICIENT_MEM);

	return ret;
}

/* ...prepare input/output buffers */
static UA_ERROR_TYPE xa_codec_preprocess(struct XACodecBase *base)
{
	struct XAAudioCodec *codec = (struct XAAudioCodec *)base;

	/* ...prepare output buffer if needed */
	if (!(base->state & XA_CODEC_FLAG_OUTPUT_SETUP)) {
		if (!xf_output_port_fill(&codec->output)) {
			/* ...no output buffer available */
			return ACODEC_NO_OUTPUT;
		}

		LOG("set output ptr ok\n");

		/* ...set the output buffer pointer */
		XA_API(base,
		       XF_API_CMD_SET_OUTPUT_PTR,
			   0,
			   codec->output.buffer);

		/* ...mark output port is setup */
		base->state ^= XA_CODEC_FLAG_OUTPUT_SETUP;
	}

	/* ...prepare input data if needed */
	if (!(base->state & XA_CODEC_FLAG_INPUT_SETUP)) {
		void   *input;
		u32     filled;

		/* ...fill input buffer */
		if (!xf_input_port_bypass(&codec->input)) {
			/* ...port is in non-bypass mode;
			 * try to fill internal buffer
			 */
			if (xf_input_port_done(&codec->input) ||
			    xf_input_port_fill(&codec->input)) {
				/* ...retrieve number of bytes
				 * in input buffer
				 */
				filled = xf_input_port_level(&codec->input);
			} else {
				/* ...return non-fatal indication to
				 * prevent further processing
				 */
			//    return XA_NO_OUTPUT;
			}
		}

		LOG1("input-buffer fill-level: %d bytes\n", filled);

		/* ...check if input stream is over */
		if (xf_input_port_done(&codec->input)) {
			/* ...pass input-over command to the codec
			 * to indicate the final buffer
			 */
			XA_API(base, XF_API_CMD_INPUT_OVER, 0, NULL);
			LOG1("Codec[%d] signal input-over\n", base->codec_id);
		}

		/* ...set input buffer pointer as needed */
		XA_API(base, XF_API_CMD_SET_INPUT_PTR, 0, codec->input.buffer);

		/* ...specify number of bytes available in the input buffer */
		XA_API(base,
		       XF_API_CMD_SET_INPUT_BYTES,
			   0,
			   &codec->input.filled);

		/* ...mark input port is setup */
		base->state ^= XA_CODEC_FLAG_INPUT_SETUP;
	}

	LOG1("Codec[%d] pre-process completed\n", base->codec_id);
	return ACODEC_SUCCESS;
}

/* ...post-processing operation; input/output ports maintenance */
static UA_ERROR_TYPE xa_codec_postprocess(struct XACodecBase *base, u32 ret)
{
	struct XAAudioCodec *codec = (struct XAAudioCodec *)base;
	struct xf_input_port *input_port = &codec->input;
	u32 consumed, produced;

	/* ...get number of consumed / produced bytes */
	XA_API(base, XF_API_CMD_GET_CONSUMED_BYTES, 0, &consumed);

	/* ...get number of produced bytes */
	XA_API(base, XF_API_CMD_GET_OUTPUT_BYTES, 0, &produced);

	LOG3("codec[%d]::postprocess(c=%d, p=%d)\n",
	     base->codec_id, consumed, produced);

	/* ...input buffer maintenance; check if we consumed anything */
	if (consumed) {
		/* ...consume specified number of bytes from input port */
		xf_input_port_consume(&codec->input, consumed);

			base->state ^= XA_CODEC_FLAG_INPUT_SETUP;

	}

	/* ...output buffer maintenance; check if we have produced anything */
	if ((xf_output_port_routed(&codec->output) && produced) || (!xf_output_port_routed(&codec->output))) {
		/* ...immediately complete output buffer
		 * (don't wait until it gets filled)
		 */
		xf_output_port_produce(&codec->output, produced, ret);

		/* ...clear output port setup flag */
		base->state ^= XA_CODEC_FLAG_OUTPUT_SETUP;
	}

	/* ...process execution stage transition */
	if (xf_input_port_done(&codec->input) &&
	    !produced && ret) {
		/* ...output stream is over; propagate condition to sink port */
		if (xf_output_port_flush(&codec->output, XF_FILL_THIS_BUFFER)) {
			/* ...flushing sequence is not needed;
			 * complete pending zero-length input
			 */
			xf_input_port_purge(&codec->input);

			/* ...clear input-ready condition */
			base->state &= ~XA_CODEC_FLAG_INPUT_SETUP;
			/* ...no propagation to output port */
			LOG1("codec[%d] playback completed\n",
			     base->codec_id);
		} else {
			/* ...flushing sequence is started;
			 * wait until flow-control message returns
			 */
			LOG("propagate end-of-stream condition\n");
		}

		/* ...return early to prevent task rescheduling */
		return ACODEC_SUCCESS;
	}

	/* ...reschedule processing if needed */
	if ((xf_input_port_ready(&codec->input) || xf_input_port_level(&codec->input)) &&
	    xf_output_port_ready(&codec->output)) {
		/* ...schedule data processing with respect to its urgency */
		xa_base_schedule(base, produced * codec->factor);
	}

	LOG1("Codec[%d] post-process completed\n", base->codec_id);

	return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xa_codec_suspend(struct XACodecBase *base, struct xf_message *m) {

	return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xa_codec_resume(struct XACodecBase *base, struct xf_message *m) {

	return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xa_codec_pause(struct XACodecBase *base, struct xf_message *m) {

	LOG1("Codec[%d] pause\n", base->codec_id);
	return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xa_codec_pause_release(struct XACodecBase *base, struct xf_message *m) {

	LOG1("Codec[%d] pause release\n", base->codec_id);
	return ACODEC_SUCCESS;
}
/*******************************************************************************
 * Component entry point
 ******************************************************************************/

/* ...command hooks */
static UA_ERROR_TYPE (* const xa_codec_cmd[])(struct XACodecBase *, struct xf_message *) = {
	[XF_OPCODE_TYPE(XF_SET_PARAM)] = xa_base_set_param,
	[XF_OPCODE_TYPE(XF_GET_PARAM)] = xa_base_get_param,
	[XF_OPCODE_TYPE(XF_ROUTE)] = xa_codec_port_route,
	[XF_OPCODE_TYPE(XF_UNROUTE)] = xa_codec_port_unroute,
	[XF_OPCODE_TYPE(XF_EMPTY_THIS_BUFFER)] = xa_codec_empty_this_buffer,
	[XF_OPCODE_TYPE(XF_FILL_THIS_BUFFER)] = xa_codec_fill_this_buffer,
	[XF_OPCODE_TYPE(XF_FLUSH)] = xa_codec_flush,
	[XF_OPCODE_TYPE(XF_RESUME)]  = xa_codec_resume,
	[XF_OPCODE_TYPE(XF_SUSPEND)]  = xa_codec_suspend,
	[XF_OPCODE_TYPE(XF_PAUSE)]  = xa_codec_pause,
	[XF_OPCODE_TYPE(XF_PAUSE_RELEASE)]  = xa_codec_pause_release,
	[XF_OPCODE_TYPE(XF_LOAD_LIB)] = xa_codec_lib_load,
	[XF_OPCODE_TYPE(XF_UNLOAD_LIB)] = xa_codec_lib_unload,
};

/* ...total number of commands supported */
#define XA_CODEC_CMD_NUM (sizeof(xa_codec_cmd) / sizeof(xa_codec_cmd[0]))

/* ...component port accessor */
static struct xf_output_port *xa_audio_codec_port(struct xf_component *comp, u32 idx)
{
	struct XAAudioCodec *codec = (struct XAAudioCodec *)comp;

	/* ...return output port data */
	return (idx == 1 ? &codec->output : NULL);
}

/* ...command processor for termination state (only for routed port case) */
static int xa_audio_codec_terminate(struct xf_component *component, struct xf_message *m)
{
	struct XAAudioCodec *codec = (struct XAAudioCodec *)component;
	u32             opcode = m->opcode;

	/* ...check if we received output port control message */
	if (m == xf_output_port_control_msg(&codec->output)) {
		/* ...output port flushing complete;
		 * mark port is idle and terminate
		 */
		xf_output_port_flush_done(&codec->output);
		return -1;
	} else if (opcode == XF_FILL_THIS_BUFFER) {
		/* ...output buffer returned by the sink component;
		 * ignore and keep waiting
		 */
		LOG("collect output buffer\n");
		return 0;
	} else if (opcode == XF_UNREGISTER) {
		/* ...ignore subsequent unregister command/response - tbd */
		return 0;
	} else {
		/* ...everything else is responded with generic failure */
		xf_response_err(m);
		return 0;
	}
}

/* ...audio codec destructor */
static int xa_audio_codec_destroy(struct xf_component *component, struct xf_message *m)
{
	struct XACodecBase  *base = (struct XACodecBase *)component;
	struct XAAudioCodec *codec = (struct XAAudioCodec *)component;
	struct dsp_main_struct *dsp_config =
		(struct dsp_main_struct *)base->component.private_data;

	/* ...destroy input port */
	xf_input_port_destroy(&codec->input, &dsp_config->scratch_mem_info);

	/* ...destroy output port */
	xf_output_port_destroy(&codec->output, &dsp_config->scratch_mem_info);

	/* ...deallocate all resources */
	xa_base_destroy(&codec->base);

	LOG1("audio-codec[%d] destroyed\n", base->codec_id);

	/* ...indicate the client has been destroyed */
	return 0;
}

/* ...audio codec destructor - first stage (ports unrouting) */
static int xa_audio_codec_cleanup(struct xf_component *component, struct xf_message *m)
{
	struct XAAudioCodec *codec = (struct XAAudioCodec *)component;

	/* ...complete message with error response */
	xf_response_err(m);

	/* ...cancel internal scheduling message if needed */
	xa_base_cancel(&codec->base);

	/* ...purge input port (returns OK?
	 * pretty strange at this point - tbd)
	 */
	xf_input_port_purge(&codec->input);

	DSP_IDLE;

	/* ...propagate unregister command to connected component */
	if (xf_output_port_flush(&codec->output, XF_FLUSH)) {
		/* ...flushing sequence is not needed; destroy audio codec */
		return xa_audio_codec_destroy(component, NULL);
	}

	/* ...wait until output port is cleaned; adjust component hooks */
	component->entry = xa_audio_codec_terminate;
	component->exit = xa_audio_codec_destroy;

	LOG1("codec[%d] cleanup sequence started\n", codec->base.codec_id);

	/* ...indicate that second stage is required */
	return 1;
}

/*******************************************************************************
 * Audio codec component factory
 ******************************************************************************/

struct xf_component *xa_audio_codec_factory(struct dsp_main_struct *dsp_config,
					    xf_codec_func_t *process,
					    u32 type)
{
	struct XAAudioCodec *codec;

	/* ...allocate local memory for codec structure */
	XF_CHK_ERR(codec = (struct XAAudioCodec *)xa_base_factory(dsp_config,
								  sizeof(*codec),
								  process,
								  type), NULL);

	/* ...set base codec API methods */
	codec->base.memtab = xa_codec_memtab;
	codec->base.preprocess = xa_codec_preprocess;
	codec->base.postprocess = xa_codec_postprocess;

	/* ...set message commands processing table */
	codec->base.command = xa_codec_cmd;
	codec->base.command_num = XA_CODEC_CMD_NUM;

	/* ...set component destructor hook */
	codec->base.component.exit = xa_audio_codec_cleanup;

	/* ...initialize input and output port */
	CODEC_API(&codec->base, memtab, INPUT_BUFFER_LEN, 0);

	if (CODEC_MP3_DEC == codec->base.codec_id) {
		DSP_LPA;
	} else {
		DSP_LPA_DRAM;
	}

	LOG1("Codec[%d] initialized\n", codec->base.codec_id);
	return (struct xf_component *)codec;
}
