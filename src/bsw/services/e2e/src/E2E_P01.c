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
 * @file    E2E_P01.c
 * @brief   E2E Profile 1 Implementation - CRC8 + Counter + DataID
 * @details Implements E2E protection according to AutoSAR Profile 1 specification
 * @author  AutoSAR Team
 * @version 1.0.0
 ******************************************************************************/

#include "E2E_P01.h"
#include <string.h>

/*=============================================================================*
 * Local CRC8 Lookup Table (SAE J1850 polynomial 0x1D)
 *=============================================================================*/
static const uint8 E2E_P01_CRC8_Table[256] = {
    0x00, 0x1D, 0x3A, 0x27, 0x74, 0x69, 0x4E, 0x53,
    0xE8, 0xF5, 0xD2, 0xCF, 0x9C, 0x81, 0xA6, 0xBB,
    0xCD, 0xD0, 0xF7, 0xEA, 0xB9, 0xA4, 0x83, 0x9E,
    0x25, 0x38, 0x1F, 0x02, 0x51, 0x4C, 0x6B, 0x76,
    0x87, 0x9A, 0xBD, 0xA0, 0xF3, 0xEE, 0xC9, 0xD4,
    0x6F, 0x72, 0x55, 0x48, 0x1B, 0x06, 0x21, 0x3C,
    0x4A, 0x57, 0x70, 0x6D, 0x3E, 0x23, 0x04, 0x19,
    0xA2, 0xBF, 0x98, 0x85, 0xD6, 0xCB, 0xEC, 0xF1,
    0x13, 0x0E, 0x29, 0x34, 0x67, 0x7A, 0x5D, 0x40,
    0xFB, 0xE6, 0xC1, 0xDC, 0x8F, 0x92, 0xB5, 0xA8,
    0xDE, 0xC3, 0xE4, 0xF9, 0xAA, 0xB7, 0x90, 0x8D,
    0x36, 0x2B, 0x0C, 0x11, 0x42, 0x5F, 0x78, 0x65,
    0x94, 0x89, 0xAE, 0xB3, 0xE0, 0xFD, 0xDA, 0xC7,
    0x7C, 0x61, 0x46, 0x5B, 0x08, 0x15, 0x32, 0x2F,
    0x59, 0x44, 0x63, 0x7E, 0x2D, 0x30, 0x17, 0x0A,
    0xB1, 0xAC, 0x8B, 0x96, 0xC5, 0xD8, 0xFF, 0xE2,
    0x26, 0x3B, 0x1C, 0x01, 0x52, 0x4F, 0x68, 0x75,
    0xCE, 0xD3, 0xF4, 0xE9, 0xBA, 0xA7, 0x80, 0x9D,
    0xEB, 0xF6, 0xD1, 0xCC, 0x9F, 0x82, 0xA5, 0xB8,
    0x03, 0x1E, 0x39, 0x24, 0x77, 0x6A, 0x4D, 0x50,
    0xA1, 0xBC, 0x9B, 0x86, 0xD5, 0xC8, 0xEF, 0xF2,
    0x49, 0x54, 0x73, 0x6E, 0x3D, 0x20, 0x07, 0x1A,
    0x6C, 0x71, 0x56, 0x4B, 0x18, 0x05, 0x22, 0x3F,
    0x84, 0x99, 0xBE, 0xA3, 0xF0, 0xED, 0xCA, 0xD7,
    0x35, 0x28, 0x0F, 0x12, 0x41, 0x5C, 0x7B, 0x66,
    0xDD, 0xC0, 0xE7, 0xFA, 0xA9, 0xB4, 0x93, 0x8E,
    0xF8, 0xE5, 0xC2, 0xDF, 0x8C, 0x91, 0xB6, 0xAB,
    0x10, 0x0D, 0x2A, 0x37, 0x64, 0x79, 0x5E, 0x43,
    0xB2, 0xAF, 0x88, 0x95, 0xC6, 0xDB, 0xFC, 0xE1,
    0x5A, 0x47, 0x60, 0x7D, 0x2E, 0x33, 0x14, 0x09,
    0x7F, 0x62, 0x45, 0x58, 0x0B, 0x16, 0x31, 0x2C,
    0x97, 0x8A, 0xAD, 0xB0, 0xE3, 0xFE, 0xD9, 0xC4
};

/*=============================================================================*
 * Local Function Prototypes
 *=============================================================================*/
static uint8 E2E_P01_CalculateCRC(
    const uint8* Data,
    uint16 Length,
    uint16 DataID,
    uint8 DataIDMode,
    uint8 DataIDNibbleOffset,
    uint8 Counter,
    uint16 CrcOffset
);

/*=============================================================================*
 * Function Implementations
 *=============================================================================*/

/**
 * @brief Calculate CRC8 over data with DataID
 */
static uint8 E2E_P01_CalculateCRC(
    const uint8* Data,
    uint16 Length,
    uint16 DataID,
    uint8 DataIDMode,
    uint8 DataIDNibbleOffset,
    uint8 Counter,
    uint16 CrcOffset
)
{
    uint8 crc = 0xFFU; /* Initial value */
    uint16 i;
    uint8 dataIdLow = (uint8)(DataID & 0xFFU);
    uint8 dataIdHigh = (uint8)((DataID >> 8) & 0xFFU);
    uint8 dataIdNibble;
    
    /* Calculate CRC over data (excluding CRC byte) */
    for (i = 0U; i < Length; i++) {
        if (i == CrcOffset) {
            continue;   /* 排除 CRC 字节自身 (E2E Profile 1: CRC 计算范围不含 CRC 字段) */
        }
        crc = E2E_P01_CRC8_Table[crc ^ Data[i]];
    }
    
    /* XOR DataID based on mode */
    switch (DataIDMode) {
        case E2E_P01_DATAID_BOTH:
            crc = E2E_P01_CRC8_Table[crc ^ dataIdLow];
            crc = E2E_P01_CRC8_Table[crc ^ dataIdHigh];
            break;
        case E2E_P01_DATAID_LOW:
            crc = E2E_P01_CRC8_Table[crc ^ dataIdLow];
            break;
        case E2E_P01_DATAID_ALT:
            /* Alternate: counter 偶数用低字节, 奇数用高字节 (与 e2e_protection.c 语义一致) */
            if ((Counter & 0x01U) == 0U) {
                crc = E2E_P01_CRC8_Table[crc ^ dataIdLow];
            } else {
                crc = E2E_P01_CRC8_Table[crc ^ dataIdHigh];
            }
            break;
        case E2E_P01_DATAID_NIBBLE:
            /* Nibble: 仅使用 DataIDNibbleOffset 指定的 nibble
             * 0 = 低字节低4位, 1 = 低字节高4位, 2 = 高字节低4位, 3 = 高字节高4位 */
            switch (DataIDNibbleOffset & 0x03U) {
                case 0U:
                    dataIdNibble = (uint8)(dataIdLow & 0x0FU);
                    break;
                case 1U:
                    dataIdNibble = (uint8)((dataIdLow >> 4) & 0x0FU);
                    break;
                case 2U:
                    dataIdNibble = (uint8)(dataIdHigh & 0x0FU);
                    break;
                default:
                    dataIdNibble = (uint8)((dataIdHigh >> 4) & 0x0FU);
                    break;
            }
            crc = E2E_P01_CRC8_Table[crc ^ dataIdNibble];
            break;
        default:
            crc = E2E_P01_CRC8_Table[crc ^ dataIdLow];
            break;
    }
    
    return crc ^ 0xFFU; /* Final XOR */
}

/** @req SWS_E2E_00003 */
/**
 * @brief E2E Profile 1 Protect
 * @details Adds E2E protection to data (CRC8 + 4-bit counter)
 */
Std_ReturnType E2E_P01Protect(
    const E2E_P01ConfigType* Config,
    E2E_P01ProtectStateType* State,
    uint8* Data
)
{
    uint8 crc;
    uint8 counterByte;
    
    #if (E2E_DEV_ERROR_DETECT == STD_ON)
    if ((Config == NULL_PTR) || (State == NULL_PTR) || (Data == NULL_PTR)) {
        return E_NOT_OK;
    }
    #endif
    
    /* Write counter to data (4-bit counter in lower nibble) */
    counterByte = Data[Config->CounterOffset];
    counterByte = (counterByte & 0xF0U) | (State->Counter & 0x0FU);
    Data[Config->CounterOffset] = counterByte;
    
    /* Calculate CRC over data and DataID */
    crc = E2E_P01_CalculateCRC(
        Data,
        Config->DataLength,
        Config->DataID,
        Config->DataIDMode,
        Config->DataIDNibbleOffset,
        (uint8)(Data[Config->CounterOffset] & 0x0FU),
        Config->CRCOffset
    );
    
    /* Write CRC */
    Data[Config->CRCOffset] = crc;
    
    /* Increment counter (modulo 15) */
    State->Counter++;
    if (State->Counter > E2E_P01_COUNTER_MAX) {
        State->Counter = 0U;
    }
    
    return E_OK;
}

/** @req SWS_E2E_00004 */
/**
 * @brief E2E Profile 1 Check
 * @details Verifies E2E protection of received data
 */
Std_ReturnType E2E_P01Check(
    const E2E_P01ConfigType* Config,
    E2E_P01CheckStateType* State,
    const uint8* Data
)
{
    uint8 receivedCrc;
    uint8 calculatedCrc;
    uint8 receivedCounter;
    sint8 delta;
    
    #if (E2E_DEV_ERROR_DETECT == STD_ON)
    if ((Config == NULL_PTR) || (State == NULL_PTR) || (Data == NULL_PTR)) {
        return E_NOT_OK;
    }
    #endif
    
    /* Get received CRC */
    receivedCrc = Data[Config->CRCOffset];
    
    /* Calculate expected CRC */
    calculatedCrc = E2E_P01_CalculateCRC(
        Data,
        Config->DataLength,
        Config->DataID,
        Config->DataIDMode,
        Config->DataIDNibbleOffset,
        (uint8)(Data[Config->CounterOffset] & 0x0FU),
        Config->CRCOffset
    );
    
    /* Verify CRC */
    if (receivedCrc != calculatedCrc) {
        State->Status = E2E_P_WRONGCRC;
        return E_OK;
    }
    
    /* Get received counter (4-bit) */
    receivedCounter = Data[Config->CounterOffset] & 0x0FU;
    
    if (State->WaitForFirstData == 0U) {
        /* First received data */
        State->LastValidCounter = receivedCounter;
        State->WaitForFirstData = 1U;
        State->Status = E2E_P_INITIAL;
    } else {
        /* Calculate delta */
        delta = (sint8)((sint8)receivedCounter - (sint8)State->LastValidCounter);
        
        if (delta == 0 ) {
            /* Same counter - repeated message */
            State->Status = E2E_P_REPEATED;
        } else if (delta < 0) {
            /* Counter decreased - wrong sequence */
            State->Status = E2E_P_WRONGSEQUENCE;
        } else if (delta == 1) {
            /* Consecutive - OK */
            State->Status = E2E_P_OK;
            State->LastValidCounter = receivedCounter;
        } else if (delta <= (sint8)State->MaxDeltaCounterInit) {
            /* Some messages lost but within tolerance */
            State->Status = E2E_P_OKSOMELOST;
            State->LastValidCounter = receivedCounter;
        } else {
            /* Too many messages lost */
            State->Status = E2E_P_SYNC;
            State->LastValidCounter = receivedCounter;
        }
    }
    
    return E_OK;
}

/** @req SWS_E2E_00005 */
/**
 * @brief Map Profile 1 check status to State Machine state
 */
void E2E_P01MapStatusToSM(
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
        case E2E_P_NONEWDATA:
        default:
            *SMState = E2E_SM_INVALID;
            break;
    }
}
