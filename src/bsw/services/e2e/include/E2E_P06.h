/**
 * @file E2E_P06.h
 * @brief E2E Profile 6 Header - CRC64 with Dynamic Length
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * Profile 6 is similar to Profile 5 but supports dynamic data length.
 * Uses CRC-64 with length field included in CRC calculation.
 */

#ifndef E2E_P06_H
#define E2E_P06_H

#include "E2E.h"

/*=============================================================================*
 * Configuration Types
 *=============================================================================*/

typedef struct {
    uint32 DataID;              /**< Unique data identifier */
    uint32 CounterOffset;       /**< Bit offset of counter in data */
    uint32 CRCOffset;           /**< Bit offset of CRC in data */
    uint32 LengthOffset;        /**< Bit offset of length field in data */
    uint32 MaxDeltaCounterInit; /**< Maximum allowed counter jump */
    uint32 MinDataLength;       /**< Minimum allowed data length */
    uint32 MaxDataLength;       /**< Maximum allowed data length */
    boolean IncludeDataID;      /**< Include DataID in CRC calculation */
} E2E_P06ConfigType;

typedef struct {
    uint16 Counter;             /**< Sequence counter (16-bit) */
} E2E_P06ProtectStateType;

typedef struct {
    uint16 LastValidCounter;    /**< Last valid counter value */
    uint32 MaxDeltaCounterInit; /**< Maximum allowed counter delta */
    uint8 WaitForFirstData;     /**< Flag for first data received */
    uint8 NewDataAvailable;     /**< Flag for new data available */
    uint8 LostData;             /**< Count of lost data */
    E2E_PCheckStatusType Status; /**< Check status */
} E2E_P06CheckStateType;

/*=============================================================================*
 * Constants
 *=============================================================================*/
#define E2E_P06_COUNTER_MAX     0xFFFFU
#define E2E_P06_CRC_SIZE        8U

/** @req SWS_E2E_00015 */
/*=============================================================================*
 * Function Prototypes
 *=============================================================================*/
Std_ReturnType E2E_P06Protect(
    const E2E_P06ConfigType* Config,
    E2E_P06ProtectStateType* State,
    uint8* Data,
    uint32 Length
);

/** @req SWS_E2E_00016 */
Std_ReturnType E2E_P06Check(
    const E2E_P06ConfigType* Config,
    E2E_P06CheckStateType* State,
    const uint8* Data,
    uint32 Length
);

/** @req SWS_E2E_00017 */
void E2E_P06MapStatusToSM(
    E2E_PCheckStatusType CheckStatus,
    E2E_SMStateType* SMState,
    boolean* Error
);

#endif /* E2E_P06_H */
