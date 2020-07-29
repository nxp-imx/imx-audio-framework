/*******************************************************************************
* Copyright (C) 2017 Cadence Design Systems, Inc.
* Copyright 2020 NXP
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
 * xa-pcm-api.h
 *
 * Generic PCM format converter API
 *
 ******************************************************************************/

#ifndef __XA_PCM_API_H__
#define __XA_PCM_API_H__

/*******************************************************************************
 * Includes
 ******************************************************************************/

/*******************************************************************************
 * Constants definitions
 ******************************************************************************/

/* ...codec-specific configuration parameters */
enum xa_config_param_pcm {
    XA_PCM_CONFIG_PARAM_SAMPLE_RATE         = 0,
    XA_PCM_CONFIG_PARAM_IN_PCM_WIDTH        = 1,
    XA_PCM_CONFIG_PARAM_IN_CHANNELS         = 2,
    XA_PCM_CONFIG_PARAM_OUT_PCM_WIDTH       = 3,
    XA_PCM_CONFIG_PARAM_OUT_CHANNELS        = 4,
    XA_PCM_CONFIG_PARAM_CHANROUTING         = 5,
    XA_PCM_CONFIG_PARAM_FUNC_PRINT          = 13,
    XA_PCM_CONFIG_PARAM_NUM                 = 14,
};

#endif /* __XA_PCM_API_H__ */

