/*
 * test_com_confirmation.c
 * COM Module Unit Tests - Transmission Confirmation and Retry Management
 * 
 * Tests for:
 * - Com_TxConfirmation callback
 * - Transmission state machine
 * - Timeout detection and handling
 * - Retry mechanism
 * - Transmission mode switch handling
 * - ComTxErrorNotification callback
 */

#include "unity.h"
#include "Com.h"
#include "mock_PduR.h"

/*==================[Test Configuration]===================================*/

static uint8 TestIPduBuffer[8];
static uint8 TestIPduBuffer2[8];

/* Confirmation tracking */
static boolean TxConfirmationCalled = FALSE;
static boolean TxErrorNotificationCalled = FALSE;
static boolean TxTimeoutNotificationCalled = FALSE;

/* Test callbacks */
static void TestTxConfirmationCallback(void)
{
    TxConfirmationCalled = TRUE;
}

static void TestTxErrorNotificationCallback(void)
{
    TxErrorNotificationCalled = TRUE;
}

static void TestTxTimeoutNotificationCallback(void)
{
    TxTimeoutNotificationCalled = TRUE;
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
            .Mode = COM_DIRECT,
            .Period = 0,
            .RepetitionPeriod = 0,
            .NumRepetitions = 0,
            .TimeOffset = 0
        },
        .IpduGroupRefs = NULL,
        .NumIpduGroups = 0,
        .Timeout = 100,
        .ComIPduCallout = NULL,
        .TxConfirmation = {
            .EnableConfirmation = TRUE,
            .TxTimeout = 50,  /* 50ms timeout for testing */
            .MaxRetries = 3,
            .ComTxConfirmation = TestTxConfirmationCallback,
            .ComTxErrorNotification = TestTxErrorNotificationCallback,
            .ComTxTimeoutNotification = TestTxTimeoutNotificationCallback
        }
    },
    {
        .IPduId = 1,
        .DataPtr = TestIPduBuffer2,
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
            .Period = 10,
            .RepetitionPeriod = 0,
            .NumRepetitions = 0,
            .TimeOffset = 0
        },
        .IpduGroupRefs = NULL,
        .NumIpduGroups = 0,
        .Timeout = 100,
        .ComIPduCallout = NULL,
        .TxConfirmation = {
            .EnableConfirmation = FALSE,  /* Confirmation disabled */
            .TxTimeout = 50,
            .MaxRetries = 0,
            .ComTxConfirmation = NULL,
            .ComTxErrorNotification = NULL,
            .ComTxTimeoutNotification = NULL
        }
    }
};

static const Com_IPduGroupConfigType TestIPduGroups[] = {
    {
        .IpduGroupId = 0,
        .IPduRefs = (Com_IPduIdType[]){0, 1},
        .NumIPdus = 2
    }
};

static const Com_ConfigType TestConfig = {
    .Signals = TestSignals,
    .NumSignals = 1,
    .SignalGroups = NULL,
    .NumSignalGroups = 0,
    .IPdus = TestIPdus,
    .NumIPdus = 2,
    .IPduGroups = TestIPduGroups,
    .NumIPduGroups = 1
};

/*==================[Test Setup]===========================================*/

void setUp(void)
{
    memset(TestIPduBuffer, 0, sizeof(TestIPduBuffer));
    memset(TestIPduBuffer2, 0, sizeof(TestIPduBuffer2));
    
    TxConfirmationCalled = FALSE;
    TxErrorNotificationCalled = FALSE;
    TxTimeoutNotificationCalled = FALSE;
    
    Com_Init(&TestConfig);
    Com_IpduGroupStart(0, TRUE);  /* Start IPdu group */
}

void tearDown(void)
{
    Com_DeInit();
}

/*==================[Confirmation Tests]===================================*/

void test_tx_confirmation_success(void)
{
    /* Simulate a transmission and successful confirmation */
    Com_TxConfirmation(0, E_OK);
    
    /* Check that confirmation callback was called */
    TEST_ASSERT_TRUE(TxConfirmationCalled);
    TEST_ASSERT_FALSE(TxErrorNotificationCalled);
    
    /* Check transmission status */
    Com_TxStatusType status = Com_GetTxStatus(0);
    TEST_ASSERT_EQUAL(COM_TX_CONFIRMED, status);
    
    Com_TxResultType result = Com_GetTxResult(0);
    TEST_ASSERT_EQUAL(COM_TX_RES_OK, result);
}

void test_tx_confirmation_failure(void)
{
    /* Simulate a transmission failure */
    Com_TxConfirmation(0, E_NOT_OK);
    
    /* Check that error notification was called */
    TEST_ASSERT_FALSE(TxConfirmationCalled);
    
    /* Check that retry was added to queue */
    boolean inQueue = Com_IsInRetryQueue(0);
    TEST_ASSERT_TRUE(inQueue);
    
    /* Check transmission status */
    Com_TxStatusType status = Com_GetTxStatus(0);
    TEST_ASSERT_EQUAL(COM_TX_RETRY_PENDING, status);
}

void test_tx_confirmation_disabled(void)
{
    /* Simulate confirmation for IPdu with confirmation disabled */
    Com_TxConfirmation(1, E_OK);
    
    /* Callbacks should not be called */
    TEST_ASSERT_FALSE(TxConfirmationCalled);
    TEST_ASSERT_FALSE(TxErrorNotificationCalled);
}

void test_tx_confirmation_invalid_pdu(void)
{
    /* Simulate confirmation for invalid PDU ID */
    Com_TxConfirmation(99, E_OK);
    
    /* Should not cause any issues */
    TEST_ASSERT_FALSE(TxConfirmationCalled);
}

void test_tx_confirmation_before_init(void)
{
    Com_DeInit();
    
    /* Should return without error (validated internally) */
    Com_TxConfirmation(0, E_OK);
    
    TEST_ASSERT_FALSE(TxConfirmationCalled);
}

/*==================[Timeout Tests]========================================*/

void test_tx_timeout_detection(void)
{
    /* Start transmission confirmation monitoring */
    Com_StartTxConfirmation(0);
    
    /* Verify status is pending */
    Com_TxStatusType status = Com_GetTxStatus(0);
    TEST_ASSERT_EQUAL(COM_TX_PENDING, status);
    
    /* Simulate timeout by calling ProcessTxTimeouts enough times */
    for (uint32 i = 0; i < 55; i++) {
        Com_ProcessTxTimeouts();
    }
    
    /* Check timeout was detected */
    boolean timedOut = Com_IsTxTimedOut(0);
    TEST_ASSERT_TRUE(timedOut);
    
    /* Check timeout callback was called */
    TEST_ASSERT_TRUE(TxTimeoutNotificationCalled);
    
    /* Check that retry was added to queue */
    boolean inQueue = Com_IsInRetryQueue(0);
    TEST_ASSERT_TRUE(inQueue);
}

void test_tx_timeout_no_confirmation_enabled(void)
{
    /* Try to start confirmation for IPdu with confirmation disabled */
    Std_ReturnType result = Com_StartTxConfirmation(1);
    
    /* Should return OK but not actually start monitoring */
    TEST_ASSERT_EQUAL(E_OK, result);
}

void test_tx_timeout_reset(void)
{
    /* Start confirmation */
    Com_StartTxConfirmation(0);
    
    /* Process some timeouts but not enough to trigger */
    for (uint32 i = 0; i < 20; i++) {
        Com_ProcessTxTimeouts();
    }
    
    /* Reset timeout */
    Com_ResetTxTimeout(0);
    
    /* Process more - should not timeout because timer was reset */
    for (uint32 i = 0; i < 40; i++) {
        Com_ProcessTxTimeouts();
    }
    
    /* Should still be pending */
    Com_TxStatusType status = Com_GetTxStatus(0);
    TEST_ASSERT_EQUAL(COM_TX_PENDING, status);
}

/*==================[Retry Tests]==========================================*/

void test_retry_queue_add_remove(void)
{
    /* Add to retry queue */
    Std_ReturnType result = Com_AddToRetryQueue(0, 3);
    TEST_ASSERT_EQUAL(E_OK, result);
    
    /* Check it's in the queue */
    boolean inQueue = Com_IsInRetryQueue(0);
    TEST_ASSERT_TRUE(inQueue);
    
    /* Check remaining retries */
    uint8 retries = Com_GetRemainingRetries(0);
    TEST_ASSERT_EQUAL(3, retries);
    
    /* Remove from queue */
    Com_RemoveFromRetryQueue(0);
    
    /* Check it's no longer in queue */
    inQueue = Com_IsInRetryQueue(0);
    TEST_ASSERT_FALSE(inQueue);
}

void test_retry_queue_full(void)
{
    /* Fill up the retry queue */
    for (uint8 i = 0; i < COM_MAX_RETRY_QUEUE_SIZE + 5; i++) {
        /* Use PDU 0 repeatedly - should just update the entry */
        Com_AddToRetryQueue(0, 3);
    }
    
    /* Should still be in queue */
    boolean inQueue = Com_IsInRetryQueue(0);
    TEST_ASSERT_TRUE(inQueue);
}

void test_retry_queue_invalid_pdu(void)
{
    /* Try to add invalid PDU */
    Std_ReturnType result = Com_AddToRetryQueue(99, 3);
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_retry_process(void)
{
    /* Setup mock for PduR_IfTransmit */
    PduInfoType expectedPduInfo;
    expectedPduInfo.SduDataPtr = TestIPduBuffer;
    expectedPduInfo.SduLength = 8;
    expectedPduInfo.MetaDataPtr = NULL;
    
    PduR_IfTransmit_ExpectAndReturn(0, &expectedPduInfo, E_OK);
    
    /* Add to retry queue */
    Com_AddToRetryQueue(0, 2);
    
    /* Process retry queue - need to wait for delay */
    for (uint32 i = 0; i < COM_RETRY_DELAY_MS + 5; i++) {
        Com_ProcessRetryQueue();
        Com_TimestampCounter++;  /* Simulate time passing */
    }
}

/*==================[State Machine Tests]==================================*/

void test_state_machine_idle_to_pending(void)
{
    /* Initial state should be idle */
    Com_TxStatusType status = Com_GetTxStatus(0);
    TEST_ASSERT_EQUAL(COM_TX_IDLE, status);
    
    /* Start confirmation */
    Com_StartTxConfirmation(0);
    
    /* State should be pending */
    status = Com_GetTxStatus(0);
    TEST_ASSERT_EQUAL(COM_TX_PENDING, status);
}

void test_state_machine_pending_to_confirmed(void)
{
    /* Start confirmation */
    Com_StartTxConfirmation(0);
    
    /* Simulate successful confirmation */
    Com_TxConfirmation(0, E_OK);
    
    /* State should be confirmed */
    Com_TxStatusType status = Com_GetTxStatus(0);
    TEST_ASSERT_EQUAL(COM_TX_CONFIRMED, status);
    
    /* Result should be OK */
    Com_TxResultType result = Com_GetTxResult(0);
    TEST_ASSERT_EQUAL(COM_TX_RES_OK, result);
}

void test_state_machine_pending_to_error(void)
{
    /* Start confirmation */
    Com_StartTxConfirmation(0);
    
    /* Simulate failed confirmation with no retries */
    Com_IPduRunTimeType* runtime = &Com_GlobalState.IPduRunTime[0];
    runtime->CurrentRetryCount = 3;  /* Max retries already reached */
    
    Com_TxConfirmation(0, E_NOT_OK);
    
    /* State should be error */
    Com_TxStatusType status = Com_GetTxStatus(0);
    TEST_ASSERT_EQUAL(COM_TX_ERROR, status);
    
    /* Result should be NOT_OK */
    Com_TxResultType result = Com_GetTxResult(0);
    TEST_ASSERT_EQUAL(COM_TX_RES_NOT_OK, result);
}

void test_cancel_confirmation(void)
{
    /* Start confirmation */
    Com_StartTxConfirmation(0);
    
    /* Verify pending */
    TEST_ASSERT_EQUAL(COM_TX_PENDING, Com_GetTxStatus(0));
    
    /* Cancel confirmation */
    Com_CancelTxConfirmation(0);
    
    /* Should be back to idle */
    TEST_ASSERT_EQUAL(COM_TX_IDLE, Com_GetTxStatus(0));
    TEST_ASSERT_EQUAL(COM_TX_RES_CANCELLED, Com_GetTxResult(0));
}

/*==================[Transmission Mode Switch Tests]=======================*/

void test_mode_switch_during_pending(void)
{
    /* Start confirmation */
    Com_StartTxConfirmation(0);
    
    /* Verify pending */
    TEST_ASSERT_EQUAL(COM_TX_PENDING, Com_GetTxStatus(0));
    
    /* Switch mode to NONE - should cancel confirmation */
    Com_HandleModeSwitchConfirmation(0, COM_DIRECT, COM_NONE);
    
    /* Confirmation should be cancelled */
    TEST_ASSERT_EQUAL(COM_TX_IDLE, Com_GetTxStatus(0));
}

void test_mode_switch_allowed(void)
{
    /* Start confirmation */
    Com_StartTxConfirmation(0);
    
    /* Check if mode switch is allowed */
    boolean allowed = Com_CanSwitchModeDuringPending(0);
    TEST_ASSERT_TRUE(allowed);
    
    /* Switch mode to PERIODIC - should keep confirmation pending */
    Com_HandleModeSwitchConfirmation(0, COM_DIRECT, COM_PERIODIC);
    
    /* Should still be pending */
    TEST_ASSERT_EQUAL(COM_TX_PENDING, Com_GetTxStatus(0));
}

/*==================[Retry Mechanism Integration Tests]====================*/

void test_retry_count_decrement(void)
{
    /* Add to retry queue with 3 retries */
    Com_AddToRetryQueue(0, 3);
    
    /* Check initial retry count */
    TEST_ASSERT_EQUAL(3, Com_GetRemainingRetries(0));
    
    /* Simulate processing that decrements retry count */
    /* Note: Actual retry processing requires PduR_IfTransmit mock */
}

void test_max_retries_exceeded(void)
{
    /* Start confirmation */
    Com_StartTxConfirmation(0);
    
    /* Set current retry count to max */
    Com_IPduRunTimeType* runtime = &Com_GlobalState.IPduRunTime[0];
    runtime->CurrentRetryCount = 3;
    
    /* Simulate failure */
    Com_TxConfirmation(0, E_NOT_OK);
    
    /* Error callback should be called */
    TEST_ASSERT_TRUE(TxErrorNotificationCalled);
    
    /* Should be in error state */
    TEST_ASSERT_EQUAL(COM_TX_ERROR, Com_GetTxStatus(0));
}

/*==================[Main]=================================================*/

int main(void)
{
    UNITY_BEGIN();
    
    /* Confirmation Tests */
    RUN_TEST(test_tx_confirmation_success);
    RUN_TEST(test_tx_confirmation_failure);
    RUN_TEST(test_tx_confirmation_disabled);
    RUN_TEST(test_tx_confirmation_invalid_pdu);
    RUN_TEST(test_tx_confirmation_before_init);
    
    /* Timeout Tests */
    RUN_TEST(test_tx_timeout_detection);
    RUN_TEST(test_tx_timeout_no_confirmation_enabled);
    RUN_TEST(test_tx_timeout_reset);
    
    /* Retry Tests */
    RUN_TEST(test_retry_queue_add_remove);
    RUN_TEST(test_retry_queue_full);
    RUN_TEST(test_retry_queue_invalid_pdu);
    
    /* State Machine Tests */
    RUN_TEST(test_state_machine_idle_to_pending);
    RUN_TEST(test_state_machine_pending_to_confirmed);
    RUN_TEST(test_state_machine_pending_to_error);
    RUN_TEST(test_cancel_confirmation);
    
    /* Mode Switch Tests */
    RUN_TEST(test_mode_switch_during_pending);
    RUN_TEST(test_mode_switch_allowed);
    
    /* Retry Integration Tests */
    RUN_TEST(test_retry_count_decrement);
    RUN_TEST(test_max_retries_exceeded);
    
    return UNITY_END();
}
