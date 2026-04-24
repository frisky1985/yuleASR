/* Unity - Simple Unit Testing for C
 * Copyright (c) 2007 Mike Karlesky, Mark VanderVoord, Greg Williams
 * Released under the MIT License
 */

#ifndef UNITY_H
#define UNITY_H

#define UNITY_VERSION_MAJOR 2
#define UNITY_VERSION_MINOR 5
#define UNITY_VERSION_BUILD 2

#include "unity_internals.h"

/* 断言宏定义 */
#define TEST_ASSERT(condition)                                                     UNITY_TEST_ASSERT(                           (condition), __LINE__, " Expression Evaluated To FALSE")
#define TEST_ASSERT_TRUE(condition)                                                UNITY_TEST_ASSERT(                           (condition), __LINE__, " Expected TRUE Was FALSE")
#define TEST_ASSERT_FALSE(condition)                                               UNITY_TEST_ASSERT(                          !(condition), __LINE__, " Expected FALSE Was TRUE")
#define TEST_ASSERT_NULL(pointer)                                                  UNITY_TEST_ASSERT_NULL(                        (pointer), __LINE__, " Expected NULL")
#define TEST_ASSERT_NOT_NULL(pointer)                                              UNITY_TEST_ASSERT_NOT_NULL(                    (pointer), __LINE__, " Expected Non-NULL")
#define TEST_ASSERT_EQUAL(expected, actual)                                        UNITY_TEST_ASSERT_EQUAL_INT((expected), (actual), __LINE__, NULL)
#define TEST_ASSERT_EQUAL_INT(expected, actual)                                    UNITY_TEST_ASSERT_EQUAL_INT((expected), (actual), __LINE__, NULL)
#define TEST_ASSERT_EQUAL_UINT(expected, actual)                                   UNITY_TEST_ASSERT_EQUAL_UINT((expected), (actual), __LINE__, NULL)
#define TEST_ASSERT_EQUAL_HEX(expected, actual)                                    UNITY_TEST_ASSERT_EQUAL_HEX32((expected), (actual), __LINE__, NULL)
#define TEST_ASSERT_EQUAL_STRING(expected, actual)                                 UNITY_TEST_ASSERT_EQUAL_STRING((expected), (actual), __LINE__, NULL)
#define TEST_ASSERT_EQUAL_MEMORY(expected, actual, len)                            UNITY_TEST_ASSERT_EQUAL_MEMORY((expected), (actual), (len), __LINE__, NULL)
#define TEST_ASSERT_EQUAL_INT_ARRAY(expected, actual, num_elements)                UNITY_TEST_ASSERT_EQUAL_INT_ARRAY((expected), (actual), (num_elements), __LINE__, NULL)
#define TEST_ASSERT_EQUAL_UINT_ARRAY(expected, actual, num_elements)               UNITY_TEST_ASSERT_EQUAL_UINT_ARRAY((expected), (actual), (num_elements), __LINE__, NULL)

/* 带消息参数的断言 */
#define TEST_ASSERT_MESSAGE(condition, message)                                    UNITY_TEST_ASSERT(                           (condition), __LINE__, (message))
#define TEST_ASSERT_EQUAL_INT_MESSAGE(expected, actual, message)                   UNITY_TEST_ASSERT_EQUAL_INT((expected), (actual), __LINE__, (message))
#define TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, actual, message)                UNITY_TEST_ASSERT_EQUAL_STRING((expected), (actual), __LINE__, (message))

/* 测试函数定义宏 */
#define TEST_SETUP()    void setUp(void)
#define TEST_TEAR_DOWN() void tearDown(void)
#define TEST(group, name) void test_##group##_##name(void)
#define TEST_IGNORE()    UNITY_TEST_IGNORE(__LINE__, NULL)
#define TEST_IGNORE_MESSAGE(message) UNITY_TEST_IGNORE(__LINE__, (message))

/* 主函数包装器 */
#define RUN_TEST(func) UnityDefaultTestRun(func, #func, __LINE__)

/* 开始和结束测试 */
#define UNITY_BEGIN() UnityBegin(__FILE__)
#define UNITY_END() UnityEnd()

/* 忽略测试 */
void UnityDefaultTestRun(void (*func)(void), const char* name, int line);
void UnityBegin(const char* filename);
int UnityEnd(void);
void setUp(void);
void tearDown(void);

#endif /* UNITY_H */
