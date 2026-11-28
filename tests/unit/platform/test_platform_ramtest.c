/**********************************************************************************************************************
 * @file       test_platform_ramtest.c
 * @brief      Host unit tests for Platform_RamTest (March C- / checkerboard)
 * @author     YuleTech AutoSAR Team
 * @date       2026-11-28
 * @copyright  Shanghai Yule Electronics Technology Co., Ltd.
 *********************************************************************************************************************/

#include <string.h>

#include "unity.h"
#include "Platform_RamTest.h"

#define TEST_WORDS  (64U)

static uint32 testBuffer[TEST_WORDS];

void setUp(void)
{
    (void)memset(testBuffer, 0xA5, sizeof(testBuffer));
}

void tearDown(void)
{
}

static void test_marchc_runs_over_region(void)
{
    Platform_RamTestResultType result;
    uint32 i;

    TEST_ASSERT_EQUAL_INT(PLATFORM_RAMTEST_OK,
        Platform_RamTest_MarchC(testBuffer, (uint32)sizeof(testBuffer), &result));
    TEST_ASSERT_EQUAL_UINT32(0U, result.errors);

    /* March C- leaves the region at all-zero. */
    for (i = 0U; i < TEST_WORDS; i++)
    {
        TEST_ASSERT_EQUAL_UINT32(0U, testBuffer[i]);
    }
}

static void test_marchc_without_result_record(void)
{
    TEST_ASSERT_EQUAL_INT(PLATFORM_RAMTEST_OK,
        Platform_RamTest_MarchC(testBuffer, (uint32)sizeof(testBuffer), NULL_PTR));
}

static void test_checkerboard_runs_over_region(void)
{
    Platform_RamTestResultType result;
    uint32 i;

    TEST_ASSERT_EQUAL_INT(PLATFORM_RAMTEST_OK,
        Platform_RamTest_Checkerboard(testBuffer, (uint32)sizeof(testBuffer), &result));
    TEST_ASSERT_EQUAL_UINT32(0U, result.errors);

    /* Final state holds the inverted checkerboard pattern. */
    for (i = 0U; i < TEST_WORDS; i++)
    {
        uint32 expected = ((i & 1U) == 0U) ? 0xAAAAAAAAU : 0x55555555U;

        TEST_ASSERT_EQUAL_UINT32(expected, testBuffer[i]);
    }
}

static void test_invalid_parameters(void)
{
    Platform_RamTestResultType result;

    TEST_ASSERT_EQUAL_INT(PLATFORM_RAMTEST_PARAM,
        Platform_RamTest_MarchC(NULL_PTR, 16U, &result));
    TEST_ASSERT_EQUAL_INT(PLATFORM_RAMTEST_PARAM,
        Platform_RamTest_MarchC(testBuffer, 0U, &result));
    TEST_ASSERT_EQUAL_INT(PLATFORM_RAMTEST_PARAM,
        Platform_RamTest_MarchC(testBuffer, 6U, &result));
    TEST_ASSERT_EQUAL_INT(PLATFORM_RAMTEST_PARAM,
        Platform_RamTest_Checkerboard(NULL_PTR, 16U, &result));
    TEST_ASSERT_EQUAL_INT(PLATFORM_RAMTEST_PARAM,
        Platform_RamTest_Checkerboard(testBuffer, 3U, &result));
}

static void test_clear_result(void)
{
    Platform_RamTestResultType result;

    result.errors = 5U;
    result.lastErrorAddress = 0x20000000U;
    result.expected = 1U;
    result.actual = 2U;

    Platform_RamTest_ClearResult(&result);
    TEST_ASSERT_EQUAL_UINT32(0U, result.errors);
    TEST_ASSERT_EQUAL_UINT32(0U, result.lastErrorAddress);
    TEST_ASSERT_EQUAL_UINT32(0U, result.expected);
    TEST_ASSERT_EQUAL_UINT32(0U, result.actual);

    Platform_RamTest_ClearResult(NULL_PTR); /* must not crash */
}

int main(void)
{
    UnityBegin();

    RUN_TEST(test_marchc_runs_over_region);
    RUN_TEST(test_marchc_without_result_record);
    RUN_TEST(test_checkerboard_runs_over_region);
    RUN_TEST(test_invalid_parameters);
    RUN_TEST(test_clear_result);

    return UnityEnd();
}

/**********************************************************************************************************************
 * END OF FILE
 *********************************************************************************************************************/
