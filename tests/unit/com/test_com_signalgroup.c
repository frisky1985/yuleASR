/*
 * test_com_signalgroup.c
 * COM Module Unit Tests - Signal Group Operations
 */

#include "unity.h"
#include "Com.h"

/*==================[Test Configuration]===================================*/

static uint8 TestIPduBuffer[8];
static uint8 TestShadowBuffer[8];

static const Com_SignalIdType EngineSignals[] = {0, 1, 2};

static const Com_SignalConfigType TestSignals[] = {
    {
        .SignalId = 0,
        .DataPtr = &TestIPduBuffer[0],
        .BitPosition = 0,
        .BitSize = 16,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT16
    },
    {
        .SignalId = 1,
        .DataPtr = &TestIPduBuffer[2],
        .BitPosition = 0,
        .BitSize = 8,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT8
    },
    {
        .SignalId = 2,
        .DataPtr = &TestIPduBuffer[3],
        .BitPosition = 0,
        .BitSize = 16,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT16
    }
};

static const Com_SignalGroupConfigType TestSignalGroups[] = {
    {
        .SignalGroupId = 0,
        .SignalRefs = (Com_SignalIdType[]){0, 1, 2},
        .NumSignals = 3,
        .ShadowBuffer = TestShadowBuffer
    }
};

static const Com_IPduConfigType TestIPdus[] = {
    {
        .IPduId = 0,
        .DataPtr = TestIPduBuffer,
        .Length = 8,
        .Direction = COM_SEND,
        .SignalProcessing = COM_DEFERRED,
        .SignalGroupRefs = (Com_SignalGroupIdType[]){0},
        .NumSignalGroups = 1
    }
};

static const Com_ConfigType TestConfig = {
    .Signals = TestSignals,
    .NumSignals = 3,
    .SignalGroups = TestSignalGroups,
    .NumSignalGroups = 1,
    .IPdus = TestIPdus,
    .NumIPdus = 1,
    .IPduGroups = NULL,
    .NumIPduGroups = 0
};

/*==================[Test Setup]===========================================*/

void setUp(void) {
    memset(TestIPduBuffer, 0, sizeof(TestIPduBuffer));
    memset(TestShadowBuffer, 0, sizeof(TestShadowBuffer));
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

/*==================[Main]=================================================*/

int main(void) {
    UNITY_BEGIN();
    
    /* UpdateShadowSignal Tests */
    RUN_TEST(test_update_shadow_signal_uint16);
    RUN_TEST(test_update_shadow_signal_uint8);
    RUN_TEST(test_update_shadow_signal_multiple);
    RUN_TEST(test_update_shadow_signal_invalid_id);
    
    /* SendSignalGroup Tests */
    RUN_TEST(test_send_signal_group);
    RUN_TEST(test_send_signal_group_invalid_id);
    RUN_TEST(test_send_signal_group_before_init);
    
    /* ReceiveSignalGroup Tests */
    RUN_TEST(test_receive_signal_group);
    RUN_TEST(test_receive_signal_group_invalid_id);
    
    return UNITY_END();
}
