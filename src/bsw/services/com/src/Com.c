/**
 * @file Com.c
 * @brief COM Implementation
 */

#include "Com.h"
#include "Com_Cfg.h"
#include "Det.h"
#include "SchM_Com.h"

typedef enum {
    COM_STATE_UNINIT = 0,
    COM_STATE_INIT
} Com_StateType;

static Com_StateType Com_State = COM_STATE_UNINIT;
static const Com_ConfigType* Com_ConfigPtr = NULL_PTR;

void Com_Init(const Com_ConfigType* ConfigPtr) {
#if (COM_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ConfigPtr) {
        Det_ReportError(COM_MODULE_ID, 0U, COM_SID_INIT, COM_E_PARAM_POINTER);
        return;
    }
#endif
    
    SchM_Enter_Com(COM_EXCLUSIVE_AREA_0);
    Com_ConfigPtr = ConfigPtr;
    Com_State = COM_STATE_INIT;
    SchM_Exit_Com(COM_EXCLUSIVE_AREA_0);
}

void Com_DeInit(void) {
    SchM_Enter_Com(COM_EXCLUSIVE_AREA_0);
    Com_ConfigPtr = NULL_PTR;
    Com_State = COM_STATE_UNINIT;
    SchM_Exit_Com(COM_EXCLUSIVE_AREA_0);
}

#if (COM_VERSION_INFO_API == STD_ON)
void Com_GetVersionInfo(Std_VersionInfoType* VersionInfo) {
#if (COM_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == VersionInfo) {
        Det_ReportError(COM_MODULE_ID, 0U, COM_SID_GET_VERSION_INFO, COM_E_PARAM_POINTER);
        return;
    }
#endif
    VersionInfo->vendorID = COM_VENDOR_ID;
    VersionInfo->moduleID = COM_MODULE_ID;
    VersionInfo->sw_major_version = 1U;
    VersionInfo->sw_minor_version = 0U;
    VersionInfo->sw_patch_version = 0U;
}
#endif

uint8 Com_SendSignal(uint16 SignalId, const void* SignalDataPtr) {
    uint8 result = 0x01U; /* COM_SERVICE_NOT_AVAILABLE */
    
#if (COM_DEV_ERROR_DETECT == STD_ON)
    if (COM_STATE_UNINIT == Com_State) {
        Det_ReportError(COM_MODULE_ID, 0U, COM_SID_SEND_SIGNAL, COM_E_UNINIT);
        return 0x01U;
    }
    if (NULL_PTR == SignalDataPtr) {
        Det_ReportError(COM_MODULE_ID, 0U, COM_SID_SEND_SIGNAL, COM_E_PARAM_POINTER);
        return 0x01U;
    }
#endif
    
    if ((NULL_PTR != Com_ConfigPtr) && (SignalId < Com_ConfigPtr->NumSignals)) {
        result = 0x00U; /* COM_OK */
    }
    
    return result;
}

uint8 Com_ReceiveSignal(uint16 SignalId, void* SignalDataPtr) {
    uint8 result = 0x01U;
    
#if (COM_DEV_ERROR_DETECT == STD_ON)
    if (COM_STATE_UNINIT == Com_State) {
        Det_ReportError(COM_MODULE_ID, 0U, COM_SID_RECEIVE_SIGNAL, COM_E_UNINIT);
        return 0x01U;
    }
    if (NULL_PTR == SignalDataPtr) {
        Det_ReportError(COM_MODULE_ID, 0U, COM_SID_RECEIVE_SIGNAL, COM_E_PARAM_POINTER);
        return 0x01U;
    }
#endif
    
    if ((NULL_PTR != Com_ConfigPtr) && (SignalId < Com_ConfigPtr->NumSignals)) {
        result = 0x00U;
    }
    
    return result;
}

void Com_IpduGroupControl(uint8 GroupId, boolean Start) {
#if (COM_DEV_ERROR_DETECT == STD_ON)
    if (COM_STATE_UNINIT == Com_State) {
        Det_ReportError(COM_MODULE_ID, 0U, COM_SID_IPDU_GROUP_CONTROL, COM_E_UNINIT);
        return;
    }
#endif
    (void)GroupId;
    (void)Start;
}

void Com_MainFunction(void) {
    if (COM_STATE_UNINIT == Com_State) {
        return;
    }
    
    /* Process transmission and reception */
}

void Com_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr) {
#if (COM_DEV_ERROR_DETECT == STD_ON)
    if (COM_STATE_UNINIT == Com_State) {
        Det_ReportError(COM_MODULE_ID, 0U, 0x10U, COM_E_UNINIT);
        return;
    }
#endif
    (void)RxPduId;
    (void)PduInfoPtr;
}

void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType Result) {
    (void)TxPduId;
    (void)Result;
}
