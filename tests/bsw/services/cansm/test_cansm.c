/**
 * @file test_cansm.c
 * @brief CanSM (CAN State Manager) Unit Tests
 * @req SWS_CanSM
 *
 * Scope note: in the current production sources only CanSm_GetVersionInfo is
 * implemented (src/bsw/services/cansm/src/CanSm.c, guarded by
 * CANSM_VERSION_INFO_API == STD_ON). The other CanSM_* entry points declared
 * in CanSm.h have no implementation yet, so the substantiated tests below
 * target the real, implemented API. The SUT reports development errors via
 * Det_ReportError (CANSM_DEV_ERROR_DETECT == STD_ON); a local mock captures
 * the report arguments.
 */

// @tests src/bsw/services/cansm/src/CanSm.c  @tests src/bsw/services/cansm/include/CanSm.h
#include "unity.h"
#include "CanSm.h"

/* SUT header/source name mismatch: CanSm.h declares CanSM_GetVersionInfo
 * (capital SM) but CanSm.c defines CanSm_GetVersionInfo. The implemented
 * symbol is what we test; production sources must not be modified. */
extern void CanSm_GetVersionInfo(Std_VersionInfoType* versioninfo);

/* Mock Det_ReportError — captures report arguments (CANSM_DEV_ERROR_DETECT = STD_ON) */
static uint16 mock_DetLastModuleId = 0xFFFFU;
static uint8 mock_DetLastInstanceId = 0xFFU;
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint8 mock_DetCallCount = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastModuleId = 0xFFFFU;
    mock_DetLastInstanceId = 0xFFU;
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetCallCount = 0U;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    mock_DetLastModuleId = ModuleId;
    mock_DetLastInstanceId = InstanceId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

void setUp(void) {
    mock_Det_Reset();
}

void tearDown(void) {
}

/** @req SWS_CanSM_00003 */
void test_CanSm_GetVersionInfo_ValidPtr_FillsVendorId(void) {
    Std_VersionInfoType info = {0U, 0U, 0U, 0U, 0U};
    CanSm_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT16(CANSM_VENDOR_ID, info.vendorID);
}

/** @req SWS_CanSM_00003 */
void test_CanSm_GetVersionInfo_ValidPtr_FillsModuleId(void) {
    Std_VersionInfoType info = {0U, 0U, 0U, 0U, 0U};
    CanSm_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT16(CANSM_MODULE_ID, info.moduleID);
}

/** @req SWS_CanSM_00003 */
void test_CanSm_GetVersionInfo_ValidPtr_FillsSwVersion(void) {
    Std_VersionInfoType info = {0U, 0U, 0U, 0U, 0U};
    CanSm_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT8(CANSM_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(CANSM_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(CANSM_SW_PATCH_VERSION, info.sw_patch_version);
}

/** @req SWS_CanSM_00003 */
void test_CanSm_GetVersionInfo_ValidPtr_DoesNotReportDet(void) {
    Std_VersionInfoType info;
    CanSm_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_CanSM_00003 */
void test_CanSm_GetVersionInfo_NullPtr_ReportsDetWithExactIds(void) {
    CanSm_GetVersionInfo(NULL_PTR);
    /* SUT hardcodes ApiId 0x02 in the NULL-pointer guard (not CANSM_SID_GETVERSIONINFO) */
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(CANSM_MODULE_ID, mock_DetLastModuleId);
    TEST_ASSERT_EQUAL_UINT8(CANSM_INSTANCE_ID, mock_DetLastInstanceId);
    TEST_ASSERT_EQUAL_UINT8(0x02U, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(CANSM_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_CanSM_00003 */
void test_CanSm_GetVersionInfo_NullThenValid_RecoversCleanly(void) {
    CanSm_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    Std_VersionInfoType info = {0U, 0U, 0U, 0U, 0U};
    CanSm_GetVersionInfo(&info);
    /* Valid call after the NULL report: no new report, version still filled */
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(CANSM_VENDOR_ID, info.vendorID);
}

/** @req SWS_CanSM_00003 */
void test_CanSm_GetVersionInfo_RepeatedCalls_KeepSilentDet(void) {
    Std_VersionInfoType info;
    CanSm_GetVersionInfo(&info);
    CanSm_GetVersionInfo(&info);
    CanSm_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_CanSm_GetVersionInfo_ValidPtr_FillsVendorId);
    RUN_TEST(test_CanSm_GetVersionInfo_ValidPtr_FillsModuleId);
    RUN_TEST(test_CanSm_GetVersionInfo_ValidPtr_FillsSwVersion);
    RUN_TEST(test_CanSm_GetVersionInfo_ValidPtr_DoesNotReportDet);
    RUN_TEST(test_CanSm_GetVersionInfo_NullPtr_ReportsDetWithExactIds);
    RUN_TEST(test_CanSm_GetVersionInfo_NullThenValid_RecoversCleanly);
    RUN_TEST(test_CanSm_GetVersionInfo_RepeatedCalls_KeepSilentDet);

    return UnityEnd();
}
