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
 * @file    E2E.h
 * @brief   E2E (End-to-End) Protection Library - Main Header
 * @details Provides E2E protection for ASIL-D level data integrity
 * @author  AutoSAR Team
 * @version 1.0.0
 ******************************************************************************/

#ifndef E2E_H
#define E2E_H

#include "Std_Types.h"

/*=============================================================================*
 * Global Preprocessor Definitions
 *=============================================================================*/
#define E2E_VENDOR_ID          0x0001U
#define E2E_MODULE_ID          0xF0U
#define E2E_AR_RELEASE_MAJOR_VERSION   4
#define E2E_AR_RELEASE_MINOR_VERSION   4
#define E2E_AR_RELEASE_PATCH_VERSION   0
#define E2E_SW_MAJOR_VERSION   1
#define E2E_SW_MINOR_VERSION   0
#define E2E_SW_PATCH_VERSION   0

/* Error Codes */
#define E2E_E_OK               0x00U
#define E2E_E_NOT_OK           0x01U
#define E2E_E_INPUTERR_NULL    0x13U
#define E2E_E_INPUTERR_WRONG   0x15U
#define E2E_E_INTERR           0x19U
#define E2E_E_OK_SOMELOST      0x26U

/*=============================================================================*
 * Global Type Definitions
 *=============================================================================*/

typedef uint8 E2E_P01DataIDType;
typedef uint8 E2E_P01CounterType;
typedef uint8 E2E_P01CRCType;

typedef uint16 E2E_P02DataIDType;
typedef uint8 E2E_P02CounterType;
typedef uint8 E2E_P02CRCType;

typedef uint32 E2E_P04DataIDType;
typedef uint16 E2E_P04CounterType;
typedef uint32 E2E_P04CRCType;

typedef uint64 E2E_P05DataIDType;
typedef uint16 E2E_P05CounterType;
typedef uint64 E2E_P05CRCType;

typedef uint16 E2E_P06DataIDType;
typedef uint8 E2E_P06CounterType;
typedef uint16 E2E_P06CRCType;

typedef uint32 E2E_P07DataIDType;
typedef uint8 E2E_P07CounterType;
typedef uint32 E2E_P07CRCType;

/* E2E State Type */
typedef enum {
    E2E_P_OK = 0,
    E2E_P_NONEWDATA,
    E2E_P_WRONGCRC,
    E2E_P_SYNC,
    E2E_P_INITIAL,
    E2E_P_REPEATED,
    E2E_P_OKSOMELOST,
    E2E_P_WRONGSEQUENCE
} E2E_PCheckStatusType;

/* E2E State machine state */
typedef enum {
    E2E_SM_VALID = 0,
    E2E_SM_DEINIT,
    E2E_SM_NODATA,
    E2E_SM_INIT,
    E2E_SM_INVALID
} E2E_SMStateType;

/** @req SWS_E2E_00001 */
/*=============================================================================*
 * Global Function Prototypes
 *=============================================================================*/
Std_ReturnType E2E_Init(const void* ConfigPtr);
/** @req SWS_E2E_00002 */
Std_ReturnType E2E_DeInit(void);

#endif /* E2E_H */
