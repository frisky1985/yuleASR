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

/*
 * Com_Confirmation.c
 * AUTOSAR COM Module - Transmission Confirmation and Retry Management
 * According to AUTOSAR SWS COM 4.4.0
 * 
 * Specifications:
 * - SWS_Com_00450: Transmission confirmation handling
 * - SWS_Com_00455: Retry mechanism support
 */

/*==================[Includes]=============================================*/

#include "Com_Private.h"

/*==================[Global Variables]=====================================*/

/* Retry Queue */
static Com_RetryQueueType Com_RetryQueue;

/* Confirmation Runtime Data Array */
static Com_TxConfirmationRunTimeType Com_TxConfirmationData[COM_MAX_IPDUS];

/* Confirmation Module Initialized Flag */
static boolean Com_ConfirmationInitialized = FALSE;

/* Global Timestamp Counter (increments every MainFunction call) */
static uint32 Com_TimestampCounter = 0u;

/*==================[Local Function Declarations]==========================*/

static void Com_UpdateConfirmationState(Com_IPduIdType PduId, 
                                         Com_TxStatusType NewStatus,
                                         Com_TxResultType Result);
static void Com_InvokeConfirmationCallbacks(Com_IPduIdType PduId, 
                                            Com_TxResultType Result);
static void Com_ProcessRetryEntry(Com_RetryQueueEntryType* Entry);
static void Com_ResetConfirmationData(Com_IPduIdType PduId);

/*==================[External Function Declarations]=======================*/

extern Std_ReturnType PduR_IfTransmit(PduIdType TxPduId, const PduInfoType *PduInfoPtr);

/*==================[Confirmation Callback Implementation]=================*/

/*------------------[Com_TxConfirmation]-----------------------------------*/
/**
 * @brief Transmission confirmation callback from PduR
 * 
 * This function is called by PduR to confirm a transmission result.
 * It updates the transmission state machine and triggers retry if needed.
 * 
 * Implements SWS_Com_00450: The COM module shall provide the callback
 * function Com_TxConfirmation to receive transmission confirmations
 * from the PDU Router.
 */
void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    COM_VALIDATE_NO_RV(Com_GlobalState.Status == COM_READY,
                       COM_SERVICE_ID_TX_CONFIRMATION, COM_E_UNINIT);
    
    /* Find corresponding COM IPdu */
    Com_IPduIdType comPduId = COM_MAX_IPDUS;
    for (uint16 i = 0; i < Com_GlobalState.Config->NumIPdus; i++) {
        if (Com_GlobalState.Config->IPdus[i].IPduId == TxPduId) {
            comPduId = (Com_IPduIdType)i;
            break;
        }
    }
    
    /* Validate PDU ID */
    if (comPduId >= Com_GlobalState.Config->NumIPdus) {
#if (COM_DEV_ERROR_DETECT == STD_ON)
        COM_REPORT_ERROR(COM_SERVICE_ID_TX_CONFIRMATION, COM_E_PARAM);
#endif
        return;
    }
    
    const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[comPduId];
    Com_IPduRunTimeType* ipduRuntime = &Com_GlobalState.IPduRunTime[comPduId];
    
    /* Check if confirmation is expected for this PDU */
    if (!ipduConfig->TxConfirmation.EnableConfirmation) {
        /* Confirmation not enabled, ignore */
        return;
    }
    
    /* Check if we were waiting for confirmation */
    if (ipduRuntime->TxStatus != COM_TX_PENDING && 
        ipduRuntime->TxStatus != COM_TX_RETRY_PENDING) {
        /* Unexpected confirmation, ignore */
        return;
    }
    
    if (result == E_OK) {
        /* Transmission successful */
        Com_UpdateConfirmationState(comPduId, COM_TX_CONFIRMED, COM_TX_RES_OK);
        
        /* Reset retry count on success */
        ipduRuntime->CurrentRetryCount = 0;
        
        /* Remove from retry queue if present */
        Com_RemoveFromRetryQueue(comPduId);
        
        /* Invoke success callbacks */
        Com_InvokeConfirmationCallbacks(comPduId, COM_TX_RES_OK);
        
    } else {
        /* Transmission failed */
        Com_UpdateConfirmationState(comPduId, COM_TX_ERROR, COM_TX_RES_NOT_OK);
        
        /* Check if retry is needed */
        if (ipduRuntime->CurrentRetryCount < ipduConfig->TxConfirmation.MaxRetries) {
            /* Add to retry queue */
            uint8 remainingRetries = ipduConfig->TxConfirmation.MaxRetries - 
                                     ipduRuntime->CurrentRetryCount;
            if (Com_AddToRetryQueue(comPduId, remainingRetries) == E_OK) {
                Com_UpdateConfirmationState(comPduId, COM_TX_RETRY_PENDING, COM_TX_RES_NONE);
            } else {
                /* Retry queue full, invoke error callback */
                Com_InvokeConfirmationCallbacks(comPduId, COM_TX_RES_NOT_OK);
            }
        } else {
            /* Max retries exceeded */
            Com_InvokeConfirmationCallbacks(comPduId, COM_TX_RES_NOT_OK);
            
#if (COM_DEV_ERROR_DETECT == STD_ON)
            COM_REPORT_ERROR(COM_SERVICE_ID_TX_CONFIRMATION, COM_E_MAX_RETRIES_EXCEEDED);
#endif
        }
    }
}

/*==================[Internal Confirmation Management]=====================*/

/*------------------[Com_InitConfirmation]---------------------------------*/
void Com_InitConfirmation(void)
{
    /* Initialize all confirmation data */
    for (uint16 i = 0; i < COM_MAX_IPDUS; i++) {
        Com_ResetConfirmationData((Com_IPduIdType)i);
    }
    
    /* Initialize retry queue */
    Com_InitRetryQueue();
    
    Com_ConfirmationInitialized = TRUE;
}

/*------------------[Com_DeInitConfirmation]-------------------------------*/
void Com_DeInitConfirmation(void)
{
    /* Clear all confirmation data */
    for (uint16 i = 0; i < COM_MAX_IPDUS; i++) {
        Com_ResetConfirmationData((Com_IPduIdType)i);
    }
    
    /* Clear retry queue */
    Com_RetryQueue.Head = 0;
    Com_RetryQueue.Tail = 0;
    Com_RetryQueue.Count = 0;
    
    Com_ConfirmationInitialized = FALSE;
}

/*------------------[Com_StartTxConfirmation]------------------------------*/
Std_ReturnType Com_StartTxConfirmation(Com_IPduIdType PduId)
{
    COM_VALIDATE(Com_GlobalState.Status == COM_READY,
                 COM_SERVICE_ID_START_TX_CONF, COM_E_UNINIT, E_NOT_OK);
    COM_VALIDATE(PduId < Com_GlobalState.Config->NumIPdus,
                 COM_SERVICE_ID_START_TX_CONF, COM_E_PARAM, E_NOT_OK);
    
    const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[PduId];
    Com_IPduRunTimeType* ipduRuntime = &Com_GlobalState.IPduRunTime[PduId];
    
    /* Check if confirmation is enabled */
    if (!ipduConfig->TxConfirmation.EnableConfirmation) {
        return E_OK; /* Confirmation not needed */
    }
    
    /* Check if already pending */
    if (ipduRuntime->TxStatus == COM_TX_PENDING ||
        ipduRuntime->TxStatus == COM_TX_RETRY_PENDING) {
        /* Already waiting for confirmation, this is a duplicate request */
        return E_NOT_OK;
    }
    
    /* Set pending status */
    Com_UpdateConfirmationState(PduId, COM_TX_PENDING, COM_TX_RES_NONE);
    
    /* Initialize timeout timer */
    if (ipduConfig->TxConfirmation.TxTimeout > 0) {
        ipduRuntime->TxConfTimeoutTimer = ipduConfig->TxConfirmation.TxTimeout;
    } else {
        ipduRuntime->TxConfTimeoutTimer = COM_DEFAULT_TX_TIMEOUT;
    }
    
    /* Set timestamp */
    ipduRuntime->LastTxTimestamp = Com_TimestampCounter;
    ipduRuntime->ConfPending = TRUE;
    
    return E_OK;
}

/*------------------[Com_CancelTxConfirmation]-----------------------------*/
void Com_CancelTxConfirmation(Com_IPduIdType PduId)
{
    COM_VALIDATE_NO_RV(Com_GlobalState.Status == COM_READY,
                       COM_SERVICE_ID_CANCEL_TX_CONF, COM_E_UNINIT);
    COM_VALIDATE_NO_RV(PduId < Com_GlobalState.Config->NumIPdus,
                       COM_SERVICE_ID_CANCEL_TX_CONF, COM_E_PARAM);
    
    Com_IPduRunTimeType* ipduRuntime = &Com_GlobalState.IPduRunTime[PduId];
    
    /* Cancel confirmation monitoring */
    Com_UpdateConfirmationState(PduId, COM_TX_IDLE, COM_TX_RES_CANCELLED);
    ipduRuntime->ConfPending = FALSE;
    ipduRuntime->TxConfTimeoutTimer = 0;
    
    /* Remove from retry queue if present */
    Com_RemoveFromRetryQueue(PduId);
}

/*------------------[Com_HandleTxTimeout]----------------------------------*/
void Com_HandleTxTimeout(Com_IPduIdType PduId)
{
    COM_VALIDATE_NO_RV(Com_GlobalState.Status == COM_READY,
                       COM_SERVICE_ID_HANDLE_TIMEOUT, COM_E_UNINIT);
    COM_VALIDATE_NO_RV(PduId < Com_GlobalState.Config->NumIPdus,
                       COM_SERVICE_ID_HANDLE_TIMEOUT, COM_E_PARAM);
    
    const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[PduId];
    Com_IPduRunTimeType* ipduRuntime = &Com_GlobalState.IPduRunTime[PduId];
    
    /* Update state */
    Com_UpdateConfirmationState(PduId, COM_TX_ERROR, COM_TX_RES_TIMEOUT);
    ipduRuntime->ConfPending = FALSE;
    
    /* Invoke timeout callback if configured */
    if (ipduConfig->TxConfirmation.ComTxTimeoutNotification != NULL_PTR) {
        ipduConfig->TxConfirmation.ComTxTimeoutNotification();
    }
    
    /* Check if retry is needed */
    if (ipduRuntime->CurrentRetryCount < ipduConfig->TxConfirmation.MaxRetries) {
        uint8 remainingRetries = ipduConfig->TxConfirmation.MaxRetries - 
                                 ipduRuntime->CurrentRetryCount;
        if (Com_AddToRetryQueue(PduId, remainingRetries) == E_OK) {
            Com_UpdateConfirmationState(PduId, COM_TX_RETRY_PENDING, COM_TX_RES_NONE);
        } else {
            /* Retry queue full, invoke error callback */
            if (ipduConfig->TxConfirmation.ComTxErrorNotification != NULL_PTR) {
                ipduConfig->TxConfirmation.ComTxErrorNotification();
            }
        }
    } else {
        /* Max retries exceeded, invoke error callback */
        if (ipduConfig->TxConfirmation.ComTxErrorNotification != NULL_PTR) {
            ipduConfig->TxConfirmation.ComTxErrorNotification();
        }
        
#if (COM_DEV_ERROR_DETECT == STD_ON)
        COM_REPORT_ERROR(COM_SERVICE_ID_HANDLE_TIMEOUT, COM_E_CONFIRMATION_TIMEOUT);
#endif
    }
}

/*------------------[Com_GetTxStatus]--------------------------------------*/
Com_TxStatusType Com_GetTxStatus(Com_IPduIdType PduId)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return COM_TX_ERROR;
    }
    
    return Com_GlobalState.IPduRunTime[PduId].TxStatus;
}

/*------------------[Com_GetTxResult]--------------------------------------*/
Com_TxResultType Com_GetTxResult(Com_IPduIdType PduId)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return COM_TX_RES_NOT_OK;
    }
    
    return Com_GlobalState.IPduRunTime[PduId].TxResult;
}

/*==================[Retry Mechanism Implementation]=======================*/

/*------------------[Com_InitRetryQueue]-----------------------------------*/
void Com_InitRetryQueue(void)
{
    Com_RetryQueue.Head = 0;
    Com_RetryQueue.Tail = 0;
    Com_RetryQueue.Count = 0;
    
    for (uint8 i = 0; i < COM_MAX_RETRY_QUEUE_SIZE; i++) {
        Com_RetryQueue.Entries[i].Active = FALSE;
        Com_RetryQueue.Entries[i].PduId = COM_MAX_IPDUS;
        Com_RetryQueue.Entries[i].RetryCount = 0;
        Com_RetryQueue.Entries[i].NextRetryTime = 0;
    }
}

/*------------------[Com_AddToRetryQueue]----------------------------------*/
Std_ReturnType Com_AddToRetryQueue(Com_IPduIdType PduId, uint8 RetryCount)
{
    COM_VALIDATE(PduId < Com_GlobalState.Config->NumIPdus,
                 COM_SERVICE_ID_ADD_RETRY, COM_E_PARAM, E_NOT_OK);
    
    /* Check if queue is full */
    if (Com_RetryQueue.Count >= COM_MAX_RETRY_QUEUE_SIZE) {
#if (COM_DEV_ERROR_DETECT == STD_ON)
        COM_REPORT_ERROR(COM_SERVICE_ID_ADD_RETRY, COM_E_RETRY_QUEUE_FULL);
#endif
        return E_NOT_OK;
    }
    
    /* Check if already in queue */
    for (uint8 i = 0; i < COM_MAX_RETRY_QUEUE_SIZE; i++) {
        if (Com_RetryQueue.Entries[i].Active && 
            Com_RetryQueue.Entries[i].PduId == PduId) {
            /* Already in queue, update retry count */
            Com_RetryQueue.Entries[i].RetryCount = RetryCount;
            return E_OK;
        }
    }
    
    /* Add to queue */
    uint8 idx = Com_RetryQueue.Tail;
    Com_RetryQueue.Entries[idx].PduId = PduId;
    Com_RetryQueue.Entries[idx].RetryCount = RetryCount;
    Com_RetryQueue.Entries[idx].NextRetryTime = Com_TimestampCounter + COM_RETRY_DELAY_MS;
    Com_RetryQueue.Entries[idx].Active = TRUE;
    
    Com_RetryQueue.Tail = (Com_RetryQueue.Tail + 1) % COM_MAX_RETRY_QUEUE_SIZE;
    Com_RetryQueue.Count++;
    
    return E_OK;
}

/*------------------[Com_RemoveFromRetryQueue]-----------------------------*/
void Com_RemoveFromRetryQueue(Com_IPduIdType PduId)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return;
    }
    
    for (uint8 i = 0; i < COM_MAX_RETRY_QUEUE_SIZE; i++) {
        if (Com_RetryQueue.Entries[i].Active && 
            Com_RetryQueue.Entries[i].PduId == PduId) {
            Com_RetryQueue.Entries[i].Active = FALSE;
            Com_RetryQueue.Count--;
            break;
        }
    }
}

/*------------------[Com_ProcessRetryQueue]--------------------------------*/
void Com_ProcessRetryQueue(void)
{
    if (Com_RetryQueue.Count == 0) {
        return;
    }
    
    uint8 processed = 0;
    uint8 idx = Com_RetryQueue.Head;
    
    while (processed < Com_RetryQueue.Count && processed < COM_MAX_RETRY_QUEUE_SIZE) {
        Com_RetryQueueEntryType* entry = &Com_RetryQueue.Entries[idx];
        
        if (entry->Active) {
            /* Check if it's time to retry */
            if (Com_TimestampCounter >= entry->NextRetryTime) {
                Com_ProcessRetryEntry(entry);
            }
            processed++;
        }
        
        idx = (idx + 1) % COM_MAX_RETRY_QUEUE_SIZE;
    }
}

/*------------------[Com_IsInRetryQueue]-----------------------------------*/
boolean Com_IsInRetryQueue(Com_IPduIdType PduId)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return FALSE;
    }
    
    for (uint8 i = 0; i < COM_MAX_RETRY_QUEUE_SIZE; i++) {
        if (Com_RetryQueue.Entries[i].Active && 
            Com_RetryQueue.Entries[i].PduId == PduId) {
            return TRUE;
        }
    }
    
    return FALSE;
}

/*------------------[Com_GetRemainingRetries]------------------------------*/
uint8 Com_GetRemainingRetries(Com_IPduIdType PduId)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return 0;
    }
    
    for (uint8 i = 0; i < COM_MAX_RETRY_QUEUE_SIZE; i++) {
        if (Com_RetryQueue.Entries[i].Active && 
            Com_RetryQueue.Entries[i].PduId == PduId) {
            return Com_RetryQueue.Entries[i].RetryCount;
        }
    }
    
    return 0;
}

/*------------------[Com_PerformRetry]-------------------------------------*/
Std_ReturnType Com_PerformRetry(Com_IPduIdType PduId)
{
    COM_VALIDATE(PduId < Com_GlobalState.Config->NumIPdus,
                 COM_SERVICE_ID_PROCESS_RETRY, COM_E_PARAM, E_NOT_OK);
    
    const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[PduId];
    Com_IPduRunTimeType* ipduRuntime = &Com_GlobalState.IPduRunTime[PduId];
    
    /* Check if IPdu is started */
    if (ipduRuntime->GroupStatus != COM_IPDU_GROUP_STARTED) {
        return E_NOT_OK;
    }
    
    /* Prepare PDU info */
    PduInfoType pduInfo;
    pduInfo.SduDataPtr = ipduConfig->DataPtr;
    pduInfo.SduLength = ipduConfig->Length;
    pduInfo.MetaDataPtr = NULL_PTR;
    
    /* Increment retry count */
    ipduRuntime->CurrentRetryCount++;
    
    /* Update state to pending */
    Com_UpdateConfirmationState(PduId, COM_TX_PENDING, COM_TX_RES_NONE);
    
    /* Reset timeout timer */
    if (ipduConfig->TxConfirmation.TxTimeout > 0) {
        ipduRuntime->TxConfTimeoutTimer = ipduConfig->TxConfirmation.TxTimeout;
    } else {
        ipduRuntime->TxConfTimeoutTimer = COM_DEFAULT_TX_TIMEOUT;
    }
    
    /* Set timestamp */
    ipduRuntime->LastTxTimestamp = Com_TimestampCounter;
    ipduRuntime->ConfPending = TRUE;
    
    /* Transmit via PduR */
    Std_ReturnType result = PduR_IfTransmit(ipduConfig->IPduId, &pduInfo);
    
    if (result != E_OK) {
        /* Transmission failed immediately */
        Com_UpdateConfirmationState(PduId, COM_TX_ERROR, COM_TX_RES_NOT_OK);
    }
    
    return result;
}

/*==================[Timeout Handling Implementation]======================*/

/*------------------[Com_ProcessTxTimeouts]--------------------------------*/
void Com_ProcessTxTimeouts(void)
{
    for (uint16 i = 0; i < Com_GlobalState.Config->NumIPdus; i++) {
        const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[i];
        Com_IPduRunTimeType* ipduRuntime = &Com_GlobalState.IPduRunTime[i];
        
        /* Check if confirmation is pending */
        if (!ipduConfig->TxConfirmation.EnableConfirmation) {
            continue;
        }
        
        if (ipduRuntime->TxStatus != COM_TX_PENDING &&
            ipduRuntime->TxStatus != COM_TX_RETRY_PENDING) {
            continue;
        }
        
        /* Check for timeout */
        if (ipduRuntime->TxConfTimeoutTimer > 0) {
            ipduRuntime->TxConfTimeoutTimer--;
        } else {
            /* Timeout occurred */
            Com_HandleTxTimeout((Com_IPduIdType)i);
        }
    }
    
    /* Increment timestamp counter */
    Com_TimestampCounter++;
}

/*------------------[Com_ResetTxTimeout]-----------------------------------*/
void Com_ResetTxTimeout(Com_IPduIdType PduId)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return;
    }
    
    const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[PduId];
    Com_IPduRunTimeType* ipduRuntime = &Com_GlobalState.IPduRunTime[PduId];
    
    if (ipduConfig->TxConfirmation.TxTimeout > 0) {
        ipduRuntime->TxConfTimeoutTimer = ipduConfig->TxConfirmation.TxTimeout;
    } else {
        ipduRuntime->TxConfTimeoutTimer = COM_DEFAULT_TX_TIMEOUT;
    }
}

/*------------------[Com_IsTxTimedOut]-------------------------------------*/
boolean Com_IsTxTimedOut(Com_IPduIdType PduId)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return FALSE;
    }
    
    return (Com_GlobalState.IPduRunTime[PduId].TxResult == COM_TX_RES_TIMEOUT);
}

/*==================[Transmission Mode Switch Handling]====================*/

/*------------------[Com_HandleModeSwitchConfirmation]---------------------*/
void Com_HandleModeSwitchConfirmation(Com_IPduIdType PduId,
                                       Com_TransferModeType OldMode,
                                       Com_TransferModeType NewMode)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return;
    }
    
    Com_IPduRunTimeType* ipduRuntime = &Com_GlobalState.IPduRunTime[PduId];
    
    /* Check if confirmation is pending during mode switch */
    if (ipduRuntime->TxStatus == COM_TX_PENDING ||
        ipduRuntime->TxStatus == COM_TX_RETRY_PENDING) {
        
        /* 
         * According to AUTOSAR spec, when transmission mode is switched
         * while confirmation is pending:
         * - If switching from DIRECT to PERIODIC: Keep waiting for confirmation
         * - If switching from PERIODIC to DIRECT: Keep waiting for confirmation
         * - If switching to NONE: Cancel confirmation
         */
        if (NewMode == COM_NONE) {
            Com_CancelTxConfirmation(PduId);
        }
        /* For other mode switches, keep the confirmation pending */
    }
}

/*------------------[Com_CanSwitchModeDuringPending]-----------------------*/
boolean Com_CanSwitchModeDuringPending(Com_IPduIdType PduId)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return FALSE;
    }
    
    Com_IPduRunTimeType* ipduRuntime = &Com_GlobalState.IPduRunTime[PduId];
    
    /* Mode switch is generally allowed, but we track the state */
    /* The actual handling is done in Com_HandleModeSwitchConfirmation */
    return TRUE;
}

/*==================[Local Functions]======================================*/

/*------------------[Com_UpdateConfirmationState]--------------------------*/
static void Com_UpdateConfirmationState(Com_IPduIdType PduId,
                                         Com_TxStatusType NewStatus,
                                         Com_TxResultType Result)
{
    Com_IPduRunTimeType* ipduRuntime = &Com_GlobalState.IPduRunTime[PduId];
    
    ipduRuntime->TxStatus = NewStatus;
    ipduRuntime->TxResult = Result;
    
    /* Update confirmation data array */
    Com_TxConfirmationData[PduId].Status = NewStatus;
    Com_TxConfirmationData[PduId].LastResult = Result;
}

/*------------------[Com_InvokeConfirmationCallbacks]----------------------*/
static void Com_InvokeConfirmationCallbacks(Com_IPduIdType PduId,
                                            Com_TxResultType Result)
{
    const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[PduId];
    
    if (Result == COM_TX_RES_OK) {
        /* Success callback */
        if (ipduConfig->TxConfirmation.ComTxConfirmation != NULL_PTR) {
            ipduConfig->TxConfirmation.ComTxConfirmation();
        }
    } else {
        /* Error callback */
        if (ipduConfig->TxConfirmation.ComTxErrorNotification != NULL_PTR) {
            ipduConfig->TxConfirmation.ComTxErrorNotification();
        }
    }
}

/*------------------[Com_ProcessRetryEntry]--------------------------------*/
static void Com_ProcessRetryEntry(Com_RetryQueueEntryType* Entry)
{
    if (!Entry->Active || Entry->PduId >= Com_GlobalState.Config->NumIPdus) {
        return;
    }
    
    Com_IPduIdType pduId = Entry->PduId;
    
    /* Check if we still have retries left */
    if (Entry->RetryCount > 0) {
        /* Perform retry */
        Std_ReturnType result = Com_PerformRetry(pduId);
        
        if (result == E_OK) {
            /* Retry initiated successfully, decrement retry count */
            Entry->RetryCount--;
            
            if (Entry->RetryCount == 0) {
                /* No more retries, remove from queue */
                Entry->Active = FALSE;
                Com_RetryQueue.Count--;
            } else {
                /* Schedule next retry */
                Entry->NextRetryTime = Com_TimestampCounter + COM_RETRY_DELAY_MS;
            }
        } else {
            /* Retry failed immediately, schedule another attempt */
            Entry->NextRetryTime = Com_TimestampCounter + COM_RETRY_DELAY_MS;
        }
    } else {
        /* No retries left, remove from queue */
        Entry->Active = FALSE;
        Com_RetryQueue.Count--;
    }
}

/*------------------[Com_ResetConfirmationData]----------------------------*/
static void Com_ResetConfirmationData(Com_IPduIdType PduId)
{
    if (PduId >= COM_MAX_IPDUS) {
        return;
    }
    
    Com_TxConfirmationData[PduId].Status = COM_TX_IDLE;
    Com_TxConfirmationData[PduId].LastResult = COM_TX_RES_NONE;
    Com_TxConfirmationData[PduId].TimeoutTimer = 0;
    Com_TxConfirmationData[PduId].RetryCount = 0;
    Com_TxConfirmationData[PduId].ConfirmationPending = FALSE;
    Com_TxConfirmationData[PduId].TxTimestamp = 0;
}

/*==================[End of File]==========================================*/
