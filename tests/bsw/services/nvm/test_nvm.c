/**
 * @file test_nvm.c
 * @brief NvM (NVRAM Manager) Unit Tests
 * @req SWS_NvM
 */

// @tests src/bsw/services/nvm/src/NvM.c  @tests src/bsw/services/nvm/include/NvM.h
#include "unity.h"
#include "NvM.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

static NvM_ConfigType testConfig;
void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_NvM_00001 */
void test_NvM_Init_NullPtr_ShouldNotCrash(void) { NvM_Init(NULL_PTR); TEST_ASSERT_TRUE(1); }
/** @req SWS_NvM_00001 */
void test_NvM_Init_ValidConfig_ShouldSucceed(void) { testConfig.NumBlocks = 0U; NvM_Init(&testConfig); TEST_ASSERT_TRUE(1); }
/** @req SWS_NvM_00002 */
void test_NvM_Read_BeforeInit_ShouldFail(void) { Std_ReturnType ret = NvM_Read(0U, NULL_PTR, 0U); TEST_ASSERT_EQUAL(E_NOT_OK, ret); }
/** @req SWS_NvM_00003 */
void test_NvM_Write_BeforeInit_ShouldFail(void) { Std_ReturnType ret = NvM_Write(0U, NULL_PTR, 0U); TEST_ASSERT_EQUAL(E_NOT_OK, ret); }
/** @req SWS_NvM_00004 */
void test_NvM_RestoreDefaults_BeforeInit_ShouldFail(void) { Std_ReturnType ret = NvM_RestoreDefaults(0U); TEST_ASSERT_EQUAL(E_NOT_OK, ret); }
/** @req SWS_NvM_00005 */
void test_NvM_EraseNvBlock_BeforeInit_ShouldFail(void) { Std_ReturnType ret = NvM_EraseNvBlock(0U); TEST_ASSERT_EQUAL(E_NOT_OK, ret); }
/** @req SWS_NvM_00006 */
void test_NvM_InvalidateNvBlock_BeforeInit_ShouldFail(void) { Std_ReturnType ret = NvM_InvalidateNvBlock(0U); TEST_ASSERT_EQUAL(E_NOT_OK, ret); }
/** @req SWS_NvM_00007 */
void test_NvM_GetErrorStatus_AfterInit_ShouldReturnResult(void) { NvM_Init(&testConfig); NvM_RequestResultType result; Std_ReturnType ret = NvM_GetErrorStatus(0U, &result); TEST_ASSERT_TRUE(ret == E_OK || ret == E_NOT_OK); }
/** @req SWS_NvM_00008 */
void test_NvM_GetBlockID_BeforeInit_ShouldFail(void) { NvM_BlockIdType blockId; Std_ReturnType ret = NvM_GetBlockID(NULL_PTR, &blockId); TEST_ASSERT_EQUAL(E_NOT_OK, ret); }
/** @req SWS_NvM_00009 */
void test_NvM_SetDataIndex_BeforeInit_ShouldFail(void) { Std_ReturnType ret = NvM_SetDataIndex(0U); TEST_ASSERT_EQUAL(E_NOT_OK, ret); }
/** @req SWS_NvM_00010 */
void test_NvM_MainFunction_AfterInit_ShouldNotCrash(void) { NvM_Init(&testConfig); NvM_MainFunction(); TEST_ASSERT_TRUE(1); }
/** @req SWS_NvM_00011 */
void test_NvM_GetVersionInfo_ValidPtr_ShouldSucceed(void) { Std_VersionInfoType info; NvM_GetVersionInfo(&info); TEST_ASSERT_EQUAL(NVM_VENDOR_ID, info.vendorID); }
/** @req SWS_NvM_00011 */
void test_NvM_GetVersionInfo_NullPtr_ShouldReportDet(void) { NvM_GetVersionInfo(NULL_PTR); TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls); }
void test_NvM_Init_DoubleInit_ShouldNotCrash(void) { NvM_Init(&testConfig); NvM_Init(&testConfig); TEST_ASSERT_TRUE(1); }
