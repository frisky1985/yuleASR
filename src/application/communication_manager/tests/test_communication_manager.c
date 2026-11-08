/**
 * @file test_communication_manager.c
 * @brief Swc_CommunicationManager host unit tests (main + assert style)
 *
 * Covers init/deinit, state handling, signal send/receive, PDU send/receive,
 * RX/TX processing, timeout detection, statistics and capacity limits.
 * RTE port APIs and Det are mocked in this file.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "Swc_CommunicationManager.h"

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            printf("  FAILED: %s at line %d\n", #cond, __LINE__); \
            return -1; \
        } \
    } while (0)

#define TEST_ASSERT_EQ(a, b) TEST_ASSERT((a) == (b))

static int tests_run = 0;
static int tests_passed = 0;

/* ============================================================================
 * RTE / DET mocks
 * ============================================================================ */
static Swc_PduInfoType mockRxPdu;
static Std_ReturnType mockReadResult;
static Std_ReturnType mockWriteResult;
static uint32 mockTime;

static Swc_PduInfoType lastWrittenPdu;
static int pduWrites;
static int pduWriteErrors;
static Swc_CommStateType lastWrittenState;
static int stateWrites;
static int detCalls;

Std_ReturnType Rte_Read_SWC_COMMUNICATIONMANAGER_PORT_PDU_DATA_R(void* data)
{
    if (mockReadResult != E_OK) { return mockReadResult; }
    *(Swc_PduInfoType*)data = mockRxPdu;
    return E_OK;
}

Std_ReturnType Rte_Write_SWC_COMMUNICATIONMANAGER_PORT_PDU_DATA_P(const void* data)
{
    lastWrittenPdu = *(const Swc_PduInfoType*)data;
    if (mockWriteResult == E_OK) {
        pduWrites++;
    } else {
        pduWriteErrors++;
    }
    return mockWriteResult;
}

Std_ReturnType Rte_Write_SWC_COMMUNICATIONMANAGER_PORT_COMM_STATE_P(const void* data)
{
    lastWrittenState = *(const Swc_CommStateType*)data;
    stateWrites++;
    return E_OK;
}

Std_ReturnType Rte_Write_SWC_COMMUNICATIONMANAGER_PORT_SIGNAL_DATA_P(const void* data)
{
    (void)data;
    return E_OK;
}

uint32 Rte_GetTime(void)
{
    return mockTime;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId,
                               uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    (void)ApiId;
    (void)ErrorId;
    detCalls++;
    return E_OK;
}

/* ============================================================================
 * Helpers
 * ============================================================================ */
static void fresh_init(void)
{
    Swc_CommunicationManager_Deinit();
    Swc_CommunicationManager_Init();
}

static void mock_reset(void)
{
    memset(&mockRxPdu, 0, sizeof(mockRxPdu));
    memset(&lastWrittenPdu, 0, sizeof(lastWrittenPdu));
    mockReadResult = E_OK;
    mockWriteResult = E_OK;
    mockTime = 100U;
    pduWrites = 0;
    pduWriteErrors = 0;
    stateWrites = 0;
    lastWrittenState = COMM_STATE_OFF;
    detCalls = 0;
}

static void make_pdu(Swc_PduInfoType* pdu, uint16 id, uint8 length)
{
    uint8 i;
    memset(pdu, 0, sizeof(*pdu));
    pdu->pduId = id;
    pdu->length = length;
    pdu->timestamp = 1234U;
    for (i = 0; i < 8U; i++) {
        pdu->data[i] = (uint8)(i + 1U);
    }
}

/* ============================================================================
 * Tests
 * ============================================================================ */
static int test_uninitialized_paths(void)
{
    Swc_CommStateType state = COMM_STATE_ACTIVE;
    Swc_PduInfoType pdu;
    Swc_CommStatisticsType stats;
    uint64 value = 0U;

    mock_reset();
    Swc_CommunicationManager_10ms();
    Swc_CommunicationManager_RxProcess();
    Swc_CommunicationManager_TxProcess();
    Swc_CommunicationManager_MainFunction();
    TEST_ASSERT_EQ(stateWrites, 0);

    TEST_ASSERT_EQ(Swc_CommunicationManager_GetState(NULL_PTR), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_CommunicationManager_GetState(&state), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_CommunicationManager_SetState(COMM_STATE_ACTIVE), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_CommunicationManager_SendSignal(1U, 1U), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_CommunicationManager_ReceiveSignal(1U, NULL_PTR), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_CommunicationManager_ReceiveSignal(1U, &value), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_CommunicationManager_SendPdu(NULL_PTR), RTE_E_INVALID);
    make_pdu(&pdu, 1U, 8U);
    TEST_ASSERT_EQ(Swc_CommunicationManager_SendPdu(&pdu), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_CommunicationManager_ReceivePdu(1U, NULL_PTR), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_CommunicationManager_ReceivePdu(1U, &pdu), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_CommunicationManager_GetStatistics(NULL_PTR), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_CommunicationManager_GetStatistics(&stats), RTE_E_UNINIT);
    TEST_ASSERT_EQ(Swc_CommunicationManager_ResetStatistics(), RTE_E_UNINIT);

    Swc_CommunicationManager_Deinit(); /* no-op */
    return 0;
}

static int test_init_state_and_deinit(void)
{
    Swc_CommStateType state = COMM_STATE_OFF;

    mock_reset();
    Swc_CommunicationManager_Init();
    TEST_ASSERT_EQ(detCalls, 1);
    Swc_CommunicationManager_Init();
    TEST_ASSERT_EQ(detCalls, 1);

    TEST_ASSERT_EQ(Swc_CommunicationManager_GetState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, COMM_STATE_INIT);

    TEST_ASSERT_EQ(Swc_CommunicationManager_SetState(COMM_STATE_ACTIVE), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_CommunicationManager_GetState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, COMM_STATE_ACTIVE);
    TEST_ASSERT_EQ(Swc_CommunicationManager_SetState(COMM_STATE_FAULT), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_CommunicationManager_SetState((Swc_CommStateType)99), RTE_E_INVALID);

    Swc_CommunicationManager_Deinit();
    TEST_ASSERT_EQ(Swc_CommunicationManager_GetState(&state), RTE_E_UNINIT);

    Swc_CommunicationManager_Init();
    TEST_ASSERT_EQ(Swc_CommunicationManager_GetState(&state), RTE_E_OK);
    TEST_ASSERT_EQ(state, COMM_STATE_INIT);
    return 0;
}

static int test_send_receive_signal(void)
{
    uint64 value = 0U;
    Swc_CommStatisticsType stats;

    mock_reset();
    fresh_init();

    /* Not active yet */
    TEST_ASSERT_EQ(Swc_CommunicationManager_SendSignal(10U, 1U), RTE_E_NOT_ACTIVE);

    TEST_ASSERT_EQ(Swc_CommunicationManager_SetState(COMM_STATE_ACTIVE), RTE_E_OK);
    mockTime = 50U;
    TEST_ASSERT_EQ(Swc_CommunicationManager_SendSignal(10U, 0x1122334455667788ULL), RTE_E_OK);

    TEST_ASSERT_EQ(Swc_CommunicationManager_ReceiveSignal(10U, &value), RTE_E_OK);
    TEST_ASSERT_EQ(value, 0x1122334455667788ULL);
    TEST_ASSERT_EQ(Swc_CommunicationManager_ReceiveSignal(99U, &value), RTE_E_INVALID);

    /* Timeout after 100ms invalidates the signal */
    mockTime = 200U;
    Swc_CommunicationManager_10ms();
    TEST_ASSERT_EQ(Swc_CommunicationManager_ReceiveSignal(10U, &value), RTE_E_NOT_OK);
    TEST_ASSERT_EQ(Swc_CommunicationManager_GetStatistics(&stats), RTE_E_OK);
    TEST_ASSERT_EQ(stats.timeouts, 1U);
    TEST_ASSERT_EQ(lastWrittenState, COMM_STATE_ACTIVE);
    return 0;
}

static int test_signal_capacity_limit(void)
{
    uint16 id;

    mock_reset();
    fresh_init();
    TEST_ASSERT_EQ(Swc_CommunicationManager_SetState(COMM_STATE_ACTIVE), RTE_E_OK);

    for (id = 0U; id < 100U; id++) {
        TEST_ASSERT_EQ(Swc_CommunicationManager_SendSignal(id, (uint64)id), RTE_E_OK);
    }
    /* Re-sending an existing signal is allowed */
    TEST_ASSERT_EQ(Swc_CommunicationManager_SendSignal(50U, 1U), RTE_E_OK);
    /* 101st distinct signal exceeds COMM_MAX_SIGNALS */
    TEST_ASSERT_EQ(Swc_CommunicationManager_SendSignal(100U, 0U), RTE_E_LIMIT);
    return 0;
}

static int test_send_pdu(void)
{
    Swc_PduInfoType pdu;

    mock_reset();
    fresh_init();

    make_pdu(&pdu, 3U, 8U);
    TEST_ASSERT_EQ(Swc_CommunicationManager_SendPdu(&pdu), RTE_E_NOT_ACTIVE);

    TEST_ASSERT_EQ(Swc_CommunicationManager_SetState(COMM_STATE_ACTIVE), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_CommunicationManager_SendPdu(&pdu), RTE_E_OK);
    TEST_ASSERT_EQ(pduWrites, 1);
    TEST_ASSERT_EQ(lastWrittenPdu.pduId, 3U);

    /* Same pdu id updates the existing entry */
    pdu.length = 6U;
    TEST_ASSERT_EQ(Swc_CommunicationManager_SendPdu(&pdu), RTE_E_OK);

    /* RTE write failure propagates as NOT_OK */
    mockWriteResult = E_NOT_OK;
    TEST_ASSERT_EQ(Swc_CommunicationManager_SendPdu(&pdu), RTE_E_NOT_OK);
    TEST_ASSERT_EQ(pduWriteErrors, 1);

    {
        Swc_CommStatisticsType stats;
        TEST_ASSERT_EQ(Swc_CommunicationManager_GetStatistics(&stats), RTE_E_OK);
        TEST_ASSERT_EQ(stats.txPdus, 2U);
        TEST_ASSERT_EQ(stats.txErrors, 1U);
    }
    return 0;
}

static int test_pdu_capacity_limit(void)
{
    Swc_PduInfoType pdu;
    uint16 id;

    mock_reset();
    fresh_init();
    TEST_ASSERT_EQ(Swc_CommunicationManager_SetState(COMM_STATE_ACTIVE), RTE_E_OK);

    for (id = 0U; id < 50U; id++) {
        make_pdu(&pdu, id, 8U);
        TEST_ASSERT_EQ(Swc_CommunicationManager_SendPdu(&pdu), RTE_E_OK);
    }
    make_pdu(&pdu, 50U, 8U);
    TEST_ASSERT_EQ(Swc_CommunicationManager_SendPdu(&pdu), RTE_E_LIMIT);
    /* Existing PDU can still be updated at capacity */
    make_pdu(&pdu, 0U, 8U);
    TEST_ASSERT_EQ(Swc_CommunicationManager_SendPdu(&pdu), RTE_E_OK);
    return 0;
}

static int test_rx_process(void)
{
    Swc_PduInfoType pdu;
    uint64 value = 0U;
    Swc_CommStatisticsType stats;

    mock_reset();
    fresh_init();

    /* RX runnable does nothing while not ACTIVE */
    make_pdu(&mockRxPdu, 2U, 8U);
    Swc_CommunicationManager_RxProcess();

    TEST_ASSERT_EQ(Swc_CommunicationManager_SetState(COMM_STATE_ACTIVE), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_CommunicationManager_SendSignal(20U, 0U), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_CommunicationManager_SendSignal(25U, 0U), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_CommunicationManager_SendSignal(99U, 0U), RTE_E_OK); /* other pdu */

    mockTime = 500U;
    make_pdu(&mockRxPdu, 2U, 8U);
    Swc_CommunicationManager_RxProcess();

    TEST_ASSERT_EQ(Swc_CommunicationManager_ReceiveSignal(20U, &value), RTE_E_OK);
    TEST_ASSERT_EQ(value, 0x0807060504030201ULL);
    TEST_ASSERT_EQ(Swc_CommunicationManager_ReceiveSignal(25U, &value), RTE_E_OK);
    TEST_ASSERT_EQ(value, 0x0807060504030201ULL);

    TEST_ASSERT_EQ(Swc_CommunicationManager_ReceivePdu(2U, &pdu), RTE_E_OK);
    TEST_ASSERT_EQ(pdu.pduId, 2U);
    TEST_ASSERT_EQ(pdu.length, 8U);
    TEST_ASSERT_EQ(Swc_CommunicationManager_ReceivePdu(7U, &pdu), RTE_E_INVALID);

    /* Update existing RX PDU entry, shorter length limits packing */
    make_pdu(&mockRxPdu, 2U, 3U);
    Swc_CommunicationManager_RxProcess();
    TEST_ASSERT_EQ(Swc_CommunicationManager_ReceiveSignal(20U, &value), RTE_E_OK);
    TEST_ASSERT_EQ(value, 0x0000000000030201ULL);

    /* A second PDU id is added */
    make_pdu(&mockRxPdu, 3U, 8U);
    Swc_CommunicationManager_RxProcess();

    /* Read failure is ignored */
    mockReadResult = E_NOT_OK;
    Swc_CommunicationManager_RxProcess();

    TEST_ASSERT_EQ(Swc_CommunicationManager_GetStatistics(&stats), RTE_E_OK);
    TEST_ASSERT_EQ(stats.rxPdus, 3U);
    TEST_ASSERT_EQ(stats.rxSignals, 4U);
    return 0;
}

static int test_rx_pdu_capacity_limit(void)
{
    Swc_PduInfoType pdu;
    uint16 id;

    mock_reset();
    fresh_init();
    TEST_ASSERT_EQ(Swc_CommunicationManager_SetState(COMM_STATE_ACTIVE), RTE_E_OK);

    for (id = 0U; id < 50U; id++) {
        make_pdu(&mockRxPdu, id, 8U);
        Swc_CommunicationManager_RxProcess();
    }
    /* 51st distinct RX PDU is dropped */
    make_pdu(&mockRxPdu, 50U, 8U);
    Swc_CommunicationManager_RxProcess();
    TEST_ASSERT_EQ(Swc_CommunicationManager_ReceivePdu(50U, &pdu), RTE_E_INVALID);
    TEST_ASSERT_EQ(Swc_CommunicationManager_ReceivePdu(0U, &pdu), RTE_E_OK);
    return 0;
}

static int test_tx_process(void)
{
    Swc_PduInfoType pdu;
    Swc_CommStatisticsType stats;

    mock_reset();
    fresh_init();

    /* TX runnable does nothing while not ACTIVE */
    Swc_CommunicationManager_TxProcess();

    TEST_ASSERT_EQ(Swc_CommunicationManager_SetState(COMM_STATE_ACTIVE), RTE_E_OK);
    make_pdu(&pdu, 4U, 8U);
    TEST_ASSERT_EQ(Swc_CommunicationManager_SendPdu(&pdu), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_CommunicationManager_SendSignal(40U, 7U), RTE_E_OK);

    mockTime = 999U;
    Swc_CommunicationManager_TxProcess();

    TEST_ASSERT_EQ(lastWrittenPdu.pduId, 4U);
    TEST_ASSERT_EQ(lastWrittenPdu.length, 8U);
    TEST_ASSERT_EQ(lastWrittenPdu.data[0], 4U);
    TEST_ASSERT_EQ(lastWrittenPdu.data[7], 11U);
    TEST_ASSERT_EQ(lastWrittenPdu.timestamp, 999U);

    TEST_ASSERT_EQ(Swc_CommunicationManager_GetStatistics(&stats), RTE_E_OK);
    TEST_ASSERT_EQ(stats.txSignals, 1U);

    /* Second run: signal no longer updated, txSignals unchanged */
    Swc_CommunicationManager_TxProcess();
    TEST_ASSERT_EQ(Swc_CommunicationManager_GetStatistics(&stats), RTE_E_OK);
    TEST_ASSERT_EQ(stats.txSignals, 1U);

    /* Write failure during TX processing increments txErrors */
    mockWriteResult = E_NOT_OK;
    Swc_CommunicationManager_TxProcess();
    TEST_ASSERT_EQ(Swc_CommunicationManager_GetStatistics(&stats), RTE_E_OK);
    TEST_ASSERT(stats.txErrors >= 1U);
    return 0;
}

static int test_mainfunction_dispatch(void)
{
    int i;
    Swc_PduInfoType pdu;

    mock_reset();
    fresh_init();
    TEST_ASSERT_EQ(Swc_CommunicationManager_SetState(COMM_STATE_ACTIVE), RTE_E_OK);
    make_pdu(&pdu, 1U, 8U);
    TEST_ASSERT_EQ(Swc_CommunicationManager_SendPdu(&pdu), RTE_E_OK);
    make_pdu(&mockRxPdu, 1U, 8U);

    for (i = 0; i < 10; i++) {
        Swc_CommunicationManager_MainFunction();
    }
    TEST_ASSERT_EQ(stateWrites, 10);
    TEST_ASSERT_EQ(lastWrittenState, COMM_STATE_ACTIVE);

    {
        Swc_CommStatisticsType stats;
        TEST_ASSERT_EQ(Swc_CommunicationManager_GetStatistics(&stats), RTE_E_OK);
        TEST_ASSERT(stats.rxPdus >= 1U);
        TEST_ASSERT(stats.txPdus >= 1U);
    }
    return 0;
}

static int test_statistics_reset(void)
{
    Swc_CommStatisticsType stats;

    mock_reset();
    fresh_init();
    TEST_ASSERT_EQ(Swc_CommunicationManager_SetState(COMM_STATE_ACTIVE), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_CommunicationManager_SendSignal(1U, 1U), RTE_E_OK);
    Swc_CommunicationManager_10ms();
    mockTime = 1000U;
    Swc_CommunicationManager_10ms();

    TEST_ASSERT_EQ(Swc_CommunicationManager_GetStatistics(&stats), RTE_E_OK);
    TEST_ASSERT(stats.timeouts >= 1U);

    TEST_ASSERT_EQ(Swc_CommunicationManager_ResetStatistics(), RTE_E_OK);
    TEST_ASSERT_EQ(Swc_CommunicationManager_GetStatistics(&stats), RTE_E_OK);
    TEST_ASSERT_EQ(stats.timeouts, 0U);
    TEST_ASSERT_EQ(stats.txPdus, 0U);
    TEST_ASSERT_EQ(stats.rxPdus, 0U);
    return 0;
}

/* ============================================================================
 * Runner
 * ============================================================================ */
typedef struct {
    const char* name;
    int (*func)(void);
} TestCaseType;

static const TestCaseType testCases[] = {
    { "uninitialized_paths", test_uninitialized_paths },
    { "init_state_deinit", test_init_state_and_deinit },
    { "send_receive_signal", test_send_receive_signal },
    { "signal_capacity_limit", test_signal_capacity_limit },
    { "send_pdu", test_send_pdu },
    { "pdu_capacity_limit", test_pdu_capacity_limit },
    { "rx_process", test_rx_process },
    { "rx_pdu_capacity_limit", test_rx_pdu_capacity_limit },
    { "tx_process", test_tx_process },
    { "mainfunction_dispatch", test_mainfunction_dispatch },
    { "statistics_reset", test_statistics_reset }
};

int main(void)
{
    size_t i;
    size_t numTests = sizeof(testCases) / sizeof(testCases[0]);

    printf("=== CommunicationManager unit tests ===\n");
    for (i = 0; i < numTests; i++) {
        int result;
        tests_run++;
        printf("[%zu/%zu] %s ... ", i + 1, numTests, testCases[i].name);
        result = testCases[i].func();
        if (result == 0) {
            tests_passed++;
            printf("PASS\n");
        } else {
            printf("FAIL\n");
        }
    }
    printf("=== %d/%d passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
