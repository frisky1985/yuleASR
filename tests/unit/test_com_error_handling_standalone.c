/******************************************************************************
 * @file    test_com_error_handling_standalone.c
 * @brief   Standalone compilation test for COM Error Handling Module (T013)
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/*==================[Minimal Type Definitions]=============================*/

typedef uint8_t   uint8;
typedef uint16_t  uint16;
typedef uint32_t  uint32;
typedef uint64_t  uint64;
typedef int8_t    sint8;
typedef int16_t   sint16;
typedef int32_t   sint32;
typedef int64_t   sint64;
typedef unsigned int boolean;

#ifndef NULL_PTR
#define NULL_PTR    ((void*)0)
#endif

#define STD_ON      1u
#define STD_OFF     0u
#define E_OK        0u
#define E_NOT_OK    1u
#define TRUE        1u
#define FALSE       0u

/* COM Module IDs */
#define COM_MODULE_ID                   0x1Eu
#define COM_INSTANCE_ID                 0x00u

/* COM Configuration */
#define COM_MAX_IPDUS                   64u
#define COM_MAX_TX_REQUESTS             32u
#define COM_MAX_SIGNALS                 128u
#define COM_MAX_SIGNAL_GROUPS           32u

/* Error Handling Configuration */
#define COM_ERROR_HANDLING_ENABLE       STD_ON
#define COM_ERROR_STATISTICS_ENABLE     STD_ON
#define COM_MAX_ERROR_LOG_ENTRIES       16u
#define COM_ERROR_LOG_WRAP_MODE         STD_ON
#define COM_DEV_ERROR_DETECT            STD_ON

/* Service IDs for error handling */
#define COM_SERVICE_ID_EH_INIT                    0x30u
#define COM_SERVICE_ID_EH_DEINIT                  0x31u
#define COM_SERVICE_ID_EH_REPORT_OVERFLOW         0x32u
#define COM_SERVICE_ID_EH_HANDLE_OVERFLOW         0x33u
#define COM_SERVICE_ID_EH_GET_STATS               0x34u
#define COM_SERVICE_ID_EH_RESET_STATS             0x35u
#define COM_SERVICE_ID_EH_GET_QUEUE_STATUS        0x36u
#define COM_SERVICE_ID_EH_APPLY_STRATEGY          0x37u

/* Error Codes */
#define COM_E_TX_QUEUE_OVERFLOW                   0x40u
#define COM_E_TX_QUEUE_FULL                       0x41u
#define COM_E_INVALID_OVERFLOW_STRATEGY           0x42u
#define COM_E_STATISTICS_CORRUPTION               0x43u
#define COM_E_ERROR_COUNTER_OVERFLOW              0x44u

/* ASIL-D Safety Configuration */
#define COM_ERROR_REDUNDANT_COUNTERS              STD_ON
#define COM_ERROR_CHECKSUM_ENABLE                 STD_ON

/* Version information for Com_ErrorHandling.c */
#define COM_SW_MAJOR_VERSION                      1u
#define COM_SW_MINOR_VERSION                      0u
#define COM_SW_PATCH_VERSION                      0u

/* Report error macro */
#define COM_REPORT_ERROR(ApiId, ErrorId) \
    Det_ReportError(COM_MODULE_ID, COM_INSTANCE_ID, (ApiId), (ErrorId))

/* Minimal COM types */
typedef uint16 Com_SignalIdType;
typedef uint16 Com_SignalGroupIdType;
typedef uint16 Com_IPduIdType;
typedef uint16 Com_IpduGroupIdType;
typedef uint8  Std_ReturnType;

typedef enum {
    COM_TXREQ_IDLE = 0,
    COM_TXREQ_PENDING,
    COM_TXREQ_IN_PROGRESS,
    COM_TXREQ_RETRY,
    COM_TXREQ_COMPLETED,
    COM_TXREQ_FAILED
} Com_TxRequestStateType;

typedef enum {
    COM_TXREQ_SIGNAL = 0,
    COM_TXREQ_SIGNALGROUP,
    COM_TXREQ_TRIGGERED
} Com_TxRequestType;

typedef struct {
    Com_TxRequestStateType State;
    Com_TxRequestType Type;
    Com_IPduIdType PduId;
    Com_SignalIdType SignalId;
    Com_SignalGroupIdType SignalGroupId;
    uint32 Timestamp;
    uint8 RetryCount;
    boolean IsPeriodic;
} Com_TxRequestEntryType;

typedef struct {
    Com_TxRequestEntryType Entries[COM_MAX_TX_REQUESTS];
    uint8 Head;
    uint8 Tail;
    uint8 Count;
    uint32 SequenceCounter;
} Com_TxRequestQueueType;

typedef struct {
    uint32 TotalRequests;
    uint32 SuccessfulTransmissions;
    uint32 FailedTransmissions;
    uint32 RetryAttempts;
    uint32 TimeoutErrors;
    uint32 QueueOverflows;
    uint32 LastErrorTimestamp;
} Com_TxStatisticsType;

/*==================[Mock Global Variables]================================*/

Com_TxRequestQueueType Com_TxRequestQueue;
Com_TxStatisticsType Com_TxStatistics;

/* Mock COM global state */
typedef struct {
    uint8 Status;
    void* Config;
} Com_GlobalMockType;

Com_GlobalMockType Com_GlobalState = {
    .Status = 1,
    .Config = NULL_PTR
};

/*==================[Mock Functions]=======================================*/

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

/* Mock DET function */
uint8 Det_ReportError(uint8 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
    return E_OK;
}

/*==================[Prevent Standard COM Includes]========================*/

/* Prevent inclusion of other COM headers by defining their guards */
#define COM_PRIVATE_H
#define COM_TRANSMIT_H
#define COM_TYPES_H
#define COM_H
#define DET_H

/*==================[Include Error Handling Module Directly]===============*/

/* Include the header */
#include "Com_ErrorHandling.h"

/* Include the implementation */
#include "Com_ErrorHandling.c"

/*==================[Test Configuration]===================================*/

const Com_ErrorHandlingConfigType Com_ErrorHandlingConfig[COM_MAX_IPDUS] = {
    [0] = {
        .OverflowStrategy = COM_TXQUEUE_REJECT_NEWEST,
        .EnableErrorNotification = FALSE,
        .ErrorNotification = NULL_PTR,
        .MaxErrorsBeforeNotification = 0u
    },
    [1] = {
        .OverflowStrategy = COM_TXQUEUE_DROP_OLDEST,
        .EnableErrorNotification = TRUE,
        .ErrorNotification = NULL_PTR,
        .MaxErrorsBeforeNotification = 5u
    },
    [2 ... (COM_MAX_IPDUS - 1)] = {
        .OverflowStrategy = COM_TXQUEUE_REJECT_NEWEST,
        .EnableErrorNotification = FALSE,
        .ErrorNotification = NULL_PTR,
        .MaxErrorsBeforeNotification = 0u
    }
};

/*==================[Simple Test Framework]================================*/

static int tests_run = 0;
static int tests_passed = 0;

#define TEST_ASSERT_EQUAL(expected, actual) \
    do { \
        tests_run++; \
        if ((expected) != (actual)) { \
            printf("  FAIL: Expected %u, got %u at line %d\n", \
                   (unsigned)(expected), (unsigned)(actual), __LINE__); \
            return 1; \
        } \
        tests_passed++; \
    } while(0)

#define TEST_ASSERT_TRUE(expr) \
    do { \
        tests_run++; \
        if (!(expr)) { \
            printf("  FAIL: Expression false at line %d\n", __LINE__); \
            return 1; \
        } \
        tests_passed++; \
    } while(0)

#define RUN_TEST(test_func) \
    do { \
        printf("Running %s...\n", #test_func); \
        if (test_func() == 0) { \
            printf("  PASSED\n"); \
        } else { \
            printf("  FAILED\n"); \
            failed_tests++; \
        } \
    } while(0)

/*==================[Test Functions]=======================================*/

static int test_init(void) {
    Com_Eh_Init();
    TEST_ASSERT_EQUAL(0u, Com_GlobalErrorStats.TxQueueOverflowCount);
    TEST_ASSERT_EQUAL(0u, Com_ErrorLogIndex);
    return 0;
}

static int test_reset_stats(void) {
    Com_GlobalErrorStats.TxQueueOverflowCount = 100u;
    Com_Eh_ResetErrorStats();
    TEST_ASSERT_EQUAL(0u, Com_GlobalErrorStats.TxQueueOverflowCount);
    TEST_ASSERT_EQUAL(0u, Com_GlobalErrorStats.TxQueueRejectCount);
    return 0;
}

static int test_get_overflow_strategy_valid(void) {
    Com_TxQueueOverflowStrategyType strategy = Com_Eh_GetOverflowStrategy(0u);
    TEST_ASSERT_EQUAL(COM_TXQUEUE_REJECT_NEWEST, strategy);

    strategy = Com_Eh_GetOverflowStrategy(1u);
    TEST_ASSERT_EQUAL(COM_TXQUEUE_DROP_OLDEST, strategy);
    return 0;
}

static int test_get_overflow_strategy_invalid(void) {
    Com_TxQueueOverflowStrategyType strategy = Com_Eh_GetOverflowStrategy(COM_MAX_IPDUS + 1u);
    TEST_ASSERT_EQUAL(COM_TXQUEUE_REJECT_NEWEST, strategy);
    return 0;
}

static int test_queue_status(void) {
    Com_TxQueueStatusType status;
    Com_TxQueueInit();

    Std_ReturnType result = Com_Eh_GetTxQueueStatus(&status);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_TRUE(status.IsEmpty);
    TEST_ASSERT_TRUE(!status.IsFull);
    return 0;
}

static int test_queue_status_null(void) {
    Std_ReturnType result = Com_Eh_GetTxQueueStatus(NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
    return 0;
}

static int test_validate_integrity(void) {
    Com_Eh_ResetErrorStats();
    Com_GlobalErrorStats.TxQueueOverflowCount = 5u;
    Com_GlobalErrorStats.TxQueueOverflowCount_Redund = 5u;
    Com_Eh_UpdateStatsChecksum();

    Std_ReturnType result = Com_Eh_ValidateStatsIntegrity();
    TEST_ASSERT_EQUAL(E_OK, result);
    return 0;
}

static int test_detect_corruption(void) {
    Com_Eh_ResetErrorStats();
    Com_GlobalErrorStats.TxQueueOverflowCount = 5u;
    Com_GlobalErrorStats.TxQueueOverflowCount_Redund = 10u;
    Com_Eh_UpdateStatsChecksum();

    Std_ReturnType result = Com_Eh_ValidateStatsIntegrity();
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
    return 0;
}

static int test_error_logging(void) {
    Com_Eh_ResetErrorStats();
    Com_Eh_LogError(0x30u, 0x40u, 0u, COM_TXQUEUE_REJECT_NEWEST);

    TEST_ASSERT_EQUAL(0x30u, Com_ErrorLog[0].ApiId);
    TEST_ASSERT_EQUAL(0x40u, Com_ErrorLog[0].ErrorId);
    TEST_ASSERT_EQUAL(1u, Com_ErrorLogIndex);
    return 0;
}

static int test_get_error_log(void) {
    Com_Eh_ResetErrorStats();
    Com_Eh_LogError(0x30u, 0x40u, 0u, COM_TXQUEUE_REJECT_NEWEST);

    Com_ErrorLogEntryType entry;
    Std_ReturnType result = Com_Eh_GetErrorLogEntry(0u, &entry);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(0x40u, entry.ErrorId);
    return 0;
}

static int test_get_error_log_invalid(void) {
    Com_ErrorLogEntryType entry;
    Std_ReturnType result = Com_Eh_GetErrorLogEntry(COM_MAX_ERROR_LOG_ENTRIES, &entry);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
    return 0;
}

static int test_get_error_stats(void) {
    Com_Eh_ResetErrorStats();
    Com_GlobalErrorStats.TxQueueOverflowCount = 42u;
    Com_GlobalErrorStats.TxQueueOverflowCount_Redund = 42u;
    Com_Eh_UpdateStatsChecksum();

    Com_GlobalErrorStatsType stats;
    Std_ReturnType result = Com_Eh_GetErrorStats(&stats);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL(42u, stats.TxQueueOverflowCount);
    return 0;
}

static int test_get_error_stats_null(void) {
    Std_ReturnType result = Com_Eh_GetErrorStats(NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
    return 0;
}

static int test_overflow_report_counters(void) {
    Com_Eh_ResetErrorStats();

    Com_Eh_ReportTxQueueOverflow(0u, COM_TXQUEUE_REJECT_NEWEST);
    TEST_ASSERT_EQUAL(1u, Com_GlobalErrorStats.TxQueueOverflowCount);
    TEST_ASSERT_EQUAL(1u, Com_GlobalErrorStats.TxQueueOverflowCount_Redund);
    TEST_ASSERT_EQUAL(1u, Com_GlobalErrorStats.TxQueueRejectCount);

    Com_Eh_ReportTxQueueOverflow(0u, COM_TXQUEUE_DROP_OLDEST);
    TEST_ASSERT_EQUAL(2u, Com_GlobalErrorStats.TxQueueOverflowCount);
    TEST_ASSERT_EQUAL(1u, Com_GlobalErrorStats.TxQueueDropOldestCount);
    return 0;
}

/*==================[Main]=================================================*/

int main(void) {
    int failed_tests = 0;

    printf("\n");
    printf("=====================================================\n");
    printf("  COM Error Handling Module Test (T013)\n");
    printf("  ASIL-D Safety Level\n");
    printf("=====================================================\n\n");

    RUN_TEST(test_init);
    RUN_TEST(test_reset_stats);
    RUN_TEST(test_get_overflow_strategy_valid);
    RUN_TEST(test_get_overflow_strategy_invalid);
    RUN_TEST(test_queue_status);
    RUN_TEST(test_queue_status_null);
    RUN_TEST(test_validate_integrity);
    RUN_TEST(test_detect_corruption);
    RUN_TEST(test_error_logging);
    RUN_TEST(test_get_error_log);
    RUN_TEST(test_get_error_log_invalid);
    RUN_TEST(test_get_error_stats);
    RUN_TEST(test_get_error_stats_null);
    RUN_TEST(test_overflow_report_counters);

    printf("\n");
    printf("=====================================================\n");
    printf("  Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("  Failed tests: %d\n", failed_tests);
    printf("=====================================================\n");

    return failed_tests;
}
