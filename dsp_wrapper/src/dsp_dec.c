/*****************************************************************
 * Copyright 2018 NXP
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
#include "fsl_unia.h"
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
			      uint8 *InputBuf,
			      uint32 InputSize,
			      uint32 *offset,
			      bool framed) {
	UA_ERROR_TYPE ret = ACODEC_SUCCESS;

	uint8 *inner_data;
	uint32 *inner_offset;
	uint32 *inner_size;
	uint32 bufsize;
	uint32 inbuf_remain, innerbuf_avail;
	uint32 threshold;

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
			    uint32 buf_size,
			    uint32 threshold) {
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
	int err = XA_SUCCESS;

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

/*
 * channel_pos_convert: Adjust channel position according to layout
 *
 * @pua_handle : decoder or encoder handle
 * @data_in    : data in buffer pointer
 * @frameSize  : bytes in data in buffer
 * @channels   : channel number
 * @depth      : width of sample
 */
void  channel_pos_convert(UniACodec_Handle pua_handle,
			  uint8 *data_in,
			  int32 frameSize,
			  int32 channels,
			  int32 depth) {
	struct DSP_Handle *pDSP_handle = (struct DSP_Handle *)pua_handle;
	int32 i, j;
	uint32 loopcnt = frameSize / depth;
	int32 ChanMap[8] = {0, 1, 2, 3, 4, 5, 6, 7};
	int16 tmp_16[8];
	int32 tmp_32[8];
	uint32 *dest_layout = pDSP_handle->outputFormat.layout;
	uint32 *aac_layout = aacd_channel_layouts[channels];
	int16 *ptr_16 = (int16 *)data_in;
	uint8 *ptr_8;

	if (pDSP_handle->outputFormat.chan_pos_set == TRUE) {
		// If set channel layout
		for (i = 0; i < channels; i++) {
			for (j = 0; j < channels; j++) {
				if (dest_layout[i] == aac_layout[j]) {
					ChanMap[i] = j;
					break;
				}
			}
			if (j == channels) {
				pDSP_handle->outputFormat.chan_pos_set = FALSE;
				break;
			}
		}
	}

	if (pDSP_handle->outputFormat.chan_pos_set) {
		if (depth == 16) {
			for (i = 0; i < loopcnt; i += channels) {
				for (j = 0; j < channels; j++)
					tmp_16[j] = ptr_16[i + ChanMap[j]];

				for (j = 0; j < channels; j++)
					*ptr_16++ = tmp_16[j];
			}
		} else if (depth == 24) {
			for (i = 0; i < loopcnt; i += channels) {
				for (j = 0; j < channels; j++) {
					ptr_8 = data_in + (i + ChanMap[j]) * 3;
					tmp_32[j] = (*ptr_8)    |
						(*(ptr_8 + 1) << 8) |
						(*(ptr_8 + 2) << 16);
				}
				for (j = 0; j < channels; j++) {
					ptr_8 = data_in + (i + j) * 3;
					*ptr_8 = tmp_32[j] & 0xff;
					*(ptr_8 + 1) = (tmp_32[j] >> 8) & 0xff;
					*(ptr_8 + 2) = (tmp_32[j] >> 16) & 0xff;
				}
			}
		}
	}
}

/* cancel_unused_channel_data - transfer 2 channel output to 1 channel output
 *    when disable mono_to_stereo mode for aacplus, bsac and dabplus
 *
 * @data_in: data in buffer pointer
 * @length: byte number in data in buffer
 * @depth: width of sample
 */
void cancel_unused_channel_data(uint8 *data_in, int32 length, int32 depth)
{
	uint32 i;
	uint8 *ptr1, *ptr2;

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
		 uint8 *input,
		 uint32 in_size,
		 uint32 *in_off,
		 uint8 *output,
		 uint32 *out_size)
{
	struct DSP_Handle *pDSP_handle = (struct DSP_Handle *)pua_handle;
	struct xaf_comp *p_comp = &pDSP_handle->component;
	struct xaf_pipeline *p_pipe = &pDSP_handle->pipeline;
	struct xaf_info_s p_info;
	int err = 0, count;
	int ret = XA_SUCCESS;

	if (!input || !output)
		return XA_PARA_ERROR;

	if (!pDSP_handle->outptr_busy) {
		err = xaf_comp_process(p_comp,
				       p_comp->outptr,
				       OUTBUF_SIZE,
				       XF_FILL_THIS_BUFFER);
		pDSP_handle->outptr_busy = TRUE;
	}

	if (!pDSP_handle->inptr_busy) {
		if (in_size) {
			memcpy(p_comp->inptr, input, in_size);
			*in_off = in_size;

			err = xaf_comp_process(p_comp,
					       p_comp->inptr,
					       in_size,
					       XF_EMPTY_THIS_BUFFER);
		} else {
			err = xaf_comp_process(p_comp,
					       NULL,
					       0,
					       XF_EMPTY_THIS_BUFFER);
		}

		pDSP_handle->inptr_busy = TRUE;
	}

	do {
		/* ...wait until result is delivered */
		err = xaf_comp_get_status(p_comp, &p_info);
		if (err) {
			ret = XA_ERR_UNKNOWN;
			break;
		}

		if ((p_info.opcode == XF_FILL_THIS_BUFFER) &&
		    (p_info.buf == p_comp->outptr)) {
			memcpy(output, p_comp->outptr, p_info.length);
			*out_size = p_info.length;

			pDSP_handle->outptr_busy = FALSE;
			ret = p_info.ret;
			break;
		} else {
			/* ...make sure response is expected */
			if ((p_info.opcode == XF_EMPTY_THIS_BUFFER) &&
			    (p_info.buf == p_comp->inptr)) {
				pDSP_handle->inptr_busy = FALSE;

				/* ...get message count in pipe */
				count = xaf_comp_get_msg_count(p_comp);
				if (count > 0) {
					continue;
				} else {
					ret = XA_SUCCESS;
					break;
				}
			} else {
				ret = XA_ERR_UNKNOWN;
				break;
			}
		}
	} while (1);

	return ret;
}

