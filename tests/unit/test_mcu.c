/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : Mcu Unit Test
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-19
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "test_framework.h"
#include "Mcu.h"

/* Mock variables */
static Mcu_ConfigType g_test_config;
static uint8 g_mcu_state = MCU_STATE_UNINIT;

/* Test: Mcu_Init with valid config */
TEST_CASE(test_mcu_init_valid_config)
{
    /* Setup */
    g_test_config.ClockSettings = NULL;
    g_test_config.RamSectorSettings = NULL;
    g_test_config.NumClockSettings = 0;
    g_test_config.NumRamSectors = 0;
    
    /* Execute */
    Mcu_Init(&g_test_config);
    
    /* Verify */
    ASSERT_EQ(MCU_STATE_INIT, g_mcu_state);
    TEST_PASS();
}

/* Test: Mcu_Init with NULL config */
TEST_CASE(test_mcu_init_null_config)
{
    /* Execute */
    Mcu_Init(NULL);
    
    /* Verify - should report error but not crash */
    TEST_PASS();
}

/* Test: Mcu_DeInit */
TEST_CASE(test_mcu_deinit)
{
    /* Setup */
    Mcu_Init(&g_test_config);
    
    /* Execute */
    Mcu_DeInit();
    
    /* Verify */
    ASSERT_EQ(MCU_STATE_UNINIT, g_mcu_state);
    TEST_PASS();
}

/* Test: Mcu_GetVersionInfo */
TEST_CASE(test_mcu_get_version_info)
{
    Std_VersionInfoType version_info;
    
    /* Execute */
    Mcu_GetVersionInfo(&version_info);
    
    /* Verify */
    ASSERT_EQ(MCU_VENDOR_ID, version_info.vendorID);
    ASSERT_EQ(MCU_SW_MAJOR_VERSION, version_info.sw_major_version);
    ASSERT_EQ(MCU_SW_MINOR_VERSION, version_info.sw_minor_version);
    TEST_PASS();
}

/* Test: Mcu_GetVersionInfo with NULL pointer */
TEST_CASE(test_mcu_get_version_info_null)
{
    /* Execute */
    Mcu_GetVersionInfo(NULL);
    
    /* Verify - should report error but not crash */
    TEST_PASS();
}

/* Test: Mcu_DistributePllClock when not initialized */
TEST_CASE(test_mcu_distribute_pll_not_init)
{
    /* Setup */
    g_mcu_state = MCU_STATE_UNINIT;
    
    /* Execute */
    Mcu_DistributePllClock();
    
    /* Verify */
    TEST_PASS();
}

/* Test: Mcu_GetPllStatus when not initialized */
TEST_CASE(test_mcu_get_pll_status_not_init)
{
    Mcu_PllStatusType status;
    
    /* Setup */
    g_mcu_state = MCU_STATE_UNINIT;
    
    /* Execute */
    status = Mcu_GetPllStatus();
    
    /* Verify */
    ASSERT_EQ(MCU_PLL_STATUS_UNDEFINED, status);
    TEST_PASS();
}

/* Test: Mcu_GetResetReason when not initialized */
TEST_CASE(test_mcu_get_reset_reason_not_init)
{
    Mcu_ResetType reset_reason;
    
    /* Setup */
    g_mcu_state = MCU_STATE_UNINIT;
    
    /* Execute */
    reset_reason = Mcu_GetResetReason();
    
    /* Verify */
    ASSERT_EQ(MCU_RESET_UNDEFINED, reset_reason);
    TEST_PASS();
}

/* Test: Mcu_GetResetRawValue when not initialized */
TEST_CASE(test_mcu_get_reset_raw_not_init)
{
    Mcu_RawResetType raw_value;
    
    /* Setup */
    g_mcu_state = MCU_STATE_UNINIT;
    
    /* Execute */
    raw_value = Mcu_GetResetRawValue();
    
    /* Verify */
    ASSERT_EQ(0U, raw_value);
    TEST_PASS();
}

/* Test: Mcu_PerformReset when not initialized */
TEST_CASE(test_mcu_perform_reset_not_init)
{
    /* Setup */
    g_mcu_state = MCU_STATE_UNINIT;
    
    /* Execute - should not actually reset in test */
    Mcu_PerformReset();
    
    /* Verify */
    TEST_PASS();
}

/* Test: Mcu_SetMode with invalid mode */
TEST_CASE(test_mcu_set_mode_invalid)
{
    /* Setup */
    Mcu_Init(&g_test_config);
    
    /* Execute */
    Mcu_SetMode(255); /* Invalid mode */
    
    /* Verify */
    TEST_PASS();
}

/* Test: Mcu_GetClockFrequency with invalid clock */
TEST_CASE(test_mcu_get_clock_freq_invalid)
{
    Mcu_ClockType clock;
    uint32 freq;
    
    /* Setup */
    Mcu_Init(&g_test_config);
    clock = 255; /* Invalid clock */
    
    /* Execute */
    freq = Mcu_GetClockFrequency(clock);
    
    /* Verify */
    ASSERT_EQ(0U, freq);
    TEST_PASS();
}

/* Main test runner */
TEST_MAIN_BEGIN()
{
    printf("\n--- Mcu Module Tests ---\n");
    
    RUN_TEST(test_mcu_init_valid_config);
    RUN_TEST(test_mcu_init_null_config);
    RUN_TEST(test_mcu_deinit);
    RUN_TEST(test_mcu_get_version_info);
    RUN_TEST(test_mcu_get_version_info_null);
    RUN_TEST(test_mcu_distribute_pll_not_init);
    RUN_TEST(test_mcu_get_pll_status_not_init);
    RUN_TEST(test_mcu_get_reset_reason_not_init);
    RUN_TEST(test_mcu_get_reset_raw_not_init);
    RUN_TEST(test_mcu_perform_reset_not_init);
    RUN_TEST(test_mcu_set_mode_invalid);
    RUN_TEST(test_mcu_get_clock_freq_invalid);
}
TEST_MAIN_END()
