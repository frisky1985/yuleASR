/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : Unified Test Framework (CMocka-backend via Unity compat layer)
*
* SW Version           : 1.0.0
* Build Date           : 2026-07-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
*
* @file test_framework.h
* @brief Unified Test Framework — delegates to tests/unit/framework/unity.h
*
* This is the canonical test framework header for YuleTech AutoSAR tests.
* It provides:
*   - Unity-style assertion macros (TEST_ASSERT_*)
*   - CMocka-style assertion aliases (assert_true, assert_int_equal, ...)
*   - Test runner macros (TEST_MAIN_BEGIN / TEST_MAIN_END / RUN_TEST)
*   - Extended assertions (ASSERT_EQ, ASSERT_STR_EQ, etc.)
*
* All definitions ultimately use the longjmp-based Unity implementation
* in tests/unit/framework/ (unity.h + unity.c).
==================================================================================================*/

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

/* ─── Core framework: Unity API + CMocka aliases ─────────────────────── */
#include "framework/unity.h"

/*==================================================================================================
*                                      TEST SUITE MACROS
==================================================================================================*/
#define TEST_SUITE(name) \
    static void test_suite_##name##_setup(void); \
    static void test_suite_##name##_teardown(void); \
    void test_suite_##name(void)

#define TEST_SUITE_SETUP(name) \
    static void test_suite_##name##_setup(void)

#define TEST_SUITE_TEARDOWN(name) \
    static void test_suite_##name##_teardown(void)

#define RUN_TEST_SUITE(name) \
    do { \
        g_test_stats.current_suite = #name; \
        printf("\n" TEST_COLOR_CYAN "=== Test Suite: %s ===" TEST_COLOR_RESET "\n", #name); \
        test_suite_##name##_setup(); \
        test_suite_##name(); \
        test_suite_##name##_teardown(); \
    } while(0)

/*==================================================================================================
*                                      TEST CASE MACROS
==================================================================================================*/
#define TEST_CASE(name) \
    void test_##name(void)

/*==================================================================================================
*                                      RUN_TEST (simple call, no setUp/tearDown)
*
* 注意: 部分早期测试用 RUN_TEST 直接调函数，不走 Unity 的 setUp/tearDown 钩子。
*       迁移完成后应统一改为 Unity 风格 (UNITY_BEGIN + UnityRunTest + UNITY_END)。
==================================================================================================*/
#ifndef RUN_TEST
#define RUN_TEST(func)  do { func(); } while (0U)
#endif

/*==================================================================================================
*                                      EXTENDED ASSERTION ALIASES
==================================================================================================*/

/* 扩展布尔/比较断言 (test_framework.h 风格的 ASSERT_*) */
#define ASSERT_EQ(expected, actual)     TEST_ASSERT_EQUAL(expected, actual)
#define ASSERT_NE(expected, actual)     do { \
    TEST_ASSERT((expected) != (actual)); \
} while(0)

#define ASSERT_LT(left, right)          TEST_ASSERT_LESS_THAN(right, left)
#define ASSERT_LE(left, right)          do { \
    TEST_ASSERT((left) <= (right)); \
} while(0)

#define ASSERT_GT(left, right)          TEST_ASSERT_GREATER_THAN(right, left)
#define ASSERT_GE(left, right)          do { \
    TEST_ASSERT((left) >= (right)); \
} while(0)

#define ASSERT_NULL(ptr)                TEST_ASSERT_NULL(ptr)
#define ASSERT_NOT_NULL(ptr)            TEST_ASSERT_NOT_NULL(ptr)
#define ASSERT_TRUE(cond)               TEST_ASSERT_TRUE(cond)
#define ASSERT_FALSE(cond)              TEST_ASSERT_FALSE(cond)

#define ASSERT_STR_EQ(expected, actual) TEST_ASSERT_EQUAL_STRING(expected, actual)
#define ASSERT_STR_NE(expected, actual) do { \
    const char* _a = (expected); const char* _b = (actual); \
    char _msg[256]; \
    snprintf(_msg, sizeof(_msg), "Strings should differ"); \
    UNITY_TEST_ASSERT(strcmp(_a ? _a : "", _b ? _b : "") != 0, _msg, __LINE__, __FILE__); \
} while(0)

#define ASSERT_MEM_EQ(expected, actual, size) \
    TEST_ASSERT_EQUAL_MEMORY(expected, actual, size)

#define ASSERT_WITHIN_RANGE(lower, upper, actual) \
    TEST_ASSERT_IN_RANGE(lower, upper, actual)

/*==================================================================================================
*                                      TEST PASS/SKIP
==================================================================================================*/
#define TEST_PASS()                     do { /* implicit pass */ } while(0)
#define TEST_SKIP(message)              TEST_IGNORE_MESSAGE(message)

/*==================================================================================================
*                                      MOCK UTILITIES (stubs)
==================================================================================================*/
#define MOCK_CALL_COUNT(func)           (0)
#define MOCK_RESET(func)                ((void)0)
#define MOCK_SET_RETURN(func, val)      ((void)(val))
#define MOCK_GET_PARAM(func, idx)       (0)

/*==================================================================================================
*                                      TEST RUNNER MACROS
*
* 提供与旧 test_framework.h 兼容的运行器宏。
* 使用方式:
*   TEST_MAIN_BEGIN()
*       RUN_TEST(test_func_1);
*       RUN_TEST(test_func_2);
*   TEST_MAIN_END()
==================================================================================================*/
#ifdef __GNUC__
#define UNUSED_TEST_ARGS __attribute__((unused))
#else
#define UNUSED_TEST_ARGS
#endif

#define TEST_MAIN_BEGIN() \
    int main(int argc, char** argv) { \
        (void)argc; (void)argv; \
        printf("\n"); \
        printf("===============================================================\n"); \
        printf("  YuleTech BSW Unit Test Framework (CMocka backend)\n"); \
        printf("===============================================================\n"); \
        do {

#define TEST_MAIN_END() \
        } while(0); \
        printf("\n===============================================================\n"); \
        printf("  Tests complete\n"); \
        printf("===============================================================\n"); \
        return 0; \
    }

#endif /* TEST_FRAMEWORK_H */
