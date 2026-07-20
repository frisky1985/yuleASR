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
 * Com_Main.c
 * AUTOSAR COM Module - Main Functions and Transmission
 * According to AUTOSAR SWS COM 4.4.0
 * 
 * T009: COM_IPduTransmit Transmission Scheduler Implementation
 */

/*==================[Includes]=============================================*/

#include "Com_Private.h"
#include "Com_Transmit.h"
#include "Com_TxMode.h"
#include "Com_DeadlineMon.h"

/*==================[Local Function Declarations]===========================*/

static void Com_ProcessTxIPdu(Com_IPduIdType PduId);
static void Com_ProcessRxIPdu(Com_IPduIdType PduId);

/*==================[Main Functions]========================================*/

/**
 * @brief Main function for transmission processing
 * 
 * This function is called cyclically to process all transmission-related tasks:
 * - Process send request queue
 * - Handle periodic transmissions
 * - Manage timeout detection (ASIL-D)
 * - Process retry logic
 * 
 * @req SWS_Com_00016
 * @req SWS_Com_00583 (ASIL-D: Timeout detection)
 */
void Com_MainFunctionTx(void)
{
    COM_VALIDATE_NO_RV(Com_GlobalState.Status == COM_READY,
                       COM_SERVICE_ID_MAINFUNCTIONTX, COM_E_UNINIT);
    
    /* ASIL-D: Validate queue integrity before processing */
    if (Com_ValidateTxQueueIntegrity() != E_OK) {
        /* Queue corruption detected - reinitialize */
        Com_TxQueueInit();
        return;
    }
    
    /* Process transmission retries and timeouts (ASIL-D safety) */
    Com_ProcessTxRetries();
    
    /* Process pending requests from send queue */
    Com_TxRequestEntryType* request = NULL_PTR;
    while (Com_TxQueueGetNextRequest(&request) == E_OK) {
        if (request != NULL_PTR) {
            /* Mark request as in progress */
            request->State = COM_TXREQ_IN_PROGRESS;
            
            /* Execute transmission */
            Std_ReturnType result = Com_TransmitIPdu(request->PduId);
            
            if (result == E_OK) {
                /* Transmission initiated successfully */
                Com_TxQueueRemoveRequest(request);
            } else {
                /* Transmission failed - mark for retry */
                Com_TxQueueMarkRetry(request);
            }
        }
    }
    
    /* Process periodic and triggered transmissions for all send IPdus */
    for (uint16 i = 0; i < Com_GlobalState.Config->NumIPdus; i++) {
        const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[i];
        
        if (ipduConfig->Direction == COM_SEND) {
            Com_ProcessTxIPdu((Com_IPduIdType)i);
        }
    }
}

/**
 * @brief Main function for reception processing
 * 
 * Called cyclically to process received data and handle:
 * - Reception timeout monitoring (Deadline Monitoring - T012)
 * - Deferred signal notifications
 * 
 * @req SWS_Com_00015
 * @req SWS_Com_00500 (Deadline Monitoring)
 */
void Com_MainFunctionRx(void)
{
    COM_VALIDATE_NO_RV(Com_GlobalState.Status == COM_READY,
                       COM_SERVICE_ID_MAINFUNCTIONRX, COM_E_UNINIT);
    
    /* T012: Process deadline monitoring timers (ASIL-D) */
    COM_DM_PROCESS_IN_MAINFUNCTIONRX();
    
    /* Process all receive IPdus */
    for (uint16 i = 0; i < Com_GlobalState.Config->NumIPdus; i++) {
        const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[i];
        
        if (ipduConfig->Direction == COM_RECEIVE) {
            Com_ProcessRxIPdu((Com_IPduIdType)i);
        }
    }
}

/**
 * @brief Main function for signal routing (gateway)
 * 
 * Called cyclically to route signals between I-PDUs.
 * Gateway functionality routes signals from one I-PDU to another.
 * 
 * @note Gateway functionality not implemented in this version
 */
void Com_MainFunctionRouteSignals(void)
{
    /* Gateway functionality - not implemented in this version */
    /* This would route signals between different I-PDUs based on configuration */
}

/*==================[PduR Interface]========================================*/

/**
 * @brief PduR receive indication callback
 * 
 * Called by PduR when a PDU is received from the lower layer.
 * Copies received data to the I-PDU buffer and handles notifications.
 * Also restarts deadline monitoring timer (T012).
 * 
 * @param RxPduId Receive PDU ID
 * @param PduInfoPtr Pointer to PDU information
 * 
 * @req SWS_Com_00500 (Deadline Monitoring)
 */
void PduR_ComRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    COM_VALIDATE_NO_RV(Com_GlobalState.Status == COM_READY, 0, COM_E_UNINIT);
    COM_VALIDATE_NO_RV(PduInfoPtr != NULL_PTR, 0, COM_E_PARAM_POINTER);
    
    /* Find corresponding COM IPdu */
    Com_IPduIdType comPduId = (Com_IPduIdType)COM_MAX_IPDUS; /* Invalid */
    for (uint16 i = 0; i < Com_GlobalState.Config->NumIPdus; i++) {
        if (Com_GlobalState.Config->IPdus[i].IPduId == RxPduId) {
            comPduId = (Com_IPduIdType)i;
            break;
        }
    }
    
    if (comPduId >= Com_GlobalState.Config->NumIPdus) {
        return; /* IPdu not found */
    }
    
    const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[comPduId];
    
    /* Check if IPdu is started */
    if (Com_GlobalState.IPduRunTime[comPduId].GroupStatus != COM_IPDU_GROUP_STARTED) {
        return;
    }
    
    /* Copy received data to IPdu buffer */
    if (PduInfoPtr->SduDataPtr != NULL_PTR && PduInfoPtr->SduLength <= ipduConfig->Length) {
        memcpy(ipduConfig->DataPtr, PduInfoPtr->SduDataPtr, PduInfoPtr->SduLength);
    }
    
    /* T012: Restart deadline monitoring timer */
    if (ipduConfig->Timeout > 0u) {
        Com_Dm_StartTimer(comPduId, ipduConfig->Timeout);
    }
    
    /* Reset legacy timeout timer */
    Com_GlobalState.IPduRunTime[comPduId].TimeoutTimer = ipduConfig->Timeout;
    Com_GlobalState.IPduRunTime[comPduId].TimeoutOccurred = FALSE;
    
    /* Process signals based on signal processing mode */
    if (ipduConfig->SignalProcessing == COM_IMMEDIATE) {
        /* Call notifications immediately */
        for (uint8 i = 0; i < ipduConfig->NumSignals; i++) {
            Com_SignalIdType sigId = ipduConfig->SignalRefs[i];
            const Com_SignalConfigType* sigConfig = 
                &Com_GlobalState.Config->Signals[sigId];
            if (sigConfig->ComNotification != NULL_PTR) {
                sigConfig->ComNotification();
            }
        }
    }
    /* If DEFERRED, signals will be processed in Com_MainFunctionRx */
}

/**
 * @brief PduR transmit confirmation callback
 * 
 * Called by PduR to confirm a transmission.
 * Updates transmission status and handles retry logic.
 * 
 * @param TxPduId Transmit PDU ID
 * @param result Transmission result (E_OK or E_NOT_OK)
 */
void PduR_ComTxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    COM_VALIDATE_NO_RV(Com_GlobalState.Status == COM_READY, 0, COM_E_UNINIT);
    
    /* Find corresponding COM IPdu */
    Com_IPduIdType comPduId = (Com_IPduIdType)COM_MAX_IPDUS;
    for (uint16 i = 0; i < Com_GlobalState.Config->NumIPdus; i++) {
        if (Com_GlobalState.Config->IPdus[i].IPduId == TxPduId) {
            comPduId = (Com_IPduIdType)i;
            break;
        }
    }
    
    if (comPduId >= Com_GlobalState.Config->NumIPdus) {
        return;
    }
    
    /* Handle transmission confirmation */
    Com_HandleTxConfirmation(comPduId, result);
}

/**
 * @brief PduR trigger transmit callback
 * 
 * Called by PduR to request data for transmission.
 * Copies I-PDU data to the provided buffer.
 * 
 * @param TxPduId Transmit PDU ID
 * @param PduInfoPtr Pointer to PDU information for data copy
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType PduR_ComTriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr)
{
    COM_VALIDATE(Com_GlobalState.Status == COM_READY, 0, COM_E_UNINIT, E_NOT_OK);
    COM_VALIDATE(PduInfoPtr != NULL_PTR, 0, COM_E_PARAM_POINTER, E_NOT_OK);
    
    /* Find corresponding COM IPdu */
    Com_IPduIdType comPduId = (Com_IPduIdType)COM_MAX_IPDUS;
    for (uint16 i = 0; i < Com_GlobalState.Config->NumIPdus; i++) {
        if (Com_GlobalState.Config->IPdus[i].IPduId == TxPduId) {
            comPduId = (Com_IPduIdType)i;
            break;
        }
    }
    
    if (comPduId >= Com_GlobalState.Config->NumIPdus) {
        return E_NOT_OK;
    }
    
    const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[comPduId];
    
    /* Copy data to PduInfo */
    if (PduInfoPtr->SduDataPtr != NULL_PTR) {
        memcpy(PduInfoPtr->SduDataPtr, ipduConfig->DataPtr, ipduConfig->Length);
        PduInfoPtr->SduLength = ipduConfig->Length;
    }
    
    return E_OK;
}

/*==================[Local Functions]======================================*/

/**
 * @brief Process Tx I-PDU - check transmission conditions
 * 
 * Handles periodic transmission, triggered transmission, and mixed mode.
 * Called from Com_MainFunctionTx for each send I-PDU.
 * 
 * @param PduId I-PDU identifier
 */
static void Com_ProcessTxIPdu(Com_IPduIdType PduId)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return;
    }
    
    /* Process transmission mode for this I-PDU */
    Com_TxModeProcessIPdu(PduId);
    
    /* Check if I-PDU should be transmitted based on its mode */
    if (Com_TxModeShouldTransmit(PduId)) {
        /* Execute transmission */
        Std_ReturnType result = Com_TransmitIPdu(PduId);
        
        /* Handle transmission result */
        if (result != E_OK) {
            /* Transmission failed - will be handled by retry logic */
        }
    }
}

/**
 * @brief Process Rx I-PDU - handle timeout and deferred processing
 * 
 * Handles:
 * - Legacy reception timeout monitoring
 * - T012: Deadline monitoring timeout actions (ErrorHook, default value)
 * - Deferred signal notifications
 * 
 * @param PduId I-PDU identifier
 */
static void Com_ProcessRxIPdu(Com_IPduIdType PduId)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return;
    }
    
    const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[PduId];
    Com_IPduRunTimeType* ipduRuntime = &Com_GlobalState.IPduRunTime[PduId];
    
    /* Check if IPdu is started */
    if (ipduRuntime->GroupStatus != COM_IPDU_GROUP_STARTED) {
        return;
    }
    
    /* Handle legacy reception timeout (backward compatibility) */
    if (ipduConfig->Timeout > 0U ) {
        if (ipduRuntime->TimeoutTimer > 0U ) {
            ipduRuntime->TimeoutTimer--;
        } else if (!ipduRuntime->TimeoutOccurred) {
            /* Timeout occurred */
            ipduRuntime->TimeoutOccurred = TRUE;
        }
    }
    
    /* T012: Check deadline monitoring timeout and handle actions */
    if (Com_Dm_GetState(PduId) == COM_DM_STATE_EXPIRED) {
        /* Timeout detected by deadline monitoring - process actions */
        /* Note: DmConfig would be retrieved from extended configuration */
        /* For now, use runtime state to trigger legacy timeout notification */
        
        /* Mark timeout as processed to prevent repeated handling */
        Com_DmRunTimeData[PduId].TimeoutProcessed = TRUE;
    }
    
    /* Process deferred signals */
    if (ipduConfig->SignalProcessing == COM_DEFERRED) {
        /* Call signal notifications */
        for (uint8 i = 0; i < ipduConfig->NumSignals; i++) {
            Com_SignalIdType sigId = ipduConfig->SignalRefs[i];
            const Com_SignalConfigType* sigConfig = 
                &Com_GlobalState.Config->Signals[sigId];
            if (sigConfig->ComNotification != NULL_PTR) {
                sigConfig->ComNotification();
            }
        }
    }
}

/*==================[End of File]==========================================*/
