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
 * Com_Signal.c
 * AUTOSAR COM Module - Signal Operations
 */

/*==================[Includes]=============================================*/

#include "Com_Private.h"
#include "Com_Transmit.h"
#include "Com_TxMode.h"

/*==================[Local Function Declarations]===========================*/

static boolean Com_ShouldTransmit(const Com_SignalConfigType* signalConfig, 
                                   uint64 oldValue, uint64 newValue);
static void Com_TriggerIPduTransmission(Com_IPduIdType PduId);

/*==================[API Implementation]====================================*/

/*------------------[Com_SendSignal]---------------------------------------*/
uint8 Com_SendSignal(Com_SignalIdType SignalId, const void* SignalDataPtr)
{
    /* Call the internal implementation with ASIL-D safety checks */
    return Com_SendSignal_Internal(SignalId, SignalDataPtr);
}

/*------------------[Com_ReceiveSignal]------------------------------------*/
uint8 Com_ReceiveSignal(Com_SignalIdType SignalId, void* SignalDataPtr)
{
    /* Validate inputs */
    COM_VALIDATE(Com_GlobalState.Status == COM_READY,
                 COM_SERVICE_ID_RECEIVESIGNAL, COM_E_UNINIT, COM_SERVICE_NOT_AVAILABLE);
    COM_VALIDATE(SignalId < Com_GlobalState.Config->NumSignals,
                 COM_SERVICE_ID_RECEIVESIGNAL, COM_E_PARAM_SIGNALID, COM_SERVICE_NOT_AVAILABLE);
    COM_VALIDATE(SignalDataPtr != NULL_PTR,
                 COM_SERVICE_ID_RECEIVESIGNAL, COM_E_PARAM_POINTER, COM_SERVICE_NOT_AVAILABLE);
    
    const Com_SignalConfigType* signalConfig = &Com_GlobalState.Config->Signals[SignalId];
    
    /* Extract value from buffer */
    uint64 value = Com_ExtractSignal(signalConfig->DataPtr,
                                      signalConfig->BitPosition,
                                      signalConfig->BitSize,
                                      signalConfig->Endianness);
    
    /* Write to output based on signal type */
    switch (signalConfig->SignalType) {
        case COM_BOOLEAN:
            *(boolean*)SignalDataPtr = (boolean)value;
            break;
        case COM_UINT8:
            *(uint8*)SignalDataPtr = (uint8)value;
            break;
        case COM_UINT16:
            *(uint16*)SignalDataPtr = (uint16)value;
            break;
        case COM_UINT32:
            *(uint32*)SignalDataPtr = (uint32)value;
            break;
        case COM_UINT64:
            *(uint64*)SignalDataPtr = value;
            break;
        case COM_SINT8:
            *(sint8*)SignalDataPtr = (sint8)value;
            break;
        case COM_SINT16:
            *(sint16*)SignalDataPtr = (sint16)value;
            break;
        case COM_SINT32:
            *(sint32*)SignalDataPtr = (sint32)value;
            break;
        case COM_SINT64:
            *(sint64*)SignalDataPtr = (sint64)value;
            break;
        case COM_FLOAT32: {
            uint32 temp = (uint32)value;
            memcpy(SignalDataPtr, &temp, sizeof(float32));
            break;
        }
        case COM_FLOAT64:
            memcpy(SignalDataPtr, &value, sizeof(float64));
            break;
        default:
            /* Handle arrays separately */
            break;
    }
    
    return E_OK;
}

/*------------------[Com_SendSignalGroup]----------------------------------*/
uint8 Com_SendSignalGroup(Com_SignalGroupIdType SignalGroupId)
{
    /* Call internal implementation */
    return Com_SendSignalGroup_Internal(SignalGroupId);
}

/*------------------[Com_ReceiveSignalGroup]-------------------------------*/
uint8 Com_ReceiveSignalGroup(Com_SignalGroupIdType SignalGroupId)
{
    COM_VALIDATE(Com_GlobalState.Status == COM_READY,
                 COM_SERVICE_ID_RECEIVESIGNALGROUP, COM_E_UNINIT, COM_SERVICE_NOT_AVAILABLE);
    COM_VALIDATE(SignalGroupId < Com_GlobalState.Config->NumSignalGroups,
                 COM_SERVICE_ID_RECEIVESIGNALGROUP, COM_E_PARAM_SIGNALID, COM_SERVICE_NOT_AVAILABLE);
    
    const Com_SignalGroupConfigType* groupConfig = 
        &Com_GlobalState.Config->SignalGroups[SignalGroupId];
    Com_SignalGroupRunTimeType* groupRuntime = 
        &Com_GlobalState.SignalGroupRunTime[SignalGroupId];
    
    /* Copy IPdu data to shadow buffer */
    for (uint8 i = 0; i < groupConfig->NumSignals; i++) {
        const Com_SignalConfigType* sigConfig = 
            &Com_GlobalState.Config->Signals[groupConfig->SignalRefs[i]];
        
        /* Extract value from IPdu */
        uint64 value = Com_ExtractSignal(sigConfig->DataPtr,
                                          sigConfig->BitPosition,
                                          sigConfig->BitSize,
                                          sigConfig->Endianness);
        
        /* Calculate offset in shadow buffer */
        uint16 shadowOffset = 0;
        for (uint8 j = 0; j < i; j++) {
            const Com_SignalConfigType* prevSig = 
                &Com_GlobalState.Config->Signals[groupConfig->SignalRefs[j]];
            shadowOffset += (prevSig->BitSize + 7U) / 8U;
        }
        
        /* Copy to shadow buffer */
        for (uint8 b = 0; b < (sigConfig->BitSize + 7U) / 8U; b++) {
            groupRuntime->ShadowBuffer[shadowOffset + b] = (uint8)((value >> (b * 8U)) & 0xFFU);
        }
    }
    
    return E_OK;
}

/*------------------[Com_UpdateShadowSignal]-------------------------------*/
uint8 Com_UpdateShadowSignal(Com_SignalIdType SignalId, const void* SignalDataPtr)
{
    /* Find signal group containing this signal */
    for (uint16 sg = 0; sg < Com_GlobalState.Config->NumSignalGroups; sg++) {
        const Com_SignalGroupConfigType* groupConfig = 
            &Com_GlobalState.Config->SignalGroups[sg];
        for (uint8 i = 0; i < groupConfig->NumSignals; i++) {
            if (groupConfig->SignalRefs[i] == SignalId) {
                /* Found the signal group */
                const Com_SignalConfigType* sigConfig = 
                    &Com_GlobalState.Config->Signals[SignalId];
                Com_SignalGroupRunTimeType* groupRuntime = 
                    &Com_GlobalState.SignalGroupRunTime[sg];
                
                /* Calculate offset in shadow buffer */
                uint16 shadowOffset = 0;
                for (uint8 j = 0; j < i; j++) {
                    const Com_SignalConfigType* prevSig = 
                        &Com_GlobalState.Config->Signals[groupConfig->SignalRefs[j]];
                    shadowOffset += (prevSig->BitSize + 7U) / 8U;
                }
                
                /* Copy data to shadow buffer */
                uint8 size = (sigConfig->BitSize + 7U) / 8U;
                memcpy(&groupRuntime->ShadowBuffer[shadowOffset], SignalDataPtr, size);
                
                return E_OK;
            }
        }
    }
    
    return COM_SERVICE_NOT_AVAILABLE;
}

/*==================[Local Functions]======================================*/

/* Check if transmission should be triggered */
static boolean Com_ShouldTransmit(const Com_SignalConfigType* signalConfig,
                                   uint64 oldValue, uint64 newValue)
{
    switch (signalConfig->TransferProperty) {
        case COM_PENDING:
            return FALSE; /* Never trigger */
            
        case COM_TRIGGERED:
            return TRUE; /* Always trigger */
            
        case COM_TRIGGERED_ON_CHANGE:
        case COM_TRIGGERED_ON_CHANGE_WITHOUT_REPETITION:
            return (oldValue != newValue);
            
        case COM_TRIGGERED_WITHOUT_REPETITION:
            return TRUE;
            
        default:
            return FALSE;
    }
}

/* Trigger IPdu transmission */
static void Com_TriggerIPduTransmission(Com_IPduIdType PduId)
{
    if (PduId >= Com_GlobalState.Config->NumIPdus) {
        return;
    }
    
    Com_IPduRunTimeType* ipduRuntime = &Com_GlobalState.IPduRunTime[PduId];
    const Com_IPduConfigType* ipduConfig ;
    
    /* Check if IPdu is started */
    if (ipduRuntime->GroupStatus != COM_IPDU_GROUP_STARTED) {
        return;
    }
    
    /* Set trigger flag */
    ipduRuntime->Triggered = TRUE;
    
    /* Notify transmission mode manager of trigger */
    Com_TxModeTriggerDirect(PduId);
}

/*==================[End of File]==========================================*/
