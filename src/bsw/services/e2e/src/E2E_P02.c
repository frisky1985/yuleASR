/*******************************************************************************
 * @file    E2E_P02.c
 * @brief   E2E Profile 2 Implementation - Dual Path
 * @details Implements dual path redundancy for maximum reliability
 * @author  AutoSAR Team
 * @version 1.0.0
 ******************************************************************************/

#include "E2E_P02.h"
#include <string.h>

/*=============================================================================*
 * Local CRC8 Lookup Table (0x2F polynomial for Profile 2)
 *=============================================================================*/
static const uint8 E2E_P02_CRC8_Table[256] = {
    0x00, 0x2F, 0x5E, 0x71, 0xBC, 0x93, 0xE2, 0xCD,
    0x57, 0x78, 0x09, 0x26, 0xEB, 0xC4, 0xB5, 0x9A,
    0xAE, 0x81, 0xF0, 0xDF, 0x12, 0x3D, 0x4C, 0x63,
    0xF9, 0xD6, 0xA7, 0x88, 0x45, 0x6A, 0x1B, 0x34,
    0x73, 0x5C, 0x2D, 0x02, 0xCF, 0xE0, 0x91, 0xBE,
    0x24, 0x0B, 0x7A, 0x55, 0x98, 0xB7, 0xC6, 0xE9,
    0xDD, 0xF2, 0x83, 0xAC, 0x61, 0x4E, 0x3F, 0x10,
    0x8A, 0xA5, 0xD4, 0xFB, 0x36, 0x19, 0x68, 0x47,
    0xE6, 0xC9, 0xB8, 0x97, 0x5A, 0x75, 0x04, 0x2B,
    0xB1, 0x9E, 0xEF, 0xC0, 0x0D, 0x22, 0x53, 0x7C,
    0x48, 0x67, 0x16, 0x39, 0xF4, 0xDB, 0xAA, 0x85,
    0x1F, 0x30, 0x41, 0x6E, 0xA3, 0x8C, 0xFD, 0xD2,
    0x95, 0xBA, 0xCB, 0xE4, 0x29, 0x06, 0x77, 0x58,
    0xC2, 0xED, 0x9C, 0xB3, 0x7E, 0x51, 0x20, 0x0F,
    0x3B, 0x14, 0x65, 0x4A, 0x87, 0xA8, 0xD9, 0xF6,
    0x6C, 0x43, 0x32, 0x1D, 0xD0, 0xFF, 0x8E, 0xA1,
    0xE3, 0xCC, 0xBD, 0x92, 0x5F, 0x70, 0x01, 0x2E,
    0xB4, 0x9B, 0xEA, 0xC5, 0x08, 0x27, 0x56, 0x79,
    0x4D, 0x62, 0x13, 0x3C, 0xF1, 0xDE, 0xAF, 0x80,
    0x1A, 0x35, 0x44, 0x6B, 0xA6, 0x89, 0xF8, 0xD7,
    0x90, 0xBF, 0xCE, 0xE1, 0x2C, 0x03, 0x72, 0x5D,
    0xC7, 0xE8, 0x99, 0xB6, 0x7B, 0x54, 0x25, 0x0A,
    0x3E, 0x11, 0x60, 0x4F, 0x82, 0xAD, 0xDC, 0xF3,
    0x69, 0x46, 0x37, 0x18, 0xD5, 0xFA, 0x8B, 0xA4,
    0x05, 0x2A, 0x5B, 0x74, 0xB9, 0x96, 0xE7, 0xC8,
    0x52, 0x7D, 0x0C, 0x23, 0xEE, 0xC1, 0xB0, 0x9F,
    0xAB, 0x84, 0xD5, 0xFA, 0x37, 0x18, 0x69, 0x46,
    0xDC, 0xF3, 0x82, 0xAD, 0x60, 0x4F, 0x3E, 0x11,
    0x76, 0x59, 0x28, 0x07, 0xCA, 0xE5, 0x94, 0xBB,
    0x21, 0x0E, 0x7F, 0x50, 0x9D, 0xB2, 0xC3, 0xEC,
    0xD8, 0xF7, 0x86, 0xA9, 0x64, 0x4B, 0x3A, 0x15,
    0x8F, 0xA0, 0xD1, 0xFE, 0x33, 0x1C, 0x6D, 0x42
};

/*=============================================================================*
 * Local Function Prototypes
 *=============================================================================*/
static uint8 E2E_P02_CalculateCRC(
    const uint8* Data,
    uint16 Length,
    uint16 DataID
);

/*=============================================================================*
 * Function Implementations
 *=============================================================================*/

/**
 * @brief Calculate CRC8 for Profile 2
 */
static uint8 E2E_P02_CalculateCRC(
    const uint8* Data,
    uint16 Length,
    uint16 DataID
)
{
    uint8 crc = 0xFFU;
    uint16 i;
    
    /* CRC over data */
    for (i = 0U; i < Length; i++) {
        crc = E2E_P02_CRC8_Table[crc ^ Data[i]];
    }
    
    /* Include DataID */
    crc = E2E_P02_CRC8_Table[crc ^ (uint8)(DataID & 0xFFU)];
    crc = E2E_P02_CRC8_Table[crc ^ (uint8)((DataID >> 8) & 0xFFU)];
    
    return crc ^ 0xFFU;
}

/**
 * @brief E2E Profile 2 Protect
 * @details Adds protection with dual path support
 */
Std_ReturnType E2E_P02Protect(
    const E2E_P02ConfigType* Config,
    E2E_P02ProtectStateType* State,
    uint8* Data
)
{
    uint8 crc;
    uint8 pathNibble;
    
    #if (E2E_DEV_ERROR_DETECT == STD_ON)
    if ((Config == NULL) || (State == NULL) || (Data == NULL)) {
        return E_NOT_OK;
    }
    #endif
    
    /* Add path ID to nibble if dual path enabled */
    if (Config->DualPathEnabled) {
        pathNibble = (State->PathId & 0x01U) << 4;
        Data[Config->DataIDNibbleOffset] = (Data[Config->DataIDNibbleOffset] & 0x0FU) | pathNibble;
    }
    
    /* Write counter */
    Data[Config->CounterOffset] = State->Counter;
    
    /* Calculate CRC */
    crc = E2E_P02_CalculateCRC(Data, Config->DataLength, Config->DataID);
    
    /* Write CRC */
    Data[Config->CRCOffset] = crc;
    
    /* Increment counter */
    State->Counter++;
    
    /* Toggle path for next transmission */
    if (Config->DualPathEnabled) {
        State->PathId = 1U - State->PathId;
    }
    
    return E_OK;
}

/**
 * @brief E2E Profile 2 Check
 * @details Verifies data from one path
 */
Std_ReturnType E2E_P02Check(
    const E2E_P02ConfigType* Config,
    E2E_P02CheckStateType* State,
    const uint8* Data,
    uint8 PathId
)
{
    uint8 receivedCrc;
    uint8 calculatedCrc;
    uint8 receivedCounter;
    sint16 delta;
    
    #if (E2E_DEV_ERROR_DETECT == STD_ON)
    if ((Config == NULL) || (State == NULL) || (Data == NULL)) {
        return E_NOT_OK;
    }
    #endif
    
    /* Verify PathId */
    if (PathId > 1U) {
        return E_NOT_OK;
    }
    
    /* Get received CRC */
    receivedCrc = Data[Config->CRCOffset];
    
    /* Calculate CRC */
    calculatedCrc = E2E_P02_CalculateCRC(Data, Config->DataLength, Config->DataID);
    
    /* Check CRC */
    if (receivedCrc != calculatedCrc) {
        State->Status = E2E_P_WRONGCRC;
        State->PathStatus[PathId] = 1U; /* Error */
        return E_OK;
    }
    
    receivedCounter = Data[Config->CounterOffset];
    
    if (State->WaitForFirstData == 0U) {
        State->LastValidCounter = receivedCounter;
        State->WaitForFirstData = 1U;
        State->Status = E2E_P_INITIAL;
        State->PathStatus[PathId] = 0U;
    } else {
        delta = (sint16)((sint16)receivedCounter - (sint16)State->LastValidCounter);
        
        if (delta == 0) {
            State->Status = E2E_P_REPEATED;
        } else if (delta < 0) {
            State->Status = E2E_P_WRONGSEQUENCE;
        } else if (delta == 1) {
            State->Status = E2E_P_OK;
            State->LastValidCounter = receivedCounter;
            State->PathStatus[PathId] = 0U;
        } else if (delta <= (sint16)State->MaxDeltaCounterInit) {
            State->Status = E2E_P_OKSOMELOST;
            State->LastValidCounter = receivedCounter;
        } else {
            State->Status = E2E_P_SYNC;
            State->LastValidCounter = receivedCounter;
        }
    }
    
    return E_OK;
}

/**
 * @brief Map Profile 2 check status to State Machine
 */
void E2E_P02MapStatusToSM(
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
