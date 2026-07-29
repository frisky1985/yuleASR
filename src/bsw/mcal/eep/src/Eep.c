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
 * @file Eep.c
 * @brief EEPROM Driver Implementation
 */

#include "Eep.h"
#include "Eep_Cfg.h"
#include "Det.h"

typedef enum {
    EEP_STATE_UNINIT = 0,
    EEP_STATE_IDLE,
    EEP_STATE_READ,
    EEP_STATE_WRITE,
    EEP_STATE_ERASE
} Eep_StateType;

static Eep_StateType Eep_State = EEP_STATE_UNINIT;
static const Eep_ConfigType* Eep_ConfigPtr = NULL_PTR;
static Eep_JobResultType Eep_JobResult = EEP_JOB_OK;
static Eep_AddressType Eep_CurrentAddress;
static uint8* Eep_CurrentDataPtr;
static Eep_LengthType Eep_CurrentLength;

void Eep_Init(const Eep_ConfigType* ConfigPtr) {
#if (EEP_DEV_ERROR_DETECT == STD_ON)
    if (NULL_PTR == ConfigPtr) {
        Det_ReportError(EEP_MODULE_ID, 0U, EEP_SID_INIT, EEP_E_PARAM_POINTER);
        return;
    }
#endif
    Eep_ConfigPtr = ConfigPtr;
    Eep_State = EEP_STATE_IDLE;
    Eep_JobResult = EEP_JOB_OK;
}

void Eep_DeInit(void) {
    Eep_State = EEP_STATE_UNINIT;
    Eep_ConfigPtr = NULL_PTR;
}

Std_ReturnType Eep_Read(Eep_AddressType Address, uint8* DataPtr, Eep_LengthType Length) {
#if (EEP_DEV_ERROR_DETECT == STD_ON)
    if (Eep_State == EEP_STATE_UNINIT) {
        Det_ReportError(EEP_MODULE_ID, 0U, EEP_SID_READ, EEP_E_UNINIT);
        return E_NOT_OK;
    }
    if ((NULL_PTR == DataPtr) || (0U == Length)) {
        Det_ReportError(EEP_MODULE_ID, 0U, EEP_SID_READ, EEP_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    if (Eep_State != EEP_STATE_IDLE) {
        return E_NOT_OK;
    }
    Eep_CurrentAddress = Address;
    Eep_CurrentDataPtr = DataPtr;
    Eep_CurrentLength = Length;
    Eep_State = EEP_STATE_READ;
    Eep_JobResult = EEP_JOB_PENDING;
    return E_OK;
}

Std_ReturnType Eep_Write(Eep_AddressType Address, const uint8* DataPtr, Eep_LengthType Length) {
#if (EEP_DEV_ERROR_DETECT == STD_ON)
    if (Eep_State == EEP_STATE_UNINIT) {
        Det_ReportError(EEP_MODULE_ID, 0U, EEP_SID_WRITE, EEP_E_UNINIT);
        return E_NOT_OK;
    }
    if ((NULL_PTR == DataPtr) || (0U == Length)) {
        Det_ReportError(EEP_MODULE_ID, 0U, EEP_SID_WRITE, EEP_E_PARAM_POINTER);
        return E_NOT_OK;
    }
#endif
    if (Eep_State != EEP_STATE_IDLE) {
        return E_NOT_OK;
    }
    Eep_CurrentAddress = Address;
    Eep_CurrentDataPtr = (uint8*)DataPtr;
    Eep_CurrentLength = Length;
    Eep_State = EEP_STATE_WRITE;
    Eep_JobResult = EEP_JOB_PENDING;
    return E_OK;
}

Std_ReturnType Eep_Erase(Eep_AddressType Address, Eep_LengthType Length) {
#if (EEP_DEV_ERROR_DETECT == STD_ON)
    if (Eep_State == EEP_STATE_UNINIT) {
        Det_ReportError(EEP_MODULE_ID, 0U, EEP_SID_ERASE, EEP_E_UNINIT);
        return E_NOT_OK;
    }
#endif
    if (Eep_State != EEP_STATE_IDLE) {
        return E_NOT_OK;
    }
    Eep_CurrentAddress = Address;
    Eep_CurrentLength = Length;
    Eep_State = EEP_STATE_ERASE;
    Eep_JobResult = EEP_JOB_PENDING;
    return E_OK;
}

void Eep_Cancel(void) {
    if (Eep_State != EEP_STATE_UNINIT) {
        Eep_State = EEP_STATE_IDLE;
        Eep_JobResult = EEP_JOB_CANCELED;
    }
}

Eep_StatusType Eep_GetStatus(void) {
    switch (Eep_State) {
        case EEP_STATE_UNINIT:
            return EEP_UNINIT;
        case EEP_STATE_IDLE:
            return EEP_IDLE;
        case EEP_STATE_READ:
        case EEP_STATE_WRITE:
        case EEP_STATE_ERASE:
            return EEP_BUSY;
        default:
            return EEP_UNINIT;
    }
}

Eep_JobResultType Eep_GetJobResult(void) {
    return Eep_JobResult;
}

void Eep_MainFunction(void) {
    if (Eep_State == EEP_STATE_IDLE || Eep_State == EEP_STATE_UNINIT) {
        return;
    }
    /* Simulate job completion */
    if (Eep_State == EEP_STATE_READ || Eep_State == EEP_STATE_WRITE || Eep_State == EEP_STATE_ERASE) {
        Eep_JobResult = EEP_JOB_OK;
        Eep_State = EEP_STATE_IDLE;
    }
}
