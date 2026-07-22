/**
 * @file test_framework.h
 * @brief Minimal test framework compatibility stub
 *
 * Provides RUN_TEST and basic assertion macros for code that includes this
 * via the include/autosar/ path. New code should include the canonical
 * test_framework.h at tests/unit/test_framework.h instead.
 *
 * (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
 */

#ifndef AUTOSAR_TEST_FRAMEWORK_H
#define AUTOSAR_TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>

/* Test assertion macros */
#define TEST_ASSERT_EQUAL(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            printf("FAIL: %s:%d - expected %d, got %d\n", __FILE__, __LINE__, (int)(expected), (int)(actual)); \
        } else { \
            printf("PASS: %s:%d\n", __FILE__, __LINE__); \
        } \
    } while (0U)

#define TEST_ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            printf("FAIL: %s:%d\n", __FILE__, __LINE__); \
        } else { \
            printf("PASS: %s:%d\n", __FILE__, __LINE__); \
        } \
    } while (0U)

#define TEST_ASSERT_FALSE(cond) TEST_ASSERT_TRUE(!(cond))

#define TEST_ASSERT_EQUAL_PTR(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            printf("FAIL: %s:%d - pointer mismatch\n", __FILE__, __LINE__); \
        } else { \
            printf("PASS: %s:%d\n", __FILE__, __LINE__); \
        } \
    } while (0U)

#define TEST_SETUP()    printf("--- Test: %s ---\n", __func__)
#define TEST_TEARDOWN() printf("--- End: %s ---\n", __func__)

#define RUN_TEST(func)  do { func(); } while (0U)

#define ASSERT_EQ(expected, actual) TEST_ASSERT_EQUAL(expected, actual)
#define ASSERT_TRUE(cond)           TEST_ASSERT_TRUE(cond)
#define ASSERT_FALSE(cond)          TEST_ASSERT_FALSE(cond)
#define TEST_PASS()                 printf("  PASS: %s\n", __func__)

#define TEST_MAIN_BEGIN()   int main(void)
#define TEST_MAIN_END()     return 0; }

#endif /* AUTOSAR_TEST_FRAMEWORK_H */
