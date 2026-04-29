/******************************************************************************
 * @file    Com_ErrorHandling.h
 * @brief   COM Module - Error Handling and Queue Overflow Detection
 *
 * This file defines the error handling infrastructure for the AUTOSAR COM module.
 * It provides:
 * - Send queue overflow detection
 * - Configurable overflow strategies (DROP_OLDEST/DROP_NEWEST/REJECT)
 * - DET (Default Error Tracer) integration
 * - Global error statistics and counters
 *
 * T013: Error Handling and Queue Overflow Detection
 * ASIL-D Safety Level
 * AUTOSAR Classic Platform R22-11 compliant
 * SWS_Com_00600: Error Handling
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#ifndef COM_ERRORHANDLING_H
#define COM_ERRORHANDLING_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "Com_Private.h"

/******************************************************************************
 * Version Information
 ******************************************************************************/
#define COM_EH_SW_MAJOR_VERSION       1u
#define COM_EH_SW_MINOR_VERSION       0u
#define COM_EH_SW_PATCH_VERSION       0u

/******************************************************************************
 * Service IDs for Error Handling
 ******************************************************************************/
#define COM_SERVICE_ID_EH_INIT                    0x30u
#define COM_SERVICE_ID_EH_DEINIT                  0x31u
#define COM_SERVICE_ID_EH_REPORT_OVERFLOW         0x32u
#define COM_SERVICE_ID_EH_HANDLE_OVERFLOW         0x33u
#define COM_SERVICE_ID_EH_GET_STATS               0x34u
#define COM_SERVICE_ID_EH_RESET_STATS             0x35u
#define COM_SERVICE_ID_EH_GET_QUEUE_STATUS        0x36u
#define COM_SERVICE_ID_EH_APPLY_STRATEGY          0x37u

/******************************************************************************
 * Error Codes (Extension to Com_Types.h)
 ******************************************************************************/
#define COM_E_TX_QUEUE_OVERFLOW                   0x40u   /*!< Tx queue overflow */
#define COM_E_TX_QUEUE_FULL                       0x41u   /*!< Tx queue full */
#define COM_E_INVALID_OVERFLOW_STRATEGY           0x42u   /*!< Invalid overflow strategy */
#define COM_E_STATISTICS_CORRUPTION               0x43u   /*!< Statistics corruption detected */
#define COM_E_ERROR_COUNTER_OVERFLOW              0x44u   /*!< Error counter overflow */

/******************************************************************************
 * Configuration Constants
 ******************************************************************************/

/** Enable error handling module */
#ifndef COM_ERROR_HANDLING_ENABLE
#define COM_ERROR_HANDLING_ENABLE                 STD_ON
#endif

/** Enable detailed error statistics */
#ifndef COM_ERROR_STATISTICS_ENABLE
#define COM_ERROR_STATISTICS_ENABLE               STD_ON
#endif

/** Maximum error log entries */
#ifndef COM_MAX_ERROR_LOG_ENTRIES
#define COM_MAX_ERROR_LOG_ENTRIES                 16u
#endif

/** Error log wrap-around mode */
#ifndef COM_ERROR_LOG_WRAP_MODE
#define COM_ERROR_LOG_WRAP_MODE                   STD_ON
#endif

/******************************************************************************
 * Overflow Strategy Configuration
 ******************************************************************************/

/**
 * @brief Tx Queue Overflow Strategy Enumeration
 *
 * Defines the behavior when the send queue becomes full.
 * Configurable per I-PDU or globally.
 */
typedef enum {
    COM_TXQUEUE_REJECT_NEWEST = 0,      /*!< Reject the newest request (default) */
    COM_TXQUEUE_DROP_OLDEST,            /*!< Drop the oldest pending request */
    COM_TXQUEUE_DROP_NEWEST,            /*!< Drop the newest request without adding */
    COM_TXQUEUE_REJECT_OLDEST,          /*!< Reject and keep all existing */
    COM_TXQUEUE_NUM_STRATEGIES          /*!< Number of strategies (for validation) */
} Com_TxQueueOverflowStrategyType;

/******************************************************************************
 * Error Types and Structures
 ******************************************************************************/

/**
 * @brief Error Log Entry Type
 *
 * Records detailed information about each error occurrence.
 */
typedef struct {
    uint8 ModuleId;                     /*!< Module ID (COM_MODULE_ID) */
    uint8 ApiId;                        /*!< API service ID */
    uint8 ErrorId;                      /*!< Error code */
    uint16 PduId;                       /*!< I-PDU ID (if applicable) */
    uint32 Timestamp;                   /*!< Error timestamp */
    uint8 QueueFillLevel;               /*!< Queue fill level at error time */
    Com_TxQueueOverflowStrategyType Strategy; /*!< Strategy applied */
} Com_ErrorLogEntryType;

/**
 * @brief Global Error Statistics Structure
 *
 * Comprehensive error statistics for diagnostics and monitoring.
 * ASIL-D: Redundant counters for critical errors.
 */
typedef struct {
    /* Tx Queue Statistics */
    uint32 TxQueueOverflowCount;        /*!< Total queue overflow occurrences */
    uint32 TxQueueOverflowCount_Redund; /*!< Redundant counter (ASIL-D) */
    uint32 TxQueueRejectCount;          /*!< Requests rejected due to full queue */
    uint32 TxQueueDropOldestCount;      /*!< Oldest entries dropped */
    uint32 TxQueueDropNewestCount;      /*!< Newest entries dropped */

    /* General Error Counters */
    uint32 DetReportCount;              /*!< Number of Det_ReportError calls */
    uint32 InternalErrorCount;          /*!< Internal error occurrences */

    /* Per-I-PDU Error Counters */
    uint32 PerPduOverflowCount[COM_MAX_IPDUS];  /*!< Per-I-PDU overflow count */

    /* Error Rate Tracking */
    uint32 ErrorWindowStartTime;        /*!< Start of error rate window */
    uint32 ErrorsInWindow;              /*!< Errors in current window */
    uint32 PeakErrorRate;               /*!< Peak error rate observed */

    /* Timestamps */
    uint32 LastOverflowTimestamp;       /*!< Last overflow timestamp */
    uint32 LastErrorTimestamp;          /*!< Last any error timestamp */
    uint32 FirstErrorTimestamp;         /*!< First error since reset */

    /* Queue Status History */
    uint8 MaxQueueFillLevel;            /*!< Maximum queue fill level reached */
    uint8 CurrentQueueFillLevel;        /*!< Current queue fill level */

    /* ASIL-D Safety Check */
    uint16 StatisticsChecksum;          /*!< Checksum for integrity verification */
} Com_GlobalErrorStatsType;

/**
 * @brief Queue Status Type
 *
 * Current status of the send request queue.
 */
typedef struct {
    uint8 FillLevel;                    /*!< Current number of entries */
    uint8 MaxFillLevel;                 /*!< Maximum allowed entries */
    boolean IsFull;                     /*!< Queue is full */
    boolean IsEmpty;                    /*!< Queue is empty */
    uint8 Head;                         /*!< Head index */
    uint8 Tail;                         /*!< Tail index */
    Com_TxQueueOverflowStrategyType CurrentStrategy; /*!< Current strategy */
} Com_TxQueueStatusType;

/**
 * @brief Error Handling Configuration per I-PDU
 */
typedef struct {
    Com_TxQueueOverflowStrategyType OverflowStrategy;   /*!< Overflow strategy */
    boolean EnableErrorNotification;                    /*!< Enable error notification */
    void (*ErrorNotification)(Com_IPduIdType PduId, uint8 ErrorId); /*!< Callback */
    uint16 MaxErrorsBeforeNotification;                 /*!< Threshold for notification */
} Com_ErrorHandlingConfigType;

/******************************************************************************
 * External Variables
 ******************************************************************************/

/** Global error statistics - defined in Com_ErrorHandling.c */
extern Com_GlobalErrorStatsType Com_GlobalErrorStats;

/** Error handling configuration - defined in Com_Lcfg.c */
extern const Com_ErrorHandlingConfigType Com_ErrorHandlingConfig[COM_MAX_IPDUS];

/** Error log buffer - defined in Com_ErrorHandling.c */
extern Com_ErrorLogEntryType Com_ErrorLog[COM_MAX_ERROR_LOG_ENTRIES];

/** Error log index - defined in Com_ErrorHandling.c */
extern uint8 Com_ErrorLogIndex;

/******************************************************************************
 * API Functions
 ******************************************************************************/

/**
 * @brief Initialize error handling module
 *
 * Initializes error statistics, log buffer, and configuration.
 * Called during Com_Init().
 *
 * @ASIL-D: Safety critical initialization
 */
void Com_Eh_Init(void);

/**
 * @brief De-initialize error handling module
 *
 * Resets all error handling state.
 */
void Com_Eh_DeInit(void);

/**
 * @brief Report a Tx queue overflow event
 *
 * Records overflow occurrence and reports to DET if enabled.
 *
 * @param PduId I-PDU identifier where overflow occurred
 * @param RequestedStrategy Strategy to apply (or COM_TXQUEUE_NUM_STRATEGIES for default)
 * @return Strategy that was actually applied
 *
 * @ASIL-D: Dual-check for counter consistency
 */
Com_TxQueueOverflowStrategyType Com_Eh_ReportTxQueueOverflow(
    Com_IPduIdType PduId,
    Com_TxQueueOverflowStrategyType RequestedStrategy);

/**
 * @brief Apply overflow strategy to the Tx queue
 *
 * Executes the configured overflow handling strategy.
 *
 * @param PduId I-PDU identifier
 * @param Strategy Strategy to apply
 * @return E_OK if strategy applied successfully, E_NOT_OK otherwise
 */
Std_ReturnType Com_Eh_ApplyOverflowStrategy(
    Com_IPduIdType PduId,
    Com_TxQueueOverflowStrategyType Strategy);

/**
 * @brief Drop oldest entry from Tx queue
 *
 * Removes the oldest pending request to make room.
 *
 * @return E_OK if entry dropped, E_NOT_OK if queue empty or error
 */
Std_ReturnType Com_Eh_DropOldestTxRequest(void);

/**
 * @brief Drop newest entry from Tx queue
 *
 * Removes the most recently added pending request.
 *
 * @return E_OK if entry dropped, E_NOT_OK if queue empty or error
 */
Std_ReturnType Com_Eh_DropNewestTxRequest(void);

/**
 * @brief Get current Tx queue status
 *
 * @param StatusPtr Pointer to status structure to fill
 * @return E_OK if successful, E_NOT_OK if StatusPtr is NULL
 */
Std_ReturnType Com_Eh_GetTxQueueStatus(Com_TxQueueStatusType* StatusPtr);

/**
 * @brief Get global error statistics
 *
 * @param StatsPtr Pointer to statistics structure to fill
 * @return E_OK if successful, E_NOT_OK if StatsPtr is NULL
 */
Std_ReturnType Com_Eh_GetErrorStats(Com_GlobalErrorStatsType* StatsPtr);

/**
 * @brief Reset global error statistics
 *
 * Clears all error counters and statistics.
 */
void Com_Eh_ResetErrorStats(void);

/**
 * @brief Get error log entry
 *
 * @param Index Error log index (0 = oldest if not wrapped)
 * @param EntryPtr Pointer to store the entry
 * @return E_OK if entry retrieved, E_NOT_OK if invalid index
 */
Std_ReturnType Com_Eh_GetErrorLogEntry(uint8 Index, Com_ErrorLogEntryType* EntryPtr);

/**
 * @brief Log an error entry
 *
 * Records error details to the error log buffer.
 *
 * @param ApiId API service ID
 * @param ErrorId Error code
 * @param PduId I-PDU ID (COM_MAX_IPDUS if not applicable)
 * @param Strategy Overflow strategy applied
 */
void Com_Eh_LogError(uint8 ApiId, uint8 ErrorId, Com_IPduIdType PduId,
                     Com_TxQueueOverflowStrategyType Strategy);

/**
 * @brief Report error to DET
 *
 * Wrapper for Det_ReportError with error statistics update.
 *
 * @param ApiId API service ID
 * @param ErrorId Error code
 */
void Com_Eh_ReportDetError(uint8 ApiId, uint8 ErrorId);

/**
 * @brief Get overflow strategy for an I-PDU
 *
 * Returns the configured overflow strategy for the specified I-PDU.
 *
 * @param PduId I-PDU identifier
 * @return Configured overflow strategy (or COM_TXQUEUE_REJECT_NEWEST if invalid PduId)
 */
Com_TxQueueOverflowStrategyType Com_Eh_GetOverflowStrategy(Com_IPduIdType PduId);

/**
 * @brief Check if error rate is within acceptable limits
 *
 * @return TRUE if error rate is acceptable, FALSE if critical
 */
boolean Com_Eh_IsErrorRateAcceptable(void);

/**
 * @brief ASIL-D Safety Check: Validate error statistics integrity
 *
 * Performs integrity checks on error statistics:
 * - Redundant counter comparison
 * - Checksum verification
 * - Bounds checking
 *
 * @return E_OK if integrity check passed, E_NOT_OK otherwise
 */
Std_ReturnType Com_Eh_ValidateStatsIntegrity(void);

/**
 * @brief Update error statistics checksum (ASIL-D)
 *
 * Recalculates and updates the statistics checksum.
 */
void Com_Eh_UpdateStatsChecksum(void);

/******************************************************************************
 * Integration Macros
 ******************************************************************************/

/**
 * @brief Macro to check and handle Tx queue overflow
 *
 * This macro should be called in Com_TxQueueAddRequest before adding
 * a new request. It checks if the queue is full and applies the
 * configured overflow strategy.
 *
 * Usage:
 *   COM_EH_CHECK_TX_QUEUE_OVERFLOW(PduId, result);
 *   if (result == E_OK) {
 *       // Safe to add request
 *   }
 */
#define COM_EH_CHECK_TX_QUEUE_OVERFLOW(PduId, result) \
    do { \
        if (Com_TxQueueGetFillLevel() >= COM_MAX_TX_REQUESTS) { \
            Com_TxQueueOverflowStrategyType strategy = Com_Eh_GetOverflowStrategy(PduId); \
            Com_Eh_ReportTxQueueOverflow((PduId), strategy); \
            (result) = Com_Eh_ApplyOverflowStrategy((PduId), strategy); \
        } else { \
            (result) = E_OK; \
        } \
    } while(0)

/**
 * @brief Macro to initialize error handling in Com_Init
 */
#define COM_EH_INIT_IN_COM_INIT() \
    do { \
        Com_Eh_Init(); \
    } while(0)

/**
 * @brief Macro to process error handling in Com_MainFunctionTx
 *
 * Performs periodic error handling tasks.
 */
#define COM_EH_PROCESS_IN_MAINFUNCTIONTX() \
    do { \
        Com_GlobalErrorStats.CurrentQueueFillLevel = Com_TxQueueGetFillLevel(); \
        if (Com_GlobalErrorStats.CurrentQueueFillLevel > Com_GlobalErrorStats.MaxQueueFillLevel) { \
            Com_GlobalErrorStats.MaxQueueFillLevel = Com_GlobalErrorStats.CurrentQueueFillLevel; \
        } \
    } while(0)

/* Compatibility macro for internal use */
#define Com_Eh_ProcessInMainFunctionTx() COM_EH_PROCESS_IN_MAINFUNCTIONTX()

#ifdef __cplusplus
}
#endif

#endif /* COM_ERRORHANDLING_H */
