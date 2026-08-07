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

/*
 * Com_Private.h
 * AUTOSAR COM Module - Internal Definitions
 */

#ifndef COM_PRIVATE_H
#define COM_PRIVATE_H

/*==================[Includes]=============================================*/

#include "Com.h"
#include "Com_Cfg.h"
#include "Det.h"
#include "Com_DeadlineMon.h"
#include "Com_ErrorHandling.h"

/*==================[Macros]==============================================*/

/* Debug macro */
#if (COM_DEV_ERROR_DETECT == STD_ON)
#define COM_REPORT_ERROR(ApiId, ErrorId) \
    Det_ReportError(COM_MODULE_ID, COM_INSTANCE_ID, (ApiId), (ErrorId))
#define COM_VALIDATE(expr, ApiId, ErrorId, ret) \
    do { \
        if (!(expr)) { \
            COM_REPORT_ERROR((ApiId), (ErrorId)); \
            return (ret); \
        } \
    } while(0)
#define COM_VALIDATE_NO_RV(expr, ApiId, ErrorId) \
    do { \
        if (!(expr)) { \
            COM_REPORT_ERROR((ApiId), (ErrorId)); \
            return; \
        } \
    } while(0)
#else
#define COM_REPORT_ERROR(ApiId, ErrorId)
#define COM_VALIDATE(expr, ApiId, ErrorId, ret)
#define COM_VALIDATE_NO_RV(expr, ApiId, ErrorId)
#endif

/* API Service IDs for error reporting */
#define COM_SERVICE_ID_INIT                 0x01u
#define COM_SERVICE_ID_DEINIT               0x02u
#define COM_SERVICE_ID_GETSTATUS            0x03u
#define COM_SERVICE_ID_GETVERSIONINFO       0x04u
#define COM_SERVICE_ID_SENDSIGNAL           0x05u
#define COM_SERVICE_ID_RECEIVESIGNAL        0x06u
#define COM_SERVICE_ID_SENDSIGNALGROUP      0x07u
#define COM_SERVICE_ID_RECEIVESIGNALGROUP   0x08u
#define COM_SERVICE_ID_UPDATESHADOWSIGNAL   0x09u
#define COM_SERVICE_ID_MAINFUNCTIONRX       0x0Au
#define COM_SERVICE_ID_MAINFUNCTIONTX       0x0Bu
#define COM_SERVICE_ID_IPDUGROUPSTART       0x0Cu
#define COM_SERVICE_ID_IPDUGROUPSTOP        0x0Du
#define COM_SERVICE_ID_TRIGGERSENDSIGNAL    0x0Eu
#define COM_SERVICE_ID_INVALIDATESIGNAL     0x10u

/*==================[Internal Types]========================================*/

/* Signal runtime data */
typedef struct {
    boolean Updated;
    uint32 TimeoutTimer;
    uint8 Data[8]; /* Temporary storage for signal data */
} Com_SignalRunTimeType;

/* Signal Group runtime data */
typedef struct {
    boolean Updated;
    uint8* ShadowBuffer;
} Com_SignalGroupRunTimeType;

/* IPdu runtime data */
typedef struct {
    Com_IpduGroupStatusType GroupStatus;
    uint32 TxTimer;
    uint32 RepetitionTimer;
    uint8 RepetitionCount;
    boolean Triggered;
    uint32 TimeoutTimer;
    boolean TimeoutOccurred;
    uint16 PduId;                       /*!< PDU ID for PduR interface */
} Com_IPduRunTimeType;

/* Global Module State */
typedef struct {
    Com_StatusType Status;
    const Com_ConfigType* Config;
    Com_SignalRunTimeType* SignalRunTime;
    Com_SignalGroupRunTimeType* SignalGroupRunTime;
    Com_IPduRunTimeType* IPduRunTime;
    boolean Initialized;
} Com_GlobalType;

/*==================[External Variables]====================================*/

/* Global state variable - defined in Com.c */
extern Com_GlobalType Com_GlobalState;

/*==================[Internal Functions]====================================*/

/* Signal packing/unpacking */
static inline uint64 Com_ExtractSignal(
    const uint8* data,
    uint16 bitPosition,
    uint8 bitSize,
    Com_SignalEndiannessType endianness);

static inline void Com_InsertSignal(
    uint8* data,
    uint16 bitPosition,
    uint8 bitSize,
    Com_SignalEndiannessType endianness,
    uint64 value);

/* IPdu transmission */
static void Com_TransmitIPdu(Com_IPduIdType PduId);
static boolean Com_ShouldTransmitIPdu(Com_IPduIdType PduId);

/* Timeout handling */
static void Com_HandleRxTimeout(Com_IPduIdType PduId);
static void Com_HandleTxTimeout(Com_IPduIdType PduId);

/*==================[Inline Function Definitions]==========================*/

/* Extract signal from buffer (little endian or big endian) */
static inline uint64 Com_ExtractSignal(
    const uint8* data,
    uint16 bitPosition,
    uint8 bitSize,
    Com_SignalEndiannessType endianness)
{
    uint64 value = 0;
    uint16 bytePos = bitPosition / 8U;
    uint8 bitOffset = bitPosition % 8U;
    
    if (endianness == COM_LITTLE_ENDIAN) {
        /* Little endian extraction */
        for (uint8 i = 0; i < ((bitSize + 7U) / 8U) && (bytePos + i) < COM_MAX_IPDU_LENGTH; i++) {
            value |= ((uint64)data[bytePos + i]) << (i * 8U);
        }
        value >>= bitOffset;
    } else {
        /* Big endian extraction */
        uint8 bytesToRead = (bitSize + bitOffset + 7U) / 8U;
        for (sint8 i = bytesToRead - 1U; i >= 0; i--) {
            value = (value << 8U) | data[(uint32_t)bytePos + (uint8_t)i];
        }
        value >>= bitOffset;
    }
    
    /* Mask to actual bit size */
    if (bitSize < 64U) {
        value &= ((uint64)1 << bitSize) - 1U;
    }
    
    return value;
}

/* Insert signal into buffer (little endian or big endian) */
static inline void Com_InsertSignal(
    uint8* data,
    uint16 bitPosition,
    uint8 bitSize,
    Com_SignalEndiannessType endianness,
    uint64 value)
{
    uint16 bytePos = bitPosition / 8U;
    uint8 bitOffset = bitPosition % 8U;
    
    /* Mask value to bit size */
    if (bitSize < 64U) {
        value &= ((uint64)1 << bitSize) - 1U;
    }
    
    if (endianness == COM_LITTLE_ENDIAN) {
        /* Little endian insertion */
        value <<= bitOffset;
        for (uint8 i = 0; i < ((bitSize + bitOffset + 7U) / 8U) && (bytePos + i) < COM_MAX_IPDU_LENGTH; i++) {
            data[bytePos + i] = (uint8)((value >> (i * 8U)) & 0xFFU);
        }
    } else {
        /* Big endian insertion */
        uint8 bytesToWrite = (bitSize + bitOffset + 7U) / 8U;
        uint64 tempValue = value << bitOffset;
        for (sint8 i = bytesToWrite - 1U; i >= 0; i--) {
            data[(uint32_t)bytePos + (uint8_t)i] = (uint8)((tempValue >> ((uint8_t)i * 8U)) & 0xFFU);
        }
    }
}

#endif /* COM_PRIVATE_H */
