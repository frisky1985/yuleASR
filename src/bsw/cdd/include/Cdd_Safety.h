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
 * @file    Cdd_Safety.h
 * @brief   Complex Driver — Safety Integrator Interface
 * @version 1.0.0
 * @date    2026-07-23
 *
 * @details
 *   Safety integrator complex driver for S32K312.
 *   Provides cohesive safety architecture integration:
 *   - FCCU (Fault Collection and Control Unit) management
 *   - Safety interrupt routing (ECC, lockstep, clock, voltage)
 *   - Safety state machine (normal → warning → safe state)
 *   - Dem / DTC event reporting for safety faults
 *   - WdgM-alive supervision reporting
 *   - CRC-based runtime integrity check of safety-relevant RAM
 *
 * @ASIL-D Safety Level
 * @implements AUTOSAR_SWS_CDD — Complex Device Driver: Safety
 */

#ifndef CDD_SAFETY_H
#define CDD_SAFETY_H

/*==================================================================================================
 *                                         VERSION INFO
 *==================================================================================================*/
#define CDD_SAFETY_VENDOR_ID                    43U
#define CDD_SAFETY_AR_RELEASE_MAJOR_VERSION     4U
#define CDD_SAFETY_AR_RELEASE_MINOR_VERSION     7U
#define CDD_SAFETY_AR_RELEASE_REVISION_VERSION  0U
#define CDD_SAFETY_SW_MAJOR_VERSION             1U
#define CDD_SAFETY_SW_MINOR_VERSION             0U
#define CDD_SAFETY_SW_PATCH_VERSION             0U

/*==================================================================================================
 *                                         INCLUDES
 *==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
 *                                         TYPES
 *==================================================================================================*/

/** @brief Safety state machine */
typedef enum {
    CDD_SAFETY_STATE_NORMAL     = 0x00U,       /**< Normal operation */
    CDD_SAFETY_STATE_WARNING    = 0x01U,       /**< Non-critical fault detected */
    CDD_SAFETY_STATE_SAFE       = 0x02U,       /**< Safe state — critical fault */
    CDD_SAFETY_STATE_RESET      = 0x03U        /**< System reset pending */
} Cdd_Safety_StateType;

/** @brief Safety fault source */
typedef enum {
    CDD_SAFETY_FAUST_NONE              = 0x00U,
    CDD_SAFETY_FAULT_LOCKSTEP          = 0x01U, /**< Lockstep mismatch */
    CDD_SAFETY_FAULT_ECC_DOUBLE        = 0x02U, /**< RAM double-bit ECC error */
    CDD_SAFETY_FAULT_ECC_SINGLE_THRESH = 0x03U, /**< ECC single-bit threshold exceeded */
    CDD_SAFETY_FAULT_CLOCK             = 0x04U, /**< Clock monitor fault */
    CDD_SAFETY_FAULT_VOLTAGE           = 0x05U, /**< Voltage monitor fault */
    CDD_SAFETY_FAULT_WATCHDOG          = 0x06U, /**< Watchdog timeout */
    CDD_SAFETY_FAULT_MEMORY_TEST       = 0x07U, /**< RAM test failure */
    CDD_SAFETY_FAULT_HSM               = 0x08U, /**< HSM self-test failure */
    CDD_SAFETY_FAULT_CRC               = 0x09U  /**< CRC integrity failure */
} Cdd_Safety_FaultSourceType;

/** @brief Safety configuration */
typedef struct {
    boolean     enableFccu;                     /**< Enable FCCU management */
    boolean     enableDemReporting;             /**< Report faults to Dem */
    boolean     enableCrcIntegrity;             /**< Enable CRC integrity checks */
    uint32      fccuTimeoutUs;                  /**< FCCU reaction timeout */
    uint32      checkIntervalMs;                /**< Safety check interval (ms) */
} Cdd_Safety_ConfigType;

/*==================================================================================================
 *                                         FUNCTION PROTOTYPES
 *==================================================================================================*/
#define CDD_SAFETY_START_SEC_CODE
#include "Cdd_MemMap.h"

/**
 * @brief   Initialize Safety integrator complex driver.
 * @param   config  [in] Pointer to safety configuration
 * @return  E_OK  — Initialized
 *          E_NOT_OK — Init failed
 */
extern Std_ReturnType Cdd_Safety_Init(const Cdd_Safety_ConfigType* config);

/**
 * @brief   De-initialize safety integrator.
 */
extern void Cdd_Safety_DeInit(void);

/**
 * @brief   Get current safety state.
 * @return  Cdd_Safety_StateType
 */
extern Cdd_Safety_StateType Cdd_Safety_GetState(void);

/**
 * @brief   Report a safety fault to the safety integrator.
 * @details Routes the fault to FCCU, Dem, and updates the safety state
 *          machine appropriately.
 * @param   source  [in] Fault source
 * @param   faultId [in] Detailed fault identifier
 * @return  E_OK  — Fault handled
 *          E_NOT_OK — Handling failed
 */
extern Std_ReturnType Cdd_Safety_ReportFault(Cdd_Safety_FaultSourceType source, uint32 faultId);

/**
 * @brief   Request transition to safe state.
 * @param   reason  [in] Reason code for safe state entry
 */
extern void Cdd_Safety_EnterSafeState(uint32 reason);

/**
 * @brief   Trigger system reset via safety integrator.
 * @param   resetType  [in] Reset type (0 = software, 1 = watchdog, etc.)
 */
extern void Cdd_Safety_SystemReset(uint8 resetType);

/**
 * @brief   Register a callback for safety state transitions.
 * @param   callback  [in] Callback invoked on state change
 * @return  E_OK  — Registered
 */
extern Std_ReturnType Cdd_Safety_RegisterStateCallback(
    void (*callback)(Cdd_Safety_StateType newState));

/**
 * @brief   Safety integrator periodic check.
 *          Called from Cdd_MainFunction.
 *          Checks integrity, FCCU status, and updates state machine.
 */
extern void Cdd_Safety_MainFunction(void);

#define CDD_SAFETY_STOP_SEC_CODE
#include "Cdd_MemMap.h"

#endif /* CDD_SAFETY_H */
