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
******************************************************************************/

/*******************************************************************************
 * xa-renderer-api.h
 *
 * Renderer component API
 ******************************************************************************/

#ifndef __XA_RENDERER_API_H__
#define __XA_RENDERER_API_H__

/*******************************************************************************
 * Includes
 ******************************************************************************/

/*******************************************************************************
 * Constants definitions
 ******************************************************************************/

/* ...renderer-specific configuration parameters */
enum xa_config_param_renderer {
    XA_RENDERER_CONFIG_PARAM_CB             = 0,
    XA_RENDERER_CONFIG_PARAM_STATE          = 1,
    XA_RENDERER_CONFIG_PARAM_PCM_WIDTH      = 2,
    XA_RENDERER_CONFIG_PARAM_CHANNELS       = 3,
    XA_RENDERER_CONFIG_PARAM_SAMPLE_RATE    = 4,
    XA_RENDERER_CONFIG_PARAM_FRAME_SIZE     = 5,
    XA_RENDERER_CONFIG_PARAM_NUM            = 6
};

/* ...XA_RENDERER_CONFIG_PARAM_CB: compound parameters data structure */
typedef struct xa_renderer_cb_s {
    /* ...input buffer completion callback */
    void      (*cb)(struct xa_renderer_cb_s *, u32 idx);

} xa_renderer_cb_t;


/* ...renderer states  */
enum xa_randerer_state {
    XA_RENDERER_STATE_IDLE  = 0,
    XA_RENDERER_STATE_RUN   = 1,
    XA_RENDERER_STATE_PAUSE = 2
};

/* ...component identifier (informative) */
#define XA_CODEC_RENDERER               2

/*******************************************************************************
 * API function definition (tbd)
 ******************************************************************************/

#endif /* __XA_RENDERER_API_H__ */
