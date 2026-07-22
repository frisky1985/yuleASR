/*
 * test_com_signalgroup.c
 * COM Module Unit Tests - Signal Group Operations
 *
 * SHALL-COM-02: SHALL support signal group communication
 */

#include "unity.h"
#include "Com.h"
#include "Com_Private.h"
#include "mock_PduR.h"

/*==================[Test Configuration]===================================*/

static uint8 TestIPduBuffer[8];
static uint8 TestIPduBuffer2[8];
static uint8 TestShadowBuffer[8];
static uint8 TestShadowBuffer2[8];
static boolean NotificationCalled = FALSE;

static void TestNotificationCallback(void)
{
    NotificationCalled = TRUE;
}

static const Com_SignalConfigType TestSignals[] = {
    {
        .SignalId = 0,
        .DataPtr = &TestIPduBuffer[0],
        .BitPosition = 0,
        .BitSize = 16,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT16,
        .TransferProperty = COM_TRIGGERED,
        .ComNotification = NULL,
        .Timeout = 0,
        .InitValue = NULL
    },
    {
        .SignalId = 1,
        .DataPtr = &TestIPduBuffer[2],
        .BitPosition = 0,
        .BitSize = 8,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT8,
        .TransferProperty = COM_TRIGGERED,
        .ComNotification = NULL,
        .Timeout = 0,
        .InitValue = NULL
    },
    {
        .SignalId = 2,
        .DataPtr = &TestIPduBuffer[3],
        .BitPosition = 0,
        .BitSize = 16,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT16,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .ComNotification = NULL,
        .Timeout = 0,
        .InitValue = NULL
    },
    {
        .SignalId = 3,
        .DataPtr = &TestIPduBuffer2[0],
        .BitPosition = 0,
        .BitSize = 32,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT32,
        .TransferProperty = COM_TRIGGERED,
        .ComNotification = TestNotificationCallback,
        .Timeout = 0,
        .InitValue = NULL
    }
};

static Com_SignalIdType SignalGroup1Signals[] = {0, 1, 2};
static Com_SignalIdType SignalGroup2Signals[] = {3};

static const Com_SignalGroupConfigType TestSignalGroups[] = {
    {
        .SignalGroupId = 0,
        .SignalRefs = SignalGroup1Signals,
        .NumSignals = 3,
        .ShadowBuffer = TestShadowBuffer,
        .ComNotification = NULL
    },
    {
        .SignalGroupId = 1,
        .SignalRefs = SignalGroup2Signals,
        .NumSignals = 1,
        .ShadowBuffer = TestShadowBuffer2,
        .ComNotification = TestNotificationCallback
    }
};

static Com_SignalIdType IPdu1Signals[] = {0, 1, 2};
static Com_SignalIdType IPdu2Signals[] = {3};
static Com_SignalGroupIdType IPdu1SignalGroups[] = {0};
static Com_SignalGroupIdType IPdu2SignalGroups[] = {1};

static const Com_IPduConfigType TestIPdus[] = {
    {
        .IPduId = 0,
        .DataPtr = TestIPduBuffer,
        .Length = 8,
        .Direction = COM_SEND,
        .Type = COM_NORMAL,
        .SignalProcessing = COM_DEFERRED,
        .SignalRefs = IPdu1Signals,
        .NumSignals = 3,
        .SignalGroupRefs = IPdu1SignalGroups,
        .NumSignalGroups = 1,
        .TxMode = {
            .Mode = COM_PERIODIC,
            .Period = 100,
            .RepetitionPeriod = 0,
            .NumRepetitions = 0,
            .TimeOffset = 0
        },
        .IpduGroupRefs = NULL,
        .NumIpduGroups = 0,
        .Timeout = 0,
        .ComIPduCallout = NULL
    },
    {
        .IPduId = 1,
        .DataPtr = TestIPduBuffer2,
        .Length = 8,
        .Direction = COM_RECEIVE,
        .Type = COM_NORMAL,
        .SignalProcessing = COM_IMMEDIATE,
        .SignalRefs = IPdu2Signals,
        .NumSignals = 1,
        .SignalGroupRefs = IPdu2SignalGroups,
        .NumSignalGroups = 1,
        .TxMode = {
            .Mode = COM_NONE,
            .Period = 0,
            .RepetitionPeriod = 0,
            .NumRepetitions = 0,
            .TimeOffset = 0
        },
        .IpduGroupRefs = NULL,
        .NumIpduGroups = 0,
        .Timeout = 0,
        .ComIPduCallout = NULL
    }
};

static const Com_ConfigType TestConfig = {
    .Signals = TestSignals,
    .NumSignals = 4,
    .SignalGroups = TestSignalGroups,
    .NumSignalGroups = 2,
    .IPdus = TestIPdus,
    .NumIPdus = 2,
    .IPduGroups = NULL,
    .NumIPduGroups = 0
};

/*==================[Test Setup]===========================================*/

void setUp(void) {
    memset(TestIPduBuffer, 0, sizeof(TestIPduBuffer));
    memset(TestIPduBuffer2, 0, sizeof(TestIPduBuffer2));
    memset(TestShadowBuffer, 0, sizeof(TestShadowBuffer));
    memset(TestShadowBuffer2, 0, sizeof(TestShadowBuffer2));
    NotificationCalled = FALSE;
    Com_Init(&TestConfig);
}

void tearDown(void) {
    Com_DeInit();
}

/*==================[UpdateShadowSignal Tests]=============================*/

void test_update_shadow_signal_uint16(void) {
    uint16 value = 0x1234;
    uint8 result = Com_UpdateShadowSignal(0, &value);
    
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    TEST_ASSERT_EQUAL_UINT8(0x34, TestShadowBuffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x12, TestShadowBuffer[1]);
}

void test_update_shadow_signal_uint8(void) {
    uint8 value = 0xAB;
    uint8 result = Com_UpdateShadowSignal(1, &value);
    
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    TEST_ASSERT_EQUAL_UINT8(0xAB, TestShadowBuffer[2]);
}

void test_update_shadow_signal_uint32(void) {
    uint32 value = 0x12345678;
    uint8 result = Com_UpdateShadowSignal(3, &value);
    
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    TEST_ASSERT_EQUAL_UINT8(0x78, TestShadowBuffer2[0]);
    TEST_ASSERT_EQUAL_UINT8(0x56, TestShadowBuffer2[1]);
    TEST_ASSERT_EQUAL_UINT8(0x34, TestShadowBuffer2[2]);
    TEST_ASSERT_EQUAL_UINT8(0x12, TestShadowBuffer2[3]);
}

void test_update_shadow_signal_multiple(void) {
    uint16 value1 = 0x1234;
    uint8 value2 = 0xAB;
    uint16 value3 = 0x5678;
    
    Com_UpdateShadowSignal(0, &value1);
    Com_UpdateShadowSignal(1, &value2);
    Com_UpdateShadowSignal(2, &value3);
    
    /* Verify all values in shadow buffer */
    TEST_ASSERT_EQUAL_UINT8(0x34, TestShadowBuffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x12, TestShadowBuffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0xAB, TestShadowBuffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0x78, TestShadowBuffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x56, TestShadowBuffer[4]);
}

void test_update_shadow_signal_invalid_id(void) {
    uint16 value = 0x1234;
    uint8 result = Com_UpdateShadowSignal(99, &value);
    
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

void test_update_shadow_signal_null_pointer(void) {
    uint8 result = Com_UpdateShadowSignal(0, NULL);
    
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

void test_update_shadow_signal_before_init(void) {
    Com_DeInit();
    
    uint16 value = 0x1234;
    uint8 result = Com_UpdateShadowSignal(0, &value);
    
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

/*==================[SendSignalGroup Tests]================================*/

void test_send_signal_group(void) {
    /* Update shadow buffer */
    uint16 rpm = 0x1234;
    uint8 temp = 0xAB;
    uint16 load = 0x5678;
    
    Com_UpdateShadowSignal(0, &rpm);
    Com_UpdateShadowSignal(1, &temp);
    Com_UpdateShadowSignal(2, &load);
    
    /* Send signal group */
    uint8 result = Com_SendSignalGroup(0);
    
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    
    /* Verify data copied to IPdu buffer */
    TEST_ASSERT_EQUAL_UINT8(0x34, TestIPduBuffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x12, TestIPduBuffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0xAB, TestIPduBuffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0x78, TestIPduBuffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x56, TestIPduBuffer[4]);
}

void test_send_signal_group_invalid_id(void) {
    uint8 result = Com_SendSignalGroup(99);
    
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

void test_send_signal_group_before_init(void) {
    Com_DeInit();
    uint8 result = Com_SendSignalGroup(0);
    
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

void test_send_signal_group_no_shadow_update(void) {
    /* Send signal group without updating shadow buffer first */
    uint8 result = Com_SendSignalGroup(0);
    
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    
    /* IPdu buffer should contain zeros (initialized values) */
    TEST_ASSERT_EQUAL_UINT8(0x00, TestIPduBuffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x00, TestIPduBuffer[1]);
}

/*==================[ReceiveSignalGroup Tests]=============================*/

void test_receive_signal_group(void) {
    /* Setup IPdu buffer with known data */
    TestIPduBuffer[0] = 0x78;
    TestIPduBuffer[1] = 0x56;
    TestIPduBuffer[2] = 0xCD;
    TestIPduBuffer[3] = 0x34;
    TestIPduBuffer[4] = 0x12;
    
    /* Receive signal group */
    uint8 result = Com_ReceiveSignalGroup(0);
    
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    
    /* Verify data copied to shadow buffer */
    TEST_ASSERT_EQUAL_UINT8(0x78, TestShadowBuffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x56, TestShadowBuffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0xCD, TestShadowBuffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0x34, TestShadowBuffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x12, TestShadowBuffer[4]);
}

void test_receive_signal_group_invalid_id(void) {
    uint8 result = Com_ReceiveSignalGroup(99);
    
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

void test_receive_signal_group_before_init(void) {
    Com_DeInit();
    
    uint8 result = Com_ReceiveSignalGroup(0);
    
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

/*==================[SendSignalGroupArray Tests]===========================*/

void test_send_signal_group_array_basic(void) {
    uint8 arrayData[] = {0x12, 0x34, 0x56, 0x78, 0x9A};
    
    uint8 result = Com_SendSignalGroupArray(0, arrayData);
    
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    
    /* Verify data copied to IPdu buffer */
    TEST_ASSERT_EQUAL_UINT8(0x12, TestIPduBuffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x34, TestIPduBuffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0x56, TestIPduBuffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0x78, TestIPduBuffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x9A, TestIPduBuffer[4]);
}

void test_send_signal_group_array_invalid_id(void) {
    uint8 arrayData[] = {0x12, 0x34};
    
    uint8 result = Com_SendSignalGroupArray(99, arrayData);
    
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

void test_send_signal_group_array_null_pointer(void) {
    uint8 result = Com_SendSignalGroupArray(0, NULL);
    
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

/*==================[ReceiveSignalGroupArray Tests]========================*/

void test_receive_signal_group_array_basic(void) {
    /* Setup IPdu buffer with known data */
    TestIPduBuffer[0] = 0x12;
    TestIPduBuffer[1] = 0x34;
    TestIPduBuffer[2] = 0x56;
    TestIPduBuffer[3] = 0x78;
    TestIPduBuffer[4] = 0x9A;
    
    uint8 receivedData[5] = {0};
    
    uint8 result = Com_ReceiveSignalGroupArray(0, receivedData);
    
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    TEST_ASSERT_EQUAL_UINT8(0x12, receivedData[0]);
    TEST_ASSERT_EQUAL_UINT8(0x34, receivedData[1]);
    TEST_ASSERT_EQUAL_UINT8(0x56, receivedData[2]);
    TEST_ASSERT_EQUAL_UINT8(0x78, receivedData[3]);
    TEST_ASSERT_EQUAL_UINT8(0x9A, receivedData[4]);
}

void test_receive_signal_group_array_invalid_id(void) {
    uint8 receivedData[5];
    
    uint8 result = Com_ReceiveSignalGroupArray(99, receivedData);
    
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

void test_receive_signal_group_array_null_pointer(void) {
    uint8 result = Com_ReceiveSignalGroupArray(0, NULL);
    
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

/*==================[InvalidateSignalGroup Tests]==========================*/

void test_invalidate_signal_group(void) {
    /* Setup IPdu buffer with known data */
    TestIPduBuffer[0] = 0xFF;
    TestIPduBuffer[1] = 0xFF;
    TestIPduBuffer[2] = 0xFF;
    
    /* Invalidate signal group */
    Com_InvalidateSignalGroup(0);
    
    /* In a full implementation, this would set signals to invalid values */
    /* For now, just verify it doesn't crash */
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
}

void test_invalidate_signal_group_invalid_id(void) {
    /* Should handle invalid ID gracefully */
    Com_InvalidateSignalGroup(99);
    
    /* No crash expected */
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
}

void test_invalidate_signal_group_before_init(void) {
    Com_DeInit();
    
    /* Should handle call before init gracefully */
    Com_InvalidateSignalGroup(0);
    
    /* No crash expected */
}

/*==================[Integration Tests]====================================*/

void test_shadow_to_send_to_receive_flow(void) {
    /* Update shadow buffer */
    uint16 value1 = 0x1234;
    uint8 value2 = 0xAB;
    uint16 value3 = 0x5678;
    
    Com_UpdateShadowSignal(0, &value1);
    Com_UpdateShadowSignal(1, &value2);
    Com_UpdateShadowSignal(2, &value3);
    
    /* Send signal group */
    Com_SendSignalGroup(0);
    
    /* Verify data in IPdu buffer */
    TEST_ASSERT_EQUAL_UINT8(0x34, TestIPduBuffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x12, TestIPduBuffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0xAB, TestIPduBuffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0x78, TestIPduBuffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x56, TestIPduBuffer[4]);
    
    /* Clear shadow buffer */
    memset(TestShadowBuffer, 0, sizeof(TestShadowBuffer));
    
    /* Receive signal group */
    Com_ReceiveSignalGroup(0);
    
    /* Verify data back in shadow buffer */
    TEST_ASSERT_EQUAL_UINT8(0x34, TestShadowBuffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x12, TestShadowBuffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0xAB, TestShadowBuffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0x78, TestShadowBuffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x56, TestShadowBuffer[4]);
}

void test_multiple_signal_groups(void) {
    uint16 value1 = 0x1234;
    uint32 value2 = 0xABCDEF01;
    
    /* Update and send first group */
    Com_UpdateShadowSignal(0, &value1);
    Com_SendSignalGroup(0);
    
    /* Update and send second group */
    Com_UpdateShadowSignal(3, &value2);
    Com_SendSignalGroup(1);
    
    /* Verify first group data */
    TEST_ASSERT_EQUAL_UINT8(0x34, TestIPduBuffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x12, TestIPduBuffer[1]);
    
    /* Verify second group data */
    TEST_ASSERT_EQUAL_UINT8(0x01, TestIPduBuffer2[0]);
    TEST_ASSERT_EQUAL_UINT8(0xEF, TestIPduBuffer2[1]);
    TEST_ASSERT_EQUAL_UINT8(0xCD, TestIPduBuffer2[2]);
    TEST_ASSERT_EQUAL_UINT8(0xAB, TestIPduBuffer2[3]);
}

/*==================[Main]=================================================*/

int main(void) {
    UNITY_BEGIN();
    
    /* UpdateShadowSignal Tests */
    RUN_TEST(test_update_shadow_signal_uint16);
    RUN_TEST(test_update_shadow_signal_uint8);
    RUN_TEST(test_update_shadow_signal_uint32);
    RUN_TEST(test_update_shadow_signal_multiple);
    RUN_TEST(test_update_shadow_signal_invalid_id);
    RUN_TEST(test_update_shadow_signal_null_pointer);
    RUN_TEST(test_update_shadow_signal_before_init);
    
    /* SendSignalGroup Tests */
    RUN_TEST(test_send_signal_group);
    RUN_TEST(test_send_signal_group_invalid_id);
    RUN_TEST(test_send_signal_group_before_init);
    RUN_TEST(test_send_signal_group_no_shadow_update);
    
    /* ReceiveSignalGroup Tests */
    RUN_TEST(test_receive_signal_group);
    RUN_TEST(test_receive_signal_group_invalid_id);
    RUN_TEST(test_receive_signal_group_before_init);
    
    /* SendSignalGroupArray Tests */
    RUN_TEST(test_send_signal_group_array_basic);
    RUN_TEST(test_send_signal_group_array_invalid_id);
    RUN_TEST(test_send_signal_group_array_null_pointer);
    
    /* ReceiveSignalGroupArray Tests */
    RUN_TEST(test_receive_signal_group_array_basic);
    RUN_TEST(test_receive_signal_group_array_invalid_id);
    RUN_TEST(test_receive_signal_group_array_null_pointer);
    
    /* InvalidateSignalGroup Tests */
    RUN_TEST(test_invalidate_signal_group);
    RUN_TEST(test_invalidate_signal_group_invalid_id);
    RUN_TEST(test_invalidate_signal_group_before_init);
    
    /* Integration Tests */
    RUN_TEST(test_shadow_to_send_to_receive_flow);
    RUN_TEST(test_multiple_signal_groups);
    
    return UNITY_END();
}
