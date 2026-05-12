/*******************************************************************************
 * @file    E2E_P04.h
 * @brief   E2E Profile 4 - CRC32 Protection
 * @details High-performance 32-bit CRC for large data protection
 * @author  AutoSAR Team
 * @version 1.0.0
 ******************************************************************************/

#ifndef E2E_P04_H
#define E2E_P04_H

#include "E2E.h"
#include "E2E_Cfg.h"

/*=============================================================================*
 * Profile 4 Specific Definitions
 *=============================================================================*/
#define E2E_P04_CRC_SIZE       4U
#define E2E_P04_COUNTER_SIZE   16U
#define E2E_P04_DATAID_SIZE    4U
#define E2E_P04_OVERHEAD       6U  /* CRC32 + Counter */

#define E2E_P04_COUNTER_MAX    0xFFFFU
#define E2E_P04_CRC32_POLYNOMIAL 0x04C11DB7U  /* Ethernet CRC */

/*=============================================================================*
 * Profile 4 Type Definitions
 *=============================================================================*/

typedef struct {
    uint32 DataID;
    uint32 DataLength;
    uint32 CounterOffset;
    uint32 CRCOffset;
    boolean IncludeDataID;
} E2E_P04ConfigType;

typedef struct {
    uint16 Counter;
} E2E_P04ProtectStateType;

typedef struct {
    uint16 LastValidCounter;
    uint16 MaxDeltaCounterInit;
    uint8 SyncCounter;
    uint8 WaitForFirstData;
    E2E_PCheckStatusType Status;
} E2E_P04CheckStateType;

/*=============================================================================*
 * Profile 4 Function Prototypes
 *=============================================================================*/
Std_ReturnType E2E_P04Protect(
    const E2E_P04ConfigType* Config,
    E2E_P04ProtectStateType* State,
    uint8* Data
);

Std_ReturnType E2E_P04Check(
    const E2E_P04ConfigType* Config,
    E2E_P04CheckStateType* State,
    const uint8* Data
);

void E2E_P04MapStatusToSM(
    E2E_PCheckStatusType CheckStatus,
    E2E_SMStateType* SMState,
    boolean* Error
);

#endif /* E2E_P04_H */
