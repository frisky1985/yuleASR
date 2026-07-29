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
 * @file EthSM.c
 * @brief Ethernet State Manager Implementation
 */

#include "EthSM.h"
#include "EthSM_Cfg.h"
#include "Det.h"

typedef struct {
    EthSM_StateType State;
    EthSM_ModeType RequestedMode;
} EthSM_ChannelType;

static EthSM_ChannelType EthSM_Channels[ETHSM_MAX_NETWORKS];
static const EthSM_ConfigType* EthSM_ConfigPtr = NULL_PTR;
static uint8 EthSM_InitState = 0U;

void EthSM_Init(const EthSM_ConfigType* ConfigPtr) {
#if (ETHSM_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ConfigPtr) {
        Det_ReportError(ETHSM_MODULE_ID, 0U, ETHSM_SID_INIT, ETHSM_E_PARAM_POINTER);
        return;
    }
#endif
    EthSM_ConfigPtr = ConfigPtr;
    for (uint8 i = 0U; i < ETHSM_MAX_NETWORKS; i++) {
        EthSM_Channels[i].State = ETHSM_STATE_OFFLINE;
        EthSM_Channels[i].RequestedMode = ETHSM_MODE_NONE;
    }
    EthSM_InitState = 1U;
}

void EthSM_DeInit(void) {
    for (uint8 i = 0U; i < ETHSM_MAX_NETWORKS; i++) {
        EthSM_Channels[i].State = ETHSM_STATE_UNINIT;
    }
    EthSM_InitState = 0U;
    EthSM_ConfigPtr = NULL_PTR;
}

Std_ReturnType EthSM_RequestComMode(uint8 NetworkHandle, EthSM_ModeType Mode) {
#if (ETHSM_DEV_ERROR_DETECT == STD_ON)
    if (EthSM_InitState == 0U) {
        Det_ReportError(ETHSM_MODULE_ID, 0U, ETHSM_SID_REQUEST_MODE, ETHSM_E_UNINIT);
        return E_NOT_OK;
    }
    if (NetworkHandle >= ETHSM_MAX_NETWORKS) {
        Det_ReportError(ETHSM_MODULE_ID, 0U, ETHSM_SID_REQUEST_MODE, ETHSM_E_PARAM_INVALID);
        return E_NOT_OK;
    }
#endif
    EthSM_Channels[NetworkHandle].RequestedMode = Mode;
    return E_OK;
}

EthSM_StateType EthSM_GetState(uint8 NetworkHandle) {
#if (ETHSM_DEV_ERROR_DETECT == STD_ON)
    if (EthSM_InitState == 0U) {
        Det_ReportError(ETHSM_MODULE_ID, 0U, ETHSM_SID_GET_STATE, ETHSM_E_UNINIT);
        return ETHSM_STATE_UNINIT;
    }
#endif
    if (NetworkHandle >= ETHSM_MAX_NETWORKS) {
        return ETHSM_STATE_UNINIT;
    }
    return EthSM_Channels[NetworkHandle].State;
}

void EthSM_MainFunction(void) {
    if (EthSM_InitState == 0U) {
        return;
    }
    for (uint8 i = 0U; i < ETHSM_MAX_NETWORKS; i++) {
        if (EthSM_Channels[i].RequestedMode == ETHSM_MODE_ACTIVE) {
            EthSM_Channels[i].State = ETHSM_STATE_ONLINE;
        } else if (EthSM_Channels[i].RequestedMode == ETHSM_MODE_PASSIVE) {
            EthSM_Channels[i].State = ETHSM_STATE_OFFLINE;
        }
    }
}

void EthSM_TcpIpModeIndication(uint8 NetworkHandle, uint8 Mode) {
    (void)NetworkHandle;
    (void)Mode;
}

void EthSM_EthIfModeIndication(uint8 CtrlIdx, uint8 Mode) {
    (void)CtrlIdx;
    (void)Mode;
}
