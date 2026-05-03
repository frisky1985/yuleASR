/**
 * @file E2E_P05.c
 * @brief E2E Profile 5 Implementation - CRC64 with 32-bit Counter
 * @copyright Copyright (c) 2025 yuleASR Project
 * @license MIT License
 * 
 * Profile 5 implements end-to-end protection using:
 * - CRC-64 (CRC-64-ECMA polynomial 0x42F0E1EBA9EA3693)
 * - 32-bit sequence counter
 * - Optional DataID inclusion
 * 
 * Suitable for ASIL-D safety requirements with highest data integrity.
 */

#include "E2E_P05.h"
#include "Crc.h"
#include <string.h>

/*=============================================================================*
 * Local CRC64 Implementation using Crc library
 *=============================================================================*/

/**
 * @brief Calculate CRC64 over data buffer
 */
static uint64 E2E_P05_CalculateCRC64(
    const uint8* Data,
    uint32 Length,
    uint64 DataID,
    boolean IncludeDataID
)
{
    uint64 crc;
    
    /* Calculate CRC over data */
    crc = Crc_CalculateCRC64(Data, Length, 0ULL, TRUE);
    
    /* Include DataID in CRC if configured */
    if (IncludeDataID) {
        uint8 dataIdBytes[8];
        dataIdBytes[0] = (uint8)(DataID & 0xFFU);
        dataIdBytes[1] = (uint8)((DataID >> 8) & 0xFFU);
        dataIdBytes[2] = (uint8)((DataID >> 16) & 0xFFU);
        dataIdBytes[3] = (uint8)((DataID >> 24) & 0xFFU);
        dataIdBytes[4] = 0U;
        dataIdBytes[5] = 0U;
        dataIdBytes[6] = 0U;
        dataIdBytes[7] = 0U;
        crc = Crc_CalculateCRC64(dataIdBytes, 4U, crc, FALSE);
    }
    
    return crc;
}

/**
 * @brief Write 32-bit counter to data buffer
 */
static void E2E_P05_WriteCounter(uint8* Data, uint32 Offset, uint32 Counter)
{
    uint32 byteOffset = Offset / 8U;
    uint32 bitOffset = Offset % 8U;
    
    if (bitOffset == 0U) {
        /* Aligned write */
        Data[byteOffset] = (uint8)(Counter & 0xFFU);
        Data[byteOffset + 1U] = (uint8)((Counter >> 8) & 0xFFU);
        Data[byteOffset + 2U] = (uint8)((Counter >> 16) & 0xFFU);
        Data[byteOffset + 3U] = (uint8)((Counter >> 24) & 0xFFU);
    } else {
        /* Unaligned write - bit manipulation */
        uint32 i;
        for (i = 0U; i < 32U; i++) {
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
 * @brief Read 32-bit counter from data buffer
 */
static uint32 E2E_P05_ReadCounter(const uint8* Data, uint32 Offset)
{
    uint32 byteOffset = Offset / 8U;
    uint32 bitOffset = Offset % 8U;
    uint32 counter = 0U;
    
    if (bitOffset == 0U) {
        /* Aligned read */
        counter = (uint32)Data[byteOffset] |
                  ((uint32)Data[byteOffset + 1U] << 8) |
                  ((uint32)Data[byteOffset + 2U] << 16) |
                  ((uint32)Data[byteOffset + 3U] << 24);
    } else {
        /* Unaligned read - bit manipulation */
        uint32 i;
        for (i = 0U; i < 32U; i++) {
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
 * @brief Write 64-bit CRC to data buffer
 */
static void E2E_P05_WriteCRC(uint8* Data, uint32 Offset, uint64 CRC)
{
    uint32 byteOffset = Offset / 8U;
    uint32 bitOffset = Offset % 8U;
    
    if (bitOffset == 0U) {
        /* Aligned write - big endian */
        Data[byteOffset] = (uint8)((CRC >> 56) & 0xFFU);
        Data[byteOffset + 1U] = (uint8)((CRC >> 48) & 0xFFU);
        Data[byteOffset + 2U] = (uint8)((CRC >> 40) & 0xFFU);
        Data[byteOffset + 3U] = (uint8)((CRC >> 32) & 0xFFU);
        Data[byteOffset + 4U] = (uint8)((CRC >> 24) & 0xFFU);
        Data[byteOffset + 5U] = (uint8)((CRC >> 16) & 0xFFU);
        Data[byteOffset + 6U] = (uint8)((CRC >> 8) & 0xFFU);
        Data[byteOffset + 7U] = (uint8)(CRC & 0xFFU);
    } else {
        /* Unaligned write */
        uint32 i;
        for (i = 0U; i < 64U; i++) {
            uint32 bitPos = Offset + i;
            uint32 byteIdx = bitPos / 8U;
            uint32 bitIdx = bitPos % 8U;
            uint8 bitValue = (uint8)((CRC >> (63U - i)) & 0x01U);
            
            if (bitValue) {
                Data[byteIdx] |= (uint8)(1U << bitIdx);
            } else {
                Data[byteIdx] &= (uint8)~(1U << bitIdx);
            }
        }
    }
}

/**
 * @brief Read 64-bit CRC from data buffer
 */
static uint64 E2E_P05_ReadCRC(const uint8* Data, uint32 Offset)
{
    uint32 byteOffset = Offset / 8U;
    uint32 bitOffset = Offset % 8U;
    uint64 crc = 0ULL;
    
    if (bitOffset == 0U) {
        /* Aligned read - big endian */
        crc = ((uint64)Data[byteOffset] << 56) |
              ((uint64)Data[byteOffset + 1U] << 48) |
              ((uint64)Data[byteOffset + 2U] << 40) |
              ((uint64)Data[byteOffset + 3U] << 32) |
              ((uint64)Data[byteOffset + 4U] << 24) |
              ((uint64)Data[byteOffset + 5U] << 16) |
              ((uint64)Data[byteOffset + 6U] << 8) |
              (uint64)Data[byteOffset + 7U];
    } else {
        /* Unaligned read */
        uint32 i;
        for (i = 0U; i < 64U; i++) {
            uint32 bitPos = Offset + i;
            uint32 byteIdx = bitPos / 8U;
            uint32 bitIdx = bitPos % 8U;
            
            if (Data[byteIdx] & (1U << bitIdx)) {
                crc |= (1ULL << (63U - i));
            }
        }
    }
    
    return crc;
}

/*=============================================================================*
 * Profile 5 Protect
 *=============================================================================*/

/**
 * @brief E2E Profile 5 Protect
 * @details Adds CRC64 protection with 32-bit counter to data
 */
Std_ReturnType E2E_P05Protect(
    const E2E_P05ConfigType* Config,
    E2E_P05ProtectStateType* State,
    uint8* Data
)
{
    uint64 crc;
    
    if ((Config == NULL_PTR) || (State == NULL_PTR) || (Data == NULL_PTR)) {
        return E2E_E_INPUTERR_NULL;
    }
    
    /* Clear CRC field before calculation */
    E2E_P05_WriteCRC(Data, Config->CRCOffset, 0ULL);
    
    /* Write 32-bit counter */
    E2E_P05_WriteCounter(Data, Config->CounterOffset, State->Counter);
    
    /* Calculate CRC64 over data and DataID */
    crc = E2E_P05_CalculateCRC64(Data, Config->DataLength, Config->DataID, Config->IncludeDataID);
    
    /* Write CRC to data */
    E2E_P05_WriteCRC(Data, Config->CRCOffset, crc);
    
    /* Increment counter */
    State->Counter++;
    if (State->Counter > E2E_P05_COUNTER_MAX) {
        State->Counter = 0U;
    }
    
    return E_OK;
}

/*=============================================================================*
 * Profile 5 Check
 *=============================================================================*/

/**
 * @brief E2E Profile 5 Check
 * @details Verifies CRC64 protected data with 32-bit counter
 */
Std_ReturnType E2E_P05Check(
    const E2E_P05ConfigType* Config,
    E2E_P05CheckStateType* State,
    const uint8* Data
)
{
    uint64 receivedCrc;
    uint64 calculatedCrc;
    uint32 receivedCounter;
    uint32 delta;
    uint8 tempData[256];  /* Temporary buffer for CRC calculation */
    
    if ((Config == NULL_PTR) || (State == NULL_PTR) || (Data == NULL_PTR)) {
        return E2E_E_INPUTERR_NULL;
    }
    
    /* Copy data to temporary buffer to clear CRC field */
    memcpy(tempData, Data, Config->DataLength);
    E2E_P05_WriteCRC(tempData, Config->CRCOffset, 0ULL);
    
    /* Read received CRC */
    receivedCrc = E2E_P05_ReadCRC(Data, Config->CRCOffset);
    
    /* Calculate expected CRC */
    calculatedCrc = E2E_P05_CalculateCRC64(tempData, Config->DataLength, Config->DataID, Config->IncludeDataID);
    
    /* Verify CRC */
    if (receivedCrc != calculatedCrc) {
        State->Status = E2E_P_WRONGCRC;
        return E_OK;
    }
    
    /* Read received counter */
    receivedCounter = E2E_P05_ReadCounter(Data, Config->CounterOffset);
    
    if (State->WaitForFirstData == 0U) {
        /* First received data */
        State->LastValidCounter = receivedCounter;
        State->WaitForFirstData = 1U;
        State->Status = E2E_P_INITIAL;
    } else {
        /* Calculate delta (handle wrap-around) */
        if (receivedCounter >= State->LastValidCounter) {
            delta = receivedCounter - State->LastValidCounter;
        } else {
            delta = (E2E_P05_COUNTER_MAX - State->LastValidCounter) + receivedCounter + 1U;
        }
        
        if (delta == 0U) {
            /* Same counter - repeated message */
            State->Status = E2E_P_REPEATED;
        } else if (delta == 1U) {
            /* Consecutive - OK */
            State->Status = E2E_P_OK;
            State->LastValidCounter = receivedCounter;
        } else if (delta <= State->MaxDeltaCounterInit) {
            /* Some messages lost but within tolerance */
            State->Status = E2E_P_OKSOMELOST;
            State->LostData = (uint8)(delta - 1U);
            State->LastValidCounter = receivedCounter;
        } else {
            /* Too many messages lost */
            State->Status = E2E_P_SYNC;
            State->LastValidCounter = receivedCounter;
        }
    }
    
    return E_OK;
}

/**
 * @brief Map Profile 5 check status to State Machine
 */
void E2E_P05MapStatusToSM(
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
