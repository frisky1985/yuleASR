/*
 * test_com_signal.c
 * COM Module Unit Tests - Signal Operations
 */

#include "unity.h"
#include "Com.h"
#include "mock_PduR.h"

/*==================[Test Configuration]===================================*/

static uint8 TestIPduBuffer[8];

static const Com_SignalConfigType TestSignals[] = {
    {
        .SignalId = 0,
        .DataPtr = &TestIPduBuffer[0],
        .BitPosition = 0,
        .BitSize = 16,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT16,
        .TransferProperty = COM_TRIGGERED,
        .InitValue = NULL
    },
    {
        .SignalId = 1,
        .DataPtr = &TestIPduBuffer[2],
        .BitPosition = 0,
        .BitSize = 8,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT8,
        .TransferProperty = COM_TRIGGERED_ON_CHANGE,
        .InitValue = NULL
    }
};

static const Com_IPduConfigType TestIPdus[] = {
    {
        .IPduId = 0,
        .DataPtr = TestIPduBuffer,
        .Length = 8,
        .Direction = COM_SEND,
        .Type = COM_NORMAL,
        .SignalProcessing = COM_IMMEDIATE,
        .SignalRefs = (Com_SignalIdType[]){0, 1},
        .NumSignals = 2
    }
};

static const Com_ConfigType TestConfig = {
    .Signals = TestSignals,
    .NumSignals = 2,
    .SignalGroups = NULL,
    .NumSignalGroups = 0,
    .IPdus = TestIPdus,
    .NumIPdus = 1,
    .IPduGroups = NULL,
    .NumIPduGroups = 0
};

/*==================[Test Setup]===========================================*/

void setUp(void) {
    memset(TestIPduBuffer, 0, sizeof(TestIPduBuffer));
    Com_Init(&TestConfig);
}

void tearDown(void) {
    Com_DeInit();
}

/*==================[SendSignal Tests]=====================================*/

void test_send_signal_uint16(void) {
    uint16 value = 0x1234;
    uint8 result = Com_SendSignal(0, &value);
    
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    TEST_ASSERT_EQUAL_UINT8(0x34, TestIPduBuffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x12, TestIPduBuffer[1]);
}

void test_send_signal_uint8(void) {
    uint8 value = 0xAB;
    uint8 result = Com_SendSignal(1, &value);
    
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    TEST_ASSERT_EQUAL_UINT8(0xAB, TestIPduBuffer[2]);
}

void test_send_signal_invalid_id(void) {
    uint16 value = 0x1234;
    uint8 result = Com_SendSignal(99, &value);
    
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

void test_send_signal_null_pointer(void) {
    uint8 result = Com_SendSignal(0, NULL);
    
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

void test_send_signal_before_init(void) {
    Com_DeInit();
    uint16 value = 0x1234;
    uint8 result = Com_SendSignal(0, &value);
    
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

/*==================[ReceiveSignal Tests]==================================*/

void test_receive_signal_uint16(void) {
    /* Setup buffer with known data */
    TestIPduBuffer[0] = 0x78;
    TestIPduBuffer[1] = 0x56;
    
    uint16 value = 0;
    uint8 result = Com_ReceiveSignal(0, &value);
    
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    TEST_ASSERT_EQUAL_UINT16(0x5678, value);
}

void test_receive_signal_uint8(void) {
    TestIPduBuffer[2] = 0xCD;
    
    uint8 value = 0;
    uint8 result = Com_ReceiveSignal(1, &value);
    
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    TEST_ASSERT_EQUAL_UINT8(0xCD, value);
}

void test_receive_signal_invalid_id(void) {
    uint16 value = 0;
    uint8 result = Com_ReceiveSignal(99, &value);
    
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

void test_receive_signal_null_pointer(void) {
    uint8 result = Com_ReceiveSignal(0, NULL);
    
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

/*==================[Data Type Tests]======================================*/

void test_send_signal_sint16_negative(void) {
    Com_SignalConfigType* sig = (Com_SignalConfigType*)&TestSignals[0];
    sig->SignalType = COM_SINT16;
    
    sint16 value = -1;  /* 0xFFFF */
    uint8 result = Com_SendSignal(0, &value);
    
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    TEST_ASSERT_EQUAL_UINT8(0xFF, TestIPduBuffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0xFF, TestIPduBuffer[1]);
}

void test_send_signal_boolean_true(void) {
    Com_SignalConfigType* sig = (Com_SignalConfigType*)&TestSignals[1];
    sig->SignalType = COM_BOOLEAN;
    sig->BitSize = 1;
    
    boolean value = TRUE;
    uint8 result = Com_SendSignal(1, &value);
    
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    TEST_ASSERT_EQUAL_UINT8(0x01, TestIPduBuffer[2] & 0x01);
}

void test_send_signal_boolean_false(void) {
    Com_SignalConfigType* sig = (Com_SignalConfigType*)&TestSignals[1];
    sig->SignalType = COM_BOOLEAN;
    sig->BitSize = 1;
    
    boolean value = FALSE;
    uint8 result = Com_SendSignal(1, &value);
    
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    TEST_ASSERT_EQUAL_UINT8(0x00, TestIPduBuffer[2] & 0x01);
}

/*==================[Main]=================================================*/

int main(void) {
    UNITY_BEGIN();
    
    /* SendSignal Tests */
    RUN_TEST(test_send_signal_uint16);
    RUN_TEST(test_send_signal_uint8);
    RUN_TEST(test_send_signal_invalid_id);
    RUN_TEST(test_send_signal_null_pointer);
    RUN_TEST(test_send_signal_before_init);
    
    /* ReceiveSignal Tests */
    RUN_TEST(test_receive_signal_uint16);
    RUN_TEST(test_receive_signal_uint8);
    RUN_TEST(test_receive_signal_invalid_id);
    RUN_TEST(test_receive_signal_null_pointer);
    
    /* Data Type Tests */
    RUN_TEST(test_send_signal_sint16_negative);
    RUN_TEST(test_send_signal_boolean_true);
    RUN_TEST(test_send_signal_boolean_false);
    
    return UNITY_END();
}
