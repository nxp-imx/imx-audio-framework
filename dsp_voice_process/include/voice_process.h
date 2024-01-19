// Copyright 2023 NXP
// SPDX-License-Identifier: BSD-3-Clause

#ifndef FSL_VOICESEEKER_H
#define FSL_VOICESEEKER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef signed char             WORD8   ;
typedef signed char         *   pWORD8  ;
typedef unsigned char           UWORD8  ;
typedef unsigned char       *   pUWORD8 ;
typedef signed int              WORD32  ;
typedef signed int          *   pWORD32 ;
typedef unsigned int            UWORD32 ;
typedef unsigned int        *   pUWORD32;

#include "fsl_unia.h"

/*******************************************************************
 *
API function ID
*******************************************************************/

enum {
	OK = 0,
	NON_FATAL_ERROR = 0x1,
	FATAL_ERROR = 0x100F,
};

enum /* API function ID */
{
    AVOICE_PROCESS_API_GET_VERSION_INFO  = 0x0,
    /* creation & deletion */
    AVOICE_PROCESS_API_CREATE     = 0x1,
    AVOICE_PROCESS_API_DELETE     = 0x2,
    /* reset */
    AVOICE_PROCESS_API_RESET = 0x3,

    /* set parameter */
    AVOICE_PROCESS_API_SET_PARAMETER  = 0x10,
    AVOICE_PROCESS_API_GET_PARAMETER  = 0x11,

    /* process frame */
    AVOICE_PROCESS_API_DEC_FRAME    = 0x20,

    AVOICE_PROCESS_API_GET_LAST_ERROR = 0x1000,
};

struct Voice_Wrap_Handle_t {
	UniACodecMemoryOps   sMemOps;

	uint32_t             init_done;

	void                 *ref_buf;
	void                 *mic_buf;
	void                 *lib_inner_buf;

	int32_t              sample_rate;
	int32_t              channels;
	int32_t              width;

	uint32_t             framesize_in_mic;
	uint32_t             framesize_in_ref;
	uint32_t             expected_framesize_in_mic;
	uint32_t             expected_framesize_in_ref;

	bool                 enable_mic_resampling;
	bool                 enable_ref_resampling;

	void                 *heap_memory;
	void                 *scratch_memory;

	void                 *lib_handle;

#ifdef DEBUG_BASE
	int                  (*printf)(const char *fmt, ...);
#endif
};
typedef struct Voice_Wrap_Handle_t Voice_Wrap_Handle;


uint32_t lib_get_handler_size();
uint32_t lib_create(Voice_Wrap_Handle *voice_handle);
uint32_t lib_heap_memory_size_get(Voice_Wrap_Handle *voice_handle);
uint32_t lib_scratch_memory_size_get(Voice_Wrap_Handle *voice_handle);
uint32_t lib_memory_set(Voice_Wrap_Handle *voice_handle);
uint32_t lib_postprocess(Voice_Wrap_Handle *voice_handle, void *input[2], uint32_t *consumed_off[2], void *output[2], uint32_t *produced[2]);
uint32_t lib_process(Voice_Wrap_Handle *voice_handle, void *input[2], uint32_t input_length[2],
		uint32_t *consumed_off[2], void *output[2], uint32_t *produced[2]);
uint32_t lib_process_times(Voice_Wrap_Handle *voice_handle, unsigned int input_length[2]);
uint32_t lib_preprocess(Voice_Wrap_Handle *voice_handle);
uint32_t lib_process_check(Voice_Wrap_Handle *voice_handle, void *input[2], unsigned int input_length[2]);
uint32_t lib_set_param(Voice_Wrap_Handle *voice_handle, int para_cmd, void *parameter);
uint32_t lib_reset(Voice_Wrap_Handle *voice_handle);
uint32_t lib_init(Voice_Wrap_Handle *voice_handle);
uint32_t lib_inner_size_get(Voice_Wrap_Handle *voice_handle);
uint32_t lib_ref_in_size_get(Voice_Wrap_Handle *voice_handle);
uint32_t lib_mic_in_size_get(Voice_Wrap_Handle *voice_handle);
uint32_t lib_pre_init(Voice_Wrap_Handle *voice_handle);

#endif
