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

/**
 * @file EcuC.c
 * @brief EcuC Implementation - ECU Configuration Module
 * @version 1.0.0
 * @date 2024-05-05
 */

#include "EcuC.h"
#include "EcuC_Cfg.h"
#include "Det.h"
#include "SchM_EcuC.h"

/*==================[Local Macros]==========================================*/
#define ECUC_BYTES_TO_BITS(bytes)          ((bytes) * 8U)
#define ECUC_BITS_TO_BYTES(bits)           (((bits) + 7U) / 8U)

/*==================[Local Types]===========================================*/
typedef struct {
    EcuC_StateType State;
    const EcuC_ConfigType* Config;
} EcuC_InternalType;

/*==================[Local Variables]=======================================*/
static EcuC_InternalType EcuC_Internal = {
    .State = ECUC_STATE_UNINIT,
    .Config = NULL_PTR
};

/*==================[Local Function Prototypes]=============================*/
static uint64 EcuC_ExtractSignal(const uint8* Data, uint16 StartBit, uint16 Size);
static void EcuC_InsertSignal(uint8* Data, uint16 StartBit, uint16 Size, uint64 Value);

/*==================[Function Definitions]==================================*/
void EcuC_Init(const EcuC_ConfigType* ConfigPtr) {
#if (ECUC_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ConfigPtr) {
        Det_ReportError(ECUC_MODULE_ID, 0U, ECUC_SID_INIT, ECUC_E_PARAM_CONFIG);
        return;
    }
#endif
    
    SchM_Enter_EcuC(ECUC_EXCLUSIVE_AREA_0);
    EcuC_Internal.Config = ConfigPtr;
    EcuC_Internal.State = ECUC_STATE_INIT;
    SchM_Exit_EcuC(ECUC_EXCLUSIVE_AREA_0);
}

void EcuC_DeInit(void) {
#if (ECUC_DEV_ERROR_DETECT == STD_ON)
    if (ECUC_STATE_UNINIT == EcuC_Internal.State) {
        Det_ReportError(ECUC_MODULE_ID, 0U, ECUC_SID_DEINIT, ECUC_E_UNINIT);
        return;
    }
#endif
    
    SchM_Enter_EcuC(ECUC_EXCLUSIVE_AREA_0);
    EcuC_Internal.Config = NULL_PTR;
    EcuC_Internal.State = ECUC_STATE_UNINIT;
    SchM_Exit_EcuC(ECUC_EXCLUSIVE_AREA_0);
}

#if (ECUC_VERSION_INFO_API == STD_ON)
void EcuC_GetVersionInfo(Std_VersionInfoType* VersionInfo) {
#if (ECUC_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == VersionInfo) {
        Det_ReportError(ECUC_MODULE_ID, 0U, ECUC_SID_GET_VERSION_INFO, ECUC_E_PARAM_POINTER);
        return;
    }
#endif
    
    VersionInfo->vendorID = ECUC_VENDOR_ID_VALUE;
    VersionInfo->moduleID = ECUC_MODULE_ID_VALUE;
    VersionInfo->sw_major_version = ECUC_SW_MAJOR_VERSION_VALUE;
    VersionInfo->sw_minor_version = ECUC_SW_MINOR_VERSION_VALUE;
    VersionInfo->sw_patch_version = ECUC_SW_PATCH_VERSION_VALUE;
}
#endif

Std_ReturnType EcuC_TransmitSignal(uint16 SignalId, const void* SignalDataPtr) {
    Std_ReturnType result = E_NOT_OK;
    
#if (ECUC_DEV_ERROR_DETECT == STD_ON)
    if (ECUC_STATE_UNINIT == EcuC_Internal.State) {
        Det_ReportError(ECUC_MODULE_ID, 0U, ECUC_SID_TRANSMIT_SIGNAL, ECUC_E_UNINIT);
        return E_NOT_OK;
    }
    if (NULL_PTR == SignalDataPtr) {
        Det_ReportError(ECUC_MODULE_ID, 0U, ECUC_SID_TRANSMIT_SIGNAL, ECUC_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    
    if ((NULL_PTR != EcuC_Internal.Config) && 
        (SignalId < EcuC_Internal.Config->SignalCount)) {
        const EcuC_SignalConfigType* signal = &EcuC_Internal.Config->Signals[SignalId];
        result = E_OK;
    }
    
    return result;
}

Std_ReturnType EcuC_ReceiveSignal(uint16 SignalId, void* SignalDataPtr) {
    Std_ReturnType result = E_NOT_OK;
    
#if (ECUC_DEV_ERROR_DETECT == STD_ON)
    if (ECUC_STATE_UNINIT == EcuC_Internal.State) {
        Det_ReportError(ECUC_MODULE_ID, 0U, ECUC_SID_RECEIVE_SIGNAL, ECUC_E_UNINIT);
        return E_NOT_OK;
    }
    if (NULL_PTR == SignalDataPtr) {
        Det_ReportError(ECUC_MODULE_ID, 0U, ECUC_SID_RECEIVE_SIGNAL, ECUC_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    
    if ((NULL_PTR != EcuC_Internal.Config) && 
        (SignalId < EcuC_Internal.Config->SignalCount)) {
        result = E_OK;
    }
    
    return result;
}

Std_ReturnType EcuC_UpdateShadowSignal(uint16 SignalId, const void* SignalDataPtr) {
#if (ECUC_DEV_ERROR_DETECT == STD_ON)
    if (ECUC_STATE_UNINIT == EcuC_Internal.State) {
        Det_ReportError(ECUC_MODULE_ID, 0U, ECUC_SID_UPDATE_SHADOW_SIGNAL, ECUC_E_UNINIT);
        return E_NOT_OK;
    }
    if (NULL_PTR == SignalDataPtr) {
        Det_ReportError(ECUC_MODULE_ID, 0U, ECUC_SID_UPDATE_SHADOW_SIGNAL, ECUC_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    return E_OK;
}

Std_ReturnType EcuC_ReceiveShadowSignal(uint16 SignalId, void* SignalDataPtr) {
#if (ECUC_DEV_ERROR_DETECT == STD_ON)
    if (ECUC_STATE_UNINIT == EcuC_Internal.State) {
        Det_ReportError(ECUC_MODULE_ID, 0U, ECUC_SID_RECEIVE_SHADOW_SIGNAL, ECUC_E_UNINIT);
        return E_NOT_OK;
    }
    if (NULL_PTR == SignalDataPtr) {
        Det_ReportError(ECUC_MODULE_ID, 0U, ECUC_SID_RECEIVE_SHADOW_SIGNAL, ECUC_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    return E_OK;
}

Std_ReturnType EcuC_SendSignal(uint16 SignalId, const void* SignalDataPtr) {
#if (ECUC_DEV_ERROR_DETECT == STD_ON)
    if (ECUC_STATE_UNINIT == EcuC_Internal.State) {
        Det_ReportError(ECUC_MODULE_ID, 0U, ECUC_SID_SEND_SIGNAL, ECUC_E_UNINIT);
        return E_NOT_OK;
    }
    if (NULL_PTR == SignalDataPtr) {
        Det_ReportError(ECUC_MODULE_ID, 0U, ECUC_SID_SEND_SIGNAL, ECUC_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    return EcuC_TransmitSignal(SignalId, SignalDataPtr);
}

Std_ReturnType EcuC_SendShadowSignal(uint16 SignalId) {
#if (ECUC_DEV_ERROR_DETECT == STD_ON)
    if (ECUC_STATE_UNINIT == EcuC_Internal.State) {
        Det_ReportError(ECUC_MODULE_ID, 0U, ECUC_SID_SEND_SHADOW_SIGNAL, ECUC_E_UNINIT);
        return E_NOT_OK;
    }
#endif
    return E_OK;
}

void EcuC_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr) {
#if (ECUC_DEV_ERROR_DETECT == STD_ON)
    if (ECUC_STATE_UNINIT == EcuC_Internal.State) {
        Det_ReportError(ECUC_MODULE_ID, 0U, 0x10U, ECUC_E_UNINIT);
        return;
    }
#endif
    
    if ((NULL_PTR != EcuC_Internal.Config) && 
        (RxPduId < EcuC_Internal.Config->PduCount)) {
        /* Process received PDU */
    }
}

void EcuC_TxConfirmation(PduIdType TxPduId, Std_ReturnType Result) {
    (void)TxPduId;
    (void)Result;
}

void EcuC_TpRxIndication(PduIdType RxPduId, Std_ReturnType Result) {
    (void)RxPduId;
    (void)Result;
}

void EcuC_TpTxConfirmation(PduIdType TxPduId, Std_ReturnType Result) {
    (void)TxPduId;
    (void)Result;
}

void EcuC_MainFunction(void) {
    if (ECUC_STATE_INIT != EcuC_Internal.State) {
        return;
    }
    
    /* Process signal gateway routing */
    if (NULL_PTR != EcuC_Internal.Config) {
        for (uint16 i = 0U; i < EcuC_Internal.Config->RoutingPathCount; i++) {
            /* Process routing path */
        }
    }
}

/*==================[Local Functions]=======================================*/
static uint64 EcuC_ExtractSignal(const uint8* Data, uint16 StartBit, uint16 Size) {
    uint64 value = 0U;
    uint16 bytePos = StartBit / 8U;
    uint16 bitPos = StartBit % 8U;
    
    for (uint16 i = 0U; i < Size; i++) {
        uint16 currentByte = bytePos + ((bitPos + i) / 8U);
        uint16 currentBit = (bitPos + i) % 8U;
        if (Data[currentByte] & (1U << currentBit)) {
            value |= (1ULL << i);
        }
    }
    
    return value;
}

static void EcuC_InsertSignal(uint8* Data, uint16 StartBit, uint16 Size, uint64 Value) {
    uint16 bytePos = StartBit / 8U;
    uint16 bitPos = StartBit % 8U;
    
    for (uint16 i = 0U; i < Size; i++) {
        uint16 currentByte = bytePos + ((bitPos + i) / 8U);
        uint16 currentBit = (bitPos + i) % 8U;
        if (Value & (1ULL << i)) {
            Data[currentByte] |= (1U << currentBit);
        } else {
            Data[currentByte] &= ~(1U << currentBit);
        }
    }
}

/*==================[End of File]===========================================*/
