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

/*******************************************************************************
 * @file    E2E_P01.h
 * @brief   E2E Profile 1 - CRC8 + Counter + DataID
 * @details ASIL-D level protection with 8-bit CRC, 4-bit counter
 * @author  AutoSAR Team
 * @version 1.0.0
 ******************************************************************************/

#ifndef E2E_P01_H
#define E2E_P01_H

#include "E2E.h"
#include "E2E_Cfg.h"

/*=============================================================================*
 * Profile 1 Specific Definitions
 *=============================================================================*/
#define E2E_P01_CRC_SIZE       1U
#define E2E_P01_COUNTER_SIZE   4U  /* 4-bit counter */
#define E2E_P01_DATAID_SIZE    2U
#define E2E_P01_OVERHEAD       2U  /* CRC + Counter byte */

#define E2E_P01_COUNTER_MAX    14U /* Counter wraps at 14 */
#define E2E_P01_CRC8_POLYNOMIAL 0x1DU

/*=============================================================================*
 * Profile 1 Type Definitions
 *=============================================================================*/

typedef struct {
    uint16 DataID;
    uint16 DataLength;
    uint8 DataIDMode;
    uint8 CounterOffset;
    uint8 CRCOffset;
    uint8 DataIDNibbleOffset;
} E2E_P01ConfigType;

typedef struct {
    uint8 Counter;
} E2E_P01ProtectStateType;

typedef struct {
    uint8 LastValidCounter;
    uint8 MaxDeltaCounterInit;
    uint8 SyncCounter;
    uint8 WaitForFirstData;
    boolean NewDataAvailable;
    E2E_PCheckStatusType Status;
} E2E_P01CheckStateType;

/* DataID Modes */
#define E2E_P01_DATAID_BOTH     0x00U  /* Both bytes included in CRC */
#define E2E_P01_DATAID_ALT      0x01U  /* Alternating high/low nibble */
#define E2E_P01_DATAID_LOW      0x02U  /* Low byte only */
#define E2E_P01_DATAID_NIBBLE   0x03U  /* Nibble mode */

/*=============================================================================*
 * Profile 1 Function Prototypes
 *=============================================================================*/
Std_ReturnType E2E_P01Protect(
    const E2E_P01ConfigType* Config,
    E2E_P01ProtectStateType* State,
    uint8* Data
);

Std_ReturnType E2E_P01Check(
    const E2E_P01ConfigType* Config,
    E2E_P01CheckStateType* State,
    const uint8* Data
);

void E2E_P01MapStatusToSM(
    E2E_PCheckStatusType CheckStatus,
    E2E_SMStateType* SMState,
    boolean* Error
);

#endif /* E2E_P01_H */
