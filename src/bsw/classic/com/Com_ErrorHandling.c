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
/* @req SHALL_CLASSIC */


/******************************************************************************
 * @file    Com_ErrorHandling.c
 * @brief   COM Module - Error Handling and Queue Overflow Detection
 *
 * This file implements the error handling infrastructure for the AUTOSAR COM module.
 * Features:
 * - Send queue overflow detection and handling
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

/*==================[Includes]=============================================*/

#include "Com_ErrorHandling.h"
#include "Com_Transmit.h"
#include <string.h>

/*==================[Version Check]=========================================*/

#if (COM_SW_MAJOR_VERSION != COM_EH_SW_MAJOR_VERSION)
#error "Com_ErrorHandling.c: Major version mismatch with Com.h"
#endif

#if (COM_SW_MINOR_VERSION != COM_EH_SW_MINOR_VERSION)
#error "Com_ErrorHandling.c: Minor version mismatch with Com.h"
#endif

/*==================[Global Variables]=====================================*/

/** Global error statistics */
Com_GlobalErrorStatsType Com_GlobalErrorStats;

/** Error log buffer */
Com_ErrorLogEntryType Com_ErrorLog[COM_MAX_ERROR_LOG_ENTRIES];

/** Error log current index */
uint8 Com_ErrorLogIndex = 0u;

/** Error handling initialized flag */
static boolean Com_EhInitialized = FALSE;

/** ASIL-D: Redundant initialization flag */
static boolean Com_EhInitialized_Redundant = FALSE;

/*==================[Local Function Declarations]==========================*/

static uint16 Com_Eh_CalculateStatsChecksum(void);
static void Com_Eh_UpdatePerPduStats(Com_IPduIdType PduId, Com_TxQueueOverflowStrategyType Strategy);
static void Com_Eh_UpdateErrorRate(void);
static void Com_Eh_RecordOverflowTimestamp(void);

/*==================[Initialization]=======================================*/

/**
 * @brief Initialize error handling module
 */
void Com_Eh_Init(void)
{
    /* Reset error statistics */
    Com_Eh_ResetErrorStats();

    /* Clear error log */
    for (uint8 i = 0u; i < COM_MAX_ERROR_LOG_ENTRIES; i++) {
        Com_ErrorLog[i].ModuleId = 0u;
        Com_ErrorLog[i].ApiId = 0u;
        Com_ErrorLog[i].ErrorId = 0u;
        Com_ErrorLog[i].PduId = 0u;
        Com_ErrorLog[i].Timestamp = 0u;
        Com_ErrorLog[i].QueueFillLevel = 0u;
        Com_ErrorLog[i].Strategy = COM_TXQUEUE_REJECT_NEWEST;
    }

    /* Reset error log index */
    Com_ErrorLogIndex = 0u;

    /* ASIL-D: Set redundant initialization flags */
    Com_EhInitialized = TRUE;
    Com_EhInitialized_Redundant = TRUE;

    /* Calculate initial checksum */
    Com_Eh_UpdateStatsChecksum();
}

/**
 * @brief De-initialize error handling module
 */
void Com_Eh_DeInit(void)
{
    /* Reset error statistics */
    Com_Eh_ResetErrorStats();

    /* Clear error log */
    for (uint8 i = 0u; i < COM_MAX_ERROR_LOG_ENTRIES; i++) {
        Com_ErrorLog[i].ModuleId = 0u;
        Com_ErrorLog[i].ApiId = 0u;
        Com_ErrorLog[i].ErrorId = 0u;
        Com_ErrorLog[i].PduId = 0u;
        Com_ErrorLog[i].Timestamp = 0u;
        Com_ErrorLog[i].QueueFillLevel = 0u;
        Com_ErrorLog[i].Strategy = COM_TXQUEUE_REJECT_NEWEST;
    }

    Com_ErrorLogIndex = 0u;

    /* ASIL-D: Clear initialization flags */
    Com_EhInitialized = FALSE;
    Com_EhInitialized_Redundant = FALSE;
}

/*==================[Overflow Handling]====================================*/

/**
 * @brief Report a Tx queue overflow event
 */
Com_TxQueueOverflowStrategyType Com_Eh_ReportTxQueueOverflow(
    Com_IPduIdType PduId,
    Com_TxQueueOverflowStrategyType RequestedStrategy)
{
    Com_TxQueueOverflowStrategyType appliedStrategy;

    /* ASIL-D: Validate strategy */
    if (RequestedStrategy >= COM_TXQUEUE_NUM_STRATEGIES) {
        appliedStrategy = COM_TXQUEUE_REJECT_NEWEST;  /* Default to safe option */
    } else {
        appliedStrategy = RequestedStrategy;
    }

    /* Update overflow statistics */
    Com_GlobalErrorStats.TxQueueOverflowCount++;
    Com_GlobalErrorStats.TxQueueOverflowCount_Redund++;

    /* ASIL-D: Check redundant counter consistency */
    if (Com_GlobalErrorStats.TxQueueOverflowCount != Com_GlobalErrorStats.TxQueueOverflowCount_Redund) {
        /* Counter corruption detected - report critical error */
#if (COM_DEV_ERROR_DETECT == STD_ON)
        COM_REPORT_ERROR(COM_SERVICE_ID_EH_REPORT_OVERFLOW, COM_E_STATISTICS_CORRUPTION);
#endif
        /* Reset counters to safe state */
        Com_GlobalErrorStats.TxQueueOverflowCount = 0u;
        Com_GlobalErrorStats.TxQueueOverflowCount_Redund = 0u;
    }

    /* Update per-I-PDU statistics */
    if (PduId < COM_MAX_IPDUS) {
        Com_GlobalErrorStats.PerPduOverflowCount[PduId]++;
    }

    /* Record timestamp */
    Com_Eh_RecordOverflowTimestamp();

    /* Update error rate tracking */
    Com_Eh_UpdateErrorRate();

    /* Log the error */
    Com_Eh_LogError(COM_SERVICE_ID_EH_REPORT_OVERFLOW, COM_E_TX_QUEUE_OVERFLOW,
                    PduId, appliedStrategy);

    /* Report to DET */
#if (COM_DEV_ERROR_DETECT == STD_ON)
    Com_Eh_ReportDetError(COM_SERVICE_ID_EH_REPORT_OVERFLOW, COM_E_TX_QUEUE_OVERFLOW);
#endif

    /* Update specific counter based on strategy */
    switch (appliedStrategy) {
        case COM_TXQUEUE_REJECT_NEWEST:
        case COM_TXQUEUE_REJECT_OLDEST:
            Com_GlobalErrorStats.TxQueueRejectCount++;
            break;

        case COM_TXQUEUE_DROP_OLDEST:
            Com_GlobalErrorStats.TxQueueDropOldestCount++;
            break;

        case COM_TXQUEUE_DROP_NEWEST:
            Com_GlobalErrorStats.TxQueueDropNewestCount++;
            break;

        default:
            /* Should not reach here due to validation above */
            break;
    }

    /* Update checksum after modifications */
    Com_Eh_UpdateStatsChecksum();

    return appliedStrategy;
}

/**
 * @brief Apply overflow strategy to the Tx queue
 */
Std_ReturnType Com_Eh_ApplyOverflowStrategy(
    Com_IPduIdType PduId,
    Com_TxQueueOverflowStrategyType Strategy)
{
    Std_ReturnType result = E_NOT_OK;

    /* ASIL-D: Validate strategy */
    if (Strategy >= COM_TXQUEUE_NUM_STRATEGIES) {
        Com_Eh_ReportDetError(COM_SERVICE_ID_EH_APPLY_STRATEGY, COM_E_INVALID_OVERFLOW_STRATEGY);
        return E_NOT_OK;
    }

    switch (Strategy) {
        case COM_TXQUEUE_REJECT_NEWEST:
            /* Simply reject the new request - no queue modification */
            result = E_NOT_OK;  /* Signal caller that request was rejected */
            break;

        case COM_TXQUEUE_REJECT_OLDEST:
            /* Keep queue as is, reject the new request */
            result = E_NOT_OK;
            break;

        case COM_TXQUEUE_DROP_OLDEST:
            /* Remove oldest entry to make room */
            result = Com_Eh_DropOldestTxRequest();
            if (result == E_OK) {
                /* Space now available - caller should retry */
                result = E_OK;
            }
            break;

        case COM_TXQUEUE_DROP_NEWEST:
            /* Remove the most recent entry */
            result = Com_Eh_DropNewestTxRequest();
            if (result == E_OK) {
                /* Space now available */
                result = E_OK;
            }
            break;

        default:
            /* Invalid strategy */
            Com_Eh_ReportDetError(COM_SERVICE_ID_EH_APPLY_STRATEGY, COM_E_INVALID_OVERFLOW_STRATEGY);
            result = E_NOT_OK;
            break;
    }

    /* Update per-I-PDU statistics */
    Com_Eh_UpdatePerPduStats(PduId, Strategy);

    return result;
}

/**
 * @brief Drop oldest entry from Tx queue
 */
Std_ReturnType Com_Eh_DropOldestTxRequest(void)
{
    /* Check if queue is empty */
    if (Com_TxRequestQueue.Count == 0u) {
        return E_NOT_OK;
    }

    /* Get the head entry (oldest) */
    uint8 headIndex = Com_TxRequestQueue.Head;
    Com_TxRequestEntryType* entry = &Com_TxRequestQueue.Entries[headIndex];

    /* Verify entry is in a state that can be dropped */
    if ((entry->State != COM_TXREQ_PENDING) && (entry->State != COM_TXREQ_RETRY)) {
        /* Entry is not droppable (might be IN_PROGRESS) */
        /* Try to find next droppable entry */
        boolean found = FALSE;
        for (uint8 i = 1u; i < Com_TxRequestQueue.Count; i++) {
            uint8 checkIndex = (headIndex + i) % COM_MAX_TX_REQUESTS;
            if ((Com_TxRequestQueue.Entries[checkIndex].State == COM_TXREQ_PENDING) ||
                (Com_TxRequestQueue.Entries[checkIndex].State == COM_TXREQ_RETRY)) {
                /* Found a droppable entry - use this one instead */
                entry = &Com_TxRequestQueue.Entries[checkIndex];
                headIndex = checkIndex;
                found = TRUE;
                break;
            }
        }
        if (!found) {
            return E_NOT_OK;
        }
    }

    /* Mark entry as idle */
    entry->State = COM_TXREQ_IDLE;

    /* Adjust head if we dropped the actual head */
    if (headIndex == Com_TxRequestQueue.Head) {
        Com_TxRequestQueue.Head = (Com_TxRequestQueue.Head + 1u) % COM_MAX_TX_REQUESTS;
    }

    /* Decrement count */
    if (Com_TxRequestQueue.Count > 0u) {
        Com_TxRequestQueue.Count--;
    }

    /* Log the drop action */
    Com_Eh_LogError(COM_SERVICE_ID_EH_HANDLE_OVERFLOW, COM_E_TX_QUEUE_OVERFLOW,
                    entry->PduId, COM_TXQUEUE_DROP_OLDEST);

    return E_OK;
}

/**
 * @brief Drop newest entry from Tx queue
 */
Std_ReturnType Com_Eh_DropNewestTxRequest(void)
{
    /* Check if queue is empty */
    if (Com_TxRequestQueue.Count == 0u) {
        return E_NOT_OK;
    }

    /* Calculate index of newest entry */
    uint8 newestIndex;
    if (Com_TxRequestQueue.Tail == 0u) {
        newestIndex = COM_MAX_TX_REQUESTS - 1u;
    } else {
        newestIndex = Com_TxRequestQueue.Tail - 1u;
    }

    Com_TxRequestEntryType* entry = &Com_TxRequestQueue.Entries[newestIndex];

    /* Verify entry is in a state that can be dropped */
    if ((entry->State != COM_TXREQ_PENDING) && (entry->State != COM_TXREQ_RETRY)) {
        /* Entry is not droppable */
        return E_NOT_OK;
    }

    /* Mark entry as idle */
    entry->State = COM_TXREQ_IDLE;

    /* Adjust tail */
    if (Com_TxRequestQueue.Tail == 0u) {
        Com_TxRequestQueue.Tail = COM_MAX_TX_REQUESTS - 1u;
    } else {
        Com_TxRequestQueue.Tail--;
    }

    /* Decrement count */
    if (Com_TxRequestQueue.Count > 0u) {
        Com_TxRequestQueue.Count--;
    }

    /* Log the drop action */
    Com_Eh_LogError(COM_SERVICE_ID_EH_HANDLE_OVERFLOW, COM_E_TX_QUEUE_OVERFLOW,
                    entry->PduId, COM_TXQUEUE_DROP_NEWEST);

    return E_OK;
}

/*==================[Statistics Functions]=================================*/

/**
 * @brief Get current Tx queue status
 */
Std_ReturnType Com_Eh_GetTxQueueStatus(Com_TxQueueStatusType* StatusPtr)
{
    if (StatusPtr == NULL_PTR) {
        return E_NOT_OK;
    }

    StatusPtr->FillLevel = Com_TxRequestQueue.Count;
    StatusPtr->MaxFillLevel = COM_MAX_TX_REQUESTS;
    StatusPtr->IsFull = (Com_TxRequestQueue.Count >= COM_MAX_TX_REQUESTS);
    StatusPtr->IsEmpty = (Com_TxRequestQueue.Count == 0u);
    StatusPtr->Head = Com_TxRequestQueue.Head;
    StatusPtr->Tail = Com_TxRequestQueue.Tail;

    return E_OK;
}

/**
 * @brief Get global error statistics
 */
Std_ReturnType Com_Eh_GetErrorStats(Com_GlobalErrorStatsType* StatsPtr)
{
    if (StatsPtr == NULL_PTR) {
        return E_NOT_OK;
    }

    /* ASIL-D: Validate integrity before returning */
    if (Com_Eh_ValidateStatsIntegrity() != E_OK) {
        /* Statistics corrupted - report error */
#if (COM_DEV_ERROR_DETECT == STD_ON)
        COM_REPORT_ERROR(COM_SERVICE_ID_EH_GET_STATS, COM_E_STATISTICS_CORRUPTION);
#endif
        return E_NOT_OK;
    }

    /* Copy statistics */
    memcpy(StatsPtr, &Com_GlobalErrorStats, sizeof(Com_GlobalErrorStatsType));

    return E_OK;
}

/**
 * @brief Reset global error statistics
 */
void Com_Eh_ResetErrorStats(void)
{
    /* Clear all statistics */
    Com_GlobalErrorStats.TxQueueOverflowCount = 0u;
    Com_GlobalErrorStats.TxQueueOverflowCount_Redund = 0u;
    Com_GlobalErrorStats.TxQueueRejectCount = 0u;
    Com_GlobalErrorStats.TxQueueDropOldestCount = 0u;
    Com_GlobalErrorStats.TxQueueDropNewestCount = 0u;
    Com_GlobalErrorStats.DetReportCount = 0u;
    Com_GlobalErrorStats.InternalErrorCount = 0u;

    /* Clear per-I-PDU counters */
    for (uint16 i = 0u; i < COM_MAX_IPDUS; i++) {
        Com_GlobalErrorStats.PerPduOverflowCount[i] = 0u;
    }

    /* Reset error rate tracking */
    Com_GlobalErrorStats.ErrorWindowStartTime = Com_GetCurrentTimestamp();
    Com_GlobalErrorStats.ErrorsInWindow = 0u;
    Com_GlobalErrorStats.PeakErrorRate = 0u;

    /* Reset timestamps */
    Com_GlobalErrorStats.LastOverflowTimestamp = 0u;
    Com_GlobalErrorStats.LastErrorTimestamp = 0u;
    Com_GlobalErrorStats.FirstErrorTimestamp = 0u;

    /* Reset queue status */
    Com_GlobalErrorStats.MaxQueueFillLevel = 0u;
    Com_GlobalErrorStats.CurrentQueueFillLevel = 0u;

    /* Reset checksum */
    Com_GlobalErrorStats.StatisticsChecksum = 0u;
}

/*==================[Error Logging]========================================*/

/**
 * @brief Get error log entry
 */
Std_ReturnType Com_Eh_GetErrorLogEntry(uint8 Index, Com_ErrorLogEntryType* EntryPtr)
{
    if ((EntryPtr == NULL_PTR) || (Index >= COM_MAX_ERROR_LOG_ENTRIES)) {
        return E_NOT_OK;
    }

    /* Copy entry */
    memcpy(EntryPtr, &Com_ErrorLog[Index], sizeof(Com_ErrorLogEntryType));

    return E_OK;
}

/**
 * @brief Log an error entry
 */
void Com_Eh_LogError(uint8 ApiId, uint8 ErrorId, Com_IPduIdType PduId,
                     Com_TxQueueOverflowStrategyType Strategy)
{
    Com_ErrorLogEntryType* entry = &Com_ErrorLog[Com_ErrorLogIndex];

    /* Fill entry */
    entry->ModuleId = COM_MODULE_ID;
    entry->ApiId = ApiId;
    entry->ErrorId = ErrorId;
    entry->PduId = (PduId < COM_MAX_IPDUS) ? PduId : COM_MAX_IPDUS;
    entry->Timestamp = Com_GetCurrentTimestamp();
    entry->QueueFillLevel = (uint8)Com_TxRequestQueue.Count;
    entry->Strategy = Strategy;

    /* Advance index with wrap-around */
    Com_ErrorLogIndex++;
    if (Com_ErrorLogIndex >= COM_MAX_ERROR_LOG_ENTRIES) {
#if (COM_ERROR_LOG_WRAP_MODE == STD_ON)
        Com_ErrorLogIndex = 0u;  /* Wrap to start */
#else
        Com_ErrorLogIndex = COM_MAX_ERROR_LOG_ENTRIES - 1u;  /* Stay at last */
#endif
    }

    /* Update last error timestamp */
    Com_GlobalErrorStats.LastErrorTimestamp = entry->Timestamp;

    /* Set first error timestamp if not set */
    if (Com_GlobalErrorStats.FirstErrorTimestamp == 0u) {
        Com_GlobalErrorStats.FirstErrorTimestamp = entry->Timestamp;
    }
}

/**
 * @brief Report error to DET
 */
void Com_Eh_ReportDetError(uint8 ApiId, uint8 ErrorId)
{
#if (COM_DEV_ERROR_DETECT == STD_ON)
    /* Call DET */
    (void)Det_ReportError(COM_MODULE_ID, COM_INSTANCE_ID, ApiId, ErrorId);

    /* Update counter */
    Com_GlobalErrorStats.DetReportCount++;
#endif
}

/*==================[Configuration Access]=================================*/

/**
 * @brief Get overflow strategy for an I-PDU
 */
Com_TxQueueOverflowStrategyType Com_Eh_GetOverflowStrategy(Com_IPduIdType PduId)
{
    /* Validate PduId */
    if (PduId >= COM_MAX_IPDUS) {
        return COM_TXQUEUE_REJECT_NEWEST;  /* Safe default */
    }

    /* Get configured strategy */
    Com_TxQueueOverflowStrategyType strategy = Com_ErrorHandlingConfig[PduId].OverflowStrategy;

    /* Validate strategy */
    if (strategy >= COM_TXQUEUE_NUM_STRATEGIES) {
        return COM_TXQUEUE_REJECT_NEWEST;  /* Safe default */
    }

    return strategy;
}

/**
 * @brief Check if error rate is within acceptable limits
 */
boolean Com_Eh_IsErrorRateAcceptable(void)
{
    /* Error rate window in milliseconds (e.g., 1000ms = 1 second) */
    const uint32 ERROR_RATE_WINDOW_MS = 1000u;
    const uint32 MAX_ERRORS_PER_WINDOW = 10u;  /* Threshold */

    uint32 currentTime = Com_GetCurrentTimestamp();

    /* Check if window has expired */
    if ((currentTime - Com_GlobalErrorStats.ErrorWindowStartTime) > ERROR_RATE_WINDOW_MS) {
        /* Start new window */
        Com_GlobalErrorStats.ErrorWindowStartTime = currentTime;
        Com_GlobalErrorStats.ErrorsInWindow = 0u;
    }

    /* Check if error rate exceeds threshold */
    return (Com_GlobalErrorStats.ErrorsInWindow < MAX_ERRORS_PER_WINDOW);
}

/*==================[ASIL-D Safety Functions]==============================*/

/**
 * @brief Validate error statistics integrity
 */
Std_ReturnType Com_Eh_ValidateStatsIntegrity(void)
{
    /* Check redundant counters */
    if (Com_GlobalErrorStats.TxQueueOverflowCount != Com_GlobalErrorStats.TxQueueOverflowCount_Redund) {
        return E_NOT_OK;
    }

    /* Verify checksum */
    uint16 calculatedChecksum = Com_Eh_CalculateStatsChecksum();
    if (calculatedChecksum != Com_GlobalErrorStats.StatisticsChecksum) {
        return E_NOT_OK;
    }

    /* Bounds checking */
    if (Com_GlobalErrorStats.CurrentQueueFillLevel > COM_MAX_TX_REQUESTS) {
        return E_NOT_OK;
    }

    if (Com_GlobalErrorStats.MaxQueueFillLevel > COM_MAX_TX_REQUESTS) {
        return E_NOT_OK;
    }

    return E_OK;
}

/**
 * @brief Update error statistics checksum
 */
void Com_Eh_UpdateStatsChecksum(void)
{
    Com_GlobalErrorStats.StatisticsChecksum = Com_Eh_CalculateStatsChecksum();
}

/*==================[Local Functions]======================================*/

/**
 * @brief Calculate statistics checksum
 */
static uint16 Com_Eh_CalculateStatsChecksum(void)
{
    uint16 checksum = 0u;
    const uint8* data = (const uint8*)&Com_GlobalErrorStats;

    /* Calculate sum of all bytes except the checksum field itself */
    /* Size minus 2 bytes for checksum */
    size_t size = sizeof(Com_GlobalErrorStatsType) - sizeof(uint16);

    for (size_t i = 0u; i < size; i++) {
        checksum += data[i];
    }

    return checksum;
}

/**
 * @brief Update per-I-PDU statistics
 */
static void Com_Eh_UpdatePerPduStats(Com_IPduIdType PduId, Com_TxQueueOverflowStrategyType Strategy)
{
    (void)Strategy;  /* Strategy already handled in caller */

    if (PduId < COM_MAX_IPDUS) {
        /* Check for counter overflow */
        if (Com_GlobalErrorStats.PerPduOverflowCount[PduId] < 0xFFFFFFFFu) {
            Com_GlobalErrorStats.PerPduOverflowCount[PduId]++;
        } else {
            /* Counter overflow - reset to prevent wrap-around issues */
            Com_GlobalErrorStats.PerPduOverflowCount[PduId] = 0u;

            /* Report counter overflow error */
#if (COM_DEV_ERROR_DETECT == STD_ON)
            COM_REPORT_ERROR(COM_SERVICE_ID_EH_REPORT_OVERFLOW, COM_E_ERROR_COUNTER_OVERFLOW);
#endif
        }
    }
}

/**
 * @brief Update error rate tracking
 */
static void Com_Eh_UpdateErrorRate(void)
{
    const uint32 ERROR_RATE_WINDOW_MS = 1000u;
    uint32 currentTime = Com_GetCurrentTimestamp();

    /* Check if window has expired */
    if ((currentTime - Com_GlobalErrorStats.ErrorWindowStartTime) > ERROR_RATE_WINDOW_MS) {
        /* Update peak error rate */
        if (Com_GlobalErrorStats.ErrorsInWindow > Com_GlobalErrorStats.PeakErrorRate) {
            Com_GlobalErrorStats.PeakErrorRate = Com_GlobalErrorStats.ErrorsInWindow;
        }

        /* Start new window */
        Com_GlobalErrorStats.ErrorWindowStartTime = currentTime;
        Com_GlobalErrorStats.ErrorsInWindow = 1u;  /* Current error counts as 1 */
    } else {
        /* Increment error count in current window */
        Com_GlobalErrorStats.ErrorsInWindow++;
    }
}

/**
 * @brief Record overflow timestamp
 */
static void Com_Eh_RecordOverflowTimestamp(void)
{
    uint32 currentTime = Com_GetCurrentTimestamp();
    Com_GlobalErrorStats.LastOverflowTimestamp = currentTime;
    Com_GlobalErrorStats.LastErrorTimestamp = currentTime;

    /* Set first error timestamp if not set */
    if (Com_GlobalErrorStats.FirstErrorTimestamp == 0u) {
        Com_GlobalErrorStats.FirstErrorTimestamp = currentTime;
    }
}

/*==================[End of File]==========================================*/
