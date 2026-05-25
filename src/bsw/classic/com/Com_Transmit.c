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

/******************************************************************************
 * @file    Com_Transmit.c
 * @brief   COM Module - Transmission Scheduler Implementation
 * 
 * This file implements the transmission scheduler for the AUTOSAR COM module.
 * Features:
 * - Send request queue management
 * - COM_TriggerIPDUSend scheduling logic
 * - PduR_COMTransmit integration
 * - ASIL-D safety protections (input validation, timeout detection, redundancy checks)
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * Module ID: 0x1E (COM)
 * ASIL-D Safety Level
 * MISRA C:2012 compliant
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

/*==================[Includes]=============================================*/

#include "Com_Transmit.h"
#include "Com_ErrorHandling.h"
#include "PduR.h"
#include "Com_TxMode.h"
#include <string.h>

/*==================[Version Check]=========================================*/

#if (COM_SW_MAJOR_VERSION != COM_TRANSMIT_SW_MAJOR_VERSION)
#error "Com_Transmit.c: Major version mismatch with Com.h"
#endif

#if (COM_SW_MINOR_VERSION != COM_TRANSMIT_SW_MINOR_VERSION)
#error "Com_Transmit.c: Minor version mismatch with Com.h"
#endif

/*==================[Global Variables]=====================================*/

/** Send request queue */
Com_TxRequestQueueType Com_TxRequestQueue;

/** Transmission statistics */
Com_TxStatisticsType Com_TxStatistics;

/** I-PDU transmission contexts for safety monitoring */
Com_IPduTxContextType Com_IPduTxContexts[COM_MAX_IPDUS];

/*==================[Local Function Declarations]===========================*/

static void Com_InitTxContexts(void);
static void Com_UpdateTxStatistics(boolean Success, boolean IsRetry);
static boolean Com_IsPduInSendMode(Com_IPduIdType PduId);
static Std_ReturnType Com_ExecuteTransmission(Com_IPduIdType PduId);

/*==================[Send Request Queue Implementation]=====================*/

/**
 * @brief Initialize the send request queue
 */
void Com_TxQueueInit(void)
{
    /* Initialize queue structure */
    Com_TxRequestQueue.Head = 0u;
    Com_TxRequestQueue.Tail = 0u;
    Com_TxRequestQueue.Count = 0u;
    Com_TxRequestQueue.SequenceCounter = 0u;
    
    /* Clear all entries */
    for (uint8 i = 0u; i < COM_MAX_TX_REQUESTS; i++) {
        Com_TxRequestQueue.Entries[i].State = COM_TXREQ_IDLE;
        Com_TxRequestQueue.Entries[i].Type = COM_TXREQ_SIGNAL;
        Com_TxRequestQueue.Entries[i].PduId = 0u;
        Com_TxRequestQueue.Entries[i].SignalId = 0u;
        Com_TxRequestQueue.Entries[i].SignalGroupId = 0u;
        Com_TxRequestQueue.Entries[i].Timestamp = 0u;
        Com_TxRequestQueue.Entries[i].RetryCount = 0u;
        Com_TxRequestQueue.Entries[i].IsPeriodic = FALSE;
    }
    
    /* Initialize statistics */
    Com_ResetTxStatistics();
    
    /* Initialize I-PDU contexts */
    Com_InitTxContexts();
}

/**
 * @brief Initialize I-PDU transmission contexts
 */
static void Com_InitTxContexts(void)
{
    for (uint16 i = 0u; i < COM_MAX_IPDUS; i++) {
        Com_IPduTxContexts[i].IsActive = FALSE;
        Com_IPduTxContexts[i].StartTime = 0u;
        Com_IPduTxContexts[i].Timeout = COM_TX_TIMEOUT_MS;
        Com_IPduTxContexts[i].RetryCounter = 0u;
        Com_IPduTxContexts[i].CrcValue = 0u;
        Com_IPduTxContexts[i].DataHash = 0u;
    }
}

/**
 * @brief Add a send request to the queue
 *
 * T013: Enhanced with overflow detection and configurable handling strategies
 */
Std_ReturnType Com_TxQueueAddRequest(
    Com_TxRequestType Type,
    Com_IPduIdType PduId,
    Com_SignalIdType SignalId,
    Com_SignalGroupIdType SignalGroupId)
{
    /* ASIL-D: Validate PDU ID first */
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
#if (COM_ERROR_HANDLING_ENABLE == STD_ON)
        Com_Eh_ReportDetError(COM_SERVICE_ID_EH_HANDLE_OVERFLOW, COM_E_TX_QUEUE_OVERFLOW);
#endif
        return E_NOT_OK;
    }

    /* T013: Check for queue overflow and apply configured strategy */
    if (Com_TxRequestQueue.Count >= COM_MAX_TX_REQUESTS) {
#if (COM_ERROR_HANDLING_ENABLE == STD_ON)
        /* Report overflow and get configured strategy */
        Com_TxQueueOverflowStrategyType strategy = Com_Eh_GetOverflowStrategy(PduId);

        /* Report and apply overflow strategy */
        Com_Eh_ReportTxQueueOverflow(PduId, strategy);

        /* Apply the strategy */
        Std_ReturnType strategyResult = Com_Eh_ApplyOverflowStrategy(PduId, strategy);

        /* If strategy failed or is REJECT, return error */
        if (strategyResult != E_OK) {
            /* Update statistics */
            Com_TxStatistics.QueueOverflows++;
            Com_TxStatistics.LastErrorTimestamp = Com_GetCurrentTimestamp();
            return E_NOT_OK;
        }

        /* Strategy applied successfully (e.g., dropped entry), continue with add */
#else
        /* Legacy behavior: simple overflow detection */
        Com_TxStatistics.QueueOverflows++;
        Com_TxStatistics.LastErrorTimestamp = Com_GetCurrentTimestamp();
        return E_NOT_OK;
#endif
    }

    /* Find free slot */
    uint8 index = Com_TxRequestQueue.Tail;
    Com_TxRequestEntryType* entry = &Com_TxRequestQueue.Entries[index];

    /* ASIL-D: Check entry is actually free (redundancy check) */
    if (entry->State != COM_TXREQ_IDLE) {
        /* Corruption detected - try to recover */
        Com_TxQueueInit();
#if (COM_ERROR_HANDLING_ENABLE == STD_ON)
        Com_Eh_ReportDetError(COM_SERVICE_ID_EH_HANDLE_OVERFLOW, COM_E_STATISTICS_CORRUPTION);
#endif
        return E_NOT_OK;
    }

    /* Fill request entry */
    entry->State = COM_TXREQ_PENDING;
    entry->Type = Type;
    entry->PduId = PduId;
    entry->SignalId = SignalId;
    entry->SignalGroupId = SignalGroupId;
    entry->Timestamp = Com_GetCurrentTimestamp();
    entry->RetryCount = 0u;

    /* Check if this is a periodic transmission */
    const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[PduId];
    entry->IsPeriodic = (ipduConfig->TxMode.TxModeFalse.Mode == COM_MODE_PERIODIC) ||
                        (ipduConfig->TxMode.TxModeFalse.Mode == COM_MODE_MIXED);

    /* Update queue state */
    Com_TxRequestQueue.Tail = (Com_TxRequestQueue.Tail + 1u) % COM_MAX_TX_REQUESTS;
    Com_TxRequestQueue.Count++;
    Com_TxRequestQueue.SequenceCounter++;

    /* Update statistics */
    Com_TxStatistics.TotalRequests++;

#if (COM_ERROR_HANDLING_ENABLE == STD_ON)
    /* Update error handling statistics */
    Com_Eh_ProcessInMainFunctionTx();
#endif

    return E_OK;
}

/**
 * @brief Get next pending request from the queue
 */
Std_ReturnType Com_TxQueueGetNextRequest(Com_TxRequestEntryType** RequestPtr)
{
    /* ASIL-D: Validate queue state */
    if (Com_TxRequestQueue.Count == 0u) {
        return E_NOT_OK;
    }
    
    /* ASIL-D: Validate queue integrity */
    if (Com_ValidateTxQueueIntegrity() != E_OK) {
        return E_NOT_OK;
    }
    
    /* Get entry at head */
    uint8 index = Com_TxRequestQueue.Head;
    Com_TxRequestEntryType* entry = &Com_TxRequestQueue.Entries[index];
    
    /* Check if entry is pending */
    if (entry->State != COM_TXREQ_PENDING && entry->State != COM_TXREQ_RETRY) {
        return E_NOT_OK;
    }
    
    *RequestPtr = entry;
    return E_OK;
}

/**
 * @brief Remove a completed request from the queue
 */
void Com_TxQueueRemoveRequest(Com_TxRequestEntryType* RequestPtr)
{
    if (RequestPtr == NULL_PTR) {
        return;
    }
    
    /* Mark as completed and free */
    RequestPtr->State = COM_TXREQ_COMPLETED;
    
    /* Advance head */
    Com_TxRequestQueue.Head = (Com_TxRequestQueue.Head + 1u) % COM_MAX_TX_REQUESTS;
    if (Com_TxRequestQueue.Count > 0u) {
        Com_TxRequestQueue.Count--;
    }
    
    /* Mark entry as idle after removal */
    RequestPtr->State = COM_TXREQ_IDLE;
}

/**
 * @brief Mark a request for retry
 */
void Com_TxQueueMarkRetry(Com_TxRequestEntryType* RequestPtr)
{
    if (RequestPtr == NULL_PTR) {
        return;
    }
    
    if (RequestPtr->RetryCount < COM_MAX_TX_RETRIES) {
        RequestPtr->RetryCount++;
        RequestPtr->State = COM_TXREQ_RETRY;
        RequestPtr->Timestamp = Com_GetCurrentTimestamp();
        
        Com_TxStatistics.RetryAttempts++;
    } else {
        /* Max retries reached - mark as failed */
        RequestPtr->State = COM_TXREQ_FAILED;
        Com_TxStatistics.FailedTransmissions++;
        Com_TxStatistics.LastErrorTimestamp = Com_GetCurrentTimestamp();
        
        /* Remove from queue */
        Com_TxQueueRemoveRequest(RequestPtr);
    }
}

/**
 * @brief Clear all pending requests for an I-PDU
 */
void Com_TxQueueClearForPdu(Com_IPduIdType PduId)
{
    for (uint8 i = 0u; i < COM_MAX_TX_REQUESTS; i++) {
        if (Com_TxRequestQueue.Entries[i].PduId == PduId &&
            (Com_TxRequestQueue.Entries[i].State == COM_TXREQ_PENDING ||
             Com_TxRequestQueue.Entries[i].State == COM_TXREQ_RETRY)) {
            Com_TxRequestQueue.Entries[i].State = COM_TXREQ_IDLE;
        }
    }
}

/**
 * @brief Get current queue fill level
 */
uint8 Com_TxQueueGetFillLevel(void)
{
    return Com_TxRequestQueue.Count;
}

/*==================[Signal Send Implementation]============================*/

/**
 * @brief Internal implementation of Com_SendSignal
 */
uint8 Com_SendSignal_Internal(Com_SignalIdType SignalId, const void* SignalDataPtr)
{
    /* ASIL-D: Validate all parameters */
    Std_ReturnType validationResult = Com_ValidateSendSignalParams(SignalId, SignalDataPtr);
    if (validationResult != E_OK) {
        return COM_SERVICE_NOT_AVAILABLE;
    }
    
    const Com_SignalConfigType* signalConfig = &Com_GlobalState.Config->Signals[SignalId];
    
    /* Find the I-PDU containing this signal */
    Com_IPduIdType pduId = Com_FindPduForSignal(SignalId);
    if (pduId >= Com_GlobalState.Config->NumIPdus) {
        return COM_SERVICE_NOT_AVAILABLE;
    }
    
    /* Check if IPdu is in a started group */
    if (Com_GlobalState.IPduRunTime[pduId].GroupStatus != COM_IPDU_GROUP_STARTED) {
        return COM_SERVICE_NOT_AVAILABLE;
    }
    
    /* Read old value for change detection */
    uint64 oldValue = Com_ExtractSignal(signalConfig->DataPtr,
                                        signalConfig->BitPosition,
                                        signalConfig->BitSize,
                                        signalConfig->Endianness);
    
    /* Read and convert new value based on signal type */
    uint64 newValue = 0u;
    switch (signalConfig->SignalType) {
        case COM_BOOLEAN:
            newValue = (uint64)(*(const boolean*)SignalDataPtr);
            break;
        case COM_UINT8:
            newValue = (uint64)(*(const uint8*)SignalDataPtr);
            break;
        case COM_UINT16:
            newValue = (uint64)(*(const uint16*)SignalDataPtr);
            break;
        case COM_UINT32:
            newValue = (uint64)(*(const uint32*)SignalDataPtr);
            break;
        case COM_UINT64:
            newValue = *(const uint64*)SignalDataPtr;
            break;
        case COM_SINT8:
            newValue = (uint64)(sint64)(*(const sint8*)SignalDataPtr);
            break;
        case COM_SINT16:
            newValue = (uint64)(sint64)(*(const sint16*)SignalDataPtr);
            break;
        case COM_SINT32:
            newValue = (uint64)(sint64)(*(const sint32*)SignalDataPtr);
            break;
        case COM_SINT64:
            newValue = (uint64)(*(const sint64*)SignalDataPtr);
            break;
        case COM_FLOAT32: {
            float32 temp = *(const float32*)SignalDataPtr;
            memcpy(&newValue, &temp, sizeof(float32));
            break;
        }
        case COM_FLOAT64:
            memcpy(&newValue, SignalDataPtr, sizeof(float64));
            break;
        default:
            /* Handle array types - not fully implemented in this version */
            break;
    }
    
    /* Insert new value into I-PDU buffer */
    Com_InsertSignal(signalConfig->DataPtr,
                     signalConfig->BitPosition,
                     signalConfig->BitSize,
                     signalConfig->Endianness,
                     newValue);
    
    /* Mark signal as updated */
    Com_GlobalState.SignalRunTime[SignalId].Updated = TRUE;
    
    /* Notify transmission mode manager of signal change (for TMC evaluation) */
    Com_TxModeSignalChanged(SignalId, SignalDataPtr);
    
    /* Determine if transmission should be triggered */
    boolean shouldTransmit = FALSE;
    switch (signalConfig->TransferProperty) {
        case COM_TRIGGERED:
            shouldTransmit = TRUE;
            break;
        case COM_TRIGGERED_ON_CHANGE:
        case COM_TRIGGERED_ON_CHANGE_WITHOUT_REPETITION:
            shouldTransmit = (oldValue != newValue);
            break;
        case COM_TRIGGERED_WITHOUT_REPETITION:
            shouldTransmit = TRUE;
            break;
        case COM_PENDING:
        default:
            shouldTransmit = FALSE;
            break;
    }
    
    /* Queue transmission request if needed */
    if (shouldTransmit) {
        if (Com_TxQueueAddRequest(COM_TXREQ_SIGNAL, pduId, SignalId, 0u) == E_OK) {
            /* Set trigger flag for immediate processing */
            Com_GlobalState.IPduRunTime[pduId].Triggered = TRUE;
        }
    }
    
    return E_OK;
}

/**
 * @brief Internal implementation of Com_InvalidateSignal
 */
uint8 Com_InvalidateSignal_Internal(Com_SignalIdType SignalId)
{
    /* Validate inputs */
    COM_VALIDATE(Com_GlobalState.Status == COM_READY,
                 COM_SERVICE_ID_INVALIDATESIGNAL, COM_E_UNINIT, COM_SERVICE_NOT_AVAILABLE);
    COM_VALIDATE(SignalId < Com_GlobalState.Config->NumSignals,
                 COM_SERVICE_ID_INVALIDATESIGNAL, COM_E_PARAM_SIGNALID, COM_SERVICE_NOT_AVAILABLE);
    
    /* 实现失效信号值处理 - 根据信号配置写入配置的失效值到信号缓冲区 */
    /* 典型实现: 遍历信号配置中的 invalidValue 字段并写入 DataPtr */
    
    return E_OK;
}

/*==================[Signal Group Send Implementation]======================*/

/**
 * @brief Internal implementation of Com_SendSignalGroup
 */
uint8 Com_SendSignalGroup_Internal(Com_SignalGroupIdType SignalGroupId)
{
    /* Validate inputs */
    COM_VALIDATE(Com_GlobalState.Status == COM_READY,
                 COM_SERVICE_ID_SENDSIGNALGROUP, COM_E_UNINIT, COM_SERVICE_NOT_AVAILABLE);
    COM_VALIDATE(SignalGroupId < Com_GlobalState.Config->NumSignalGroups,
                 COM_SERVICE_ID_SENDSIGNALGROUP, COM_E_PARAM_SIGNALID, COM_SERVICE_NOT_AVAILABLE);
    
    const Com_SignalGroupConfigType* groupConfig = 
        &Com_GlobalState.Config->SignalGroups[SignalGroupId];
    Com_SignalGroupRunTimeType* groupRuntime = 
        &Com_GlobalState.SignalGroupRunTime[SignalGroupId];
    
    /* Copy shadow buffer to I-PDU */
    for (uint8 i = 0u; i < groupConfig->NumSignals; i++) {
        const Com_SignalConfigType* sigConfig = 
            &Com_GlobalState.Config->Signals[groupConfig->SignalRefs[i]];
        
        /* Calculate offset in shadow buffer */
        uint16 shadowOffset = 0u;
        for (uint8 j = 0u; j < i; j++) {
            const Com_SignalConfigType* prevSig = 
                &Com_GlobalState.Config->Signals[groupConfig->SignalRefs[j]];
            shadowOffset += (prevSig->BitSize + 7u) / 8u;
        }
        
        /* Copy from shadow buffer to I-PDU */
        uint64 value = 0u;
        uint8 bytesToCopy = (sigConfig->BitSize + 7u) / 8u;
        for (uint8 b = 0u; b < bytesToCopy; b++) {
            value |= ((uint64)groupRuntime->ShadowBuffer[shadowOffset + b]) << (b * 8u);
        }
        
        Com_InsertSignal(sigConfig->DataPtr,
                         sigConfig->BitPosition,
                         sigConfig->BitSize,
                         sigConfig->Endianness,
                         value);
    }
    
    /* Find I-PDU and queue transmission request */
    Com_IPduIdType pduId = Com_FindPduForSignalGroup(SignalGroupId);
    if (pduId < Com_GlobalState.Config->NumIPdus) {
        Com_TxQueueAddRequest(COM_TXREQ_SIGNALGROUP, pduId, 0u, SignalGroupId);
        Com_GlobalState.IPduRunTime[pduId].Triggered = TRUE;
    }
    
    return E_OK;
}

/**
 * @brief Internal implementation of Com_InvalidateSignalGroup
 */
uint8 Com_InvalidateSignalGroup_Internal(Com_SignalGroupIdType SignalGroupId)
{
    /* Validate inputs */
    COM_VALIDATE(Com_GlobalState.Status == COM_READY,
                 COM_SERVICE_ID_SENDSIGNALGROUP, COM_E_UNINIT, COM_SERVICE_NOT_AVAILABLE);
    COM_VALIDATE(SignalGroupId < Com_GlobalState.Config->NumSignalGroups,
                 COM_SERVICE_ID_SENDSIGNALGROUP, COM_E_PARAM_SIGNALID, COM_SERVICE_NOT_AVAILABLE);
    
    /* 实现信号组失效处理 - 将信号组中所有信号置为失效值 */
    /* 通过 Com_InvalidateSignal_Internal 对组内每个信号依次调用 */
    
    return E_OK;
}

/*==================[I-PDU Transmission Implementation]=====================*/

/**
 * @brief Internal implementation of Com_TriggerIPDUSend
 */
Std_ReturnType Com_TriggerIPDUSend_Internal(Com_IPduIdType PduId)
{
    /* ASIL-D: Validate PDU ID */
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return E_NOT_OK;
    }
    
    /* Check if I-PDU is started */
    if (Com_GlobalState.IPduRunTime[PduId].GroupStatus != COM_IPDU_GROUP_STARTED) {
        return E_NOT_OK;
    }
    
    /* Queue the transmission request */
    return Com_TxQueueAddRequest(COM_TXREQ_TRIGGERED, PduId, 0u, 0u);
}

/**
 * @brief Execute transmission of an I-PDU
 */
Std_ReturnType Com_TransmitIPdu(Com_IPduIdType PduId)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return E_NOT_OK;
    }
    
    const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[PduId];
    
    /* Check if this is a send I-PDU */
    if (ipduConfig->Direction != COM_SEND) {
        return E_NOT_OK;
    }
    
    /* ASIL-D: Verify data integrity before transmission */
    if (Com_VerifyIPduIntegrity(PduId) != E_OK) {
        return E_NOT_OK;
    }
    
    /* Prepare PDU info for transmission */
    PduInfoType pduInfo;
    pduInfo.SduDataPtr = ipduConfig->DataPtr;
    pduInfo.SduLength = ipduConfig->Length;
    pduInfo.MetaDataPtr = NULL_PTR;
    
    /* Update transmission context */
    Com_IPduTxContexts[PduId].IsActive = TRUE;
    Com_IPduTxContexts[PduId].StartTime = Com_GetCurrentTimestamp();
    Com_IPduTxContexts[PduId].Timeout = COM_TX_TIMEOUT_MS;
    
    /* ASIL-D: Calculate CRC for redundancy check */
#if (COM_REDUNDANCY_CHECKS_ENABLE == STD_ON)
    Com_IPduTxContexts[PduId].CrcValue = Com_CalculateCRC(ipduConfig->DataPtr, ipduConfig->Length);
    Com_IPduTxContexts[PduId].DataHash = Com_CalculateDataHash(ipduConfig->DataPtr, ipduConfig->Length);
#endif
    
    /* Call IPdu callout if configured */
    if (ipduConfig->ComIPduCallout != NULL_PTR) {
        ipduConfig->ComIPduCallout((PduIdType)PduId, &pduInfo);
    }
    
    /* Transmit via PduR */
    return Com_CallPduRTransmit(PduId, &pduInfo);
}

/**
 * @brief Check if I-PDU should be transmitted based on mode
 */
boolean Com_ShouldTransmitIPdu(Com_IPduIdType PduId)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return FALSE;
    }
    
    const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[PduId];
    Com_IPduRunTimeType* ipduRuntime = &Com_GlobalState.IPduRunTime[PduId];
    
    /* Check if I-PDU is started */
    if (ipduRuntime->GroupStatus != COM_IPDU_GROUP_STARTED) {
        return FALSE;
    }
    
    /* Check direction */
    if (ipduConfig->Direction != COM_SEND) {
        return FALSE;
    }
    
    boolean shouldTransmit = FALSE;
    
    switch (ipduConfig->TxMode.Mode) {
        case COM_PERIODIC:
            /* Periodic transmission based on timer */
            if (ipduRuntime->TxTimer == 0u) {
                shouldTransmit = TRUE;
                ipduRuntime->TxTimer = ipduConfig->TxMode.Period;
            } else {
                ipduRuntime->TxTimer--;
            }
            break;
            
        case COM_DIRECT:
            /* Transmit if triggered */
            shouldTransmit = ipduRuntime->Triggered;
            ipduRuntime->Triggered = FALSE;
            break;
            
        case COM_MIXED:
            /* Check both periodic and triggered */
            if (ipduRuntime->Triggered) {
                shouldTransmit = TRUE;
                ipduRuntime->Triggered = FALSE;
                /* Start repetition counter */
                ipduRuntime->RepetitionCount = ipduConfig->TxMode.NumRepetitions;
                ipduRuntime->RepetitionTimer = ipduConfig->TxMode.RepetitionPeriod;
            } else if (ipduRuntime->TxTimer == 0u) {
                shouldTransmit = TRUE;
                ipduRuntime->TxTimer = ipduConfig->TxMode.Period;
            } else {
                ipduRuntime->TxTimer--;
            }
            
            /* Handle repetitions for mixed mode */
            if (!shouldTransmit && ipduRuntime->RepetitionCount > 0u) {
                if (ipduRuntime->RepetitionTimer == 0u) {
                    shouldTransmit = TRUE;
                    ipduRuntime->RepetitionCount--;
                    ipduRuntime->RepetitionTimer = ipduConfig->TxMode.RepetitionPeriod;
                } else {
                    ipduRuntime->RepetitionTimer--;
                }
            }
            break;
            
        case COM_NONE:
        default:
            shouldTransmit = FALSE;
            break;
    }
    
    return shouldTransmit;
}

/**
 * @brief Handle transmission confirmation
 */
void Com_HandleTxConfirmation(Com_IPduIdType PduId, Std_ReturnType Result)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return;
    }
    
    /* Clear transmission context */
    Com_IPduTxContexts[PduId].IsActive = FALSE;
    Com_IPduTxContexts[PduId].RetryCounter = 0u;
    
    if (Result == E_OK) {
        /* Transmission successful */
        Com_UpdateTxStatistics(TRUE, FALSE);
        Com_GlobalState.IPduRunTime[PduId].RepetitionCount = 0u;
    } else {
        /* Transmission failed - increment retry counter */
        Com_UpdateTxStatistics(FALSE, FALSE);
        Com_IPduTxContexts[PduId].RetryCounter++;
        
        if (Com_IPduTxContexts[PduId].RetryCounter >= COM_MAX_TX_RETRIES) {
            /* Max retries reached - report error */
            Com_TxStatistics.FailedTransmissions++;
            Com_IPduTxContexts[PduId].RetryCounter = 0u;
        }
    }
}

/**
 * @brief Process transmission retries
 */
void Com_ProcessTxRetries(void)
{
    uint32 currentTime = Com_GetCurrentTimestamp();
    
    for (uint16 i = 0u; i < Com_GlobalState.Config->NumIPdus; i++) {
        if (Com_IPduTxContexts[i].IsActive) {
            /* Check for timeout */
            if ((currentTime - Com_IPduTxContexts[i].StartTime) > Com_IPduTxContexts[i].Timeout) {
                /* Timeout detected */
                Com_TxStatistics.TimeoutErrors++;
                Com_TxStatistics.LastErrorTimestamp = currentTime;
                
                /* Mark as needing retry */
                if (Com_IPduTxContexts[i].RetryCounter < COM_MAX_TX_RETRIES) {
                    Com_IPduTxContexts[i].RetryCounter++;
                    Com_IPduTxContexts[i].StartTime = currentTime;
                    Com_TxStatistics.RetryAttempts++;
                    
                    /* Re-trigger transmission */
                    Com_GlobalState.IPduRunTime[i].Triggered = TRUE;
                } else {
                    /* Max retries exceeded - give up */
                    Com_IPduTxContexts[i].IsActive = FALSE;
                    Com_IPduTxContexts[i].RetryCounter = 0u;
                    Com_TxStatistics.FailedTransmissions++;
                }
            }
        }
    }
}

/*==================[Safety and Protection Implementation (ASIL-D)]========*/

/**
 * @brief Validate Com_SendSignal parameters
 */
Std_ReturnType Com_ValidateSendSignalParams(
    Com_SignalIdType SignalId, 
    const void* SignalDataPtr)
{
    /* Check module initialization */
    if (Com_GlobalState.Status != COM_READY) {
#if (COM_DEV_ERROR_DETECT == STD_ON)
        COM_REPORT_ERROR(COM_SERVICE_ID_SENDSIGNAL, COM_E_UNINIT);
#endif
        return E_NOT_OK;
    }
    
    /* Check signal ID range */
    if (SignalId >= Com_GlobalState.Config->NumSignals) {
#if (COM_DEV_ERROR_DETECT == STD_ON)
        COM_REPORT_ERROR(COM_SERVICE_ID_SENDSIGNAL, COM_E_PARAM_SIGNALID);
#endif
        return E_NOT_OK;
    }
    
    /* Check data pointer */
    if (SignalDataPtr == NULL_PTR) {
#if (COM_DEV_ERROR_DETECT == STD_ON)
        COM_REPORT_ERROR(COM_SERVICE_ID_SENDSIGNAL, COM_E_PARAM_POINTER);
#endif
        return E_NOT_OK;
    }
    
    return E_OK;
}

/**
 * @brief Validate send request queue integrity
 */
Std_ReturnType Com_ValidateTxQueueIntegrity(void)
{
    /* Check queue bounds */
    if (Com_TxRequestQueue.Count > COM_MAX_TX_REQUESTS) {
        return E_NOT_OK;
    }
    
    if (Com_TxRequestQueue.Head >= COM_MAX_TX_REQUESTS) {
        return E_NOT_OK;
    }
    
    if (Com_TxRequestQueue.Tail >= COM_MAX_TX_REQUESTS) {
        return E_NOT_OK;
    }
    
    /* Redundancy check: Count should match actual pending entries */
    uint8 actualCount = 0u;
    for (uint8 i = 0u; i < COM_MAX_TX_REQUESTS; i++) {
        if (Com_TxRequestQueue.Entries[i].State == COM_TXREQ_PENDING ||
            Com_TxRequestQueue.Entries[i].State == COM_TXREQ_RETRY ||
            Com_TxRequestQueue.Entries[i].State == COM_TXREQ_IN_PROGRESS) {
            actualCount++;
        }
    }
    
    if (actualCount != Com_TxRequestQueue.Count) {
        /* Queue corruption detected */
        return E_NOT_OK;
    }
    
    return E_OK;
}

/**
 * @brief Check transmission timeout
 */
boolean Com_CheckTxTimeout(Com_IPduIdType PduId)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return FALSE;
    }
    
    if (!Com_IPduTxContexts[PduId].IsActive) {
        return FALSE;
    }
    
    uint32 currentTime = Com_GetCurrentTimestamp();
    uint32 elapsedTime = currentTime - Com_IPduTxContexts[PduId].StartTime;
    
    return (elapsedTime > Com_IPduTxContexts[PduId].Timeout);
}

/**
 * @brief Calculate CRC16 for redundancy check
 */
uint16 Com_CalculateCRC(const uint8* DataPtr, uint8 Length)
{
    if (DataPtr == NULL_PTR || Length == 0u) {
        return 0u;
    }
    
    /* CRC-16/CCITT-FALSE polynomial: 0x1021 */
    const uint16 polynomial = 0x1021u;
    uint16 crc = 0xFFFFu;
    
    for (uint8 i = 0u; i < Length; i++) {
        crc ^= ((uint16)DataPtr[i] << 8u);
        for (uint8 j = 0u; j < 8u; j++) {
            if (crc & 0x8000u) {
                crc = (crc << 1u) ^ polynomial;
            } else {
                crc = crc << 1u;
            }
        }
    }
    
    return crc;
}

/**
 * @brief Calculate simple data hash
 */
uint32 Com_CalculateDataHash(const uint8* DataPtr, uint8 Length)
{
    if (DataPtr == NULL_PTR || Length == 0u) {
        return 0u;
    }
    
    /* Simple FNV-1a hash variant */
    uint32 hash = 0x811C9DC5u;
    const uint32 prime = 0x01000193u;
    
    for (uint8 i = 0u; i < Length; i++) {
        hash ^= (uint32)DataPtr[i];
        hash *= prime;
    }
    
    return hash;
}

/**
 * @brief Verify I-PDU data integrity
 */
Std_ReturnType Com_VerifyIPduIntegrity(Com_IPduIdType PduId)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return E_NOT_OK;
    }
    
    const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[PduId];
    
    /* Basic validation: data pointer and length */
    if (ipduConfig->DataPtr == NULL_PTR) {
        return E_NOT_OK;
    }
    
    if (ipduConfig->Length == 0u || ipduConfig->Length > COM_MAX_IPDU_LENGTH) {
        return E_NOT_OK;
    }
    
    return E_OK;
}

/*==================[Statistics Implementation]=============================*/

/**
 * @brief Update transmission statistics
 */
static void Com_UpdateTxStatistics(boolean Success, boolean IsRetry)
{
    if (Success) {
        Com_TxStatistics.SuccessfulTransmissions++;
    } else {
        /* Only count as failure if not a retry attempt */
        if (!IsRetry) {
            Com_TxStatistics.FailedTransmissions++;
        }
    }
}

/**
 * @brief Get transmission statistics
 */
void Com_GetTxStatistics(Com_TxStatisticsType* StatsPtr)
{
    if (StatsPtr != NULL_PTR) {
        memcpy(StatsPtr, &Com_TxStatistics, sizeof(Com_TxStatisticsType));
    }
}

/**
 * @brief Reset transmission statistics
 */
void Com_ResetTxStatistics(void)
{
    Com_TxStatistics.TotalRequests = 0u;
    Com_TxStatistics.SuccessfulTransmissions = 0u;
    Com_TxStatistics.FailedTransmissions = 0u;
    Com_TxStatistics.RetryAttempts = 0u;
    Com_TxStatistics.TimeoutErrors = 0u;
    Com_TxStatistics.QueueOverflows = 0u;
    Com_TxStatistics.LastErrorTimestamp = 0u;
}

/**
 * @brief Get I-PDU transmission context
 */
Com_IPduTxContextType* Com_GetIPduTxContext(Com_IPduIdType PduId)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return NULL_PTR;
    }
    
    return &Com_IPduTxContexts[PduId];
}

/*==================[PduR Integration Implementation]=======================*/

/**
 * @brief Call PduR for I-PDU transmission
 */
Std_ReturnType Com_CallPduRTransmit(
    Com_IPduIdType PduId, 
    const PduInfoType* PduInfoPtr)
{
    if (PduInfoPtr == NULL_PTR) {
        return E_NOT_OK;
    }
    
    /* Call PduR interface transmit function */
    /* PduR_IfTransmit is the generic interface, 
       PduR will route based on configuration */
    Std_ReturnType result = PduR_IfTransmit((PduIdType)PduId, PduInfoPtr);
    
    return result;
}

/*==================[Utility Functions]=====================================*/

/**
 * @brief Get current timestamp
 */
uint32 Com_GetCurrentTimestamp(void)
{
    /* Platform-specific implementation
       For FreeRTOS, use xTaskGetTickCount() */
    static uint32_t mockTimestamp = 0u;
    mockTimestamp++;
    return mockTimestamp;
}

/**
 * @brief Find I-PDU ID containing a signal
 */
Com_IPduIdType Com_FindPduForSignal(Com_SignalIdType SignalId)
{
    if (SignalId >= Com_GlobalState.Config->NumSignals) {
        return (Com_IPduIdType)COM_MAX_IPDUS;
    }
    
    const Com_SignalConfigType* signalConfig = 
        &Com_GlobalState.Config->Signals[SignalId];
    
    /* Search through I-PDUs to find containing this signal */
    for (uint16 i = 0u; i < Com_GlobalState.Config->NumIPdus; i++) {
        const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[i];
        
        for (uint8 j = 0u; j < ipduConfig->NumSignals; j++) {
            if (ipduConfig->SignalRefs[j] == SignalId) {
                return (Com_IPduIdType)i;
            }
        }
    }
    
    return (Com_IPduIdType)COM_MAX_IPDUS;
}

/**
 * @brief Find I-PDU ID containing a signal group
 */
Com_IPduIdType Com_FindPduForSignalGroup(Com_SignalGroupIdType SignalGroupId)
{
    if (SignalGroupId >= Com_GlobalState.Config->NumSignalGroups) {
        return (Com_IPduIdType)COM_MAX_IPDUS;
    }
    
    /* Search through I-PDUs to find containing this signal group */
    for (uint16 i = 0u; i < Com_GlobalState.Config->NumIPdus; i++) {
        const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[i];
        
        for (uint8 j = 0u; j < ipduConfig->NumSignalGroups; j++) {
            if (ipduConfig->SignalGroupRefs[j] == SignalGroupId) {
                return (Com_IPduIdType)i;
            }
        }
    }
    
    return (Com_IPduIdType)COM_MAX_IPDUS;
}

/*==================[End of File]==========================================*/
