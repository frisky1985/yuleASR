/*==================================================================================================
 *                            YULETECH AUTOSAR BSW INTEGRATION TEST FRAMEWORK
 *==================================================================================================
 * FILENAME: integration_test_framework.h
 * AUTOSAR VERSION: R22-11
 *==================================================================================================
 * PROJECT: yuleASR Classic AUTOSAR BSW
 * DESCRIPTION: Integration test framework header for cross-module testing
 *==================================================================================================
 */

#ifndef INTEGRATION_TEST_FRAMEWORK_H
#define INTEGRATION_TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <setjmp.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                      FRAMEWORK VERSION
 *==================================================================================================*/
#define INT_TEST_FRAMEWORK_VERSION_MAJOR    1
#define INT_TEST_FRAMEWORK_VERSION_MINOR    0
#define INT_TEST_FRAMEWORK_VERSION_PATCH    0

/*==================================================================================================
 *                                      COLOR DEFINITIONS
 *==================================================================================================*/
#define TEST_COLOR_RESET                "\033[0m"
#define TEST_COLOR_RED                  "\033[31m"
#define TEST_COLOR_GREEN                "\033[32m"
#define TEST_COLOR_YELLOW               "\033[33m"
#define TEST_COLOR_BLUE                 "\033[34m"
#define TEST_COLOR_MAGENTA              "\033[35m"
#define TEST_COLOR_CYAN                 "\033[36m"
#define TEST_COLOR_WHITE                "\033[37m"
#define TEST_COLOR_BOLD                 "\033[1m"

/*==================================================================================================
 *                                      TEST CONFIGURATION
 *==================================================================================================*/
#define INT_TEST_TIMEOUT_MS             10000u
#define INT_TEST_MAX_RETRIES            3u
#define INT_TEST_BUFFER_SIZE            256u
#define INT_TEST_MAX_STACK_DEPTH        10u

/*==================================================================================================
 *                                      TEST RESULT TYPES
 *==================================================================================================*/
typedef enum {
    INT_TEST_RESULT_NOT_RUN = 0,
    INT_TEST_RESULT_PASS,
    INT_TEST_RESULT_FAIL,
    INT_TEST_RESULT_SKIP,
    INT_TEST_RESULT_TIMEOUT
} IntTestResultType;

/*==================================================================================================
 *                                      TEST METRICS
 *==================================================================================================*/
typedef struct {
    uint32_t executionTimeMs;
    uint32_t cpuCycles;
    uint32_t memoryUsed;
    uint32_t stackDepth;
    uint32_t interruptLatency;
} IntTestMetricsType;

/*==================================================================================================
 *                                      TEST CONTEXT
 *==================================================================================================*/
typedef struct {
    const char* testName;
    const char* suiteName;
    IntTestResultType result;
    IntTestMetricsType metrics;
    char errorMessage[256];
    uint32_t retryCount;
    void* userData;
} IntTestContextType;

/*==================================================================================================
 *                                      TEST STATISTICS
 *==================================================================================================*/
typedef struct {
    uint32_t totalTests;
    uint32_t passed;
    uint32_t failed;
    uint32_t skipped;
    uint32_t timeouts;
    uint32_t totalExecutionTimeMs;
    clock_t startTime;
} IntTestStatsType;

/*==================================================================================================
 *                                      STACK TEST INFO
 *==================================================================================================*/
typedef struct {
    const char* stackName;
    const char* modules[INT_TEST_MAX_STACK_DEPTH];
    uint32_t moduleCount;
    void (*initFunc)(void);
    void (*deinitFunc)(void);
} IntStackInfoType;

/*==================================================================================================
 *                                      GLOBAL VARIABLES
 *==================================================================================================*/
extern IntTestStatsType g_intTestStats;
extern IntTestContextType g_intTestContext;
extern jmp_buf g_intTestJumpBuffer;
extern int g_intTestJumpReady;

/*==================================================================================================
 *                                      STACK REGISTRATION
 *==================================================================================================*/
#define INT_TEST_REGISTER_STACK(name, ...) \
    static const char* _stack_modules_##name[] = { __VA_ARGS__ }; \
    static IntStackInfoType _stack_info_##name = { \
        .stackName = #name, \
        .modules = { __VA_ARGS__ }, \
        .moduleCount = sizeof(_stack_modules_##name)/sizeof(char*), \
        .initFunc = NULL, \
        .deinitFunc = NULL \
    }

/*==================================================================================================
 *                                      TEST SUITE MACROS
 *==================================================================================================*/
#define INT_TEST_SUITE_BEGIN(name) \
    void int_test_suite_##name(void) { \
        const char* _current_suite = #name; \
        g_intTestContext.suiteName = #name; \
        printf("\n" TEST_COLOR_CYAN TEST_COLOR_BOLD "=== Integration Test Suite: %s ===" TEST_COLOR_RESET "\n", #name); \
        printf("  Stack: %s\n", _get_stack_name_for_suite(#name)); \
        do

#define INT_TEST_SUITE_END() \
        while(0); \
    }

#define INT_TEST_SETUP(name) \
    void int_test_setup_##name(void)

#define INT_TEST_TEARDOWN(name) \
    void int_test_teardown_##name(void)

#define INT_TEST_RUN_SUITE(name) \
    do { \
        int_test_setup_##name(); \
        int_test_suite_##name(); \
        int_test_teardown_##name(); \
    } while(0)

/*==================================================================================================
 *                                      TEST CASE MACROS
 *==================================================================================================*/
#define INT_TEST_CASE(name) \
    void int_test_case_##name(void)

#define INT_TEST_RUN(name) \
    do { \
        g_intTestContext.testName = #name; \
        g_intTestContext.result = INT_TEST_RESULT_NOT_RUN; \
        memset(&g_intTestContext.metrics, 0, sizeof(IntTestMetricsType)); \
        printf("  [%s] %-50s ... ", g_intTestContext.suiteName, #name); \
        fflush(stdout); \
        g_intTestStats.totalTests++; \
        clock_t _test_start = clock(); \
        g_intTestJumpReady = 1; \
        if (setjmp(g_intTestJumpBuffer) == 0) { \
            int_test_case_##name(); \
        } \
        g_intTestJumpReady = 0; \
        g_intTestContext.metrics.executionTimeMs = (clock() - _test_start) * 1000 / CLOCKS_PER_SEC; \
        g_intTestStats.totalExecutionTimeMs += g_intTestContext.metrics.executionTimeMs; \
    } while(0)

/*==================================================================================================
 *                                      ASSERTION MACROS
 *==================================================================================================*/
#define INT_TEST_FAIL(message) \
    do { \
        snprintf(g_intTestContext.errorMessage, sizeof(g_intTestContext.errorMessage), "%s", message); \
        printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
        printf("    %s:%d: %s\n", __FILE__, __LINE__, message); \
        g_intTestContext.result = INT_TEST_RESULT_FAIL; \
        g_intTestStats.failed++; \
        if (g_intTestJumpReady) longjmp(g_intTestJumpBuffer, 1); \
        return; \
    } while(0)

#define INT_ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
            printf("    %s:%d: Assertion failed: %s\n", __FILE__, __LINE__, #condition); \
            g_intTestContext.result = INT_TEST_RESULT_FAIL; \
            g_intTestStats.failed++; \
            if (g_intTestJumpReady) longjmp(g_intTestJumpBuffer, 1); \
            return; \
        } \
    } while(0)

#define INT_ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
            printf("    %s:%d: Expected false: %s\n", __FILE__, __LINE__, #condition); \
            g_intTestContext.result = INT_TEST_RESULT_FAIL; \
            g_intTestStats.failed++; \
            if (g_intTestJumpReady) longjmp(g_intTestJumpBuffer, 1); \
            return; \
        } \
    } while(0)

#define INT_ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
            printf("    %s:%d: Expected: %ld, Actual: %ld\n", \
                   __FILE__, __LINE__, (long)(expected), (long)(actual)); \
            g_intTestContext.result = INT_TEST_RESULT_FAIL; \
            g_intTestStats.failed++; \
            if (g_intTestJumpReady) longjmp(g_intTestJumpBuffer, 1); \
            return; \
        } \
    } while(0)

#define INT_ASSERT_NE(expected, actual) \
    do { \
        if ((expected) == (actual)) { \
            printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
            printf("    %s:%d: Expected not equal to: %ld\n", \
                   __FILE__, __LINE__, (long)(expected)); \
            g_intTestContext.result = INT_TEST_RESULT_FAIL; \
            g_intTestStats.failed++; \
            if (g_intTestJumpReady) longjmp(g_intTestJumpBuffer, 1); \
            return; \
        } \
    } while(0)

#define INT_ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
            printf("    %s:%d: Expected NULL, got: %p\n", \
                   __FILE__, __LINE__, (void*)(ptr)); \
            g_intTestContext.result = INT_TEST_RESULT_FAIL; \
            g_intTestStats.failed++; \
            if (g_intTestJumpReady) longjmp(g_intTestJumpBuffer, 1); \
            return; \
        } \
    } while(0)

#define INT_ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
            printf("    %s:%d: Expected not NULL\n", __FILE__, __LINE__); \
            g_intTestContext.result = INT_TEST_RESULT_FAIL; \
            g_intTestStats.failed++; \
            if (g_intTestJumpReady) longjmp(g_intTestJumpBuffer, 1); \
            return; \
        } \
    } while(0)

#define INT_ASSERT_MEM_EQ(expected, actual, size) \
    do { \
        if (memcmp((expected), (actual), (size)) != 0) { \
            printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
            printf("    %s:%d: Memory content differs\n", __FILE__, __LINE__); \
            g_intTestContext.result = INT_TEST_RESULT_FAIL; \
            g_intTestStats.failed++; \
            if (g_intTestJumpReady) longjmp(g_intTestJumpBuffer, 1); \
            return; \
        } \
    } while(0)

#define INT_ASSERT_WITHIN_RANGE(lower, upper, actual) \
    do { \
        if ((actual) < (lower) || (actual) > (upper)) { \
            printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
            printf("    %s:%d: Expected within [%ld, %ld], got: %ld\n", \
                   __FILE__, __LINE__, (long)(lower), (long)(upper), (long)(actual)); \
            g_intTestContext.result = INT_TEST_RESULT_FAIL; \
            g_intTestStats.failed++; \
            if (g_intTestJumpReady) longjmp(g_intTestJumpBuffer, 1); \
            return; \
        } \
    } while(0)

/*==================================================================================================
 *                                      DATA FLOW ASSERTIONS
 *==================================================================================================*/
#define INT_ASSERT_DATA_FLOW(srcModule, dstModule, data, size) \
    do { \
        bool _flowOk = IntTest_VerifyDataFlow(srcModule, dstModule, data, size); \
        if (!_flowOk) { \
            printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
            printf("    %s:%d: Data flow from %s to %s failed\n", \
                   __FILE__, __LINE__, srcModule, dstModule); \
            g_intTestContext.result = INT_TEST_RESULT_FAIL; \
            g_intTestStats.failed++; \
            if (g_intTestJumpReady) longjmp(g_intTestJumpBuffer, 1); \
            return; \
        } \
    } while(0)

#define INT_ASSERT_TIMING(maxMs) \
    do { \
        if (g_intTestContext.metrics.executionTimeMs > (maxMs)) { \
            printf(TEST_COLOR_RED "FAILED" TEST_COLOR_RESET "\n"); \
            printf("    %s:%d: Execution time %ldms exceeds limit %ldms\n", \
                   __FILE__, __LINE__, (long)g_intTestContext.metrics.executionTimeMs, (long)(maxMs)); \
            g_intTestContext.result = INT_TEST_RESULT_FAIL; \
            g_intTestStats.failed++; \
            if (g_intTestJumpReady) longjmp(g_intTestJumpBuffer, 1); \
            return; \
        } \
    } while(0)

/*==================================================================================================
 *                                      PASS/SKIP MACROS
 *==================================================================================================*/
#define INT_TEST_PASS() \
    do { \
        if (g_intTestContext.result == INT_TEST_RESULT_NOT_RUN) { \
            printf(TEST_COLOR_GREEN "PASSED" TEST_COLOR_RESET " (%ld ms)\n", \
                   (long)g_intTestContext.metrics.executionTimeMs); \
            g_intTestContext.result = INT_TEST_RESULT_PASS; \
            g_intTestStats.passed++; \
        } \
    } while(0)

#define INT_TEST_SKIP(message) \
    do { \
        printf(TEST_COLOR_YELLOW "SKIPPED" TEST_COLOR_RESET "\n"); \
        printf("    %s\n", message); \
        g_intTestContext.result = INT_TEST_RESULT_SKIP; \
        g_intTestStats.skipped++; \
    } while(0)

/*==================================================================================================
 *                                      UTILITY FUNCTIONS
 *==================================================================================================*/
const char* IntTest_GetResultString(IntTestResultType result);
void IntTest_PrintSummary(void);
void IntTest_ResetStats(void);
bool IntTest_VerifyDataFlow(const char* srcModule, const char* dstModule, const void* data, uint32_t size);
const char* _get_stack_name_for_suite(const char* suiteName);

/*==================================================================================================
 *                                      MAIN MACRO
 *==================================================================================================*/
#define INT_TEST_MAIN_BEGIN() \
    IntTestStatsType g_intTestStats = {0}; \
    IntTestContextType g_intTestContext = {0}; \
    jmp_buf g_intTestJumpBuffer; \
    int g_intTestJumpReady = 0; \
    int main(int argc, char** argv) { \
        (void)argc; (void)argv; \
        printf("\n"); \
        printf("===============================================================\n"); \
        printf("  " TEST_COLOR_CYAN TEST_COLOR_BOLD "YuleTech BSW Integration Test Framework" TEST_COLOR_RESET "\n"); \
        printf("  Version: %d.%d.%d\n", \
               INT_TEST_FRAMEWORK_VERSION_MAJOR, \
               INT_TEST_FRAMEWORK_VERSION_MINOR, \
               INT_TEST_FRAMEWORK_VERSION_PATCH); \
        printf("===============================================================\n"); \
        IntTest_ResetStats(); \
        do {

#define INT_TEST_MAIN_END() \
        } while(0); \
        IntTest_PrintSummary(); \
        return (g_intTestStats.failed > 0) ? 1 : 0; \
    }

#ifdef __cplusplus
}
#endif

#endif /* INTEGRATION_TEST_FRAMEWORK_H */
