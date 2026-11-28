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
 * @file Platform_RamTest.c
 * @brief Software March C- / checkerboard RAM self-test implementation
 */

/*==================================================================================================
*                                        INCLUDE FILES
==================================================================================================*/
#include "Platform_RamTest.h"

/*==================================================================================================
*                                        LOCAL MACROS
==================================================================================================*/
#define PLATFORM_RAMTEST_WORD_SIZE      (4U)
#define PLATFORM_RAMTEST_PATTERN_A      (0x55555555U)
#define PLATFORM_RAMTEST_PATTERN_B      (0xAAAAAAAAU)

/*==================================================================================================
*                                        LOCAL FUNCTIONS
==================================================================================================*/
static void Platform_RamTest_RecordError(Platform_RamTestResultType* result,
                                         uint32 address,
                                         uint32 expected,
                                         uint32 actual)
{
    if (result != NULL_PTR)
    {
        result->errors++;
        result->lastErrorAddress = address;
        result->expected = expected;
        result->actual = actual;
    }
}

static uint32 Platform_RamTest_AddressOf(const volatile uint32* base, uint32 index)
{
    return (uint32)(uintptr)(base + index);
}

static boolean Platform_RamTest_Validate(volatile uint32* base, uint32 sizeBytes)
{
    if (base == NULL_PTR)
    {
        return FALSE;
    }
    if (sizeBytes == 0U)
    {
        return FALSE;
    }
    if ((sizeBytes % PLATFORM_RAMTEST_WORD_SIZE) != 0U)
    {
        return FALSE;
    }
    /* Reject misaligned bases (uintptr keeps the check portable). */
    if (((uint32)(uintptr)base % PLATFORM_RAMTEST_WORD_SIZE) != 0U)
    {
        return FALSE;
    }
    return TRUE;
}

/* March element: ascending read expected / write pattern. */
static void Platform_RamTest_MarchAscending(volatile uint32* base,
                                            uint32 words,
                                            uint32 expected,
                                            uint32 writePattern,
                                            Platform_RamTestResultType* result)
{
    uint32 i;

    for (i = 0U; i < words; i++)
    {
        uint32 actual = base[i];

        if (actual != expected)
        {
            Platform_RamTest_RecordError(result, Platform_RamTest_AddressOf(base, i), expected, actual);
        }
        base[i] = writePattern;
    }
}

/* March element: descending read expected / write pattern. */
static void Platform_RamTest_MarchDescending(volatile uint32* base,
                                             uint32 words,
                                             uint32 expected,
                                             uint32 writePattern,
                                             Platform_RamTestResultType* result)
{
    uint32 i = words;

    while (i > 0U)
    {
        uint32 index = i - 1U;
        uint32 actual = base[index];

        if (actual != expected)
        {
            Platform_RamTest_RecordError(result, Platform_RamTest_AddressOf(base, index), expected, actual);
        }
        base[index] = writePattern;
        i--;
    }
}

/* March element: ascending read-only check. */
static void Platform_RamTest_MarchCheckAscending(volatile uint32* base,
                                                 uint32 words,
                                                 uint32 expected,
                                                 Platform_RamTestResultType* result)
{
    uint32 i;

    for (i = 0U; i < words; i++)
    {
        uint32 actual = base[i];

        if (actual != expected)
        {
            Platform_RamTest_RecordError(result, Platform_RamTest_AddressOf(base, i), expected, actual);
        }
    }
}

/*==================================================================================================
*                                        GLOBAL FUNCTIONS
==================================================================================================*/
void Platform_RamTest_ClearResult(Platform_RamTestResultType* result)
{
    if (result != NULL_PTR)
    {
        result->errors = 0U;
        result->lastErrorAddress = 0U;
        result->expected = 0U;
        result->actual = 0U;
    }
}

Platform_RamTestStatusType Platform_RamTest_MarchC(volatile uint32* base,
                                                   uint32 sizeBytes,
                                                   Platform_RamTestResultType* result)
{
    uint32 words;
    uint32 i;

    Platform_RamTest_ClearResult(result);

    if (Platform_RamTest_Validate(base, sizeBytes) == FALSE)
    {
        return PLATFORM_RAMTEST_PARAM;
    }

    words = sizeBytes / PLATFORM_RAMTEST_WORD_SIZE;

    /* M0: ⇕(w0) */
    for (i = 0U; i < words; i++)
    {
        base[i] = 0x00000000U;
    }
    /* M1: ⇑(r0,w1) */
    Platform_RamTest_MarchAscending(base, words, 0x00000000U, 0xFFFFFFFFU, result);
    /* M2: ⇑(r1,w0) */
    Platform_RamTest_MarchAscending(base, words, 0xFFFFFFFFU, 0x00000000U, result);
    /* M3: ⇓(r0,w1) */
    Platform_RamTest_MarchDescending(base, words, 0x00000000U, 0xFFFFFFFFU, result);
    /* M4: ⇓(r1,w0) */
    Platform_RamTest_MarchDescending(base, words, 0xFFFFFFFFU, 0x00000000U, result);
    /* M5: ⇕(r0) */
    Platform_RamTest_MarchCheckAscending(base, words, 0x00000000U, result);

    if ((result != NULL_PTR) && (result->errors > 0U))
    {
        return PLATFORM_RAMTEST_FAILED;
    }
    return PLATFORM_RAMTEST_OK;
}

Platform_RamTestStatusType Platform_RamTest_Checkerboard(volatile uint32* base,
                                                         uint32 sizeBytes,
                                                         Platform_RamTestResultType* result)
{
    uint32 words;
    uint32 i;

    Platform_RamTest_ClearResult(result);

    if (Platform_RamTest_Validate(base, sizeBytes) == FALSE)
    {
        return PLATFORM_RAMTEST_PARAM;
    }

    words = sizeBytes / PLATFORM_RAMTEST_WORD_SIZE;

    /* Pattern A on even words, pattern B on odd words. */
    for (i = 0U; i < words; i++)
    {
        base[i] = ((i & 1U) == 0U) ? PLATFORM_RAMTEST_PATTERN_A : PLATFORM_RAMTEST_PATTERN_B;
    }
    for (i = 0U; i < words; i++)
    {
        uint32 expected = ((i & 1U) == 0U) ? PLATFORM_RAMTEST_PATTERN_A : PLATFORM_RAMTEST_PATTERN_B;
        uint32 actual = base[i];

        if (actual != expected)
        {
            Platform_RamTest_RecordError(result, Platform_RamTest_AddressOf(base, i), expected, actual);
        }
    }

    /* Inverted pattern. */
    for (i = 0U; i < words; i++)
    {
        base[i] = ((i & 1U) == 0U) ? PLATFORM_RAMTEST_PATTERN_B : PLATFORM_RAMTEST_PATTERN_A;
    }
    for (i = 0U; i < words; i++)
    {
        uint32 expected = ((i & 1U) == 0U) ? PLATFORM_RAMTEST_PATTERN_B : PLATFORM_RAMTEST_PATTERN_A;
        uint32 actual = base[i];

        if (actual != expected)
        {
            Platform_RamTest_RecordError(result, Platform_RamTest_AddressOf(base, i), expected, actual);
        }
    }

    if ((result != NULL_PTR) && (result->errors > 0U))
    {
        return PLATFORM_RAMTEST_FAILED;
    }
    return PLATFORM_RAMTEST_OK;
}

/*==================[end of file]===========================================*/
