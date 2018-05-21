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
#include "mxc_dsp.h"

/* ...size of auxiliary pool for communication with DSP */
#define XA_AUX_POOL_SIZE                32

/* ...length of auxiliary pool messages */
#define XA_AUX_POOL_MSG_LENGTH          128

/* ...number of max input buffers */
#define INBUF_SIZE                      4096
#define OUTBUF_SIZE                     16384

/* ...pre-declaration of struct */
struct xaf_pipeline;

struct xaf_adev_s {
	struct xf_proxy  proxy;
};

struct xaf_info_s {
	u32             opcode;
	void            *buf;
	u32             length;
	u32             ret;
};

struct xaf_comp {
	struct xaf_comp      *next;

	struct xaf_pipeline  *pipeline;
	struct xf_handle     handle;

	const char      *dec_id;
	int             comp_type;

	struct xf_pool  *inpool;
	struct xf_pool  *outpool;
	void            *inptr;
	void            *outptr;

	struct lib_info codec_lib;
	struct lib_info codec_wrap_lib;
};

struct xaf_pipeline {
	struct xaf_comp      *comp_chain;

	u32 input_eos;
	u32 output_eos;
};

int xaf_adev_open(struct xaf_adev_s *p_adev);
int xaf_adev_close(struct xaf_adev_s *p_adev);

int xaf_comp_create(struct xaf_adev_s *p_adev,
		    struct xaf_comp *p_comp,
		    int comp_type);
int xaf_comp_delete(struct xaf_comp *p_comp);

int xaf_comp_set_config(struct xaf_comp *p_comp, u32 num_param, void *p_param);
int xaf_comp_get_config(struct xaf_comp *p_comp, u32 num_param, void *p_param);

int xaf_comp_flush(struct xaf_comp *p_comp);

int xaf_comp_process(struct xaf_comp *p_comp,
		     void *p_buf,
		     u32 length,
		     u32 flag);
int xaf_comp_get_status(struct xaf_comp *p_comp, struct xaf_info_s *p_info);
int xaf_comp_get_msg_count(struct xaf_comp *p_comp);

int xaf_connect(struct xaf_comp *p_src,
		struct xaf_comp *p_dest,
		u32 num_buf,
		u32 buf_length);
int xaf_disconnect(struct xaf_comp *p_comp);

int xaf_comp_add(struct xaf_pipeline *p_pipe, struct xaf_comp *p_comp);

int xaf_pipeline_create(struct xaf_adev_s *p_adev, struct xaf_pipeline *p_pipe);
int xaf_pipeline_delete(struct xaf_pipeline *p_pipe);

int xaf_pipeline_send_eos(struct xaf_pipeline *p_pipe);

#endif
