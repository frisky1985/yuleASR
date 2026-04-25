/**
 * @file unity_config.h
 * @brief Unity测试框架配置
 * @version 1.0
 * @date 2026-04-25
 */

#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

/*==============================================================================
 * Unity基本配置
 *==============================================================================*/

/* 输出配置 */
#define UNITY_OUTPUT_COLOR
#define UNITY_INCLUDE_PRINT_FORMATTED

/* 浮点数支持 */
#define UNITY_INCLUDE_FLOAT
#define UNITY_INCLUDE_DOUBLE
#define UNITY_FLOAT_PRECISION 0.00001f
#define UNITY_DOUBLE_PRECISION 0.000000001

/* 行号类型 */
#define UNITY_LINE_TYPE int

/* 计数器类型 */
#define UNITY_COUNTER_TYPE int

/* 输出字符函数 */
#ifndef UNITY_OUTPUT_CHAR
    #include <stdio.h>
    #define UNITY_OUTPUT_CHAR(c) putchar(c)
#endif

/* 刷新输出函数 */
#ifndef UNITY_OUTPUT_FLUSH
    #include <stdio.h>
    #define UNITY_OUTPUT_FLUSH() fflush(stdout)
#endif

/*==============================================================================
 * 内存泄漏检测配置
 *==============================================================================*/

#ifdef ENABLE_MEMORY_LEAK_DETECTION
    #define UNITY_MALLOC(size) test_malloc(size)
    #define UNITY_FREE(ptr) test_free(ptr)
    #define UNITY_CALLOC(num, size) test_calloc(num, size)
    #define UNITY_REALLOC(ptr, size) test_realloc(ptr, size)
#else
    #include <stdlib.h>
    #define UNITY_MALLOC(size) malloc(size)
    #define UNITY_FREE(ptr) free(ptr)
    #define UNITY_CALLOC(num, size) calloc(num, size)
    #define UNITY_REALLOC(ptr, size) realloc(ptr, size)
#endif

/*==============================================================================
 * 平台适配配置
 *==============================================================================*/

#ifdef PLATFORM_FREERTOS
    /* FreeRTOS平台适配 */
    #define UNITY_EXCLUDE_SETJMP_H
    #define UNITY_EXCLUDE_MATH_H
    #include "FreeRTOS.h"
    #define UNITY_MALLOC(size) pvPortMalloc(size)
    #define UNITY_FREE(ptr) vPortFree(ptr)
#endif

#ifdef PLATFORM_BAREMETAL
    /* 裸机平台适配 */
    #define UNITY_EXCLUDE_SETJMP_H
    #define UNITY_EXCLUDE_MATH_H
    #define UNITY_EXCLUDE_FLOAT
    #define UNITY_EXCLUDE_DOUBLE
#endif

/*==============================================================================
 * 断言配置
 *==============================================================================*/

/* 最大失败断言数量 */
#define UNITY_MAX_ASSERTIONS 50

/* 字符串截断长度 */
#define UNITY_DISPLAY_RANGE_INT  (255)
#define UNITY_DISPLAY_RANGE_UINT (255u)
#define UNITY_DISPLAY_RANGE_HEX  (255u)
#define UNITY_DISPLAY_RANGE_CHAR (255u)

/* 64位整数支持 */
#define UNITY_SUPPORT_64

/*==============================================================================
 * 测试配置
 *==============================================================================*/

/* 测试超时配置 (毫秒) */
#define UNITY_TEST_TIMEOUT_MS 5000

/* 忽略消息最大长度 */
#define UNITY_IGNORE_MESSAGE_MAX_LENGTH 128

/* 详细输出 */
#define UNITY_VERBOSE

/*==============================================================================
 * 回调函数声明
 *==============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/* 测试开始/结束回调 */
void unity_test_start_callback(const char* name);
void unity_test_end_callback(const char* name, int result);

/* 断言失败回调 */
void unity_assert_failed_callback(const char* message, int line);

/* 内存分配跟踪回调 */
void* test_malloc(size_t size);
void test_free(void* ptr);
void* test_calloc(size_t num, size_t size);
void* test_realloc(void* ptr, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* UNITY_CONFIG_H */
