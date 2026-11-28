/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP S32K312
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file Platform_RamTest.h
 * @brief Software RAM self-test primitives (March C- / checkerboard)
 *
 * Pure-software March and checkerboard algorithms used by the RamTst driver
 * for RAM regions that do not have a hardware BIST.  The implementation only
 * touches the supplied word-aligned region, so it is usable both on the
 * S32K312 target and on the host (unit tests pass a RAM buffer).
 *
 * @author yuleASR Team
 * @version 1.0.0
 */

#ifndef PLATFORM_RAMTEST_H
#define PLATFORM_RAMTEST_H

#include "Std_Types.h"

/** @brief Result of a RAM self-test run */
typedef enum
{
    PLATFORM_RAMTEST_OK = 0,    /**< Region tested without errors */
    PLATFORM_RAMTEST_FAILED,    /**< At least one mismatch was detected */
    PLATFORM_RAMTEST_PARAM      /**< Invalid argument */
} Platform_RamTestStatusType;

/** @brief Error record of the most recent RAM self-test run */
typedef struct
{
    uint32 errors;            /**< Number of mismatches detected */
    uint32 lastErrorAddress;  /**< Address of the last mismatch */
    uint32 expected;          /**< Expected value at that address */
    uint32 actual;            /**< Value actually read back */
} Platform_RamTestResultType;

/**
 * @brief Runs the March C- algorithm over a word-aligned RAM region
 *
 * March C- (6 elements):
 *   ⇕(w0); ⇑(r0,w1); ⇑(r1,w0); ⇓(r0,w1); ⇓(r1,w0); ⇕(r0)
 *
 * @param base       Start of the region (must be 4-byte aligned)
 * @param sizeBytes  Region size in bytes (must be a non-zero multiple of 4)
 * @param result     Optional error record (may be NULL_PTR)
 * @return PLATFORM_RAMTEST_OK when no mismatch was found
 */
Platform_RamTestStatusType Platform_RamTest_MarchC(volatile uint32* base,
                                                   uint32 sizeBytes,
                                                   Platform_RamTestResultType* result);

/**
 * @brief Runs the checkerboard test over a word-aligned RAM region
 *
 * Writes 0x55555555 / 0xAAAAAAAA on alternating words, reads back, then
 * repeats with the inverted pattern.  On success the region is left holding
 * the inverted pattern.
 *
 * @param base       Start of the region (must be 4-byte aligned)
 * @param sizeBytes  Region size in bytes (must be a non-zero multiple of 4)
 * @param result     Optional error record (may be NULL_PTR)
 * @return PLATFORM_RAMTEST_OK when no mismatch was found
 */
Platform_RamTestStatusType Platform_RamTest_Checkerboard(volatile uint32* base,
                                                         uint32 sizeBytes,
                                                         Platform_RamTestResultType* result);

/**
 * @brief Clears an error record
 * @param result Error record to reset (NULL_PTR is ignored)
 */
void Platform_RamTest_ClearResult(Platform_RamTestResultType* result);

#endif /* PLATFORM_RAMTEST_H */

/*==================[end of file]===========================================*/
