//*****************************************************************
// Copyright 2018 NXP
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//*****************************************************************

#ifndef _PROCESS_H_
#define _PROCESS_H_

#include "mytypes.h"

HIFI_ERROR_TYPE hifiCdc_init(codec_info *codec_info_ptr, hifi_main_struct *hifi_config);
HIFI_ERROR_TYPE hifiCdc_open(codec_info *codec_info_ptr);
HIFI_ERROR_TYPE hifiCdc_close (codec_info *codec_info_ptr);
HIFI_ERROR_TYPE hifiCdc_reset (codec_info *codec_info_ptr);
HIFI_ERROR_TYPE hifiCdc_getprop (icm_pcm_prop_t *ext_msg, codec_info *codec_info_ptr);
HIFI_ERROR_TYPE hifiCdc_process_emptybuf(icm_cdc_iobuf_t *pext_msg, codec_info *codec_info_ptr);
HIFI_ERROR_TYPE xa_hifi_cdc_set_param_config(icm_prop_config *pext_msg, codec_info *codec_info_ptr);

#endif
