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
 * @file    E2E_P02.h
 * @brief   E2E Profile 2 - Dual Path (Redundant Transmission)
 * @details Uses dual transmission path for high reliability applications
 * @author  AutoSAR Team
 * @version 1.0.0
 ******************************************************************************/

#ifndef E2E_P02_H
#define E2E_P02_H

#include "E2E.h"
#include "E2E_Cfg.h"

/*=============================================================================*
 * Profile 2 Specific Definitions
 *=============================================================================*/
#define E2E_P02_CRC_SIZE       1U
#define E2E_P02_COUNTER_SIZE   8U
#define E2E_P02_DATAID_SIZE    2U
#define E2E_P02_OVERHEAD       2U

#define E2E_P02_COUNTER_MAX    255U
#define E2E_P02_CRC8_POLYNOMIAL 0x2FU

/*=============================================================================*
 * Profile 2 Type Definitions
 *=============================================================================*/

typedef struct {
    uint16 DataID;
    uint16 DataLength;
    uint8 CounterOffset;
    uint8 CRCOffset;
    uint8 DataIDNibbleOffset;
    boolean DualPathEnabled;
} E2E_P02ConfigType;

typedef struct {
    uint8 Counter;
    uint8 PathId;       /* 0 or 1 for dual path */
} E2E_P02ProtectStateType;

typedef struct {
    uint8 LastValidCounter;
    uint8 MaxDeltaCounterInit;
    uint8 SyncCounter;
    uint8 WaitForFirstData;
    uint8 PathStatus[2]; /* Status for each path */
    E2E_PCheckStatusType Status;
} E2E_P02CheckStateType;

/** @req SWS_E2E_00006 */
/*=============================================================================*
 * Profile 2 Function Prototypes
 *=============================================================================*/
Std_ReturnType E2E_P02Protect(
    const E2E_P02ConfigType* Config,
    E2E_P02ProtectStateType* State,
    uint8* Data
);

/** @req SWS_E2E_00007 */
Std_ReturnType E2E_P02Check(
    const E2E_P02ConfigType* Config,
    E2E_P02CheckStateType* State,
    const uint8* Data,
    uint8 PathId
);

/** @req SWS_E2E_00008 */
void E2E_P02MapStatusToSM(
    E2E_PCheckStatusType CheckStatus,
    E2E_SMStateType* SMState,
    boolean* Error
);

#endif /* E2E_P02_H */
