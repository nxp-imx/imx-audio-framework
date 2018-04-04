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
******************************************************************************/


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include "xaf-api.h"


#ifdef  TGT_OS_ANDROID
#define CORE_LIB_PATH   "/vendor/lib/"
#else
#define CORE_LIB_PATH   "/usr/lib/imx-mm/audio-codec/hifi/"
#endif


int xaf_adev_open(xaf_adev_t *p_adev)
{
	xf_proxy_t *p_proxy;
	int ret = 0;

	memset(p_adev, 0, sizeof(xaf_adev_t));

	p_proxy = &p_adev->proxy;

	/* ...open DSP proxy - specify "DSP#0" */
	ret = xf_proxy_init(p_proxy, 0);
	if(ret)
	{
		TRACE("Error in proxy init, err = %d\n", ret);
		return ret;
	}

	/* ...create auxiliary buffers pool for control commands */
	ret = xf_pool_alloc(p_proxy, XA_AUX_POOL_SIZE, XA_AUX_POOL_MSG_LENGTH, XF_POOL_AUX, &p_proxy->aux);
	if(ret)
	{
		TRACE("Error when creating auxiliary buffers pool, err = %d\n", ret);
	}

	return ret;
}

int xaf_adev_close(xaf_adev_t *p_adev)
{
	xf_proxy_t *p_proxy;
	int ret = 0;

	p_proxy = &p_adev->proxy;
	ret = xf_pool_free(p_proxy->aux);
	if(ret)
	{
		return ret;
	}

	xf_proxy_close(p_proxy);

	return ret;
}

static void my_comp_response(xf_handle_t *h, xf_user_msg_t *msg)
{
	if (msg->opcode == XF_UNREGISTER)
	{
		/* ...component execution failed unexpectedly; die */
		BUG(1, _x("[%p] Abnormal termination"), h);
	}
	else if ((msg->opcode == XF_EMPTY_THIS_BUFFER) || (msg->opcode == XF_FILL_THIS_BUFFER))
	{
		/* ...submit response to no-timely delivery queue */
		xf_response_put_ack(h, msg);
	}
	else
	{
		/* ...submit response to asynchronous delivery queue */
		xf_response_put(h, msg);
	}
}

int xaf_comp_set_config(xaf_comp_t *p_comp, u32 num_param, void *p_param)
{
	xf_handle_t            *p_handle;
	xf_user_msg_t           rmsg;
	xf_set_param_msg_t     *smsg;
	xf_set_param_msg_t     *param = (xf_set_param_msg_t *)p_param;
	u32 i;
	int ret = 0;

	p_handle = &p_comp->handle;

	/* ...set persistent stream characteristics */
	smsg = xf_buffer_data(p_handle->aux);

	for(i = 0; i < num_param; i++)
	{
		smsg[i].id = param[i].id;
		smsg[i].value = param[i].value;
	}

	/* ...pass command to the component */
	ret = xf_command(p_handle, 0, XF_SET_PARAM, smsg, sizeof(*smsg) * num_param);
	if(ret)
	{
		TRACE("Error when passing cmd to component in set parameter function, err = %d\n", ret);
		return ret;
	}

	/* ...wait until result is delivered */
	ret = xf_response_get(p_handle, &rmsg);
	if(ret)
	{
		TRACE("response timeout when setting parameter, err = %d\n", ret);
		return ret;
	}

	/* ...make sure response is expected */
	if((rmsg.opcode != XF_SET_PARAM) || (rmsg.buffer != smsg))
	{
		TRACE("response is not expected when setting parameter, err = %d\n", -EPIPE);
		return -EPIPE;
	}

	return 0;
}

int xaf_comp_get_config(xaf_comp_t *p_comp, u32 num_param, void *p_param)
{
	xf_handle_t            *p_handle;
	xf_user_msg_t           rmsg;
	xf_get_param_msg_t     *smsg;
	xf_get_param_msg_t     *param = (xf_get_param_msg_t *)p_param;
	u32 i;
	int ret = 0;

	p_handle = &p_comp->handle;

	/* ...set persistent stream characteristics */
	smsg = xf_buffer_data(p_handle->aux);

	for(i = 0; i < num_param; i++)
	{
		smsg[i].id = param[i].id;
	}

	/* ...pass command to the component */
	ret = xf_command(p_handle, 0, XF_GET_PARAM, smsg, num_param * sizeof(*smsg));
	if(ret)
	{
		TRACE("Error when passing cmd to component in get parameter function, err = %d\n", ret);
		return ret;
	}

	/* ...wait until result is delivered */
	ret = xf_response_get(p_handle, &rmsg);
	if(ret)
	{
		TRACE("response timeout when getting parameter, err = %d\n", ret);
		return ret;
	}

	/* ...make sure response is expected */
	if((rmsg.opcode != (u32) XF_GET_PARAM) || (rmsg.buffer != smsg))
	{
		TRACE("response is not expected when getting parameter, err = %d\n", -EPIPE);
		return -EPIPE;
	}

	for (i=0; i<num_param; i++)
	{
		param[i].value = smsg[i].value;
	}

	return 0;
}

int xaf_comp_create(xaf_adev_t *p_adev, xaf_comp_t *p_comp, int comp_type)
{
	xf_proxy_t *p_proxy;
	xf_handle_t *p_handle;
	xf_buffer_t *buf;
	char lib_path[200];
	char lib_wrap_path[200];
	int ret = 0;

	memset((void *)p_comp, 0, sizeof(xaf_comp_t));

	p_proxy = &p_adev->proxy;
	p_handle = &p_comp->handle;
	p_comp->comp_type = comp_type;

	/* ...init codec lib and codec wrap lib */
	strcpy(lib_path, CORE_LIB_PATH);
	strcpy(lib_wrap_path, CORE_LIB_PATH);
	p_comp->codec_lib.filename = lib_path;
	p_comp->codec_wrap_lib.filename = lib_wrap_path;

	p_comp->codec_lib.lib_type = HIFI_CODEC_LIB;
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
	}

	strcat(lib_wrap_path, "lib_dsp_codec_wrap.so");
	p_comp->codec_wrap_lib.lib_type = HIFI_CODEC_WRAP_LIB;

	/* ...create decoder component instance (select core-0) */
	ret = xf_open(p_proxy, p_handle, p_comp->dec_id, 0, my_comp_response);
	if(ret)
	{
		TRACE("create (%s) component failed, err = %d\n", p_comp->dec_id, ret);
		return ret;
	}

	/* ...load codec wrapper lib */
	ret = xf_load_lib(p_handle, &p_comp->codec_wrap_lib);
	if(ret)
	{
		TRACE("load codec wrapper lib failed, err = %d\n", ret);
		return ret;
	}

	/* ...load codec lib */
	ret = xf_load_lib(p_handle, &p_comp->codec_lib);
	if(ret)
	{
		TRACE("load codec lib failed, err = %d\n", ret);
		return ret;
	}

	/* ...allocate input buffer */
	ret = xf_pool_alloc(p_proxy, 1, INBUF_SIZE, XF_POOL_INPUT, &p_comp->inpool);
	if(ret)
	{
		TRACE("allocate component input buffer failed, err = %d\n", ret);
		return ret;
	}

	/* ...allocate output buffer */
	ret = xf_pool_alloc(p_proxy, 1, OUTBUF_SIZE, XF_POOL_OUTPUT, &p_comp->outpool);
	if(ret)
	{
		TRACE("allocate component output buffer failed, err = %d\n", ret);
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

int xaf_comp_delete(xaf_comp_t *p_comp)
{
	xf_handle_t *p_handle;
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

int xaf_comp_process(xaf_comp_t *p_comp, void *p_buf, u32 length, u32 flag)
{
	xf_handle_t *p_handle;
	u32 ret = 0;

	p_handle = &p_comp->handle;

	switch(flag)
	{
		case XF_FILL_THIS_BUFFER:
			/* ...issue asynchronous zero-length buffer to output port (port-id=1) */
			ret = xf_command(p_handle, 1, XF_FILL_THIS_BUFFER, p_buf, length);
			break;
		case XF_EMPTY_THIS_BUFFER:
			/* ...issue asynchronous zero-length buffer to input port (port-id=1) */
			ret = xf_command(p_handle, 0, XF_EMPTY_THIS_BUFFER, p_buf, length);
			break;
		default:
			break;
	}

	return ret;
}

int xaf_comp_get_status(xaf_comp_t *p_comp, xaf_info_t *p_info)
{
	xf_handle_t *p_handle;
	xf_user_msg_t msg;
	u32 ret = 0;

	p_handle = &p_comp->handle;

	TRACE("Waiting ...\n");
	/* ...wait until result is delivered */
	ret = xf_response_get_ack(p_handle, &msg);
	if(ret)
	{
		return ret;
	}

	p_info->opcode = msg.opcode;
	p_info->buf = msg.buffer;
	p_info->length = msg.length;
	p_info->ret = msg.ret;

	return ret;
}

int xaf_connect(xaf_comp_t *p_src, xaf_comp_t *p_dest, u32 num_buf, u32 buf_length)
{
	int ret = 0;

	/* ...connect p_src output port with p_dest input port */
	ret = xf_route(&p_src->handle, 0, &p_dest->handle, 0, num_buf, buf_length, 8);

	return ret;
}

int xaf_disconnect(xaf_comp_t *p_comp)
{
	int ret = 0;

	/* ...disconnect p_src output port with p_dest input port */
	ret = xf_unroute(&p_comp->handle, 0);

	return ret;
}

int xaf_comp_add(xaf_pipeline_t *p_pipe, xaf_comp_t *p_comp)
{
	int ret = 0;

	p_comp->next = p_pipe->comp_chain;
	p_comp->pipeline = p_pipe;
	p_pipe->comp_chain = p_comp;

    return ret;
}

int xaf_pipeline_create(xaf_adev_t *p_adev, xaf_pipeline_t *p_pipe)
{
	int ret = 0;

	memset(p_pipe, 0, sizeof(xaf_pipeline_t));

	return ret;
}

int xaf_pipeline_delete(xaf_pipeline_t *p_pipe)
{
	int ret = 0;

	memset(p_pipe, 0, sizeof(xaf_pipeline_t));

	return ret;
}

int xaf_pipeline_send_eos(xaf_pipeline_t *p_pipe)
{
	xaf_comp_t *p;
	xf_user_msg_t msg;
	int ret = 0;

	msg.id = 0;
	msg.opcode = XF_OUTPUT_EOS;
	msg.length = 0;
	msg.buffer = NULL;
	msg.ret = 0;

	for(p = p_pipe->comp_chain; p != NULL; p = p->next)
	{
		/* ...submit response to no-timely delivery queue */
		xf_response_put_ack(&p->handle, &msg);
	}

	return ret;
}
