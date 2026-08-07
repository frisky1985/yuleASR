/**
 * @file E2E_P06.c
 * @brief E2E Profile 6 Implementation - CRC64 with Dynamic Length
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 *
 * Profile 6 implements end-to-end protection with dynamic data length:
 * - CRC-64 (CRC-64-ECMA polynomial 0x42F0E1EBA9EA3693)
 * - 16-bit sequence counter
 * - Dynamic length field included in CRC calculation
 * - Optional DataID inclusion
 *
 * Suitable for variable-length data with ASIL-D requirements.
 */

#include "E2E_P06.h"
#include "Crc.h"
#include <string.h>

/*=============================================================================*
 * Local Helper Functions
 *=============================================================================*/

/**
 * @brief Calculate CRC64 including length field
 */
static uint64 E2E_P06_CalculateCRC64(
    const uint8* Data,
    uint32 Length,
    uint64 DataID,
    boolean IncludeDataID
)
{
    uint64 crc;
    uint8 lengthBytes[4];

    /* Include length in CRC calculation */
    lengthBytes[0] = (uint8)(Length & 0xFFU);
    lengthBytes[1] = (uint8)((Length >> 8) & 0xFFU);
    lengthBytes[2] = (uint8)((Length >> 16) & 0xFFU);
    lengthBytes[3] = (uint8)((Length >> 24) & 0xFFU);

    crc = Crc_CalculateCRC64(lengthBytes, 4U, 0ULL, TRUE);

    /* Calculate CRC over data */
    crc = Crc_CalculateCRC64(Data, Length, crc, FALSE);

    /* Include DataID in CRC if configured */
    if ((IncludeDataID) != 0U) {
        uint8 dataIdBytes[4];
        dataIdBytes[0] = (uint8)(DataID & 0xFFU);
        dataIdBytes[1] = (uint8)((DataID >> 8) & 0xFFU);
        dataIdBytes[2] = (uint8)((DataID >> 16) & 0xFFU);
        dataIdBytes[3] = (uint8)((DataID >> 24) & 0xFFU);
        crc = Crc_CalculateCRC64(dataIdBytes, 4U, crc, FALSE);
    }

    return crc;
}

/**
 * @brief Write 16-bit counter to data buffer
 */
static void E2E_P06_WriteCounter(uint8* Data, uint32 Offset, uint16 Counter)
{
    uint32 byteOffset = Offset / 8U;
    uint32 bitOffset = Offset % 8U;

    if (bitOffset == 0U) {
        Data[byteOffset] = (uint8)(Counter & 0xFFU);
        Data[byteOffset + 1U] = (uint8)((Counter >> 8) & 0xFFU);
    } else {
        uint32 i;
        for (i = 0U; i < 16U; i++) {
            uint32 bitPos = Offset + i;
            uint32 byteIdx = bitPos / 8U;
            uint32 bitIdx = bitPos % 8U;
            uint8 bitValue = (uint8)((Counter >> i) & 0x01U);

            if ((bitValue) != 0U) {
                Data[byteIdx] |= (uint8)(1U << bitIdx);
            } else {
                Data[byteIdx] &= (uint8)~(1U << bitIdx);
            }
        }
    }
}

/**
 * @brief Read 16-bit counter from data buffer
 */
static uint16 E2E_P06_ReadCounter(const uint8* Data, uint32 Offset)
{
    uint32 byteOffset = Offset / 8U;
    uint32 bitOffset = Offset % 8U;
    uint16 counter = 0U;

    if (bitOffset == 0U) {
        counter = (uint16)Data[byteOffset] | ((uint16)Data[byteOffset + 1U] << 8);
    } else {
        uint32 i;
        for (i = 0U; i < 16U; i++) {
            uint32 bitPos = Offset + i;
            uint32 byteIdx = bitPos / 8U;
            uint32 bitIdx = bitPos % 8U;

            if ((Data[byteIdx] & (1U << bitIdx)) != 0U) {
                counter |= (1U << i);
            }
        }
    }

    return counter;
}

/**
 * @brief Write 64-bit CRC to data buffer (big endian)
 */
static void E2E_P06_WriteCRC(uint8* Data, uint32 Offset, uint64 CRC)
{
    uint32 byteOffset = Offset / 8U;

    Data[byteOffset] = (uint8)((CRC >> 56) & 0xFFU);
    Data[byteOffset + 1U] = (uint8)((CRC >> 48) & 0xFFU);
    Data[byteOffset + 2U] = (uint8)((CRC >> 40) & 0xFFU);
    Data[byteOffset + 3U] = (uint8)((CRC >> 32) & 0xFFU);
    Data[byteOffset + 4U] = (uint8)((CRC >> 24) & 0xFFU);
    Data[byteOffset + 5U] = (uint8)((CRC >> 16) & 0xFFU);
    Data[byteOffset + 6U] = (uint8)((CRC >> 8) & 0xFFU);
    Data[byteOffset + 7U] = (uint8)(CRC & 0xFFU);
}

/**
 * @brief Read 64-bit CRC from data buffer (big endian)
 */
static uint64 E2E_P06_ReadCRC(const uint8* Data, uint32 Offset)
{
    uint32 byteOffset = Offset / 8U;

    return ((uint64)Data[byteOffset] << 56) |
           ((uint64)Data[byteOffset + 1U] << 48) |
           ((uint64)Data[byteOffset + 2U] << 40) |
           ((uint64)Data[byteOffset + 3U] << 32) |
           ((uint64)Data[byteOffset + 4U] << 24) |
           ((uint64)Data[byteOffset + 5U] << 16) |
           ((uint64)Data[byteOffset + 6U] << 8) |
           (uint64)Data[byteOffset + 7U];
}

/*=============================================================================*
 * Profile 6 Protect
 *=============================================================================*/

Std_ReturnType E2E_P06Protect(
    const E2E_P06ConfigType* Config,
    E2E_P06ProtectStateType* State,
    uint8* Data,
    uint32 Length
)
{
    uint64 crc;

    if ((Config == NULL_PTR) || (State == NULL_PTR) || (Data == NULL_PTR)) {
        return E2E_E_INPUTERR_NULL;
    }

    /* Validate length */
    if ((Length < Config->MinDataLength) || (Length > Config->MaxDataLength)) {
        return E2E_E_INPUTERR_WRONG;
    }

    /* Clear CRC field before calculation */
    E2E_P06_WriteCRC(Data, Config->CRCOffset, 0ULL);

    /* Write 16-bit counter */
    E2E_P06_WriteCounter(Data, Config->CounterOffset, State->Counter);

    /* Calculate CRC64 including length and DataID */
    crc = E2E_P06_CalculateCRC64(Data, Length, Config->DataID, Config->IncludeDataID);

    /* Write CRC to data */
    E2E_P06_WriteCRC(Data, Config->CRCOffset, crc);

    /* Increment counter */
    State->Counter++;
    if (State->Counter > E2E_P06_COUNTER_MAX) {
        State->Counter = 0U;
    }

    return E_OK;
}

/*=============================================================================*
 * Profile 6 Check
 *=============================================================================*/

Std_ReturnType E2E_P06Check(
    const E2E_P06ConfigType* Config,
    E2E_P06CheckStateType* State,
    const uint8* Data,
    uint32 Length
)
{
    uint64 receivedCrc;
    uint64 calculatedCrc;
    uint16 receivedCounter;
    uint16 delta;

    if ((Config == NULL_PTR) || (State == NULL_PTR) || (Data == NULL_PTR)) {
        return E2E_E_INPUTERR_NULL;
    }

    /* Validate length */
    if ((Length < Config->MinDataLength) || (Length > Config->MaxDataLength)) {
        State->Status = E2E_P_WRONGCRC;
        return E_OK;
    }

    /* Read received CRC */
    receivedCrc = E2E_P06_ReadCRC(Data, Config->CRCOffset);

    /* Create temporary buffer with cleared CRC */
    uint8 tempData[256];
    if (Length > 256U) {
        return E2E_E_INPUTERR_WRONG;
    }
    (void)memcpy(tempData, Data, Length);
    E2E_P06_WriteCRC(tempData, Config->CRCOffset, 0ULL);

    /* Calculate expected CRC */
    calculatedCrc = E2E_P06_CalculateCRC64(tempData, Length, Config->DataID, Config->IncludeDataID);

    /* Verify CRC */
    if (receivedCrc != calculatedCrc) {
        State->Status = E2E_P_WRONGCRC;
        return E_OK;
    }

    /* Read received counter */
    receivedCounter = E2E_P06_ReadCounter(Data, Config->CounterOffset);

    if (State->WaitForFirstData == 0U) {
        State->LastValidCounter = receivedCounter;
        State->WaitForFirstData = 1U;
        State->Status = E2E_P_INITIAL;
    } else {
        if (receivedCounter >= State->LastValidCounter) {
            delta = receivedCounter - State->LastValidCounter;
        } else {
            delta = (E2E_P06_COUNTER_MAX - State->LastValidCounter) + receivedCounter + 1U;
        }

        if (delta == 0U) {
            State->Status = E2E_P_REPEATED;
        } else if (delta == 1U) {
            State->Status = E2E_P_OK;
            State->LastValidCounter = receivedCounter;
        } else if (delta <= State->MaxDeltaCounterInit) {
            State->Status = E2E_P_OKSOMELOST;
            State->LostData = (uint8)(delta - 1U);
            State->LastValidCounter = receivedCounter;
        } else {
            State->Status = E2E_P_SYNC;
            State->LastValidCounter = receivedCounter;
        }
    }

    return E_OK;
}

/**
 * @brief Map Profile 6 check status to State Machine
 */
void E2E_P06MapStatusToSM(
    E2E_PCheckStatusType CheckStatus,
    E2E_SMStateType* SMState,
    boolean* Error
)
{
    *Error = FALSE;

    switch (CheckStatus) {
        case E2E_P_OK:
        case E2E_P_OKSOMELOST:
            *SMState = E2E_SM_VALID;
            break;
        case E2E_P_WRONGCRC:
        case E2E_P_WRONGSEQUENCE:
        case E2E_P_REPEATED:
            *SMState = E2E_SM_INVALID;
            *Error = TRUE;
            break;
        case E2E_P_SYNC:
            *SMState = E2E_SM_INIT;
            break;
        case E2E_P_INITIAL:
            *SMState = E2E_SM_NODATA;
            break;
        default:
            *SMState = E2E_SM_INVALID;
            break;
    }
}
