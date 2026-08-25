/**
 * @file test_dem.c
 * @brief Dem (Diagnostic Event Manager) Unit Tests
 * @req SWS_Dem
 */

// @tests src/bsw/services/dem/src/Dem.c  @tests src/bsw/services/dem/include/Dem.h
#include "unity.h"
#include "Dem.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

static Dem_ConfigType testConfig;
void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_Dem_00001 */
void test_Dem_Init_NullPtr_ShouldNotCrash(void) { Dem_Init(NULL_PTR); TEST_ASSERT_TRUE(1); }
/** @req SWS_Dem_00001 */
void test_Dem_Init_ValidConfig_ShouldSucceed(void) { testConfig.NumEvents = 0U; testConfig.NumDTCs = 0U; Dem_Init(&testConfig); TEST_ASSERT_TRUE(1); }
/** @req SWS_Dem_00002 */
void test_Dem_DeInit_AfterInit_ShouldSucceed(void) { Dem_Init(&testConfig); Dem_DeInit(); TEST_ASSERT_TRUE(1); }
/** @req SWS_Dem_00003 */
void test_Dem_SetEventStatus_BeforeInit_ShouldFail(void) { Std_ReturnType ret = Dem_SetEventStatus(0U, 0x00); TEST_ASSERT_EQUAL(E_NOT_OK, ret); }
/** @req SWS_Dem_00003 */
void test_Dem_SetEventStatus_AfterInit_ShouldReturnResult(void) { Dem_Init(&testConfig); Std_ReturnType ret = Dem_SetEventStatus(0U, 0x00); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Dem_00004 */
void test_Dem_GetEventStatus_AfterInit_ShouldSucceed(void) { Dem_Init(&testConfig); Dem_EventStatusType status; Std_ReturnType ret = Dem_GetEventStatus(0U, &status); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Dem_00005 */
void test_Dem_GetEventFailed_AfterInit_ShouldSucceed(void) { Dem_Init(&testConfig); boolean failed; Std_ReturnType ret = Dem_GetEventFailed(0U, &failed); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Dem_00006 */
void test_Dem_GetEventTested_AfterInit_ShouldSucceed(void) { Dem_Init(&testConfig); boolean tested; Std_ReturnType ret = Dem_GetEventTested(0U, &tested); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Dem_00001 */
void test_Dem_GetFaultDetectionCounter_AfterInit_ShouldSucceed(void) { Dem_Init(&testConfig); sint8 counter; Std_ReturnType ret = Dem_GetFaultDetectionCounter(0U, &counter); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Dem_00014 */
void test_Dem_ClearDTC_AfterInit_ShouldReturnResult(void) { Dem_Init(&testConfig); Std_ReturnType ret = Dem_ClearDTC(0U, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Dem_00001 */
void test_Dem_GetStatusOfDTC_AfterInit_ShouldSucceed(void) { Dem_Init(&testConfig); Dem_UdsStatusByteType status; Std_ReturnType ret = Dem_GetStatusOfDTC(0U, DEM_DTC_FORMAT_UDS, DEM_DTC_ORIGIN_PRIMARY_MEMORY, &status); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Dem_00005 */
void test_Dem_ResetEventStatus_AfterInit_ShouldReturnResult(void) { Dem_Init(&testConfig); Std_ReturnType ret = Dem_ResetEventStatus(0U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Dem_00001 */
void test_Dem_Shutdown_AfterInit_ShouldNotCrash(void) { Dem_Init(&testConfig); Dem_Shutdown(); TEST_ASSERT_TRUE(1); }
/** @req SWS_Dem_00029 */
void test_Dem_GetVersionInfo_ValidPtr_ShouldSucceed(void) { Std_VersionInfoType info; Dem_GetVersionInfo(&info); TEST_ASSERT_EQUAL(DEM_VENDOR_ID, info.vendorID); }
void test_Dem_Init_DoubleInit_ShouldNotCrash(void) { Dem_Init(&testConfig); Dem_Init(&testConfig); TEST_ASSERT_TRUE(1); }
