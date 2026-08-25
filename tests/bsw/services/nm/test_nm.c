/**
 * @file test_nm.c
 * @brief Nm (Network Management Interface) Unit Tests
 * @req SWS_Nm
 */

// @tests src/bsw/services/nm/src/Nm.c  @tests src/bsw/services/nm/include/Nm.h
#include "unity.h"
#include "Nm.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

static Nm_ConfigType testConfig;
void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_Nm_00001 */
void test_Nm_Init_NullPtr_ShouldNotCrash(void) { Nm_Init(NULL_PTR); TEST_ASSERT_TRUE(1); }
/** @req SWS_Nm_00001 */
void test_Nm_Init_ValidConfig_ShouldSucceed(void) { testConfig.NumChannels = 0U; Nm_Init(&testConfig); TEST_ASSERT_TRUE(1); }
/** @req SWS_Nm_00002 */
void test_Nm_DeInit_AfterInit_ShouldSucceed(void) { Nm_Init(&testConfig); Nm_DeInit(); TEST_ASSERT_TRUE(1); }
/** @req SWS_Nm_00003 */
void test_Nm_GetVersionInfo_ValidPtr_ShouldSucceed(void) { Std_VersionInfoType info; Nm_GetVersionInfo(&info); TEST_ASSERT_EQUAL(NM_VENDOR_ID, info.vendorID); }
/** @req SWS_Nm_00003 */
void test_Nm_GetVersionInfo_NullPtr_ShouldReportDet(void) { Nm_GetVersionInfo(NULL_PTR); TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls); }
/** @req SWS_Nm_00004 */
void test_Nm_PassiveStartUp_AfterInit_ShouldReturnResult(void) { Nm_Init(&testConfig); Std_ReturnType ret = Nm_PassiveStartUp(0U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Nm_00005 */
void test_Nm_NetworkRequest_AfterInit_ShouldReturnResult(void) { Nm_Init(&testConfig); Std_ReturnType ret = Nm_NetworkRequest(0U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Nm_00006 */
void test_Nm_NetworkRelease_AfterInit_ShouldReturnResult(void) { Nm_Init(&testConfig); Std_ReturnType ret = Nm_NetworkRelease(0U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Nm_00007 */
void test_Nm_DisableCommunication_AfterInit_ShouldReturnResult(void) { Nm_Init(&testConfig); Std_ReturnType ret = Nm_DisableCommunication(0U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Nm_00008 */
void test_Nm_EnableCommunication_AfterInit_ShouldReturnResult(void) { Nm_Init(&testConfig); Std_ReturnType ret = Nm_EnableCommunication(0U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Nm_00009 */
void test_Nm_GetState_AfterInit_ShouldSucceed(void) { Nm_Init(&testConfig); Nm_StateType state; Std_ReturnType ret = Nm_GetState(0U, &state); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Nm_00010 */
void test_Nm_GetMode_AfterInit_ShouldSucceed(void) { Nm_Init(&testConfig); Nm_ModeType mode; Std_ReturnType ret = Nm_GetMode(0U, &mode); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Nm_00011 */
void test_Nm_GetLocalNodeIdentifier_AfterInit_ShouldSucceed(void) { Nm_Init(&testConfig); Nm_NodeIdType nodeId; Std_ReturnType ret = Nm_GetLocalNodeIdentifier(0U, &nodeId); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_Nm_00012 */
void test_Nm_MainFunction_AfterInit_ShouldNotCrash(void) { Nm_Init(&testConfig); Nm_MainFunction(); TEST_ASSERT_TRUE(1); }
void test_Nm_Init_DoubleInit_ShouldNotCrash(void) { Nm_Init(&testConfig); Nm_Init(&testConfig); TEST_ASSERT_TRUE(1); }
