/*******************************************************************************
 * Copyright 2018-2020 NXP
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
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

#include "debug.h"
#include "mydefs.h"
#include "dsp_codec_interface.h"
#include "xa-unia-codec.h"
#include "memory.h"
#include "dpu_lib_load.h"
/******************************************************************************
 * Internal functions definitions
 *****************************************************************************/

struct sCodecFun {
	UniACodecVersionInfo    VersionInfo;
	UniACodecCreate         Create;
	UniACodecDelete         Delete;
	UniACodecReset          Reset;
	UniACodecSetParameter   SetPara;
	UniACodecGetParameter   GetPara;
	UniACodec_decode_frame  Process;
	UniACodec_get_last_error    GetLastError;
};

/* ...API structure */
struct XFUniaCodec {
	UWORD32                 state;

	/* ...codec identifier */
	UWORD32 codec_id;

	/* ...codec input buffer pointer */
	u8 *inptr;

	/* ...codec output buffer pointer */
	u8 *outptr;

	/* ...codec input data size */
	UWORD32 in_size;

	/* ...codec output data size */
	UWORD32 out_size;

	/* ...codec consumed data size */
	UWORD32 consumed;

	/* ...codec input over flag */
	UWORD32 input_over;

	/* loading library info of codec wrap */
	struct dpu_lib_stat_t lib_codec_wrap_stat;

	/* loading library info of codec */
	struct dpu_lib_stat_t lib_codec_stat;

	/* ...codec API entry point (function) */
	tUniACodecQueryInterface codecwrapinterface;

	/* ...codec API handle, passed to *process */
	void *codecinterface;

	/* ...dsp codec wrapper handle */
	UniACodec_Handle pWrpHdl;

	/* ...dsp codec wrapper api */
	struct sCodecFun WrapFun;

	/* ...private data pointer */
	void *private_data;

	/* ...chan table buffer */
	void *chan_map_table;
};

/*******************************************************************************
 * Operating flags
 ******************************************************************************/
#define XA_DEC_FLAG_PREINIT_DONE   (1 << 0)
#define XA_DEC_FLAG_POSTINIT_DONE  (1 << 1)
#define XA_DEC_FLAG_IDLE           (1 << 2)
#define XA_DEC_FLAG_RUNNING        (1 << 3)
#define XA_DEC_FLAG_PAUSED         (1 << 4)
#define XA_DEC_FLAG_RUNTIME_INIT   (1 << 5)
/******************************************************************************
 * Commands processing
 *****************************************************************************/

static UA_ERROR_TYPE xf_uniacodec_get_api_size(struct XFUniaCodec *d,
						UWORD32 i_idx,
						void *pv_value)
{
	/* ...retrieve API structure size */
	*(UWORD32 *)pv_value = sizeof(*d);

	return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xf_uniacodec_preinit(struct XFUniaCodec *d,
					   UWORD32 i_idx,
					   void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	/* ...set codec id */
	d->codec_id = i_idx;

	/* ...set private data pointer */
	d->private_data = pv_value;

	return ret;
}

static UA_ERROR_TYPE xf_uniacodec_init_process(struct XFUniaCodec *d)
{
	UniACodecMemoryOps  memops;
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	if (!d->codecwrapinterface) {
		LOG("base->codecwrapinterface Pointer is NULL\n");
		return ACODEC_INIT_ERR;
	}

	d->codecwrapinterface(ACODEC_API_CREATE_CODEC, (void **)&d->WrapFun.Create);
	d->codecwrapinterface(ACODEC_API_DEC_FRAME, (void **)&d->WrapFun.Process);
	d->codecwrapinterface(ACODEC_API_RESET_CODEC, (void **)&d->WrapFun.Reset);
	d->codecwrapinterface(ACODEC_API_DELETE_CODEC, (void **)&d->WrapFun.Delete);
	d->codecwrapinterface(ACODEC_API_GET_PARAMETER, (void **)&d->WrapFun.GetPara);
	d->codecwrapinterface(ACODEC_API_SET_PARAMETER, (void **)&d->WrapFun.SetPara);
	d->codecwrapinterface(ACODEC_API_GET_LAST_ERROR, (void **)&d->WrapFun.GetLastError);

	memops.Malloc = (void *)xf_uniacodec_malloc;
	memops.Free = (void *)xf_uniacodec_free;

	if (!d->WrapFun.Create) {
		LOG("WrapFun.Create Pointer is NULL\n");
		return ACODEC_INIT_ERR;
	}

	d->pWrpHdl = d->WrapFun.Create(&memops);
	if (!d->pWrpHdl) {
		LOG("Create codec error in codec wrapper\n");
		return ACODEC_INIT_ERR;
	}

	return ret;
}
static UA_ERROR_TYPE xf_uniacodec_init(struct XFUniaCodec *d,
					UWORD32 i_idx,
					void *pv_value)
{
	/* ...sanity check - pcm gain component must be valid */
	XF_CHK_ERR(d, XA_API_FATAL_INVALID_CMD_TYPE);

	/* ...process particular initialization type */
	switch (i_idx)
	{
	case XA_CMD_TYPE_INIT_API_PRE_CONFIG_PARAMS:
	{
		d->consumed = 0;
		d->chan_map_table = NULL;
		/* ...and mark pcm gain component has been created */
		d->state = XA_DEC_FLAG_PREINIT_DONE;

		return XA_NO_ERROR;
	}
	case XA_CMD_TYPE_INIT_API_POST_CONFIG_PARAMS:
	{
		/* ...post-configuration initialization (all parameters are set) */
		XF_CHK_ERR(d->state & XA_DEC_FLAG_PREINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

		/* ...mark post-initialization is complete */
		d->state |= XA_DEC_FLAG_POSTINIT_DONE;

		return XA_NO_ERROR;
	}

	case XA_CMD_TYPE_INIT_PROCESS:
	{
		/* ...kick run-time initialization process; make sure pcm gain component is setup */
		//XF_CHK_ERR(d->state & XA_DEC_FLAG_POSTINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

//		xf_uniacodec_init_process(d, i_idx, pv_value);
		/* ...enter into execution stage */
		d->state |= XA_DEC_FLAG_RUNNING;

		return XA_NO_ERROR;
	}

	case XA_CMD_TYPE_INIT_DONE_QUERY:
	{
		/* ...check if initialization is done; make sure pointer is sane */
		XF_CHK_ERR(pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

		/* ...put current status */
		*(WORD32 *)pv_value = (d->state & XA_DEC_FLAG_RUNNING ? 1 : 0);

		return XA_NO_ERROR;
	}

	default:
		/* ...unrecognised command type */
		TRACE(ERROR, _x("Unrecognised command type: %X"), i_idx);
		return XA_API_FATAL_INVALID_CMD_TYPE;
	}
}

static UA_ERROR_TYPE xf_uniacodec_postinit(struct XFUniaCodec *d,
					    UWORD32 i_idx,
					    void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	return ret;
}

static UA_ERROR_TYPE xf_unia_load_lib(struct XFUniaCodec *d, void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;
        UWORD32 core = 0;
	struct icm_xtlib_pil_info  *cmd;
	struct dpu_lib_stat_t *lib_stat;
	void *lib_interface;
	UniACodecParameter parameter;
	int lib_type;

	cmd = (struct icm_xtlib_pil_info *)xf_ipc_a2b(core, *(UWORD32 *)pv_value);

	cmd->pil_info.dst_addr      = xf_ipc_a2b(core, (UWORD32)cmd->pil_info.dst_addr);
	cmd->pil_info.dst_data_addr = xf_ipc_a2b(core, (UWORD32)cmd->pil_info.dst_data_addr);
	cmd->pil_info.start_sym     = xf_ipc_a2b(core, (UWORD32)cmd->pil_info.start_sym);
	cmd->pil_info.text_addr     = xf_ipc_a2b(core, (UWORD32)cmd->pil_info.text_addr);
	cmd->pil_info.init          = xf_ipc_a2b(core, (UWORD32)cmd->pil_info.init);
	cmd->pil_info.fini          = xf_ipc_a2b(core, (UWORD32)cmd->pil_info.fini);
	cmd->pil_info.rel           = xf_ipc_a2b(core, (UWORD32)cmd->pil_info.rel);
	cmd->pil_info.hash          = xf_ipc_a2b(core, (UWORD32)cmd->pil_info.hash);
	cmd->pil_info.symtab        = xf_ipc_a2b(core, (UWORD32)cmd->pil_info.symtab);
	cmd->pil_info.strtab        = xf_ipc_a2b(core, (UWORD32)cmd->pil_info.strtab);

	lib_type = GET_LIB_TYPE(cmd->lib_type);
	switch(lib_type) {
	case DSP_CODEC_LIB:
		lib_stat = &d->lib_codec_stat;
		d->codecinterface = dpu_process_init_pi_lib(&cmd->pil_info, lib_stat, 0);
		if (!d->codecinterface) {
			LOG("load codec lib failed\n");
			return XA_API_FATAL_INVALID_CMD;
		}
		LOG1("load codec lib: %x\n", d->codecinterface);
		break;
	case DSP_CODEC_WRAP_LIB:
		lib_stat = &d->lib_codec_wrap_stat;
		d->codecwrapinterface = (tUniACodecQueryInterface)dpu_process_init_pi_lib(&cmd->pil_info, lib_stat, 0);
		if (!d->codecwrapinterface) {
			LOG("load codec wrap lib failed\n");
			return XA_API_FATAL_INVALID_CMD;
		}
		LOG1("load codec wrap lib: %x\n", d->codecwrapinterface);
		break;
	default:
		return XA_API_FATAL_INVALID_CMD;
	}

	/* codec wrap is last loadable lib */
	if (lib_type == DSP_CODEC_WRAP_LIB) {
		ret = xf_uniacodec_init_process(d);
		if (ret)
			return XA_API_FATAL_INVALID_CMD;
		if (d->codecinterface && d->WrapFun.SetPara) {
			d->codec_id = GET_CODEC_TYPE(cmd->lib_type);
			parameter.codec_id = d->codec_id;
			ret = d->WrapFun.SetPara(d->pWrpHdl, UNIA_CODEC_ID, &parameter);
			parameter.codec_entry_addr = (UWORD32)d->codecinterface;
			ret = d->WrapFun.SetPara(d->pWrpHdl, UNIA_CODEC_ENTRY_ADDR, &parameter);
#ifdef DEBUG
			ret = d->WrapFun.SetPara(d->pWrpHdl, UNIA_FUNC_PRINT, NULL);
#endif
		}
	}

	return XA_NO_ERROR;
}

static UA_ERROR_TYPE xf_unia_unload_lib(struct XFUniaCodec *d, void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;
	struct icm_xtlib_pil_info  *cmd;
	struct dpu_lib_stat_t *lib_stat;
	int do_cleanup = 0;
	int lib_type;

	cmd = (struct icm_xtlib_pil_info *)xf_ipc_a2b(0, *(UWORD32 *)pv_value);
	lib_type = GET_LIB_TYPE(cmd->lib_type);
	LOG1("unload lib, type %d\n", lib_type);
	switch(lib_type) {
	case DSP_CODEC_LIB:
		lib_stat = &d->lib_codec_stat;
		break;
	case DSP_CODEC_WRAP_LIB:
		lib_stat = &d->lib_codec_wrap_stat;
		do_cleanup = 1;
		break;
	default:
		/* ... just don't care about stray unloads */
		lib_stat = NULL;
	}

	if (lib_stat && lib_stat->stat == lib_loaded) {
		/* ...destory loadable lib resources */
		if (do_cleanup && d->WrapFun.Delete)
			ret = d->WrapFun.Delete(d->pWrpHdl);

		dpu_process_unload_pi_lib(lib_stat);
	}

	return XA_NO_ERROR;
}

static UA_ERROR_TYPE xf_uniacodec_setparam(struct XFUniaCodec *d,
					    UWORD32 i_idx,
					    void *pv_value)
{
	UniACodecParameter parameter;
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	switch (i_idx) {
	case UNIA_LOAD_LIB:
		return xf_unia_load_lib(d, pv_value);
	case UNIA_UNLOAD_LIB:
		return xf_unia_unload_lib(d, pv_value);
	case UNIA_RESET_BUF:
		if (d->WrapFun.Reset)
			ret = d->WrapFun.Reset(d->pWrpHdl);
		return ret;
	case UNIA_SAMPLERATE:
		parameter.samplerate = *(UWORD32 *)pv_value;
		break;
	case UNIA_CHANNEL:
		parameter.channels = *(UWORD32 *)pv_value;
		break;
	case UNIA_DEPTH:
		parameter.depth = *(UWORD32 *)pv_value;
		break;
	case UNIA_DOWNMIX_STEREO:
		parameter.downmix = *(UWORD32 *)pv_value;
		break;
	case UNIA_STREAM_TYPE:
		parameter.stream_type = *(UWORD32 *)pv_value;
		break;
	case UNIA_BITRATE:
		parameter.bitrate = *(UWORD32 *)pv_value;
		break;
	case UNIA_TO_STEREO:
		parameter.mono_to_stereo = *(UWORD32 *)pv_value;
		break;
	case UNIA_FRAMED:
		parameter.framed = *(UWORD32 *)pv_value;
		break;
	case UNIA_CODEC_DATA:
		parameter.codecData.buf = NULL;
		parameter.codecData.size = *(UWORD32 *)pv_value;
		break;
	/*****dedicate for mp3 dec and mp2 dec*****/
	case UNIA_MP3_DEC_CRC_CHECK:
		parameter.crc_check = *(UWORD32 *)pv_value;
		break;
	case UNIA_MP3_DEC_MCH_ENABLE:
		parameter.mch_enable = *(UWORD32 *)pv_value;
		break;
	case UNIA_MP3_DEC_NONSTD_STRM_SUPPORT:
		parameter.nonstd_strm_support = *(UWORD32 *)pv_value;
		break;
	/*****dedicate for bsac dec***************/
	case UNIA_BSAC_DEC_DECODELAYERS:
		parameter.layers = *(UWORD32 *)pv_value;
		break;
	/*****dedicate for aacplus dec***********/
	case UNIA_AACPLUS_DEC_BDOWNSAMPLE:
		parameter.bdownsample = *(UWORD32 *)pv_value;
		break;
	case UNIA_AACPLUS_DEC_BBITSTREAMDOWNMIX:
		parameter.bbitstreamdownmix = *(UWORD32 *)pv_value;
		break;
	case UNIA_AACPLUS_DEC_CHANROUTING:
		parameter.chanrouting = *(UWORD32 *)pv_value;
		break;
	/*****************dedicate for dabplus dec******************/
	case UNIA_DABPLUS_DEC_BDOWNSAMPLE:
		parameter.bdownsample = *(UWORD32 *)pv_value;
		break;
	case UNIA_DABPLUS_DEC_BBITSTREAMDOWNMIX:
		parameter.bbitstreamdownmix = *(UWORD32 *)pv_value;
		break;
	case UNIA_DABPLUS_DEC_CHANROUTING:
		parameter.chanrouting = *(UWORD32 *)pv_value;
		break;
	/*******************dedicate for sbc enc******************/
	case UNIA_SBC_ENC_SUBBANDS:
		parameter.enc_subbands = *(UWORD32 *)pv_value;
		break;
	case UNIA_SBC_ENC_BLOCKS:
		parameter.enc_blocks = *(UWORD32 *)pv_value;
		break;
	case UNIA_SBC_ENC_SNR:
		parameter.enc_snr = *(UWORD32 *)pv_value;
		break;
	case UNIA_SBC_ENC_BITPOOL:
		parameter.enc_bitpool = *(UWORD32 *)pv_value;
		break;
	case UNIA_SBC_ENC_CHMODE:
		parameter.enc_chmode = *(UWORD32 *)pv_value;
		break;
	/*******************dedicate for wma dec******************/
	case UNIA_WMA_BlOCKALIGN:
		parameter.blockalign = *(UWORD32 *)pv_value;
		break;
	case UNIA_WMA_VERSION:
		parameter.version = *(UWORD32 *)pv_value;
		break;
	case UNIA_CHAN_MAP_TABLE:
	{
		int i;
		void *p_buf;
		void *para_buf = xf_ipc_a2b(0, *(UWORD32 *)pv_value);
		CHAN_TABLE *chan_map_tab;

		if (!d->chan_map_table)
			ret = xaf_malloc(&d->chan_map_table, 256, 0);
		if (ret)
			return ACODEC_PARA_ERROR;
		memset(d->chan_map_table, 0, 256);
		memcpy(d->chan_map_table, para_buf, 256);

		chan_map_tab = (CHAN_TABLE *)d->chan_map_table;
		LOG1("chan map %x\n", chan_map_tab->channel_table[3]);
		LOG1("chan map %x\n", chan_map_tab->channel_table[5]);
		LOG1("chan map %x\n", chan_map_tab->channel_table[7]);
		LOG1("chan map %x\n", chan_map_tab->channel_table[9]);
		LOG1("chan map %x\n", chan_map_tab->channel_table[11]);
		LOG1("chan map %x\n", chan_map_tab->channel_table[13]);
		/* cause sizeof(int) == 4 in dsp side */
		p_buf = (void *)(chan_map_tab + 2);
		for (i = 1; i < 10; i++) {
			if (chan_map_tab->channel_table[i * 2 + 1]) {
				chan_map_tab->channel_table[i] = p_buf;
				p_buf += i * 2;
			}
		}
		LOG1("chan map %x\n", chan_map_tab->channel_table[1]);
		LOG1("chan map %x\n", chan_map_tab->channel_table[2]);
		LOG1("chan map %x\n", chan_map_tab->channel_table[3]);
		LOG1("chan map %x\n", chan_map_tab->channel_table[4]);
		LOG1("chan map %x\n", chan_map_tab->channel_table[5]);
		parameter.chan_map_tab = *chan_map_tab;
		break;
	}
	default:
		return ret;
	}

	if (!d->WrapFun.SetPara) {
		LOG("WrapFun.SetPara Pointer is NULL\n");
		return ACODEC_INIT_ERR;
	}
	ret = d->WrapFun.SetPara(d->pWrpHdl, i_idx, &parameter);

	return ret;
}

static UA_ERROR_TYPE xf_uniacodec_getparam(struct XFUniaCodec *d,
					    UWORD32 i_idx,
					    void *pv_value)
{
	UniACodecParameter param;
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	if (!d->WrapFun.GetPara) {
		LOG("WrapFun.GetPara Pointer is NULL\n");
		return ACODEC_INIT_ERR;
	}
	switch (i_idx) {
	case XA_CODEC_CONFIG_PARAM_SAMPLE_RATE:
	case UNIA_SAMPLERATE:
		ret = d->WrapFun.GetPara(d->pWrpHdl, UNIA_SAMPLERATE, &param);
		if (ret)
			return ACODEC_PARA_ERROR;

		if (!param.samplerate)
			param.samplerate = 44100;
		*(UWORD32 *)pv_value = param.samplerate;
		break;
	case XA_CODEC_CONFIG_PARAM_CHANNELS:
	case UNIA_CHANNEL:
		ret = d->WrapFun.GetPara(d->pWrpHdl, UNIA_CHANNEL, &param);
		if (ret)
			return ACODEC_PARA_ERROR;

		if (!param.channels)
			param.channels = 2;
		*(UWORD32 *)pv_value = param.channels;
		break;
	case XA_CODEC_CONFIG_PARAM_PCM_WIDTH:
	case UNIA_DEPTH:
		ret = d->WrapFun.GetPara(d->pWrpHdl, UNIA_DEPTH, &param);
		if (ret)
			return ACODEC_PARA_ERROR;

		if (!param.depth)
			param.depth = 16;
		*(UWORD32 *)pv_value = param.depth;
		break;

	case XA_CODEC_CONFIG_PARAM_PRODUCED:
		*(UWORD32 *)pv_value = d->out_size;
		break;
	default:
		*(UWORD32 *)pv_value = 0;
		break;
	}

	return ret;
}

static UA_ERROR_TYPE xf_uniacodec_exec_process(struct XFUniaCodec *d,
					   UWORD32 i_idx,
					   void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;
	UWORD32 offset = 0;

	if (!d->WrapFun.Process) {
		LOG("WrapFun.Process Pointer is NULL\n");
		return ACODEC_INIT_ERR;
	}

	LOG4("in_buf = %x, in_size = %x, offset = %d, out_buf = %x\n",
	     d->inptr, d->in_size, d->consumed, d->outptr);

	/* pass to ua wrapper input is over */
	if (d->input_over && d->in_size == 0 && d->codec_id >= CODEC_FSL_OGG_DEC)
		d->inptr = NULL;

	ret = d->WrapFun.Process(d->pWrpHdl,
				d->inptr,
				d->in_size,
				&offset,
				(u8 **)&d->outptr,
				&d->out_size);

	d->consumed = offset;
	/* set ret fatal error for avoiding dead loop in dsp */
	if (!d->consumed && ret && ret <= ACODEC_INIT_ERR)
		ret |= XA_FATAL_ERROR;

	LOG3("process: consumed = %d, produced = %d, ret = %d\n",
	     d->consumed, d->out_size, ret);

	return ret;
}

static UA_ERROR_TYPE xf_uniacodec_runtime_init(struct XFUniaCodec *d,
						UWORD32 i_idx,
						void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	if (d->WrapFun.Reset)
		ret = d->WrapFun.Reset(d->pWrpHdl);

	d->inptr = NULL;
	d->input_over = 0;
	d->in_size = 0;
	d->consumed = 0;

	return ret;
}

static UA_ERROR_TYPE xf_uniacodec_execute(struct XFUniaCodec *d,
					   UWORD32 i_idx,
					   void *pv_value)
{
	switch (i_idx) {
	case XA_CMD_TYPE_DO_EXECUTE:
		return xf_uniacodec_exec_process(d, i_idx, pv_value);

	case XA_CMD_TYPE_DONE_QUERY:
		/* ...check if processing is complete */
		XF_CHK_ERR(pv_value, XA_API_FATAL_INVALID_CMD_TYPE);
		/* decoder is done */
		if (d->input_over && d->in_size == 0 && d->out_size == 0) {
			*(WORD32 *)pv_value = 1;
		} else
			*(WORD32 *)pv_value = 0;

		return XA_NO_ERROR;

	case XA_CMD_TYPE_DO_RUNTIME_INIT:
		return xf_uniacodec_runtime_init(d, i_idx, pv_value);

	default:
		/* ...unrecognised command */
		TRACE(ERROR, _x("Invalid index: %X"), i_idx);
		return XA_API_FATAL_INVALID_CMD_TYPE;
	}
}

static UA_ERROR_TYPE xf_uniacodec_set_input_ptr(struct XFUniaCodec *d,
						 UWORD32 i_idx,
						 void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	d->inptr = (u8 *)pv_value;

	return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xf_uniacodec_set_output_ptr(struct XFUniaCodec *d,
						  UWORD32 i_idx,
						  void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	d->outptr = (u8 *)pv_value;

	return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xf_uniacodec_set_input_bytes(struct XFUniaCodec *d,
						   UWORD32 i_idx,
						   void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	d->in_size = *(UWORD32 *)pv_value;

	return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xf_uniacodec_get_output_bytes(struct XFUniaCodec *d,
						    UWORD32 i_idx,
						    void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	*(UWORD32 *)pv_value = d->out_size;

	return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xf_uniacodec_get_consumed_bytes(struct XFUniaCodec *d,
						      UWORD32 i_idx,
						      void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	*(UWORD32 *)pv_value = d->consumed;

	return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xf_uniacodec_input_over(struct XFUniaCodec *d,
					      UWORD32 i_idx,
					      void *pv_value)
{
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	d->input_over = 1;
	xf_uniacodec_setparam(d, UNIA_INPUT_OVER, NULL);

	return ACODEC_SUCCESS;
}

static UA_ERROR_TYPE xa_uniacodec_get_memtabs_size(struct XFUniaCodec *d, UWORD32 i_idx, void* pv_value)
{
	/* ...basic sanity checks */
	XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

	/* ...check pcm gain component is pre-initialized */
	XF_CHK_ERR(d->state & XA_DEC_FLAG_PREINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

	/* ...we have all our tables inside API structure - good? tbd */
	*(WORD32 *)pv_value = 0;

	return XA_NO_ERROR;
}

static UA_ERROR_TYPE xa_uniacodec_set_memtabs_ptr(struct XFUniaCodec *d, UWORD32 i_idx, void* pv_value)
{
	return XA_NO_ERROR;
}

static UA_ERROR_TYPE xa_uniacodec_get_n_memtabs(struct XFUniaCodec *d, UWORD32 i_idx, pVOID pv_value)
{
	/* ...basic sanity checks */
	XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

	/* ...we have 1 input buffer, 1 output buffer */
	*(WORD32 *)pv_value = 2;

	return XA_NO_ERROR;
}

static UA_ERROR_TYPE xa_uniacodec_get_mem_info_size(struct XFUniaCodec *d, UWORD32 i_idx, pVOID pv_value)
{
	 UWORD32     i_value;
	/* ...basic sanity check */
	XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

	/* ...return frame buffer minimal size only after post-initialization is done */
	XF_CHK_ERR(d->state & XA_DEC_FLAG_POSTINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

	switch (i_idx)
	{
	case 0:
		/* ...input buffer specification; accept exact audio frame */
		/* set input length 0 for bypass mode */
		i_value = 0;
		break;

	case 1:
		/* ...output buffer specification; accept exact audio frame */
		i_value = 16384;
		break;

	default:
		/* ...invalid index */
		return XF_CHK_ERR(0, XA_API_FATAL_INVALID_CMD_TYPE);
	}

	/* ...return buffer size to caller */
	*(WORD32 *)pv_value = (WORD32) i_value;

	return XA_NO_ERROR;
}

static UA_ERROR_TYPE xa_uniacodec_get_mem_info_alignment(struct XFUniaCodec *d, UWORD32 i_idx, pVOID pv_value)
{
	/* ...basic sanity check */
	XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

	/* ...return frame buffer minimal size only after post-initialization is done */
	XF_CHK_ERR(d->state & XA_DEC_FLAG_POSTINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

	/* ...all buffers are 4-bytes aligned */
	*(WORD32 *)pv_value = 4;

	return XA_NO_ERROR;
}

static UA_ERROR_TYPE xa_uniacodec_get_mem_info_type(struct XFUniaCodec *d, UWORD32 i_idx, pVOID pv_value)
{
	/* ...basic sanity check */
	XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

	/* ...return frame buffer minimal size only after post-initialization is done */
	XF_CHK_ERR(d->state & XA_DEC_FLAG_POSTINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

	switch (i_idx)
	{
	case 0:
		/* ...input buffer */
		*(WORD32 *)pv_value = XA_MEMTYPE_INPUT;
		return XA_NO_ERROR;

	case 1:
		/* ...output buffer */
		*(WORD32 *)pv_value = XA_MEMTYPE_OUTPUT;
		return XA_NO_ERROR;

	default:
		/* ...invalid index */
		return XF_CHK_ERR(0, XA_API_FATAL_INVALID_CMD_TYPE);
	}
}
static UA_ERROR_TYPE xa_uniacodec_set_mem_ptr(struct XFUniaCodec *d, UWORD32 i_idx, pVOID pv_value)
{
	/* ...basic sanity check */
	XF_CHK_ERR(d && pv_value, XA_API_FATAL_INVALID_CMD_TYPE);

	/* ...codec must be initialized */
	XF_CHK_ERR(d->state & XA_DEC_FLAG_POSTINIT_DONE, XA_API_FATAL_INVALID_CMD_TYPE);

	/* ...select memory buffer */
	switch (i_idx)
	{
	case 0:
		/* ...input buffers */
		d->inptr = pv_value;
		return XA_NO_ERROR;

	case 1:
		/* ...output buffer */
		d->outptr = pv_value;
		return XA_NO_ERROR;

	default:
		/* ...invalid index */
		return XF_CHK_ERR(0, XA_API_FATAL_INVALID_CMD_TYPE);
	}
}

/*******************************************************************************
 * API command hooks
 ******************************************************************************/

static UA_ERROR_TYPE (* const xf_unia_codec_api[])(struct XFUniaCodec *, UWORD32, void *) = {
	[XA_API_CMD_GET_API_SIZE]             = xf_uniacodec_get_api_size,
	[XA_API_CMD_INIT]                     = xf_uniacodec_init,
	[XA_API_CMD_SET_CONFIG_PARAM]         = xf_uniacodec_setparam,
	[XA_API_CMD_GET_CONFIG_PARAM]         = xf_uniacodec_getparam,
	[XA_API_CMD_EXECUTE]                  = xf_uniacodec_execute,
	[XA_API_CMD_SET_INPUT_BYTES]          = xf_uniacodec_set_input_bytes,
	[XA_API_CMD_GET_OUTPUT_BYTES]         = xf_uniacodec_get_output_bytes,
	[XA_API_CMD_GET_CURIDX_INPUT_BUF]     = xf_uniacodec_get_consumed_bytes,
	[XA_API_CMD_INPUT_OVER]               = xf_uniacodec_input_over,

	[XA_API_CMD_GET_MEMTABS_SIZE]         = xa_uniacodec_get_memtabs_size,
	[XA_API_CMD_SET_MEMTABS_PTR]          = xa_uniacodec_set_memtabs_ptr,
	[XA_API_CMD_GET_N_MEMTABS]            = xa_uniacodec_get_n_memtabs,
	[XA_API_CMD_GET_MEM_INFO_SIZE]        = xa_uniacodec_get_mem_info_size,
	[XA_API_CMD_GET_MEM_INFO_ALIGNMENT]   = xa_uniacodec_get_mem_info_alignment,
	[XA_API_CMD_GET_MEM_INFO_TYPE]        = xa_uniacodec_get_mem_info_type,
	[XA_API_CMD_SET_MEM_PTR]              = xa_uniacodec_set_mem_ptr,
};

/* ...total numer of commands supported */
#define XF_UNIACODEC_API_COMMANDS_NUM        \
	(sizeof(xf_unia_codec_api) / sizeof(xf_unia_codec_api[0]))

/*****************************************************************************
 * API entry point
 ****************************************************************************/

UWORD32 xa_unia_codec(xa_codec_handle_t handle,
		  UWORD32 i_cmd,
		  UWORD32 i_idx,
		  void *pv_value)
{
	struct XFUniaCodec *uniacodec = (struct XFUniaCodec *)handle;

	/* ...check if command index is sane */
	XF_CHK_ERR(i_cmd < XF_UNIACODEC_API_COMMANDS_NUM, ACODEC_PARA_ERROR);

	/* ...see if command is defined */
	XF_CHK_ERR(xf_unia_codec_api[i_cmd] != NULL, ACODEC_PARA_ERROR);

	/* ...execute requested command */
	return xf_unia_codec_api[i_cmd](uniacodec, i_idx, pv_value);
}
