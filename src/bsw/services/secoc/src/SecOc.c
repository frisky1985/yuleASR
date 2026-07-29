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

/**
 * @file SecOc.c
 * @brief SecOc Implementation
 */

#include "SecOc.h"
#include "SecOc_Cfg.h"
#include "Det.h"
#include "Csm.h"
#include "SchM_SecOc.h"

typedef enum {
    SECOC_UNINIT = 0,
    SECOC_INIT
} SecOc_StateType;

static SecOc_StateType SecOc_State = SECOC_UNINIT;
static const SecOc_ConfigType* SecOc_ConfigPtr = NULL_PTR;

void SecOc_Init(const SecOc_ConfigType* ConfigPtr) {
#if (SECOC_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ConfigPtr) {
        Det_ReportError(SECOC_MODULE_ID, 0U, SECOC_SID_INIT, SECOC_E_PARAM_POINTER);
        return;
    }
#endif
    
    SchM_Enter_SecOc(SECOC_EXCLUSIVE_AREA_0);
    SecOc_ConfigPtr = ConfigPtr;
    SecOc_State = SECOC_INIT;
    SchM_Exit_SecOc(SECOC_EXCLUSIVE_AREA_0);
}

void SecOc_DeInit(void) {
    SchM_Enter_SecOc(SECOC_EXCLUSIVE_AREA_0);
    SecOc_ConfigPtr = NULL_PTR;
    SecOc_State = SECOC_UNINIT;
    SchM_Exit_SecOc(SECOC_EXCLUSIVE_AREA_0);
}

#if (SECOC_VERSION_INFO_API == STD_ON)
void SecOc_GetVersionInfo(Std_VersionInfoType* VersionInfo) {
#if (SECOC_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == VersionInfo) {
        Det_ReportError(SECOC_MODULE_ID, 0U, SECOC_SID_GET_VERSION_INFO, SECOC_E_PARAM_POINTER);
        return;
    }
#endif
    VersionInfo->vendorID = SECOC_VENDOR_ID;
    VersionInfo->moduleID = SECOC_MODULE_ID;
    VersionInfo->sw_major_version = 1U;
    VersionInfo->sw_minor_version = 0U;
    VersionInfo->sw_patch_version = 0U;
}
#endif

Std_ReturnType SecOc_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr) {
    Std_ReturnType result = E_NOT_OK;
    
#if (SECOC_DEV_ERROR_DETECT == STD_ON)
    if (SECOC_UNINIT == SecOc_State) {
        Det_ReportError(SECOC_MODULE_ID, 0U, SECOC_SID_TRANSMIT, SECOC_E_UNINIT);
        return E_NOT_OK;
    }
    if (NULL_PTR == PduInfoPtr) {
        Det_ReportError(SECOC_MODULE_ID, 0U, SECOC_SID_TRANSMIT, SECOC_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    
    if ((NULL_PTR != SecOc_ConfigPtr) && (TxPduId < SECOC_MAX_PDUS)) {
        /* Generate authenticator and transmit */
        result = E_OK;
    }
    
    return result;
}

void SecOc_VerifyStatusOverride(PduIdType PduId, SecOc_VerificationStatusType Status) {
    (void)PduId;
    (void)Status;
}

void SecOc_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr) {
#if (SECOC_DEV_ERROR_DETECT == STD_ON)
    if (SECOC_UNINIT == SecOc_State) {
        Det_ReportError(SECOC_MODULE_ID, 0U, 0x10U, SECOC_E_UNINIT);
        return;
    }
#endif
    (void)RxPduId;
    (void)PduInfoPtr;
}

void SecOc_MainFunction(void) {
    if (SECOC_UNINIT == SecOc_State) {
        return;
    }
    /* Process verification */
}
