/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : Unit Test Framework
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                      TEST CONFIGURATION
==================================================================================================*/
#define TEST_FRAMEWORK_VERSION_MAJOR    1
#define TEST_FRAMEWORK_VERSION_MINOR    0
#define TEST_FRAMEWORK_VERSION_PATCH    0

/* Enable colored output */
#define TEST_COLOR_OUTPUT               1

/*==================================================================================================
*                                      COLOR DEFINITIONS
==================================================================================================*/
#if TEST_COLOR_OUTPUT
#define TEST_COLOR_RESET                "\033[0m"
#define TEST_COLOR_RED                  "\033[31m"
#define TEST_COLOR_GREEN                "\033[32m"
#define TEST_COLOR_YELLOW               "\033[33m"
#define TEST_COLOR_BLUE                 "\033[34m"
#define TEST_COLOR_MAGENTA              "\033[35m"
#define TEST_COLOR_CYAN                 "\033[36m"
#define TEST_COLOR_WHITE                "\033[37m"
#else
#define TEST_COLOR_RESET                ""
#define TEST_COLOR_RED                  ""
#define TEST_COLOR_GREEN                ""
#define TEST_COLOR_YELLOW               ""
#define TEST_COLOR_BLUE                 ""
#define TEST_COLOR_MAGENTA              ""
#define TEST_COLOR_CYAN                 ""
#define TEST_COLOR_WHITE                ""
#endif

/*==================================================================================================
*                                      TEST RESULT TRACKING
==================================================================================================*/
typedef struct {
    int total;
    int passed;
    int failed;
    int skipped;
    const char* current_suite;
    const char* current_test;
    jmp_buf jump_buffer;
    int jump_ready;
} TestStats;

static TestStats g_test_stats = {0, 0, 0, 0, NULL, NULL, {0}, 0};

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

#define RUN_TEST(name) \
    do { \
        g_test_stats.current_test = #name; \
        printf("  [%s] %-50s ... ", g_test_stats.current_suite, #name); \
        fflush(stdout); \
        g_test_stats.total++; \
        g_test_stats.jump_ready = 1; \
        if (setjmp(g_test_stats.jump_buffer) == 0) { \
            test_##name(); \
        } \
        g_test_stats.jump_ready = 0; \
    } while(0)

/*==================================================================================================
*                                      ASSERTION MACROS
==================================================================================================*/
#define TEST_FAIL(message) \
    do { \
        printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
        printf("    %s:%d: %s\n", __FILE__, __LINE__, message); \
        g_test_stats.failed++; \
        if (g_test_stats.jump_ready) longjmp(g_test_stats.jump_buffer, 1); \
        return; \
    } while(0)

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
            printf("    %s:%d: Assertion failed: %s\n", __FILE__, __LINE__, #condition); \
            g_test_stats.failed++; \
            if (g_test_stats.jump_ready) longjmp(g_test_stats.jump_buffer, 1); \
            return; \
        } \
    } while(0)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
            printf("    %s:%d: Expected false: %s\n", __FILE__, __LINE__, #condition); \
            g_test_stats.failed++; \
            if (g_test_stats.jump_ready) longjmp(g_test_stats.jump_buffer, 1); \
            return; \
        } \
    } while(0)

#define ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
            printf("    %s:%d: Expected: %ld, Actual: %ld\n", \
                   __FILE__, __LINE__, (long)(expected), (long)(actual)); \
            g_test_stats.failed++; \
            if (g_test_stats.jump_ready) longjmp(g_test_stats.jump_buffer, 1); \
            return; \
        } \
    } while(0)

#define ASSERT_NE(expected, actual) \
    do { \
        if ((expected) == (actual)) { \
            printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
            printf("    %s:%d: Expected not equal to: %ld\n", \
                   __FILE__, __LINE__, (long)(expected)); \
            g_test_stats.failed++; \
            if (g_test_stats.jump_ready) longjmp(g_test_stats.jump_buffer, 1); \
            return; \
        } \
    } while(0)

#define ASSERT_LT(left, right) \
    do { \
        if (!((left) < (right))) { \
            printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
            printf("    %s:%d: Expected %ld < %ld\n", \
                   __FILE__, __LINE__, (long)(left), (long)(right)); \
            g_test_stats.failed++; \
            if (g_test_stats.jump_ready) longjmp(g_test_stats.jump_buffer, 1); \
            return; \
        } \
    } while(0)

#define ASSERT_LE(left, right) \
    do { \
        if (!((left) <= (right))) { \
            printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
            printf("    %s:%d: Expected %ld <= %ld\n", \
                   __FILE__, __LINE__, (long)(left), (long)(right)); \
            g_test_stats.failed++; \
            if (g_test_stats.jump_ready) longjmp(g_test_stats.jump_buffer, 1); \
            return; \
        } \
    } while(0)

#define ASSERT_GT(left, right) \
    do { \
        if (!((left) > (right))) { \
            printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
            printf("    %s:%d: Expected %ld > %ld\n", \
                   __FILE__, __LINE__, (long)(left), (long)(right)); \
            g_test_stats.failed++; \
            if (g_test_stats.jump_ready) longjmp(g_test_stats.jump_buffer, 1); \
            return; \
        } \
    } while(0)

#define ASSERT_GE(left, right) \
    do { \
        if (!((left) >= (right))) { \
            printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
            printf("    %s:%d: Expected %ld >= %ld\n", \
                   __FILE__, __LINE__, (long)(left), (long)(right)); \
            g_test_stats.failed++; \
            if (g_test_stats.jump_ready) longjmp(g_test_stats.jump_buffer, 1); \
            return; \
        } \
    } while(0)

#define ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
            printf("    %s:%d: Expected NULL, got: %p\n", \
                   __FILE__, __LINE__, (void*)(ptr)); \
            g_test_stats.failed++; \
            if (g_test_stats.jump_ready) longjmp(g_test_stats.jump_buffer, 1); \
            return; \
        } \
    } while(0)

#define ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
            printf("    %s:%d: Expected not NULL\n", __FILE__, __LINE__); \
            g_test_stats.failed++; \
            if (g_test_stats.jump_ready) longjmp(g_test_stats.jump_buffer, 1); \
            return; \
        } \
    } while(0)

#define ASSERT_STR_EQ(expected, actual) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
            printf("    %s:%d: Expected: \"%s\", Actual: \"%s\"\n", \
                   __FILE__, __LINE__, (expected), (actual)); \
            g_test_stats.failed++; \
            if (g_test_stats.jump_ready) longjmp(g_test_stats.jump_buffer, 1); \
            return; \
        } \
    } while(0)

#define ASSERT_STR_NE(expected, actual) \
    do { \
        if (strcmp((expected), (actual)) == 0) { \
            printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
            printf("    %s:%d: Strings should differ\n", __FILE__, __LINE__); \
            g_test_stats.failed++; \
            if (g_test_stats.jump_ready) longjmp(g_test_stats.jump_buffer, 1); \
            return; \
        } \
    } while(0)

#define ASSERT_MEM_EQ(expected, actual, size) \
    do { \
        if (memcmp((expected), (actual), (size)) != 0) { \
            printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
            printf("    %s:%d: Memory content differs\n", __FILE__, __LINE__); \
            g_test_stats.failed++; \
            if (g_test_stats.jump_ready) longjmp(g_test_stats.jump_buffer, 1); \
            return; \
        } \
    } while(0)

#define ASSERT_WITHIN_RANGE(lower, upper, actual) \
    do { \
        if ((actual) < (lower) || (actual) > (upper)) { \
            printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
            printf("    %s:%d: Expected within [%ld, %ld], got: %ld\n", \
                   __FILE__, __LINE__, (long)(lower), (long)(upper), (long)(actual)); \
            g_test_stats.failed++; \
            if (g_test_stats.jump_ready) longjmp(g_test_stats.jump_buffer, 1); \
            return; \
        } \
    } while(0)

/*==================================================================================================
*                                      TEST PASS/FAIL
==================================================================================================*/
#define TEST_PASS() \
    do { \
        printf(TEST_COLOR_GREEN "PASSED" TEST_COLOR_RESET "\n"); \
        g_test_stats.passed++; \
    } while(0)

#define TEST_SKIP(message) \
    do { \
        printf(TEST_COLOR_YELLOW "SKIPPED" TEST_COLOR_RESET "\n"); \
        printf("    %s\n", message); \
        g_test_stats.skipped++; \
    } while(0)

/*==================================================================================================
*                                      MOCK UTILITIES
==================================================================================================*/
#define MOCK_CALL_COUNT(func) mock_##func##_call_count()
#define MOCK_RESET(func) mock_##func##_reset()
#define MOCK_SET_RETURN(func, val) mock_##func##_set_return(val)
#define MOCK_GET_PARAM(func, idx) mock_##func##_get_param(idx)

/*==================================================================================================
*                                      TEST RUNNER MACROS
==================================================================================================*/
#define TEST_MAIN_BEGIN() \
    int main(int argc, char** argv) { \
        (void)argc; (void)argv; \
        printf("\n"); \
        printf("===============================================================\n"); \
        printf("  " TEST_COLOR_CYAN "YuleTech BSW Unit Test Framework" TEST_COLOR_RESET "\n"); \
        printf("  Version: %d.%d.%d\n", \
               TEST_FRAMEWORK_VERSION_MAJOR, \
               TEST_FRAMEWORK_VERSION_MINOR, \
               TEST_FRAMEWORK_VERSION_PATCH); \
        printf("===============================================================\n"); \
        do {

#define TEST_MAIN_END() \
        } while(0); \
        printf("\n===============================================================\n"); \
        printf("  " TEST_COLOR_CYAN "Test Results:" TEST_COLOR_RESET "\n"); \
        printf("    Total:   %d\n", g_test_stats.total); \
        printf("    " TEST_COLOR_GREEN "Passed:  %d" TEST_COLOR_RESET "\n", g_test_stats.passed); \
        printf("    " TEST_COLOR_RED "Failed:  %d" TEST_COLOR_RESET "\n", g_test_stats.failed); \
        if (g_test_stats.skipped > 0) { \
            printf("    " TEST_COLOR_YELLOW "Skipped: %d" TEST_COLOR_RESET "\n", g_test_stats.skipped); \
        } \
        printf("===============================================================\n"); \
        if (g_test_stats.failed == 0 && g_test_stats.passed > 0) { \
            printf("  " TEST_COLOR_GREEN "ALL TESTS PASSED!" TEST_COLOR_RESET "\n\n"); \
        } else if (g_test_stats.failed > 0) { \
            printf("  " TEST_COLOR_RED "SOME TESTS FAILED!" TEST_COLOR_RESET "\n\n"); \
        } else { \
            printf("  " TEST_COLOR_YELLOW "NO TESTS RUN!" TEST_COLOR_RESET "\n\n"); \
        } \
        return (g_test_stats.failed > 0) ? 1 : 0; \
    }

#ifdef __cplusplus
}
#endif

#endif /* TEST_FRAMEWORK_H */
