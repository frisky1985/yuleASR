/**
 * @file LinIf.c
 * @brief LIN Interface Implementation
 */

#include "LinIf.h"
#include "LinIf_Cfg.h"
#include "Det.h"
#include "SchM_LinIf.h"

typedef enum {
    LINIF_STATE_UNINIT = 0,
    LINIF_STATE_INIT,
    LINIF_STATE_RUNNING
} LinIf_StateType;

static LinIf_StateType LinIf_State = LINIF_STATE_UNINIT;
static const LinIf_ConfigType* LinIf_ConfigPtr = NULL_PTR;

void LinIf_Init(const LinIf_ConfigType* ConfigPtr) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ConfigPtr) {
        Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_INIT, LINIF_E_PARAM_POINTER);
        return;
    }
#endif
    
    SchM_Enter_LinIf(LINIF_EXCLUSIVE_AREA_0);
    LinIf_ConfigPtr = ConfigPtr;
    LinIf_State = LINIF_STATE_INIT;
    SchM_Exit_LinIf(LINIF_EXCLUSIVE_AREA_0);
}

void LinIf_DeInit(void) {
    SchM_Enter_LinIf(LINIF_EXCLUSIVE_AREA_0);
    LinIf_ConfigPtr = NULL_PTR;
    LinIf_State = LINIF_STATE_UNINIT;
    SchM_Exit_LinIf(LINIF_EXCLUSIVE_AREA_0);
}

#if (LINIF_VERSION_INFO_API == STD_ON)
void LinIf_GetVersionInfo(Std_VersionInfoType* VersionInfo) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == VersionInfo) {
        Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_GET_VERSION_INFO, LINIF_E_PARAM_POINTER);
        return;
    }
#endif
    VersionInfo->vendorID = LINIF_VENDOR_ID;
    VersionInfo->moduleID = LINIF_MODULE_ID;
    VersionInfo->sw_major_version = 1U;
    VersionInfo->sw_minor_version = 0U;
    VersionInfo->sw_patch_version = 0U;
}
#endif

Std_ReturnType LinIf_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr) {
    Std_ReturnType result = E_NOT_OK;
    
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
    if (LINIF_STATE_UNINIT == LinIf_State) {
        Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_TRANSMIT, LINIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (NULL_PTR == PduInfoPtr) {
        Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_TRANSMIT, LINIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    
    (void)TxPduId;
    result = E_OK;
    
    return result;
}

Std_ReturnType LinIf_ScheduleRequest(uint8 Channel, LinIf_ScheduleTableType Schedule) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
    if (LINIF_STATE_UNINIT == LinIf_State) {
        Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_SCHEDULE_REQUEST, LINIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (Channel >= LINIF_MAX_CHANNELS) {
        Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_SCHEDULE_REQUEST, LINIF_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
#endif
    (void)Schedule;
    return E_OK;
}

Std_ReturnType LinIf_WakeUp(uint8 Channel) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
    if (LINIF_STATE_UNINIT == LinIf_State) {
        Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_WAKEUP, LINIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (Channel >= LINIF_MAX_CHANNELS) {
        Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_WAKEUP, LINIF_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
#endif
    return E_OK;
}

Std_ReturnType LinIf_GotoSleep(uint8 Channel) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
    if (LINIF_STATE_UNINIT == LinIf_State) {
        Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_GOTOSLEEP, LINIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (Channel >= LINIF_MAX_CHANNELS) {
        Det_ReportError(LINIF_MODULE_ID, 0U, LINIF_SID_GOTOSLEEP, LINIF_E_INVALID_CHANNEL);
        return E_NOT_OK;
    }
#endif
    return E_OK;
}

void LinIf_MainFunction(void) {
    if (LINIF_STATE_UNINIT == LinIf_State) {
        return;
    }
}

void LinIf_RxIndication(uint8 Channel, uint8* Data, uint8 Length) {
#if (LINIF_DEV_ERROR_DETECT == STD_ON)
    if (LINIF_STATE_UNINIT == LinIf_State) {
        Det_ReportError(LINIF_MODULE_ID, 0U, 0x10U, LINIF_E_UNINIT);
        return;
    }
#endif
    (void)Channel;
    (void)Data;
    (void)Length;
}

void LinIf_TxConfirmation(uint8 Channel, Std_ReturnType Result) {
    (void)Channel;
    (void)Result;
}

void LinIf_WakeUpConfirmation(uint8 Channel, boolean Success) {
    (void)Channel;
    (void)Success;
}
