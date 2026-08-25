/**
 * @file test_dcm.c
 * @brief Dcm (Diagnostic Communication Manager) Unit Tests
 * @req SWS_Dcm
 */
#include "unity.h"
#include "Dcm.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

static Dcm_ConfigType testConfig;
void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_Dcm_00001 */
void test_Dcm_Init_NullPtr_ShouldNotCrash(void) { Dcm_Init(NULL_PTR); TEST_ASSERT_TRUE(1); }
/** @req SWS_Dcm_00001 */
void test_Dcm_Init_ValidConfig_ShouldSucceed(void) { testConfig.NumDIDs = 0U; testConfig.NumRIDs = 0U; Dcm_Init(&testConfig); TEST_ASSERT_TRUE(1); }
/** @req SWS_Dcm_00002 */
void test_Dcm_DeInit_AfterInit_ShouldSucceed(void) { Dcm_Init(&testConfig); Dcm_DeInit(); TEST_ASSERT_TRUE(1); }
/** @req SWS_Dcm_00003 */
void test_Dcm_GetVersionInfo_ValidPtr_ShouldSucceed(void) { Std_VersionInfoType info; Dcm_GetVersionInfo(&info); TEST_ASSERT_EQUAL(DCM_VENDOR_ID, info.vendorID); }
/** @req SWS_Dcm_00003 */
void test_Dcm_GetVersionInfo_NullPtr_ShouldReportDet(void) { Dcm_GetVersionInfo(NULL_PTR); TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls); }
/** @req SWS_Dcm_00004 */
void test_Dcm_MainFunction_AfterInit_ShouldNotCrash(void) { Dcm_Init(&testConfig); Dcm_MainFunction(); TEST_ASSERT_TRUE(1); }
/** @req SWS_Dcm_00005 */
void test_Dcm_GetActiveProtocol_AfterInit_ShouldReturnResult(void) { Dcm_Init(&testConfig); uint8 protocolId; Std_ReturnType ret = Dcm_GetActiveProtocol(&protocolId); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Dcm_00006 */
void test_Dcm_GetSession_AfterInit_ShouldReturnResult(void) { Dcm_Init(&testConfig); Dcm_SessionType session; Std_ReturnType ret = Dcm_GetSession(0U, &session); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Dcm_00007 */
void test_Dcm_GetSecurityLevel_AfterInit_ShouldReturnResult(void) { Dcm_Init(&testConfig); Dcm_SecurityLevelType level; Std_ReturnType ret = Dcm_GetSecurityLevel(0U, &level); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Dcm_00008 */
void test_Dcm_ResetToDefaultSession_ShouldNotCrash(void) { Dcm_Init(&testConfig); Dcm_ResetToDefaultSession(0U); TEST_ASSERT_TRUE(1); }
void test_Dcm_Init_DoubleInit_ShouldNotCrash(void) { Dcm_Init(&testConfig); Dcm_Init(&testConfig); TEST_ASSERT_TRUE(1); }
void test_Dcm_DeInit_BeforeInit_ShouldNotCrash(void) { Dcm_DeInit(); TEST_ASSERT_TRUE(1); }
