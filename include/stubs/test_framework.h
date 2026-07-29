/**
 * @file test_framework.h
 * @brief Test framework header - stub for compilation
 * 
 * Provides test assertion macros so test source files compile
 * even without the full test framework infrastructure.
 */
#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Test assertion macros */
#define TEST_ASSERT_TRUE(cond) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond); exit(1); } } while(0)

#define TEST_ASSERT_FALSE(cond) \
    TEST_ASSERT_TRUE(!(cond))

#define TEST_ASSERT_EQUAL(expected, actual) \
    TEST_ASSERT_TRUE((expected) == (actual))

#define TEST_ASSERT_NOT_EQUAL(expected, actual) \
    TEST_ASSERT_TRUE((expected) != (actual))

#define TEST_ASSERT_NULL(ptr) \
    TEST_ASSERT_TRUE((ptr) == NULL)

#define TEST_ASSERT_NOT_NULL(ptr) \
    TEST_ASSERT_TRUE((ptr) != NULL)

#define TEST_ASSERT_EQUAL_INT(expected, actual) \
    TEST_ASSERT_TRUE((int)(expected) == (int)(actual))

#define TEST_ASSERT_EQUAL_UINT(expected, actual) \
    TEST_ASSERT_TRUE((unsigned int)(expected) == (unsigned int)(actual))

#define TEST_ASSERT_EQUAL_HEX(expected, actual) \
    TEST_ASSERT_TRUE((unsigned int)(expected) == (unsigned int)(actual))

#define TEST_ASSERT_EQUAL_STRING(expected, actual) \
    TEST_ASSERT_TRUE(strcmp((expected), (actual)) == 0)

#define TEST_ASSERT_EQUAL_MEMORY(expected, actual, size) \
    TEST_ASSERT_TRUE(memcmp((expected), (actual), (size)) == 0)

#define TEST_ASSERT_BIT_HIGH(reg, bit) \
    TEST_ASSERT_TRUE(((reg) & (1u << (bit))) != 0)

#define TEST_ASSERT_BIT_LOW(reg, bit) \
    TEST_ASSERT_TRUE(((reg) & (1u << (bit))) == 0)

#define TEST_ASSERT_BITS_HIGH(mask, reg) \
    TEST_ASSERT_TRUE(((reg) & (mask)) == (mask))

#define TEST_ASSERT_BITS_LOW(mask, reg) \
    TEST_ASSERT_TRUE(((reg) & (mask)) == 0)

#define TEST_ASSERT_BITS_EQUAL(expected, mask, reg) \
    TEST_ASSERT_TRUE(((reg) & (mask)) == (expected))

#define TEST_ASSERT_RANGE(low, high, value) \
    TEST_ASSERT_TRUE((value) >= (low) && (value) <= (high))

#define TEST_ASSERT_WITHIN(delta, expected, actual) \
    TEST_ASSERT_TRUE(((expected) - (delta)) <= (actual) && (actual) <= ((expected) + (delta)))

#define TEST_FAIL(msg) \
    do { fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, msg); exit(1); } while(0)

#define TEST_PASS() ((void)0)

#define TEST_GROUP(name)
#define TEST_SETUP(name)
#define TEST_TEAR_DOWN(name)
#define TEST(name) static void name(void)
#define IGNORE_TEST(name) static void name(void)

#define RUN_TEST_GROUP(name)
#define RUN_TEST(name)

/* Short-form test macros used by some test files */
#define ASSERT_EQ(expected, actual)             TEST_ASSERT_EQUAL(expected, actual)
#define ASSERT_NE(expected, actual)             TEST_ASSERT_NOT_EQUAL(expected, actual)
#define ASSERT_TRUE(cond)                       TEST_ASSERT_TRUE(cond)
#define ASSERT_FALSE(cond)                      TEST_ASSERT_FALSE(cond)
#define ASSERT_NULL(ptr)                        TEST_ASSERT_NULL(ptr)
#define ASSERT_NOT_NULL(ptr)                    TEST_ASSERT_NOT_NULL(ptr)
#define ASSERT_STREQ(expected, actual)          TEST_ASSERT_EQUAL_STRING(expected, actual)
#define ASSERT_STRNE(expected, actual)          do { TEST_ASSERT_TRUE(strcmp((expected),(actual)) != 0); } while(0)
#define ASSERT_GE(val1, val2)                   TEST_ASSERT_TRUE((val1) >= (val2))
#define ASSERT_LE(val1, val2)                   TEST_ASSERT_TRUE((val1) <= (val2))
#define ASSERT_GT(val1, val2)                   TEST_ASSERT_TRUE((val1) > (val2))
#define ASSERT_LT(val1, val2)                   TEST_ASSERT_TRUE((val1) < (val2))
#define ASSERT_IN_RANGE(low, high, value)       TEST_ASSERT_RANGE(low, high, value)

#endif /* TEST_FRAMEWORK_H */
