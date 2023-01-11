/*
* Copyright 2023 NXP.
* SPDX-License-Identifier: BSD-3-Clause
*
*/
/*******************************************************************************
 * xa-voice_proc-api.h
 *
 * voice process component API
 ******************************************************************************/

#ifndef __XA_VOICE_PROC_API_H__
#define __XA_VOICE_PROC_API_H__

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "xa_memory_standards.h"

/* ...generic commands */
#include "xa_apicmd_standards.h"

/* ...generic error codes */
#include "xa_error_standards.h"

/* ...common types */
#include "xa_type_def.h"

/* ...component identifier (informative) */
#define XA_MIMO_PROC_VOICE_PROC                  0x80

/*******************************************************************************
 * Class 1: Configuration Errors
 ******************************************************************************/

enum xa_error_nonfatal_config_voice_proc {
    XA_VOICE_PROC_CONFIG_NONFATAL_RANGE  = XA_ERROR_CODE(xa_severity_nonfatal, xa_class_config, XA_MIMO_PROC_VOICE_PROC, 0),
};

enum xa_error_fatal_config_voice_proc {
    XA_VOICE_PROC_CONFIG_FATAL_RANGE     = XA_ERROR_CODE(xa_severity_fatal, xa_class_config, XA_MIMO_PROC_VOICE_PROC, 0),
};

/*******************************************************************************
 * Class 2: Execution Class Errors
 ******************************************************************************/

enum xa_error_nonfatal_execute_voice_proc {
    XA_VOICE_PROC_EXEC_NONFATAL_NO_DATA  = XA_ERROR_CODE(xa_severity_nonfatal, xa_class_execute, XA_MIMO_PROC_VOICE_PROC, 0),
};

enum xa_error_fatal_execute_voice_proc {
    XA_VOICE_PROC_EXEC_FATAL_STATE       = XA_ERROR_CODE(xa_severity_fatal, xa_class_execute, XA_MIMO_PROC_VOICE_PROC, 0),
};

#endif /* __XA_VOICE_PROC_API_H__ */
