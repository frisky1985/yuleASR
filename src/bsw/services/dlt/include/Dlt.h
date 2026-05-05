/**
 * @file Dlt.h
 * @brief Diagnostic Log and Trace module following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-30
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Diagnostic Log and Trace (DLT)
 * Layer: Service Layer
 * Purpose: Provide logging and tracing functionality for debugging and diagnostics
 */

#ifndef DLT_H
#define DLT_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Dlt_Cfg.h"
#include "ComStack_Types.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define DLT_VENDOR_ID                   (0x01U) /* YuleTech Vendor ID */
#define DLT_MODULE_ID                   (0x4CU) /* DLT Module ID (76) */
#define DLT_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define DLT_AR_RELEASE_MINOR_VERSION    (0x04U)
#define DLT_AR_RELEASE_REVISION_VERSION (0x00U)
#define DLT_SW_MAJOR_VERSION            (0x01U)
#define DLT_SW_MINOR_VERSION            (0x00U)
#define DLT_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    SERVICE IDs
==================================================================================================*/
#define DLT_SID_INIT                    (0x01U)
#define DLT_SID_DEINIT                  (0x02U)
#define DLT_SID_GETVERSIONINFO          (0x03U)
#define DLT_SID_MAINFUNCTION            (0x04U)
#define DLT_SID_LOGMESSAGE              (0x05U)
#define DLT_SID_LOGMESSAGEWITHARGS      (0x06U)
#define DLT_SID_TRACEPOINT              (0x07U)
#define DLT_SID_TRACEVARIABLE           (0x08U)
#define DLT_SID_REGISTERCONTEXT         (0x09U)
#define DLT_SID_SETLOGLEVEL             (0x0AU)
#define DLT_SID_GETLOGLEVEL             (0x0BU)
#define DLT_SID_TXCONFIRMATION          (0x40U)
#define DLT_SID_RXINDICATION            (0x41U)

/*==================================================================================================
*                                    DET ERROR CODES
==================================================================================================*/
#define DLT_E_UNINIT                    (0x01U)
#define DLT_E_PARAM                     (0x02U)
#define DLT_E_PARAM_POINTER             (0x03U)
#define DLT_E_INIT_FAILED               (0x04U)
#define DLT_E_BUFFER_FULL               (0x05U)
#define DLT_E_INVALID_CONTEXT           (0x06U)
#define DLT_E_INVALID_LOGLEVEL          (0x07U)
#define DLT_E_INVALID_TRACETYPE         (0x08U)
#define DLT_E_INTERFACE_ERROR           (0x09U)

/*==================================================================================================
*                                    DLT LOG LEVELS
==================================================================================================*/
/** @brief Log level - Verbose (most detailed) */
#define DLT_LOG_VERBOSE                 (0x01U)
/** @brief Log level - Debug */
#define DLT_LOG_DEBUG                   (0x02U)
/** @brief Log level - Info */
#define DLT_LOG_INFO                    (0x03U)
/** @brief Log level - Warning */
#define DLT_LOG_WARN                    (0x04U)
/** @brief Log level - Error */
#define DLT_LOG_ERROR                   (0x05U)
/** @brief Log level - Fatal (most severe) */
#define DLT_LOG_FATAL                   (0x06U)
/** @brief Log level - Off (logging disabled) */
#define DLT_LOG_OFF                     (0x07U)

/*==================================================================================================
*                                    DLT TRACE TYPES
==================================================================================================*/
/** @brief Trace type - Variable */
#define DLT_TRACE_TYPE_VARIABLE         (0x01U)
/** @brief Trace type - Function In */
#define DLT_TRACE_TYPE_FUNCTION_IN      (0x02U)
/** @brief Trace type - Function Out */
#define DLT_TRACE_TYPE_FUNCTION_OUT     (0x03U)
/** @brief Trace type - State */
#define DLT_TRACE_TYPE_STATE            (0x04U)
/** @brief Trace type - VFB (Virtual Functional Bus) */
#define DLT_TRACE_TYPE_VFB              (0x05U)

/*==================================================================================================
*                                    DLT OUTPUT MODES
==================================================================================================*/
/** @brief Output mode - None */
#define DLT_OUTPUT_MODE_NONE            (0x00U)
/** @brief Output mode - Serial/UART */
#define DLT_OUTPUT_MODE_SERIAL          (0x01U)
/** @brief Output mode - Network (TCP/UDP) */
#define DLT_OUTPUT_MODE_NETWORK         (0x02U)
/** @brief Output mode - Both Serial and Network */
#define DLT_OUTPUT_MODE_BOTH            (0x03U)
/** @brief Output mode - Internal Buffer only */
#define DLT_OUTPUT_MODE_BUFFER          (0x04U)

/*==================================================================================================
*                                    DLT MESSAGE TYPES
==================================================================================================*/
/** @brief Message type - Log */
#define DLT_TYPE_LOG                    (0x00U)
/** @brief Message type - Trace */
#define DLT_TYPE_TRACE                  (0x01U)
/** @brief Message type - Control */
#define DLT_TYPE_CONTROL                (0x02U)

/*==================================================================================================
*                                    DLT HEADER CONSTANTS
==================================================================================================*/
/** @brief Standard header version */
#define DLT_HEADER_VERSION              (0x01U)
/** @brief Header use extended */
#define DLT_HEADER_EXTENDED             (0x01U)
/** @brief Header use timestamp */
#define DLT_HEADER_TIMESTAMP            (0x02U)
/** @brief Header use storage session */
#define DLT_HEADER_STORAGE_SESSION      (0x04U)

/*==================================================================================================
*                                    DLT CONTROL MESSAGES
==================================================================================================*/
/** @brief Control message - Set Log Level */
#define DLT_CTRL_SET_LOG_LEVEL          (0x01U)
/** @brief Control message - Get Log Level */
#define DLT_CTRL_GET_LOG_LEVEL          (0x02U)
/** @brief Control message - Register Context */
#define DLT_CTRL_REGISTER_CONTEXT       (0x03U)
/** @brief Control message - Unregister Context */
#define DLT_CTRL_UNREGISTER_CONTEXT     (0x04U)

/*==================================================================================================
*                                    TYPE DEFINITIONS
==================================================================================================*/

/** @brief DLT Log Level Type */
typedef uint8 Dlt_LogLevelType;

/** @brief DLT Trace Type */
typedef uint8 Dlt_TraceType;

/** @brief DLT Output Mode Type */
typedef uint8 Dlt_OutputModeType;

/** @brief DLT Message Type */
typedef uint8 Dlt_MessageType;

/** @brief DLT Context ID Type (4 chars + null) */
typedef uint8 Dlt_ContextIdType[5];

/** @brief DLT Application ID Type (4 chars + null) */
typedef uint8 Dlt_ApplicationIdType[5];

/** @brief DLT Session ID Type */
typedef uint32 Dlt_SessionIdType;

/** @brief DLT Timestamp Type (microseconds) */
typedef uint32 Dlt_TimestampType;

/** @brief DLT Message Counter Type */
typedef uint8 Dlt_MessageCounterType;

/** @brief DLT ECU ID Type (4 chars + null) */
typedef uint8 Dlt_EcuIdType[5];

/** @brief DLT Return Type */
typedef Std_ReturnType Dlt_ReturnType;

/*==================================================================================================
*                                    DLT CONTEXT CONFIG TYPE
==================================================================================================*/
typedef struct {
    Dlt_ApplicationIdType appId;
    Dlt_ContextIdType contextId;
    Dlt_LogLevelType logLevel;
    boolean traceStatus;
} Dlt_ContextConfigType;

/*==================================================================================================
*                                    DLT BUFFER ENTRY TYPE
==================================================================================================*/
typedef struct {
    uint8 data[DLT_BUFFER_ENTRY_SIZE];
    uint16 length;
    boolean used;
} Dlt_BufferEntryType;

/*==================================================================================================
*                                    DLT RING BUFFER TYPE
==================================================================================================*/
typedef struct {
    Dlt_BufferEntryType entries[DLT_RING_BUFFER_SIZE];
    uint16 head;
    uint16 tail;
    uint16 count;
    boolean overflow;
} Dlt_RingBufferType;

/*==================================================================================================
*                                    DLT CONFIG TYPE
==================================================================================================*/
typedef struct {
    Dlt_OutputModeType outputMode;
    Dlt_LogLevelType defaultLogLevel;
    boolean timestampEnabled;
    boolean ecuIdEnabled;
    boolean sessionIdEnabled;
    boolean devErrorDetect;
    boolean versionInfoApi;
    uint16 networkPort;
    Dlt_EcuIdType ecuId;
} Dlt_ConfigType;

/*==================================================================================================
*                                    DLT CONTEXT TYPE
==================================================================================================*/
typedef struct {
    Dlt_ApplicationIdType appId;
    Dlt_ContextIdType contextId;
    Dlt_LogLevelType logLevel;
    boolean traceStatus;
    boolean registered;
    uint32 messageCount;
} Dlt_ContextType;

/*==================================================================================================
*                                    DLT MESSAGE HEADER TYPE
==================================================================================================*/
typedef struct {
    uint8 headerType;
    uint8 messageCounter;
    uint16 length;
    Dlt_EcuIdType ecuId;
    Dlt_SessionIdType sessionId;
    Dlt_TimestampType timestamp;
    Dlt_ApplicationIdType appId;
    Dlt_ContextIdType contextId;
    uint8 messageType;
    uint8 messageInfo;
    uint16 numberOfArguments;
} Dlt_StandardHeaderType;

/*==================================================================================================
*                                    DLT CALLBACK TYPES
==================================================================================================*/
/** @brief Serial output callback function type */
typedef Dlt_ReturnType (*Dlt_SerialOutputCbkType)(const uint8* data, uint16 length);

/** @brief Network output callback function type */
typedef Dlt_ReturnType (*Dlt_NetworkOutputCbkType)(const uint8* data, uint16 length);

/** @brief Timestamp callback function type */
typedef Dlt_TimestampType (*Dlt_GetTimestampCbkType)(void);

/*==================================================================================================
*                                    GLOBAL CONFIG POINTER
==================================================================================================*/
#define DLT_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

extern const Dlt_ConfigType Dlt_Config;

#define DLT_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
#define DLT_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the Diagnostic Log and Trace module
 * @param ConfigPtr Pointer to configuration structure
 */
void Dlt_Init(const Dlt_ConfigType* ConfigPtr);

/**
 * @brief Deinitializes the DLT module
 */
void Dlt_DeInit(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
void Dlt_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Logs a message without arguments
 * @param appId Application ID
 * @param contextId Context ID
 * @param logLevel Log level
 * @param message Message string
 * @return Result of operation
 */
Dlt_ReturnType Dlt_LogMessage(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    Dlt_LogLevelType logLevel,
    const char* message
);

/**
 * @brief Logs a formatted message with arguments
 * @param appId Application ID
 * @param contextId Context ID
 * @param logLevel Log level
 * @param format Format string (printf-style)
 * @param ... Variable arguments
 * @return Result of operation
 */
Dlt_ReturnType Dlt_LogMessageWithArg(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    Dlt_LogLevelType logLevel,
    const char* format,
    ...
);

/**
 * @brief Sends a trace point
 * @param appId Application ID
 * @param contextId Context ID
 * @param traceType Trace type
 * @param traceInfo Additional trace information
 * @return Result of operation
 */
Dlt_ReturnType Dlt_TracePoint(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    Dlt_TraceType traceType,
    uint32 traceInfo
);

/**
 * @brief Traces a variable value
 * @param appId Application ID
 * @param contextId Context ID
 * @param variableName Variable name
 * @param variableValue Variable value
 * @return Result of operation
 */
Dlt_ReturnType Dlt_TraceVariable(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    const char* variableName,
    sint32 variableValue
);

/**
 * @brief Registers a context
 * @param appId Application ID
 * @param contextId Context ID
 * @param description Context description
 * @return Result of operation
 */
Dlt_ReturnType Dlt_RegisterContext(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    const char* description
);

/**
 * @brief Unregisters a context
 * @param appId Application ID
 * @param contextId Context ID
 * @return Result of operation
 */
Dlt_ReturnType Dlt_UnregisterContext(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId
);

/**
 * @brief Sets the log level for a context
 * @param appId Application ID
 * @param contextId Context ID
 * @param newLogLevel New log level
 * @return Result of operation
 */
Dlt_ReturnType Dlt_SetLogLevel(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    Dlt_LogLevelType newLogLevel
);

/**
 * @brief Gets the log level for a context
 * @param appId Application ID
 * @param contextId Context ID
 * @param logLevel Pointer to store current log level
 * @return Result of operation
 */
Dlt_ReturnType Dlt_GetLogLevel(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    Dlt_LogLevelType* logLevel
);

/**
 * @brief Sets the trace status for a context
 * @param appId Application ID
 * @param contextId Context ID
 * @param traceStatus New trace status
 * @return Result of operation
 */
Dlt_ReturnType Dlt_SetTraceStatus(
    const Dlt_ApplicationIdType appId,
    const Dlt_ContextIdType contextId,
    boolean traceStatus
);

/**
 * @brief Sets the output mode
 * @param outputMode New output mode
 * @return Result of operation
 */
Dlt_ReturnType Dlt_SetOutputMode(Dlt_OutputModeType outputMode);

/**
 * @brief Gets the output mode
 * @param outputMode Pointer to store current output mode
 * @return Result of operation
 */
Dlt_ReturnType Dlt_GetOutputMode(Dlt_OutputModeType* outputMode);

/**
 * @brief Registers serial output callback
 * @param callback Serial output callback function
 * @return Result of operation
 */
Dlt_ReturnType Dlt_RegisterSerialOutputCbk(Dlt_SerialOutputCbkType callback);

/**
 * @brief Registers network output callback
 * @param callback Network output callback function
 * @return Result of operation
 */
Dlt_ReturnType Dlt_RegisterNetworkOutputCbk(Dlt_NetworkOutputCbkType callback);

/**
 * @brief Registers timestamp callback
 * @param callback Timestamp callback function
 * @return Result of operation
 */
Dlt_ReturnType Dlt_RegisterGetTimestampCbk(Dlt_GetTimestampCbkType callback);

/**
 * @brief Flushes the ring buffer
 * @return Result of operation
 */
Dlt_ReturnType Dlt_FlushBuffer(void);

/**
 * @brief Gets the buffer status
 * @param usedEntries Pointer to store used entries count
 * @param freeEntries Pointer to store free entries count
 * @return Result of operation
 */
Dlt_ReturnType Dlt_GetBufferStatus(uint16* usedEntries, uint16* freeEntries);

/**
 * @brief Tx confirmation callback
 * @param TxPduId PDU ID
 * @param result Result of transmission
 */
void Dlt_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

/**
 * @brief Rx indication callback
 * @param RxPduId PDU ID
 * @param PduInfoPtr Pointer to PDU info
 */
void Dlt_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);

/**
 * @brief Main function for periodic processing
 */
void Dlt_MainFunction(void);

#define DLT_STOP_SEC_CODE
#include "MemMap.h"

#endif /* DLT_H */
