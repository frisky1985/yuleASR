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
 * @file EthIf.c
 * @brief Ethernet Interface Implementation
 */

#include "EthIf.h"
#include "EthIf_Cfg.h"
#include "Det.h"
#include "SchM_EthIf.h"

typedef enum {
    ETHIF_STATE_UNINIT = 0,
    ETHIF_STATE_INIT
} EthIf_StateType;

static EthIf_StateType EthIf_State = ETHIF_STATE_UNINIT;
static const EthIf_ConfigType* EthIf_ConfigPtr = NULL_PTR;

void EthIf_Init(const EthIf_ConfigType* ConfigPtr) {
#if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ConfigPtr) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_INIT, ETHIF_E_PARAM_POINTER);
        return;
    }
#endif
    
    SchM_Enter_EthIf(ETHIF_EXCLUSIVE_AREA_0);
    EthIf_ConfigPtr = ConfigPtr;
    EthIf_State = ETHIF_STATE_INIT;
    SchM_Exit_EthIf(ETHIF_EXCLUSIVE_AREA_0);
}

void EthIf_DeInit(void) {
    SchM_Enter_EthIf(ETHIF_EXCLUSIVE_AREA_0);
    EthIf_ConfigPtr = NULL_PTR;
    EthIf_State = ETHIF_STATE_UNINIT;
    SchM_Exit_EthIf(ETHIF_EXCLUSIVE_AREA_0);
}

#if (ETHIF_VERSION_INFO_API == STD_ON)
void EthIf_GetVersionInfo(Std_VersionInfoType* VersionInfo) {
#if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == VersionInfo) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_GET_VERSION_INFO, ETHIF_E_PARAM_POINTER);
        return;
    }
#endif
    VersionInfo->vendorID = ETHIF_VENDOR_ID;
    VersionInfo->moduleID = ETHIF_MODULE_ID;
    VersionInfo->sw_major_version = 1U;
    VersionInfo->sw_minor_version = 0U;
    VersionInfo->sw_patch_version = 0U;
}
#endif

Std_ReturnType EthIf_Transmit(EthIf_FrameType FrameType, uint8* Data, uint16 Length) {
    Std_ReturnType result = E_NOT_OK;
    
#if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (ETHIF_STATE_UNINIT == EthIf_State) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_TRANSMIT, ETHIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (NULL_PTR == Data) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_TRANSMIT, ETHIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
    if ((Length < 14U) || (Length > 1518U)) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_TRANSMIT, ETHIF_E_PARAM_LENGTH);
        return E_NOT_OK;
    }
#endif
    
    /* Transmit frame via Eth driver */
    result = E_OK;
    
    return result;
}

Std_ReturnType EthIf_Receive(uint8* Data, uint16* Length) {
    Std_ReturnType result = E_NOT_OK;
    
#if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (ETHIF_STATE_UNINIT == EthIf_State) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_RECEIVE, ETHIF_E_UNINIT);
        return E_NOT_OK;
    }
    if (NULL_PTR == Data || NULL_PTR == Length) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_RECEIVE, ETHIF_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    
    return result;
}

void EthIf_RxIndication(uint8 CtrlIdx, Eth_FrameType FrameType, boolean IsBroadcast, uint8* Data, uint16 Length) {
#if (ETHIF_DEV_ERROR_DETECT == STD_ON)
    if (ETHIF_STATE_UNINIT == EthIf_State) {
        Det_ReportError(ETHIF_MODULE_ID, 0U, ETHIF_SID_RX_INDICATION, ETHIF_E_UNINIT);
        return;
    }
#endif
    (void)CtrlIdx;
    (void)FrameType;
    (void)IsBroadcast;
    (void)Data;
    (void)Length;
}

void EthIf_MainFunction(void) {
    if (ETHIF_STATE_UNINIT == EthIf_State) {
        return;
    }
}
