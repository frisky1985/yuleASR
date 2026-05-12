/* Dlt_Cfg.h - AUTOSAR Diagnostic Log and Trace Configuration */
#ifndef DLT_CFG_H
#define DLT_CFG_H

#include "Std_Types.h"

/* AUTOSAR Version Information */
#define DLT_CFG_AR_MAJOR_VERSION    4
#define DLT_CFG_AR_MINOR_VERSION    4
#define DLT_CFG_AR_PATCH_VERSION    0

/* Vendor ID */
#define DLT_CFG_VENDOR_ID           1

/* Module ID */
#define DLT_MODULE_ID               210

/* Configuration Switches */
#define DLT_VERSION_INFO_API        STD_ON
#define DLT_DEV_ERROR_DETECT        STD_ON
#define DLT_USE_COM                 STD_ON
#define DLT_USE_VERBOSE_MODE        STD_ON
#define DLT_USE_LOG_LEVEL_FILTER    STD_ON
#define DLT_USE_TRACE_STATUS        STD_ON
#define DLT_USE_BUFFERING           STD_ON

/* Protocol Settings */
#define DLT_PROTOCOL_VERSION_MAJOR  1
#define DLT_PROTOCOL_VERSION_MINOR  0
#define DLT_MAX_MESSAGE_LENGTH      4096
#define DLT_MAX_CONTEXT_DESCRIPTION 32
#define DLT_MAX_APP_DESCRIPTION     32

/* Buffer Configuration */
#define DLT_BUFFER_COUNT            4
#define DLT_BUFFER_SIZE             2048
#define DLT_MAX_MESSAGE_COUNT       256
#define DLT_BUFFERING_TIMEOUT       1000    /* ms */

/* Context Configuration */
#define DLT_MAX_CONTEXT_COUNT       32
#define DLT_MAX_CONTEXT_ID_LENGTH   4
#define DLT_MAX_APP_ID_LENGTH       4

/* Log Level Configuration */
#define DLT_DEFAULT_LOG_LEVEL       DLT_LOG_INFO
#define DLT_DEFAULT_TRACE_STATUS    DLT_TRACE_STATUS_ON

/* ECU ID Configuration */
#define DLT_ECU_ID                  "ECU1"
#define DLT_ECU_ID_LENGTH           4

/* Session ID Configuration */
#define DLT_DEFAULT_SESSION_ID      0x0001

/* Timing Configuration */
#define DLT_MAIN_FUNCTION_PERIOD    10      /* ms */

/* Error Codes */
#define DLT_E_NO_ERROR              0x00
#define DLT_E_NOT_INITIALIZED       0x01
#define DLT_E_NULL_POINTER          0x02
#define DLT_E_INVALID_PARAMETER     0x03
#define DLT_E_CONTEXT_FULL          0x04
#define DLT_E_CONTEXT_NOT_FOUND     0x05
#define DLT_E_BUFFER_FULL           0x06
#define DLT_E_COM_FAILURE           0x07

/* Service IDs for Development Error Detection */
#define DLT_SID_INIT                0x01
#define DLT_SID_DEINIT              0x02
#define DLT_SID_SEND_LOG_MESSAGE    0x03
#define DLT_SID_SEND_TRACE_MESSAGE  0x04
#define DLT_SID_REGISTER_CONTEXT    0x05
#define DLT_SID_UNREGISTER_CONTEXT  0x06
#define DLT_SID_GET_VERSION_INFO    0x07
#define DLT_SID_SET_LOG_LEVEL       0x08
#define DLT_SID_GET_LOG_LEVEL       0x09
#define DLT_SID_SET_TRACE_STATUS    0x0A
#define DLT_SID_GET_TRACE_STATUS    0x0B
#define DLT_SID_MAIN_FUNCTION       0x80

#endif /* DLT_CFG_H */
