/**
 * @file E2E_P05.h
 * @brief E2E Profile 5 Header - CRC64 with 32-bit Counter
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * Profile 5 uses 64-bit CRC (CRC-64-ECMA) with 32-bit sequence counter
 * for highest integrity requirements.
 */

#ifndef E2E_P05_H
#define E2E_P05_H

#include "E2E.h"

/*=============================================================================*
 * Configuration Types
 *=============================================================================*/

typedef struct {
    uint32 DataLength;          /**< Length of data in bytes */
    uint32 DataID;              /**< Unique data identifier */
    uint32 CounterOffset;       /**< Bit offset of counter in data */
    uint32 CRCOffset;           /**< Bit offset of CRC in data */
    uint32 DataIDOffset;        /**< Bit offset of DataID in data */
    uint32 MaxDeltaCounterInit; /**< Maximum allowed counter jump */
    boolean IncludeDataID;      /**< Include DataID in CRC calculation */
} E2E_P05ConfigType;

typedef struct {
    uint32 Counter;             /**< Sequence counter */
} E2E_P05ProtectStateType;

typedef struct {
    uint32 LastValidCounter;    /**< Last valid counter value */
    uint32 MaxDeltaCounterInit; /**< Maximum allowed counter delta */
    uint8 WaitForFirstData;     /**< Flag for first data received */
    uint8 NewDataAvailable;     /**< Flag for new data available */
    uint8 LostData;             /**< Count of lost data */
    E2E_PCheckStatusType Status; /**< Check status */
} E2E_P05CheckStateType;

/*=============================================================================*
 * Constants
 *=============================================================================*/
#define E2E_P05_COUNTER_MAX     0xFFFFFFFFU
#define E2E_P05_CRC_SIZE        8U

/*=============================================================================*
 * Function Prototypes
 *=============================================================================*/
Std_ReturnType E2E_P05Protect(
    const E2E_P05ConfigType* Config,
    E2E_P05ProtectStateType* State,
    uint8* Data
);

Std_ReturnType E2E_P05Check(
    const E2E_P05ConfigType* Config,
    E2E_P05CheckStateType* State,
    const uint8* Data
);

void E2E_P05MapStatusToSM(
    E2E_PCheckStatusType CheckStatus,
    E2E_SMStateType* SMState,
    boolean* Error
);

#endif /* E2E_P05_H */
