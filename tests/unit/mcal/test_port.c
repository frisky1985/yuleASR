/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Port Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "test_framework.h"
#include "Port.h"
#include "mock_mcal.h"
#include "mock_det.h"

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static Port_ConfigType g_test_config;
static Port_PinConfigType g_test_pins[4];

/*==================================================================================================
*                                      HELPER FUNCTIONS
==================================================================================================*/
static void setup_test_config(void)
{
    g_test_pins[0].Pin = 0;
    g_test_pins[0].Direction = PORT_PIN_OUT;
    g_test_pins[0].Mode = 0;
    g_test_pins[0].DirectionChangeable = FALSE;
    g_test_pins[0].ModeChangeable = FALSE;
    g_test_pins[0].InitialLevel = PORT_PIN_LEVEL_LOW;
    g_test_pins[0].PullUpEnable = FALSE;
    g_test_pins[0].PullDownEnable = FALSE;

    g_test_pins[1].Pin = 1;
    g_test_pins[1].Direction = PORT_PIN_IN;
    g_test_pins[1].Mode = 0;
    g_test_pins[1].DirectionChangeable = TRUE;
    g_test_pins[1].ModeChangeable = TRUE;
    g_test_pins[1].InitialLevel = PORT_PIN_LEVEL_LOW;
    g_test_pins[1].PullUpEnable = TRUE;
    g_test_pins[1].PullDownEnable = FALSE;

    g_test_config.NumPins = 2;
    g_test_config.PinConfig = g_test_pins;
}

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

/* Test: Port_Init with valid config */
TEST_CASE(port_init_valid_config)
{
    setup_test_config();
    
    Port_Init(&g_test_config);
    
    /* Verify pins are configured */
    ASSERT_TRUE(Port_MockPinStates[0].Initialized);
    ASSERT_TRUE(Port_MockPinStates[1].Initialized);
    TEST_PASS();
}

/* Test: Port_Init with NULL config */
TEST_CASE(port_init_null_config)
{
    Det_Mock_Reset();
    
    Port_Init(NULL);
    
    /* Verify DET error was reported */
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(PORT_MODULE_ID, Det_MockData.ModuleId);
    ASSERT_EQ(PORT_E_PARAM_CONFIG, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Port_Init twice without deinit */
TEST_CASE(port_init_twice)
{
    setup_test_config();
    
    Port_Init(&g_test_config);
    Det_Mock_Reset();
    
    /* Second init should report error */
    Port_Init(&g_test_config);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(PORT_E_ALREADY_INITIALIZED, Det_MockData.ErrorId);
    TEST_PASS();
}

#if (PORT_SET_PIN_DIRECTION_API == STD_ON)
/* Test: Port_SetPinDirection with valid pin */
TEST_CASE(port_set_pin_direction_valid)
{
    setup_test_config();
    Port_Init(&g_test_config);
    
    /* Pin 1 has DirectionChangeable = TRUE */
    Port_SetPinDirection(1, PORT_PIN_OUT);
    
    ASSERT_EQ(PORT_PIN_OUT, Port_MockPinStates[1].Direction);
    TEST_PASS();
}

/* Test: Port_SetPinDirection with unchangeable pin */
TEST_CASE(port_set_pin_direction_unchangeable)
{
    setup_test_config();
    Port_Init(&g_test_config);
    Det_Mock_Reset();
    
    /* Pin 0 has DirectionChangeable = FALSE */
    Port_SetPinDirection(0, PORT_PIN_IN);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(PORT_E_DIRECTION_UNCHANGEABLE, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Port_SetPinDirection with invalid pin */
TEST_CASE(port_set_pin_direction_invalid)
{
    setup_test_config();
    Port_Init(&g_test_config);
    Det_Mock_Reset();
    
    Port_SetPinDirection(999, PORT_PIN_OUT);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(PORT_E_PARAM_PIN, Det_MockData.ErrorId);
    TEST_PASS();
}
#endif /* PORT_SET_PIN_DIRECTION_API */

/* Test: Port_RefreshPortDirection */
TEST_CASE(port_refresh_port_direction)
{
    setup_test_config();
    Port_Init(&g_test_config);
    
    /* Change mock pin direction to verify refresh restores it */
    Port_MockPinStates[0].Direction = PORT_PIN_IN;
    
    Port_RefreshPortDirection();
    
    /* Direction should be restored to configured value */
    ASSERT_EQ(PORT_PIN_OUT, Port_MockPinStates[0].Direction);
    TEST_PASS();
}

/* Test: Port_RefreshPortDirection when not initialized */
TEST_CASE(port_refresh_port_direction_uninit)
{
    Det_Mock_Reset();
    
    Port_RefreshPortDirection();
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(PORT_E_UNINIT, Det_MockData.ErrorId);
    TEST_PASS();
}

#if (PORT_VERSION_INFO_API == STD_ON)
/* Test: Port_GetVersionInfo with valid pointer */
TEST_CASE(port_get_version_info_valid)
{
    Std_VersionInfoType version_info;
    
    Port_GetVersionInfo(&version_info);
    
    ASSERT_EQ(PORT_VENDOR_ID, version_info.vendorID);
    ASSERT_EQ(PORT_MODULE_ID, version_info.moduleID);
    TEST_PASS();
}

/* Test: Port_GetVersionInfo with NULL pointer */
TEST_CASE(port_get_version_info_null)
{
    Det_Mock_Reset();
    
    Port_GetVersionInfo(NULL);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(PORT_E_PARAM_POINTER, Det_MockData.ErrorId);
    TEST_PASS();
}
#endif /* PORT_VERSION_INFO_API */

#if (PORT_SET_PIN_MODE_API == STD_ON)
/* Test: Port_SetPinMode with valid parameters */
TEST_CASE(port_set_pin_mode_valid)
{
    setup_test_config();
    Port_Init(&g_test_config);
    
    /* Pin 1 has ModeChangeable = TRUE */
    Port_SetPinMode(1, 2);
    
    ASSERT_EQ(2, Port_MockPinStates[1].Mode);
    TEST_PASS();
}

/* Test: Port_SetPinMode with unchangeable mode */
TEST_CASE(port_set_pin_mode_unchangeable)
{
    setup_test_config();
    Port_Init(&g_test_config);
    Det_Mock_Reset();
    
    /* Pin 0 has ModeChangeable = FALSE */
    Port_SetPinMode(0, 1);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(PORT_E_MODE_UNCHANGEABLE, Det_MockData.ErrorId);
    TEST_PASS();
}

/* Test: Port_SetPinMode with invalid mode */
TEST_CASE(port_set_pin_mode_invalid)
{
    setup_test_config();
    Port_Init(&g_test_config);
    Det_Mock_Reset();
    
    Port_SetPinMode(1, 255);
    
    ASSERT_EQ(1, Det_MockData.CallCount);
    ASSERT_EQ(PORT_E_PARAM_INVALID_MODE, Det_MockData.ErrorId);
    TEST_PASS();
}
#endif /* PORT_SET_PIN_MODE_API */

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(port)
{
    Port_Mock_Reset();
    Det_Mock_Reset();
    memset(g_test_pins, 0, sizeof(g_test_pins));
}

TEST_SUITE_TEARDOWN(port)
{
    /* Cleanup after all tests */
}

TEST_SUITE(port)
{
    RUN_TEST(port_init_valid_config);
    RUN_TEST(port_init_null_config);
    RUN_TEST(port_init_twice);
#if (PORT_SET_PIN_DIRECTION_API == STD_ON)
    RUN_TEST(port_set_pin_direction_valid);
    RUN_TEST(port_set_pin_direction_unchangeable);
    RUN_TEST(port_set_pin_direction_invalid);
#endif
    RUN_TEST(port_refresh_port_direction);
    RUN_TEST(port_refresh_port_direction_uninit);
#if (PORT_VERSION_INFO_API == STD_ON)
    RUN_TEST(port_get_version_info_valid);
    RUN_TEST(port_get_version_info_null);
#endif
#if (PORT_SET_PIN_MODE_API == STD_ON)
    RUN_TEST(port_set_pin_mode_valid);
    RUN_TEST(port_set_pin_mode_unchangeable);
    RUN_TEST(port_set_pin_mode_invalid);
#endif
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
{
    printf("\n" TEST_COLOR_BLUE "--- Port Driver Unit Tests ---" TEST_COLOR_RESET "\n");
    RUN_TEST_SUITE(port);
}
TEST_MAIN_END()
