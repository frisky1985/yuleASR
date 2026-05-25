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
 * @file EthSM.h
 * @brief Ethernet State Manager (Services Layer)
 * @version 1.0.0
 */

#ifndef ETHSM_H
#define ETHSM_H

#include "Std_Types.h"

#define ETHSM_MODULE_ID         0x4EU
#define ETHSM_VENDOR_ID         0x0001U

/* Error Codes */
#define ETHSM_E_NO_ERROR        0x00U
#define ETHSM_E_PARAM_POINTER   0x01U
#define ETHSM_E_UNINIT          0x02U
#define ETHSM_E_PARAM_INVALID   0x03U

/* Service IDs */
#define ETHSM_SID_INIT          0x01U
#define ETHSM_SID_DEINIT        0x02U
#define ETHSM_SID_REQUEST_MODE  0x03U
#define ETHSM_SID_GET_STATE     0x04U
#define ETHSM_SID_MAIN_FUNCTION 0x05U

/* Network Modes */
typedef enum {
    ETHSM_STATE_UNINIT = 0,
    ETHSM_STATE_OFFLINE,
    ETHSM_STATE_ONLINE,
    ETHSM_STATE_WAIT_OFFLINE
} EthSM_StateType;

typedef enum {
    ETHSM_MODE_NONE = 0,
    ETHSM_MODE_ACTIVE,
    ETHSM_MODE_PASSIVE
} EthSM_ModeType;

typedef struct {
    uint8 NetworkHandle;
    uint32 MainFunctionPeriod;
} EthSM_ChannelConfigType;

typedef struct {
    uint8 NumChannels;
    const EthSM_ChannelConfigType* Channels;
} EthSM_ConfigType;

/* Functions */
void EthSM_Init(const EthSM_ConfigType* ConfigPtr);
void EthSM_DeInit(void);
Std_ReturnType EthSM_RequestComMode(uint8 NetworkHandle, EthSM_ModeType Mode);
EthSM_StateType EthSM_GetState(uint8 NetworkHandle);
void EthSM_MainFunction(void);

/* Callbacks */
void EthSM_TcpIpModeIndication(uint8 NetworkHandle, uint8 Mode);
void EthSM_EthIfModeIndication(uint8 CtrlIdx, uint8 Mode);

#endif
