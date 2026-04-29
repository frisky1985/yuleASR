/*
 * test_com_error_handling.c
 * COM Module Unit Tests - Error Handling
 */

#include "unity.h"
#include "Com.h"
#include "Com_Private.h"
#include "Com_ErrorHandling.h"
#include "Com_DeadlineMon.h"

/*==================[Test Configuration]===================================*/

static uint8 TestIPduBuffer[8];
static uint8 TestShadowBuffer[8];

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
        .SignalRefs = (Com_SignalIdType[]){0},
        .NumSignals = 1,
        .SignalGroupRefs = NULL,
        .NumSignalGroups = 0,
        .TxMode = {
            .Mode = COM_PERIODIC,
            .Period = 100,
            .RepetitionPeriod = 0,
            .NumRepetitions = 0,
            .TimeOffset = 0
        },
        .IpduGroupRefs = NULL,
        .NumIpduGroups = 0,
        .Timeout = 1000,
        .ComIPduCallout = NULL
    }
};

static const Com_ConfigType TestConfig = {
    .Signals = TestSignals,
    .NumSignals = 1,
    .SignalGroups = NULL,
    .NumSignalGroups = 0,
    .IPdus = TestIPdus,
    .NumIPdus = 1,
    .IPduGroups = NULL,
    .NumIPduGroups = 0
};

/*==================[Test Setup]===========================================*/

void setUp(void)
{
    memset(TestIPduBuffer, 0, sizeof(TestIPduBuffer));
    memset(TestShadowBuffer, 0, sizeof(TestShadowBuffer));
    Com_Init(&TestConfig);
}

void tearDown(void)
{
    Com_DeInit();
}

/*==================[Parameter Validation Tests]===========================*/

void test_error_signal_id_out_of_range(void)
{
    uint16 value = 0x1234;
    
    /* Try to send with invalid signal ID */
    uint8 result = Com_SendSignal(99, &value);
    
    /* Should return service not available */
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

void test_error_null_pointer_signal_data(void)
{
    /* Try to send with NULL pointer */
    uint8 result = Com_SendSignal(0, NULL);
    
    /* Should return service not available */
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

void test_error_invalid_signal_group_id(void)
{
    /* Try to send invalid signal group */
    uint8 result = Com_SendSignalGroup(99);
    
    /* Should return service not available */
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

void test_error_invalid_ipdu_id(void)
{
    /* Try to trigger invalid IPdu */
    Std_ReturnType result = Com_TriggerIPDUSend(99);
    
    /* Should return error */
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_error_null_version_info(void)
{
    /* Try to get version info with NULL pointer */
    /* This should be handled gracefully */
    Com_GetVersionInfo(NULL);
    
    /* No crash expected */
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
}

/*==================[State Validation Tests]===============================*/

void test_error_send_before_init(void)
{
    Com_DeInit();
    
    uint16 value = 0x1234;
    uint8 result = Com_SendSignal(0, &value);
    
    /* Should return service not available when not initialized */
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

void test_error_receive_before_init(void)
{
    Com_DeInit();
    
    uint16 value = 0;
    uint8 result = Com_ReceiveSignal(0, &value);
    
    /* Should return service not available when not initialized */
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

void test_error_mainfunctiontx_before_init(void)
{
    Com_DeInit();
    
    /* Should not crash */
    Com_MainFunctionTx();
}

void test_error_mainfunctionrx_before_init(void)
{
    Com_DeInit();
    
    /* Should not crash */
    Com_MainFunctionRx();
}

void test_error_trigger_ipdu_before_init(void)
{
    Com_DeInit();
    
    Std_ReturnType result = Com_TriggerIPDUSend(0);
    
    /* Should return error when not initialized */
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/*==================[IPdu Group State Tests]===============================*/

void test_error_send_to_stopped_group(void)
{
    /* Ensure IPdu group is stopped */
    Com_GlobalState.IPduRunTime[0].GroupStatus = COM_IPDU_GROUP_STOPPED;
    
    uint16 value = 0x1234;
    
    /* Try to send signal - may succeed at COM level but won't trigger transmission */
    uint8 result = Com_SendSignal(0, &value);
    
    /* Signal write should succeed */
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
    
    /* But data should still be written */
    TEST_ASSERT_EQUAL_UINT8(0x34, TestIPduBuffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x12, TestIPduBuffer[1]);
}

void test_error_trigger_stopped_ipdu(void)
{
    /* Ensure IPdu group is stopped */
    Com_GlobalState.IPduRunTime[0].GroupStatus = COM_IPDU_GROUP_STOPPED;
    
    /* Try to trigger send */
    Std_ReturnType result = Com_TriggerIPDUSend(0);
    
    /* Should return error */
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/*==================[Invalidation Tests]===================================*/

void test_error_invalidate_invalid_signal(void)
{
    /* Try to invalidate invalid signal */
    Com_InvalidateSignal(99);
    
    /* Should be handled gracefully */
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
}

void test_error_invalidate_invalid_signal_group(void)
{
    /* Try to invalidate invalid signal group */
    Com_InvalidateSignalGroup(99);
    
    /* Should be handled gracefully */
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
}

void test_error_invalidate_before_init(void)
{
    Com_DeInit();
    
    /* Try to invalidate when not initialized */
    Com_InvalidateSignal(0);
    
    /* Should be handled gracefully */
}

/*==================[Queue Management Error Tests]=========================*/

void test_error_queue_operations_before_init(void)
{
    Com_DeInit();
    
    /* Queue operations should handle uninitialized state */
    uint8 fillLevel = Com_GetTxQueueFillLevel();
    Com_ClearTxQueueForPdu(0);
    
    /* Should return 0 or handle gracefully */
    TEST_ASSERT_EQUAL(0, fillLevel);
}

/*==================[Double Init/DeInit Tests]=============================*/

void test_error_double_init(void)
{
    /* First init */
    Com_Init(&TestConfig);
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
    
    /* Second init - should be handled gracefully */
    Com_Init(&TestConfig);
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
}

void test_error_deinit_without_init(void)
{
    /* DeInit without prior Init */
    Com_DeInit();
    
    /* Should remain UNINIT */
    TEST_ASSERT_EQUAL(COM_UNINIT, Com_GetStatus());
}

void test_error_double_deinit(void)
{
    Com_Init(&TestConfig);
    Com_DeInit();
    
    /* Second DeInit */
    Com_DeInit();
    
    /* Should remain UNINIT */
    TEST_ASSERT_EQUAL(COM_UNINIT, Com_GetStatus());
}

/*==================[Boundary Tests]=======================================*/

void test_error_signal_id_at_boundary(void)
{
    uint16 value = 0x1234;
    
    /* Signal ID equal to NumSignals (out of range) */
    uint8 result = Com_SendSignal(TestConfig.NumSignals, &value);
    
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

void test_error_signal_id_max_value(void)
{
    uint16 value = 0x1234;
    
    /* Signal ID at max uint16 value */
    uint8 result = Com_SendSignal(0xFFFF, &value);
    
    TEST_ASSERT_EQUAL_UINT8(COM_SERVICE_NOT_AVAILABLE, result);
}

void test_error_ipdu_id_at_boundary(void)
{
    /* IPdu ID equal to NumIPdus (out of range) */
    Std_ReturnType result = Com_TriggerIPDUSend(TestConfig.NumIPdus);
    
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/*==================[Stress Tests]=========================================*/

void test_error_rapid_init_deinit(void)
{
    for (int i = 0; i < 100; i++) {
        Com_Init(&TestConfig);
        TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
        
        Com_DeInit();
        TEST_ASSERT_EQUAL(COM_UNINIT, Com_GetStatus());
    }
}

void test_error_many_invalid_operations(void)
{
    uint16 value = 0x1234;
    
    /* Perform many invalid operations */
    for (int i = 0; i < 100; i++) {
        Com_SendSignal(99, &value);
        Com_SendSignal(0, NULL);
        Com_TriggerIPDUSend(99);
        Com_InvalidateSignal(99);
    }
    
    /* Module should still be functional */
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
    
    /* Valid operation should still work */
    uint8 result = Com_SendSignal(0, &value);
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
}

/*==================[Error Recovery Tests]=================================*/

void test_error_recovery_after_invalid_send(void)
{
    uint16 value = 0x1234;
    
    /* Invalid send */
    Com_SendSignal(99, &value);
    
    /* Valid send should still work */
    uint8 result = Com_SendSignal(0, &value);
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
}

void test_error_recovery_after_null_pointer(void)
{
    uint16 value = 0x1234;
    
    /* Send with NULL pointer */
    Com_SendSignal(0, NULL);
    
    /* Valid send should still work */
    uint8 result = Com_SendSignal(0, &value);
    TEST_ASSERT_EQUAL_UINT8(E_OK, result);
}

/*==================[Main]=================================================*/

int main(void)
{
    UNITY_BEGIN();
    
    /* Parameter Validation Tests */
    RUN_TEST(test_error_signal_id_out_of_range);
    RUN_TEST(test_error_null_pointer_signal_data);
    RUN_TEST(test_error_invalid_signal_group_id);
    RUN_TEST(test_error_invalid_ipdu_id);
    RUN_TEST(test_error_null_version_info);
    
    /* State Validation Tests */
    RUN_TEST(test_error_send_before_init);
    RUN_TEST(test_error_receive_before_init);
    RUN_TEST(test_error_mainfunctiontx_before_init);
    RUN_TEST(test_error_mainfunctionrx_before_init);
    RUN_TEST(test_error_trigger_ipdu_before_init);
    
    /* IPdu Group State Tests */
    RUN_TEST(test_error_send_to_stopped_group);
    RUN_TEST(test_error_trigger_stopped_ipdu);
    
    /* Invalidation Tests */
    RUN_TEST(test_error_invalidate_invalid_signal);
    RUN_TEST(test_error_invalidate_invalid_signal_group);
    RUN_TEST(test_error_invalidate_before_init);
    
    /* Queue Management Error Tests */
    RUN_TEST(test_error_queue_operations_before_init);
    
    /* Double Init/DeInit Tests */
    RUN_TEST(test_error_double_init);
    RUN_TEST(test_error_deinit_without_init);
    RUN_TEST(test_error_double_deinit);
    
    /* Boundary Tests */
    RUN_TEST(test_error_signal_id_at_boundary);
    RUN_TEST(test_error_signal_id_max_value);
    RUN_TEST(test_error_ipdu_id_at_boundary);
    
    /* Stress Tests */
    RUN_TEST(test_error_rapid_init_deinit);
    RUN_TEST(test_error_many_invalid_operations);
    
    /* Error Recovery Tests */
    RUN_TEST(test_error_recovery_after_invalid_send);
    RUN_TEST(test_error_recovery_after_null_pointer);
    
    return UNITY_END();
}
