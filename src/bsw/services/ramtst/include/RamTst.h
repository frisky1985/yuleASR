/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : Det
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file RamTst.h
 * @brief RAM Test — March C Algorithm (Service Layer)
 * @version 1.0.0
 *
 * Performs March C and March C- algorithms on RAM regions
 * to detect stuck-at, transition, and coupling faults.
 * Designed for startup POST and runtime periodic checks.
 *
 * @implements AUTOSAR_SWS_RamTest.pdf (partial)
 */

#ifndef RAMTST_H
#define RAMTST_H

#include "Std_Types.h"
#include "RamTst_Cfg.h"

/*==================================================================================================
 *                                    VERSION INFORMATION
 *==================================================================================================*/
#define RAMTST_VENDOR_ID                        (0x0001U)
#define RAMTST_MODULE_ID                        (0x8AU)
#define RAMTST_INSTANCE_ID                      (0x00U)

#define RAMTST_AR_RELEASE_MAJOR_VERSION         (0x04U)
#define RAMTST_AR_RELEASE_MINOR_VERSION         (0x04U)
#define RAMTST_AR_RELEASE_REVISION_VERSION      (0x00U)
#define RAMTST_SW_MAJOR_VERSION                 (0x01U)
#define RAMTST_SW_MINOR_VERSION                 (0x00U)
#define RAMTST_SW_PATCH_VERSION                 (0x00U)

/*==================================================================================================
 *                                    SERVICE IDs
 *==================================================================================================*/
#define RAMTST_SID_INIT                         (0x01U)
#define RAMTST_SID_DEINIT                       (0x02U)
#define RAMTST_SID_GETVERSIONINFO               (0x03U)
#define RAMTST_SID_RUNTEST                      (0x10U)
#define RAMTST_SID_GETRESULT                    (0x11U)
#define RAMTST_SID_ABORT                        (0x12U)
#define RAMTST_SID_MAINFUNCTION                 (0x13U)

/*==================================================================================================
 *                                    DET ERROR CODES
 *==================================================================================================*/
#define RAMTST_E_PARAM_POINTER                  (0x01U)
#define RAMTST_E_PARAM_CONFIG                   (0x02U)
#define RAMTST_E_UNINIT                         (0x03U)
#define RAMTST_E_ALREADY_INITIALIZED            (0x04U)
#define RAMTST_E_INVALID_REGION                 (0x05U)
#define RAMTST_E_TEST_FAILED                    (0x06U)
#define RAMTST_E_TEST_ABORTED                   (0x07U)
#define RAMTST_E_BUSY                           (0x08U)

/*==================================================================================================
 *                                    TYPE DEFINITIONS
 *==================================================================================================*/

/** Algorithm type */
typedef uint8 RamTst_AlgorithmType;
#define RAMTST_ALGO_MARCH_C                     (0x00U)
#define RAMTST_ALGO_MARCH_C_MINUS               (0x01U)
#define RAMTST_ALGO_CHECKERBOARD                (0x02U)
#define RAMTST_ALGO_WALKING_ONES                (0x03U)

/** Test phase */
typedef uint8 RamTst_PhaseType;
#define RAMTST_PHASE_IDLE                       (0x00U)
#define RAMTST_PHASE_MARCH_C_UP                 (0x01U)
#define RAMTST_PHASE_MARCH_C_DOWN               (0x02U)
#define RAMTST_PHASE_COMPLETE                   (0x03U)
#define RAMTST_PHASE_ABORTED                    (0x04U)
#define RAMTST_PHASE_ERROR                      (0x05U)

/** Test result */
typedef uint8 RamTst_ResultType;
#define RAMTST_RESULT_NOT_RUN                   (0x00U)
#define RAMTST_RESULT_PASSED                    (0x01U)
#define RAMTST_RESULT_FAILED                    (0x02U)
#define RAMTST_RESULT_ABORTED                   (0x03U)

/** RAM region descriptor */
typedef struct {
    uint32  StartAddr;
    uint32  Size;
    uint16  RegionId;
} RamTst_RegionType;

/** Callback on test completion */
typedef void (*RamTst_CompletionCallback)(RamTst_ResultType Result);

/** Global configuration */
typedef struct {
    uint8                     NumRegions;
    const RamTst_RegionType*  Regions;
    RamTst_AlgorithmType      Algorithm;
    boolean                   RunOnStartup;
    RamTst_CompletionCallback CompletionCb;
    boolean                   DevErrorDetect;
    boolean                   VersionInfoApi;
} RamTst_ConfigType;

/*==================================================================================================
 *                                    FUNCTION DECLARATIONS
 *==================================================================================================*/

/** @brief Initialise the RAM Test module */
void RamTst_Init(const RamTst_ConfigType* ConfigPtr);

/** @brief De-initialise the RAM Test module */
void RamTst_DeInit(void);

/** @brief Get version information */
#if (RAMTST_VERSION_INFO_API == STD_ON)
void RamTst_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/** @brief Run March C test on a RAM region */
Std_ReturnType RamTst_RunTest(uint16 RegionId);

/** @brief Get the last test result */
Std_ReturnType RamTst_GetResult(RamTst_ResultType* Result);

/** @brief Abort a running test */
Std_ReturnType RamTst_Abort(void);

/** @brief Main function — step through long-running test phases */
void RamTst_MainFunction(void);

#endif /* RAMTST_H */
