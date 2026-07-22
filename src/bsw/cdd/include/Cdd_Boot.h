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
 * @file    Cdd_Boot.h
 * @brief   Complex Driver — Boot-Time Hardware Initialization Interface
 * @version 1.0.0
 * @date    2026-07-23
 *
 * @details
 *   Boot-time platform initialization complex driver.
 *   Consolidates hardware-level initialization needed before AUTOSAR
 *   BSW takes full control:
 *   - HSM early init (for secure boot verification)
 *   - MC_RGM reset reason capture
 *   - Platform clock tree validation
 *   - Boot CRC validation of critical data
 *   - Early safety configuration (FCCU pre-init)
 *
 * @ASIL-D Safety Level
 * @implements AUTOSAR_SWS_CDD — Complex Device Driver: Boot
 */

#ifndef CDD_BOOT_H
#define CDD_BOOT_H

/*==================================================================================================
 *                                         VERSION INFO
 *==================================================================================================*/
#define CDD_BOOT_VENDOR_ID                      43U
#define CDD_BOOT_AR_RELEASE_MAJOR_VERSION       4U
#define CDD_BOOT_AR_RELEASE_MINOR_VERSION       7U
#define CDD_BOOT_AR_RELEASE_REVISION_VERSION    0U
#define CDD_BOOT_SW_MAJOR_VERSION               1U
#define CDD_BOOT_SW_MINOR_VERSION               0U
#define CDD_BOOT_SW_PATCH_VERSION               0U

/*==================================================================================================
 *                                         INCLUDES
 *==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
 *                                         TYPES
 *==================================================================================================*/

/** @brief Reset reason type */
typedef enum {
    CDD_BOOT_RESET_POWER_ON      = 0x00U,  /**< Power-on reset */
    CDD_BOOT_RESET_WATCHDOG      = 0x01U,  /**< Watchdog reset */
    CDD_BOOT_RESET_LOCKUP        = 0x02U,  /**< CPU lockup */
    CDD_BOOT_RESET_SOFTWARE      = 0x03U,  /**< Software-initiated */
    CDD_BOOT_RESET_LOCKSTEP      = 0x04U,  /**< Lockstep fault */
    CDD_BOOT_RESET_FCCU_SAFE     = 0x05U,  /**< FCCU safe state */
    CDD_BOOT_RESET_UNKNOWN       = 0xFFU   /**< Unknown */
} Cdd_Boot_ResetReasonType;

/** @brief Boot configuration */
typedef struct {
    boolean     enableSecureBoot;              /**< Verify secure boot signature */
    boolean     enableCrcValidation;            /**< Validate boot-critical data CRC */
    boolean     enableClockCheck;               /**< Verify clock tree */
    uint32      bootTimeoutMs;                  /**< Boot init timeout */
} Cdd_Boot_ConfigType;

/*==================================================================================================
 *                                         FUNCTION PROTOTYPES
 *==================================================================================================*/
#define CDD_BOOT_START_SEC_CODE
#include "Cdd_MemMap.h"

/**
 * @brief   Initialize boot-time CDD module.
 * @details Called very early by EcuM, before other CDD modules.
 *          Captures reset reason, performs early HSM init,
 *          validates boot data integrity.
 * @param   config  [in] Boot configuration
 * @return  E_OK  — Initialized
 *          E_NOT_OK — Critical boot failure
 */
extern Std_ReturnType Cdd_Boot_Init(const Cdd_Boot_ConfigType* config);

/**
 * @brief   Get boot reset reason.
 * @return  Cdd_Boot_ResetReasonType
 */
extern Cdd_Boot_ResetReasonType Cdd_Boot_GetResetReason(void);

/**
 * @brief   Get raw reset reason register value.
 * @param   reason  [out] Raw reset reason value
 * @return  E_OK  — Valid
 */
extern Std_ReturnType Cdd_Boot_GetResetReasonRaw(uint32* reason);

/**
 * @brief   Clear boot-time reset reason register.
 * @return  E_OK  — Cleared
 */
extern Std_ReturnType Cdd_Boot_ClearResetReason(void);

/**
 * @brief   Boot main function (post-BSW).
 *          Called from Cdd_MainFunction.  Deferred actions.
 */
extern void Cdd_Boot_MainFunction(void);

#define CDD_BOOT_STOP_SEC_CODE
#include "Cdd_MemMap.h"

#endif /* CDD_BOOT_H */
