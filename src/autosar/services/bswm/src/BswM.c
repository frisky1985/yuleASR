/**
 * @file BswM.c
 * @brief BswM Implementation
 */

#include "BswM.h"
#include "BswM_Cfg.h"
#include "Det.h"
#include "SchM_BswM.h"

typedef enum {
    BSWM_UNINIT = 0,
    BSWM_INIT
} BswM_StateType;

static BswM_StateType BswM_State = BSWM_UNINIT;
static const BswM_ConfigType* BswM_ConfigPtr = NULL_PTR;

void BswM_Init(const BswM_ConfigType* ConfigPtr) {
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ConfigPtr) {
        Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_INIT, BSWM_E_PARAM_POINTER);
        return;
    }
#endif
    
    SchM_Enter_BswM(BSWM_EXCLUSIVE_AREA_0);
    BswM_ConfigPtr = ConfigPtr;
    BswM_State = BSWM_INIT;
    SchM_Exit_BswM(BSWM_EXCLUSIVE_AREA_0);
}

void BswM_DeInit(void) {
    SchM_Enter_BswM(BSWM_EXCLUSIVE_AREA_0);
    BswM_ConfigPtr = NULL_PTR;
    BswM_State = BSWM_UNINIT;
    SchM_Exit_BswM(BSWM_EXCLUSIVE_AREA_0);
}

#if (BSWM_VERSION_INFO_API == STD_ON)
void BswM_GetVersionInfo(Std_VersionInfoType* VersionInfo) {
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == VersionInfo) {
        Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_GET_VERSION_INFO, BSWM_E_PARAM_POINTER);
        return;
    }
#endif
    VersionInfo->vendorID = BSWM_VENDOR_ID;
    VersionInfo->moduleID = BSWM_MODULE_ID;
    VersionInfo->sw_major_version = 1U;
    VersionInfo->sw_minor_version = 0U;
    VersionInfo->sw_patch_version = 0U;
}
#endif

void BswM_RequestMode(uint16 PortId, BswM_ModeType Mode) {
#if (BSWM_DEV_ERROR_DETECT == STD_ON)
    if (BSWM_UNINIT == BswM_State) {
        Det_ReportError(BSWM_MODULE_ID, 0U, BSWM_SID_REQUEST_MODE, BSWM_E_UNINIT);
        return;
    }
#endif
    
    SchM_Enter_BswM(BSWM_EXCLUSIVE_AREA_0);
    /* Update mode request */
    SchM_Exit_BswM(BSWM_EXCLUSIVE_AREA_0);
}

void BswM_MainFunction(void) {
    if (BSWM_UNINIT == BswM_State) {
        return;
    }
    
    /* Process rules */
    if (NULL_PTR != BswM_ConfigPtr) {
        for (uint16 i = 0U; i < BSWM_MAX_RULES; i++) {
            /* Evaluate and execute rules */
        }
    }
}

void BswM_EcuM_CurrentState(uint8 State) {
    (void)State;
}

void BswM_ComM_CurrentMode(uint8 Network, uint8 Mode) {
    (void)Network;
    (void)Mode;
}

void BswM_Dcm_RequestCommunicationMode(uint8 Mode) {
    (void)Mode;
}
