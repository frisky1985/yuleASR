/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : CanIf Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "test_framework.h"
#include "CanIf.h"
#include "mock_ecual.h"
#include "mock_det.h"

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static CanIf_ConfigType g_test_config;
static Can_PduType g_test_pdu;
static uint8_t g_test_data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

/*==================================================================================================
*                                      HELPER FUNCTIONS
==================================================================================================*/
static void setup_test_config(void)
{
    g_test_config.CanIfInitConfig = NULL_PTR;
    g_test_config.CanIfTxPduConfig = NULL_PTR;
    g_test_config.CanIfRxPduConfig = NULL_PTR;
}

static void setup_test_pdu(void)
{
    g_test_pdu.swPduHandle = 0;
    g_test_pdu.length = 8;
    g_test_pdu.sdu = g_test_data;
    g_test_pdu.id = 0x123;
}

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

/* Test: CanIf_Init with valid config */
TEST_CASE(canif_init_valid_config)
{
    setup_test_config();
    
    CanIf_Init(&g_test_config);
    
    ASSERT_EQ(CANIF_INITIALIZED, CanIf_GetStatus());
    TEST_PASS();
}

/* Test: CanIf_Init with NULL config (should trigger DET) */
TEST_CASE(canif_init_null_config)
{
    /* Expect DET error */
    mock_Det_ReportError_set_return(E_OK);
    
    CanIf_Init(NULL_PTR);
    
    ASSERT_EQ(CANIF_UNINITIALIZED, CanIf_GetStatus());
    TEST_PASS();
}

/* Test: CanIf_Transmit when not initialized */
TEST_CASE(canif_transmit_not_initialized)
{
    Std_ReturnType result;
    PduInfoType pdu;
    
    CanIf_Uninit();
    pdu.SduDataPtr = g_test_data;
    pdu.SduLength = 8;
    
    result = CanIf_Transmit(0, &pdu);
    
    ASSERT_EQ(E_NOT_OK, result);
    TEST_PASS();
}

/* Test: CanIf_Transmit with valid PDU */
TEST_CASE(canif_transmit_valid_pdu)
{
    Std_ReturnType result;
    PduInfoType pdu;
    
    setup_test_config();
    CanIf_Init(&g_test_config);
    pdu.SduDataPtr = g_test_data;
    pdu.SduLength = 8;
    
    result = CanIf_Transmit(0, &pdu);
    
    ASSERT_TRUE(result == E_OK || result == CAN_BUSY);
    TEST_PASS();
}

/* Test: CanIf_Transmit with NULL PDU */
TEST_CASE(canif_transmit_null_pdu)
{
    Std_ReturnType result;
    
    setup_test_config();
    CanIf_Init(&g_test_config);
    
    result = CanIf_Transmit(0, NULL_PTR);
    
    ASSERT_EQ(E_NOT_OK, result);
    TEST_PASS();
}

/* Test: CanIf_SetControllerMode */
TEST_CASE(canif_set_controller_mode)
{
    Std_ReturnType result;
    
    setup_test_config();
    CanIf_Init(&g_test_config);
    
    result = CanIf_SetControllerMode(0, CAN_CS_STARTED);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/* Test: CanIf_GetControllerMode */
TEST_CASE(canif_get_controller_mode)
{
    Std_ReturnType result;
    Can_ControllerStateType mode;
    
    setup_test_config();
    CanIf_Init(&g_test_config);
    CanIf_SetControllerMode(0, CAN_CS_STARTED);
    
    result = CanIf_GetControllerMode(0, &mode);
    
    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(canif)
{
    mock_Det_Reset();
}

TEST_SUITE_TEARDOWN(canif)
{
}

TEST_SUITE(canif)
{
    RUN_TEST(canif_init_valid_config);
    RUN_TEST(canif_init_null_config);
    RUN_TEST(canif_transmit_not_initialized);
    RUN_TEST(canif_transmit_valid_pdu);
    RUN_TEST(canif_transmit_null_pdu);
    RUN_TEST(canif_set_controller_mode);
    RUN_TEST(canif_get_controller_mode);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
    RUN_TEST_SUITE(canif);
TEST_MAIN_END()
