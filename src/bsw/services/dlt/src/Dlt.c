/**
 * @file Dlt.c
 * @brief Diagnostic Log and Trace module implementation
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Diagnostic Log and Trace (DLT)
 * Layer: Service Layer
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Dlt.h"
#include "Det.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/*==================================================================================================
*                                      LOCAL DEFINES
==================================================================================================*/
#define DLT_STATE_UNINIT                    (0x00U)
#define DLT_STATE_INIT                      (0x01U)
#define DLT_STATE_ACTIVE                    (0x02U)

#define DLT_HEADER_SIZE_MIN                 (4U)
#define DLT_HEADER_SIZE_EXT                 (16U)
#define DLT_HEADER_SIZE_FULL                (32U)

#define DLT_HTYP_UEH                        (0x01U) /* Use Extended Header */
#define DLT_HTYP_MSBF                       (0x02U) /* MSB First */
#define DLT_HTYP_WEID                       (0x04U) /* With ECU ID */
#define DLT_HTYP_WSID                       (0x08U) /* With Session ID */
#define DLT_HTYP_WTMS                       (0x10U) /* With Timestamp */
#define DLT_HTYP_VERS                       (0x20U) /* Version */

#define DLT_MSIN_VERB                       (0x01U) /* Verbose */
#define DLT_MSIN_MTYP_LOG                   (0x00U) /* Log Message */
#define DLT_MSIN_MTYP_TRACE                 (0x02U) /* Trace Message */
#define DLT_MSIN_MTYP_CONTROL               (0x04U) /* Control Message */

#define DLT_MTIN_LOG_OFF                    (0x00U)
#define DLT_MTIN_LOG_FATAL                  (0x01U)
#define DLT_MTIN_LOG_ERROR                  (0x02U)
#define DLT_MTIN_LOG_WARN                   (0x03U)
#define DLT_MTIN_LOG_INFO                   (0x04U)
#define DLT_MTIN_LOG_DEBUG                  (0x05U)
#define DLT_MTIN_LOG_VERBOSE                (0x06U)

#define DLT_INVALID_CONTEXT_INDEX           (0xFFU)

/*==================================================================================================
*                                      LOCAL TYPEDEFS
==================================================================================================*/
typedef struct {
    uint8 state;
    Dlt_RingBufferType ringBuffer;
    Dlt_ContextType contexts[DLT_MAX_CONTEXTS];
    uint8 numContexts;
    Dlt_OutputModeType outputMode;
    Dlt_SerialOutputCbkType serialCbk;
    Dlt_NetworkOutputCbkType networkCbk;
    Dlt_GetTimestampCbkType timestampCbk;
    uint32 sessionId;
    uint8 messageCounter;
    Dlt_EcuIdType ecuId;
    boolean overflowOccurred;
} Dlt_InternalType;

/*==================================================================================================
*                                      LOCAL VARIABLES
==================================================================================================*/
#define DLT_START_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

static Dlt_InternalType Dlt_Internal = {
    .state = DLT_STATE_UNINIT,
    .ringBuffer = {{0}},
    .contexts = {{0}},
    .numContexts = 0U,
    .outputMode = DLT_DEFAULT_OUTPUT_MODE,
    .serialCbk = NULL_PTR,
    .networkCbk = NULL_PTR,
    .timestampCbk = NULL_PTR,
    .sessionId = DLT_SESSION_ID,
    .messageCounter = 0U,
    .ecuId = DLT_ECU_ID,
    .overflowOccurred = FALSE
};

static const Dlt_ConfigType Dlt_DefaultConfig = {
    .outputMode = DLT_DEFAULT_OUTPUT_MODE,
    .defaultLogLevel = DLT_DEFAULT_LOG_LEVEL,
    .timestampEnabled = DLT_USE_TIMESTAMP,
    .ecuIdEnabled = DLT_USE_ECU_ID,
    .sessionIdEnabled = DLT_USE_SESSION_ID,
    .devErrorDetect = DLT_DEV_ERROR_DETECT,
    .versionInfoApi = DLT_VERSION_INFO_API,
    .networkPort = DLT_DEFAULT_NETWORK_PORT,
    .ecuId = DLT_ECU_ID
};

#define DLT_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                      LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
static uint8 Dlt_FindContextIndex(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId
);

static uint8 Dlt_AllocateContext(void);

static Dlt_ReturnType Dlt_BuildHeader(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    Dlt_MessageType msgType,
    uint8 msgInfo,
    uint16 payloadLength,
    uint8* header,
    uint16* headerLength
);

static Dlt_ReturnType Dlt_BuildLogPayload(
    Dlt_LogLevelType logLevel,
    const char* message,
    uint8* payload,
    uint16* payloadLength
);

static Dlt_ReturnType Dlt_BuildFormattedPayload(
    Dlt_LogLevelType logLevel,
    const char* format,
    va_list args,
    uint8* payload,
    uint16* payloadLength
);

static Dlt_ReturnType Dlt_BuildTracePayload(
    Dlt_TraceType traceType,
    uint32 traceInfo,
    const char* variableName,
    sint32 variableValue,
    uint8* payload,
    uint16* payloadLength
);

static Dlt_ReturnType Dlt_RingBufferWrite(
    const uint8* data,
    uint16 length
);

static Dlt_ReturnType Dlt_RingBufferRead(
    uint8* data,
    uint16* length
);

static Dlt_ReturnType Dlt_SendToOutputs(
    const uint8* data,
    uint16 length
);

static Dlt_TimestampType Dlt_GetTimestamp(void);

static boolean Dlt_ShouldLog(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    Dlt_LogLevelType logLevel
);

static uint8 Dlt_ConvertLogLevel(Dlt_LogLevelType logLevel);

/*==================================================================================================
*                                      LOCAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Finds context index by application ID and context ID
 */
static uint8 Dlt_FindContextIndex(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId
)
{
    uint8 i;
    
    for (i = 0U; i < Dlt_Internal.numContexts; i++) {
        if ((Dlt_Internal.contexts[i].registered == TRUE) &&
            (strncmp((const char*)Dlt_Internal.contexts[i].appId, 
                     (const char*)appId, 4) == 0) &&
            (strncmp((const char*)Dlt_Internal.contexts[i].contextId, 
                     (const char*)contextId, 4) == 0)) {
            return i;
        }
    }
    
    return DLT_INVALID_CONTEXT_INDEX;
}

/**
 * @brief Allocates a new context slot
 */
static uint8 Dlt_AllocateContext(void)
{
    uint8 i;
    
    if (Dlt_Internal.numContexts >= DLT_MAX_CONTEXTS) {
        return DLT_INVALID_CONTEXT_INDEX;
    }
    
    for (i = 0U; i < DLT_MAX_CONTEXTS; i++) {
        if (Dlt_Internal.contexts[i].registered == FALSE) {
            return i;
        }
    }
    
    return DLT_INVALID_CONTEXT_INDEX;
}

/**
 * @brief Builds DLT message header
 */
static Dlt_ReturnType Dlt_BuildHeader(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    Dlt_MessageType msgType,
    uint8 msgInfo,
    uint16 payloadLength,
    uint8* header,
    uint16* headerLength
)
{
    uint16 idx = 0U;
    uint8 htyp = 0U;
    uint16 totalLength;
    Dlt_TimestampType timestamp;
    
    if ((header == NULL_PTR) || (headerLength == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    /* Calculate header type */
    htyp = DLT_HTYP_VERS; /* Version 1 */
    
    #if (DLT_USE_EXTENDED_HEADER == STD_ON)
    htyp |= DLT_HTYP_UEH;
    #endif
    
    #if (DLT_USE_ECU_ID == STD_ON)
    htyp |= DLT_HTYP_WEID;
    #endif
    
    #if (DLT_USE_SESSION_ID == STD_ON)
    htyp |= DLT_HTYP_WSID;
    #endif
    
    #if (DLT_USE_TIMESTAMP == STD_ON)
    htyp |= DLT_HTYP_WTMS;
    #endif
    
    /* Header Type */
    header[idx++] = htyp;
    
    /* Message Counter */
    header[idx++] = Dlt_Internal.messageCounter++;
    
    /* Calculate total length (header + payload) */
    totalLength = payloadLength;
    #if (DLT_USE_EXTENDED_HEADER == STD_ON)
    totalLength += 10U; /* Extended header size */
    #endif
    #if (DLT_USE_ECU_ID == STD_ON)
    totalLength += 4U;
    #endif
    #if (DLT_USE_SESSION_ID == STD_ON)
    totalLength += 4U;
    #endif
    #if (DLT_USE_TIMESTAMP == STD_ON)
    totalLength += 4U;
    #endif
    totalLength += 4U; /* Standard header minimum */
    
    /* Length (2 bytes, big endian) */
    header[idx++] = (uint8)((totalLength >> 8) & 0xFFU);
    header[idx++] = (uint8)(totalLength & 0xFFU);
    
    /* ECU ID (optional) */
    #if (DLT_USE_ECU_ID == STD_ON)
    memcpy(&header[idx], Dlt_Internal.ecuId, 4);
    idx += 4U;
    #endif
    
    /* Session ID (optional) */
    #if (DLT_USE_SESSION_ID == STD_ON)
    header[idx++] = (uint8)((Dlt_Internal.sessionId >> 24) & 0xFFU);
    header[idx++] = (uint8)((Dlt_Internal.sessionId >> 16) & 0xFFU);
    header[idx++] = (uint8)((Dlt_Internal.sessionId >> 8) & 0xFFU);
    header[idx++] = (uint8)(Dlt_Internal.sessionId & 0xFFU);
    #endif
    
    /* Timestamp (optional) */
    #if (DLT_USE_TIMESTAMP == STD_ON)
    timestamp = Dlt_GetTimestamp();
    header[idx++] = (uint8)((timestamp >> 24) & 0xFFU);
    header[idx++] = (uint8)((timestamp >> 16) & 0xFFU);
    header[idx++] = (uint8)((timestamp >> 8) & 0xFFU);
    header[idx++] = (uint8)(timestamp & 0xFFU);
    #endif
    
    /* Extended Header (optional) */
    #if (DLT_USE_EXTENDED_HEADER == STD_ON)
    /* MSIN (Message Info) */
    header[idx++] = msgInfo;
    /* Number of Arguments (2 bytes) */
    header[idx++] = 0x00U;
    header[idx++] = 0x00U;
    /* Application ID */
    memcpy(&header[idx], appId, 4);
    idx += 4U;
    /* Context ID */
    memcpy(&header[idx], contextId, 4);
    idx += 4U;
    #endif
    
    *headerLength = idx;
    
    return E_OK;
}

/**
 * @brief Builds log message payload
 */
static Dlt_ReturnType Dlt_BuildLogPayload(
    Dlt_LogLevelType logLevel,
    const char* message,
    uint8* payload,
    uint16* payloadLength
)
{
    uint16 msgLen;
    uint16 idx = 0U;
    
    if ((message == NULL_PTR) || (payload == NULL_PTR) || (payloadLength == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    msgLen = (uint16)strlen(message);
    if (msgLen > DLT_MAX_MESSAGE_LENGTH) {
        msgLen = DLT_MAX_MESSAGE_LENGTH;
    }
    
    /* Type Info for String (8 bytes) */
    payload[idx++] = 0x00U;
    payload[idx++] = 0x00U;
    payload[idx++] = 0x00U;
    payload[idx++] = 0x02U; /* Type String */
    payload[idx++] = 0x00U;
    payload[idx++] = 0x00U;
    
    /* String length (2 bytes) */
    payload[idx++] = (uint8)((msgLen >> 8) & 0xFFU);
    payload[idx++] = (uint8)(msgLen & 0xFFU);
    
    /* String data */
    memcpy(&payload[idx], message, msgLen);
    idx += msgLen;
    
    *payloadLength = idx;
    
    return E_OK;
}

/**
 * @brief Builds formatted log message payload
 */
static Dlt_ReturnType Dlt_BuildFormattedPayload(
    Dlt_LogLevelType logLevel,
    const char* format,
    va_list args,
    uint8* payload,
    uint16* payloadLength
)
{
    sint32 result;
    uint16 formattedLen;
    
    if ((format == NULL_PTR) || (payload == NULL_PTR) || (payloadLength == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    /* Format the string - Use safe vsnprintf for all compilers */
    result = vsnprintf((char*)&payload[8], DLT_MAX_MESSAGE_LENGTH, format, args);
    
    if (result < 0) {
        return E_NOT_OK;
    }
    
    formattedLen = (uint16)result;
    if (formattedLen > DLT_MAX_MESSAGE_LENGTH) {
        formattedLen = DLT_MAX_MESSAGE_LENGTH;
    }
    
    /* Type Info for String */
    payload[0] = 0x00U;
    payload[1] = 0x00U;
    payload[2] = 0x00U;
    payload[3] = 0x02U; /* Type String */
    payload[4] = 0x00U;
    payload[5] = 0x00U;
    
    /* String length (2 bytes) */
    payload[6] = (uint8)((formattedLen >> 8) & 0xFFU);
    payload[7] = (uint8)(formattedLen & 0xFFU);
    
    *payloadLength = 8U + formattedLen;
    
    return E_OK;
}

/**
 * @brief Builds trace payload
 */
static Dlt_ReturnType Dlt_BuildTracePayload(
    Dlt_TraceType traceType,
    uint32 traceInfo,
    const char* variableName,
    sint32 variableValue,
    uint8* payload,
    uint16* payloadLength
)
{
    uint16 idx = 0U;
    uint16 nameLen;
    
    if (payload == NULL_PTR || payloadLength == NULL_PTR) {
        return E_NOT_OK;
    }
    
    /* Type Info for Trace Type */
    payload[idx++] = 0x00U;
    payload[idx++] = 0x00U;
    payload[idx++] = 0x00U;
    payload[idx++] = traceType;
    
    /* Trace Info (4 bytes) */
    payload[idx++] = (uint8)((traceInfo >> 24) & 0xFFU);
    payload[idx++] = (uint8)((traceInfo >> 16) & 0xFFU);
    payload[idx++] = (uint8)((traceInfo >> 8) & 0xFFU);
    payload[idx++] = (uint8)(traceInfo & 0xFFU);
    
    /* Variable Name (if provided) */
    if (variableName != NULL_PTR) {
        nameLen = (uint16)strlen(variableName);
        if (nameLen > 16U) {
            nameLen = 16U;
        }
        
        payload[idx++] = (uint8)((nameLen >> 8) & 0xFFU);
        payload[idx++] = (uint8)(nameLen & 0xFFU);
        
        memcpy(&payload[idx], variableName, nameLen);
        idx += nameLen;
        
        /* Variable Value */
        payload[idx++] = (uint8)((variableValue >> 24) & 0xFFU);
        payload[idx++] = (uint8)((variableValue >> 16) & 0xFFU);
        payload[idx++] = (uint8)((variableValue >> 8) & 0xFFU);
        payload[idx++] = (uint8)(variableValue & 0xFFU);
    }
    
    *payloadLength = idx;
    
    return E_OK;
}

/**
 * @brief Writes data to ring buffer
 */
static Dlt_ReturnType Dlt_RingBufferWrite(
    const uint8* data,
    uint16 length
)
{
    Dlt_RingBufferType* rb = &Dlt_Internal.ringBuffer;
    Dlt_BufferEntryType* entry;
    
    if ((data == NULL_PTR) || (length == 0U) || (length > DLT_BUFFER_ENTRY_SIZE)) {
        return E_NOT_OK;
    }
    
    /* Check if buffer is full */
    if (rb->count >= DLT_RING_BUFFER_SIZE) {
        Dlt_Internal.overflowOccurred = TRUE;
        
        #if (DLT_DROP_ON_OVERFLOW == STD_ON)
        return E_NOT_OK; /* Drop message */
        #else
        /* Wait for space - non-blocking mode should return immediately */
        return E_NOT_OK;
        #endif
    }
    
    /* Get entry at head */
    entry = &rb->entries[rb->head];
    
    /* Copy data */
    memcpy(entry->data, data, length);
    entry->length = length;
    entry->used = TRUE;
    
    /* Advance head */
    rb->head = (rb->head + 1U) % DLT_RING_BUFFER_SIZE;
    rb->count++;
    
    return E_OK;
}

/**
 * @brief Reads data from ring buffer
 */
static Dlt_ReturnType Dlt_RingBufferRead(
    uint8* data,
    uint16* length
)
{
    Dlt_RingBufferType* rb = &Dlt_Internal.ringBuffer;
    Dlt_BufferEntryType* entry;
    
    if ((data == NULL_PTR) || (length == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    /* Check if buffer is empty */
    if (rb->count == 0U) {
        *length = 0U;
        return E_NOT_OK;
    }
    
    /* Get entry at tail */
    entry = &rb->entries[rb->tail];
    
    if (entry->used == FALSE) {
        *length = 0U;
        return E_NOT_OK;
    }
    
    /* Copy data */
    memcpy(data, entry->data, entry->length);
    *length = entry->length;
    
    /* Mark as unused */
    entry->used = FALSE;
    
    /* Advance tail */
    rb->tail = (rb->tail + 1U) % DLT_RING_BUFFER_SIZE;
    rb->count--;
    
    return E_OK;
}

/**
 * @brief Sends data to configured outputs
 */
static Dlt_ReturnType Dlt_SendToOutputs(
    const uint8* data,
    uint16 length
)
{
    Dlt_ReturnType result = E_OK;
    
    if ((data == NULL_PTR) || (length == 0U)) {
        return E_NOT_OK;
    }
    
    /* Serial output */
    #if (DLT_SERIAL_OUTPUT_ENABLED == STD_ON)
    if (((Dlt_Internal.outputMode == DLT_OUTPUT_MODE_SERIAL) ||
         (Dlt_Internal.outputMode == DLT_OUTPUT_MODE_BOTH)) &&
        (Dlt_Internal.serialCbk != NULL_PTR)) {
        result = Dlt_Internal.serialCbk(data, length);
    }
    #endif
    
    /* Network output */
    #if (DLT_NETWORK_OUTPUT_ENABLED == STD_ON)
    if (((Dlt_Internal.outputMode == DLT_OUTPUT_MODE_NETWORK) ||
         (Dlt_Internal.outputMode == DLT_OUTPUT_MODE_BOTH)) &&
        (Dlt_Internal.networkCbk != NULL_PTR)) {
        result = Dlt_Internal.networkCbk(data, length);
    }
    #endif
    
    /* Buffer output (always write to buffer) */
    #if (DLT_BUFFER_OUTPUT_ENABLED == STD_ON)
    (void)Dlt_RingBufferWrite(data, length);
    #endif
    
    return result;
}

/**
 * @brief Gets current timestamp
 */
static Dlt_TimestampType Dlt_GetTimestamp(void)
{
    Dlt_TimestampType timestamp = 0U;
    
    #if (DLT_USER_TIMESTAMP_CALLBACK == STD_ON)
    if (Dlt_Internal.timestampCbk != NULL_PTR) {
        timestamp = Dlt_Internal.timestampCbk();
    }
    #else
    /* Default implementation - return counter value */
    static Dlt_TimestampType counter = 0U;
    timestamp = counter++;
    #endif
    
    return timestamp;
}

/**
 * @brief Checks if message should be logged based on level
 */
static boolean Dlt_ShouldLog(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    Dlt_LogLevelType logLevel
)
{
    uint8 ctxIdx;
    Dlt_LogLevelType ctxLogLevel;
    
    #if (DLT_LOG_LEVEL_FILTER_ENABLED == STD_OFF)
    (void)appId;
    (void)contextId;
    (void)logLevel;
    return TRUE;
    #else
    
    /* Find context */
    ctxIdx = Dlt_FindContextIndex(appId, contextId);
    
    if (ctxIdx != DLT_INVALID_CONTEXT_INDEX) {
        ctxLogLevel = Dlt_Internal.contexts[ctxIdx].logLevel;
    } else {
        ctxLogLevel = Dlt_DefaultConfig.defaultLogLevel;
    }
    
    /* Check log level (lower number = higher priority) */
    /* DLT_LOG_VERBOSE (1) < DLT_LOG_DEBUG (2) < ... < DLT_LOG_FATAL (6) */
    return (logLevel >= ctxLogLevel);
    #endif
}

/**
 * @brief Converts internal log level to DLT protocol log level
 */
static uint8 Dlt_ConvertLogLevel(Dlt_LogLevelType logLevel)
{
    switch (logLevel) {
        case DLT_LOG_OFF:
            return DLT_MTIN_LOG_OFF;
        case DLT_LOG_FATAL:
            return DLT_MTIN_LOG_FATAL;
        case DLT_LOG_ERROR:
            return DLT_MTIN_LOG_ERROR;
        case DLT_LOG_WARN:
            return DLT_MTIN_LOG_WARN;
        case DLT_LOG_INFO:
            return DLT_MTIN_LOG_INFO;
        case DLT_LOG_DEBUG:
            return DLT_MTIN_LOG_DEBUG;
        case DLT_LOG_VERBOSE:
            return DLT_MTIN_LOG_VERBOSE;
        default:
            return DLT_MTIN_LOG_DEBUG;
    }
}

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initializes the DLT module
 */
void Dlt_Init(const Dlt_ConfigType* ConfigPtr)
{
    uint8 i;
    
    /* Check if already initialized */
    if (Dlt_Internal.state != DLT_STATE_UNINIT) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_INIT, DLT_E_INIT_FAILED);
        #endif
        return;
    }
    
    /* Use default config if NULL */
    if (ConfigPtr == NULL_PTR) {
        ConfigPtr = &Dlt_DefaultConfig;
    }
    
    /* Initialize contexts */
    for (i = 0U; i < DLT_MAX_CONTEXTS; i++) {
        Dlt_Internal.contexts[i].registered = FALSE;
        Dlt_Internal.contexts[i].logLevel = ConfigPtr->defaultLogLevel;
        Dlt_Internal.contexts[i].traceStatus = TRUE;
        Dlt_Internal.contexts[i].messageCount = 0U;
    }
    
    Dlt_Internal.numContexts = 0U;
    
    /* Initialize ring buffer */
    for (i = 0U; i < DLT_RING_BUFFER_SIZE; i++) {
        Dlt_Internal.ringBuffer.entries[i].used = FALSE;
        Dlt_Internal.ringBuffer.entries[i].length = 0U;
    }
    Dlt_Internal.ringBuffer.head = 0U;
    Dlt_Internal.ringBuffer.tail = 0U;
    Dlt_Internal.ringBuffer.count = 0U;
    Dlt_Internal.ringBuffer.overflow = FALSE;
    
    /* Initialize configuration */
    Dlt_Internal.outputMode = ConfigPtr->outputMode;
    Dlt_Internal.sessionId = DLT_SESSION_ID;
    Dlt_Internal.messageCounter = 0U;
    memcpy(Dlt_Internal.ecuId, ConfigPtr->ecuId, 4);
    Dlt_Internal.overflowOccurred = FALSE;
    
    /* Set state to initialized */
    Dlt_Internal.state = DLT_STATE_INIT;
}

/**
 * @brief Deinitializes the DLT module
 */
void Dlt_DeInit(void)
{
    /* Check if initialized */
    if (Dlt_Internal.state == DLT_STATE_UNINIT) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_DEINIT, DLT_E_UNINIT);
        #endif
        return;
    }
    
    /* Flush remaining messages */
    (void)Dlt_FlushBuffer();
    
    /* Reset state */
    Dlt_Internal.state = DLT_STATE_UNINIT;
}

/**
 * @brief Gets version information
 */
void Dlt_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (DLT_VERSION_INFO_API == STD_ON)
    if (versioninfo == NULL_PTR) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_GETVERSIONINFO, DLT_E_PARAM_POINTER);
        #endif
        return;
    }
    
    versioninfo->vendorID = DLT_VENDOR_ID;
    versioninfo->moduleID = DLT_MODULE_ID;
    versioninfo->sw_major_version = DLT_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = DLT_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = DLT_SW_PATCH_VERSION;
    #else
    (void)versioninfo;
    #endif
}

/**
 * @brief Logs a message without arguments
 */
Dlt_ReturnType Dlt_LogMessage(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    Dlt_LogLevelType logLevel,
    const char* message
)
{
    uint8 header[DLT_HEADER_SIZE_FULL];
    uint8 payload[DLT_BUFFER_ENTRY_SIZE];
    uint8 messageBuffer[DLT_BUFFER_ENTRY_SIZE];
    uint16 headerLength;
    uint16 payloadLength;
    uint16 messageLength;
    
    /* Check initialization */
    if (Dlt_Internal.state != DLT_STATE_INIT) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_LOGMESSAGE, DLT_E_UNINIT);
        #endif
        return E_NOT_OK;
    }
    
    /* Check parameters */
    if ((appId == NULL_PTR) || (contextId == NULL_PTR) || (message == NULL_PTR)) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_LOGMESSAGE, DLT_E_PARAM_POINTER);
        #endif
        return E_NOT_OK;
    }
    
    /* Check log level */
    if (Dlt_ShouldLog(appId, contextId, logLevel) == FALSE) {
        return E_OK; /* Silently drop */
    }
    
    /* Build payload */
    if (Dlt_BuildLogPayload(logLevel, message, payload, &payloadLength) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Build header */
    if (Dlt_BuildHeader(appId, contextId, DLT_TYPE_LOG, 
                        Dlt_ConvertLogLevel(logLevel), 
                        payloadLength, header, &headerLength) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Combine header and payload */
    if ((headerLength + payloadLength) > DLT_BUFFER_ENTRY_SIZE) {
        return E_NOT_OK;
    }
    
    memcpy(messageBuffer, header, headerLength);
    memcpy(&messageBuffer[headerLength], payload, payloadLength);
    messageLength = headerLength + payloadLength;
    
    /* Send to outputs */
    return Dlt_SendToOutputs(messageBuffer, messageLength);
}

/**
 * @brief Logs a formatted message with arguments
 */
Dlt_ReturnType Dlt_LogMessageWithArg(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    Dlt_LogLevelType logLevel,
    const char* format,
    ...
)
{
    uint8 header[DLT_HEADER_SIZE_FULL];
    uint8 payload[DLT_BUFFER_ENTRY_SIZE];
    uint8 messageBuffer[DLT_BUFFER_ENTRY_SIZE];
    uint16 headerLength;
    uint16 payloadLength;
    uint16 messageLength;
    va_list args;
    
    /* Check initialization */
    if (Dlt_Internal.state != DLT_STATE_INIT) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_LOGMESSAGEWITHARGS, DLT_E_UNINIT);
        #endif
        return E_NOT_OK;
    }
    
    /* Check parameters */
    if ((appId == NULL_PTR) || (contextId == NULL_PTR) || (format == NULL_PTR)) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_LOGMESSAGEWITHARGS, DLT_E_PARAM_POINTER);
        #endif
        return E_NOT_OK;
    }
    
    /* Check log level */
    if (Dlt_ShouldLog(appId, contextId, logLevel) == FALSE) {
        return E_OK; /* Silently drop */
    }
    
    /* Build payload with variable arguments */
    va_start(args, format);
    if (Dlt_BuildFormattedPayload(logLevel, format, args, payload, &payloadLength) != E_OK) {
        va_end(args);
        return E_NOT_OK;
    }
    va_end(args);
    
    /* Build header */
    if (Dlt_BuildHeader(appId, contextId, DLT_TYPE_LOG, 
                        Dlt_ConvertLogLevel(logLevel), 
                        payloadLength, header, &headerLength) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Combine header and payload */
    if ((headerLength + payloadLength) > DLT_BUFFER_ENTRY_SIZE) {
        return E_NOT_OK;
    }
    
    memcpy(messageBuffer, header, headerLength);
    memcpy(&messageBuffer[headerLength], payload, payloadLength);
    messageLength = headerLength + payloadLength;
    
    /* Send to outputs */
    return Dlt_SendToOutputs(messageBuffer, messageLength);
}

/**
 * @brief Sends a trace point
 */
Dlt_ReturnType Dlt_TracePoint(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    Dlt_TraceType traceType,
    uint32 traceInfo
)
{
    uint8 header[DLT_HEADER_SIZE_FULL];
    uint8 payload[DLT_BUFFER_ENTRY_SIZE];
    uint8 messageBuffer[DLT_BUFFER_ENTRY_SIZE];
    uint16 headerLength;
    uint16 payloadLength;
    uint16 messageLength;
    uint8 ctxIdx;
    
    /* Check initialization */
    if (Dlt_Internal.state != DLT_STATE_INIT) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_TRACEPOINT, DLT_E_UNINIT);
        #endif
        return E_NOT_OK;
    }
    
    /* Check parameters */
    if ((appId == NULL_PTR) || (contextId == NULL_PTR)) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_TRACEPOINT, DLT_E_PARAM_POINTER);
        #endif
        return E_NOT_OK;
    }
    
    /* Check trace type */
    if ((traceType < DLT_TRACE_TYPE_VARIABLE) || (traceType > DLT_TRACE_TYPE_VFB)) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_TRACEPOINT, DLT_E_INVALID_TRACETYPE);
        #endif
        return E_NOT_OK;
    }
    
    /* Check if tracing is enabled for this context */
    ctxIdx = Dlt_FindContextIndex(appId, contextId);
    if ((ctxIdx != DLT_INVALID_CONTEXT_INDEX) && 
        (Dlt_Internal.contexts[ctxIdx].traceStatus == FALSE)) {
        return E_OK; /* Tracing disabled for this context */
    }
    
    /* Build payload */
    if (Dlt_BuildTracePayload(traceType, traceInfo, NULL_PTR, 0, payload, &payloadLength) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Build header */
    if (Dlt_BuildHeader(appId, contextId, DLT_TYPE_TRACE, 
                        traceType << 1, 
                        payloadLength, header, &headerLength) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Combine header and payload */
    if ((headerLength + payloadLength) > DLT_BUFFER_ENTRY_SIZE) {
        return E_NOT_OK;
    }
    
    memcpy(messageBuffer, header, headerLength);
    memcpy(&messageBuffer[headerLength], payload, payloadLength);
    messageLength = headerLength + payloadLength;
    
    /* Send to outputs */
    return Dlt_SendToOutputs(messageBuffer, messageLength);
}

/**
 * @brief Traces a variable value
 */
Dlt_ReturnType Dlt_TraceVariable(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    const char* variableName,
    sint32 variableValue
)
{
    uint8 header[DLT_HEADER_SIZE_FULL];
    uint8 payload[DLT_BUFFER_ENTRY_SIZE];
    uint8 messageBuffer[DLT_BUFFER_ENTRY_SIZE];
    uint16 headerLength;
    uint16 payloadLength;
    uint16 messageLength;
    uint8 ctxIdx;
    
    /* Check initialization */
    if (Dlt_Internal.state != DLT_STATE_INIT) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_TRACEVARIABLE, DLT_E_UNINIT);
        #endif
        return E_NOT_OK;
    }
    
    /* Check parameters */
    if ((appId == NULL_PTR) || (contextId == NULL_PTR) || (variableName == NULL_PTR)) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_TRACEVARIABLE, DLT_E_PARAM_POINTER);
        #endif
        return E_NOT_OK;
    }
    
    /* Check if tracing is enabled for this context */
    ctxIdx = Dlt_FindContextIndex(appId, contextId);
    if ((ctxIdx != DLT_INVALID_CONTEXT_INDEX) && 
        (Dlt_Internal.contexts[ctxIdx].traceStatus == FALSE)) {
        return E_OK; /* Tracing disabled for this context */
    }
    
    /* Build payload */
    if (Dlt_BuildTracePayload(DLT_TRACE_TYPE_VARIABLE, 0U, 
                              variableName, variableValue, 
                              payload, &payloadLength) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Build header */
    if (Dlt_BuildHeader(appId, contextId, DLT_TYPE_TRACE, 
                        DLT_TRACE_TYPE_VARIABLE << 1, 
                        payloadLength, header, &headerLength) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Combine header and payload */
    if ((headerLength + payloadLength) > DLT_BUFFER_ENTRY_SIZE) {
        return E_NOT_OK;
    }
    
    memcpy(messageBuffer, header, headerLength);
    memcpy(&messageBuffer[headerLength], payload, payloadLength);
    messageLength = headerLength + payloadLength;
    
    /* Send to outputs */
    return Dlt_SendToOutputs(messageBuffer, messageLength);
}

/**
 * @brief Registers a context
 */
Dlt_ReturnType Dlt_RegisterContext(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    const char* description
)
{
    uint8 ctxIdx;
    
    /* Check initialization */
    if (Dlt_Internal.state != DLT_STATE_INIT) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_REGISTERCONTEXT, DLT_E_UNINIT);
        #endif
        return E_NOT_OK;
    }
    
    /* Check parameters */
    if ((appId == NULL_PTR) || (contextId == NULL_PTR)) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_REGISTERCONTEXT, DLT_E_PARAM_POINTER);
        #endif
        return E_NOT_OK;
    }
    
    /* Check if context already exists */
    ctxIdx = Dlt_FindContextIndex(appId, contextId);
    if (ctxIdx != DLT_INVALID_CONTEXT_INDEX) {
        /* Context already registered, update it */
        Dlt_Internal.contexts[ctxIdx].logLevel = Dlt_DefaultConfig.defaultLogLevel;
        Dlt_Internal.contexts[ctxIdx].traceStatus = TRUE;
        return E_OK;
    }
    
    /* Allocate new context */
    ctxIdx = Dlt_AllocateContext();
    if (ctxIdx == DLT_INVALID_CONTEXT_INDEX) {
        return E_NOT_OK;
    }
    
    /* Initialize context */
    memcpy(Dlt_Internal.contexts[ctxIdx].appId, appId, 4);
    Dlt_Internal.contexts[ctxIdx].appId[4] = '\0';
    memcpy(Dlt_Internal.contexts[ctxIdx].contextId, contextId, 4);
    Dlt_Internal.contexts[ctxIdx].contextId[4] = '\0';
    Dlt_Internal.contexts[ctxIdx].logLevel = Dlt_DefaultConfig.defaultLogLevel;
    Dlt_Internal.contexts[ctxIdx].traceStatus = TRUE;
    Dlt_Internal.contexts[ctxIdx].registered = TRUE;
    Dlt_Internal.contexts[ctxIdx].messageCount = 0U;
    
    if (ctxIdx >= Dlt_Internal.numContexts) {
        Dlt_Internal.numContexts = ctxIdx + 1U;
    }
    
    return E_OK;
}

/**
 * @brief Unregisters a context
 */
Dlt_ReturnType Dlt_UnregisterContext(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId
)
{
    uint8 ctxIdx;
    
    /* Check initialization */
    if (Dlt_Internal.state != DLT_STATE_INIT) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_REGISTERCONTEXT, DLT_E_UNINIT);
        #endif
        return E_NOT_OK;
    }
    
    /* Check parameters */
    if ((appId == NULL_PTR) || (contextId == NULL_PTR)) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_REGISTERCONTEXT, DLT_E_PARAM_POINTER);
        #endif
        return E_NOT_OK;
    }
    
    /* Find context */
    ctxIdx = Dlt_FindContextIndex(appId, contextId);
    if (ctxIdx == DLT_INVALID_CONTEXT_INDEX) {
        return E_NOT_OK;
    }
    
    /* Mark as unregistered */
    Dlt_Internal.contexts[ctxIdx].registered = FALSE;
    
    return E_OK;
}

/**
 * @brief Sets the log level for a context
 */
Dlt_ReturnType Dlt_SetLogLevel(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    Dlt_LogLevelType newLogLevel
)
{
    uint8 ctxIdx;
    
    /* Check initialization */
    if (Dlt_Internal.state != DLT_STATE_INIT) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_SETLOGLEVEL, DLT_E_UNINIT);
        #endif
        return E_NOT_OK;
    }
    
    /* Check parameters */
    if ((appId == NULL_PTR) || (contextId == NULL_PTR)) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_SETLOGLEVEL, DLT_E_PARAM_POINTER);
        #endif
        return E_NOT_OK;
    }
    
    /* Check log level validity */
    if ((newLogLevel < DLT_LOG_VERBOSE) || (newLogLevel > DLT_LOG_OFF)) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_SETLOGLEVEL, DLT_E_INVALID_LOGLEVEL);
        #endif
        return E_NOT_OK;
    }
    
    /* Find context */
    ctxIdx = Dlt_FindContextIndex(appId, contextId);
    if (ctxIdx == DLT_INVALID_CONTEXT_INDEX) {
        /* Auto-register context if not found */
        if (Dlt_RegisterContext(appId, contextId, NULL_PTR) != E_OK) {
            return E_NOT_OK;
        }
        ctxIdx = Dlt_FindContextIndex(appId, contextId);
        if (ctxIdx == DLT_INVALID_CONTEXT_INDEX) {
            return E_NOT_OK;
        }
    }
    
    /* Set log level */
    Dlt_Internal.contexts[ctxIdx].logLevel = newLogLevel;
    
    return E_OK;
}

/**
 * @brief Gets the log level for a context
 */
Dlt_ReturnType Dlt_GetLogLevel(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    Dlt_LogLevelType* logLevel
)
{
    uint8 ctxIdx;
    
    /* Check initialization */
    if (Dlt_Internal.state != DLT_STATE_INIT) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_GETLOGLEVEL, DLT_E_UNINIT);
        #endif
        return E_NOT_OK;
    }
    
    /* Check parameters */
    if ((appId == NULL_PTR) || (contextId == NULL_PTR) || (logLevel == NULL_PTR)) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_GETLOGLEVEL, DLT_E_PARAM_POINTER);
        #endif
        return E_NOT_OK;
    }
    
    /* Find context */
    ctxIdx = Dlt_FindContextIndex(appId, contextId);
    if (ctxIdx == DLT_INVALID_CONTEXT_INDEX) {
        *logLevel = Dlt_DefaultConfig.defaultLogLevel;
    } else {
        *logLevel = Dlt_Internal.contexts[ctxIdx].logLevel;
    }
    
    return E_OK;
}

/**
 * @brief Sets the trace status for a context
 */
Dlt_ReturnType Dlt_SetTraceStatus(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    boolean traceStatus
)
{
    uint8 ctxIdx;
    
    /* Check initialization */
    if (Dlt_Internal.state != DLT_STATE_INIT) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_SETLOGLEVEL, DLT_E_UNINIT);
        #endif
        return E_NOT_OK;
    }
    
    /* Check parameters */
    if ((appId == NULL_PTR) || (contextId == NULL_PTR)) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_SETLOGLEVEL, DLT_E_PARAM_POINTER);
        #endif
        return E_NOT_OK;
    }
    
    /* Find context */
    ctxIdx = Dlt_FindContextIndex(appId, contextId);
    if (ctxIdx == DLT_INVALID_CONTEXT_INDEX) {
        return E_NOT_OK;
    }
    
    /* Set trace status */
    Dlt_Internal.contexts[ctxIdx].traceStatus = traceStatus;
    
    return E_OK;
}

/**
 * @brief Sets the output mode
 */
Dlt_ReturnType Dlt_SetOutputMode(Dlt_OutputModeType outputMode)
{
    /* Check initialization */
    if (Dlt_Internal.state != DLT_STATE_INIT) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_SETLOGLEVEL, DLT_E_UNINIT);
        #endif
        return E_NOT_OK;
    }
    
    /* Check validity */
    if (outputMode > DLT_OUTPUT_MODE_BUFFER) {
        return E_NOT_OK;
    }
    
    Dlt_Internal.outputMode = outputMode;
    
    return E_OK;
}

/**
 * @brief Gets the output mode
 */
Dlt_ReturnType Dlt_GetOutputMode(Dlt_OutputModeType* outputMode)
{
    /* Check initialization */
    if (Dlt_Internal.state != DLT_STATE_INIT) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_GETLOGLEVEL, DLT_E_UNINIT);
        #endif
        return E_NOT_OK;
    }
    
    /* Check parameters */
    if (outputMode == NULL_PTR) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_GETLOGLEVEL, DLT_E_PARAM_POINTER);
        #endif
        return E_NOT_OK;
    }
    
    *outputMode = Dlt_Internal.outputMode;
    
    return E_OK;
}

/**
 * @brief Registers serial output callback
 */
Dlt_ReturnType Dlt_RegisterSerialOutputCbk(Dlt_SerialOutputCbkType callback)
{
    /* Check initialization */
    if (Dlt_Internal.state != DLT_STATE_INIT) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_REGISTERCONTEXT, DLT_E_UNINIT);
        #endif
        return E_NOT_OK;
    }
    
    Dlt_Internal.serialCbk = callback;
    
    return E_OK;
}

/**
 * @brief Registers network output callback
 */
Dlt_ReturnType Dlt_RegisterNetworkOutputCbk(Dlt_NetworkOutputCbkType callback)
{
    /* Check initialization */
    if (Dlt_Internal.state != DLT_STATE_INIT) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_REGISTERCONTEXT, DLT_E_UNINIT);
        #endif
        return E_NOT_OK;
    }
    
    Dlt_Internal.networkCbk = callback;
    
    return E_OK;
}

/**
 * @brief Registers timestamp callback
 */
Dlt_ReturnType Dlt_RegisterGetTimestampCbk(Dlt_GetTimestampCbkType callback)
{
    /* Check initialization */
    if (Dlt_Internal.state != DLT_STATE_INIT) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_REGISTERCONTEXT, DLT_E_UNINIT);
        #endif
        return E_NOT_OK;
    }
    
    Dlt_Internal.timestampCbk = callback;
    
    return E_OK;
}

/**
 * @brief Flushes the ring buffer
 */
Dlt_ReturnType Dlt_FlushBuffer(void)
{
    uint8 data[DLT_BUFFER_ENTRY_SIZE];
    uint16 length;
    Dlt_ReturnType result = E_OK;
    
    /* Check initialization */
    if (Dlt_Internal.state != DLT_STATE_INIT) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_MAINFUNCTION, DLT_E_UNINIT);
        #endif
        return E_NOT_OK;
    }
    
    /* Flush all buffered messages */
    while (Dlt_Internal.ringBuffer.count > 0U) {
        if (Dlt_RingBufferRead(data, &length) == E_OK) {
            /* Send to active outputs */
            if (Dlt_Internal.serialCbk != NULL_PTR) {
                (void)Dlt_Internal.serialCbk(data, length);
            }
            if (Dlt_Internal.networkCbk != NULL_PTR) {
                (void)Dlt_Internal.networkCbk(data, length);
            }
        } else {
            result = E_NOT_OK;
            break;
        }
    }
    
    return result;
}

/**
 * @brief Gets the buffer status
 */
Dlt_ReturnType Dlt_GetBufferStatus(uint16* usedEntries, uint16* freeEntries)
{
    /* Check initialization */
    if (Dlt_Internal.state != DLT_STATE_INIT) {
        #if (DLT_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(DLT_MODULE_ID, 0U, DLT_SID_MAINFUNCTION, DLT_E_UNINIT);
        #endif
        return E_NOT_OK;
    }
    
    if (usedEntries != NULL_PTR) {
        *usedEntries = Dlt_Internal.ringBuffer.count;
    }
    
    if (freeEntries != NULL_PTR) {
        *freeEntries = DLT_RING_BUFFER_SIZE - Dlt_Internal.ringBuffer.count;
    }
    
    return E_OK;
}

/**
 * @brief Tx confirmation callback
 */
void Dlt_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    (void)TxPduId;
    (void)result;
    /* Currently not used - placeholder for future implementation */
}

/**
 * @brief Rx indication callback
 */
void Dlt_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    #if (DLT_CONTROL_MESSAGES_ENABLED == STD_ON)
    /* Process control messages from external tools */
    if ((PduInfoPtr != NULL_PTR) && (PduInfoPtr->SduDataPtr != NULL_PTR) && 
        (PduInfoPtr->SduLength > 0U)) {
        /* Parse and handle control messages */
        /* This is a placeholder for full control message implementation */
    }
    #else
    (void)RxPduId;
    (void)PduInfoPtr;
    #endif
}

/**
 * @brief Main function for periodic processing
 */
void Dlt_MainFunction(void)
{
    static uint16 flushCounter = 0U;
    
    /* Check initialization */
    if (Dlt_Internal.state != DLT_STATE_INIT) {
        return;
    }
    
    /* Periodic flush if buffer is getting full */
    flushCounter++;
    if (flushCounter >= (DLT_FLUSH_PERIOD_MS / DLT_MAIN_FUNCTION_PERIOD_MS)) {
        flushCounter = 0U;
        
        /* Flush buffer if needed */
        if (Dlt_Internal.ringBuffer.count > (DLT_RING_BUFFER_SIZE / 2U)) {
            (void)Dlt_FlushBuffer();
        }
    }
    
    /* Clear overflow flag periodically */
    if (Dlt_Internal.overflowOccurred) {
        Dlt_Internal.overflowOccurred = FALSE;
    }
}
