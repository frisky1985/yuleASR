/**
 * @file E2E_P07.c
 * @brief E2E Profile 7 Implementation - CRC32 with Dynamic Length
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 *
 * Profile 7 implements end-to-end protection with:
 * - CRC32 (IEEE 802.3 polynomial 0x04C11DB7)
 * - 8-bit sequence counter
 * - Dynamic data length support
 * - Optional DataID inclusion
 *
 * Optimized for systems where 64-bit CRC is not needed but
 * dynamic length support is required.
 */

#include "E2E_P07.h"
#include "Crc.h"
#include <string.h>

/*=============================================================================*
 * Local Helper Functions
 *=============================================================================*/

/**
 * @brief Calculate CRC32 including DataID
 */
static uint32 E2E_P07_CalculateCRC32(
    const uint8* Data,
    uint32 Length,
    uint32 DataID,
    boolean IncludeDataID
)
{
    uint32 crc;

    /* Calculate CRC over data */
    crc = Crc_CalculateCRC32(Data, Length, 0U, TRUE);

    /* Include DataID in CRC if configured */
    if (IncludeDataID) {
        uint8 dataIdBytes[4];
        dataIdBytes[0] = (uint8)(DataID & 0xFFU);
        dataIdBytes[1] = (uint8)((DataID >> 8) & 0xFFU);
        dataIdBytes[2] = (uint8)((DataID >> 16) & 0xFFU);
        dataIdBytes[3] = (uint8)((DataID >> 24) & 0xFFU);
        crc = Crc_CalculateCRC32(dataIdBytes, 4U, crc, FALSE);
    }

    return crc;
}

/**
 * @brief Write 8-bit counter to data buffer
 */
static void E2E_P07_WriteCounter(uint8* Data, uint32 Offset, uint8 Counter)
{
    uint32 byteOffset = Offset / 8U;
    uint32 bitOffset = Offset % 8U;

    if (bitOffset == 0U) {
        Data[byteOffset] = Counter;
    } else {
        uint32 i;
        for (i = 0U; i < 8U; i++) {
            uint32 bitPos = Offset + i;
            uint32 byteIdx = bitPos / 8U;
            uint32 bitIdx = bitPos % 8U;
            uint8 bitValue = (uint8)((Counter >> i) & 0x01U);

            if (bitValue) {
                Data[byteIdx] |= (uint8)(1U << bitIdx);
            } else {
                Data[byteIdx] &= (uint8)~(1U << bitIdx);
            }
        }
    }
}

/**
 * @brief Read 8-bit counter from data buffer
 */
static uint8 E2E_P07_ReadCounter(const uint8* Data, uint32 Offset)
{
    uint32 byteOffset = Offset / 8U;
    uint32 bitOffset = Offset % 8U;
    uint8 counter = 0U;

    if (bitOffset == 0U) {
        counter = Data[byteOffset];
    } else {
        uint32 i;
        for (i = 0U; i < 8U; i++) {
            uint32 bitPos = Offset + i;
            uint32 byteIdx = bitPos / 8U;
            uint32 bitIdx = bitPos % 8U;

            if (Data[byteIdx] & (1U << bitIdx)) {
                counter |= (1U << i);
            }
        }
    }

    return counter;
}

/**
 * @brief Write 32-bit CRC to data buffer (big endian)
 */
static void E2E_P07_WriteCRC(uint8* Data, uint32 Offset, uint32 CRC)
{
    uint32 byteOffset = Offset / 8U;

    Data[byteOffset] = (uint8)((CRC >> 24) & 0xFFU);
    Data[byteOffset + 1U] = (uint8)((CRC >> 16) & 0xFFU);
    Data[byteOffset + 2U] = (uint8)((CRC >> 8) & 0xFFU);
    Data[byteOffset + 3U] = (uint8)(CRC & 0xFFU);
}

/**
 * @brief Read 32-bit CRC from data buffer (big endian)
 */
static uint32 E2E_P07_ReadCRC(const uint8* Data, uint32 Offset)
{
    uint32 byteOffset = Offset / 8U;

    return ((uint32)Data[byteOffset] << 24) |
           ((uint32)Data[byteOffset + 1U] << 16) |
           ((uint32)Data[byteOffset + 2U] << 8) |
           (uint32)Data[byteOffset + 3U];
}

/*=============================================================================*
 * Profile 7 Protect
 *=============================================================================*/

Std_ReturnType E2E_P07Protect(
    const E2E_P07ConfigType* Config,
    E2E_P07ProtectStateType* State,
    uint8* Data,
    uint32 Length
)
{
    uint32 crc;

    if ((Config == NULL_PTR) || (State == NULL_PTR) || (Data == NULL_PTR)) {
        return E2E_E_INPUTERR_NULL;
    }

    /* Validate length */
    if ((Length < Config->MinDataLength) || (Length > Config->MaxDataLength)) {
        return E2E_E_INPUTERR_WRONG;
    }

    /* Clear CRC field before calculation */
    E2E_P07_WriteCRC(Data, Config->CRCOffset, 0U);

    /* Write 8-bit counter */
    E2E_P07_WriteCounter(Data, Config->CounterOffset, State->Counter);

    /* Calculate CRC32 including DataID */
    crc = E2E_P07_CalculateCRC32(Data, Length, Config->DataID, Config->IncludeDataID);

    /* Write CRC to data */
    E2E_P07_WriteCRC(Data, Config->CRCOffset, crc);

    /* Increment counter */
    State->Counter++;
    if (State->Counter > E2E_P07_COUNTER_MAX) {
        State->Counter = 0U;
    }

    return E_OK;
}

/*=============================================================================*
 * Profile 7 Check
 *=============================================================================*/

Std_ReturnType E2E_P07Check(
    const E2E_P07ConfigType* Config,
    E2E_P07CheckStateType* State,
    const uint8* Data,
    uint32 Length
)
{
    uint32 receivedCrc;
    uint32 calculatedCrc;
    uint8 receivedCounter;
    uint8 delta;

    if ((Config == NULL_PTR) || (State == NULL_PTR) || (Data == NULL_PTR)) {
        return E2E_E_INPUTERR_NULL;
    }

    /* Validate length */
    if ((Length < Config->MinDataLength) || (Length > Config->MaxDataLength)) {
        State->Status = E2E_P_WRONGCRC;
        return E_OK;
    }

    /* Read received CRC */
    receivedCrc = E2E_P07_ReadCRC(Data, Config->CRCOffset);

    /* Create temporary buffer with cleared CRC */
    uint8 tempData[256];
    if (Length > 256U) {
        return E2E_E_INPUTERR_WRONG;
    }
    (void)memcpy(tempData, Data, Length);
    E2E_P07_WriteCRC(tempData, Config->CRCOffset, 0U);

    /* Calculate expected CRC */
    calculatedCrc = E2E_P07_CalculateCRC32(tempData, Length, Config->DataID, Config->IncludeDataID);

    /* Verify CRC */
    if (receivedCrc != calculatedCrc) {
        State->Status = E2E_P_WRONGCRC;
        return E_OK;
    }

    /* Read received counter */
    receivedCounter = E2E_P07_ReadCounter(Data, Config->CounterOffset);

    if (State->WaitForFirstData == 0U) {
        State->LastValidCounter = receivedCounter;
        State->WaitForFirstData = 1U;
        State->Status = E2E_P_INITIAL;
    } else {
        if (receivedCounter >= State->LastValidCounter) {
            delta = receivedCounter - State->LastValidCounter;
        } else {
            delta = (E2E_P07_COUNTER_MAX - State->LastValidCounter) + receivedCounter + 1U;
        }

        if (delta == 0U) {
            State->Status = E2E_P_REPEATED;
        } else if (delta == 1U) {
            State->Status = E2E_P_OK;
            State->LastValidCounter = receivedCounter;
        } else if (delta <= State->MaxDeltaCounterInit) {
            State->Status = E2E_P_OKSOMELOST;
            State->LostData = delta - 1U;
            State->LastValidCounter = receivedCounter;
        } else {
            State->Status = E2E_P_SYNC;
            State->LastValidCounter = receivedCounter;
        }
    }

    return E_OK;
}

/**
 * @brief Map Profile 7 check status to State Machine
 */
void E2E_P07MapStatusToSM(
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
