/*
 * Com_Main.c
 * AUTOSAR COM Module - Main Functions and Transmission
 */

/*==================[Includes]=============================================*/

#include "Com_Private.h"

/*==================[Local Function Declarations]===========================*/

static void Com_ProcessTxIPdu(Com_IPduIdType PduId);
static void Com_ProcessRxIPdu(Com_IPduIdType PduId);
static void Com_TransmitIPdu(Com_IPduIdType PduId);
static boolean Com_IsTxModePeriodic(Com_IPduIdType PduId);

/*==================[Main Functions]========================================*/

/*------------------[Com_MainFunctionTx]-----------------------------------*/
void Com_MainFunctionTx(void)
{
    COM_VALIDATE_NO_RV(Com_GlobalState.Status == COM_READY,
                       COM_SERVICE_ID_MAINFUNCTIONTX, COM_E_UNINIT);
    
    /* Process all send IPdus */
    for (uint16 i = 0; i < Com_GlobalState.Config->NumIPdus; i++) {
        const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[i];
        
        if (ipduConfig->Direction == COM_SEND) {
            Com_ProcessTxIPdu((Com_IPduIdType)i);
        }
    }
}

/*------------------[Com_MainFunctionRx]-----------------------------------*/
void Com_MainFunctionRx(void)
{
    COM_VALIDATE_NO_RV(Com_GlobalState.Status == COM_READY,
                       COM_SERVICE_ID_MAINFUNCTIONRX, COM_E_UNINIT);
    
    /* Process all receive IPdus */
    for (uint16 i = 0; i < Com_GlobalState.Config->NumIPdus; i++) {
        const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[i];
        
        if (ipduConfig->Direction == COM_RECEIVE) {
            Com_ProcessRxIPdu((Com_IPduIdType)i);
        }
    }
}

/*------------------[Com_MainFunctionRouteSignals]-------------------------*/
void Com_MainFunctionRouteSignals(void)
{
    /* Gateway functionality - not implemented in this version */
}

/*==================[PduR Interface]========================================*/

/*------------------[PduR_ComRxIndication]---------------------------------*/
void PduR_ComRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    COM_VALIDATE_NO_RV(Com_GlobalState.Status == COM_READY, 0, COM_E_UNINIT);
    COM_VALIDATE_NO_RV(PduInfoPtr != NULL_PTR, 0, COM_E_PARAM_POINTER);
    
    /* Find corresponding COM IPdu */
    Com_IPduIdType comPduId = COM_MAX_IPDUS; /* Invalid */
    for (uint16 i = 0; i < Com_GlobalState.Config->NumIPdus; i++) {
        if (Com_GlobalState.Config->IPdus[i].PduId == RxPduId) {
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
    
    /* Reset timeout timer */
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

/*------------------[PduR_ComTxConfirmation]-------------------------------*/
void PduR_ComTxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    COM_VALIDATE_NO_RV(Com_GlobalState.Status == COM_READY, 0, COM_E_UNINIT);
    
    /* Find corresponding COM IPdu */
    Com_IPduIdType comPduId = COM_MAX_IPDUS;
    for (uint16 i = 0; i < Com_GlobalState.Config->NumIPdus; i++) {
        if (Com_GlobalState.Config->IPdus[i].PduId == TxPduId) {
            comPduId = (Com_IPduIdType)i;
            break;
        }
    }
    
    if (comPduId >= Com_GlobalState.Config->NumIPdus) {
        return;
    }
    
    if (result == E_OK) {
        /* Transmission successful */
        Com_GlobalState.IPduRunTime[comPduId].RepetitionCount = 0;
        Com_GlobalState.IPduRunTime[comPduId].Triggered = FALSE;
    } else {
        /* Transmission failed - handle error */
        /* Could trigger retry or error notification */
    }
}

/*------------------[PduR_ComTriggerTransmit]------------------------------*/
Std_ReturnType PduR_ComTriggerTransmit(PduIdType TxPduId, PduInfoType* PduInfoPtr)
{
    COM_VALIDATE(Com_GlobalState.Status == COM_READY, 0, COM_E_UNINIT, E_NOT_OK);
    COM_VALIDATE(PduInfoPtr != NULL_PTR, 0, COM_E_PARAM_POINTER, E_NOT_OK);
    
    /* Find corresponding COM IPdu */
    Com_IPduIdType comPduId = COM_MAX_IPDUS;
    for (uint16 i = 0; i < Com_GlobalState.Config->NumIPdus; i++) {
        if (Com_GlobalState.Config->IPdus[i].PduId == TxPduId) {
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

/* Process Tx IPdu - check transmission conditions */
static void Com_ProcessTxIPdu(Com_IPduIdType PduId)
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
    
    boolean shouldTransmit = FALSE;
    
    switch (ipduConfig->TxMode.Mode) {
        case COM_PERIODIC:
            /* Check periodic timer */
            if (ipduRuntime->TxTimer == 0) {
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
            /* Periodic transmission + triggered transmission */
            if (ipduRuntime->Triggered) {
                shouldTransmit = TRUE;
                ipduRuntime->Triggered = FALSE;
                /* Start repetition */
                ipduRuntime->RepetitionCount = ipduConfig->TxMode.NumRepetitions;
                ipduRuntime->RepetitionTimer = ipduConfig->TxMode.RepetitionPeriod;
            } else if (ipduRuntime->TxTimer == 0) {
                shouldTransmit = TRUE;
                ipduRuntime->TxTimer = ipduConfig->TxMode.Period;
            } else {
                ipduRuntime->TxTimer--;
            }
            
            /* Handle repetitions */
            if (ipduRuntime->RepetitionCount > 0 && !shouldTransmit) {
                if (ipduRuntime->RepetitionTimer == 0) {
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
            /* No transmission */
            break;
    }
    
    if (shouldTransmit) {
        Com_TransmitIPdu(PduId);
    }
}

/* Process Rx IPdu - handle timeout and deferred processing */
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
    
    /* Handle reception timeout */
    if (ipduConfig->Timeout > 0) {
        if (ipduRuntime->TimeoutTimer > 0) {
            ipduRuntime->TimeoutTimer--;
        } else if (!ipduRuntime->TimeoutOccurred) {
            /* Timeout occurred */
            ipduRuntime->TimeoutOccurred = TRUE;
            /* Call timeout notification if configured */
        }
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

/* Transmit IPdu via PduR */
static void Com_TransmitIPdu(Com_IPduIdType PduId)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return;
    }
    
    const Com_IPduConfigType* ipduConfig = &Com_GlobalState.Config->IPdus[PduId];
    
    PduInfoType pduInfo;
    pduInfo.SduDataPtr = ipduConfig->DataPtr;
    pduInfo.SduLength = ipduConfig->Length;
    pduInfo.MetaDataPtr = NULL_PTR;
    
    /* Call IPdu callout if configured */
    if (ipduConfig->ComIPduCallout != NULL_PTR) {
        ipduConfig->ComIPduCallout(ipduConfig->PduId, &pduInfo);
    }
    
    /* Transmit via PduR */
    /* Note: PduR_ComTransmit would be the actual API */
    /* Std_ReturnType result = PduR_ComTransmit(ipduConfig->PduId, &pduInfo); */
}

/*==================[End of File]==========================================*/
