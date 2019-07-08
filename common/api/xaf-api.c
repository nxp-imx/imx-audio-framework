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
 *
 *****************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include "xaf-api.h"

#ifdef TGT_OS_ANDROID
#define CORE_LIB_PATH   "/vendor/lib/"
#else
#define CORE_LIB_PATH   "/usr/lib/imx-mm/audio-codec/dsp/"
#endif

int xaf_adev_open(struct xaf_adev_s *p_adev)
{
	struct xf_proxy *p_proxy;
	int ret = 0;

	memset(p_adev, 0, sizeof(struct xaf_adev_s));

	p_proxy = &p_adev->proxy;

	/* ...open DSP proxy - specify "DSP#0" */
	ret = xf_proxy_init(p_proxy, 0);
	if (ret) {
		TRACE("Error in proxy init, err = %d\n", ret);
		return ret;
	}

	/* ...create auxiliary buffers pool for control commands */
	ret = xf_pool_alloc(p_proxy,
			    XA_AUX_POOL_SIZE,
			    XA_AUX_POOL_MSG_LENGTH,
			    XF_POOL_AUX,
			    &p_proxy->aux);
	if (ret)
		TRACE("Error when creating auxiliary buffers pool\n");

	return ret;
}

int xaf_adev_close(struct xaf_adev_s *p_adev)
{
	struct xf_proxy *p_proxy;
	int ret = 0;

	p_proxy = &p_adev->proxy;
	ret = xf_pool_free(p_proxy->aux);
	if (ret)
		return ret;

	xf_proxy_close(p_proxy);

	return ret;
}

static void my_comp_response(struct xf_handle *h, struct xf_user_msg *msg)
{
	if (msg->opcode == XF_UNREGISTER) {
		/* ...component execution failed unexpectedly; die */
		TRACE("[0x%x] Abnormal termination\n", h);
	} else if ((msg->opcode == XF_EMPTY_THIS_BUFFER) ||
			 (msg->opcode == XF_FILL_THIS_BUFFER)) {
		/* ...submit response to no-timely delivery queue */
		xf_response_put_ack(h, msg);
	} else {
		/* ...submit response to asynchronous delivery queue */
		xf_response_put(h, msg);
	}
}

int xaf_comp_set_config(struct xaf_comp *p_comp, u32 num_param, void *p_param)
{
	struct xf_handle       *p_handle;
	struct xf_user_msg     rmsg;
	struct xf_set_param_msg     *smsg;
	struct xf_set_param_msg     *param = (struct xf_set_param_msg *)p_param;
	u32 i;
	int ret = 0;

	p_handle = &p_comp->handle;

	/* ...set persistent stream characteristics */
	smsg = xf_buffer_data(p_handle->aux);

	for (i = 0; i < num_param; i++) {
		smsg[i].id = param[i].id;
		smsg[i].value = param[i].value;
	}

	/* ...pass command to the component */
	ret = xf_command(p_handle,
			 0,
			 XF_SET_PARAM,
			 smsg,
			 sizeof(*smsg) * num_param);
	if (ret) {
		TRACE("Set parameter error: %d\n", ret);
		return ret;
	}

	/* ...wait until result is delivered */
	ret = xf_response_get(p_handle, &rmsg);
	if (ret) {
		TRACE("Setting parameter timeout: %d\n", ret);
		return ret;
	}

	/* ...make sure response is expected */
	if ((rmsg.opcode != XF_SET_PARAM) || (rmsg.buffer != smsg)) {
		TRACE("Not expected response when setting parameter\n");
		return -EPIPE;
	}

	return 0;
}

int xaf_comp_get_config(struct xaf_comp *p_comp, u32 num_param, void *p_param)
{
	struct xf_handle       *p_handle;
	struct xf_user_msg     rmsg;
	struct xf_get_param_msg     *smsg;
	struct xf_get_param_msg     *param = (struct xf_get_param_msg *)p_param;
	u32 i;
	int ret = 0;

	p_handle = &p_comp->handle;

	/* ...set persistent stream characteristics */
	smsg = xf_buffer_data(p_handle->aux);

	for (i = 0; i < num_param; i++)
		smsg[i].id = param[i].id;

	/* ...pass command to the component */
	ret = xf_command(p_handle,
			 0,
			 XF_GET_PARAM,
			 smsg,
			 num_param * sizeof(*smsg));
	if (ret) {
		TRACE("Get parameter error: %d\n", ret);
		return ret;
	}

	/* ...wait until result is delivered */
	ret = xf_response_get(p_handle, &rmsg);
	if (ret) {
		TRACE("Getting parameter timeout: %d\n", ret);
		return ret;
	}

	/* ...make sure response is expected */
	if ((rmsg.opcode != (u32)XF_GET_PARAM) || (rmsg.buffer != smsg)) {
		TRACE("Not expected response when setting parameter\n");
		return -EPIPE;
	}

	for (i = 0; i < num_param; i++)
		param[i].value = smsg[i].value;

	return 0;
}

int xaf_comp_flush(struct xaf_comp *p_comp)
{
	struct xf_handle       *p_handle;
	struct xf_user_msg     rmsg;
	int ret = 0;

	p_handle = &p_comp->handle;

	/* ...pass command to the component */
	ret = xf_command(p_handle, 0, XF_FLUSH, NULL, 0);
	if (ret) {
		TRACE("Component flush error: %d\n", ret);
		return ret;
	}

	/* ...wait until result is delivered */
	ret = xf_response_get(p_handle, &rmsg);
	if (ret) {
		TRACE("Component flush timeout: %d\n", ret);
		return ret;
	}

	/* ...make sure response is expected */
	if ((rmsg.opcode != (u32)XF_FLUSH) || rmsg.buffer) {
		TRACE("Not expected response when component flush\n");
		return -EPIPE;
	}

	return 0;
}

int xaf_comp_create(struct xaf_adev_s *p_adev,
		    struct xaf_comp *p_comp,
		    int comp_type)
{
	struct xf_proxy *p_proxy;
	struct xf_handle *p_handle;
	struct xf_buffer *buf;
	char lib_path[200];
	char lib_wrap_path[200];
	int ret = 0;

	memset((void *)p_comp, 0, sizeof(struct xaf_comp));

	p_proxy = &p_adev->proxy;
	p_handle = &p_comp->handle;
	p_comp->comp_type = comp_type;

	/* ...init codec lib and codec wrap lib */
	strcpy(lib_path, CORE_LIB_PATH);
	strcpy(lib_wrap_path, CORE_LIB_PATH);
	p_comp->codec_lib.filename = lib_path;
	p_comp->codec_wrap_lib.filename = lib_wrap_path;

	p_comp->codec_lib.lib_type = DSP_CODEC_LIB;
	if (comp_type == CODEC_MP3_DEC) {
		p_comp->dec_id = "audio-decoder/mp3";
		strcat(lib_path, "lib_dsp_mp3_dec.so");
	} else if (comp_type == CODEC_AAC_DEC) {
		p_comp->dec_id = "audio-decoder/aac";
		strcat(lib_path, "lib_dsp_aac_dec.so");
	} else if (comp_type == CODEC_BSAC_DEC) {
		p_comp->dec_id = "audio-decoder/bsac";
		strcat(lib_path, "lib_dsp_bsac_dec.so");
	} else if (comp_type == CODEC_DAB_DEC) {
		p_comp->dec_id = "audio-decoder/dabplus";
		strcat(lib_path, "lib_dsp_dabplus_dec.so");
	} else if (comp_type == CODEC_MP2_DEC) {
		p_comp->dec_id = "audio-decoder/mp2";
		strcat(lib_path, "lib_dsp_mp2_dec.so");
	} else if (comp_type == CODEC_DRM_DEC) {
		p_comp->dec_id = "audio-decoder/drm";
		strcat(lib_path, "lib_dsp_drm_dec.so");
	} else if (comp_type == CODEC_SBC_DEC) {
		p_comp->dec_id = "audio-decoder/sbc";
		strcat(lib_path, "lib_dsp_sbc_dec.so");
	} else if (comp_type == CODEC_SBC_ENC) {
		p_comp->dec_id = "audio-encoder/sbc";
		strcat(lib_path, "lib_dsp_sbc_enc.so");
	} else if (comp_type == CODEC_FSL_OGG_DEC) {
		p_comp->dec_id = "audio-decoder/ogg";
	} else if (comp_type == CODEC_FSL_MP3_DEC) {
		p_comp->dec_id = "audio-decoder/mp3ext";
	} else if (comp_type == CODEC_FSL_AAC_DEC) {
		p_comp->dec_id = "audio-decoder/aacext";
	} else if (comp_type == CODEC_FSL_AC3_DEC) {
		p_comp->dec_id = "audio-decoder/ac3";
	} else if (comp_type == CODEC_FSL_DDP_DEC) {
		p_comp->dec_id = "audio-decoder/ddp";
	} else if (comp_type == CODEC_FSL_NBAMR_DEC) {
		p_comp->dec_id = "audio-decoder/nbamr";
	} else if (comp_type == CODEC_FSL_WBAMR_DEC) {
		p_comp->dec_id = "audio-decoder/wbamr";
	} else if (comp_type == CODEC_FSL_WMA_DEC) {
		p_comp->dec_id = "audio-decoder/wma";
	}


	if (comp_type < CODEC_FSL_OGG_DEC)
		strcat(lib_wrap_path, "lib_dsp_codec_wrap.so");
	else if (comp_type == CODEC_FSL_OGG_DEC)
		strcat(lib_wrap_path, "lib_vorbisd_wrap_dsp.so");
	else if (comp_type == CODEC_FSL_MP3_DEC)
		strcat(lib_wrap_path, "lib_mp3d_wrap_dsp.so");
	else if (comp_type == CODEC_FSL_AAC_DEC)
		strcat(lib_wrap_path, "lib_aacd_wrap_dsp.so");
	else if (comp_type == CODEC_FSL_AC3_DEC)
		strcat(lib_wrap_path, "lib_ac3d_wrap_dsp.so");
	else if (comp_type == CODEC_FSL_DDP_DEC)
		strcat(lib_wrap_path, "lib_ddpd_wrap_dsp.so");
	else if (comp_type == CODEC_FSL_NBAMR_DEC)
		strcat(lib_wrap_path, "lib_nbamrd_wrap_dsp.so");
	else if (comp_type == CODEC_FSL_WBAMR_DEC)
		strcat(lib_wrap_path, "lib_wbamrd_wrap_dsp.so");
	else if (comp_type == CODEC_FSL_WMA_DEC)
		strcat(lib_wrap_path, "lib_wma10d_wrap_dsp.so");

	p_comp->codec_wrap_lib.lib_type = DSP_CODEC_WRAP_LIB;

	/* ...create decoder component instance (select core-0) */
	ret = xf_open(p_proxy, p_handle, p_comp->dec_id, 0, my_comp_response);
	if (ret) {
		TRACE("create (%s) component error: %d\n", p_comp->dec_id, ret);
		return ret;
	}

	/* ...load codec wrapper lib */
	ret = xf_load_lib(p_handle, &p_comp->codec_wrap_lib);
	if (ret) {
		TRACE("load codec wrapper lib error: %d\n", ret);
		return ret;
	}
	/* ...load codec lib */
	if (comp_type < CODEC_FSL_OGG_DEC) {
		ret = xf_load_lib(p_handle, &p_comp->codec_lib);
		if (ret) {
			TRACE("load codec lib error: %d\n", ret);
			return ret;
		}
	}

	/* ...allocate input buffer */
	ret = xf_pool_alloc(p_proxy,
			    1,
			    INBUF_SIZE,
			    XF_POOL_INPUT,
			    &p_comp->inpool);
	if (ret) {
		TRACE("allocate component input buffer error: %d\n", ret);
		return ret;
	}

	/* ...allocate output buffer */
	ret = xf_pool_alloc(p_proxy,
			    1,
			    OUTBUF_SIZE,
			    XF_POOL_OUTPUT,
			    &p_comp->outpool);
	if (ret) {
		TRACE("allocate component output buffer error: %d\n", ret);
		return ret;
	}

	/* ...initialize input buffer pointer */
	buf   = xf_buffer_get(p_comp->inpool);
	p_comp->inptr = xf_buffer_data(buf);

	/* ...initialize output buffer pointer */
	buf   = xf_buffer_get(p_comp->outpool);
	p_comp->outptr = xf_buffer_data(buf);

	return ret;
}

int xaf_comp_delete(struct xaf_comp *p_comp)
{
	struct xf_handle *p_handle;
	u32 ret = 0;

	p_handle = &p_comp->handle;

	/* ...unload codec wrapper library */
	xf_unload_lib(p_handle, &p_comp->codec_wrap_lib);

	/* ...unload codec library */
	xf_unload_lib(p_handle, &p_comp->codec_lib);

	/* ...delete component */
	xf_close(p_handle);

	/* ...free component input buffer */
	xf_pool_free(p_comp->inpool);

	/* ...free component output buffer */
	xf_pool_free(p_comp->outpool);

	return ret;
}

int xaf_comp_process(struct xaf_comp *p_comp, void *p_buf, u32 length, u32 flag)
{
	struct xf_handle *p_handle;
	u32 ret = 0;

	p_handle = &p_comp->handle;

	switch (flag) {
	case XF_FILL_THIS_BUFFER:
		/* ...send message to component output port (port-id=1) */
		ret = xf_command(p_handle,
				 1,
				 XF_FILL_THIS_BUFFER,
				 p_buf,
				 length);
		break;
	case XF_EMPTY_THIS_BUFFER:
		/* ...send message to component input port (port-id=0) */
		ret = xf_command(p_handle,
				 0,
				 XF_EMPTY_THIS_BUFFER,
				 p_buf,
				 length);
		break;
	default:
		break;
	}

	return ret;
}

int xaf_comp_get_status(struct xaf_comp *p_comp, struct xaf_info_s *p_info)
{
	struct xf_handle *p_handle;
	struct xf_user_msg msg;
	u32 ret = 0;

	p_handle = &p_comp->handle;

	TRACE("Waiting ...\n");
	/* ...wait until result is delivered */
	ret = xf_response_get_ack(p_handle, &msg);
	if (ret)
		return ret;

	p_info->opcode = msg.opcode;
	p_info->buf = msg.buffer;
	p_info->length = msg.length;
	p_info->ret = msg.ret;

	return ret;
}

int xaf_comp_get_msg_count(struct xaf_comp *p_comp)
{
	struct xf_handle *p_handle;

	p_handle = &p_comp->handle;

	/* ...get message count in pipe */
	return xf_response_get_ack_count(p_handle);
}

int xaf_connect(struct xaf_comp *p_src,
		struct xaf_comp *p_dest,
		u32 num_buf,
		u32 buf_length)
{
	int ret = 0;

	/* ...connect p_src output port with p_dest input port */
	ret = xf_route(&p_src->handle,
		       0,
		       &p_dest->handle,
		       0,
		       num_buf,
		       buf_length,
		       8);

	return ret;
}

int xaf_disconnect(struct xaf_comp *p_comp)
{
	int ret = 0;

	/* ...disconnect p_src output port with p_dest input port */
	ret = xf_unroute(&p_comp->handle, 0);

	return ret;
}

int xaf_comp_add(struct xaf_pipeline *p_pipe, struct xaf_comp *p_comp)
{
	int ret = 0;

	p_comp->next = p_pipe->comp_chain;
	p_comp->pipeline = p_pipe;
	p_pipe->comp_chain = p_comp;

	return ret;
}

int xaf_pipeline_create(struct xaf_adev_s *p_adev, struct xaf_pipeline *p_pipe)
{
	int ret = 0;

	memset(p_pipe, 0, sizeof(struct xaf_pipeline));

	return ret;
}

int xaf_pipeline_delete(struct xaf_pipeline *p_pipe)
{
	int ret = 0;

	memset(p_pipe, 0, sizeof(struct xaf_pipeline));

	return ret;
}

int xaf_pipeline_send_eos(struct xaf_pipeline *p_pipe)
{
	struct xaf_comp *p;
	struct xf_user_msg msg;
	int ret = 0;

	msg.id = 0;
	msg.opcode = XF_OUTPUT_EOS;
	msg.length = 0;
	msg.buffer = NULL;
	msg.ret = 0;

	for (p = p_pipe->comp_chain; p; p = p->next)
		/* ...submit response to no-timely delivery queue */
		xf_response_put_ack(&p->handle, &msg);

	return ret;
}
