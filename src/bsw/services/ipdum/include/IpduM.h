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
 * @file IpduM.h
 * @brief IPDU Multiplexer (Services Layer)
 * @version 1.0.0
 */

#ifndef IPDUM_H
#define IPDUM_H

#include "Std_Types.h"
#include "ComStack_Types.h"

#define IPDUM_MODULE_ID         0x51U
#define IPDUM_VENDOR_ID         0x0001U

/* Error Codes */
#define IPDUM_E_NO_ERROR        0x00U
#define IPDUM_E_PARAM_POINTER   0x01U
#define IPDUM_E_UNINIT          0x02U
#define IPDUM_E_PARAM_INVALID   0x03U

/* Service IDs */
#define IPDUM_SID_INIT          0x01U
#define IPDUM_SID_DEINIT        0x02U
#define IPDUM_SID_TRANSMIT      0x03U
#define IPDUM_SID_RX_INDICATION 0x04U
#define IPDUM_SID_MAIN_FUNCTION 0x05U

/* Selector Field Types */
typedef uint8 IpduM_SelType;

typedef struct {
    PduIdType TxPduId;
    PduIdType RxPduId;
    IpduM_SelType SelectorValue;
} IpduM_StaticPartType;

typedef struct {
    uint8 NumStaticParts;
    const IpduM_StaticPartType* StaticParts;
} IpduM_ConfigType;

/* Functions */
void IpduM_Init(const IpduM_ConfigType* ConfigPtr);
void IpduM_DeInit(void);
Std_ReturnType IpduM_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);
void IpduM_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
void IpduM_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);
void IpduM_MainFunction(void);

#endif
