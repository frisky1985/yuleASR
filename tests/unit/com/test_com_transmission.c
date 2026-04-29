/******************************************************************************
 * @file    test_com_transmission.c
 * @brief   COM Module Unit Tests - Transmission Scheduler (T009)
 * 
 * Test suite for COM transmission scheduler including:
 * - Send request queue management
 * - Com_SendSignal transmission path
 * - Com_InvalidateSignal functionality
 * - Com_TriggerIPDUSend scheduling logic
 * - PduR_COMTransmit integration
 * - ASIL-D safety protections
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include "unity.h"
#include "Com.h"
#include "Com_Transmit.h"
#include "mock_PduR.h"

/*==================[Test Configuration]===================================*/

static uint8 TestIPduBuffer1[8];
static uint8 TestIPduBuffer2[8];
static uint8 TestShadowBuffer[8];

/* Signal configuration for testing */
static const Com_SignalConfigType TestSignals[] = {
    {
        .SignalId = 0,
        .DataPtr = &TestIPduBuffer1[0],
        .BitPosition = 0,
        .BitSize = 16,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT16,
        .TransferProperty = COM_TRIGGERED,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = NULL_PTR
    },
    {
        .SignalId = 1,
        .DataPtr = &TestIPduBuffer1[2],
        .BitPosition = 16,
        .BitSize = 8,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT8,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = NULL_PTR
    },
    {
        .SignalId = 2,
        .DataPtr = &TestIPduBuffer2[0],
        .BitPosition = 0,
        .BitSize = 32,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT32,
        .TransferProperty = COM_PENDING, /* No trigger */
        .ComNotification = NULL_PTR,
        .Timeout = 0,
        .InitValue = NULL_PTR
    }
};

/* Signal group configuration */
static Com_SignalIdType SignalGroup1Signals[] = {0, 1};

static const Com_SignalGroupConfigType TestSignalGroups[] = {
    {
        .SignalGroupId = 0,
        .SignalRefs = SignalGroup1Signals,
        .NumSignals = 2,
        .ShadowBuffer = TestShadowBuffer,
        .ComNotification = NULL_PTR
    }
};

/* I-PDU configuration */
static Com_SignalIdType IPdu1Signals[] = {0, 1};
static Com_SignalIdType IPdu2Signals[] = {2};
static Com_SignalGroupIdType IPdu1SignalGroups[] = {0};

static const Com_IPduConfigType TestIPdus[] = {
    {
        .IPduId = 0,
        .DataPtr = TestIPduBuffer1,
        .Length = 8,
        .Direction = COM_SEND,
        .Type = COM_NORMAL,
        .SignalProcessing = COM_IMMEDIATE,
        .SignalRefs = IPdu1Signals,
        .NumSignals = 2,
        .SignalGroupRefs = IPdu1SignalGroups,
        .NumSignalGroups = 1,
        .TxMode = {
            .Mode = COM_MIXED,
            .Period = 100,
            .RepetitionPeriod = 10,
            .NumRepetitions = 3,
            .TimeOffset = 0
        },
        .IpduGroupRefs = NULL_PTR,
        .NumIpduGroups = 0,
        .Timeout = 1000,
        .ComIPduCallout = NULL_PTR
    },
    {
        .IPduId = 1,
        .DataPtr = TestIPduBuffer2,
        .Length = 8,
        .Direction = COM_SEND,
        .Type = COM_NORMAL,
        .SignalProcessing = COM_DEFERRED,
        .SignalRefs = IPdu2Signals,
        .NumSignals = 1,
        .SignalGroupRefs = NULL_PTR,
        .NumSignalGroups = 0,
        .TxMode = {
            .Mode = COM_PERIODIC,
            .Period = 50,
            .RepetitionPeriod = 0,
            .NumRepetitions = 0,
            .TimeOffset = 0
        },
        .IpduGroupRefs = NULL_PTR,
        .NumIpduGroups = 0,
        .Timeout = 500,
        .ComIPduCallout = NULL_PTR
    }
};

/* Global configuration */
static const Com_ConfigType TestConfig = {
    .Signals = TestSignals,
    .NumSignals = 3,
    .SignalGroups = TestSignalGroups,
    .NumSignalGroups = 1,
    .IPdus = TestIPdus,
    .NumIPdus = 2,
    .IPduGroups = NULL_PTR,
    .NumIPduGroups = 0
};

/*==================[Test Setup]===========================================*/

void setUp(void)
{
    memset(TestIPduBuffer1, 0, sizeof(TestIPduBuffer1));
    memset(TestIPduBuffer2, 0, sizeof(TestIPduBuffer2));
    memset(TestShadowBuffer, 0, sizeof(TestShadowBuffer));
    
    Com_Init(&TestConfig);
    
    /* Start IPdu groups */
    for (uint16 i = 0; i < TestConfig.NumIPdus; i++) {
        Com_GlobalState.IPduRunTime[i].GroupStatus = COM_IPDU_GROUP_STARTED;
    }
}

void tearDown(void)
{
    Com_DeInit();
}

/*==================[Send Request Queue Tests]============================*/

/**
 * @test Test send request queue initialization
 */
void test_tx_queue_init(void)
{
    /* Queue should be initialized to empty */
    TEST_ASSERT_EQUAL_UINT8(0, Com_TxQueueGetFillLevel());
    
    /* Check statistics are zeroed */
    Com_TxStatisticsType stats;
    Com_GetTxStatistics(&stats);
    TEST_ASSERT_EQUAL_UINT32(0, stats.TotalRequests);
}

/**
 * @test Test adding requests to send queue
 */
void test_tx_queue_add_request(void)
{
    Std_ReturnType result;
    
    /* Add signal send request */
    result = Com_TxQueueAddRequest(COM_TXREQ_SIGNAL, 0, 0, 0);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL_UINT8(1, Com_TxQueueGetFillLevel());
    
    /* Add triggered request */
    result = Com_TxQueueAddRequest(COM_TXREQ_TRIGGERED, 1, 0, 0);
    TEST_ASSERT_EQUAL(E_OK, result);
    TEST_ASSERT_EQUAL_UINT8(2, Com_TxQueueGetFillLevel());
}

/**
 * @test Test queue overflow handling
 */
void test_tx_queue_overflow(void)
{
    Std_ReturnType result;
    
    /* Fill queue to capacity */
    for (uint8 i = 0; i < COM_MAX_TX_REQUESTS + 5; i++) {
        result = Com_TxQueueAddRequest(COM_TXREQ_SIGNAL, 0, 0, 0);
    }
    
    /* Should have failed after capacity reached */
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
    
    /* Check overflow was counted */
    Com_TxStatisticsType stats;
    Com_GetTxStatistics(&stats);
    TEST_ASSERT_GREATER_THAN(0, stats.QueueOverflows);
}

/**
 * @test Test clearing queue for specific PDU
 */
void test_tx_queue_clear_for_pdu(void)
{
    /* Add requests for different PDUs */
    Com_TxQueueAddRequest(COM_TXREQ_SIGNAL, 0, 0, 0);
    Com_TxQueueAddRequest(COM_TXREQ_SIGNAL, 1, 2, 0);
    Com_TxQueueAddRequest(COM_TXREQ_SIGNAL, 0, 1, 0);
    
    TEST_ASSERT_EQUAL_UINT8(3, Com_TxQueueGetFillLevel());
    
    /* Clear requests for PDU 0 */
    Com_TxQueueClearForPdu(0);
    
    /* Should have 1 request remaining (for PDU 1) */
    TEST_ASSERT_EQUAL_UINT8(1, Com_TxQueueGetFillLevel());
}

/*==================[Com_SendSignal Tests]=================================*/

/**
 * @test Test sending a uint16 signal
 */
void test_send_signal_uint16_triggered(void)
{
    uint16 value = 0x1234;
    
    /* Setup PduR mock - expect transmission to be triggered */
    PduR_IfTransmit_ExpectAndReturn(0, NULL, E_OK);
    PduR_IfTransmit_IgnoreArg_PduInfoPtr();
    
    uint8 result = Com_SendSignal(0, &value);
    
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    TEST_ASSERT_EQUAL_UINT8(0x34, TestIPduBuffer1[0]);
    TEST_ASSERT_EQUAL_UINT8(0x12, TestIPduBuffer1[1]);
}

/**
 * @test Test sending signal with TRIGGERED_ON_CHANGE property
 */
void test_send_signal_on_change(void)
{
    uint8 value1 = 0xAB;
    uint8 value2 = 0xAB; /* Same value - should not trigger */
    
    /* First send - should trigger */
    PduR_IfTransmit_ExpectAndReturn(0, NULL, E_OK);
    PduR_IfTransmit_IgnoreArg_PduInfoPtr();
    
    uint8 result = Com_SendSignal(1, &value1);
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    
    /* Second send with same value - should NOT trigger */
    result = Com_SendSignal(1, &value2);
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
}

/**
 * @test Test sending signal before initialization
 */
void test_send_signal_before_init(void)
{
    Com_DeInit();
    
    uint16 value = 0x1234;
    uint8 result = Com_SendSignal(0, &value);
    
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

/**
 * @test Test sending signal with invalid ID
 */
void test_send_signal_invalid_id(void)
{
    uint16 value = 0x1234;
    uint8 result = Com_SendSignal(99, &value);
    
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

/**
 * @test Test sending signal with NULL pointer
 */
void test_send_signal_null_pointer(void)
{
    uint8 result = Com_SendSignal(0, NULL_PTR);
    
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

/*==================[Com_TriggerIPDUSend Tests]============================*/

/**
 * @test Test triggering I-PDU send
 */
void test_trigger_ipdu_send(void)
{
    Std_ReturnType result = Com_TriggerIPDUSend(0);
    TEST_ASSERT_EQUAL(E_OK, result);
    
    /* Queue should have the request */
    TEST_ASSERT_EQUAL_UINT8(1, Com_TxQueueGetFillLevel());
}

/**
 * @test Test triggering send for invalid PDU ID
 */
void test_trigger_ipdu_send_invalid_id(void)
{
    Std_ReturnType result = Com_TriggerIPDUSend(99);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * @test Test triggering send for stopped I-PDU group
 */
void test_trigger_ipdu_send_stopped_group(void)
{
    /* Stop the I-PDU group */
    Com_GlobalState.IPduRunTime[0].GroupStatus = COM_IPDU_GROUP_STOPPED;
    
    Std_ReturnType result = Com_TriggerIPDUSend(0);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/*==================[PduR Integration Tests]===============================*/

/**
 * @test Test successful PduR transmission
 */
void test_pdur_transmit_success(void)
{
    uint16 value = 0x5678;
    
    /* Setup PduR mock to return success */
    PduR_IfTransmit_ExpectAndReturn(0, NULL, E_OK);
    PduR_IfTransmit_IgnoreArg_PduInfoPtr();
    
    uint8 result = Com_SendSignal(0, &value);
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    
    /* Verify data was written */
    TEST_ASSERT_EQUAL_UINT8(0x78, TestIPduBuffer1[0]);
    TEST_ASSERT_EQUAL_UINT8(0x56, TestIPduBuffer1[1]);
}

/**
 * @test Test PduR transmission failure
 */
void test_pdur_transmit_failure(void)
{
    uint16 value = 0x1234;
    
    /* Setup PduR mock to return failure */
    PduR_IfTransmit_ExpectAndReturn(0, NULL, E_NOT_OK);
    PduR_IfTransmit_IgnoreArg_PduInfoPtr();
    
    /* Signal should still be written to buffer even if PduR fails */
    uint8 result = Com_SendSignal(0, &value);
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
}

/**
 * @test Test TxConfirmation handling
 */
void test_tx_confirmation_success(void)
{
    /* Simulate successful transmission */
    Com_HandleTxConfirmation(0, E_OK);
    
    /* Verify statistics updated */
    Com_TxStatisticsType stats;
    Com_GetTxStatistics(&stats);
    TEST_ASSERT_EQUAL_UINT32(1, stats.SuccessfulTransmissions);
}

/**
 * @test Test TxConfirmation failure handling
 */
void test_tx_confirmation_failure(void)
{
    /* Simulate failed transmission */
    Com_HandleTxConfirmation(0, E_NOT_OK);
    
    /* Verify statistics updated */
    Com_TxStatisticsType stats;
    Com_GetTxStatistics(&stats);
    TEST_ASSERT_EQUAL_UINT32(1, stats.FailedTransmissions);
}

/*==================[ASIL-D Safety Tests]==================================*/

/**
 * @test Test send signal parameter validation (ASIL-D)
 */
void test_asil_d_send_signal_validation(void)
{
    /* Valid parameters should pass */
    uint16 value = 0x1234;
    Std_ReturnType result = Com_ValidateSendSignalParams(0, &value);
    TEST_ASSERT_EQUAL(E_OK, result);
    
    /* NULL pointer should fail */
    result = Com_ValidateSendSignalParams(0, NULL_PTR);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
    
    /* Invalid signal ID should fail */
    result = Com_ValidateSendSignalParams(99, &value);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/**
 * @test Test queue integrity validation (ASIL-D)
 */
void test_asil_d_queue_integrity(void)
{
    /* Queue should be valid after init */
    Std_ReturnType result = Com_ValidateTxQueueIntegrity();
    TEST_ASSERT_EQUAL(E_OK, result);
}

/**
 * @test Test CRC calculation for redundancy check (ASIL-D)
 */
void test_asil_d_crc_calculation(void)
{
    uint8 testData[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint16 crc = Com_CalculateCRC(testData, sizeof(testData));
    
    /* CRC should be non-zero for non-empty data */
    TEST_ASSERT_NOT_EQUAL(0, crc);
    
    /* Same data should produce same CRC */
    uint16 crc2 = Com_CalculateCRC(testData, sizeof(testData));
    TEST_ASSERT_EQUAL(crc, crc2);
}

/**
 * @test Test data hash calculation (ASIL-D)
 */
void test_asil_d_data_hash(void)
{
    uint8 testData[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint32 hash = Com_CalculateDataHash(testData, sizeof(testData));
    
    /* Hash should be non-zero for non-empty data */
    TEST_ASSERT_NOT_EQUAL(0, hash);
    
    /* Same data should produce same hash */
    uint32 hash2 = Com_CalculateDataHash(testData, sizeof(testData));
    TEST_ASSERT_EQUAL(hash, hash2);
}

/**
 * @test Test I-PDU integrity verification (ASIL-D)
 */
void test_asil_d_ipdu_integrity(void)
{
    /* Valid I-PDU should pass */
    Std_ReturnType result = Com_VerifyIPduIntegrity(0);
    TEST_ASSERT_EQUAL(E_OK, result);
    
    /* Invalid I-PDU ID should fail */
    result = Com_VerifyIPduIntegrity(99);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/*==================[Signal Group Tests]===================================*/

/**
 * @test Test sending signal group
 */
void test_send_signal_group(void)
{
    /* Update shadow buffer */
    uint16 value1 = 0x1234;
    uint8 value2 = 0xAB;
    
    memcpy(&TestShadowBuffer[0], &value1, sizeof(value1));
    TestShadowBuffer[2] = value2;
    
    /* Setup PduR mock */
    PduR_IfTransmit_ExpectAndReturn(0, NULL, E_OK);
    PduR_IfTransmit_IgnoreArg_PduInfoPtr();
    
    /* Send signal group */
    uint8 result = Com_SendSignalGroup(0);
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
}

/*==================[Statistics Tests]=====================================*/

/**
 * @test Test transmission statistics
 */
void test_transmission_statistics(void)
{
    /* Reset statistics */
    Com_ResetTxStatistics();
    
    Com_TxStatisticsType stats;
    Com_GetTxStatistics(&stats);
    TEST_ASSERT_EQUAL_UINT32(0, stats.TotalRequests);
    TEST_ASSERT_EQUAL_UINT32(0, stats.SuccessfulTransmissions);
    
    /* Send a signal */
    uint16 value = 0x1234;
    PduR_IfTransmit_ExpectAndReturn(0, NULL, E_OK);
    PduR_IfTransmit_IgnoreArg_PduInfoPtr();
    Com_SendSignal(0, &value);
    
    /* Check statistics updated */
    Com_GetTxStatistics(&stats);
    TEST_ASSERT_GREATER_THAN(0, stats.TotalRequests);
}

/*==================[Main]=================================================*/

int main(void)
{
    UNITY_BEGIN();
    
    /* Send Request Queue Tests */
    RUN_TEST(test_tx_queue_init);
    RUN_TEST(test_tx_queue_add_request);
    RUN_TEST(test_tx_queue_overflow);
    RUN_TEST(test_tx_queue_clear_for_pdu);
    
    /* Com_SendSignal Tests */
    RUN_TEST(test_send_signal_uint16_triggered);
    RUN_TEST(test_send_signal_on_change);
    RUN_TEST(test_send_signal_before_init);
    RUN_TEST(test_send_signal_invalid_id);
    RUN_TEST(test_send_signal_null_pointer);
    
    /* Com_TriggerIPDUSend Tests */
    RUN_TEST(test_trigger_ipdu_send);
    RUN_TEST(test_trigger_ipdu_send_invalid_id);
    RUN_TEST(test_trigger_ipdu_send_stopped_group);
    
    /* PduR Integration Tests */
    RUN_TEST(test_pdur_transmit_success);
    RUN_TEST(test_pdur_transmit_failure);
    RUN_TEST(test_tx_confirmation_success);
    RUN_TEST(test_tx_confirmation_failure);
    
    /* ASIL-D Safety Tests */
    RUN_TEST(test_asil_d_send_signal_validation);
    RUN_TEST(test_asil_d_queue_integrity);
    RUN_TEST(test_asil_d_crc_calculation);
    RUN_TEST(test_asil_d_data_hash);
    RUN_TEST(test_asil_d_ipdu_integrity);
    
    /* Signal Group Tests */
    RUN_TEST(test_send_signal_group);
    
    /* Statistics Tests */
    RUN_TEST(test_transmission_statistics);
    
    return UNITY_END();
}
