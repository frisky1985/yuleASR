/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : CanTp Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "test_framework.h"
#include "CanTp.h"
#include "mock_ecual.h"
#include "mock_det.h"

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static CanTp_ConfigType g_test_config;
static PduInfoType g_test_pdu;
static uint8_t g_test_data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

/*==================================================================================================
*                                      HELPER FUNCTIONS
==================================================================================================*/
static void setup_test_config(void)
{
    g_test_config.CanTpGeneral = NULL_PTR;
    g_test_config.CanTpNSa = NULL_PTR;
    g_test_config.CanTpNTa = NULL_PTR;
    g_test_config.CanTpNPdu = NULL_PTR;
}

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

/* Test: CanTp_Init with valid config */
TEST_CASE(cantp_init_valid_config)
{
    setup_test_config();
    
    CanTp_Init(&g_test_config);
    
    ASSERT_EQ(CANTP_INITIALIZED, CanTp_GetStatus());
    TEST_PASS();
}

/* Test: CanTp_Init with NULL config */
TEST_CASE(cantp_init_null_config)
{
    mock_Det_ReportError_set_return(E_OK);
    
    CanTp_Init(NULL_PTR);
    
    /* Should report DET error */
    ASSERT_EQ(CANTP_UNINITIALIZED, CanTp_GetStatus());
    TEST_PASS();
}

/* Test: CanTp_Transmit when not initialized */
TEST_CASE(cantp_transmit_not_initialized)
{
    Std_ReturnType result;
    PduInfoType pdu;
    
    CanTp_Uninit();
    pdu.SduDataPtr = g_test_data;
    pdu.SduLength = 8;
    
    result = CanTp_Transmit(0, &pdu);
    
    ASSERT_EQ(E_NOT_OK, result);
    TEST_PASS();
}

/* Test: CanTp_Transmit single frame */
TEST_CASE(cantp_transmit_single_frame)
{
    Std_ReturnType result;
    PduInfoType pdu;
    
    setup_test_config();
    CanTp_Init(&g_test_config);
    pdu.SduDataPtr = g_test_data;
    pdu.SduLength = 7;  /* Single frame max payload */
    
    result = CanTp_Transmit(0, &pdu);
    
    ASSERT_TRUE(result == E_OK || result == CAN_BUSY);
    TEST_PASS();
}

/* Test: CanTp_Shutdown */
TEST_CASE(cantp_shutdown)
{
    setup_test_config();
    CanTp_Init(&g_test_config);
    
    CanTp_Shutdown();
    
    ASSERT_EQ(CANTP_UNINITIALIZED, CanTp_GetStatus());
    TEST_PASS();
}

/* Test: CanTp_GetVersionInfo */
TEST_CASE(cantp_get_version_info)
{
    Std_VersionInfoType version;
    
    CanTp_GetVersionInfo(&version);
    
    ASSERT_EQ(CANTP_VENDOR_ID, version.vendorID);
    ASSERT_EQ(CANTP_MODULE_ID, version.moduleID);
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(cantp)
{
    mock_Det_Reset();
}

TEST_SUITE_TEARDOWN(cantp)
{
}

TEST_SUITE(cantp)
{
    RUN_TEST(cantp_init_valid_config);
    RUN_TEST(cantp_init_null_config);
    RUN_TEST(cantp_transmit_not_initialized);
    RUN_TEST(cantp_transmit_single_frame);
    RUN_TEST(cantp_shutdown);
    RUN_TEST(cantp_get_version_info);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
    RUN_TEST_SUITE(cantp);
TEST_MAIN_END()
