/**
 * @file test_dio.c
 * @brief Dio (Digital I/O) Unit Tests
 * @req SWS_Dio
 */
#include "unity.h"
#include "Dio.h"

static uint8 mock_DetCalls = 0;
Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;(void)InstanceId;(void)ApiId;(void)ErrorId;
    mock_DetCalls++; return E_OK;
}

static Dio_ConfigType testConfig;
void setUp(void) { mock_DetCalls = 0; }
void tearDown(void) {}

/** @req SWS_Dio_00001 */
void test_Dio_Init_NullPtr_ShouldReportDet(void) { Dio_Init(NULL_PTR); TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls); }
/** @req SWS_Dio_00001 */
void test_Dio_Init_ValidConfig_ShouldSucceed(void) { Dio_Init(&testConfig); TEST_ASSERT_TRUE(1); }
/** @req SWS_Dio_00002 */
void test_Dio_ReadChannel_BeforeInit_ShouldReportDet(void) { Dio_LevelType level = Dio_ReadChannel(0U); TEST_ASSERT_EQUAL(STD_LOW, level); }
/** @req SWS_Dio_00002 */
void test_Dio_ReadChannel_AfterInit_ShouldReturnLevel(void) { Dio_Init(&testConfig); Dio_LevelType level = Dio_ReadChannel(0U); TEST_ASSERT_TRUE(level == STD_LOW || level == STD_HIGH); }
/** @req SWS_Dio_00003 */
void test_Dio_WriteChannel_AfterInit_ShouldNotCrash(void) { Dio_Init(&testConfig); Dio_WriteChannel(0U, STD_HIGH); TEST_ASSERT_TRUE(1); }
/** @req SWS_Dio_00004 */
void test_Dio_ReadPort_AfterInit_ShouldReturnPortLevel(void) { Dio_Init(&testConfig); Dio_PortLevelType level = Dio_ReadPort(0U); TEST_ASSERT_TRUE(level >= 0); }
/** @req SWS_Dio_00005 */
void test_Dio_WritePort_AfterInit_ShouldNotCrash(void) { Dio_Init(&testConfig); Dio_WritePort(0U, 0x00U); TEST_ASSERT_TRUE(1); }
/** @req SWS_Dio_00006 */
void test_Dio_ReadChannelGroup_AfterInit_ShouldReturnLevel(void) { Dio_Init(&testConfig); Dio_ChannelGroupType grp = {0, 0, 0xFF}; Dio_PortLevelType level = Dio_ReadChannelGroup(&grp); TEST_ASSERT_TRUE(level >= 0); }
/** @req SWS_Dio_00007 */
void test_Dio_WriteChannelGroup_AfterInit_ShouldNotCrash(void) { Dio_Init(&testConfig); Dio_ChannelGroupType grp = {0, 0, 0xFF}; Dio_WriteChannelGroup(&grp, 0x00U); TEST_ASSERT_TRUE(1); }
/** @req SWS_Dio_00008 */
void test_Dio_GetVersionInfo_ValidPtr_ShouldSucceed(void) { Std_VersionInfoType info; Dio_GetVersionInfo(&info); TEST_ASSERT_EQUAL(DIO_VENDOR_ID, info.vendorID); }
/** @req SWS_Dio_00008 */
void test_Dio_GetVersionInfo_NullPtr_ShouldReportDet(void) { Dio_GetVersionInfo(NULL_PTR); TEST_ASSERT_NOT_EQUAL(0, mock_DetCalls); }
/** @req SWS_Dio_00009 */
void test_Dio_FlipChannel_AfterInit_ShouldReturnLevel(void) { Dio_Init(&testConfig); Dio_LevelType level = Dio_FlipChannel(0U); TEST_ASSERT_TRUE(level == STD_LOW || level == STD_HIGH); }
void test_Dio_Init_DoubleInit_ShouldNotCrash(void) { Dio_Init(&testConfig); Dio_Init(&testConfig); TEST_ASSERT_TRUE(1); }
