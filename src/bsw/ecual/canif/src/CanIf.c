/**
 * @file CanIf.c
 * @brief CAN Interface implementation
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 */

#include "CanIf.h"
#include "CanIf_Cfg.h"
#include "Can.h"
#include "PduR.h"
#include "Det.h"

#define CANIF_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

static boolean CanIf_DriverInitialized = FALSE;
static CanIf_ControllerModeType CanIf_ControllerMode[CANIF_NUM_CONTROLLERS];
static CanIf_PduModeType CanIf_PduMode[CANIF_NUM_CONTROLLERS];
static const CanIf_ConfigType* CanIf_ConfigPtr = NULL_PTR;

#define CANIF_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

#define CANIF_START_SEC_CODE
#include "MemMap.h"

void CanIf_Init(const CanIf_ConfigType* ConfigPtr)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (ConfigPtr == NULL_PTR) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_INIT, CANIF_E_PARAM_POINTER);
        return;
    }
    if (CanIf_DriverInitialized == TRUE) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_INIT, CANIF_E_ALREADY_INITIALIZED);
        return;
    }
    #endif
    
    CanIf_ConfigPtr = ConfigPtr;
    
    for (uint8 i = 0U; i < CANIF_NUM_CONTROLLERS; i++) {
        CanIf_ControllerMode[i] = CANIF_CS_STOPPED;
        CanIf_PduMode[i] = CANIF_OFFLINE;
    }
    
    CanIf_DriverInitialized = TRUE;
}

void CanIf_DeInit(void)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_DEINIT, CANIF_E_UNINIT);
        return;
    }
    #endif
    
    for (uint8 i = 0U; i < CANIF_NUM_CONTROLLERS; i++) {
        CanIf_ControllerMode[i] = CANIF_CS_UNINIT;
        CanIf_PduMode[i] = CANIF_OFFLINE;
    }
    
    CanIf_DriverInitialized = FALSE;
}

Std_ReturnType CanIf_SetControllerMode(uint8 ControllerId, CanIf_ControllerModeType ControllerMode)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETCONTROLLERMODE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (ControllerId >= CANIF_NUM_CONTROLLERS) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETCONTROLLERMODE, CANIF_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }
    #endif
    
    Std_ReturnType status = E_OK;
    Can_ReturnType canStatus;
    
    switch (ControllerMode) {
        case CANIF_CS_STARTED:
            canStatus = Can_SetControllerMode(ControllerId, CAN_CS_STARTED);
            if (canStatus == CAN_OK) {
                CanIf_ControllerMode[ControllerId] = CANIF_CS_STARTED;
            } else {
                status = E_NOT_OK;
            }
            break;
            
        case CANIF_CS_STOPPED:
            canStatus = Can_SetControllerMode(ControllerId, CAN_CS_STOPPED);
            if (canStatus == CAN_OK) {
                CanIf_ControllerMode[ControllerId] = CANIF_CS_STOPPED;
            } else {
                status = E_NOT_OK;
            }
            break;
            
        case CANIF_CS_SLEEP:
            canStatus = Can_SetControllerMode(ControllerId, CAN_CS_SLEEP);
            if (canStatus == CAN_OK) {
                CanIf_ControllerMode[ControllerId] = CANIF_CS_SLEEP;
            } else {
                status = E_NOT_OK;
            }
            break;
            
        default:
            status = E_NOT_OK;
            break;
    }
    
    return status;
}

Std_ReturnType CanIf_GetControllerMode(uint8 ControllerId, CanIf_ControllerModeType* ControllerModePtr)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETCONTROLLERMODE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (ControllerId >= CANIF_NUM_CONTROLLERS) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETCONTROLLERMODE, CANIF_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }
    if (ControllerModePtr == NULL_PTR) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETCONTROLLERMODE, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    *ControllerModePtr = CanIf_ControllerMode[ControllerId];
    return E_OK;
}

Std_ReturnType CanIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_TRANSMIT, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (TxPduId >= CANIF_NUM_TX_PDUS) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_TRANSMIT, CANIF_E_INVALID_TXPDUID);
        return E_NOT_OK;
    }
    if (PduInfoPtr == NULL_PTR) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_TRANSMIT, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    const CanIf_TxPduConfigType* txPduConfig = &CanIf_ConfigPtr->TxPdus[TxPduId];
    uint8 controllerId = txPduConfig->ControllerId;
    
    if (CanIf_ControllerMode[controllerId] != CANIF_CS_STARTED) {
        return E_NOT_OK;
    }
    
    if (CanIf_PduMode[controllerId] == CANIF_OFFLINE) {
        return E_NOT_OK;
    }
    
    Can_PduType canPdu;
    canPdu.idType = txPduConfig->CanIdType;
    canPdu.CanId = txPduConfig->CanId;
    canPdu.CanDlc = (uint8)PduInfoPtr->SduLength;
    canPdu.SduPtr = PduInfoPtr->SduDataPtr;
    
    Can_ReturnType canStatus = Can_Write(txPduConfig->Hth, &canPdu);
    
    if (canStatus == CAN_OK) {
        return E_OK;
    } else if (canStatus == CAN_BUSY) {
        return E_NOT_OK;
    }
    
    return E_NOT_OK;
}

Std_ReturnType CanIf_SetPduMode(uint8 ControllerId, CanIf_PduModeType PduModeRequest)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETPDUMODE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (ControllerId >= CANIF_NUM_CONTROLLERS) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETPDUMODE, CANIF_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }
    #endif
    
    CanIf_PduMode[ControllerId] = PduModeRequest;
    return E_OK;
}

Std_ReturnType CanIf_GetPduMode(uint8 ControllerId, CanIf_PduModeType* PduModePtr)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETPDUMODE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (ControllerId >= CANIF_NUM_CONTROLLERS) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETPDUMODE, CANIF_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }
    if (PduModePtr == NULL_PTR) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETPDUMODE, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    *PduModePtr = CanIf_PduMode[ControllerId];
    return E_OK;
}

void CanIf_GetVersionInfo(Std_VersionInfoType* versioninfo)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (versioninfo == NULL_PTR) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETVERSIONINFO, CANIF_E_PARAM_POINTER);
        return;
    }
    #endif
    versioninfo->vendorID = CANIF_VENDOR_ID;
    versioninfo->moduleID = CANIF_MODULE_ID;
    versioninfo->sw_major_version = CANIF_SW_MAJOR_VERSION;
    versioninfo->sw_minor_version = CANIF_SW_MINOR_VERSION;
    versioninfo->sw_patch_version = CANIF_SW_PATCH_VERSION;
}

void CanIf_TxConfirmation(PduIdType CanTxPduId)
{
    if (CanIf_DriverInitialized == FALSE) {
        return;
    }
    
    if (CanTxPduId < CANIF_NUM_TX_PDUS) {
        const CanIf_TxPduConfigType* txPduConfig = &CanIf_ConfigPtr->TxPdus[CanTxPduId];
        if (txPduConfig->TxConfirmation) {
            PduR_TxConfirmation(CanTxPduId);
        }
    }
}

void CanIf_RxIndication(const Can_HwType* Mailbox, const PduInfoType* PduInfoPtr)
{
    if (CanIf_DriverInitialized == FALSE) {
        return;
    }
    
    /* Find matching Rx PDU */
    for (PduIdType i = 0U; i < CANIF_NUM_RX_PDUS; i++) {
        const CanIf_RxPduConfigType* rxPduConfig = &CanIf_ConfigPtr->RxPdus[i];
        
        if (rxPduConfig->Hrh == Mailbox->Hoh &&
            rxPduConfig->CanId == Mailbox->CanId) {
            
            if (rxPduConfig->RxIndication) {
                PduInfoType pduInfo;
                pduInfo.SduDataPtr = PduInfoPtr->SduDataPtr;
                pduInfo.SduLength = PduInfoPtr->SduLength;
                pduInfo.MetaDataPtr = NULL_PTR;
                
                PduR_RxIndication(rxPduConfig->PduId, &pduInfo);
            }
            break;
        }
    }
}

void CanIf_ControllerBusOff(uint8 ControllerId)
{
    if (CanIf_DriverInitialized == FALSE) {
        return;
    }
    
    if (ControllerId < CANIF_NUM_CONTROLLERS) {
        CanIf_ControllerMode[ControllerId] = CANIF_CS_STOPPED;
        
        /* Notify upper layer */
        /* CanSM_ControllerBusOff(ControllerId); */
    }
}

void CanIf_ControllerModeIndication(uint8 ControllerId, CanIf_ControllerModeType ControllerMode)
{
    if (CanIf_DriverInitialized == FALSE) {
        return;
    }
    
    if (ControllerId < CANIF_NUM_CONTROLLERS) {
        CanIf_ControllerMode[ControllerId] = ControllerMode;
        
        /* Notify upper layer */
        /* CanSM_ControllerModeIndication(ControllerId, ControllerMode); */
    }
}

Std_ReturnType CanIf_SetDynamicTxId(PduIdType CanTxPduId, Can_IdType CanId)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETDYNAMICTXID, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (CanTxPduId >= CANIF_NUM_TX_PDUS) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETDYNAMICTXID, CANIF_E_INVALID_TXPDUID);
        return E_NOT_OK;
    }
    #endif
    
    /* Update dynamic CAN ID */
    /* Note: In real implementation, this would modify the configuration */
    (void)CanTxPduId;
    (void)CanId;
    
    return E_OK;
}

Std_ReturnType CanIf_CheckWakeup(EcuM_WakeupSourceType WakeupSource)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_CHECKWAKEUP, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    #endif
    
    /* Check all controllers for wakeup */
    for (uint8 i = 0U; i < CANIF_NUM_CONTROLLERS; i++) {
        if (Can_CheckWakeup(i) == E_OK) {
            return E_OK;
        }
    }
    
    (void)WakeupSource;
    return E_NOT_OK;
}

Std_ReturnType CanIf_SetTrcvMode(uint8 TransceiverId, CanIf_TransceiverModeType TransceiverMode)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETTRCVMODE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (TransceiverId >= CANIF_NUM_TRANSCEIVERS) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETTRCVMODE, CANIF_E_PARAM_TRCV);
        return E_NOT_OK;
    }
    #endif
    
    (void)TransceiverId;
    (void)TransceiverMode;
    return E_OK;
}

Std_ReturnType CanIf_GetTrcvMode(uint8 TransceiverId, CanIf_TransceiverModeType* TransceiverModePtr)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETTRCVMODE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (TransceiverId >= CANIF_NUM_TRANSCEIVERS) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETTRCVMODE, CANIF_E_PARAM_TRCV);
        return E_NOT_OK;
    }
    if (TransceiverModePtr == NULL_PTR) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETTRCVMODE, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    *TransceiverModePtr = CANIF_TRCV_MODE_NORMAL;
    return E_OK;
}

Std_ReturnType CanIf_GetTrcvWakeupReason(uint8 TransceiverId, CanIf_TrcvWakeupReasonType* TrcvWuReasonPtr)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETTRCVWAKEUPREASON, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (TransceiverId >= CANIF_NUM_TRANSCEIVERS) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETTRCVWAKEUPREASON, CANIF_E_PARAM_TRCV);
        return E_NOT_OK;
    }
    if (TrcvWuReasonPtr == NULL_PTR) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETTRCVWAKEUPREASON, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    *TrcvWuReasonPtr = CANIF_TRCV_WU_NOT_SUPPORTED;
    return E_OK;
}

Std_ReturnType CanIf_SetTrcvWakeupMode(uint8 TransceiverId, CanIf_TrcvWakeupModeType TrcvWakeupMode)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETTRCVWAKEUPMODE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (TransceiverId >= CANIF_NUM_TRANSCEIVERS) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETTRCVWAKEUPMODE, CANIF_E_PARAM_TRCV);
        return E_NOT_OK;
    }
    #endif
    
    (void)TransceiverId;
    (void)TrcvWakeupMode;
    return E_OK;
}

Std_ReturnType CanIf_SetBaudrate(uint8 ControllerId, uint16 BaudRate)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETBAUDRATE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (ControllerId >= CANIF_NUM_CONTROLLERS) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_SETBAUDRATE, CANIF_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }
    #endif
    
    (void)ControllerId;
    (void)BaudRate;
    return E_OK;
}

Std_ReturnType CanIf_GetBaudrate(uint8 ControllerId, uint16* BaudRatePtr)
{
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)
    if (CanIf_DriverInitialized == FALSE) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETBAUDRATE, CANIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (ControllerId >= CANIF_NUM_CONTROLLERS) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETBAUDRATE, CANIF_E_PARAM_CONTROLLER);
        return E_NOT_OK;
    }
    if (BaudRatePtr == NULL_PTR) {
        Det_ReportError(CANIF_MODULE_ID, 0U, CANIF_SID_GETBAUDRATE, CANIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    #endif
    
    *BaudRatePtr = CANIF_DEFAULT_BAUDRATE;
    return E_OK;
}

#define CANIF_STOP_SEC_CODE
#include "MemMap.h"
