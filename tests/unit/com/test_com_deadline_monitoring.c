/*
 * test_com_deadline_monitoring.c
 * COM Module Unit Tests - Deadline Monitoring (T012)
 */

#include "unity.h"
#include "Com.h"
#include "Com_Private.h"
#include "Com_DeadlineMon.h"

/*==================[Test Configuration]===================================*/

static uint8 TestRxIPduBuffer[8];
static uint8 TestDefaultValue[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11};
static boolean ErrorHookCalled = FALSE;

static void TestErrorHook(Com_IPduIdType PduId)
{
    (void)PduId;
    ErrorHookCalled = TRUE;
}

static const Com_SignalConfigType TestSignals[] = {
    {
        .SignalId = 0,
        .DataPtr = &TestRxIPduBuffer[0],
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
        .DataPtr = TestRxIPduBuffer,
        .Length = 8,
        .Direction = COM_RECEIVE,
        .Type = COM_NORMAL,
        .SignalProcessing = COM_DEFERRED,
        .SignalRefs = (Com_SignalIdType[]){0},
        .NumSignals = 1,
        .SignalGroupRefs = NULL,
        .NumSignalGroups = 0,
        .TxMode = {
            .Mode = COM_NONE,
            .Period = 0,
            .RepetitionPeriod = 0,
            .NumRepetitions = 0,
            .TimeOffset = 0
        },
        .IpduGroupRefs = NULL,
        .NumIpduGroups = 0,
        .Timeout = 100,  /* 100ms timeout */
        .ComIPduCallout = NULL,
        .TxConfirmation = {
            .EnableConfirmation = FALSE,
            .TxTimeout = 0,
            .MaxRetries = 0,
            .ComTxConfirmation = NULL,
            .ComTxErrorNotification = NULL,
            .ComTxTimeoutNotification = NULL
        }
    },
    {
        .IPduId = 1,
        .DataPtr = TestRxIPduBuffer,
        .Length = 8,
        .Direction = COM_RECEIVE,
        .Type = COM_NORMAL,
        .SignalProcessing = COM_DEFERRED,
        .SignalRefs = (Com_SignalIdType[]){0},
        .NumSignals = 1,
        .SignalGroupRefs = NULL,
        .NumSignalGroups = 0,
        .TxMode = {
            .Mode = COM_NONE,
            .Period = 0,
            .RepetitionPeriod = 0,
            .NumRepetitions = 0,
            .TimeOffset = 0
        },
        .IpduGroupRefs = NULL,
        .NumIpduGroups = 0,
        .Timeout = 0,  /* No timeout */
        .ComIPduCallout = NULL,
        .TxConfirmation = {
            .EnableConfirmation = FALSE,
            .TxTimeout = 0,
            .MaxRetries = 0,
            .ComTxConfirmation = NULL,
            .ComTxErrorNotification = NULL,
            .ComTxTimeoutNotification = NULL
        }
    }
};

static const Com_ConfigType TestConfig = {
    .Signals = TestSignals,
    .NumSignals = 1,
    .SignalGroups = NULL,
    .NumSignalGroups = 0,
    .IPdus = TestIPdus,
    .NumIPdus = 2,
    .IPduGroups = NULL,
    .NumIPduGroups = 0
};

/*==================[Test Setup]===========================================*/

void setUp(void)
{
    memset(TestRxIPduBuffer, 0, sizeof(TestRxIPduBuffer));
    ErrorHookCalled = FALSE;
    Com_Init(&TestConfig);
}

void tearDown(void)
{
    Com_DeInit();
}

/*==================[Com_Dm_Init Tests]====================================*/

void test_dm_init_basic(void)
{
    /* Deadline monitoring should be initialized by Com_Init */
    /* Check that DmRunTimeData is initialized */
    for (uint16 i = 0; i < TestConfig.NumIPdus; i++) {
        TEST_ASSERT_EQUAL(COM_DM_STATE_STOPPED, Com_DmRunTimeData[i].State);
        TEST_ASSERT_EQUAL(0, Com_DmRunTimeData[i].Timer);
        TEST_ASSERT_FALSE(Com_DmRunTimeData[i].TimeoutProcessed);
    }
}

/*==================[Com_Dm_StartTimer Tests]==============================*/

void test_dm_starttimer_basic(void)
{
    /* Start timer for IPdu 0 */
    Com_Dm_StartTimer(0, 100);
    
    /* Check state and timer */
    TEST_ASSERT_EQUAL(COM_DM_STATE_RUNNING, Com_DmRunTimeData[0].State);
    TEST_ASSERT_EQUAL(100, Com_DmRunTimeData[0].Timer);
}

void test_dm_starttimer_zero_timeout(void)
{
    /* Start timer with zero timeout */
    Com_Dm_StartTimer(0, 0);
    
    /* Should either not start or start with 0 */
    /* Behavior depends on implementation */
}

void test_dm_starttimer_invalid_pdu(void)
{
    /* Should handle invalid PDU ID gracefully */
    Com_Dm_StartTimer(99, 100);
    
    /* No crash expected */
}

/*==================[Com_Dm_StopTimer Tests]===============================*/

void test_dm_stoptimer_basic(void)
{
    /* Start then stop timer */
    Com_Dm_StartTimer(0, 100);
    Com_Dm_StopTimer(0);
    
    /* Check state */
    TEST_ASSERT_EQUAL(COM_DM_STATE_STOPPED, Com_DmRunTimeData[0].State);
}

void test_dm_stoptimer_not_running(void)
{
    /* Stop timer that was never started */
    Com_Dm_StopTimer(0);
    
    /* Should remain stopped */
    TEST_ASSERT_EQUAL(COM_DM_STATE_STOPPED, Com_DmRunTimeData[0].State);
}

void test_dm_stoptimer_invalid_pdu(void)
{
    /* Should handle invalid PDU ID gracefully */
    Com_Dm_StopTimer(99);
    
    /* No crash expected */
}

/*==================[Com_Dm_ResetTimer Tests]==============================*/

void test_dm_resettimer_basic(void)
{
    /* Start timer, decrement, then reset */
    Com_Dm_StartTimer(0, 100);
    Com_DmRunTimeData[0].Timer = 50;  /* Simulate time passing */
    
    Com_Dm_ResetTimer(0, 100);
    
    /* Timer should be back to full value */
    TEST_ASSERT_EQUAL(100, Com_DmRunTimeData[0].Timer);
    TEST_ASSERT_EQUAL(COM_DM_STATE_RUNNING, Com_DmRunTimeData[0].State);
}

void test_dm_resettimer_invalid_pdu(void)
{
    /* Should handle invalid PDU ID gracefully */
    Com_Dm_ResetTimer(99, 100);
    
    /* No crash expected */
}

/*==================[Com_Dm_ProcessTimer Tests]============================*/

void test_dm_processtimer_no_timeout(void)
{
    /* Start timer */
    Com_Dm_StartTimer(0, 100);
    
    /* Process timer (decrements by 1) */
    Com_Dm_ProcessTimer(0);
    
    /* Timer should be decremented */
    TEST_ASSERT_EQUAL(99, Com_DmRunTimeData[0].Timer);
    TEST_ASSERT_EQUAL(COM_DM_STATE_RUNNING, Com_DmRunTimeData[0].State);
}

void test_dm_processtimer_timeout(void)
{
    /* Start timer */
    Com_Dm_StartTimer(0, 1);
    
    /* Process timer to cause timeout */
    Com_Dm_ProcessTimer(0);
    
    /* State should be expired */
    TEST_ASSERT_EQUAL(COM_DM_STATE_EXPIRED, Com_DmRunTimeData[0].State);
}

void test_dm_processtimer_stopped(void)
{
    /* Process timer that is stopped */
    Com_Dm_ProcessTimer(0);
    
    /* Should remain stopped */
    TEST_ASSERT_EQUAL(COM_DM_STATE_STOPPED, Com_DmRunTimeData[0].State);
}

void test_dm_processtimer_already_expired(void)
{
    /* Start and expire timer */
    Com_Dm_StartTimer(0, 1);
    Com_Dm_ProcessTimer(0);
    TEST_ASSERT_EQUAL(COM_DM_STATE_EXPIRED, Com_DmRunTimeData[0].State);
    
    /* Process again */
    Com_Dm_ProcessTimer(0);
    
    /* Should remain expired */
    TEST_ASSERT_EQUAL(COM_DM_STATE_EXPIRED, Com_DmRunTimeData[0].State);
}

/*==================[Com_Dm_GetState Tests]================================*/

void test_dm_getstate_stopped(void)
{
    Com_DmStateType state = Com_Dm_GetState(0);
    TEST_ASSERT_EQUAL(COM_DM_STATE_STOPPED, state);
}

void test_dm_getstate_running(void)
{
    Com_Dm_StartTimer(0, 100);
    
    Com_DmStateType state = Com_Dm_GetState(0);
    TEST_ASSERT_EQUAL(COM_DM_STATE_RUNNING, state);
}

void test_dm_getstate_expired(void)
{
    Com_Dm_StartTimer(0, 1);
    Com_Dm_ProcessTimer(0);
    
    Com_DmStateType state = Com_Dm_GetState(0);
    TEST_ASSERT_EQUAL(COM_DM_STATE_EXPIRED, state);
}

void test_dm_getstate_invalid_pdu(void)
{
    /* Should return error state for invalid PDU */
    Com_DmStateType state = Com_Dm_GetState(99);
    TEST_ASSERT_EQUAL(COM_DM_STATE_ERROR, state);
}

/*==================[Com_Dm_ProcessAllTimers Tests]========================*/

void test_dm_processalltimers_basic(void)
{
    /* Start timers for multiple IPdus */
    Com_Dm_StartTimer(0, 100);
    Com_Dm_StartTimer(1, 50);
    
    /* Process all timers */
    Com_Dm_ProcessAllTimers();
    
    /* Both timers should be decremented */
    TEST_ASSERT_EQUAL(99, Com_DmRunTimeData[0].Timer);
    TEST_ASSERT_EQUAL(49, Com_DmRunTimeData[1].Timer);
}

void test_dm_processalltimers_mixed_states(void)
{
    /* Set up mixed states */
    Com_Dm_StartTimer(0, 100);  /* Running */
    /* PDU 1 stays stopped */
    
    Com_Dm_ProcessAllTimers();
    
    /* Only running timer should be decremented */
    TEST_ASSERT_EQUAL(99, Com_DmRunTimeData[0].Timer);
}

/*==================[Integration with MainFunctionRx Tests]================*/

void test_dm_integration_with_mainfunctionrx(void)
{
    /* Start timer */
    Com_Dm_StartTimer(0, 10);
    
    /* Set IPdu group to started */
    Com_GlobalState.IPduRunTime[0].GroupStatus = COM_IPDU_GROUP_STARTED;
    
    /* Call MainFunctionRx multiple times to process deadline monitoring */
    for (int i = 0; i < 15; i++) {
        Com_MainFunctionRx();
    }
    
    /* Timeout should have been detected */
    /* Note: Actual behavior depends on DM integration in MainFunctionRx */
}

/*==================[Timeout Action Tests]=================================*/

void test_dm_timeout_action_none(void)
{
    /* Start timer and let it timeout */
    Com_Dm_StartTimer(0, 1);
    Com_Dm_ProcessTimer(0);
    
    /* With no action configured, just timeout detection */
    TEST_ASSERT_EQUAL(COM_DM_STATE_EXPIRED, Com_DmRunTimeData[0].State);
}

/*==================[Edge Cases]===========================================*/

void test_dm_multiple_start_stop_cycles(void)
{
    for (int i = 0; i < 5; i++) {
        Com_Dm_StartTimer(0, 100);
        TEST_ASSERT_EQUAL(COM_DM_STATE_RUNNING, Com_DmRunTimeData[0].State);
        
        Com_Dm_StopTimer(0);
        TEST_ASSERT_EQUAL(COM_DM_STATE_STOPPED, Com_DmRunTimeData[0].State);
    }
}

void test_dm_restart_after_timeout(void)
{
    /* Start and timeout */
    Com_Dm_StartTimer(0, 1);
    Com_Dm_ProcessTimer(0);
    TEST_ASSERT_EQUAL(COM_DM_STATE_EXPIRED, Com_DmRunTimeData[0].State);
    
    /* Restart timer */
    Com_Dm_StartTimer(0, 100);
    TEST_ASSERT_EQUAL(COM_DM_STATE_RUNNING, Com_DmRunTimeData[0].State);
    TEST_ASSERT_EQUAL(100, Com_DmRunTimeData[0].Timer);
    TEST_ASSERT_FALSE(Com_DmRunTimeData[0].TimeoutProcessed);
}

void test_dm_timer_at_max_value(void)
{
    /* Start timer with maximum reasonable value */
    Com_Dm_StartTimer(0, 0xFFFFFFFF);
    
    TEST_ASSERT_EQUAL(COM_DM_STATE_RUNNING, Com_DmRunTimeData[0].State);
    TEST_ASSERT_EQUAL(0xFFFFFFFF, Com_DmRunTimeData[0].Timer);
}

void test_dm_before_init(void)
{
    Com_DeInit();
    
    /* Operations should handle uninitialized state gracefully */
    Com_Dm_StartTimer(0, 100);
    Com_Dm_StopTimer(0);
    Com_Dm_ResetTimer(0, 100);
    Com_Dm_ProcessTimer(0);
    
    /* No crash expected */
}

/*==================[Main]=================================================*/

int main(void)
{
    UNITY_BEGIN();
    
    /* Com_Dm_Init Tests */
    RUN_TEST(test_dm_init_basic);
    
    /* Com_Dm_StartTimer Tests */
    RUN_TEST(test_dm_starttimer_basic);
    RUN_TEST(test_dm_starttimer_zero_timeout);
    RUN_TEST(test_dm_starttimer_invalid_pdu);
    
    /* Com_Dm_StopTimer Tests */
    RUN_TEST(test_dm_stoptimer_basic);
    RUN_TEST(test_dm_stoptimer_not_running);
    RUN_TEST(test_dm_stoptimer_invalid_pdu);
    
    /* Com_Dm_ResetTimer Tests */
    RUN_TEST(test_dm_resettimer_basic);
    RUN_TEST(test_dm_resettimer_invalid_pdu);
    
    /* Com_Dm_ProcessTimer Tests */
    RUN_TEST(test_dm_processtimer_no_timeout);
    RUN_TEST(test_dm_processtimer_timeout);
    RUN_TEST(test_dm_processtimer_stopped);
    RUN_TEST(test_dm_processtimer_already_expired);
    
    /* Com_Dm_GetState Tests */
    RUN_TEST(test_dm_getstate_stopped);
    RUN_TEST(test_dm_getstate_running);
    RUN_TEST(test_dm_getstate_expired);
    RUN_TEST(test_dm_getstate_invalid_pdu);
    
    /* Com_Dm_ProcessAllTimers Tests */
    RUN_TEST(test_dm_processalltimers_basic);
    RUN_TEST(test_dm_processalltimers_mixed_states);
    
    /* Integration Tests */
    RUN_TEST(test_dm_integration_with_mainfunctionrx);
    
    /* Timeout Action Tests */
    RUN_TEST(test_dm_timeout_action_none);
    
    /* Edge Cases */
    RUN_TEST(test_dm_multiple_start_stop_cycles);
    RUN_TEST(test_dm_restart_after_timeout);
    RUN_TEST(test_dm_timer_at_max_value);
    RUN_TEST(test_dm_before_init);
    
    return UNITY_END();
}
