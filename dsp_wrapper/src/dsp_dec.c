/*****************************************************************
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
 *****************************************************************/
#include <string.h>
#include <stdbool.h>
#include "xf-types.h"
#include "dsp_wrap.h"

/*
 * InputBufHandle - Handle the input buffer and the inner buffer data
 *
 * @inner_buf  : inner buf, offset and hold data size
 * @InputBuf   : input buffer
 * @InputSize  : total input size
 * @offset     : current input buffer offset of start position
 * @framed     : flag to indicate if the the buffer threshold will be checked
 * Returns:
 *         ACODEC_NOT_ENOUGH_DATA: if data less than threshold
 *         ACODEC_SUCCESS: buffer is handled successfully
 */
UA_ERROR_TYPE  InputBufHandle(struct innerBuf *inner_buf,
			      UWORD8 *InputBuf,
			      UWORD32 InputSize,
			      UWORD32 *offset,
			      bool framed) {
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	UWORD8 *inner_data;
	UWORD32 *inner_offset;
	UWORD32 *inner_size;
	UWORD32 bufsize;
	UWORD32 inbuf_remain, innerbuf_avail;
	UWORD32 threshold;

	if (!inner_buf)
		return ACODEC_ERR_UNKNOWN;

	inner_data = inner_buf->data;
	inner_offset = &inner_buf->inner_offset;
	inner_size = &inner_buf->inner_size;
	bufsize = inner_buf->buf_size;
	threshold = inner_buf->threshold;

	inbuf_remain = InputSize - (*offset);
	innerbuf_avail = threshold - (*inner_size);

	if (!InputBuf)
		return ret;

	if (!framed) {
		if (((*inner_size) + inbuf_remain) < threshold)
			ret = ACODEC_NOT_ENOUGH_DATA;
	}

	if ((*inner_offset != 0) && (bufsize - (*inner_offset)) < threshold) {
		memmove(inner_data, inner_data + (*inner_offset), *inner_size);
		*inner_offset = 0;
	}

	if (inbuf_remain > innerbuf_avail) {
		memcpy((inner_data + (*inner_offset) + (*inner_size)),
		       InputBuf + (*offset), innerbuf_avail);
		*inner_size = threshold;
		*offset += innerbuf_avail;
	} else {
		memcpy((inner_data + (*inner_offset) + (*inner_size)),
		       InputBuf + (*offset), inbuf_remain);
		*inner_size += inbuf_remain;
		*offset += inbuf_remain;
	}

	return ret;
}

/*
 * ResetInnerBuf - Reset the inner buffer data
 *
 * @inner_buf : inner buffer
 * @buf_size  : inner buffer size
 * @threshold : threshold of data size in inner buffer
 * Returns:
 *          ACODEC_SUCCESS: reset inner buffer successfully
 *          ACODEC_ERR_UNKNOWN: Fail to reset inner buffer
 */
UA_ERROR_TYPE ResetInnerBuf(struct innerBuf *inner_buf,
			    UWORD32 buf_size,
			    UWORD32 threshold) {
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	inner_buf->inner_offset = 0;
	inner_buf->inner_size = 0;
	inner_buf->buf_size = buf_size;
	inner_buf->threshold = threshold;

	if (inner_buf->data)
		memset(inner_buf->data, 0, inner_buf->buf_size);
	else
		ret = ACODEC_ERR_UNKNOWN;

	return ret;
}

/*
 * SetDefaultFeature - Set default feature of DSP codec
 *
 * @pua_handle: decoder or encoder handle
 * Returns:
 *         XA_NO_ERROR on success
 *         other error code on failure
 */
UA_ERROR_TYPE SetDefaultFeature(UniACodec_Handle pua_handle)
{
	struct DSP_Handle *pDSP_handle = (struct DSP_Handle *)pua_handle;
	UniACodecParameter parameter;
	int err = ACODEC_SUCCESS;

	switch (pDSP_handle->codec_type) {
	case MP2:
		/* disable header CRC check default */
		parameter.crc_check = 0;
		err = DSPDecSetPara(pua_handle,
				    UNIA_MP3_DEC_CRC_CHECK,
				    &parameter);
		break;
	case MP3:
		/* disable header CRC check default */
		parameter.crc_check = 0;
		err = DSPDecSetPara(pua_handle,
				    UNIA_MP3_DEC_CRC_CHECK,
				    &parameter);
		/* enable non-standard stream format support default */
		parameter.nonstd_strm_support = 1;
		err = DSPDecSetPara(pua_handle,
				    UNIA_MP3_DEC_NONSTD_STRM_SUPPORT,
				    &parameter);
		break;
	case AAC:
	case AAC_PLUS:
		/* disable force mono to stereo output */
		/*parameter.mono_to_stereo = 0;
		err = DSPDecSetPara(pua_handle,
				    UNIA_TO_STEREO,
				    &parameter);*/
		break;
	case DAB_PLUS:
		/* disable force mono to stereo output */
		parameter.mono_to_stereo = 0;
		err = DSPDecSetPara(pua_handle,
				    UNIA_TO_STEREO,
				    &parameter);
		break;
	case BSAC:
		/* disable force mono to stereo output */
		/*parameter.mono_to_stereo = 0;
		err = DSPDecSetPara(pua_handle,
				    UNIA_TO_STEREO,
				    &parameter);*/
		break;
	case DRM:
		break;
	case SBCDEC:
		break;
	case SBCENC:
		break;
	default:
		break;
	}

	if (err)
		return err;

	if (!(pDSP_handle->depth_is_set)) {
		parameter.depth = 16;
		err = DSPDecSetPara(pua_handle,
				    UNIA_DEPTH,
				    &parameter);
	}

	return err;
}

/* cancel_unused_channel_data - transfer 2 channel output to 1 channel output
 *    when disable mono_to_stereo mode for aacplus, bsac and dabplus
 *
 * @data_in: data in buffer pointer
 * @length: byte number in data in buffer
 * @depth: width of sample
 */
void cancel_unused_channel_data(UWORD8 *data_in, WORD32 length, WORD32 depth)
{
	UWORD32 i;
	UWORD8 *ptr1, *ptr2;

	if (depth == 16) {
		ptr1 = data_in + 2;
		ptr2 = data_in + 4;

		for (i = 1; i < length / 4; i++) {
			*ptr1++ = *ptr2;
			*ptr1++ = *(ptr2 + 1);

			ptr2 += 4;
		}
	} else if (depth == 24) {
		ptr1 = data_in + 3;
		ptr2 = data_in + 6;

		for (i = 1; i < length / 6; i++) {
			*ptr1++ = *ptr2;
			*ptr1++ = *(ptr2 + 1);
			*ptr1++ = *(ptr2 + 2);

			ptr2 += 6;
		}
	}
}

int comp_process(UniACodec_Handle pua_handle,
		 UWORD8 *input,
		 UWORD32 in_size,
		 UWORD32 *in_off,
		 UWORD8 *output,
		 UWORD32 *out_size)
{
	struct DSP_Handle *pDSP_handle = (struct DSP_Handle *)pua_handle;
	xaf_adev_t *p_adev = pDSP_handle->p_adev;
	xaf_comp_t *p_decoder = pDSP_handle->p_comp;
	int error = 0, count;
	xaf_comp_status *comp_status = &pDSP_handle->comp_status;
	long comp_info[4];
	long *p_buf = NULL;
	long size = 0;

	if (!input || !output)
		return ACODEC_PARA_ERROR;

	if (!pDSP_handle->outptr_busy) {
		if (*comp_status == XAF_STARTING) {
			error = xaf_comp_process(p_adev, p_decoder, NULL, 0, XAF_START_FLAG);
		} else if (*comp_status == XAF_INIT_DONE) {
			error = xaf_comp_process(NULL, p_decoder, NULL, 0, XAF_EXEC_FLAG);
		} else if (*comp_status == XAF_OUTPUT_READY) {
			error = xaf_comp_process(NULL, p_decoder, p_decoder->pout_buf[0], OUTBUF_SIZE, XAF_NEED_OUTPUT_FLAG);
		}
		if (error) {
			fprintf(stderr, "outbuf error: %d\n", error);
			return ACODEC_ERROR_STREAM;
		}
		pDSP_handle->outptr_busy = true;
	}

	if (!pDSP_handle->inptr_busy && !pDSP_handle->input_over) {
		if (in_size) {
			memcpy(p_decoder->p_input[0], input, in_size);
			*in_off = in_size;
			/* only use input buf0 */
			error = xaf_comp_process(p_adev, p_decoder, p_decoder->p_input[0], in_size, XAF_INPUT_READY_FLAG);
		} else {
			pDSP_handle->input_over = 1;
			error = xaf_comp_process(p_adev, p_decoder, NULL, 0, XAF_INPUT_OVER_FLAG);
		}
		if (error) {
			fprintf(stderr, "inbuf error: %d\n", error);
			return ACODEC_ERROR_STREAM;
		}
		pDSP_handle->inptr_busy = true;
	}

	/* ...wait until result is delivered */
	error = xaf_comp_get_status(p_adev, p_decoder, comp_status, &comp_info[0]);
	if (error < XAF_NO_ERR) {
		fprintf(stderr, "xaf_comp_get_status err %d\n", error);
		return ACODEC_ERROR_STREAM;
	} else if (error > XAF_NO_ERR) {
		/* Get event from dsp */
		int *config_buf = (long *)comp_info[0];
		int decode_err = *config_buf;
		free(config_buf);
		return ACODEC_ERROR_STREAM;
	}

	switch (*comp_status) {
	case XAF_PROBE_READY:
	case XAF_PROBE_DONE:
		/* the status not used */
		fprintf(stderr, "xaf_comp_get_status get wrong status\n");
		return ACODEC_ERROR_STREAM;
	case XAF_EXEC_DONE:
#ifdef DEBUG
		fprintf(stdout, "xaf_comp_get_status exec done\n");
#endif
		return ACODEC_END_OF_STREAM;
	case XAF_INIT_DONE:
		p_buf = (long *)comp_info[0];
		size = comp_info[1];
		*out_size = 0;
		pDSP_handle->outptr_busy = false;
#ifdef DEBUG
		fprintf(stdout, "xaf_comp_get_status comp init done\n");
#endif
		/* init done not start to decode then no output */
		return ACODEC_NO_OUTPUT;
	case XAF_NEED_INPUT:
		p_buf = (long *)comp_info[0];
		size = comp_info[1];
		pDSP_handle->inptr_busy = false;
#ifdef DEBUG
		fprintf(stdout, "xaf need input\n");
#endif
		return ACODEC_NO_OUTPUT;
	case XAF_OUTPUT_READY:
		p_buf = (long *)comp_info[0];
		size = comp_info[1];
		if (p_buf && size) {
			memcpy(output, p_buf, size);
			*out_size = size;
		}
		pDSP_handle->outptr_busy = false;
#ifdef DEBUG
		fprintf(stdout, "xaf output ready\n");
#endif
		return ACODEC_SUCCESS;
		break;
	default:
		return ACODEC_ERR_UNKNOWN;
	}
}

int comp_flush_msg(UniACodec_Handle pua_handle)
{
	int error;
	struct DSP_Handle *pDSP_handle = (struct DSP_Handle *)pua_handle;
	xaf_adev_t *p_adev = pDSP_handle->p_adev;
	xaf_comp_t *p_decoder = pDSP_handle->p_comp;
	xaf_comp_status *comp_status = &pDSP_handle->comp_status;
	long comp_info[4];

	while (p_decoder->pending_resp) {
		error = xaf_comp_get_status(p_adev, p_decoder, comp_status, &comp_info[0]);
		if (error != XAF_NO_ERR) {
			fprintf(stderr, "xaf_comp_get_status err %d\n", error);
			return ACODEC_ERROR_STREAM;
		}
		switch (*comp_status) {
		case XAF_PROBE_READY:
		case XAF_PROBE_DONE:
			return ACODEC_ERROR_STREAM;
		case XAF_EXEC_DONE:
			return ACODEC_END_OF_STREAM;
		case XAF_NEED_INPUT:
			pDSP_handle->inptr_busy = false;
			break;
		case XAF_INIT_DONE:
		case XAF_OUTPUT_READY:
			pDSP_handle->outptr_busy = false;
			break;
		default:
			return ACODEC_ERR_UNKNOWN;
		}
	}
	return ACODEC_SUCCESS;
}
