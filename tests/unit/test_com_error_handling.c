/******************************************************************************
 * @file    test_com_error_handling.c
 * @brief   Unit Tests for COM Error Handling Module (T013)
 *
 * Test coverage for:
 * - Queue overflow detection
 * - Overflow strategies (DROP_OLDEST/DROP_NEWEST/REJECT)
 * - DET integration
 * - Error statistics
 * - ASIL-D safety checks
 *
 * T013: Error Handling and Queue Overflow Detection
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

/*==================[Includes]=============================================*/

#include "unity.h"
#include "Com_ErrorHandling.h"
#include "Com_Transmit.h"
#include "Com.h"
#include "mock_Det.h"

#include <string.h>

/*==================[Test Setup]===========================================*/

/* Test configuration */
#define TEST_PDU_ID_0           0u
#define TEST_PDU_ID_1           1u
#define TEST_PDU_ID_INVALID     100u

/* Mock global state for testing */
static Com_GlobalType TestComGlobalState;
static Com_SignalRunTimeType TestSignalRunTime[COM_MAX_SIGNALS];
static Com_SignalGroupRunTimeType TestSignalGroupRunTime[COM_MAX_SIGNAL_GROUPS];
static Com_IPduRunTimeType TestIPduRunTime[COM_MAX_IPDUS];

/*==================[Unity Setup/Teardown]=================================*/

void setUp(void)
{
    /* Initialize test state */
    memset(&TestComGlobalState, 0, sizeof(Com_GlobalType));
    memset(TestSignalRunTime, 0, sizeof(TestSignalRunTime));
    memset(TestSignalGroupRunTime, 0, sizeof(TestSignalGroupRunTime));
    memset(TestIPduRunTime, 0, sizeof(TestIPduRunTime));

    /* Setup global state */
    TestComGlobalState.Status = COM_READY;
    TestComGlobalState.Config = NULL_PTR;  /* Will be set per test */
    TestComGlobalState.SignalRunTime = TestSignalRunTime;
    TestComGlobalState.SignalGroupRunTime = TestSignalGroupRunTime;
    TestComGlobalState.IPduRunTime = TestIPduRunTime;
    TestComGlobalState.Initialized = TRUE;

    /* Point global state to test state */
    memcpy(&Com_GlobalState, &TestComGlobalState, sizeof(Com_GlobalType));

    /* Initialize error handling module */
    Com_Eh_Init();
}

void tearDown(void)
{
    /* Deinitialize error handling */
    Com_Eh_DeInit();

    /* Reset mocks */
    Det_ReportError_StopIgnore();
}

/*==================[Test Cases: Initialization]===========================*/

/**
 * @test Test Com_Eh_Init properly initializes all structures
 */
void test_Com_Eh_Init_InitializesAllStructures(void)
{
    /* Pre-condition: Set some non-zero values */
    Com_GlobalErrorStats.TxQueueOverflowCount = 100u;
    Com_ErrorLog[0].ErrorId = 0xFFu;
    Com_ErrorLogIndex = 5u;

    /* Call init */
    Com_Eh_Init();

    /* Verify statistics are reset */
    TEST_ASSERT_EQUAL_UINT32(0u, Com_GlobalErrorStats.TxQueueOverflowCount);
    TEST_ASSERT_EQUAL_UINT32(0u, Com_GlobalErrorStats.TxQueueOverflowCount_Redund);
    TEST_ASSERT_EQUAL_UINT32(0u, Com_GlobalErrorStats.TxQueueRejectCount);
    TEST_ASSERT_EQUAL_UINT32(0u, Com_GlobalErrorStats.TxQueueDropOldestCount);
    TEST_ASSERT_EQUAL_UINT32(0u, Com_GlobalErrorStats.TxQueueDropNewestCount);

    /* Verify error log is cleared */
    for (uint8 i = 0u; i < COM_MAX_ERROR_LOG_ENTRIES; i++) {
        TEST_ASSERT_EQUAL_UINT8(0u, Com_ErrorLog[i].ErrorId);
        TEST_ASSERT_EQUAL_UINT8(0u, Com_ErrorLog[i].ApiId);
    }

    /* Verify log index is reset */
    TEST_ASSERT_EQUAL_UINT8(0u, Com_ErrorLogIndex);
}

/**
 * @test Test Com_Eh_DeInit properly resets all structures
 */
void test_Com_Eh_DeInit_ResetsAllStructures(void)
{
    /* Setup: Simulate some errors */
    Com_GlobalErrorStats.TxQueueOverflowCount = 10u;
    Com_ErrorLogIndex = 3u;

    /* Call deinit */
    Com_Eh_DeInit();

    /* Verify statistics are reset */
    TEST_ASSERT_EQUAL_UINT32(0u, Com_GlobalErrorStats.TxQueueOverflowCount);
    TEST_ASSERT_EQUAL_UINT8(0u, Com_ErrorLogIndex);
}

/*==================[Test Cases: Overflow Strategy]========================*/

/**
 * @test Test GetOverflowStrategy returns correct strategy for valid PduId
 */
void test_Com_Eh_GetOverflowStrategy_ValidPduId(void)
{
    /* Note: Default configuration is COM_TXQUEUE_REJECT_NEWEST */
    Com_TxQueueOverflowStrategyType strategy;

    strategy = Com_Eh_GetOverflowStrategy(TEST_PDU_ID_0);
    TEST_ASSERT_EQUAL(COM_TXQUEUE_REJECT_NEWEST, strategy);

    strategy = Com_Eh_GetOverflowStrategy(TEST_PDU_ID_1);
    TEST_ASSERT_EQUAL(COM_TXQUEUE_REJECT_NEWEST, strategy);
}

/**
 * @test Test GetOverflowStrategy returns default for invalid PduId
 */
void test_Com_Eh_GetOverflowStrategy_InvalidPduId(void)
{
    Com_TxQueueOverflowStrategyType strategy;

    strategy = Com_Eh_GetOverflowStrategy(TEST_PDU_ID_INVALID);
    TEST_ASSERT_EQUAL(COM_TXQUEUE_REJECT_NEWEST, strategy);
}

/**
 * @test Test GetOverflowStrategy handles max boundary
 */
void test_Com_Eh_GetOverflowStrategy_BoundaryPduId(void)
{
    Com_TxQueueOverflowStrategyType strategy;

    /* Test at COM_MAX_IPDUS boundary */
    strategy = Com_Eh_GetOverflowStrategy(COM_MAX_IPDUS);
    TEST_ASSERT_EQUAL(COM_TXQUEUE_REJECT_NEWEST, strategy);
}

/*==================[Test Cases: Overflow Reporting]=======================*/

/**
 * @test Test ReportTxQueueOverflow increments counters correctly
 */
void test_Com_Eh_ReportTxQueueOverflow_IncrementsCounters(void)
{
    /* Setup: Clear counters */
    Com_GlobalErrorStats.TxQueueOverflowCount = 0u;
    Com_GlobalErrorStats.TxQueueOverflowCount_Redund = 0u;

    /* Report overflow */
    Com_Eh_ReportTxQueueOverflow(TEST_PDU_ID_0, COM_TXQUEUE_REJECT_NEWEST);

    /* Verify counters incremented */
    TEST_ASSERT_EQUAL_UINT32(1u, Com_GlobalErrorStats.TxQueueOverflowCount);
    TEST_ASSERT_EQUAL_UINT32(1u, Com_GlobalErrorStats.TxQueueOverflowCount_Redund);
    TEST_ASSERT_EQUAL_UINT32(1u, Com_GlobalErrorStats.TxQueueRejectCount);

    /* Report another overflow */
    Com_Eh_ReportTxQueueOverflow(TEST_PDU_ID_0, COM_TXQUEUE_REJECT_NEWEST);

    /* Verify counters incremented again */
    TEST_ASSERT_EQUAL_UINT32(2u, Com_GlobalErrorStats.TxQueueOverflowCount);
    TEST_ASSERT_EQUAL_UINT32(2u, Com_GlobalErrorStats.TxQueueOverflowCount_Redund);
    TEST_ASSERT_EQUAL_UINT32(2u, Com_GlobalErrorStats.TxQueueRejectCount);
}

/**
 * @test Test ReportTxQueueOverflow with different strategies
 */
void test_Com_Eh_ReportTxQueueOverflow_DifferentStrategies(void)
{
    /* Test DROP_OLDEST strategy */
    Com_Eh_ReportTxQueueOverflow(TEST_PDU_ID_0, COM_TXQUEUE_DROP_OLDEST);
    TEST_ASSERT_EQUAL_UINT32(1u, Com_GlobalErrorStats.TxQueueDropOldestCount);

    /* Reset and test DROP_NEWEST strategy */
    Com_Eh_ResetErrorStats();
    Com_Eh_ReportTxQueueOverflow(TEST_PDU_ID_0, COM_TXQUEUE_DROP_NEWEST);
    TEST_ASSERT_EQUAL_UINT32(1u, Com_GlobalErrorStats.TxQueueDropNewestCount);
}

/**
 * @test Test ReportTxQueueOverflow validates redundant counters
 */
void test_Com_Eh_ReportTxQueueOverflow_ValidatesRedundancy(void)
{
    /* Corrupt the redundant counter */
    Com_GlobalErrorStats.TxQueueOverflowCount = 5u;
    Com_GlobalErrorStats.TxQueueOverflowCount_Redund = 10u;  /* Mismatch! */

    /* Setup DET mock to expect error report */
    Det_ReportError_ExpectAndReturn(
        COM_MODULE_ID,
        COM_INSTANCE_ID,
        COM_SERVICE_ID_EH_REPORT_OVERFLOW,
        COM_E_STATISTICS_CORRUPTION,
        E_OK
    );

    /* Report overflow - should detect corruption */
    Com_Eh_ReportTxQueueOverflow(TEST_PDU_ID_0, COM_TXQUEUE_REJECT_NEWEST);

    /* Counters should be reset */
    TEST_ASSERT_EQUAL_UINT32(1u, Com_GlobalErrorStats.TxQueueOverflowCount);
    TEST_ASSERT_EQUAL_UINT32(1u, Com_GlobalErrorStats.TxQueueOverflowCount_Redund);
}

/*==================[Test Cases: Error Logging]============================*/

/**
 * @test Test LogError records entry correctly
 */
void test_Com_Eh_LogError_RecordsEntry(void)
{
    uint8 testApiId = 0x30u;
    uint8 testErrorId = COM_E_TX_QUEUE_OVERFLOW;

    /* Log an error */
    Com_Eh_LogError(testApiId, testErrorId, TEST_PDU_ID_0, COM_TXQUEUE_REJECT_NEWEST);

    /* Verify entry was recorded */
    TEST_ASSERT_EQUAL_UINT8(COM_MODULE_ID, Com_ErrorLog[0].ModuleId);
    TEST_ASSERT_EQUAL_UINT8(testApiId, Com_ErrorLog[0].ApiId);
    TEST_ASSERT_EQUAL_UINT8(testErrorId, Com_ErrorLog[0].ErrorId);
    TEST_ASSERT_EQUAL_UINT16(TEST_PDU_ID_0, Com_ErrorLog[0].PduId);
    TEST_ASSERT_EQUAL(COM_TXQUEUE_REJECT_NEWEST, Com_ErrorLog[0].Strategy);

    /* Verify index advanced */
    TEST_ASSERT_EQUAL_UINT8(1u, Com_ErrorLogIndex);
}

/**
 * @test Test LogError wraps around when buffer full
 */
void test_Com_Eh_LogError_WrapsAround(void)
{
    /* Fill the entire buffer */
    for (uint8 i = 0u; i < COM_MAX_ERROR_LOG_ENTRIES; i++) {
        Com_Eh_LogError((uint8)(0x30u + i), COM_E_TX_QUEUE_OVERFLOW, TEST_PDU_ID_0, COM_TXQUEUE_REJECT_NEWEST);
    }

    /* Verify index wrapped */
#if (COM_ERROR_LOG_WRAP_MODE == STD_ON)
    TEST_ASSERT_EQUAL_UINT8(0u, Com_ErrorLogIndex);
#else
    TEST_ASSERT_EQUAL_UINT8(COM_MAX_ERROR_LOG_ENTRIES - 1u, Com_ErrorLogIndex);
#endif
}

/**
 * @test Test GetErrorLogEntry retrieves correct entry
 */
void test_Com_Eh_GetErrorLogEntry_RetrievesEntry(void)
{
    Com_ErrorLogEntryType entry;

    /* Log an error first */
    Com_Eh_LogError(0x30u, COM_E_TX_QUEUE_OVERFLOW, TEST_PDU_ID_0, COM_TXQUEUE_DROP_OLDEST);

    /* Retrieve the entry */
    Std_ReturnType result = Com_Eh_GetErrorLogEntry(0u, &entry);

    /* Verify result */
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL_UINT8(0x30u, entry.ApiId);
    TEST_ASSERT_EQUAL_UINT8(COM_E_TX_QUEUE_OVERFLOW, entry.ErrorId);
    TEST_ASSERT_EQUAL(COM_TXQUEUE_DROP_OLDEST, entry.Strategy);
}

/**
 * @test Test GetErrorLogEntry returns error for invalid index
 */
void test_Com_Eh_GetErrorLogEntry_InvalidIndex(void)
{
    Com_ErrorLogEntryType entry;

    /* Try to get entry beyond buffer */
    Std_ReturnType result = Com_Eh_GetErrorLogEntry(COM_MAX_ERROR_LOG_ENTRIES, &entry);

    /* Should return error */
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * @test Test GetErrorLogEntry returns error for NULL pointer
 */
void test_Com_Eh_GetErrorLogEntry_NullPointer(void)
{
    /* Try to get entry with NULL pointer */
    Std_ReturnType result = Com_Eh_GetErrorLogEntry(0u, NULL_PTR);

    /* Should return error */
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/*==================[Test Cases: Statistics]===============================*/

/**
 * @test Test GetErrorStats returns correct data
 */
void test_Com_Eh_GetErrorStats_ReturnsCorrectData(void)
{
    Com_GlobalErrorStatsType stats;

    /* Setup some statistics */
    Com_GlobalErrorStats.TxQueueOverflowCount = 42u;
    Com_GlobalErrorStats.TxQueueOverflowCount_Redund = 42u;
    Com_GlobalErrorStats.TxQueueRejectCount = 10u;
    Com_Eh_UpdateStatsChecksum();

    /* Get statistics */
    Std_ReturnType result = Com_Eh_GetErrorStats(&stats);

    /* Verify result */
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL_UINT32(42u, stats.TxQueueOverflowCount);
    TEST_ASSERT_EQUAL_UINT32(10u, stats.TxQueueRejectCount);
}

/**
 * @test Test GetErrorStats returns error for NULL pointer
 */
void test_Com_Eh_GetErrorStats_NullPointer(void)
{
    Std_ReturnType result = Com_Eh_GetErrorStats(NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * @test Test GetErrorStats detects corruption
 */
void test_Com_Eh_GetErrorStats_DetectsCorruption(void)
{
    Com_GlobalErrorStatsType stats;

    /* Setup valid statistics */
    Com_GlobalErrorStats.TxQueueOverflowCount = 5u;
    Com_GlobalErrorStats.TxQueueOverflowCount_Redund = 5u;
    Com_Eh_UpdateStatsChecksum();

    /* Corrupt the statistics */
    Com_GlobalErrorStats.TxQueueOverflowCount = 10u;

    /* Setup DET mock to expect error report */
    Det_ReportError_ExpectAndReturn(
        COM_MODULE_ID,
        COM_INSTANCE_ID,
        COM_SERVICE_ID_EH_GET_STATS,
        COM_E_STATISTICS_CORRUPTION,
        E_OK
    );

    /* Get statistics - should detect corruption */
    Std_ReturnType result = Com_Eh_GetErrorStats(&stats);

    /* Should return error */
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * @test Test ResetErrorStats clears all counters
 */
void test_Com_Eh_ResetErrorStats_ClearsAll(void)
{
    /* Setup some statistics */
    Com_GlobalErrorStats.TxQueueOverflowCount = 100u;
    Com_GlobalErrorStats.TxQueueRejectCount = 50u;
    Com_GlobalErrorStats.TxQueueDropOldestCount = 25u;
    Com_GlobalErrorStats.TxQueueDropNewestCount = 25u;
    Com_GlobalErrorStats.PerPduOverflowCount[TEST_PDU_ID_0] = 10u;

    /* Reset statistics */
    Com_Eh_ResetErrorStats();

    /* Verify all cleared */
    TEST_ASSERT_EQUAL_UINT32(0u, Com_GlobalErrorStats.TxQueueOverflowCount);
    TEST_ASSERT_EQUAL_UINT32(0u, Com_GlobalErrorStats.TxQueueOverflowCount_Redund);
    TEST_ASSERT_EQUAL_UINT32(0u, Com_GlobalErrorStats.TxQueueRejectCount);
    TEST_ASSERT_EQUAL_UINT32(0u, Com_GlobalErrorStats.TxQueueDropOldestCount);
    TEST_ASSERT_EQUAL_UINT32(0u, Com_GlobalErrorStats.TxQueueDropNewestCount);
    TEST_ASSERT_EQUAL_UINT32(0u, Com_GlobalErrorStats.PerPduOverflowCount[TEST_PDU_ID_0]);
}

/*==================[Test Cases: Queue Status]=============================*/

/**
 * @test Test GetTxQueueStatus returns correct status
 */
void test_Com_Eh_GetTxQueueStatus_ReturnsCorrectStatus(void)
{
    Com_TxQueueStatusType status;

    /* Initialize transmit queue first */
    Com_TxQueueInit();

    /* Get status of empty queue */
    Std_ReturnType result = Com_Eh_GetTxQueueStatus(&status);

    /* Verify result */
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL_UINT8(0u, status.FillLevel);
    TEST_ASSERT_EQUAL_UINT8(COM_MAX_TX_REQUESTS, status.MaxFillLevel);
    TEST_ASSERT_TRUE(status.IsEmpty);
    TEST_ASSERT_FALSE(status.IsFull);
}

/**
 * @test Test GetTxQueueStatus returns error for NULL pointer
 */
void test_Com_Eh_GetTxQueueStatus_NullPointer(void)
{
    Std_ReturnType result = Com_Eh_GetTxQueueStatus(NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/*==================[Test Cases: ASIL-D Safety]============================*/

/**
 * @test Test ValidateStatsIntegrity checks redundant counters
 */
void test_Com_Eh_ValidateStatsIntegrity_ChecksRedundancy(void)
{
    /* Valid state */
    Com_GlobalErrorStats.TxQueueOverflowCount = 5u;
    Com_GlobalErrorStats.TxQueueOverflowCount_Redund = 5u;
    Com_Eh_UpdateStatsChecksum();

    Std_ReturnType result = Com_Eh_ValidateStatsIntegrity();
    TEST_ASSERT_EQUAL(E_OK, result);

    /* Corrupt redundant counter */
    Com_GlobalErrorStats.TxQueueOverflowCount_Redund = 10u;

    result = Com_Eh_ValidateStatsIntegrity();
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * @test Test ValidateStatsIntegrity checks checksum
 */
void test_Com_Eh_ValidateStatsIntegrity_ChecksChecksum(void)
{
    /* Valid state */
    Com_GlobalErrorStats.TxQueueOverflowCount = 5u;
    Com_GlobalErrorStats.TxQueueOverflowCount_Redund = 5u;
    Com_Eh_UpdateStatsChecksum();

    Std_ReturnType result = Com_Eh_ValidateStatsIntegrity();
    TEST_ASSERT_EQUAL(E_OK, result);

    /* Corrupt data without updating checksum */
    Com_GlobalErrorStats.TxQueueOverflowCount = 10u;

    result = Com_Eh_ValidateStatsIntegrity();
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * @test Test ValidateStatsIntegrity checks bounds
 */
void test_Com_Eh_ValidateStatsIntegrity_ChecksBounds(void)
{
    /* Valid bounds */
    Com_GlobalErrorStats.CurrentQueueFillLevel = 5u;
    Com_GlobalErrorStats.MaxQueueFillLevel = COM_MAX_TX_REQUESTS;
    Com_Eh_UpdateStatsChecksum();

    Std_ReturnType result = Com_Eh_ValidateStatsIntegrity();
    TEST_ASSERT_EQUAL(E_OK, result);

    /* Invalid current fill level */
    Com_GlobalErrorStats.CurrentQueueFillLevel = COM_MAX_TX_REQUESTS + 1u;
    Com_Eh_UpdateStatsChecksum();

    result = Com_Eh_ValidateStatsIntegrity();
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * @test Test UpdateStatsChecksum updates correctly
 */
void test_Com_Eh_UpdateStatsChecksum_UpdatesCorrectly(void)
{
    /* Initial checksum */
    Com_GlobalErrorStats.TxQueueOverflowCount = 10u;
    Com_Eh_UpdateStatsChecksum();
    uint16 checksum1 = Com_GlobalErrorStats.StatisticsChecksum;

    /* Change data and update */
    Com_GlobalErrorStats.TxQueueOverflowCount = 20u;
    Com_Eh_UpdateStatsChecksum();
    uint16 checksum2 = Com_GlobalErrorStats.StatisticsChecksum;

    /* Checksums should differ */
    TEST_ASSERT_NOT_EQUAL(checksum1, checksum2);
}

/*==================[Test Cases: Apply Strategy]===========================*/

/**
 * @test Test ApplyOverflowStrategy with invalid strategy returns error
 */
void test_Com_Eh_ApplyOverflowStrategy_InvalidStrategy(void)
{
    /* Setup DET mock */
    Det_ReportError_ExpectAndReturn(
        COM_MODULE_ID,
        COM_INSTANCE_ID,
        COM_SERVICE_ID_EH_APPLY_STRATEGY,
        COM_E_INVALID_OVERFLOW_STRATEGY,
        E_OK
    );

    /* Try to apply invalid strategy */
    Std_ReturnType result = Com_Eh_ApplyOverflowStrategy(TEST_PDU_ID_0, COM_TXQUEUE_NUM_STRATEGIES);

    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * @test Test ApplyOverflowStrategy with REJECT_NEWEST returns NOT_OK
 */
void test_Com_Eh_ApplyOverflowStrategy_RejectNewest(void)
{
    /* REJECT_NEWEST should signal caller that request was rejected */
    Std_ReturnType result = Com_Eh_ApplyOverflowStrategy(TEST_PDU_ID_0, COM_TXQUEUE_REJECT_NEWEST);

    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/*==================[Test Cases: Error Rate]===============================*/

/**
 * @test Test IsErrorRateAcceptable returns TRUE initially
 */
void test_Com_Eh_IsErrorRateAcceptable_ReturnsTrueInitially(void)
{
    boolean result = Com_Eh_IsErrorRateAcceptable();
    TEST_ASSERT_TRUE(result);
}

/*==================[Main]=================================================*/

int main(void)
{
    UNITY_BEGIN();

    /* Initialization tests */
    RUN_TEST(test_Com_Eh_Init_InitializesAllStructures);
    RUN_TEST(test_Com_Eh_DeInit_ResetsAllStructures);

    /* Overflow strategy tests */
    RUN_TEST(test_Com_Eh_GetOverflowStrategy_ValidPduId);
    RUN_TEST(test_Com_Eh_GetOverflowStrategy_InvalidPduId);
    RUN_TEST(test_Com_Eh_GetOverflowStrategy_BoundaryPduId);

    /* Overflow reporting tests */
    RUN_TEST(test_Com_Eh_ReportTxQueueOverflow_IncrementsCounters);
    RUN_TEST(test_Com_Eh_ReportTxQueueOverflow_DifferentStrategies);
    RUN_TEST(test_Com_Eh_ReportTxQueueOverflow_ValidatesRedundancy);

    /* Error logging tests */
    RUN_TEST(test_Com_Eh_LogError_RecordsEntry);
    RUN_TEST(test_Com_Eh_LogError_WrapsAround);
    RUN_TEST(test_Com_Eh_GetErrorLogEntry_RetrievesEntry);
    RUN_TEST(test_Com_Eh_GetErrorLogEntry_InvalidIndex);
    RUN_TEST(test_Com_Eh_GetErrorLogEntry_NullPointer);

    /* Statistics tests */
    RUN_TEST(test_Com_Eh_GetErrorStats_ReturnsCorrectData);
    RUN_TEST(test_Com_Eh_GetErrorStats_NullPointer);
    RUN_TEST(test_Com_Eh_GetErrorStats_DetectsCorruption);
    RUN_TEST(test_Com_Eh_ResetErrorStats_ClearsAll);

    /* Queue status tests */
    RUN_TEST(test_Com_Eh_GetTxQueueStatus_ReturnsCorrectStatus);
    RUN_TEST(test_Com_Eh_GetTxQueueStatus_NullPointer);

    /* ASIL-D Safety tests */
    RUN_TEST(test_Com_Eh_ValidateStatsIntegrity_ChecksRedundancy);
    RUN_TEST(test_Com_Eh_ValidateStatsIntegrity_ChecksChecksum);
    RUN_TEST(test_Com_Eh_ValidateStatsIntegrity_ChecksBounds);
    RUN_TEST(test_Com_Eh_UpdateStatsChecksum_UpdatesCorrectly);

    /* Apply strategy tests */
    RUN_TEST(test_Com_Eh_ApplyOverflowStrategy_InvalidStrategy);
    RUN_TEST(test_Com_Eh_ApplyOverflowStrategy_RejectNewest);

    /* Error rate tests */
    RUN_TEST(test_Com_Eh_IsErrorRateAcceptable_ReturnsTrueInitially);

    return UNITY_END();
}
