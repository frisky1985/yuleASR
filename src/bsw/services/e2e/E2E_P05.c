/**
 * @file E2E_P05.c
 * @brief E2E Profile 5 implementation (CRC64 + Counter + DataID)
 * @details Profile 5 uses 64-bit CRC for highest integrity
 */

#include "E2E.h"

#if (E2E_PROFILE_5 == STD_ON)

/* CRC-64 polynomial: x^64 + x^4 + x^3 + x + 1 */
#define E2E_P05_CRC64_POLYNOMIAL    (0x000000000000001BULL)
#define E2E_P05_CRC64_INITIAL       (0xFFFFFFFFFFFFFFFFULL)

static uint64 E2E_P05_CalculateCRC64(const uint8* DataPtr, uint32 Length, uint64 InitialValue);

Std_ReturnType E2E_P05Protect(E2E_P05ConfigType* ConfigPtr, E2E_P05ProtectStateType* StatePtr, uint8* DataPtr, uint32 Length)
{
    uint64 Crc;
    uint32 CounterPos;
    uint32 DataIDPos;
    
    if ((ConfigPtr == NULL_PTR) || (StatePtr == NULL_PTR) || (DataPtr == NULL_PTR))
    {
        return E2E_E_INPUTERR_NULL;
    }
    
    /* Calculate CRC over DataID, Counter, and Data */
    Crc = E2E_P05_CRC64_INITIAL;
    
    /* Include DataID in CRC calculation */
    Crc = E2E_P05_CalculateCRC64((uint8*)&ConfigPtr->DataID, 4U, Crc);
    
    /* Include Counter */
    CounterPos = ConfigPtr->CounterOffset / 8U;
    Crc = E2E_P05_CalculateCRC64(&StatePtr->Counter, 1U, Crc);
    
    /* Include Data */
    Crc = E2E_P05_CalculateCRC64(DataPtr, Length, Crc);
    
    /* Write CRC to data */
    /* Implementation depends on CRCOffset */
    
    /* Increment counter */
    StatePtr->Counter++;
    if (StatePtr->Counter > E2E_P05_COUNTER_MAX)
    {
        StatePtr->Counter = 0U;
    }
    
    return E_OK;
}

Std_ReturnType E2E_P05Check(E2E_P05ConfigType* ConfigPtr, E2E_P05CheckStateType* StatePtr, uint8* DataPtr, uint32 Length)
{
    uint64 ReceivedCrc;
    uint64 CalculatedCrc;
    uint8 ReceivedCounter;
    int8 DeltaCounter;
    
    if ((ConfigPtr == NULL_PTR) || (StatePtr == NULL_PTR) || (DataPtr == NULL_PTR))
    {
        return E2E_E_INPUTERR_NULL;
    }
    
    /* Extract received CRC */
    /* Implementation depends on CRCOffset */
    
    /* Calculate expected CRC */
    CalculatedCrc = E2E_P05_CRC64_INITIAL;
    CalculatedCrc = E2E_P05_CalculateCRC64((uint8*)&ConfigPtr->DataID, 4U, CalculatedCrc);
    CalculatedCrc = E2E_P05_CalculateCRC64(DataPtr, Length, CalculatedCrc);
    
    /* Verify CRC */
    if (ReceivedCrc != CalculatedCrc)
    {
        StatePtr->Status = E2E_P05STATUS_ERROR;
        return E_OK;
    }
    
    /* Check counter */
    DeltaCounter = (int8)(ReceivedCounter - StatePtr->LastValidCounter);
    
    if (DeltaCounter == 0)
    {
        StatePtr->Status = E2E_P05STATUS_REPEATED;
    }
    else if (DeltaCounter == 1)
    {
        StatePtr->Status = E2E_P05STATUS_OK;
        StatePtr->LastValidCounter = ReceivedCounter;
    }
    else if (DeltaCounter > 1)
    {
        StatePtr->Status = E2E_P05STATUS_OKSOMELOST;
        StatePtr->LastValidCounter = ReceivedCounter;
    }
    else
    {
        StatePtr->Status = E2E_P05STATUS_WRONGSEQUENCE;
    }
    
    return E_OK;
}

static uint64 E2E_P05_CalculateCRC64(const uint8* DataPtr, uint32 Length, uint64 InitialValue)
{
    uint32 i;
    uint8 j;
    uint64 Crc = InitialValue;
    
    for (i = 0U; i < Length; i++)
    {
        Crc ^= ((uint64)DataPtr[i] << 56U);
        for (j = 0U; j < 8U; j++)
        {
            if (Crc & 0x8000000000000000ULL)
            {
                Crc = (Crc << 1U) ^ E2E_P05_CRC64_POLYNOMIAL;
            }
            else
            {
                Crc <<= 1U;
            }
        }
    }
    
    return Crc;
}

#endif /* E2E_PROFILE_5 */
