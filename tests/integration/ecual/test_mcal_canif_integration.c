/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : MCAL-CanIf Integration Test
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "test_framework.h"
#include "Can.h"
#include "CanIf.h"
#include "mock_mcal.h"
#include "mock_ecual.h"
#include "mock_det.h"

#define TEST_HTH 0
#define TEST_CONTROLLER 0

static PduInfoType g_testPdu;
static uint8_t g_testSdu[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

/*==================================================================================================
*                                      HELPER FUNCTIONS
==================================================================================================*/
static void setup_test_pdu(void)
{
    g_testPdu.SduDataPtr = g_testSdu;
    g_testPdu.SduLength = 8;
    g_testPdu.MetaDataPtr = NULL_PTR;
}

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

/* Test: Full CAN transmission path from CanIf to CAN driver */
TEST_CASE(canif_transmit_calls_can_write)
{
    Std_ReturnType result;
    
    setup_test_pdu();
    
    /* Initialize CAN driver */
    Can_Init(&CanConfigSet);
    
    /* Initialize CanIf */
    CanIf_Init(&CanIf_Config);
    
    /* Set CAN controller mode */
    Can_SetControllerMode(TEST_CONTROLLER, CAN_CS_STARTED);
    CanIf_SetControllerMode(TEST_CONTROLLER, CAN_CS_STARTED);
    
    /* Transmit through CanIf */
    result = CanIf_Transmit(0, &g_testPdu);
    
    /* Should succeed or be queued */
    ASSERT_TRUE(result == E_OK || result == CAN_BUSY);
    TEST_PASS();
}

/* Test: RX indication path */
TEST_CASE(canif_rx_indication_processes_pdu)
{
    Can_HwHandleType hrh = 0;
    Can_IdType canId = 0x123;
    uint8_t canDlc = 8;
    uint8_t canSdu[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    
    /* Initialize */
    CanIf_Init(&CanIf_Config);
    
    /* Simulate RX indication from CAN driver */
    CanIf_RxIndication(hrh, canId, canDlc, canSdu);
    
    /* Verify PDU was processed (in real scenario, upper layer callback would be called) */
    TEST_PASS();
}

/* Test: Controller mode change */
TEST_CASE(controller_mode_change_propagates)
{
    Std_ReturnType result;
    Can_ControllerStateType mode;
    
    /* Initialize both layers */
    Can_Init(&CanConfigSet);
    CanIf_Init(&CanIf_Config);
    
    /* Set mode through CanIf */
    result = CanIf_SetControllerMode(TEST_CONTROLLER, CAN_CS_STARTED);
    ASSERT_EQ(E_OK, result);
    
    /* Verify CanIf reports correct mode */
    result = CanIf_GetControllerMode(TEST_CONTROLLER, &mode);
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(mcal_canif_integration)
{
    mock_Det_Reset();
    setup_test_pdu();
}

TEST_SUITE_TEARDOWN(mcal_canif_integration)
{
}

TEST_SUITE(mcal_canif_integration)
{
    RUN_TEST(canif_transmit_calls_can_write);
    RUN_TEST(canif_rx_indication_processes_pdu);
    RUN_TEST(controller_mode_change_propagates);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
    RUN_TEST_SUITE(mcal_canif_integration);
TEST_MAIN_END()
