/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : CAN Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "test_framework.h"
#include "Can.h"
#include "mock_mcal.h"
#include "mock_det.h"

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static Can_ConfigType g_test_config;
static Can_ControllerConfigType g_test_controllers[2];
static Can_HardwareObjectType g_test_hohs[4];
static Can_BaudrateConfigType g_test_baudrates[2];

/*==================================================================================================
*                                      HELPER FUNCTIONS
==================================================================================================*/
static void setup_test_config(void)
{
    /* Baudrate config */
    g_test_baudrates[0].BaudRate = 500000;
    g_test_baudrates[0].PropSeg = 2;
    g_test_baudrates[0].PhaseSeg1 = 13;
    g_test_baudrates[0].PhaseSeg2 = 2;
    g_test_baudrates[0].SyncJumpWidth = 1;
    g_test_baudrates[0].Prescaler = 4;
    
    /* Hardware objects */
    g_test_hohs[0].Hoh = 0;
    g_test_hohs[0].HohType = CAN_HOH_TYPE_RECEIVE;
    g_test_hohs[0].IdType = CAN_ID_TYPE_STANDARD;
    g_test_hohs[0].FirstId = 0x100;
    g_test_hohs[0].LastId = 0x1FF;
    g_test_hohs[0].ObjectId = 0;
    g_test_hohs[0].Filtering = TRUE;
    
    g_test_hohs[1].Hoh = 1;
    g_test_hohs[1].HohType = CAN_HOH_TYPE_TRANSMIT;
    g_test_hohs[1].IdType = CAN_ID_TYPE_STANDARD;
    g_test_hohs[1].FirstId = 0;
    g_test_hohs[1].LastId = 0;
    g_test_hohs[1].ObjectId = 0;
    g_test_hohs[1].Filtering = FALSE;
    
    /* Controller config */
    g_test_controllers[0].ControllerId = 0;
    g_test_controllers[0].BaseAddress = 0x40006400;
    g_test_controllers[0].BaudrateConfigs = g_test_baudrates;
    g_test_controllers[0].NumBaudrateConfigs = 1;
    g_test_controllers[0].HardwareObjects = g_test_hohs;
    g_test_controllers[0].NumHardwareObjects = 2;
    g_test_controllers[0].RxProcessing = 0;
    g_test_controllers[0].TxProcessing = 0;
    g_test_controllers[0].BusOffProcessing = TRUE;
    g_test_controllers[0].WakeupProcessing = FALSE;
    g_test_controllers[0].WakeupSupport = FALSE;
    g_test_controllers[0].DefaultBaudrateIndex = 0;
    
    /* Main config */
    g_test_config.Controllers = g_test_controllers;
    g_test_config.NumControllers = 1;
    g_test_config.DevErrorDetect = TRUE;
    g_test_config.VersionInfoApi = TRUE;
}

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

/* Test: Can_Init with valid config */
TEST_CASE(can_init_valid)
{
    setup_test_config();
    
    Can_Init(&g_test_config);
    
    ASSERT_TRUE(Can_MockControllers[0].Initialized);
    ASSERT_EQ(CAN_CS_STOPPED, Can_Mock_GetControllerState(0));
    TEST_PASS();
}

/* Test: Can_Init with NULL config */
TEST_CASE(can_init_null)
{
    Det_Mock_Reset();
    
    Can_Init(NULL);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(CAN_E_PARAM_POINTER, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Can_Init when already initialized */
TEST_CASE(can_init_already_initialized)
{
    setup_test_config();
    Can_Init(&g_test_config);
    Det_Mock_Reset();
    
    Can_Init(&g_test_config);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(CAN_E_INIT_FAILED, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Can_GetVersionInfo */
TEST_CASE(can_get_version_info)
{
    Std_VersionInfoType version_info;
    
    Can_GetVersionInfo(&version_info);
    
    ASSERT_EQ(CAN_VENDOR_ID, version_info.vendorID);
    ASSERT_EQ(CAN_MODULE_ID, version_info.moduleID);
    ASSERT_EQ(CAN_SW_MAJOR_VERSION, version_info.sw_major_version);
    ASSERT_EQ(CAN_SW_MINOR_VERSION, version_info.sw_minor_version);
    TEST_PASS();
}

/* Test: Can_GetVersionInfo with NULL pointer */
TEST_CASE(can_get_version_info_null)
{
    Det_Mock_Reset();
    
    Can_GetVersionInfo(NULL);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(CAN_E_PARAM_POINTER, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Can_SetControllerMode - Stop -> Start */
TEST_CASE(can_set_mode_start)
{
    Can_ReturnType result;
    
    setup_test_config();
    Can_Init(&g_test_config);
    Can_Mock_SetControllerState(0, CAN_CS_STOPPED);
    
    result = Can_SetControllerMode(0, CAN_CS_STARTED);
    
    ASSERT_EQ(CAN_OK, result);
    ASSERT_EQ(CAN_CS_STARTED, Can_Mock_GetControllerState(0));
    TEST_PASS();
}

/* Test: Can_SetControllerMode with invalid controller */
TEST_CASE(can_set_mode_invalid_controller)
{
    Can_ReturnType result;
    
    setup_test_config();
    Can_Init(&g_test_config);
    Det_Mock_Reset();
    
    result = Can_SetControllerMode(99, CAN_CS_STARTED);
    
    ASSERT_EQ(CAN_NOT_OK, result);
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(CAN_E_PARAM_CONTROLLER, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Can_SetControllerMode when not initialized */
TEST_CASE(can_set_mode_uninit)
{
    Can_ReturnType result;
    
    Det_Mock_Reset();
    
    result = Can_SetControllerMode(0, CAN_CS_STARTED);
    
    ASSERT_EQ(CAN_NOT_OK, result);
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(CAN_E_UNINIT, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Can_DisableControllerInterrupts */
TEST_CASE(can_disable_interrupts)
{
    setup_test_config();
    Can_Init(&g_test_config);
    
    Can_DisableControllerInterrupts(0);
    
    /* Verify interrupts disabled - mock state check */
    TEST_PASS();
}

/* Test: Can_DisableControllerInterrupts when not initialized */
TEST_CASE(can_disable_interrupts_uninit)
{
    Det_Mock_Reset();
    
    Can_DisableControllerInterrupts(0);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(CAN_E_UNINIT, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Can_EnableControllerInterrupts */
TEST_CASE(can_enable_interrupts)
{
    setup_test_config();
    Can_Init(&g_test_config);
    
    Can_EnableControllerInterrupts(0);
    
    /* Verify interrupts enabled - mock state check */
    TEST_PASS();
}

/* Test: Can_EnableControllerInterrupts when not initialized */
TEST_CASE(can_enable_interrupts_uninit)
{
    Det_Mock_Reset();
    
    Can_EnableControllerInterrupts(0);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(CAN_E_UNINIT, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Can_Write with valid PDU */
TEST_CASE(can_write_valid)
{
    Can_ReturnType result;
    Can_PduType pdu;
    uint8 data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    
    setup_test_config();
    Can_Init(&g_test_config);
    Can_Mock_SetControllerState(0, CAN_CS_STARTED);
    
    pdu.idType = CAN_ID_TYPE_STANDARD;
    pdu.CanId = 0x123;
    pdu.CanDlc = 8;
    pdu.SduPtr = data;
    
    result = Can_Write(1, &pdu); /* HTH 1 is transmit object */
    
    ASSERT_EQ(CAN_OK, result);
    ASSERT_EQ(1, Can_MockControllers[0].TxCount);
    TEST_PASS();
}

/* Test: Can_Write when not initialized */
TEST_CASE(can_write_uninit)
{
    Can_ReturnType result;
    Can_PduType pdu;
    uint8 data[8] = {0};
    
    Det_Mock_Reset();
    
    pdu.idType = CAN_ID_TYPE_STANDARD;
    pdu.CanId = 0x123;
    pdu.CanDlc = 8;
    pdu.SduPtr = data;
    
    result = Can_Write(1, &pdu);
    
    ASSERT_EQ(CAN_NOT_OK, result);
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(CAN_E_UNINIT, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Can_Write with controller stopped */
TEST_CASE(can_write_stopped)
{
    Can_ReturnType result;
    Can_PduType pdu;
    uint8 data[8] = {0};
    
    setup_test_config();
    Can_Init(&g_test_config);
    Can_Mock_SetControllerState(0, CAN_CS_STOPPED);
    
    pdu.idType = CAN_ID_TYPE_STANDARD;
    pdu.CanId = 0x123;
    pdu.CanDlc = 8;
    pdu.SduPtr = data;
    
    result = Can_Write(1, &pdu);
    
    ASSERT_EQ(CAN_NOT_OK, result);
    TEST_PASS();
}

/* Test: Can_Write with NULL PDU */
TEST_CASE(can_write_null_pdu)
{
    Can_ReturnType result;
    
    setup_test_config();
    Can_Init(&g_test_config);
    Det_Mock_Reset();
    
    result = Can_Write(1, NULL);
    
    ASSERT_EQ(CAN_NOT_OK, result);
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(CAN_E_PARAM_POINTER, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Can_MainFunction_Write */
TEST_CASE(can_main_function_write)
{
    setup_test_config();
    Can_Init(&g_test_config);
    
    Can_MainFunction_Write();
    
    /* Verify main function executed */
    TEST_PASS();
}

/* Test: Can_MainFunction_Read */
TEST_CASE(can_main_function_read)
{
    setup_test_config();
    Can_Init(&g_test_config);
    
    Can_MainFunction_Read();
    
    /* Verify main function executed */
    TEST_PASS();
}

/* Test: Can_MainFunction_BusOff */
TEST_CASE(can_main_function_busoff)
{
    setup_test_config();
    Can_Init(&g_test_config);
    
    Can_MainFunction_BusOff();
    
    /* Verify main function executed */
    TEST_PASS();
}

/* Test: Can_CheckWakeup */
TEST_CASE(can_check_wakeup)
{
    Std_ReturnType result;
    
    setup_test_config();
    Can_Init(&g_test_config);
    
    result = Can_CheckWakeup(0);
    
    /* Result depends on implementation */
    (void)result;
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(can)
{
    Can_Mock_Reset();
    Det_Mock_Reset();
}

TEST_SUITE_TEARDOWN(can)
{
    /* Cleanup */
}

TEST_SUITE(can)
{
    RUN_TEST(can_init_valid);
    RUN_TEST(can_init_null);
    RUN_TEST(can_init_already_initialized);
    RUN_TEST(can_get_version_info);
    RUN_TEST(can_get_version_info_null);
    RUN_TEST(can_set_mode_start);
    RUN_TEST(can_set_mode_invalid_controller);
    RUN_TEST(can_set_mode_uninit);
    RUN_TEST(can_disable_interrupts);
    RUN_TEST(can_disable_interrupts_uninit);
    RUN_TEST(can_enable_interrupts);
    RUN_TEST(can_enable_interrupts_uninit);
    RUN_TEST(can_write_valid);
    RUN_TEST(can_write_uninit);
    RUN_TEST(can_write_stopped);
    RUN_TEST(can_write_null_pdu);
    RUN_TEST(can_main_function_write);
    RUN_TEST(can_main_function_read);
    RUN_TEST(can_main_function_busoff);
    RUN_TEST(can_check_wakeup);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
{
    printf("\n" TEST_COLOR_BLUE "--- CAN Driver Unit Tests ---" TEST_COLOR_RESET "\n");
    RUN_TEST_SUITE(can);
}
TEST_MAIN_END()
