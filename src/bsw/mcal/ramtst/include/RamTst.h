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
 * @file RamTst.h
 * @brief RAM Test Driver
 * @version 1.0.0
 */

#ifndef RAMTST_H
#define RAMTST_H

#include "Std_Types.h"

#define RAMTST_MODULE_ID        0x64U
#define RAMTST_VENDOR_ID        0x0001U

/* Error Codes */
#define RAMTST_E_NO_ERROR       0x00U
#define RAMTST_E_PARAM_POINTER  0x01U
#define RAMTST_E_UNINIT         0x02U

/* Service IDs */
#define RAMTST_SID_INIT         0x01U
#define RAMTST_SID_DEINIT       0x02U
#define RAMTST_SID_RUN          0x03U
#define RAMTST_SID_STOP         0x04U
#define RAMTST_SID_GET_RESULT   0x05U
#define RAMTST_SID_GET_STATUS   0x06U

/* Types */
typedef enum {
    RAMTST_ALGORITHM_MARCH = 0,
    RAMTST_ALGORITHM_GALPAT,
    RAMTST_ALGORITHM_WALKPATH
} RamTst_AlgType;

typedef enum {
    RAMTST_RESULT_OK = 0,
    RAMTST_RESULT_NOT_TESTED,
    RAMTST_RESULT_FAILED
} RamTst_TestResultType;

typedef enum {
    RAMTST_STATUS_UNINIT = 0,
    RAMTST_STATUS_IDLE,
    RAMTST_STATUS_RUNNING
} RamTst_StatusType;

typedef struct {
    uint32 StartAddress;
    uint32 Size;
    RamTst_AlgType Algorithm;
    uint32 CallCycle;
} RamTst_ConfigType;

/* Functions */
void RamTst_Init(const RamTst_ConfigType* ConfigPtr);
void RamTst_DeInit(void);
Std_ReturnType RamTst_Run(void);
void RamTst_Stop(void);
RamTst_TestResultType RamTst_GetTestResult(void);
RamTst_StatusType RamTst_GetTestStatus(void);
void RamTst_MainFunction(void);

#endif
