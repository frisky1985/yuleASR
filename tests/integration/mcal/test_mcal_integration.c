/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : MCAL Integration Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-23
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "test_framework.h"
#include "Mcu.h"
#include "Port.h"
#include "Dio.h"
#include "mock_mcal.h"

/*==================================================================================================
*                                      INTEGRATION TESTS
==================================================================================================*/

/* Test: MCU -> Port initialization sequence */
TEST_CASE(integration_mcu_port_init)
{
    Mcu_ConfigType mcu_config;
    Port_ConfigType port_config;
    Port_PinConfigType port_pins[2];
    
    /* Initialize MCU first */
    mcu_config.ClockSetting = 0;
    mcu_config.ClockFrequency = 16000000;
    mcu_config.PllMultiplier = 16;
    mcu_config.PllDivider = 1;
    mcu_config.PllEnabled = TRUE;
    
    Mcu_Init(&mcu_config);
    Mcu_InitClock(0);
    Mcu_DistributePllClock();
    
    /* Then initialize Port */
    port_pins[0].Pin = 0;
    port_pins[0].Direction = PORT_PIN_OUT;
    port_pins[0].Mode = 0;
    port_pins[0].DirectionChangeable = FALSE;
    port_pins[0].ModeChangeable = FALSE;
    port_pins[0].InitialLevel = PORT_PIN_LEVEL_LOW;
    port_pins[0].PullUpEnable = FALSE;
    port_pins[0].PullDownEnable = FALSE;
    
    port_config.NumPins = 1;
    port_config.PinConfig = port_pins;
    
    Port_Init(&port_config);
    
    /* Verify both are initialized */
    ASSERT_EQ(MCU_CLOCK_INITIALIZED, Mcu_MockState);
    ASSERT_TRUE(Port_MockPinStates[0].Initialized);
    TEST_PASS();
}

/* Test: Port -> DIO interaction */
TEST_CASE(integration_port_dio)
{
    Port_ConfigType port_config;
    Port_PinConfigType port_pins[1];
    Dio_LevelType level;
    
    /* Setup port as output */
    port_pins[0].Pin = 0;
    port_pins[0].Direction = PORT_PIN_OUT;
    port_pins[0].Mode = 0;
    port_pins[0].DirectionChangeable = FALSE;
    port_pins[0].ModeChangeable = FALSE;
    port_pins[0].InitialLevel = PORT_PIN_LEVEL_LOW;
    port_pins[0].PullUpEnable = FALSE;
    port_pins[0].PullDownEnable = FALSE;
    
    port_config.NumPins = 1;
    port_config.PinConfig = port_pins;
    
    Port_Init(&port_config);
    
    /* Use DIO to write */
    Dio_WriteChannel(0, STD_HIGH);
    
    /* Verify through DIO read */
    level = Dio_ReadChannel(0);
    
    ASSERT_EQ(STD_HIGH, level);
    ASSERT_EQ(PORT_PIN_OUT, Port_MockPinStates[0].Direction);
    TEST_PASS();
}

/* Test: MCU clock change affects other drivers */
TEST_CASE(integration_mcu_clock_change)
{
    Mcu_ConfigType mcu_config;
    uint32 freq_before, freq_after;
    
    mcu_config.ClockSetting = 0;
    mcu_config.ClockFrequency = 16000000;
    mcu_config.PllMultiplier = 16;
    mcu_config.PllDivider = 1;
    mcu_config.PllEnabled = TRUE;
    
    Mcu_Init(&mcu_config);
    
    freq_before = Mcu_GetClockFrequency(0);
    ASSERT_EQ(16000000, freq_before);
    
    /* Change clock setting */
    mcu_config.ClockFrequency = 32000000;
    mcu_config.PllMultiplier = 32;
    
    Mcu_InitClock(1);
    
    freq_after = Mcu_GetClockFrequency(0);
    ASSERT_EQ(32000000, freq_after);
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(mcal_integration)
{
    Mcu_Mock_Reset();
    Port_Mock_Reset();
    Dio_Mock_Reset();
}

TEST_SUITE_TEARDOWN(mcal_integration)
{
}

TEST_SUITE(mcal_integration)
{
    RUN_TEST(integration_mcu_port_init);
    RUN_TEST(integration_port_dio);
    RUN_TEST(integration_mcu_clock_change);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
{
    printf("\n" TEST_COLOR_BLUE "--- MCAL Integration Tests ---" TEST_COLOR_RESET "\n");
    RUN_TEST_SUITE(mcal_integration);
}
TEST_MAIN_END()
