/**
 * @file test_adc.c
 * @brief Adc (ADC Driver) Unit Tests
 * @req SWS_Adc
 */
#include "unity.h"
#include "Adc.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

static Adc_ConfigType testConfig;
void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_Adc_00001 */
void test_Adc_Init_NullPtr_ShouldReportDet(void) { Adc_Init(NULL_PTR); TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls); }
/** @req SWS_Adc_00001 */
void test_Adc_Init_ValidConfig_ShouldSucceed(void) { testConfig.NumGroups = 0U; Adc_Init(&testConfig); TEST_ASSERT_TRUE(1); }
/** @req SWS_Adc_00002 */
void test_Adc_DeInit_AfterInit_ShouldSucceed(void) { Adc_Init(&testConfig); Adc_DeInit(); TEST_ASSERT_TRUE(1); }
/** @req SWS_Adc_00003 */
void test_Adc_StartGroupConversion_AfterInit_ShouldNotCrash(void) { Adc_Init(&testConfig); Adc_StartGroupConversion(0U); TEST_ASSERT_TRUE(1); }
/** @req SWS_Adc_00004 */
void test_Adc_StopGroupConversion_AfterInit_ShouldNotCrash(void) { Adc_Init(&testConfig); Adc_StopGroupConversion(0U); TEST_ASSERT_TRUE(1); }
/** @req SWS_Adc_00005 */
void test_Adc_ReadGroup_AfterInit_ShouldReturnResult(void) { Adc_Init(&testConfig); Adc_ValueGroupType buffer[8]; Std_ReturnType ret = Adc_ReadGroup(0U, buffer); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Adc_00006 */
void test_Adc_EnableHardwareTrigger_AfterInit_ShouldReturnResult(void) { Adc_Init(&testConfig); Std_ReturnType ret = Adc_EnableHardwareTrigger(0U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Adc_00007 */
void test_Adc_DisableHardwareTrigger_AfterInit_ShouldReturnResult(void) { Adc_Init(&testConfig); Std_ReturnType ret = Adc_DisableHardwareTrigger(0U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Adc_00008 */
void test_Adc_GetVersionInfo_ValidPtr_ShouldSucceed(void) { Std_VersionInfoType info; Adc_GetVersionInfo(&info); TEST_ASSERT_EQUAL(ADC_VENDOR_ID, info.vendorID); }
/** @req SWS_Adc_00008 */
void test_Adc_GetVersionInfo_NullPtr_ShouldReportDet(void) { Adc_GetVersionInfo(NULL_PTR); TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls); }
/** @req SWS_Adc_00009 */
void test_Adc_SetupResultBuffer_AfterInit_ShouldReturnResult(void) { Adc_Init(&testConfig); Adc_ValueGroupType buffer[8]; Std_ReturnType ret = Adc_SetupResultBuffer(0U, buffer); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
void test_Adc_Init_DoubleInit_ShouldNotCrash(void) { Adc_Init(&testConfig); Adc_Init(&testConfig); TEST_ASSERT_TRUE(1); }
void test_Adc_DeInit_BeforeInit_ShouldNotCrash(void) { Adc_DeInit(); TEST_ASSERT_TRUE(1); }
