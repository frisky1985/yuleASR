/******************************************************************************
 * @file    test_com_error_handling_simple.c
 * @brief   Simple compilation test for COM Error Handling Module (T013)
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Minimal type definitions for testing */
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t sint8;
typedef int16_t sint16;
typedef int32_t sint32;
typedef int64_t sint64;
typedef unsigned int boolean;

#define STD_ON      1u
#define STD_OFF     0u
#define E_OK        0u
#define E_NOT_OK    1u
#define NULL_PTR    ((void*)0)
#define TRUE        1u
#define FALSE       0u

/* Module and error IDs */
#define COM_MODULE_ID           0x1Eu
#define COM_INSTANCE_ID         0x00u
#define COM_MAX_IPDUS           64u
#define COM_MAX_TX_REQUESTS     32u
#define COM_MAX_SIGNALS         128u
#define COM_MAX_SIGNAL_GROUPS   32u

/* Error handling config */
#define COM_ERROR_HANDLING_ENABLE       STD_ON
#define COM_ERROR_STATISTICS_ENABLE     STD_ON
#define COM_MAX_ERROR_LOG_ENTRIES       16u
#define COM_ERROR_LOG_WRAP_MODE         STD_ON

/* Include the error handling header */
#include "Com_ErrorHandling.h"

/* Include the implementation for testing */
#include "Com_ErrorHandling.c"

/* Simple test framework */
#define TEST_ASSERT_EQUAL(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            printf("FAIL: Expected %u, got %u at line %d\n", \
                   (unsigned)(expected), (unsigned)(actual), __LINE__); \
            return 1; \
        } \
    } while(0)

#define TEST_ASSERT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            printf("FAIL: Expression false at line %d\n", __LINE__); \
            return 1; \
        } \
    } while(0)

/* Mock COM global state */
typedef struct {
    uint8 Status;
    void* Config;
} Com_GlobalMockType;

Com_GlobalMockType Com_GlobalState = {
    .Status = 1,  /* COM_READY */
    .Config = NULL_PTR
};

/* Mock Tx queue */
Com_TxRequestQueueType Com_TxRequestQueue;
Com_TxStatisticsType Com_TxStatistics;
typedef struct { int dummy; } Com_IPduTxContextType;
Com_IPduTxContextType Com_IPduTxContexts[COM_MAX_IPDUS];

/* Error handling configuration */
const Com_ErrorHandlingConfigType Com_ErrorHandlingConfig[COM_MAX_IPDUS] = {
    [0] = {
        .OverflowStrategy = COM_TXQUEUE_REJECT_NEWEST,
        .EnableErrorNotification = FALSE,
        .ErrorNotification = NULL_PTR,
        .MaxErrorsBeforeNotification = 0u
    },
    [1 ... (COM_MAX_IPDUS - 1)] = {
        .OverflowStrategy = COM_TXQUEUE_REJECT_NEWEST,
        .EnableErrorNotification = FALSE,
        .ErrorNotification = NULL_PTR,
        .MaxErrorsBeforeNotification = 0u
    }
};

/* Mock functions */
uint32 Com_GetCurrentTimestamp(void) {
    static uint32 counter = 0;
    return ++counter;
}

uint8 Com_TxQueueGetFillLevel(void) {
    return Com_TxRequestQueue.Count;
}

void Com_TxQueueInit(void) {
    memset(&Com_TxRequestQueue, 0, sizeof(Com_TxRequestQueue));
}

/* Simple test main */
int main(void) {
    printf("=== COM Error Handling Module Compilation Test (T013) ===\n\n");

    /* Test 1: Module initialization */
    printf("Test 1: Module initialization...\n");
    Com_Eh_Init();
    TEST_ASSERT_EQUAL(0u, Com_GlobalErrorStats.TxQueueOverflowCount);
    TEST_ASSERT_EQUAL(0u, Com_ErrorLogIndex);
    printf("  PASSED\n");

    /* Test 2: Reset statistics */
    printf("Test 2: Reset statistics...\n");
    Com_GlobalErrorStats.TxQueueOverflowCount = 100u;
    Com_Eh_ResetErrorStats();
    TEST_ASSERT_EQUAL(0u, Com_GlobalErrorStats.TxQueueOverflowCount);
    printf("  PASSED\n");

    /* Test 3: Get overflow strategy */
    printf("Test 3: Get overflow strategy...\n");
    Com_TxQueueOverflowStrategyType strategy = Com_Eh_GetOverflowStrategy(0u);
    TEST_ASSERT_EQUAL(COM_TXQUEUE_REJECT_NEWEST, strategy);
    printf("  PASSED\n");

    /* Test 4: Invalid PduId returns default strategy */
    printf("Test 4: Invalid PduId returns default...\n");
    strategy = Com_Eh_GetOverflowStrategy(COM_MAX_IPDUS + 1u);
    TEST_ASSERT_EQUAL(COM_TXQUEUE_REJECT_NEWEST, strategy);
    printf("  PASSED\n");

    /* Test 5: Get Tx queue status */
    printf("Test 5: Get Tx queue status...\n");
    Com_TxQueueStatusType status;
    Com_TxQueueInit();
    Std_ReturnType result = Com_Eh_GetTxQueueStatus(&status);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_TRUE(status.IsEmpty);
    printf("  PASSED\n");

    /* Test 6: NULL pointer returns error */
    printf("Test 6: NULL pointer returns error...\n");
    result = Com_Eh_GetTxQueueStatus(NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
    printf("  PASSED\n");

    /* Test 7: Validate statistics integrity */
    printf("Test 7: Validate statistics integrity...\n");
    Com_Eh_ResetErrorStats();
    Com_GlobalErrorStats.TxQueueOverflowCount = 5u;
    Com_GlobalErrorStats.TxQueueOverflowCount_Redund = 5u;
    Com_Eh_UpdateStatsChecksum();
    result = Com_Eh_ValidateStatsIntegrity();
    TEST_ASSERT_EQUAL(E_OK, result);
    printf("  PASSED\n");

    /* Test 8: Detect corruption */
    printf("Test 8: Detect corruption...\n");
    Com_GlobalErrorStats.TxQueueOverflowCount_Redund = 10u;  /* Corrupt */
    result = Com_Eh_ValidateStatsIntegrity();
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
    printf("  PASSED\n");

    /* Test 9: Error log functionality */
    printf("Test 9: Error log functionality...\n");
    Com_Eh_ResetErrorStats();
    Com_Eh_LogError(0x30u, 0x40u, 0u, COM_TXQUEUE_REJECT_NEWEST);
    TEST_ASSERT_EQUAL(0x30u, Com_ErrorLog[0].ApiId);
    TEST_ASSERT_EQUAL(0x40u, Com_ErrorLog[0].ErrorId);
    TEST_ASSERT_EQUAL(1u, Com_ErrorLogIndex);
    printf("  PASSED\n");

    /* Test 10: Get error log entry */
    printf("Test 10: Get error log entry...\n");
    Com_ErrorLogEntryType entry;
    result = Com_Eh_GetErrorLogEntry(0u, &entry);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(0x40u, entry.ErrorId);
    printf("  PASSED\n");

    /* Test 11: Invalid log index returns error */
    printf("Test 11: Invalid log index returns error...\n");
    result = Com_Eh_GetErrorLogEntry(COM_MAX_ERROR_LOG_ENTRIES, &entry);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
    printf("  PASSED\n");

    /* Test 12: Get error stats */
    printf("Test 12: Get error stats...\n");
    Com_GlobalErrorStatsType stats;
    Com_Eh_ResetErrorStats();
    Com_GlobalErrorStats.TxQueueOverflowCount = 42u;
    Com_GlobalErrorStats.TxQueueOverflowCount_Redund = 42u;
    Com_Eh_UpdateStatsChecksum();
    result = Com_Eh_GetErrorStats(&stats);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(42u, stats.TxQueueOverflowCount);
    printf("  PASSED\n");

    printf("\n=== All tests PASSED! ===\n");
    return 0;
}
