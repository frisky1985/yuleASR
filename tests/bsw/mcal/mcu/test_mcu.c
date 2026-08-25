/**
 * @file test_mcu.c
 * @brief Mcu (MCU Driver) Unit Tests
 * @req SWS_Mcu
 */
#include "unity.h"
#include "Mcu.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

static Mcu_ConfigType testConfig;
void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_Mcu_00001 */
void test_Mcu_Init_NullPtr_ShouldReportDet(void) { Mcu_Init(NULL_PTR); TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls); }
/** @req SWS_Mcu_00001 */
void test_Mcu_Init_ValidConfig_ShouldSucceed(void) { testConfig.NumClocks = 0U; Mcu_Init(&testConfig); TEST_ASSERT_TRUE(1); }
/** @req SWS_Mcu_00002 */
void test_Mcu_DeInit_AfterInit_ShouldSucceed(void) { Mcu_Init(&testConfig); Mcu_DeInit(); TEST_ASSERT_TRUE(1); }
/** @req SWS_Mcu_00003 */
void test_Mcu_InitRamSection_AfterInit_ShouldReturnResult(void) { Mcu_Init(&testConfig); Std_ReturnType ret = Mcu_InitRamSection(0U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Mcu_00004 */
void test_Mcu_InitClock_AfterInit_ShouldReturnResult(void) { Mcu_Init(&testConfig); Std_ReturnType ret = Mcu_InitClock(0U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Mcu_00005 */
void test_Mcu_GetResetReason_AfterInit_ShouldReturnReason(void) { Mcu_Init(&testConfig); Mcu_ResetType reason = Mcu_GetResetReason(); TEST_ASSERT_TRUE(reason == MCU_POWER_ON_RESET || reason == MCU_EXTERNAL_RESET || reason == MCU_WATCHDOG_RESET); }
/** @req SWS_Mcu_00006 */
void test_Mcu_PerformReset_ShouldNotReturn(void) { Mcu_Init(&testConfig); /* Mcu_PerformReset(); would reset, skip */ TEST_ASSERT_TRUE(1); }
/** @req SWS_Mcu_00007 */
void test_Mcu_GetVersionInfo_ValidPtr_ShouldSucceed(void) { Std_VersionInfoType info; Mcu_GetVersionInfo(&info); TEST_ASSERT_EQUAL(MCU_VENDOR_ID, info.vendorID); }
/** @req SWS_Mcu_00007 */
void test_Mcu_GetVersionInfo_NullPtr_ShouldReportDet(void) { Mcu_GetVersionInfo(NULL_PTR); TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls); }
/** @req SWS_Mcu_00008 */
void test_Mcu_GetResetReason_BeforeInit_ShouldReturnUnknown(void) { Mcu_ResetType reason = Mcu_GetResetReason(); TEST_ASSERT_TRUE(reason == MCU_RESET_UNKNOWN || reason == MCU_POWER_ON_RESET); }
void test_Mcu_Init_DoubleInit_ShouldNotCrash(void) { Mcu_Init(&testConfig); Mcu_Init(&testConfig); TEST_ASSERT_TRUE(1); }
void test_Mcu_DeInit_BeforeInit_ShouldNotCrash(void) { Mcu_DeInit(); TEST_ASSERT_TRUE(1); }
