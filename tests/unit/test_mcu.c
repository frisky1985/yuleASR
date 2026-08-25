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

// @tests src/bsw/mcal/mcu/src/Mcu.c  @tests src/bsw/mcal/mcu/include/Mcu.h

#include "test_framework.h"
#include "Mcu.h"

/* Mock variables */
static Mcu_ConfigType g_test_config;
static uint8 g_mcu_state = MCU_STATE_UNINIT;

/* Test: Mcu_Init with valid config */
void test_mcu_init_valid_config(void)
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
}

/* Test: Mcu_Init with NULL config */
void test_mcu_init_null_config(void)
{
    /* Execute */
    Mcu_Init(NULL);
    
    /* Verify - should report error but not crash */
    ASSERT_EQ(MCU_STATE_UNINIT, g_mcu_state);
}

/* Test: Mcu_DeInit */
void test_mcu_deinit(void)
{
    /* Setup */
    Mcu_Init(&g_test_config);
    
    /* Execute */
    Mcu_DeInit();
    
    /* Verify */
    ASSERT_EQ(MCU_STATE_UNINIT, g_mcu_state);
}

/* Test: Mcu_GetVersionInfo */
void test_mcu_get_version_info(void)
{
    Std_VersionInfoType version_info;
    
    /* Execute */
    Mcu_GetVersionInfo(&version_info);
    
    /* Verify */
    ASSERT_EQ(MCU_VENDOR_ID, version_info.vendorID);
    ASSERT_EQ(MCU_SW_MAJOR_VERSION, version_info.sw_major_version);
    ASSERT_EQ(MCU_SW_MINOR_VERSION, version_info.sw_minor_version);
}

/* Test: Mcu_GetVersionInfo with NULL pointer */
void test_mcu_get_version_info_null(void)
{
    /* Execute */
    Mcu_GetVersionInfo(NULL);
    
    /* Verify - should report error but not crash */
    ASSERT_EQ(MCU_STATE_UNINIT, g_mcu_state);
}

/* Test: Mcu_DistributePllClock when not initialized */
void test_mcu_distribute_pll_not_init(void)
{
    /* Setup */
    g_mcu_state = MCU_STATE_UNINIT;
    
    /* Execute */
    Mcu_DistributePllClock();
    
    /* Verify */
    ASSERT_TRUE(g_mcu_pll_state == PLL_STATE_LOCKED || g_mcu_pll_state == PLL_STATE_BYPASS);
}

/* Test: Mcu_GetPllStatus when not initialized */
void test_mcu_get_pll_status_not_init(void)
{
    Mcu_PllStatusType status;
    
    /* Setup */
    g_mcu_state = MCU_STATE_UNINIT;
    
    /* Execute */
    status = Mcu_GetPllStatus();
    
    /* Verify */
    ASSERT_EQ(MCU_PLL_STATUS_UNDEFINED, status);
}

/* Test: Mcu_GetResetReason when not initialized */
void test_mcu_get_reset_reason_not_init(void)
{
    Mcu_ResetType reset_reason;
    
    /* Setup */
    g_mcu_state = MCU_STATE_UNINIT;
    
    /* Execute */
    reset_reason = Mcu_GetResetReason();
    
    /* Verify */
    ASSERT_EQ(MCU_RESET_UNDEFINED, reset_reason);
}

/* Test: Mcu_GetResetRawValue when not initialized */
void test_mcu_get_reset_raw_not_init(void)
{
    Mcu_RawResetType raw_value;
    
    /* Setup */
    g_mcu_state = MCU_STATE_UNINIT;
    
    /* Execute */
    raw_value = Mcu_GetResetRawValue();
    
    /* Verify */
    ASSERT_EQ(0U, raw_value);
}

/* Test: Mcu_PerformReset when not initialized */
void test_mcu_perform_reset_not_init(void)
{
    /* Setup */
    g_mcu_state = MCU_STATE_UNINIT;
    
    /* Execute - should not actually reset in test */
    Mcu_PerformReset();
    
    /* Verify */
    ASSERT_TRUE(g_mcu_reset_performed);
}

/* Test: Mcu_SetMode with invalid mode */
void test_mcu_set_mode_invalid(void)
{
    /* Setup */
    Mcu_Init(&g_test_config);
    
    /* Execute */
    Mcu_SetMode(255); /* Invalid mode */
    
    /* Verify */
    ASSERT_EQ(1, g_det_error_count);  /* Det should report invalid mode */
}

/* Test: Mcu_GetClockFrequency with invalid clock */
void test_mcu_get_clock_freq_invalid(void)
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
