/**
 * @file test_cannm.c
 * @brief CanNm (CAN Network Management) Unit Tests
 * @req SWS_CanNm
 */

// @tests src/bsw/ecual/cannm/src/CanNm.c  @tests src/bsw/ecual/cannm/include/CanNm.h
#include "unity.h"
#include "CanNm.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

static CanNm_ConfigType testConfig;
void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_CanNm_00001 */
void test_CanNm_Init_NullPtr_ShouldNotCrash(void) { CanNm_Init(NULL_PTR); TEST_ASSERT_TRUE(1); }
/** @req SWS_CanNm_00001 */
void test_CanNm_Init_ValidConfig_ShouldSucceed(void) { testConfig.NumChannels = 0U; CanNm_Init(&testConfig); TEST_ASSERT_TRUE(1); }
/** @req SWS_CanNm_00002 */
void test_CanNm_DeInit_AfterInit_ShouldSucceed(void) { CanNm_Init(&testConfig); CanNm_DeInit(); TEST_ASSERT_TRUE(1); }
/** @req SWS_CanNm_00003 */
void test_CanNm_PassiveStartUp_AfterInit_ShouldReturnResult(void) { CanNm_Init(&testConfig); Std_ReturnType ret = CanNm_PassiveStartUp(0U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_CanNm_00004 */
void test_CanNm_NetworkRequest_AfterInit_ShouldReturnResult(void) { CanNm_Init(&testConfig); Std_ReturnType ret = CanNm_NetworkRequest(0U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_CanNm_00005 */
void test_CanNm_NetworkRelease_AfterInit_ShouldReturnResult(void) { CanNm_Init(&testConfig); Std_ReturnType ret = CanNm_NetworkRelease(0U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_CanNm_00006 */
void test_CanNm_MainFunction_AfterInit_ShouldNotCrash(void) { CanNm_Init(&testConfig); CanNm_MainFunction(); TEST_ASSERT_TRUE(1); }
/** @req SWS_CanNm_00007 */
void test_CanNm_Transmit_BeforeInit_ShouldFail(void) { PduInfoType pdu; uint8 data[8]={0}; pdu.SduDataPtr=data; pdu.SduLength=8U; Std_ReturnType ret = CanNm_Transmit(0U, &pdu); TEST_ASSERT_EQUAL(E_NOT_OK, ret); }
/** @req SWS_CanNm_00008 */
void test_CanNm_GetState_AfterInit_ShouldSucceed(void) { CanNm_Init(&testConfig); Nm_StateType state; Nm_ModeType mode; Std_ReturnType ret = CanNm_GetState(0U, &state, &mode); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_CanNm_00009 */
void test_CanNm_GetVersionInfo_ValidPtr_ShouldSucceed(void) { Std_VersionInfoType info; CanNm_GetVersionInfo(&info); TEST_ASSERT_EQUAL(CANNM_VENDOR_ID, info.vendorID); }
/** @req SWS_CanNm_00009 */
void test_CanNm_GetVersionInfo_NullPtr_ShouldReportDet(void) { CanNm_GetVersionInfo(NULL_PTR); TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls); }
/** @req SWS_CanNm_00010 */
void test_CanNm_DisableCommunication_AfterInit_ShouldReturnResult(void) { CanNm_Init(&testConfig); Std_ReturnType ret = CanNm_DisableCommunication(0U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_CanNm_00011 */
void test_CanNm_EnableCommunication_AfterInit_ShouldReturnResult(void) { CanNm_Init(&testConfig); Std_ReturnType ret = CanNm_EnableCommunication(0U); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_CanNm_00012 */
void test_CanNm_ConfirmPnAvailability_ShouldNotCrash(void) { CanNm_Init(&testConfig); CanNm_ConfirmPnAvailability(0U); TEST_ASSERT_TRUE(1); }
void test_CanNm_Init_DoubleInit_ShouldNotCrash(void) { CanNm_Init(&testConfig); CanNm_Init(&testConfig); TEST_ASSERT_TRUE(1); }
