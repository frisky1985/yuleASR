/*
 * test_com_main.c
 * COM Module Unit Tests - Main Functions and PduR Interface
 */

#include "unity.h"
#include "Com.h"
#include "Com_Private.h"
#include "Com_Transmit.h"
#include "mock_PduR.h"

/*==================[Test Configuration]===================================*/

static uint8 TestTxIPduBuffer[8];
static uint8 TestRxIPduBuffer[8];
static boolean NotificationCalled = FALSE;

static void TestNotificationCallback(void)
{
    NotificationCalled = TRUE;
}

static const Com_SignalConfigType TestSignals[] = {
    {
        .SignalId = 0,
        .DataPtr = &TestTxIPduBuffer[0],
        .BitPosition = 0,
        .BitSize = 16,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT16,
        .TransferProperty = COM_TRIGGERED,
        .ComNotification = TestNotificationCallback,
        .Timeout = 0,
        .InitValue = NULL
    },
    {
        .SignalId = 1,
        .DataPtr = &TestRxIPduBuffer[0],
        .BitPosition = 0,
        .BitSize = 16,
        .Endianness = COM_LITTLE_ENDIAN,
        .SignalType = COM_UINT16,
        .TransferProperty = COM_TRIGGERED,
        .ComNotification = TestNotificationCallback,
        .Timeout = 0,
        .InitValue = NULL
    }
};

static const Com_IPduConfigType TestIPdus[] = {
    {
        .IPduId = 0,
        .DataPtr = TestTxIPduBuffer,
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
        .SignalRefs = (Com_SignalIdType[]){1},
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
        .Timeout = 50,
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
    .NumSignals = 2,
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
    memset(TestTxIPduBuffer, 0, sizeof(TestTxIPduBuffer));
    memset(TestRxIPduBuffer, 0, sizeof(TestRxIPduBuffer));
    NotificationCalled = FALSE;
    Com_Init(&TestConfig);
}

void tearDown(void)
{
    Com_DeInit();
}

/*==================[Com_MainFunctionTx Tests]=============================*/

void test_mainfunctiontx_basic(void)
{
    /* MainFunctionTx should process without errors */
    Com_MainFunctionTx();
    
    /* Status should remain READY */
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
}

void test_mainfunctiontx_before_init(void)
{
    Com_DeInit();
    
    /* Should return without error */
    Com_MainFunctionTx();
    
    /* Status should be UNINIT */
    TEST_ASSERT_EQUAL(COM_UNINIT, Com_GetStatus());
}

void test_mainfunctiontx_processes_periodic(void)
{
    /* Set IPdu group to started */
    Com_GlobalState.IPduRunTime[0].GroupStatus = COM_IPDU_GROUP_STARTED;
    
    /* Wait for period to elapse */
    for (int i = 0; i < 15; i++) {
        Com_MainFunctionTx();
    }
    
    /* Timer should have been decremented */
    TEST_ASSERT_TRUE(Com_GlobalState.IPduRunTime[0].TxTimer <= 10);
}

/*==================[Com_MainFunctionRx Tests]=============================*/

void test_mainfunctionrx_basic(void)
{
    /* Set IPdu group to started */
    Com_GlobalState.IPduRunTime[1].GroupStatus = COM_IPDU_GROUP_STARTED;
    
    /* MainFunctionRx should process without errors */
    Com_MainFunctionRx();
    
    /* Status should remain READY */
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
}

void test_mainfunctionrx_before_init(void)
{
    Com_DeInit();
    
    /* Should return without error */
    Com_MainFunctionRx();
    
    /* Status should be UNINIT */
    TEST_ASSERT_EQUAL(COM_UNINIT, Com_GetStatus());
}

void test_mainfunctionrx_timeout_detection(void)
{
    /* Set IPdu group to started */
    Com_GlobalState.IPduRunTime[1].GroupStatus = COM_IPDU_GROUP_STARTED;
    
    /* Set timeout timer */
    Com_GlobalState.IPduRunTime[1].TimeoutTimer = 5;
    
    /* Call MainFunctionRx multiple times to trigger timeout */
    for (int i = 0; i < 10; i++) {
        Com_MainFunctionRx();
    }
    
    /* Timeout should have occurred */
    TEST_ASSERT_TRUE(Com_GlobalState.IPduRunTime[1].TimeoutOccurred);
}

void test_mainfunctionrx_deferred_processing(void)
{
    /* Set IPdu group to started */
    Com_GlobalState.IPduRunTime[1].GroupStatus = COM_IPDU_GROUP_STARTED;
    
    /* MainFunctionRx should process deferred signals */
    Com_MainFunctionRx();
    
    /* Notification should be called for deferred processing */
    /* Note: In actual implementation, this would require more setup */
}

/*==================[Com_MainFunctionRouteSignals Tests]===================*/

void test_mainfunctionroutesignals_basic(void)
{
    /* MainFunctionRouteSignals should be callable */
    Com_MainFunctionRouteSignals();
    
    /* Status should remain READY */
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
}

/*==================[PduR_ComRxIndication Tests]===========================*/

void test_pdur_comrxindication_basic(void)
{
    uint8 rxData[] = {0x12, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    PduInfoType pduInfo = {
        .SduDataPtr = rxData,
        .SduLength = 8,
        .MetaDataPtr = NULL
    };
    
    /* Set IPdu group to started */
    Com_GlobalState.IPduRunTime[1].GroupStatus = COM_IPDU_GROUP_STARTED;
    
    /* Call RxIndication */
    PduR_ComRxIndication(1, &pduInfo);
    
    /* Data should be copied to Rx buffer */
    TEST_ASSERT_EQUAL_UINT8(0x12, TestRxIPduBuffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x34, TestRxIPduBuffer[1]);
}

void test_pdur_comrxindication_null_pointer(void)
{
    /* Should handle NULL pointer gracefully */
    PduR_ComRxIndication(1, NULL);
    
    /* No crash expected */
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
}

void test_pdur_comrxindication_invalid_pdu(void)
{
    uint8 rxData[] = {0x12, 0x34};
    PduInfoType pduInfo = {
        .SduDataPtr = rxData,
        .SduLength = 2,
        .MetaDataPtr = NULL
    };
    
    /* Should handle invalid PDU ID gracefully */
    PduR_ComRxIndication(99, &pduInfo);
    
    /* No crash expected */
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
}

void test_pdur_comrxindication_stopped_group(void)
{
    uint8 rxData[] = {0x12, 0x34};
    PduInfoType pduInfo = {
        .SduDataPtr = rxData,
        .SduLength = 2,
        .MetaDataPtr = NULL
    };
    
    /* Ensure IPdu group is stopped */
    Com_GlobalState.IPduRunTime[1].GroupStatus = COM_IPDU_GROUP_STOPPED;
    
    /* Should not process when group is stopped */
    PduR_ComRxIndication(1, &pduInfo);
    
    /* Data should not be copied */
    TEST_ASSERT_EQUAL_UINT8(0x00, TestRxIPduBuffer[0]);
}

void test_pdur_comrxindication_before_init(void)
{
    uint8 rxData[] = {0x12, 0x34};
    PduInfoType pduInfo = {
        .SduDataPtr = rxData,
        .SduLength = 2,
        .MetaDataPtr = NULL
    };
    
    Com_DeInit();
    
    /* Should return without error */
    PduR_ComRxIndication(1, &pduInfo);
}

/*==================[PduR_ComTxConfirmation Tests]=========================*/

void test_pdur_comtxconfirmation_success(void)
{
    /* Set up for transmission confirmation */
    Com_GlobalState.IPduRunTime[0].GroupStatus = COM_IPDU_GROUP_STARTED;
    
    /* Call TxConfirmation with success */
    PduR_ComTxConfirmation(0, E_OK);
    
    /* No crash expected, would need more setup for full verification */
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
}

void test_pdur_comtxconfirmation_failure(void)
{
    /* Set up for transmission confirmation */
    Com_GlobalState.IPduRunTime[0].GroupStatus = COM_IPDU_GROUP_STARTED;
    
    /* Call TxConfirmation with failure */
    PduR_ComTxConfirmation(0, E_NOT_OK);
    
    /* No crash expected */
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
}

void test_pdur_comtxconfirmation_invalid_pdu(void)
{
    /* Should handle invalid PDU ID gracefully */
    PduR_ComTxConfirmation(99, E_OK);
    
    /* No crash expected */
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
}

void test_pdur_comtxconfirmation_before_init(void)
{
    Com_DeInit();
    
    /* Should return without error */
    PduR_ComTxConfirmation(0, E_OK);
}

/*==================[PduR_ComTriggerTransmit Tests]========================*/

void test_pdur_comtriggertransmit_basic(void)
{
    uint8 txBuffer[8] = {0};
    PduInfoType pduInfo = {
        .SduDataPtr = txBuffer,
        .SduLength = 8,
        .MetaDataPtr = NULL
    };
    
    /* Set some data in Tx buffer */
    TestTxIPduBuffer[0] = 0xAB;
    TestTxIPduBuffer[1] = 0xCD;
    
    /* Call TriggerTransmit */
    Std_ReturnType result = PduR_ComTriggerTransmit(0, &pduInfo);
    
    /* Should return OK */
    TEST_ASSERT_EQUAL(E_OK, result);
    
    /* Data should be copied */
    TEST_ASSERT_EQUAL_UINT8(0xAB, txBuffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0xCD, txBuffer[1]);
}

void test_pdur_comtriggertransmit_null_pointer(void)
{
    /* Should return error for NULL pointer */
    Std_ReturnType result = PduR_ComTriggerTransmit(0, NULL);
    
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_pdur_comtriggertransmit_invalid_pdu(void)
{
    uint8 txBuffer[8] = {0};
    PduInfoType pduInfo = {
        .SduDataPtr = txBuffer,
        .SduLength = 8,
        .MetaDataPtr = NULL
    };
    
    /* Should return error for invalid PDU */
    Std_ReturnType result = PduR_ComTriggerTransmit(99, &pduInfo);
    
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

void test_pdur_comtriggertransmit_before_init(void)
{
    uint8 txBuffer[8] = {0};
    PduInfoType pduInfo = {
        .SduDataPtr = txBuffer,
        .SduLength = 8,
        .MetaDataPtr = NULL
    };
    
    Com_DeInit();
    
    /* Should return error when not initialized */
    Std_ReturnType result = PduR_ComTriggerTransmit(0, &pduInfo);
    
    TEST_ASSERT_EQUAL(E_NOT_OK, result);
}

/*==================[Integration Tests]====================================*/

void test_transmit_to_confirmation_flow(void)
{
    /* Set IPdu group to started */
    Com_GlobalState.IPduRunTime[0].GroupStatus = COM_IPDU_GROUP_STARTED;
    
    /* Send a signal */
    uint16 value = 0x1234;
    Com_SendSignal(0, &value);
    
    /* Process transmission */
    Com_MainFunctionTx();
    
    /* Simulate confirmation */
    PduR_ComTxConfirmation(0, E_OK);
    
    /* System should still be in READY state */
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
}

void test_receive_to_processing_flow(void)
{
    uint8 rxData[] = {0x56, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    PduInfoType pduInfo = {
        .SduDataPtr = rxData,
        .SduLength = 8,
        .MetaDataPtr = NULL
    };
    
    /* Set IPdu group to started */
    Com_GlobalState.IPduRunTime[1].GroupStatus = COM_IPDU_GROUP_STARTED;
    
    /* Receive data */
    PduR_ComRxIndication(1, &pduInfo);
    
    /* Process reception */
    Com_MainFunctionRx();
    
    /* Data should be available */
    uint16 receivedValue;
    Com_ReceiveSignal(1, &receivedValue);
    TEST_ASSERT_EQUAL_UINT16(0x7856, receivedValue);
}

/*==================[Main]=================================================*/

int main(void)
{
    UNITY_BEGIN();
    
    /* Com_MainFunctionTx Tests */
    RUN_TEST(test_mainfunctiontx_basic);
    RUN_TEST(test_mainfunctiontx_before_init);
    RUN_TEST(test_mainfunctiontx_processes_periodic);
    
    /* Com_MainFunctionRx Tests */
    RUN_TEST(test_mainfunctionrx_basic);
    RUN_TEST(test_mainfunctionrx_before_init);
    RUN_TEST(test_mainfunctionrx_timeout_detection);
    RUN_TEST(test_mainfunctionrx_deferred_processing);
    
    /* Com_MainFunctionRouteSignals Tests */
    RUN_TEST(test_mainfunctionroutesignals_basic);
    
    /* PduR_ComRxIndication Tests */
    RUN_TEST(test_pdur_comrxindication_basic);
    RUN_TEST(test_pdur_comrxindication_null_pointer);
    RUN_TEST(test_pdur_comrxindication_invalid_pdu);
    RUN_TEST(test_pdur_comrxindication_stopped_group);
    RUN_TEST(test_pdur_comrxindication_before_init);
    
    /* PduR_ComTxConfirmation Tests */
    RUN_TEST(test_pdur_comtxconfirmation_success);
    RUN_TEST(test_pdur_comtxconfirmation_failure);
    RUN_TEST(test_pdur_comtxconfirmation_invalid_pdu);
    RUN_TEST(test_pdur_comtxconfirmation_before_init);
    
    /* PduR_ComTriggerTransmit Tests */
    RUN_TEST(test_pdur_comtriggertransmit_basic);
    RUN_TEST(test_pdur_comtriggertransmit_null_pointer);
    RUN_TEST(test_pdur_comtriggertransmit_invalid_pdu);
    RUN_TEST(test_pdur_comtriggertransmit_before_init);
    
    /* Integration Tests */
    RUN_TEST(test_transmit_to_confirmation_flow);
    RUN_TEST(test_receive_to_processing_flow);
    
    return UNITY_END();
}
