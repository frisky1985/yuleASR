/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : Unit Test Framework
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-19
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>

/* Test result tracking */
typedef struct {
    int total;
    int passed;
    int failed;
    const char* current_suite;
} TestStats;

static TestStats g_test_stats = {0, 0, 0, NULL};

/* Test macros */
#define TEST_SUITE(name) \
    void test_suite_##name(void) { \
        g_test_stats.current_suite = #name; \
        printf("\n=== Test Suite: %s ===\n", #name); \
    }

#define TEST_CASE(name) \
    void test_case_##name(void)

#define RUN_TEST(name) \
    do { \
        printf("  Running: %s ... ", #name); \
        g_test_stats.total++; \
        test_case_##name(); \
    } while(0)

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            printf("FAILED\n"); \
            printf("    Assertion failed: %s\n", #condition); \
            g_test_stats.failed++; \
            return; \
        } \
    } while(0)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            printf("FAILED\n"); \
            printf("    Assertion failed: %s should be false\n", #condition); \
            g_test_stats.failed++; \
            return; \
        } \
    } while(0)

#define ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            printf("FAILED\n"); \
            printf("    Expected: %d, Actual: %d\n", (int)(expected), (int)(actual)); \
            g_test_stats.failed++; \
            return; \
        } \
    } while(0)

#define ASSERT_NE(expected, actual) \
    do { \
        if ((expected) == (actual)) { \
            printf("FAILED\n"); \
            printf("    Expected not equal: %d\n", (int)(expected)); \
            g_test_stats.failed++; \
            return; \
        } \
    } while(0)

#define ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            printf("FAILED\n"); \
            printf("    Expected NULL, got: %p\n", (void*)(ptr)); \
            g_test_stats.failed++; \
            return; \
        } \
    } while(0)

#define ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            printf("FAILED\n"); \
            printf("    Expected not NULL\n"); \
            g_test_stats.failed++; \
            return; \
        } \
    } while(0)

#define ASSERT_STR_EQ(expected, actual) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            printf("FAILED\n"); \
            printf("    Expected: \"%s\", Actual: \"%s\"\n", (expected), (actual)); \
            g_test_stats.failed++; \
            return; \
        } \
    } while(0)

#define TEST_PASS() \
    do { \
        printf("PASSED\n"); \
        g_test_stats.passed++; \
    } while(0)

/* Test runner */
#define TEST_MAIN_BEGIN() \
    int main(void) { \
        printf("\n"); \
        printf("===============================================\n"); \
        printf("  YuleTech BSW Unit Test Framework\n"); \
        printf("===============================================\n");

#define TEST_MAIN_END() \
        printf("\n===============================================\n"); \
        printf("  Test Results:\n"); \
        printf("    Total:  %d\n", g_test_stats.total); \
        printf("    Passed: %d\n", g_test_stats.passed); \
        printf("    Failed: %d\n", g_test_stats.failed); \
        printf("===============================================\n"); \
        return (g_test_stats.failed > 0) ? 1 : 0; \
    }

#endif /* TEST_FRAMEWORK_H */
