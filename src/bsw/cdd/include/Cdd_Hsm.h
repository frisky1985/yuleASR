/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP S32K312 / i.MX8M Mini
* Dependencies         : AUTOSAR 4.7
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file    Cdd_Hsm.h
 * @brief   Complex Driver — Hardware Security Module (HSM) Interface
 * @version 1.0.0
 * @date    2026-07-23
 *
 * @details
 *   Hardware Security Module complex driver for S32K312.
 *   Provides hardware-accelerated cryptographic operations, secure key
 *   management, TRNG, and secure boot verification via the on-chip HSM
 *   (CSEc / SHE-compatible security module).
 *
 * Responsibilities:
 *   - HSM hardware initialization / de-initialization
 *   - AES-GCM / SHA-256 / ECDSA / ECDH hardware acceleration
 *   - Secure key storage management
 *   - True Random Number Generation (TRNG)
 *   - Secure boot signature verification
 *   - HSM self-test and health monitoring
 *
 * @ASIL-D Safety Level
 * @implements AUTOSAR_SWS_CDD — Complex Device Driver: HSM
 */

#ifndef CDD_HSM_H
#define CDD_HSM_H

/*==================================================================================================
 *                                         VERSION INFO
 *==================================================================================================*/
#define CDD_HSM_VENDOR_ID                       43U
#define CDD_HSM_AR_RELEASE_MAJOR_VERSION        4U
#define CDD_HSM_AR_RELEASE_MINOR_VERSION        7U
#define CDD_HSM_AR_RELEASE_REVISION_VERSION     0U
#define CDD_HSM_SW_MAJOR_VERSION                1U
#define CDD_HSM_SW_MINOR_VERSION                0U
#define CDD_HSM_SW_PATCH_VERSION                0U

/*==================================================================================================
 *                                         INCLUDES
 *==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
 *                                         MACROS
 *==================================================================================================*/
/** @brief Timeout for HSM command execution (ms) */
#define CDD_HSM_CMD_TIMEOUT_MS                  (1000U)

/** @brief Timeout for HSM response (ms) */
#define CDD_HSM_RSP_TIMEOUT_MS                  (5000U)

/** @brief Max key slots supported by HSM */
#define CDD_HSM_MAX_KEY_SLOTS                   16U

/*==================================================================================================
 *                                         TYPES
 *==================================================================================================*/

/** @brief HSM internal state */
typedef enum {
    CDD_HSM_STATE_UNINIT = 0,                   /**< Not initialized */
    CDD_HSM_STATE_INIT,                         /**< Init in progress */
    CDD_HSM_STATE_READY,                        /**< Ready for operations */
    CDD_HSM_STATE_BUSY,                         /**< Processing a job */
    CDD_HSM_STATE_ERROR                         /**< Error / fault */
} Cdd_Hsm_StateType;

/** @brief HSM external status */
typedef enum {
    CDD_HSM_STATUS_IDLE     = 0x00U,            /**< Idle */
    CDD_HSM_STATUS_BUSY     = 0x01U,            /**< Busy */
    CDD_HSM_STATUS_ERROR    = 0x02U,            /**< Error */
    CDD_HSM_STATUS_UNINIT   = 0x03U             /**< Not initialized */
} Cdd_Hsm_StatusType;

/** @brief HSM configuration structure */
typedef struct {
    boolean     enableAes;                      /**< Enable AES-GCM */
    boolean     enableEcc;                      /**< Enable ECDSA/ECDH */
    boolean     enableSha;                      /**< Enable SHA-256 */
    boolean     enableTrng;                     /**< Enable TRNG */
    boolean     enableKeyStore;                 /**< Enable secure key store */
    uint32      timeoutUs;                      /**< Hardware operation timeout */
} Cdd_Hsm_ConfigType;

/*==================================================================================================
 *                                         FUNCTION PROTOTYPES
 *==================================================================================================*/
#define CDD_HSM_START_SEC_CODE
#include "Cdd_MemMap.h"

/**
 * @brief   Initialize the HSM complex driver.
 * @param   config  [in] Pointer to HSM configuration
 * @return  E_OK  — HSM initialized and ready
 *          E_NOT_OK — Init failed
 */
extern Std_ReturnType Cdd_Hsm_Init(const Cdd_Hsm_ConfigType* config);

/**
 * @brief   De-initialize the HSM complex driver.
 */
extern void Cdd_Hsm_DeInit(void);

/**
 * @brief   Get HSM availability status.
 * @return  TRUE  — HSM is available and ready
 *          FALSE — HSM unavailable / error
 */
extern boolean Cdd_Hsm_IsAvailable(void);

/**
 * @brief   Get HSM external status.
 * @return  Cdd_Hsm_StatusType
 */
extern Cdd_Hsm_StatusType Cdd_Hsm_GetStatus(void);

/**
 * @brief   Generate random bytes using HSM TRNG.
 * @param   output  [out] Buffer for random bytes
 * @param   length  [in]  Number of bytes to generate
 * @return  E_OK  — Generation succeeded
 *          E_NOT_OK — TRNG failure
 */
extern Std_ReturnType Cdd_Hsm_GenerateRandom(uint8* output, uint32 length);

/**
 * @brief   HSM self-test.
 * @return  E_OK  — Self-test passed
 *          E_NOT_OK — Self-test failed
 */
extern Std_ReturnType Cdd_Hsm_SelfTest(void);

/**
 * @brief   Verify secure boot image signature via HSM.
 * @param   imageHash       [in] Hash of the boot image
 * @param   imageHashLen    [in] Hash length
 * @param   signature       [in] Signature to verify
 * @param   signatureLen    [in] Signature length
 * @param   verifyResult    [out] Verification result
 * @return  E_OK  — Verification completed (check verifyResult)
 *          E_NOT_OK — HSM error during verification
 */
extern Std_ReturnType Cdd_Hsm_SecureBootVerify(
    const uint8*        imageHash,
    uint32              imageHashLen,
    const uint8*        signature,
    uint32              signatureLen,
    boolean*            verifyResult);

/**
 * @brief   HSM periodic health check.
 *          Called from Cdd_MainFunction.
 */
extern void Cdd_Hsm_MainFunction(void);

#define CDD_HSM_STOP_SEC_CODE
#include "Cdd_MemMap.h"

#endif /* CDD_HSM_H */
