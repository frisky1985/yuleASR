/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : NvM (or Flash driver), Det
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file FlStSt.h
 * @brief Flash Test (March C + Erase Verify) — AUTOSAR Service Layer
 * @version 1.0.0
 *
 * Performs March C algorithm on flash sectors to detect stuck-at
 * and coupling faults, plus erase / program verify for post-write
 * data integrity checks.
 *
 * @implements AUTOSAR_SWS_FlashTest.pdf (partial)
 */

#ifndef FLSTST_H
#define FLSTST_H

#include "Std_Types.h"
#include "FlStSt_Cfg.h"

/*==================================================================================================
 *                                    VERSION INFORMATION
 *==================================================================================================*/
#define FLSTST_VENDOR_ID                        (0x0001U)
#define FLSTST_MODULE_ID                        (0x89U)
#define FLSTST_INSTANCE_ID                      (0x00U)

#define FLSTST_AR_RELEASE_MAJOR_VERSION         (0x04U)
#define FLSTST_AR_RELEASE_MINOR_VERSION         (0x04U)
#define FLSTST_AR_RELEASE_REVISION_VERSION      (0x00U)
#define FLSTST_SW_MAJOR_VERSION                 (0x01U)
#define FLSTST_SW_MINOR_VERSION                 (0x00U)
#define FLSTST_SW_PATCH_VERSION                 (0x00U)

/*==================================================================================================
 *                                    SERVICE IDs
 *==================================================================================================*/
#define FLSTST_SID_INIT                         (0x01U)
#define FLSTST_SID_DEINIT                       (0x02U)
#define FLSTST_SID_GETVERSIONINFO               (0x03U)
#define FLSTST_SID_RUNTEST                      (0x10U)
#define FLSTST_SID_VERIFYERASE                  (0x11U)
#define FLSTST_SID_VERIFYPROGRAM                (0x12U)
#define FLSTST_SID_GETRESULT                    (0x13U)
#define FLSTST_SID_ABORT                        (0x14U)
#define FLSTST_SID_MAINFUNCTION                 (0x15U)

/*==================================================================================================
 *                                    DET ERROR CODES
 *==================================================================================================*/
#define FLSTST_E_PARAM_POINTER                  (0x01U)
#define FLSTST_E_PARAM_CONFIG                   (0x02U)
#define FLSTST_E_UNINIT                         (0x03U)
#define FLSTST_E_ALREADY_INITIALIZED            (0x04U)
#define FLSTST_E_INVALID_SECTOR                 (0x05U)
#define FLSTST_E_TEST_FAILED                    (0x06U)
#define FLSTST_E_TEST_ABORTED                   (0x07U)
#define FLSTST_E_BUSY                           (0x08U)
#define FLSTST_E_INIT_FAILED                    (0x09U)
#define FLSTST_E_NOT_SUPPORTED                  (0x0AU)

/*==================================================================================================
 *                                    TYPE DEFINITIONS
 *==================================================================================================*/

/** Test algorithm type */
typedef uint8 FlStSt_AlgorithmType;
#define FLSTST_ALGO_MARCH_C                     (0x00U)
#define FLSTST_ALGO_MARCH_C_MINUS               (0x01U)
#define FLSTST_ALGO_CHECKERBOARD                (0x02U)

/** Test phase */
typedef uint8 FlStSt_PhaseType;
#define FLSTST_PHASE_IDLE                       (0x00U)
#define FLSTST_PHASE_MARCH_C_UP                 (0x01U)
#define FLSTST_PHASE_MARCH_C_DOWN               (0x02U)
#define FLSTST_PHASE_WRITE_BACKGROUND           (0x03U)
#define FLSTST_PHASE_VERIFY                     (0x04U)
#define FLSTST_PHASE_COMPLETE                   (0x05U)
#define FLSTST_PHASE_ABORTED                    (0x06U)
#define FLSTST_PHASE_ERROR                      (0x07U)

/** Test result */
typedef uint8 FlStSt_ResultType;
#define FLSTST_RESULT_NOT_RUN                   (0x00U)
#define FLSTST_RESULT_PASSED                    (0x01U)
#define FLSTST_RESULT_FAILED                    (0x02U)
#define FLSTST_RESULT_ABORTED                   (0x03U)

/** Sector descriptor */
typedef struct {
    uint32  StartAddr;     /* Sector start address */
    uint32  Size;          /* Sector size in bytes */
    uint16  SectorId;      /* Logical sector identifier */
    uint16  PageSize;      /* Flash page size in bytes */
} FlStSt_SectorType;

/** Global configuration */
typedef struct {
    uint8                 NumSectors;
    const FlStSt_SectorType* Sectors;
    FlStSt_AlgorithmType  Algorithm;
    boolean               RunOnInit;
    boolean               DevErrorDetect;
    boolean               VersionInfoApi;
} FlStSt_ConfigType;

/*==================================================================================================
 *                                    FUNCTION DECLARATIONS
 *==================================================================================================*/

/** @brief Initialise the Flash Test module */
void FlStSt_Init(const FlStSt_ConfigType* ConfigPtr);

/** @brief De-initialise the Flash Test module */
void FlStSt_DeInit(void);

/** @brief Get version information */
#if (FLSTST_VERSION_INFO_API == STD_ON)
void FlStSt_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/** @brief Run March C test on a flash sector */
Std_ReturnType FlStSt_RunTest(uint16 SectorId);

/** @brief Verify an erase operation (read-back = 0xFF) */
Std_ReturnType FlStSt_VerifyErase(uint16 SectorId, boolean* Result);

/** @brief Verify a program operation (read-back = expected data) */
Std_ReturnType FlStSt_VerifyProgram(uint16 SectorId, const uint8* ExpectedData,
                                    uint16 Length, boolean* Result);

/** @brief Get the last test result */
Std_ReturnType FlStSt_GetResult(FlStSt_ResultType* Result);

/** @brief Abort a running test */
Std_ReturnType FlStSt_Abort(void);

/** @brief Main function — step through long-running test phases */
void FlStSt_MainFunction(void);

#endif /* FLSTST_H */
