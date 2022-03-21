/*******************************************************************************
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
 ******************************************************************************/

#ifndef _XF_AUDIO_APICMD_H
#define _XF_AUDIO_APICMD_H

typedef void *  xf_codec_handle_t;

typedef UWORD32 xf_codec_func_t(xf_codec_handle_t handle,
				UWORD32 cmd,
				UWORD32 idx,
				void *p_value);

/*****************************************************************************/
/* Standard API commands                                                     */
/*****************************************************************************/

enum xf_api_cmd_generic {
	XF_API_CMD_GET_API_SIZE       = 0x0001,

	XF_API_CMD_PRE_INIT           = 0x0002,
	XF_API_CMD_INIT               = 0x0003,
	XF_API_CMD_POST_INIT          = 0x0004,

	XF_API_CMD_SET_PARAM          = 0x0005,
	XF_API_CMD_GET_PARAM          = 0x0006,

	XF_API_CMD_EXECUTE            = 0x0007,

	XF_API_CMD_SET_INPUT_PTR      = 0x0008,
	XF_API_CMD_SET_OUTPUT_PTR     = 0x0009,

	XF_API_CMD_SET_INPUT_BYTES    = 0x000A,
	XF_API_CMD_GET_OUTPUT_BYTES   = 0x000B,
	XF_API_CMD_GET_CONSUMED_BYTES = 0x000C,
	XF_API_CMD_INPUT_OVER         = 0x000D,
	XF_API_CMD_RUNTIME_INIT       = 0x000E,
	XF_API_CMD_CLEANUP            = 0x000F,
	XF_API_CMD_SET_LIB_ENTRY      = 0x0010,
	XF_API_CMD_SUSPEND            = 0x0011,
	XF_API_CMD_RESUME             = 0x0012,
	XF_API_CMD_PAUSE              = 0x0013,
	XF_API_CMD_PAUSE_RELEASE      = 0x0014,

};

#endif
