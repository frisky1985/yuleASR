/**
 * @file E2E_P07.h
 * @brief E2E Profile 7 Header - CRC32 with Dynamic Length
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 *
 * Profile 7 uses CRC32 with dynamic data length for flexible data protection.
 * Similar to Profile 6 but uses 32-bit CRC instead of 64-bit.
 */

#ifndef E2E_P07_H
#define E2E_P07_H

#include "E2E.h"

/*=============================================================================*
 * Configuration Types
 *=============================================================================*/

typedef struct {
    uint32 DataID;              /**< Unique data identifier */
    uint32 CounterOffset;       /**< Bit offset of counter in data */
    uint32 CRCOffset;           /**< Bit offset of CRC in data */
    uint32 MaxDeltaCounterInit; /**< Maximum allowed counter jump */
    uint32 MinDataLength;       /**< Minimum allowed data length */
    uint32 MaxDataLength;       /**< Maximum allowed data length */
    boolean IncludeDataID;      /**< Include DataID in CRC calculation */
} E2E_P07ConfigType;

typedef struct {
    uint8 Counter;              /**< Sequence counter (8-bit) */
} E2E_P07ProtectStateType;

typedef struct {
    uint8 LastValidCounter;     /**< Last valid counter value */
    uint32 MaxDeltaCounterInit; /**< Maximum allowed counter delta */
    uint8 WaitForFirstData;     /**< Flag for first data received */
    uint8 NewDataAvailable;     /**< Flag for new data available */
    uint8 LostData;             /**< Count of lost data */
    E2E_PCheckStatusType Status; /**< Check status */
} E2E_P07CheckStateType;

/*=============================================================================*
 * Constants
 *=============================================================================*/
#define E2E_P07_COUNTER_MAX     0xFFU
#define E2E_P07_CRC_SIZE        4U

/** @req SWS_E2E_00018 */
/*=============================================================================*
 * Function Prototypes
 *=============================================================================*/
Std_ReturnType E2E_P07Protect(
    const E2E_P07ConfigType* Config,
    E2E_P07ProtectStateType* State,
    uint8* Data,
    uint32 Length
);

/** @req SWS_E2E_00019 */
Std_ReturnType E2E_P07Check(
    const E2E_P07ConfigType* Config,
    E2E_P07CheckStateType* State,
    const uint8* Data,
    uint32 Length
);

/** @req SWS_E2E_00020 */
void E2E_P07MapStatusToSM(
    E2E_PCheckStatusType CheckStatus,
    E2E_SMStateType* SMState,
    boolean* Error
);

#endif /* E2E_P07_H */
