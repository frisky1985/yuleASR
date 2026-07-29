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
 * @file Eep.h
 * @brief EEPROM Driver
 * @version 1.0.0
 */

#ifndef EEP_H
#define EEP_H

#include "Std_Types.h"

#define EEP_MODULE_ID           0x5FU
#define EEP_VENDOR_ID           0x0001U

/* Error Codes */
#define EEP_E_NO_ERROR          0x00U
#define EEP_E_PARAM_POINTER     0x01U
#define EEP_E_PARAM_ADDRESS     0x02U
#define EEP_E_PARAM_LENGTH      0x03U
#define EEP_E_UNINIT            0x04U
#define EEP_E_BUSY              0x05U

/* Service IDs */
#define EEP_SID_INIT            0x01U
#define EEP_SID_DEINIT          0x02U
#define EEP_SID_READ            0x03U
#define EEP_SID_WRITE           0x04U
#define EEP_SID_ERASE           0x05U
#define EEP_SID_CANCEL          0x06U
#define EEP_SID_GET_STATUS      0x07U
#define EEP_SID_GET_JOB_RESULT  0x08U

/* Types */
typedef uint32 Eep_AddressType;
typedef uint32 Eep_LengthType;

typedef enum {
    EEP_JOB_OK = 0,
    EEP_JOB_PENDING,
    EEP_JOB_FAILED,
    EEP_JOB_CANCELED
} Eep_JobResultType;

typedef enum {
    EEP_UNINIT = 0,
    EEP_IDLE,
    EEP_BUSY
} Eep_StatusType;

typedef struct {
    Eep_AddressType BaseAddress;
    Eep_LengthType Size;
    uint32 JobCallCycle;
} Eep_ConfigType;

/* Functions */
void Eep_Init(const Eep_ConfigType* ConfigPtr);
void Eep_DeInit(void);
Std_ReturnType Eep_Read(Eep_AddressType Address, uint8* DataPtr, Eep_LengthType Length);
Std_ReturnType Eep_Write(Eep_AddressType Address, const uint8* DataPtr, Eep_LengthType Length);
Std_ReturnType Eep_Erase(Eep_AddressType Address, Eep_LengthType Length);
void Eep_Cancel(void);
Eep_StatusType Eep_GetStatus(void);
Eep_JobResultType Eep_GetJobResult(void);
void Eep_MainFunction(void);

#endif
