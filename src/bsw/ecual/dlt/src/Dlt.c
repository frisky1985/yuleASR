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

/* Dlt.c - AUTOSAR Diagnostic Log and Trace Implementation */

#include "Dlt.h"
#include "Det.h"
#include "Com.h"
#include <string.h>

/* Module State */
#define DLT_STATE_UNINIT    0x00
#define DLT_STATE_INIT      0x01

static uint8 Dlt_ModuleState = DLT_STATE_UNINIT;
static uint8 Dlt_MessageCounter = 0;
static uint32 Dlt_SessionId = DLT_DEFAULT_SESSION_ID;

/* External Configuration */
extern const Dlt_ContextType Dlt_ContextConfig[DLT_MAX_CONTEXT_COUNT];
extern Dlt_ContextType Dlt_RuntimeContext[DLT_MAX_CONTEXT_COUNT];
extern Dlt_BufferType Dlt_Buffer[DLT_BUFFER_COUNT];

/* Internal Function Prototypes */
static Dlt_ReturnType Dlt_FindContext(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId,
    uint16* contextIndex
);

static Dlt_ReturnType Dlt_BuildStandardHeader(
    Dlt_StandardHeaderType* header,
    uint16 payloadLength
);

static Dlt_ReturnType Dlt_BuildExtendedHeader(
    Dlt_ExtendedHeaderType* header,
    Dlt_MessageTypeType msgType,
    Dlt_MessageInfoType msgInfo,
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId
);

static Dlt_ReturnType Dlt_BufferMessage(
    const uint8* data,
    uint16 length,
    uint16* bufferIndex
);

static Dlt_ReturnType Dlt_TransmitMessage(
    const uint8* data,
    uint16 length
);

static uint32 Dlt_GetTimestamp(void);

static boolean Dlt_IsLogLevelEnabled(
    Dlt_LogLevelType contextLevel,
    Dlt_LogLevelType messageLevel
);

/* Initialization */
void Dlt_Init(const void* configPtr)
{
    uint16 i;
    
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    if (Dlt_ModuleState == DLT_STATE_INIT)
    {
        Det_ReportError(DLT_MODULE_ID, 0, DLT_SID_INIT, DLT_E_NOT_INITIALIZED);
        return;
    }
#endif
    
    (void)configPtr; /* Config pointer not used in this implementation */
    
    /* Initialize runtime contexts from configuration */
    for (i = 0; i < DLT_MAX_CONTEXT_COUNT; i++)
    {
        Dlt_RuntimeContext[i] = Dlt_ContextConfig[i];
    }
    
    /* Initialize buffers */
    for (i = 0; i < DLT_BUFFER_COUNT; i++)
    {
        memset(Dlt_Buffer[i].data, 0, DLT_BUFFER_SIZE);
        Dlt_Buffer[i].writeIndex = 0;
        Dlt_Buffer[i].readIndex = 0;
        Dlt_Buffer[i].count = 0;
        Dlt_Buffer[i].locked = FALSE;
    }
    
    Dlt_MessageCounter = 0;
    Dlt_SessionId = DLT_DEFAULT_SESSION_ID;
    Dlt_ModuleState = DLT_STATE_INIT;
}

/* De-initialization */
void Dlt_DeInit(void)
{
    uint16 i;
    
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    if (Dlt_ModuleState != DLT_STATE_INIT)
    {
        Det_ReportError(DLT_MODULE_ID, 0, DLT_SID_DEINIT, DLT_E_NOT_INITIALIZED);
        return;
    }
#endif
    
    /* Flush any remaining buffered messages */
    for (i = 0; i < DLT_BUFFER_COUNT; i++)
    {
        Dlt_Buffer[i].writeIndex = 0;
        Dlt_Buffer[i].readIndex = 0;
        Dlt_Buffer[i].count = 0;
        Dlt_Buffer[i].locked = FALSE;
    }
    
    Dlt_ModuleState = DLT_STATE_UNINIT;
}

/* Send Log Message */
Dlt_ReturnType Dlt_SendLogMessage(
    Dlt_ContextIdType contextId,
    const uint8* payload,
    uint16 payloadLength,
    Dlt_LogLevelType logLevel)
{
    Dlt_ReturnType result;
    uint16 contextIndex;
    Dlt_ApplicationIdType appId = 0;
    Dlt_StandardHeaderType stdHeader;
    Dlt_ExtendedHeaderType extHeader;
    uint8 messageBuffer[DLT_MAX_MESSAGE_LENGTH];
    uint16 msgOffset = 0;
    uint16 i;
    
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    if (Dlt_ModuleState != DLT_STATE_INIT)
    {
        Det_ReportError(DLT_MODULE_ID, 0, DLT_SID_SEND_LOG_MESSAGE, DLT_E_NOT_INITIALIZED);
        return DLT_NOT_OK;
    }
    
    if (payload == NULL_PTR)
    {
        Det_ReportError(DLT_MODULE_ID, 0, DLT_SID_SEND_LOG_MESSAGE, DLT_E_NULL_POINTER);
        return DLT_NOT_OK;
    }
    
    if (payloadLength > DLT_MAX_MESSAGE_LENGTH)
    {
        Det_ReportError(DLT_MODULE_ID, 0, DLT_SID_SEND_LOG_MESSAGE, DLT_E_INVALID_PARAMETER);
        return DLT_NOT_OK;
    }
#endif
    
    /* Find context */
    for (i = 0; i < DLT_MAX_CONTEXT_COUNT; i++)
    {
        if (Dlt_RuntimeContext[i].registered && 
            (Dlt_RuntimeContext[i].contextId == contextId))
        {
            contextIndex = i;
            appId = Dlt_RuntimeContext[i].appId;
            break;
        }
    }
    
    if (i >= DLT_MAX_CONTEXT_COUNT)
    {
        return DLT_NOT_OK;
    }
    
    /* Check log level filtering */
#if (DLT_USE_LOG_LEVEL_FILTER == STD_ON)
    if (!Dlt_IsLogLevelEnabled(Dlt_RuntimeContext[contextIndex].logLevel, logLevel))
    {
        return DLT_FILTERED;
    }
#endif
    
    /* Build standard header */
    result = Dlt_BuildStandardHeader(&stdHeader, payloadLength + sizeof(Dlt_ExtendedHeaderType));
    if (result != DLT_OK)
    {
        return result;
    }
    
    /* Serialize standard header */
    messageBuffer[msgOffset] = stdHeader.headerType;
    msgOffset++;
    messageBuffer[msgOffset] = stdHeader.messageCounter;
    msgOffset++;
    messageBuffer[msgOffset] = (uint8)(stdHeader.length >> 8);
    msgOffset++;
    messageBuffer[msgOffset] = (uint8)(stdHeader.length);
    msgOffset++;
    
    for (i = 0; i < DLT_ECU_ID_LENGTH; i++)
    {
        messageBuffer[msgOffset] = stdHeader.ecuId[i];
        msgOffset++;
    }
    
    /* Session ID (4 bytes, big endian) */
    messageBuffer[msgOffset] = (uint8)(stdHeader.sessionId >> 24);
    msgOffset++;
    messageBuffer[msgOffset] = (uint8)(stdHeader.sessionId >> 16);
    msgOffset++;
    messageBuffer[msgOffset] = (uint8)(stdHeader.sessionId >> 8);
    msgOffset++;
    messageBuffer[msgOffset] = (uint8)(stdHeader.sessionId);
    msgOffset++;
    
    /* Timestamp (4 bytes, big endian) */
    messageBuffer[msgOffset] = (uint8)(stdHeader.timestamp >> 24);
    msgOffset++;
    messageBuffer[msgOffset] = (uint8)(stdHeader.timestamp >> 16);
    msgOffset++;
    messageBuffer[msgOffset] = (uint8)(stdHeader.timestamp >> 8);
    msgOffset++;
    messageBuffer[msgOffset] = (uint8)(stdHeader.timestamp);
    msgOffset++;
    
    /* Build extended header */
    result = Dlt_BuildExtendedHeader(&extHeader, DLT_TYPE_LOG, (Dlt_MessageInfoType)logLevel, appId, contextId);
    if (result != DLT_OK)
    {
        return result;
    }
    
    /* Serialize extended header */
    messageBuffer[msgOffset] = extHeader.msin;
    msgOffset++;
    messageBuffer[msgOffset] = extHeader.argCount;
    msgOffset++;
    
    for (i = 0; i < DLT_MAX_APP_ID_LENGTH; i++)
    {
        messageBuffer[msgOffset] = extHeader.appId[i];
        msgOffset++;
    }
    
    for (i = 0; i < DLT_MAX_CONTEXT_ID_LENGTH; i++)
    {
        messageBuffer[msgOffset] = extHeader.contextId[i];
        msgOffset++;
    }
    
    /* Copy payload */
    memcpy(&messageBuffer[msgOffset], payload, payloadLength);
    msgOffset += payloadLength;
    
    /* Transmit or buffer message */
#if (DLT_USE_BUFFERING == STD_ON)
    if (Dlt_Buffer[0].count >= DLT_MAX_MESSAGE_COUNT)
    {
        return Dlt_BufferMessage(messageBuffer, msgOffset, NULL_PTR);
    }
#endif
    
    return Dlt_TransmitMessage(messageBuffer, msgOffset);
}

/* Send Trace Message */
Dlt_ReturnType Dlt_SendTraceMessage(
    Dlt_ContextIdType contextId,
    const uint8* payload,
    uint16 payloadLength,
    Dlt_MessageInfoType traceInfo)
{
    Dlt_ReturnType result;
    uint16 contextIndex;
    Dlt_ApplicationIdType appId = 0;
    Dlt_StandardHeaderType stdHeader;
    Dlt_ExtendedHeaderType extHeader;
    uint8 messageBuffer[DLT_MAX_MESSAGE_LENGTH];
    uint16 msgOffset = 0;
    uint16 i;
    
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    if (Dlt_ModuleState != DLT_STATE_INIT)
    {
        Det_ReportError(DLT_MODULE_ID, 0, DLT_SID_SEND_TRACE_MESSAGE, DLT_E_NOT_INITIALIZED);
        return DLT_NOT_OK;
    }
    
    if (payload == NULL_PTR)
    {
        Det_ReportError(DLT_MODULE_ID, 0, DLT_SID_SEND_TRACE_MESSAGE, DLT_E_NULL_POINTER);
        return DLT_NOT_OK;
    }
    
    if (payloadLength > DLT_MAX_MESSAGE_LENGTH)
    {
        Det_ReportError(DLT_MODULE_ID, 0, DLT_SID_SEND_TRACE_MESSAGE, DLT_E_INVALID_PARAMETER);
        return DLT_NOT_OK;
    }
#endif
    
    /* Find context */
    for (i = 0; i < DLT_MAX_CONTEXT_COUNT; i++)
    {
        if (Dlt_RuntimeContext[i].registered && 
            (Dlt_RuntimeContext[i].contextId == contextId))
        {
            contextIndex = i;
            appId = Dlt_RuntimeContext[i].appId;
            break;
        }
    }
    
    if (i >= DLT_MAX_CONTEXT_COUNT)
    {
        return DLT_NOT_OK;
    }
    
    /* Check trace status */
#if (DLT_USE_TRACE_STATUS == STD_ON)
    if (Dlt_RuntimeContext[contextIndex].traceStatus == DLT_TRACE_STATUS_OFF)
    {
        return DLT_FILTERED;
    }
#endif
    
    /* Build standard header */
    result = Dlt_BuildStandardHeader(&stdHeader, payloadLength + sizeof(Dlt_ExtendedHeaderType));
    if (result != DLT_OK)
    {
        return result;
    }
    
    /* Serialize standard header */
    messageBuffer[msgOffset] = stdHeader.headerType;
    msgOffset++;
    messageBuffer[msgOffset] = stdHeader.messageCounter;
    msgOffset++;
    messageBuffer[msgOffset] = (uint8)(stdHeader.length >> 8);
    msgOffset++;
    messageBuffer[msgOffset] = (uint8)(stdHeader.length);
    msgOffset++;
    
    for (i = 0; i < DLT_ECU_ID_LENGTH; i++)
    {
        messageBuffer[msgOffset] = stdHeader.ecuId[i];
        msgOffset++;
    }
    
    /* Session ID */
    messageBuffer[msgOffset] = (uint8)(stdHeader.sessionId >> 24);
    msgOffset++;
    messageBuffer[msgOffset] = (uint8)(stdHeader.sessionId >> 16);
    msgOffset++;
    messageBuffer[msgOffset] = (uint8)(stdHeader.sessionId >> 8);
    msgOffset++;
    messageBuffer[msgOffset] = (uint8)(stdHeader.sessionId);
    msgOffset++;
    
    /* Timestamp */
    messageBuffer[msgOffset] = (uint8)(stdHeader.timestamp >> 24);
    msgOffset++;
    messageBuffer[msgOffset] = (uint8)(stdHeader.timestamp >> 16);
    msgOffset++;
    messageBuffer[msgOffset] = (uint8)(stdHeader.timestamp >> 8);
    msgOffset++;
    messageBuffer[msgOffset] = (uint8)(stdHeader.timestamp);
    msgOffset++;
    
    /* Build extended header */
    result = Dlt_BuildExtendedHeader(&extHeader, DLT_TYPE_APP_TRACE, traceInfo, appId, contextId);
    if (result != DLT_OK)
    {
        return result;
    }
    
    /* Serialize extended header */
    messageBuffer[msgOffset] = extHeader.msin;
    msgOffset++;
    messageBuffer[msgOffset] = extHeader.argCount;
    msgOffset++;
    
    for (i = 0; i < DLT_MAX_APP_ID_LENGTH; i++)
    {
        messageBuffer[msgOffset] = extHeader.appId[i];
        msgOffset++;
    }
    
    for (i = 0; i < DLT_MAX_CONTEXT_ID_LENGTH; i++)
    {
        messageBuffer[msgOffset] = extHeader.contextId[i];
        msgOffset++;
    }
    
    /* Copy payload */
    memcpy(&messageBuffer[msgOffset], payload, payloadLength);
    msgOffset += payloadLength;
    
    /* Transmit message */
    return Dlt_TransmitMessage(messageBuffer, msgOffset);
}

/* Register Context */
Dlt_ReturnType Dlt_RegisterContext(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId,
    const uint8* description,
    uint8 descriptionLength)
{
    uint16 i;
    uint16 freeIndex = DLT_MAX_CONTEXT_COUNT;
    
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    if (Dlt_ModuleState != DLT_STATE_INIT)
    {
        Det_ReportError(DLT_MODULE_ID, 0, DLT_SID_REGISTER_CONTEXT, DLT_E_NOT_INITIALIZED);
        return DLT_NOT_OK;
    }
    
    if (description == NULL_PTR)
    {
        Det_ReportError(DLT_MODULE_ID, 0, DLT_SID_REGISTER_CONTEXT, DLT_E_NULL_POINTER);
        return DLT_NOT_OK;
    }
    
    if (descriptionLength > DLT_MAX_CONTEXT_DESCRIPTION)
    {
        Det_ReportError(DLT_MODULE_ID, 0, DLT_SID_REGISTER_CONTEXT, DLT_E_INVALID_PARAMETER);
        return DLT_NOT_OK;
    }
#endif
    
    /* Check if context already exists */
    for (i = 0; i < DLT_MAX_CONTEXT_COUNT; i++)
    {
        if (Dlt_RuntimeContext[i].registered &&
            ((Dlt_RuntimeContext[i].appId == appId)) &&
            (Dlt_RuntimeContext[i].contextId == contextId))
        {
            /* Context already registered */
            return DLT_OK;
        }
        
        if (!Dlt_RuntimeContext[i].registered && (freeIndex == DLT_MAX_CONTEXT_COUNT))
        {
            freeIndex = i;
        }
    }
    
    if (freeIndex >= DLT_MAX_CONTEXT_COUNT)
    {
#if (DLT_DEV_ERROR_DETECT == STD_ON)
        Det_ReportError(DLT_MODULE_ID, 0, DLT_SID_REGISTER_CONTEXT, DLT_E_CONTEXT_FULL);
#endif
        return DLT_NOT_OK;
    }
    
    /* Register new context */
    Dlt_RuntimeContext[freeIndex].appId = appId;
    Dlt_RuntimeContext[freeIndex].contextId = contextId;
    Dlt_RuntimeContext[freeIndex].logLevel = DLT_DEFAULT_LOG_LEVEL;
    Dlt_RuntimeContext[freeIndex].traceStatus = DLT_DEFAULT_TRACE_STATUS;
    Dlt_RuntimeContext[freeIndex].registered = TRUE;
    
    /* Copy description */
    if (descriptionLength > 0U )
    {
        memcpy(Dlt_RuntimeContext[freeIndex].description, description, descriptionLength);
    }
    
    return DLT_OK;
}

/* Unregister Context */
Dlt_ReturnType Dlt_UnregisterContext(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId)
{
    uint16 i;
    
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    if (Dlt_ModuleState != DLT_STATE_INIT)
    {
        Det_ReportError(DLT_MODULE_ID, 0, DLT_SID_UNREGISTER_CONTEXT, DLT_E_NOT_INITIALIZED);
        return DLT_NOT_OK;
    }
#endif
    
    /* Find and unregister context */
    for (i = 0; i < DLT_MAX_CONTEXT_COUNT; i++)
    {
        if (Dlt_RuntimeContext[i].registered &&
            ((Dlt_RuntimeContext[i].appId == appId)) &&
            (Dlt_RuntimeContext[i].contextId == contextId))
        {
            Dlt_RuntimeContext[i].registered = FALSE;
            Dlt_RuntimeContext[i].appId = 0;
            Dlt_RuntimeContext[i].contextId = 0;
            memset(Dlt_RuntimeContext[i].description, 0, DLT_MAX_CONTEXT_DESCRIPTION);
            return DLT_OK;
        }
    }
    
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    Det_ReportError(DLT_MODULE_ID, 0, DLT_SID_UNREGISTER_CONTEXT, DLT_E_CONTEXT_NOT_FOUND);
#endif
    return DLT_NOT_OK;
}

/* Set Log Level */
Dlt_ReturnType Dlt_SetLogLevel(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId,
    Dlt_LogLevelType logLevel)
{
    uint16 i;
    
    for (i = 0; i < DLT_MAX_CONTEXT_COUNT; i++)
    {
        if (Dlt_RuntimeContext[i].registered &&
            ((Dlt_RuntimeContext[i].appId == appId)) &&
            (Dlt_RuntimeContext[i].contextId == contextId))
        {
            Dlt_RuntimeContext[i].logLevel = logLevel;
            return DLT_OK;
        }
    }
    
    return DLT_NOT_OK;
}

/* Get Log Level */
Dlt_ReturnType Dlt_GetLogLevel(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId,
    Dlt_LogLevelType* logLevel)
{
    uint16 i;
    
    if (logLevel == NULL_PTR)
    {
        return DLT_NOT_OK;
    }
    
    for (i = 0; i < DLT_MAX_CONTEXT_COUNT; i++)
    {
        if (Dlt_RuntimeContext[i].registered &&
            ((Dlt_RuntimeContext[i].appId == appId)) &&
            (Dlt_RuntimeContext[i].contextId == contextId))
        {
            *logLevel = Dlt_RuntimeContext[i].logLevel;
            return DLT_OK;
        }
    }
    
    return DLT_NOT_OK;
}

/* Set Trace Status */
Dlt_ReturnType Dlt_SetTraceStatus(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId,
    Dlt_TraceStatusType traceStatus)
{
    uint16 i;
    
    for (i = 0; i < DLT_MAX_CONTEXT_COUNT; i++)
    {
        if (Dlt_RuntimeContext[i].registered &&
            ((Dlt_RuntimeContext[i].appId == appId)) &&
            (Dlt_RuntimeContext[i].contextId == contextId))
        {
            Dlt_RuntimeContext[i].traceStatus = traceStatus;
            return DLT_OK;
        }
    }
    
    return DLT_NOT_OK;
}

/* Get Trace Status */
Dlt_ReturnType Dlt_GetTraceStatus(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId,
    Dlt_TraceStatusType* traceStatus)
{
    uint16 i;
    
    if (traceStatus == NULL_PTR)
    {
        return DLT_NOT_OK;
    }
    
    for (i = 0; i < DLT_MAX_CONTEXT_COUNT; i++)
    {
        if (Dlt_RuntimeContext[i].registered &&
            ((Dlt_RuntimeContext[i].appId == appId)) &&
            (Dlt_RuntimeContext[i].contextId == contextId))
        {
            *traceStatus = Dlt_RuntimeContext[i].traceStatus;
            return DLT_OK;
        }
    }
    
    return DLT_NOT_OK;
}

/* Get Version Info */
#if (DLT_VERSION_INFO_API == STD_ON)
void Dlt_GetVersionInfo(Std_VersionInfoType* versionInfo)
{
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    if (versionInfo == NULL_PTR)
    {
        Det_ReportError(DLT_MODULE_ID, 0, DLT_SID_GET_VERSION_INFO, DLT_E_NULL_POINTER);
        return;
    }
#endif
    
    versionInfo->vendorID = DLT_VENDOR_ID;
    versionInfo->moduleID = DLT_MODULE_ID;
    versionInfo->sw_major_version = DLT_SW_MAJOR_VERSION;
    versionInfo->sw_minor_version = DLT_SW_MINOR_VERSION;
    versionInfo->sw_patch_version = DLT_SW_PATCH_VERSION;
}
#endif

/* Main Function */
void Dlt_MainFunction(void)
{
    uint16 i;
    
#if (DLT_DEV_ERROR_DETECT == STD_ON)
    if (Dlt_ModuleState != DLT_STATE_INIT)
    {
        Det_ReportError(DLT_MODULE_ID, 0, DLT_SID_MAIN_FUNCTION, DLT_E_NOT_INITIALIZED);
        return;
    }
#endif
    
    /* Process buffered messages */
#if (DLT_USE_BUFFERING == STD_ON)
    for (i = 0; i < DLT_BUFFER_COUNT; i++)
    {
        if (Dlt_Buffer[i].count > 0U && !Dlt_Buffer[i].locked)
        {
            /* Transmit buffered messages */
            uint16 length = Dlt_Buffer[i].writeIndex - Dlt_Buffer[i].readIndex;
            if (length > 0U )
            {
                Dlt_TransmitMessage(&Dlt_Buffer[i].data[Dlt_Buffer[i].readIndex], length);
                Dlt_Buffer[i].readIndex = Dlt_Buffer[i].writeIndex;
                Dlt_Buffer[i].count = 0;
            }
        }
    }
#endif
}

/* Com Tx Confirmation */
#if (DLT_USE_COM == STD_ON)
void Dlt_ComTxConfirmation(uint8 result)
{
    (void)result;
    /* Handle transmission confirmation */
}

/* Com Rx Indication */
void Dlt_ComRxIndication(const uint8* data, uint16 length)
{
    (void)data;
    (void)length;
    /* Handle received DLT control messages */
}
#endif

/* Internal Functions */

static Dlt_ReturnType Dlt_FindContext(
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId,
    uint16* contextIndex)
{
    uint16 i;
    
    for (i = 0; i < DLT_MAX_CONTEXT_COUNT; i++)
    {
        if (Dlt_RuntimeContext[i].registered &&
            ((Dlt_RuntimeContext[i].appId == appId)) &&
            (Dlt_RuntimeContext[i].contextId == contextId))
        {
            *contextIndex = i;
            return DLT_OK;
        }
    }
    
    return DLT_NOT_OK;
}

static Dlt_ReturnType Dlt_BuildStandardHeader(
    Dlt_StandardHeaderType* header,
    uint16 payloadLength)
{
    header->headerType = 0x3D; /* Use extended header, with ECU ID, Session ID, and Timestamp */
    header->messageCounter = Dlt_MessageCounter;
    Dlt_MessageCounter++;
    header->length = sizeof(Dlt_StandardHeaderType) + payloadLength;
    
    memcpy(header->ecuId, DLT_ECU_ID, DLT_ECU_ID_LENGTH);
    header->sessionId = Dlt_SessionId;
    header->timestamp = Dlt_GetTimestamp();
    
    return DLT_OK;
}

static Dlt_ReturnType Dlt_BuildExtendedHeader(
    Dlt_ExtendedHeaderType* header,
    Dlt_MessageTypeType msgType,
    Dlt_MessageInfoType msgInfo,
    Dlt_ApplicationIdType appId,
    Dlt_ContextIdType contextId)
{
    header->msin = (uint8)((msgType << 4) | msgInfo);
    header->argCount = 0; /* Variable arguments not supported in this implementation */
    
    /* Convert IDs to ASCII representation */
    header->appId[0] = (uint8)((appId >> 24) & 0xFF);
    header->appId[1] = (uint8)((appId >> 16) & 0xFF);
    header->appId[2] = (uint8)((appId >> 8) & 0xFF);
    header->appId[3] = (uint8)(appId & 0xFF);
    
    header->contextId[0] = (uint8)((contextId >> 24) & 0xFFU);
    header->contextId[1] = (uint8)((contextId >> 16) & 0xFFU);
    header->contextId[2] = (uint8)((contextId >> 8) & 0xFFU);
    header->contextId[3] = (uint8)(contextId & 0xFFU);
    
    return DLT_OK;
}

static Dlt_ReturnType Dlt_BufferMessage(
    const uint8* data,
    uint16 length,
    uint16* bufferIndex)
{
#if (DLT_USE_BUFFERING == STD_ON)
    uint16 i;
    
    for (i = 0; i < DLT_BUFFER_COUNT; i++)
    {
        if (!Dlt_Buffer[i].locked)
        {
            if ((DLT_BUFFER_SIZE - Dlt_Buffer[i].writeIndex) >= length)
            {
                memcpy(&Dlt_Buffer[i].data[Dlt_Buffer[i].writeIndex], data, length);
                Dlt_Buffer[i].writeIndex += length;
                Dlt_Buffer[i].count++;
                
                if (bufferIndex != NULL_PTR)
                {
                    *bufferIndex = i;
                }
                return DLT_OK;
            }
        }
    }
    
    return DLT_BUFFER_FULL;
#else
    (void)data;
    (void)length;
    (void)bufferIndex;
    return DLT_NOT_OK;
#endif
}

static Dlt_ReturnType Dlt_TransmitMessage(
    const uint8* data,
    uint16 length)
{
#if (DLT_USE_COM == STD_ON)
    /* Transmit via Com module */
    Std_ReturnType comResult = Com_SendSignal(0, (uint8*)data);
    if (comResult != E_OK)
    {
        return DLT_NOT_OK;
    }
    return DLT_OK;
#else
    (void)data;
    (void)length;
    /* No Com module available, message would be discarded or logged locally */
    return DLT_OK;
#endif
}

static uint32 Dlt_GetTimestamp(void)
{
    /* Return system tick count */
    /* This should be replaced with actual OS timer */
    static uint32 timestamp = 0;
    return timestamp++;
}

static boolean Dlt_IsLogLevelEnabled(
    Dlt_LogLevelType contextLevel,
    Dlt_LogLevelType messageLevel)
{
    /* Return TRUE if message level is <= context level (lower number = higher priority) */
    /* DLT_LOG_OFF (0) blocks all, DLT_LOG_VERBOSE (6) allows all */
    if (contextLevel == DLT_LOG_OFF)
    {
        return FALSE;
    }
    
    return (messageLevel <= contextLevel);
}