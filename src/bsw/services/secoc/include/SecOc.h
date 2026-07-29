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
 * @file SecOc.h
 * @brief Secure Onboard Communication
 * @version 1.0.0
 */

#ifndef SECOC_H
#define SECOC_H

#include "Std_Types.h"
#include "ComStack_Types.h"

#define SECOC_MODULE_ID         159U
#define SECOC_VENDOR_ID         0x0001U

/* Error Codes */
#define SECOC_E_NO_ERROR        0x00U
#define SECOC_E_PARAM_POINTER   0x01U
#define SECOC_E_UNINIT          0x02U
#define SECOC_E_INVALID_PDU     0x03U

/* Service IDs */
#define SECOC_SID_INIT                  0x01U
#define SECOC_SID_DEINIT                0x02U
#define SECOC_SID_GET_VERSION_INFO      0x03U
#define SECOC_SID_TRANSMIT              0x04U
#define SECOC_SID_VERIFY_STATUS_OVERRIDE 0x05U

/* Verification Status */
typedef enum {
    SECOC_VERIFICATION_SUCCESS = 0,
    SECOC_VERIFICATION_FAILED,
    SECOC_VERIFICATION_PENDING,
    SECOC_VERIFICATION_OVERRIDE
} SecOc_VerificationStatusType;

/* Freshness Value Type */
typedef uint64 SecOc_FreshnessValueType;

/* Authenticator Type */
#define SECOC_MAX_AUTHENTICATOR_SIZE    16U
typedef uint8 SecOc_AuthenticatorType[SECOC_MAX_AUTHENTICATOR_SIZE];

/* Security Profile */
typedef struct {
    uint16 ProfileId;
    uint8 AuthenticatorLength;
    uint8 FreshnessLength;
    boolean UseTrippleFreshness;
} SecOc_SecurityProfileType;

/* PDU Configuration */
typedef struct {
    PduIdType PduId;
    uint16 DataId;
    const SecOc_SecurityProfileType* Profile;
    uint16 FreshnessValueId;
} SecOc_PduConfigType;

/* Configuration */
typedef struct {
    uint16 NumPdus;
    const SecOc_PduConfigType* Pdus;
} SecOc_ConfigType;

/* Functions */
void SecOc_Init(const SecOc_ConfigType* ConfigPtr);
void SecOc_DeInit(void);
#if (SECOC_VERSION_INFO_API == STD_ON)
void SecOc_GetVersionInfo(Std_VersionInfoType* VersionInfo);
#endif
Std_ReturnType SecOc_Transmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);
void SecOc_VerifyStatusOverride(PduIdType PduId, SecOc_VerificationStatusType Status);
void SecOc_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr);
void SecOc_MainFunction(void);

#endif
