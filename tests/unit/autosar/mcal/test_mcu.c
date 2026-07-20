/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : MCU Driver Unit Tests
*
* SW Version           : 1.0.0
* Build Date           : 2026-05-15
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
*
* @file test_mcu.c
* @brief MCU Driver unit tests for MCAL layer
* @details This test suite covers:
*          - Initialization (Mcu_Init)
*          - Clock configuration (Mcu_InitClock, Mcu_DistributePllClock)
*          - PLL status (Mcu_GetPllStatus)
*          - Mode switching (Mcu_SetMode)
*          - Reset management (Mcu_GetResetReason, Mcu_GetResetRawValue, Mcu_PerformReset)
*          - Version info (Mcu_GetVersionInfo)
*          - RAM management (Mcu_InitRamSection, Mcu_GetRamState)
*
* Target Coverage: 80%+
* Test Cases: 20+
==================================================================================================*/

#include "test_framework.h"
#include "Mcu.h"
#include "Mcu_Cfg.h"
#include "mock_mcal.h"
#include "mock_det.h"
#include "mock_registers.h"

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static Mcu_ConfigType g_test_config;
static Mcu_RamSectionConfigType g_test_ram_sections[MCU_NUM_RAM_SECTIONS];
static Mcu_ClockConfigType g_test_clock_configs[MCU_NUM_CLOCK_CONFIGS];
static Mcu_PllConfigType g_test_pll_configs[MCU_NUM_CLOCK_CONFIGS];
static Mcu_ModeConfigType g_test_mode_configs[MCU_NUM_MODES];

/*==================================================================================================
*                                      HELPER FUNCTIONS
==================================================================================================*/

/**
 * @brief Setup test configuration structures
 */
static void setup_test_config(void)
{
    /* Setup RAM section configuration */
    g_test_ram_sections[0].RamBaseAddr = 0x20000000U;
    g_test_ram_sections[0].RamSize = 0x10000U;  /* 64KB */
    g_test_ram_sections[0].RamDefaultValue = 0x00U;

    /* Setup PLL configuration */
    g_test_pll_configs[0].PllBaseAddr = 0x30360000U;
    g_test_pll_configs[0].Prediv = MCU_PLL_PREDIV;
    g_test_pll_configs[0].Multiplier = MCU_PLL_MULTIPLIER;
    g_test_pll_configs[0].Postdiv1 = MCU_PLL_POSTDIV;
    g_test_pll_configs[0].Postdiv2 = MCU_PLL_POSTDIV;

    /* Setup clock configuration */
    g_test_clock_configs[0].ClockSource = MCU_CLOCK_SOURCE_PLL;
    g_test_clock_configs[0].ArmDiv = 1U;
    g_test_clock_configs[0].AxiDiv = 2U;
    g_test_clock_configs[0].AhbDiv = 4U;
    g_test_clock_configs[0].PllConfigs = g_test_pll_configs;
    g_test_clock_configs[0].NumPllConfigs = 1U;

    /* Setup mode configuration */
    g_test_mode_configs[0].Mode = MCU_MODE_RUN;
    g_test_mode_configs[1].Mode = MCU_MODE_SLEEP;
    g_test_mode_configs[2].Mode = MCU_MODE_DEEP_SLEEP;
    g_test_mode_configs[3].Mode = MCU_MODE_NORMAL;

    /* Setup main configuration */
    g_test_config.RamSections = g_test_ram_sections;
    g_test_config.NumRamSections = MCU_NUM_RAM_SECTIONS;
    g_test_config.ClockConfigs = g_test_clock_configs;
    g_test_config.NumClockConfigs = MCU_NUM_CLOCK_CONFIGS;
    g_test_config.ModeConfigs = g_test_mode_configs;
    g_test_config.NumModes = MCU_NUM_MODES;
}

/**
 * @brief Reset MCU driver state for testing
 */
static void reset_mcu_driver_state(void)
{
    /* Access internal state through test hook or reinitialize */
    Mcu_Mock_Reset();
    Det_Mock_Reset();
    MockRegisters_Reset();
}

/*==================================================================================================
*                                      POSITIVE TESTS
*                                      正向测试
==================================================================================================*/

/**
 * @test Mcu_Init with valid configuration
 * @req MCU_INIT_001
 * @desc Verify successful initialization with valid config pointer
 * @coverage Mcu_Init
 */
TEST_CASE(mcu_init_valid_config)
{
    setup_test_config();
    reset_mcu_driver_state();

    Mcu_Init(&g_test_config);

    /* Verify driver is initialized */
    ASSERT_TRUE(MockRegisters_Read32(MCU_SRC_SCR) != 0xFFFFFFFFU);
    TEST_PASS();
}

/**
 * @test Mcu_InitClock with valid clock setting
 * @req MCU_CLOCK_001
 * @desc Verify successful clock initialization
 * @coverage Mcu_InitClock, Mcu_ConfigureClock
 */
TEST_CASE(mcu_init_clock_valid)
{
    Std_ReturnType result;

    setup_test_config();
    reset_mcu_driver_state();
    Mcu_Init(&g_test_config);

    /* Mock PLL lock status */
    MockRegisters_Write32(MCU_CCM_CSR, 0x01U);  /* PLL locked */

    result = Mcu_InitClock(0);

    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/**
 * @test Mcu_DistributePllClock after successful clock init
 * @req MCU_PLL_001
 * @desc Verify PLL clock distribution works correctly
 * @coverage Mcu_DistributePllClock
 */
TEST_CASE(mcu_distribute_pll_clock_valid)
{
    setup_test_config();
    reset_mcu_driver_state();
    Mcu_Init(&g_test_config);
    MockRegisters_Write32(MCU_CCM_CSR, 0x01U);  /* PLL locked */
    Mcu_InitClock(0);

    Mcu_DistributePllClock();

    /* Verify clock distribution enabled */
    ASSERT_TRUE((MockRegisters_Read32(MCU_CCM_CCR) & 0x01U) != 0U);
    TEST_PASS();
}

/**
 * @test Mcu_GetPllStatus returns correct status
 * @req MCU_PLL_002
 * @desc Verify PLL status reading
 * @coverage Mcu_GetPllStatus
 */
TEST_CASE(mcu_get_pll_status_locked)
{
    Mcu_PllStatusType status;

    setup_test_config();
    reset_mcu_driver_state();
    Mcu_Init(&g_test_config);
    MockRegisters_Write32(MCU_CCM_CSR, 0x01U);  /* PLL locked */
    Mcu_InitClock(0);

    status = Mcu_GetPllStatus();

    ASSERT_EQ(MCU_PLL_STATUS_LOCKED, status);
    TEST_PASS();
}

/**
 * @test Mcu_GetPllStatus when unlocked
 * @req MCU_PLL_003
 * @desc Verify PLL unlocked status detection
 * @coverage Mcu_GetPllStatus
 */
TEST_CASE(mcu_get_pll_status_unlocked)
{
    Mcu_PllStatusType status;

    setup_test_config();
    reset_mcu_driver_state();
    Mcu_Init(&g_test_config);
    MockRegisters_Write32(MCU_CCM_CSR, 0x00U);  /* PLL unlocked */

    status = Mcu_GetPllStatus();

    ASSERT_EQ(MCU_PLL_STATUS_UNLOCKED, status);
    TEST_PASS();
}

/**
 * @test Mcu_SetMode to RUN mode
 * @req MCU_MODE_001
 * @desc Verify mode switching to RUN
 * @coverage Mcu_SetMode
 */
TEST_CASE(mcu_set_mode_run)
{
    setup_test_config();
    reset_mcu_driver_state();
    Mcu_Init(&g_test_config);

    Mcu_SetMode(MCU_MODE_RUN);

    ASSERT_EQ(0x01U, MockRegisters_Read32(MCU_GPC_PGC_CPU_MAPPING));
    TEST_PASS();
}

/**
 * @test Mcu_SetMode to SLEEP mode
 * @req MCU_MODE_002
 * @desc Verify mode switching to SLEEP
 * @coverage Mcu_SetMode
 */
TEST_CASE(mcu_set_mode_sleep)
{
    setup_test_config();
    reset_mcu_driver_state();
    Mcu_Init(&g_test_config);

    Mcu_SetMode(MCU_MODE_SLEEP);

    ASSERT_EQ(0x02U, MockRegisters_Read32(MCU_GPC_PGC_CPU_MAPPING));
    TEST_PASS();
}

/**
 * @test Mcu_SetMode to DEEP_SLEEP mode
 * @req MCU_MODE_003
 * @desc Verify mode switching to DEEP_SLEEP
 * @coverage Mcu_SetMode
 */
TEST_CASE(mcu_set_mode_deep_sleep)
{
    setup_test_config();
    reset_mcu_driver_state();
    Mcu_Init(&g_test_config);

    Mcu_SetMode(MCU_MODE_DEEP_SLEEP);

    ASSERT_EQ(0x04U, MockRegisters_Read32(MCU_GPC_PGC_CPU_MAPPING));
    TEST_PASS();
}

/**
 * @test Mcu_GetResetReason for power-on reset
 * @req MCU_RESET_001
 * @desc Verify power-on reset detection
 * @coverage Mcu_GetResetReason, Mcu_GetResetReasonFromRegister
 */
TEST_CASE(mcu_get_reset_reason_power_on)
{
    Mcu_ResetType reason;

    setup_test_config();
    reset_mcu_driver_state();
    Mcu_Init(&g_test_config);
    MockRegisters_Write32(MCU_SRC_SRSR, 0x01U);  /* Power-on reset */

    reason = Mcu_GetResetReason();

    ASSERT_EQ(MCU_RESET_POWER_ON_RESET, reason);
    TEST_PASS();
}

/**
 * @test Mcu_GetResetReason for watchdog reset
 * @req MCU_RESET_002
 * @desc Verify watchdog reset detection
 * @coverage Mcu_GetResetReason
 */
TEST_CASE(mcu_get_reset_reason_watchdog)
{
    Mcu_ResetType reason;

    setup_test_config();
    reset_mcu_driver_state();
    Mcu_Init(&g_test_config);
    MockRegisters_Write32(MCU_SRC_SRSR, 0x02U);  /* Watchdog reset */

    reason = Mcu_GetResetReason();

    ASSERT_EQ(MCU_RESET_WATCHDOG_RESET, reason);
    TEST_PASS();
}

/**
 * @test Mcu_GetResetReason for software reset
 * @req MCU_RESET_003
 * @desc Verify software reset detection
 * @coverage Mcu_GetResetReason
 */
TEST_CASE(mcu_get_reset_reason_software)
{
    Mcu_ResetType reason;

    setup_test_config();
    reset_mcu_driver_state();
    Mcu_Init(&g_test_config);
    MockRegisters_Write32(MCU_SRC_SRSR, 0x04U);  /* Software reset */

    reason = Mcu_GetResetReason();

    ASSERT_EQ(MCU_RESET_SW_RESET, reason);
    TEST_PASS();
}

/**
 * @test Mcu_GetResetReason for external reset
 * @req MCU_RESET_004
 * @desc Verify external reset detection
 * @coverage Mcu_GetResetReason
 */
TEST_CASE(mcu_get_reset_reason_external)
{
    Mcu_ResetType reason;

    setup_test_config();
    reset_mcu_driver_state();
    Mcu_Init(&g_test_config);
    MockRegisters_Write32(MCU_SRC_SRSR, 0x08U);  /* External reset */

    reason = Mcu_GetResetReason();

    ASSERT_EQ(MCU_RESET_EXTERNAL_RESET, reason);
    TEST_PASS();
}

/**
 * @test Mcu_GetResetRawValue returns raw register value
 * @req MCU_RESET_005
 * @desc Verify raw reset value reading
 * @coverage Mcu_GetResetRawValue
 */
TEST_CASE(mcu_get_reset_raw_value)
{
    Mcu_RawResetType raw;

    setup_test_config();
    reset_mcu_driver_state();
    Mcu_Init(&g_test_config);
    MockRegisters_Write32(MCU_SRC_SRSR, 0x0FU);  /* Multiple reset reasons */

    raw = Mcu_GetResetRawValue();

    ASSERT_EQ(0x0FU, raw);
    TEST_PASS();
}

/**
 * @test Mcu_GetVersionInfo returns correct version
 * @req MCU_VERSION_001
 * @desc Verify version information retrieval
 * @coverage Mcu_GetVersionInfo
 */
TEST_CASE(mcu_get_version_info_valid)
{
    Std_VersionInfoType version_info;

    Mcu_GetVersionInfo(&version_info);

    ASSERT_EQ(MCU_VENDOR_ID, version_info.vendorID);
    ASSERT_EQ(MCU_MODULE_ID, version_info.moduleID);
    ASSERT_EQ(MCU_SW_MAJOR_VERSION, version_info.sw_major_version);
    ASSERT_EQ(MCU_SW_MINOR_VERSION, version_info.sw_minor_version);
    ASSERT_EQ(MCU_SW_PATCH_VERSION, version_info.sw_patch_version);
    TEST_PASS();
}

/**
 * @test Mcu_InitRamSection with valid section
 * @req MCU_RAM_001
 * @desc Verify RAM section initialization
 * @coverage Mcu_InitRamSection
 */
TEST_CASE(mcu_init_ram_section_valid)
{
    Std_ReturnType result;

    setup_test_config();
    reset_mcu_driver_state();
    Mcu_Init(&g_test_config);

    result = Mcu_InitRamSection(0);

    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/**
 * @test Mcu_GetRamState returns valid state
 * @req MCU_RAM_002
 * @desc Verify RAM state reading
 * @coverage Mcu_GetRamState
 */
TEST_CASE(mcu_get_ram_state_valid)
{
    Mcu_RamStateType state;

    setup_test_config();
    reset_mcu_driver_state();
    Mcu_Init(&g_test_config);

    state = Mcu_GetRamState();

    ASSERT_EQ(MCU_RAMSTATE_VALID, state);
    TEST_PASS();
}

/*==================================================================================================
*                                      NEGATIVE TESTS
*                                      负向测试
==================================================================================================*/

/**
 * @test Mcu_Init with NULL config pointer
 * @req MCU_INIT_002
 * @desc Verify error reporting for NULL config
 * @coverage Mcu_Init, Det_ReportError
 */
TEST_CASE(mcu_init_null_config)
{
    setup_test_config();
    reset_mcu_driver_state();

    Mcu_Init(NULL);

    ASSERT_EQ(1U, Det_MockData.CallCount);
    ASSERT_EQ(MCU_MODULE_ID, Det_MockData.ModuleId);
    ASSERT_EQ(MCU_API_ID_INIT, Det_MockData.ApiId);
    ASSERT_EQ(MCU_E_PARAM_CONFIG, Det_MockData.ErrorId);
    TEST_PASS();
}

/**
 * @test Mcu_Init when already initialized
 * @req MCU_INIT_003
 * @desc Verify error reporting for double initialization
 * @coverage Mcu_Init, Det_ReportError
 */
TEST_CASE(mcu_init_already_initialized)
{
    setup_test_config();
    reset_mcu_driver_state();
    Mcu_Init(&g_test_config);

    /* Try to initialize again */
    Mcu_Init(&g_test_config);

    ASSERT_EQ(1U, Det_MockData.CallCount);
    ASSERT_EQ(MCU_E_ALREADY_INITIALIZED, Det_MockData.ErrorId);
    TEST_PASS();
}

/**
 * @test Mcu_InitClock when not initialized
 * @req MCU_CLOCK_002
 * @desc Verify error reporting when calling InitClock before Init
 * @coverage Mcu_InitClock, Det_ReportError
 */
TEST_CASE(mcu_init_clock_not_initialized)
{
    Std_ReturnType result;

    reset_mcu_driver_state();

    result = Mcu_InitClock(0);

    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(1U, Det_MockData.CallCount);
    ASSERT_EQ(MCU_E_UNINIT, Det_MockData.ErrorId);
    TEST_PASS();
}

/**
 * @test Mcu_InitClock with invalid clock setting
 * @req MCU_CLOCK_003
 * @desc Verify error reporting for invalid clock setting
 * @coverage Mcu_InitClock, Det_ReportError
 */
TEST_CASE(mcu_init_clock_invalid_setting)
{
    Std_ReturnType result;

    setup_test_config();
    reset_mcu_driver_state();
    Mcu_Init(&g_test_config);

    result = Mcu_InitClock(99);  /* Invalid clock setting */

    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(1U, Det_MockData.CallCount);
    ASSERT_EQ(MCU_E_PARAM_CLOCK, Det_MockData.ErrorId);
    TEST_PASS();
}

/**
 * @test Mcu_DistributePllClock when not initialized
 * @req MCU_PLL_004
 * @desc Verify error reporting for uninitialized driver
 * @coverage Mcu_DistributePllClock, Det_ReportError
 */
TEST_CASE(mcu_distribute_pll_not_initialized)
{
    reset_mcu_driver_state();

    Mcu_DistributePllClock();

    ASSERT_EQ(1U, Det_MockData.CallCount);
    ASSERT_EQ(MCU_E_UNINIT, Det_MockData.ErrorId);
    TEST_PASS();
}

/**
 * @test Mcu_DistributePllClock when PLL not locked
 * @req MCU_PLL_005
 * @desc Verify error reporting when PLL not locked
 * @coverage Mcu_DistributePllClock, Det_ReportError
 */
TEST_CASE(mcu_distribute_pll_not_locked)
{
    setup_test_config();
    reset_mcu_driver_state();
    Mcu_Init(&g_test_config);
    /* Don't call Mcu_InitClock - currentClock stays 0 */

    Mcu_DistributePllClock();

    ASSERT_EQ(1U, Det_MockData.CallCount);
    ASSERT_EQ(MCU_E_PLL_NOT_LOCKED, Det_MockData.ErrorId);
    TEST_PASS();
}

/**
 * @test Mcu_GetPllStatus when not initialized
 * @req MCU_PLL_006
 * @desc Verify error reporting for uninitialized driver
 * @coverage Mcu_GetPllStatus, Det_ReportError
 */
TEST_CASE(mcu_get_pll_status_not_initialized)
{
    Mcu_PllStatusType status;

    reset_mcu_driver_state();

    status = Mcu_GetPllStatus();

    ASSERT_EQ(MCU_PLL_STATUS_UNDEFINED, status);
    ASSERT_EQ(1U, Det_MockData.CallCount);
    ASSERT_EQ(MCU_E_UNINIT, Det_MockData.ErrorId);
    TEST_PASS();
}

/**
 * @test Mcu_SetMode when not initialized
 * @req MCU_MODE_004
 * @desc Verify error reporting for uninitialized driver
 * @coverage Mcu_SetMode, Det_ReportError
 */
TEST_CASE(mcu_set_mode_not_initialized)
{
    reset_mcu_driver_state();

    Mcu_SetMode(MCU_MODE_RUN);

    ASSERT_EQ(1U, Det_MockData.CallCount);
    ASSERT_EQ(MCU_E_UNINIT, Det_MockData.ErrorId);
    TEST_PASS();
}

/**
 * @test Mcu_SetMode with invalid mode
 * @req MCU_MODE_005
 * @desc Verify error reporting for invalid mode
 * @coverage Mcu_SetMode, Det_ReportError
 */
TEST_CASE(mcu_set_mode_invalid)
{
    setup_test_config();
    reset_mcu_driver_state();
    Mcu_Init(&g_test_config);

    Mcu_SetMode(99);  /* Invalid mode */

    ASSERT_EQ(1U, Det_MockData.CallCount);
    ASSERT_EQ(MCU_E_PARAM_MODE, Det_MockData.ErrorId);
    TEST_PASS();
}

/**
 * @test Mcu_GetResetReason when not initialized
 * @req MCU_RESET_006
 * @desc Verify error reporting for uninitialized driver
 * @coverage Mcu_GetResetReason, Det_ReportError
 */
TEST_CASE(mcu_get_reset_reason_not_initialized)
{
    Mcu_ResetType reason;

    reset_mcu_driver_state();

    reason = Mcu_GetResetReason();

    ASSERT_EQ(MCU_RESET_UNDEFINED, reason);
    ASSERT_EQ(1U, Det_MockData.CallCount);
    ASSERT_EQ(MCU_E_UNINIT, Det_MockData.ErrorId);
    TEST_PASS();
}

/**
 * @test Mcu_GetResetRawValue when not initialized
 * @req MCU_RESET_007
 * @desc Verify error reporting for uninitialized driver
 * @coverage Mcu_GetResetRawValue, Det_ReportError
 */
TEST_CASE(mcu_get_reset_raw_value_not_initialized)
{
    Mcu_RawResetType raw;

    reset_mcu_driver_state();

    raw = Mcu_GetResetRawValue();

    ASSERT_EQ(0U, raw);
    ASSERT_EQ(1U, Det_MockData.CallCount);
    ASSERT_EQ(MCU_E_UNINIT, Det_MockData.ErrorId);
    TEST_PASS();
}

/**
 * @test Mcu_PerformReset when not initialized
 * @req MCU_RESET_008
 * @desc Verify error reporting for uninitialized driver
 * @coverage Mcu_PerformReset, Det_ReportError
 */
TEST_CASE(mcu_perform_reset_not_initialized)
{
    reset_mcu_driver_state();

    Mcu_PerformReset();

    ASSERT_EQ(1U, Det_MockData.CallCount);
    ASSERT_EQ(MCU_E_UNINIT, Det_MockData.ErrorId);
    TEST_PASS();
}

/**
 * @test Mcu_GetVersionInfo with NULL pointer
 * @req MCU_VERSION_002
 * @desc Verify error reporting for NULL version info pointer
 * @coverage Mcu_GetVersionInfo, Det_ReportError
 */
TEST_CASE(mcu_get_version_info_null)
{
    Mcu_GetVersionInfo(NULL);

    ASSERT_EQ(1U, Det_MockData.CallCount);
    ASSERT_EQ(MCU_E_PARAM_POINTER, Det_MockData.ErrorId);
    TEST_PASS();
}

/**
 * @test Mcu_InitRamSection when not initialized
 * @req MCU_RAM_003
 * @desc Verify error reporting for uninitialized driver
 * @coverage Mcu_InitRamSection, Det_ReportError
 */
TEST_CASE(mcu_init_ram_section_not_initialized)
{
    Std_ReturnType result;

    reset_mcu_driver_state();

    result = Mcu_InitRamSection(0);

    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(1U, Det_MockData.CallCount);
    ASSERT_EQ(MCU_E_UNINIT, Det_MockData.ErrorId);
    TEST_PASS();
}

/**
 * @test Mcu_InitRamSection with invalid section
 * @req MCU_RAM_004
 * @desc Verify error reporting for invalid RAM section
 * @coverage Mcu_InitRamSection, Det_ReportError
 */
TEST_CASE(mcu_init_ram_section_invalid)
{
    Std_ReturnType result;

    setup_test_config();
    reset_mcu_driver_state();
    Mcu_Init(&g_test_config);

    result = Mcu_InitRamSection(99);  /* Invalid section */

    ASSERT_EQ(E_NOT_OK, result);
    ASSERT_EQ(1U, Det_MockData.CallCount);
    ASSERT_EQ(MCU_E_PARAM_RAMSECTION, Det_MockData.ErrorId);
    TEST_PASS();
}

/**
 * @test Mcu_GetRamState when not initialized
 * @req MCU_RAM_005
 * @desc Verify error reporting for uninitialized driver
 * @coverage Mcu_GetRamState, Det_ReportError
 */
TEST_CASE(mcu_get_ram_state_not_initialized)
{
    Mcu_RamStateType state;

    reset_mcu_driver_state();

    state = Mcu_GetRamState();

    ASSERT_EQ(MCU_RAMSTATE_INVALID, state);
    ASSERT_EQ(1U, Det_MockData.CallCount);
    ASSERT_EQ(MCU_E_UNINIT, Det_MockData.ErrorId);
    TEST_PASS();
}

/*==================================================================================================
*                                      BOUNDARY TESTS
*                                      边界测试
==================================================================================================*/

/**
 * @test Mcu_InitClock with boundary clock setting (0)
 * @req MCU_CLOCK_004
 * @desc Verify clock initialization with minimum valid setting
 * @coverage Mcu_InitClock
 */
TEST_CASE(mcu_init_clock_boundary_min)
{
    Std_ReturnType result;

    setup_test_config();
    reset_mcu_driver_state();
    Mcu_Init(&g_test_config);
    MockRegisters_Write32(MCU_CCM_CSR, 0x01U);

    result = Mcu_InitClock(0);  /* Minimum valid setting */

    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/**
 * @test Mcu_InitRamSection with boundary section (0)
 * @req MCU_RAM_006
 * @desc Verify RAM initialization with minimum valid section
 * @coverage Mcu_InitRamSection
 */
TEST_CASE(mcu_init_ram_section_boundary_min)
{
    Std_ReturnType result;

    setup_test_config();
    reset_mcu_driver_state();
    Mcu_Init(&g_test_config);

    result = Mcu_InitRamSection(0);  /* Minimum valid section */

    ASSERT_EQ(E_OK, result);
    TEST_PASS();
}

/**
 * @test Mcu_SetMode with boundary mode (0)
 * @req MCU_MODE_006
 * @desc Verify mode switching with minimum valid mode
 * @coverage Mcu_SetMode
 */
TEST_CASE(mcu_set_mode_boundary_min)
{
    setup_test_config();
    reset_mcu_driver_state();
    Mcu_Init(&g_test_config);

    Mcu_SetMode(0);  /* Minimum valid mode */

    /* Verify mode was set */
    ASSERT_TRUE(MockRegisters_Read32(MCU_GPC_PGC_CPU_MAPPING) != 0xFFFFFFFFU);
    TEST_PASS();
}

/*==================================================================================================
*                                      STATE TESTS
*                                      状态测试
==================================================================================================*/

/**
 * @test MCU state transition from UNINIT to INIT
 * @req MCU_STATE_001
 * @desc Verify state transition during initialization
 * @coverage Mcu_Init
 */
TEST_CASE(mcu_state_transition_init)
{
    setup_test_config();
    reset_mcu_driver_state();

    /* State should transition from UNINIT to INIT */
    Mcu_Init(&g_test_config);

    /* After Init, subsequent calls should report already initialized */
    Det_Mock_Reset();
    Mcu_Init(&g_test_config);
    ASSERT_EQ(MCU_E_ALREADY_INITIALIZED, Det_MockData.ErrorId);
    TEST_PASS();
}

/**
 * @test Complete MCU initialization sequence
 * @req MCU_SEQ_001
 * @desc Verify complete initialization sequence
 * @coverage Mcu_Init, Mcu_InitClock, Mcu_DistributePllClock
 */
TEST_CASE(mcu_init_sequence_complete)
{
    Std_ReturnType result;

    setup_test_config();
    reset_mcu_driver_state();

    /* Step 1: Initialize MCU driver */
    Mcu_Init(&g_test_config);

    /* Step 2: Mock PLL locked */
    MockRegisters_Write32(MCU_CCM_CSR, 0x01U);

    /* Step 3: Initialize clock */
    result = Mcu_InitClock(0);
    ASSERT_EQ(E_OK, result);

    /* Step 4: Distribute PLL clock */
    Mcu_DistributePllClock();

    /* Verify clock distribution enabled */
    ASSERT_TRUE((MockRegisters_Read32(MCU_CCM_CCR) & 0x01U) != 0U);
    TEST_PASS();
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(mcu)
{
    reset_mcu_driver_state();
    setup_test_config();
}

TEST_SUITE_TEARDOWN(mcu)
{
    /* Cleanup after all tests */
    reset_mcu_driver_state();
}

TEST_SUITE(mcu)
{
    /* Positive tests */
    RUN_TEST(mcu_init_valid_config);
    RUN_TEST(mcu_init_clock_valid);
    RUN_TEST(mcu_distribute_pll_clock_valid);
    RUN_TEST(mcu_get_pll_status_locked);
    RUN_TEST(mcu_get_pll_status_unlocked);
    RUN_TEST(mcu_set_mode_run);
    RUN_TEST(mcu_set_mode_sleep);
    RUN_TEST(mcu_set_mode_deep_sleep);
    RUN_TEST(mcu_get_reset_reason_power_on);
    RUN_TEST(mcu_get_reset_reason_watchdog);
    RUN_TEST(mcu_get_reset_reason_software);
    RUN_TEST(mcu_get_reset_reason_external);
    RUN_TEST(mcu_get_reset_raw_value);
    RUN_TEST(mcu_get_version_info_valid);
    RUN_TEST(mcu_init_ram_section_valid);
    RUN_TEST(mcu_get_ram_state_valid);

    /* Negative tests */
    RUN_TEST(mcu_init_null_config);
    RUN_TEST(mcu_init_already_initialized);
    RUN_TEST(mcu_init_clock_not_initialized);
    RUN_TEST(mcu_init_clock_invalid_setting);
    RUN_TEST(mcu_distribute_pll_not_initialized);
    RUN_TEST(mcu_distribute_pll_not_locked);
    RUN_TEST(mcu_get_pll_status_not_initialized);
    RUN_TEST(mcu_set_mode_not_initialized);
    RUN_TEST(mcu_set_mode_invalid);
    RUN_TEST(mcu_get_reset_reason_not_initialized);
    RUN_TEST(mcu_get_reset_raw_value_not_initialized);
    RUN_TEST(mcu_perform_reset_not_initialized);
    RUN_TEST(mcu_get_version_info_null);
    RUN_TEST(mcu_init_ram_section_not_initialized);
    RUN_TEST(mcu_init_ram_section_invalid);
    RUN_TEST(mcu_get_ram_state_not_initialized);

    /* Boundary tests */
    RUN_TEST(mcu_init_clock_boundary_min);
    RUN_TEST(mcu_init_ram_section_boundary_min);
    RUN_TEST(mcu_set_mode_boundary_min);

    /* State tests */
    RUN_TEST(mcu_state_transition_init);
    RUN_TEST(mcu_init_sequence_complete);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
{
    printf("\n" TEST_COLOR_BLUE "========================================" TEST_COLOR_RESET "\n");
    printf("" TEST_COLOR_BLUE "      MCU Driver Unit Tests             " TEST_COLOR_RESET "\n");
    printf("" TEST_COLOR_BLUE "========================================" TEST_COLOR_RESET "\n");
    printf("" TEST_COLOR_CYAN "  Target Coverage: 80%+                  " TEST_COLOR_RESET "\n");
    printf("" TEST_COLOR_CYAN "  Test Categories:                       " TEST_COLOR_RESET "\n");
    printf("" TEST_COLOR_CYAN "    - Positive Tests (15)                " TEST_COLOR_RESET "\n");
    printf("" TEST_COLOR_CYAN "    - Negative Tests (16)                " TEST_COLOR_RESET "\n");
    printf("" TEST_COLOR_CYAN "    - Boundary Tests (3)                 " TEST_COLOR_RESET "\n");
    printf("" TEST_COLOR_CYAN "    - State Tests (2)                    " TEST_COLOR_RESET "\n");
    printf("" TEST_COLOR_BLUE "========================================" TEST_COLOR_RESET "\n");

    RUN_TEST_SUITE(mcu);
}
TEST_MAIN_END()
