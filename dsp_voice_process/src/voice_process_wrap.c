// Copyright 2023 NXP
// SPDX-License-Identifier: BSD-3-Clause

#include <string.h>

#include "voice_process.h"

#define WRAP_VERSION_STR  \
	( "dsp voice process wrap" \
	" " "build on" \
	" __DATE__ " " __TIME__")

#define VOICESEEKER_OUT_NHOP 200
#define RDSP_BUFFER_LENGTH_SEC 1.5f
#define RDSP_AEC_FILTER_LENGTH_MS 150 // TODO: can AEC filter length be reduced?
#define RDSP_DISABLE_TRIGGER_TIMEOUT_SEC 2
#define RDSP_MAX_TRIGGERS 100

void *voice_proc_create(UniACodecMemoryOps * memOps)
{
	Voice_Wrap_Handle *voice_handle = NULL;

	if((memOps == NULL) ||
	   (memOps->Malloc == NULL) ||
	   (memOps->Free == NULL)
	)
		return NULL;

	voice_handle = memOps->Malloc(sizeof(Voice_Wrap_Handle));
	if(voice_handle == NULL)
		return NULL;

	memset(voice_handle, 0, sizeof(Voice_Wrap_Handle));
	memcpy(&(voice_handle->sMemOps), memOps, sizeof(UniACodecMemoryOps));

	voice_handle->init_done = 0;

	voice_handle->sample_rate = 48000;
	voice_handle->channels = 2;
	voice_handle->width = 16;

	return voice_handle;
}

uint32_t voice_proc_init(void *p_voice_handle)
{
	uint32_t ret = 0;
	Voice_Wrap_Handle *voice_handle = p_voice_handle;
	uint32_t heap_size = 0;
	uint32_t scratch_size = 0;
	uint32_t ref_in_buf_size = 0;
	uint32_t mic_in_buf_size = 0;
	uint32_t lib_inner_buf_size = 0;

	if (!voice_handle)
		return FATAL_ERROR;

	uint32_t lib_size = lib_get_handler_size();
	if (lib_size > 0) {
		voice_handle->lib_handle = voice_handle->sMemOps.Malloc(lib_size);
		if (voice_handle->lib_handle == NULL) {
			return FATAL_ERROR;
		}
	}
	/* Get lib handler */
	ret = lib_create(voice_handle);
	if (ret) {
		return ret;
	}

	heap_size = lib_heap_memory_size_get(voice_handle);
	if (heap_size > 0) {
		voice_handle->heap_memory = voice_handle->sMemOps.Malloc(heap_size);
		if (voice_handle->heap_memory == NULL) {
			return FATAL_ERROR;
		}
	}
	scratch_size = lib_scratch_memory_size_get(voice_handle);
	if (scratch_size > 0) {
		voice_handle->scratch_memory = voice_handle->sMemOps.Malloc(scratch_size);
		if (voice_handle->scratch_memory == NULL) {
			return FATAL_ERROR;
		}
	}
	ret = lib_memory_set(voice_handle);
	if (ret) {
		return ret;
	}

	ret = lib_pre_init(voice_handle);
	if (ret) {
		return ret;
	}

	mic_in_buf_size = lib_mic_in_size_get(voice_handle);
	if (mic_in_buf_size < 0)
		return FATAL_ERROR;
	voice_handle->mic_buf = voice_handle->sMemOps.Malloc(mic_in_buf_size);
	if (voice_handle->mic_buf == NULL) {
		return FATAL_ERROR;
	}
	ref_in_buf_size = lib_ref_in_size_get(voice_handle);
	if (ref_in_buf_size < 0)
		return FATAL_ERROR;
	voice_handle->ref_buf = voice_handle->sMemOps.Malloc(ref_in_buf_size);
	if (voice_handle->ref_buf == NULL) {
		return FATAL_ERROR;
	}
	lib_inner_buf_size = lib_inner_size_get(voice_handle);
	if (lib_inner_buf_size > 0) {
		voice_handle->lib_inner_buf = voice_handle->sMemOps.Malloc(lib_inner_buf_size);
		if (voice_handle->lib_inner_buf == NULL) {
			return FATAL_ERROR;
		}
	}

	ret = lib_init(voice_handle);
	if (ret) {
		return ret;
	}

	voice_handle->init_done = 1;

	return OK;
}

uint32_t voice_proc_delete(void *p_voice_handle)
{
	Voice_Wrap_Handle *voice_handle = p_voice_handle;
	if (!voice_handle)
		return FATAL_ERROR;
	uint32_t ret = OK;

	if (voice_handle->mic_buf)
		voice_handle->sMemOps.Free(voice_handle->mic_buf);
	if (voice_handle->ref_buf)
		voice_handle->sMemOps.Free(voice_handle->ref_buf);
	if (voice_handle->lib_inner_buf)
		voice_handle->sMemOps.Free(voice_handle->lib_inner_buf);
	if (voice_handle->scratch_memory)
		voice_handle->sMemOps.Free(voice_handle->scratch_memory);
	if (voice_handle->heap_memory)
		voice_handle->sMemOps.Free(voice_handle->heap_memory);

	voice_handle->sMemOps.Free(voice_handle);

	return ret;
}

uint32_t voice_proc_reset(void *voice_handle)
{
	if (!voice_handle)
		return FATAL_ERROR;
	return lib_reset(voice_handle);
}

uint32_t voice_proc_setparameter(void *p_voice_handle, UA_ParaType para_cmd, UniACodecParameter *parameter)
{
	if (!p_voice_handle)
		return FATAL_ERROR;
	Voice_Wrap_Handle *voice_handle = p_voice_handle;

	switch (para_cmd) {
	case UNIA_SAMPLERATE:
		voice_handle->sample_rate = (uint32_t)parameter->samplerate;
		break;
	case UNIA_CHANNEL:
		voice_handle->channels = (uint32_t)parameter->channels;
		break;
	case UNIA_DEPTH:
		voice_handle->width = (uint32_t)parameter->depth;
		break;
#ifdef DEBUG_BASE
	case UNIA_FUNC_PRINT:
		voice_handle->printf = parameter->Printf;
		voice_handle->printf("voice process lib print set!\n");
		break;
#endif
	default:
		return FATAL_ERROR;
	}

	return lib_set_param(voice_handle, para_cmd, parameter);
}

uint32_t voice_proc_getparameter(void *p_voice_handle, UA_ParaType para_cmd, UniACodecParameter *parameter)
{
	if (!p_voice_handle)
		return FATAL_ERROR;
	Voice_Wrap_Handle *voice_handle = p_voice_handle;

	switch (para_cmd) {
	case UNIA_SAMPLERATE:
		parameter->samplerate = voice_handle->sample_rate;
		break;
	case UNIA_CHANNEL:
		parameter->channels = voice_handle->channels;
		break;
	case UNIA_DEPTH:
		parameter->depth = voice_handle->width;
		break;
	default:
		return FATAL_ERROR;
	}
	return OK;
}

/*
 * input[0]: ref_in stream, input[1]: mic_in stream
 */
uint32_t voice_proc_execute(void *p_voice_handle, void *input[2], int input_length[2],
		int *consumed[2], void *output[2], int *produced[2])
{
	Voice_Wrap_Handle *voice_handle = p_voice_handle;
	if (!voice_handle)
		return FATAL_ERROR;
	uint32_t ret = OK;

	uint32_t voice_proc_iteration = 0;
	uint32_t ref_consumed_off = 0, mic_consumed_off = 0;
	uint32_t *consumed_off[2];

	consumed_off[0] = &ref_consumed_off;
	consumed_off[1] = &mic_consumed_off;

	if (voice_handle->init_done == 0) {
		ret = voice_proc_init(voice_handle);
		if (ret)
			return ret;
	}

	ret = lib_process_check(voice_handle, input, input_length);
	if (ret) {
		return ret;
	}

	ret = lib_preprocess(voice_handle);
	if (ret) {
		return ret;
	}

	voice_proc_iteration = lib_process_times(voice_handle, input_length[2]);

	while (voice_proc_iteration) {

		ret = lib_process(voice_handle, input, input_length, consumed_off, output, produced);
		if (ret) {
			break;
		}

		ret = lib_postprocess(voice_handle, input, consumed_off, output, produced);
		if (ret) {
			break;
		}

		voice_proc_iteration--;
	}

	*consumed[0] = *consumed_off[0];
	*consumed[1] = *consumed_off[1];

	return ret;
}

const char *voice_lib_vers_info() {
	return (const char *)WRAP_VERSION_STR;
}

uint32_t VoiceSeekerQueryInterface(unsigned int id, void ** func)
{
	if (func == NULL)
		return -1;

	switch(id) {
		case AVOICE_PROCESS_API_GET_VERSION_INFO:
			*func = (void *)voice_lib_vers_info;
			break;
		case AVOICE_PROCESS_API_CREATE:
			*func = (void *)voice_proc_create;
			break;
		case AVOICE_PROCESS_API_DELETE:
			*func = (void *)voice_proc_delete;
			break;
		case AVOICE_PROCESS_API_RESET:
			*func = voice_proc_reset;
			break;
		case AVOICE_PROCESS_API_SET_PARAMETER:
			*func = voice_proc_setparameter;
			break;
		case AVOICE_PROCESS_API_GET_PARAMETER:
			*func = voice_proc_getparameter;
			break;
		case AVOICE_PROCESS_API_DEC_FRAME:
			*func = voice_proc_execute;
			break;
		case AVOICE_PROCESS_API_GET_LAST_ERROR:
			*func = NULL;
			break;
		default:
			*func = NULL;
			break;
	}

	return OK;
}

void *_start(void)
{
	return VoiceSeekerQueryInterface;
}

