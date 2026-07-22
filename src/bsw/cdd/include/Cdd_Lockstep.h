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
 * @file    Cdd_Lockstep.h
 * @brief   Complex Driver — Lockstep Core Monitor Interface
 * @version 1.0.0
 * @date    2026-07-23
 *
 * @details
 *   Lockstep core monitor complex driver for S32K312.
 *   Manages the Cortex-M7 lockstep core through MSCM registers:
 *   - Lockstep mode control (split/lockstep)
 *   - Lockstep error detection & reporting
 *   - Built-In Self-Test (LBIST) for lockstep cores
 *   - FCCU fault reporting integration
 *   - Reset reason tracking (lockstep faults)
 *
 *   Provides @ASIL-D safety coverage for dual-core lockstep monitoring.
 *
 * @implements AUTOSAR_SWS_CDD — Complex Device Driver: Lockstep
 */

#ifndef CDD_LOCKSTEP_H
#define CDD_LOCKSTEP_H

/*==================================================================================================
 *                                         VERSION INFO
 *==================================================================================================*/
#define CDD_LOCKSTEP_VENDOR_ID                  43U
#define CDD_LOCKSTEP_AR_RELEASE_MAJOR_VERSION   4U
#define CDD_LOCKSTEP_AR_RELEASE_MINOR_VERSION   7U
#define CDD_LOCKSTEP_AR_RELEASE_REVISION_VERSION 0U
#define CDD_LOCKSTEP_SW_MAJOR_VERSION           1U
#define CDD_LOCKSTEP_SW_MINOR_VERSION           0U
#define CDD_LOCKSTEP_SW_PATCH_VERSION           0U

/*==================================================================================================
 *                                         INCLUDES
 *==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
 *                                         MACROS
 *==================================================================================================*/

/** @brief S32K312 MSCM register base address */
#define CDD_LOCKSTEP_MSCM_BASE                  (0x40260000UL)

/** @brief S32K312 MC_RGM register base address */
#define CDD_LOCKSTEP_RGM_BASE                   (0x40080000UL)

/** @brief S32K312 FCCU register base address */
#define CDD_LOCKSTEP_FCCU_BASE                  (0x40090000UL)

/** @brief Lockstep mode values */
#define CDD_LOCKSTEP_MODE_SPLIT                 0U      /**< Dual-core split (debug) */
#define CDD_LOCKSTEP_MODE_LOCKSTEP              1U      /**< Lockstep (redundant) */

/** @brief BIST timeout default (us) */
#define CDD_LOCKSTEP_BIST_TIMEOUT_US            10000U

/*==================================================================================================
 *                                         TYPES
 *==================================================================================================*/

/** @brief Lockstep mode */
typedef enum {
    CDD_LOCKSTEP_MODE_DISABLED  = 0x00U,       /**< Lockstep disabled (split mode) */
    CDD_LOCKSTEP_MODE_ENABLED   = 0x01U,       /**< Lockstep enabled (redundant execution) */
    CDD_LOCKSTEP_MODE_DEBUG     = 0x02U        /**< Debug mode (split + debug support) */
} Cdd_Lockstep_ModeType;

/** @brief BIST status */
typedef enum {
    CDD_LOCKSTEP_BIST_IDLE          = 0x00U,   /**< No BIST in progress */
    CDD_LOCKSTEP_BIST_RUNNING       = 0x01U,   /**< BIST running */
    CDD_LOCKSTEP_BIST_COMPLETE_PASS = 0x02U,   /**< BIST passed */
    CDD_LOCKSTEP_BIST_COMPLETE_FAIL = 0x03U    /**< BIST failed */
} Cdd_Lockstep_BistStatusType;

/** @brief Lockstep status */
typedef struct {
    boolean     isActive;                       /**< Lockstep is active */
    boolean     hasError;                       /**< Lockstep error detected */
    boolean     mismatchDetected;               /**< Core mismatch detected */
    Cdd_Lockstep_BistStatusType bistStatus;     /**< Last BIST status */
    uint32      resetReason;                    /**< Last reset reason register */
} Cdd_Lockstep_StatusType;

/** @brief Lockstep configuration */
typedef struct {
    Cdd_Lockstep_ModeType   mode;               /**< Lockstep operating mode */
    boolean                 enableBist;         /**< Enable BIST during init */
    boolean                 enableEout;         /**< Enable error output */
    boolean                 enableFccuReporting;/**< Report faults to FCCU */
    uint32                  bistTimeoutUs;      /**< BIST timeout in microseconds */
    uint32                  checkIntervalMs;    /**< Health check interval (ms) */
} Cdd_Lockstep_ConfigType;

/*==================================================================================================
 *                                         FUNCTION PROTOTYPES
 *==================================================================================================*/
#define CDD_LOCKSTEP_START_SEC_CODE
#include "Cdd_MemMap.h"

/**
 * @brief   Initialize Lockstep complex driver.
 * @param   config  [in] Pointer to lockstep configuration
 * @return  E_OK  — Initialized
 *          E_NOT_OK — Init failed
 */
extern Std_ReturnType Cdd_Lockstep_Init(const Cdd_Lockstep_ConfigType* config);

/**
 * @brief   De-initialize lockstep driver.
 */
extern void Cdd_Lockstep_DeInit(void);

/**
 * @brief   Set lockstep operating mode.
 * @param   mode  [in] Target mode
 * @return  E_OK  — Mode set
 *          E_NOT_OK — Failed
 */
extern Std_ReturnType Cdd_Lockstep_SetMode(Cdd_Lockstep_ModeType mode);

/**
 * @brief   Get current lockstep status.
 * @param   status  [out] Status structure
 * @return  E_OK  — Status valid
 *          E_NOT_OK — Error
 */
extern Std_ReturnType Cdd_Lockstep_GetStatus(Cdd_Lockstep_StatusType* status);

/**
 * @brief   Run lockstep Built-In Self-Test (LBIST).
 * @param   timeoutUs  [in] Timeout in microseconds (0 = default)
 * @return  E_OK  — BIST passed
 *          E_NOT_OK — BIST failed or timeout
 */
extern Std_ReturnType Cdd_Lockstep_RunBist(uint32 timeoutUs);

/**
 * @brief   Clear lockstep error status.
 * @return  E_OK  — Cleared
 *          E_NOT_OK — Clear failed
 */
extern Std_ReturnType Cdd_Lockstep_ClearError(void);

/**
 * @brief   Lockstep periodic health check.
 *          Called from Cdd_MainFunction.
 *          Reports mismatches to Dem / FCCU.
 */
extern void Cdd_Lockstep_MainFunction(void);

#define CDD_LOCKSTEP_STOP_SEC_CODE
#include "Cdd_MemMap.h"

#endif /* CDD_LOCKSTEP_H */
