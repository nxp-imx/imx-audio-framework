// Copyright 2023 NXP
// SPDX-License-Identifier: BSD-3-Clause

#include "voice_process.h"

#define DEFAULT_MIC_IN_SAMPLES 200
#define DEFAULT_REF_IN_SAMPLES 200

typedef struct lib_handle_t {
	int32_t              mic_in_samples;
	int32_t              ref_in_samples;

	void                 *heap_memory;
	void                 *scratch_memory;
	void                 *lib_inner_buf;
	int32_t              heap_size;
	int32_t              scratch_size;
	int32_t              inner_size;

	uint32_t             mic_in_fs;
	uint32_t             mic_channels;
	uint32_t             mic_pcm_width;
	uint32_t             ref_in_fs;
	uint32_t             ref_channels;
	uint32_t             ref_pcm_width;

} Lib_Handle;

uint32_t lib_get_handler_size()
{
	return sizeof(Lib_Handle);
}

/* ... create/open library, set env value for library to run */
uint32_t lib_create(Voice_Wrap_Handle *voice_handle)
{
	uint32_t ret = OK;
	Lib_Handle *lib_handle = (Lib_Handle *)voice_handle->lib_handle;

	lib_handle->mic_in_samples = DEFAULT_MIC_IN_SAMPLES;
	lib_handle->ref_in_samples = DEFAULT_REF_IN_SAMPLES;

	lib_handle->heap_size = 0;
	lib_handle->scratch_size = 0;
	lib_handle->inner_size = 0;

	/* set default mic in 16KHz, 2 channels, 16bit */
	lib_handle->mic_in_fs = 16000;
	lib_handle->mic_channels = 2;
	lib_handle->mic_pcm_width = 16;

	/* set default ref in 16KHz, 2 channels, 16bit */
	lib_handle->ref_in_fs = 16000;
	lib_handle->ref_channels = 2;
	lib_handle->ref_pcm_width = 16;

	return ret;
}

/* ... get heap memory size, set in lib_memory_set */
uint32_t lib_heap_memory_size_get(Voice_Wrap_Handle *voice_handle)
{
	Lib_Handle *lib_handle = (Lib_Handle *)voice_handle->lib_handle;
	return lib_handle->heap_size;
}

/* ... get scratch memory size, set in lib_memory_set */
uint32_t lib_scratch_memory_size_get(Voice_Wrap_Handle *voice_handle)
{
	Lib_Handle *lib_handle = (Lib_Handle *)voice_handle->lib_handle;
	return lib_handle->scratch_size;
}

/* ... get mic in buffer size, mic in buffer keeps capturered data */
uint32_t lib_mic_in_size_get(Voice_Wrap_Handle *voice_handle)
{
	Lib_Handle *lib_handle = (Lib_Handle *)voice_handle->lib_handle;
	uint32_t mic_in =  lib_handle->mic_in_samples * lib_handle->mic_channels
		* (lib_handle->mic_pcm_width / 8);
	return mic_in;
}

/* ... get ref in buffer size, ref in buffer keeps playback data */
uint32_t lib_ref_in_size_get(Voice_Wrap_Handle *voice_handle)
{
	Lib_Handle *lib_handle = (Lib_Handle *)voice_handle->lib_handle;
	uint32_t ref_in =  lib_handle->ref_in_samples * lib_handle->ref_channels
		* (lib_handle->ref_pcm_width / 8);
	return ref_in;
}

/* ... get inner memory size, set in lib_memory_set */
uint32_t lib_inner_size_get(Voice_Wrap_Handle *voice_handle)
{
	Lib_Handle *lib_handle = (Lib_Handle *)voice_handle->lib_handle;
	return lib_handle->inner_size;
}

/* ... set memory for library to use */
uint32_t lib_memory_set(Voice_Wrap_Handle *voice_handle)
{
	Lib_Handle *lib_handle = (Lib_Handle *)voice_handle->lib_handle;
	uint32_t ret = OK;

	lib_handle->heap_memory = voice_handle->heap_memory;
	lib_handle->scratch_memory = voice_handle->scratch_memory;
	lib_handle->lib_inner_buf = voice_handle->lib_inner_buf;

	return ret;
}

/* ... library preinit */
uint32_t lib_pre_init(Voice_Wrap_Handle *voice_handle)
{
	uint32_t ret = OK;
	return ret;
}

/* ... library init */
uint32_t lib_init(Voice_Wrap_Handle *voice_handle)
{
	uint32_t ret = OK;

	return ret;
}

/* ... library reset */
uint32_t lib_reset(Voice_Wrap_Handle *voice_handle)
{
	uint32_t ret = OK;
	return ret;
}

/* ... set parameter */
uint32_t lib_set_param(Voice_Wrap_Handle *voice_handle, int para_cmd, void *parameter)
{
	Lib_Handle *lib_handle = (Lib_Handle *)voice_handle->lib_handle;
	UniACodecParameter *para = (UniACodecParameter *)parameter;
	uint32_t ret = OK;

	switch (para_cmd) {
	case UNIA_SAMPLERATE:
		lib_handle->ref_in_fs = (uint32_t)para->samplerate;
		break;
	case UNIA_CHANNEL:
		lib_handle->ref_channels = (uint32_t)para->channels;
		break;
	case UNIA_DEPTH:
		lib_handle->ref_pcm_width = (uint32_t)para->depth;
		break;
	default:
		return NON_FATAL_ERROR;
	}
	return ret;
}

/* ... get parameter */
uint32_t lib_get_param(Voice_Wrap_Handle *voice_handle, int para_cmd, void *parameter)
{
	Lib_Handle *lib_handle = (Lib_Handle *)voice_handle->lib_handle;
	uint32_t ret = OK;
	UniACodecParameter *para = (UniACodecParameter *)parameter;

	switch (para_cmd) {
	case UNIA_SAMPLERATE:
		para->samplerate = lib_handle->ref_in_fs;
		break;
	case UNIA_CHANNEL:
		para->channels = lib_handle->ref_channels;
		break;
	case UNIA_DEPTH:
		para->depth = lib_handle->ref_pcm_width;
		break;
	default:
		return NON_FATAL_ERROR;
	}

	return ret;
}

/* ... check if library ready to process data */
uint32_t lib_process_check(Voice_Wrap_Handle *voice_handle, void *input[2], int input_length[2])
{
	Lib_Handle *lib_handle = (Lib_Handle *)voice_handle->lib_handle;
	uint32_t ret = OK;

	uint32_t mic_channels = lib_handle->mic_channels;
	uint32_t mic_pcm_width = lib_handle->mic_pcm_width;
	uint32_t ref_channels = lib_handle->ref_channels;
	uint32_t ref_pcm_width = lib_handle->ref_pcm_width;
	uint32_t expected_mic_in = lib_handle->mic_in_samples * mic_channels
		* (mic_pcm_width / 8);
	uint32_t expected_ref_in = lib_handle->ref_in_samples * ref_channels
		* (ref_pcm_width / 8);

	if (expected_mic_in > input_length[1] || expected_ref_in > input_length[0])
		ret = NON_FATAL_ERROR;

	return ret;
}

/* ... library preprocessing */
uint32_t lib_preprocess(Voice_Wrap_Handle *voice_handle)
{
	uint32_t ret = OK;
	return ret;
}

/* ... calculate library process times */
uint32_t lib_process_times(Voice_Wrap_Handle *voice_handle, int input_length[2])
{
	return 1;
}

/* ... library processing function */
uint32_t lib_process(Voice_Wrap_Handle *voice_handle, void *input[2], uint32_t input_length[2],
		uint32_t *consumed_off[2], void *output[2], uint32_t *produced[2])
{
	uint32_t ret = OK;
	char *input0 = NULL;
	char *input1 = NULL;
	uint32_t input_length0, input_length1;
	char *output0 = NULL;
	char *output1 = NULL;
	if (!voice_handle)
		return FATAL_ERROR;

	input0 = input[0];
	input1 = input[1];
	input_length0 = input_length[0];
	input_length1 = input_length[1];
	output0 = output[0];
	output1 = output[1];

	if (!input0 && !input1)
		return ret;

	if (input0 && output0 && input_length0) {
		memcpy(output0, input0, input_length0);
		*consumed_off[0] = input_length0;
		*produced[0] = input_length0;
	}
	if (input1 && output1 && input_length1) {
		memcpy(output1, input1, input_length1);
		*consumed_off[1] = input_length1;
		*produced[1] = input_length1;
	}

	return ret;
}

/* ... library postprocessing */
uint32_t lib_postprocess(Voice_Wrap_Handle *voice_handle, void *input[2], uint32_t *consumed_off[2], void *output[2], uint32_t produced[2])
{
	uint32_t ret = OK;

	return ret;
}

