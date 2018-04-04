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


#ifndef _XAF_API_H
#define _XAF_API_H

#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <math.h>
#include <malloc.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <linux/types.h>
#include "xf-proxy.h"
#include "library_load.h"
#include "mxc_hifi4.h"

/* ...size of auxiliary pool for communication with HiFi */
#define XA_AUX_POOL_SIZE                32

/* ...length of auxiliary pool messages */
#define XA_AUX_POOL_MSG_LENGTH          128

/* ...number of max input buffers */
#define INBUF_SIZE                      4096
#define OUTBUF_SIZE                     16384

typedef struct xaf_adev_s   xaf_adev_t;
typedef struct xaf_info_s   xaf_info_t;
typedef struct xaf_comp     xaf_comp_t;
typedef struct xaf_pipeline xaf_pipeline_t;

struct xaf_adev_s {
	xf_proxy_t      proxy;
};

struct xaf_info_s {
	u32             opcode;
	void            *buf;
	u32             length;
	u32             ret;
};

struct xaf_comp {
	xaf_comp_t      *next;

	xaf_pipeline_t  *pipeline;
	xf_handle_t     handle;

	xf_id_t         dec_id;
	int             comp_type;

	xf_pool_t       *inpool;
	xf_pool_t       *outpool;
	void            *inptr;
	void            *outptr;

	struct lib_info codec_lib;
	struct lib_info codec_wrap_lib;
};

struct xaf_pipeline {
	xaf_comp_t      *comp_chain;

	u32 input_eos;
	u32 output_eos;
};

int xaf_adev_open(xaf_adev_t *p_adev);
int xaf_adev_close(xaf_adev_t *p_adev);

int xaf_comp_create(xaf_adev_t *p_adev, xaf_comp_t *p_comp, int comp_type);
int xaf_comp_delete(xaf_comp_t *p_comp);

int xaf_comp_set_config(xaf_comp_t *p_comp, u32 num_param, void *p_param);
int xaf_comp_get_config(xaf_comp_t *p_comp, u32 num_param, void *p_param);

int xaf_comp_process(xaf_comp_t *p_comp, void *p_buf, u32 length, u32 flag);
int xaf_comp_get_status(xaf_comp_t *p_comp, xaf_info_t *p_info);

int xaf_connect(xaf_comp_t *p_src, xaf_comp_t *p_dest, u32 num_buf, u32 buf_length);
int xaf_disconnect(xaf_comp_t *p_comp);

int xaf_comp_add(xaf_pipeline_t *p_pipe, xaf_comp_t *p_comp);

int xaf_pipeline_create(xaf_adev_t *p_adev, xaf_pipeline_t *p_pipe);
int xaf_pipeline_delete(xaf_pipeline_t *p_pipe);

int xaf_pipeline_send_eos(xaf_pipeline_t *p_pipe);

#endif
