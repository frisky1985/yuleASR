/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/* Dlt.h - AUTOSAR Diagnostic Log and Trace Header */
#ifndef DLT_H
#define DLT_H

#include "Std_Types.h"
#include "Dlt_Cfg.h"

/* AUTOSAR Version Information */
#define DLT_AR_MAJOR_VERSION        4
#define DLT_AR_MINOR_VERSION        4
#define DLT_AR_PATCH_VERSION        0

/* Vendor ID */
#define DLT_VENDOR_ID               1

/* Module ID */
#define DLT_MODULE_ID               210

/* Software Version */
#define DLT_SW_MAJOR_VERSION        1
#define DLT_SW_MINOR_VERSION        0
#define DLT_SW_PATCH_VERSION        0

/* Log Level Types */
typedef enum {
    DLT_LOG_OFF = 0x00,
    DLT_LOG_FATAL = 0x01,
    DLT_LOG_ERROR = 0x02,
    DLT_LOG_WARN = 0x03,
    DLT_LOG_INFO = 0x04,
    DLT_LOG_DEBUG = 0x05,
    DLT_LOG_VERBOSE = 0x06
} Dlt_LogLevelType;

/* Trace Status Types */
typedef enum {
    DLT_TRACE_STATUS_OFF = 0x00,
    DLT_TRACE_STATUS_ON = 0x01
} Dlt_TraceStatusType;

/* Message Type Types */
typedef enum {
    DLT_TYPE_LOG = 0x00,
    DLT_TYPE_APP_TRACE = 0x01,
    DLT_TYPE_NW_TRACE = 0x02,
    DLT_TYPE_CONTROL = 0x03
} Dlt_MessageTypeType;

/* Message Info Types */
typedef enum {
    DLT_INFO_LOG_FATAL = 0x01,
    DLT_INFO_LOG_ERROR = 0x02,
    DLT_INFO_LOG_WARN = 0x03,
    DLT_INFO_LOG_INFO = 0x04,
    DLT_INFO_LOG_DEBUG = 0x05,
    DLT_INFO_LOG_VERBOSE = 0x06,
    DLT_INFO_TRACE_VARIABLE = 0x21,
    DLT_INFO_TRACE_FUNCTION_IN = 0x22,
    DLT_INFO_TRACE_FUNCTION_OUT = 0x23,
    DLT_INFO_TRACE_STATE = 0x24,
    DLT_INFO_TRACE_VFB = 0x25
} Dlt_MessageInfoType;

/* Return Type */
typedef enum {
    DLT_OK = 0,
    DLT_NOT_OK = 1,
    DLT_BUFFER_FULL = 2,
    DLT_FILTERED = 3
} Dlt_ReturnType;

/* Context ID Type */
typedef uint32 Dlt_ContextIdType;

/* Application ID Type */
typedef uint32 Dlt_ApplicationIdType;

/* Session ID Type */
typedef uint32 Dlt_SessionIdType;

/* Message ID Type */
typedef uint32 Dlt_MessageIdType;

/* Context Structure */
typedef struct {
    Dlt_ApplicationIdType appId;
    Dlt_ContextIdType contextId;
    Dlt_LogLevelType logLevel;
    Dlt_TraceStatusType traceStatus;
    uint8 description[DLT_MAX_CONTEXT_DESCRIPTION];
    boolean registered;
} Dlt_ContextType;

/* Message Structure */
typedef struct {
    Dlt_MessageTypeType msgType;
    Dlt_MessageInfoType msgInfo;
    Dlt_ApplicationIdType appId;
    Dlt_ContextIdType contextId;
    uint32 timestamp;
    uint8* payload;
    uint16 payloadLength;
    uint16 bufferIndex;
    boolean buffered;
} Dlt_MessageType;

/* Buffer Structure */
typedef struct {
    uint8 data[DLT_BUFFER_SIZE];
    uint16 writeIndex;
    uint16 readIndex;
    uint16 count;
    boolean locked;
} Dlt_BufferType;

/* DLT Header Structure */
typedef struct {
    uint8 headerType;
    uint8 messageCounter;
    uint16 length;
    uint8 ecuId[DLT_ECU_ID_LENGTH];
    uint32 sessionId;
    uint32 timestamp;
    uint8 appId[DLT_MAX_APP_ID_LENGTH];
    uint8 contextId[DLT_MAX_CONTEXT_ID_LENGTH];
} Dlt_StandardHeaderType;

/* Extended Header Structure */
typedef struct {
    uint8 msin;
    uint8 argCount;
    uint8 appId[DLT_MAX_APP_ID_LENGTH];
    uint8 contextId[DLT_MAX_CONTEXT_ID_LENGTH];
} Dlt_ExtendedHeaderType;

/* Function Prototypes */

/* Initialization and De-initialization */
extern void Dlt_Init(const void* configPtr);
extern void Dlt_DeInit(void);

/* Message Sending */
extern Dlt_ReturnType Dlt_SendLogMessage(
    Dlt_ContextIdType contextId,
    const uint8* payload,
    uint16 payloadLength,
    Dlt_LogLevelType logLevel
);

extern Dlt_ReturnType Dlt_SendTraceMessage(
    Dlt_ContextIdType contextId,
    const uint8* payload,
    uint16 payloadLength,
    Dlt_MessageInfoType traceInfo
);

/* Context Management */
extern Dlt_ReturnType Dlt_RegisterContext(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId,
    const uint8* description,
    uint8 descriptionLength
);

extern Dlt_ReturnType Dlt_UnregisterContext(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId
);

/* Log Level Management */
extern Dlt_ReturnType Dlt_SetLogLevel(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId,
    Dlt_LogLevelType logLevel
);

extern Dlt_ReturnType Dlt_GetLogLevel(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId,
    Dlt_LogLevelType* logLevel
);

/* Trace Status Management */
extern Dlt_ReturnType Dlt_SetTraceStatus(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId,
    Dlt_TraceStatusType traceStatus
);

extern Dlt_ReturnType Dlt_GetTraceStatus(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId,
    Dlt_TraceStatusType* traceStatus
);

/* Version Information */
#if (DLT_VERSION_INFO_API == STD_ON)
extern void Dlt_GetVersionInfo(Std_VersionInfoType* versionInfo);
#endif

/* Main Function */
extern void Dlt_MainFunction(void);

/* Callback Functions for Com Integration */
#if (DLT_USE_COM == STD_ON)
extern void Dlt_ComTxConfirmation(uint8 result);
extern void Dlt_ComRxIndication(const uint8* data, uint16 length);
#endif

#endif /* DLT_H */
