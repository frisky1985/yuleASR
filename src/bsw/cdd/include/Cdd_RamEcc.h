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
 * @file    Cdd_RamEcc.h
 * @brief   Complex Driver — RAM ECC Error Handler Interface
 * @version 1.0.0
 * @date    2026-07-23
 *
 * @details
 *   RAM ECC complex driver for S32K312.
 *   Provides interrupt-level ECC error handling for on-chip SRAM:
 *   - Single-bit error correction (recoverable)
 *   - Double-bit error detection (critical fault)
 *   - ECC error logging and threshold monitoring
 *   - Integration with Dem for DTC reporting
 *   - Integration with FCCU for safety reaction
 *
 *   This module replaces the platform-level Platform_EccHandler.c
 *   with a proper AUTOSAR CDD-layer driver.
 *
 * @ASIL-D Safety Level
 * @implements AUTOSAR_SWS_CDD — Complex Device Driver: RAM ECC
 */

#ifndef CDD_RAMECC_H
#define CDD_RAMECC_H

/*==================================================================================================
 *                                         VERSION INFO
 *==================================================================================================*/
#define CDD_RAMECC_VENDOR_ID                    43U
#define CDD_RAMECC_AR_RELEASE_MAJOR_VERSION     4U
#define CDD_RAMECC_AR_RELEASE_MINOR_VERSION     7U
#define CDD_RAMECC_AR_RELEASE_REVISION_VERSION  0U
#define CDD_RAMECC_SW_MAJOR_VERSION             1U
#define CDD_RAMECC_SW_MINOR_VERSION             0U
#define CDD_RAMECC_SW_PATCH_VERSION             0U

/*==================================================================================================
 *                                         INCLUDES
 *==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
 *                                         MACROS
 *==================================================================================================*/
/** @brief Max ECC error log entries */
#define CDD_RAMECC_MAX_ERROR_LOG                16U

/** @brief ECC single-bit threshold before escalation */
#define CDD_RAMECC_SINGLE_BIT_THRESHOLD         10U

/** @brief ECC double-bit threshold (immediate escalation) */
#define CDD_RAMECC_DOUBLE_BIT_THRESHOLD         1U

/*==================================================================================================
 *                                         TYPES
 *==================================================================================================*/

/** @brief ECC error type */
typedef enum {
    CDD_RAMECC_ERROR_NONE           = 0x00U,   /**< No error */
    CDD_RAMECC_ERROR_SINGLE_BIT     = 0x01U,   /**< Correctable single-bit error */
    CDD_RAMECC_ERROR_DOUBLE_BIT     = 0x02U,   /**< Uncorrectable double-bit error */
    CDD_RAMECC_ERROR_BUS            = 0x04U,   /**< Bus error */
    CDD_RAMECC_ERROR_OVERFLOW       = 0x08U    /**< Error counter overflow */
} Cdd_RamEcc_ErrorType;

/** @brief ECC error record */
typedef struct {
    Cdd_RamEcc_ErrorType    errorType;          /**< Type of error */
    uint32                  errorAddress;       /**< Address where error occurred */
    uint32                  timestamp;          /**< Timestamp */
    uint32                  correctedData;      /**< Corrected data (single-bit) */
    boolean                 isNvMBlock;         /**< TRUE if NvM memory */
    uint16                  nvMBlockId;         /**< NvM block ID if applicable */
} Cdd_RamEcc_ErrorRecordType;

/** @brief ECC handling policy */
typedef enum {
    CDD_RAMECC_POLICY_LOG       = 0x00U,       /**< Log only */
    CDD_RAMECC_POLICY_CORRECT   = 0x01U,       /**< Correct single-bit */
    CDD_RAMECC_POLICY_NOTIFY    = 0x02U,       /**< Notify Dem */
    CDD_RAMECC_POLICY_SAFE_STATE = 0x03U,      /**< Enter safe state */
    CDD_RAMECC_POLICY_RESET     = 0x04U        /**< System reset */
} Cdd_RamEcc_PolicyType;

/** @brief ECC handler configuration */
typedef struct {
    Cdd_RamEcc_PolicyType   singleBitPolicy;    /**< Policy for single-bit errors */
    Cdd_RamEcc_PolicyType   doubleBitPolicy;    /**< Policy for double-bit errors */
    boolean                 enableInterrupt;    /**< Enable ECC interrupt */
    boolean                 enableLogging;      /**< Enable error logging */
    uint8                   singleBitThreshold; /**< Single-bit error threshold */
} Cdd_RamEcc_ConfigType;

/** @brief ECC error callback */
typedef void (*Cdd_RamEcc_CallbackType)(const Cdd_RamEcc_ErrorRecordType* errorInfo);

/*==================================================================================================
 *                                         FUNCTION PROTOTYPES
 *==================================================================================================*/
#define CDD_RAMECC_START_SEC_CODE
#include "Cdd_MemMap.h"

/**
 * @brief   Initialize RAM ECC complex driver.
 * @param   config  [in] Pointer to ECC configuration
 * @return  E_OK  — Init successful
 *          E_NOT_OK — Init failed
 */
extern Std_ReturnType Cdd_RamEcc_Init(const Cdd_RamEcc_ConfigType* config);

/**
 * @brief   De-initialize RAM ECC complex driver.
 */
extern void Cdd_RamEcc_DeInit(void);

/**
 * @brief   ECC interrupt service routine.
 *          Connect this to the MSCM_ECC interrupt vector.
 */
extern void Cdd_RamEcc_Isr(void);

/**
 * @brief   Register error callback.
 * @param   callback  [in] Function pointer
 * @return  E_OK  — Registered
 *          E_NOT_OK — NULL pointer
 */
extern Std_ReturnType Cdd_RamEcc_RegisterCallback(Cdd_RamEcc_CallbackType callback);

/**
 * @brief   Get ECC error log entry.
 * @param   index     [in]  Log index (0-based)
 * @param   errorInfo [out] Error record
 * @return  E_OK  — Valid error info
 *          E_NOT_OK — Invalid index
 */
extern Std_ReturnType Cdd_RamEcc_GetErrorLog(uint8 index, Cdd_RamEcc_ErrorRecordType* errorInfo);

/**
 * @brief   Get ECC error counts.
 * @param   singleBitCount  [out] Single-bit error count (may be NULL)
 * @param   doubleBitCount  [out] Double-bit error count (may be NULL)
 * @return  E_OK  — Success
 */
extern Std_ReturnType Cdd_RamEcc_GetErrorCounts(uint32* singleBitCount, uint32* doubleBitCount);

/**
 * @brief   Clear ECC error log.
 * @return  E_OK  — Cleared
 */
extern Std_ReturnType Cdd_RamEcc_ClearErrorLog(void);

/**
 * @brief   RAM ECC periodic monitoring.
 *          Called from Cdd_MainFunction.
 *          Checks threshold violations and reports to Dem.
 */
extern void Cdd_RamEcc_MainFunction(void);

/**
 * @brief   Enable ECC for a RAM region.
 * @param   startAddr  [in] Start address
 * @param   size       [in] Size in bytes
 * @return  E_OK  — Enabled
 *          E_NOT_OK — Failed
 */
extern Std_ReturnType Cdd_RamEcc_EnableRegion(uint32 startAddr, uint32 size);

/**
 * @brief   Disable ECC for a RAM region.
 * @param   startAddr  [in] Start address
 * @param   size       [in] Size in bytes
 * @return  E_OK  — Disabled
 *          E_NOT_OK — Failed
 */
extern Std_ReturnType Cdd_RamEcc_DisableRegion(uint32 startAddr, uint32 size);

#define CDD_RAMECC_STOP_SEC_CODE
#include "Cdd_MemMap.h"

#endif /* CDD_RAMECC_H */
