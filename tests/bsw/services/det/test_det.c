/**
 * @file test_det.c
 * @brief Det (Default Error Tracer) Unit Tests
 * @req SWS_Det
 */

// @tests src/bsw/services/det/src/Det.c  @tests src/bsw/services/det/include/Det.h
#include "unity.h"
#include "Det.h"

void setUp(void)
{
}

void tearDown(void)
{
}

/** @req SWS_Det_00001 */
void test_Det_Init_ValidConfig_ShouldSucceed(void)
{
    Det_ConfigType cfg = {0};
    Det_Init(&cfg);
    TEST_ASSERT_TRUE(DetInitialized);
}

/** @req SWS_Det_00001 */
void test_Det_Init_NullPtr_ShouldNotCrash(void)
{
    Det_Init(NULL_PTR);
    /* No crash; if already initialized the call is ignored. */
}

/** @req SWS_Det_00001 */
void test_Det_Init_DoubleInit_ShouldNotCrash(void)
{
    Det_ConfigType cfg = {0};
    Det_Init(&cfg);
    Det_Init(&cfg);
    TEST_ASSERT_TRUE(DetInitialized);
}

/** @req SWS_Det_00002 */
void test_Det_Start_AfterInit_ShouldNotCrash(void)
{
    Det_ConfigType cfg = {0};
    Det_Init(&cfg);
    Det_Start();
    TEST_ASSERT_EQUAL(E_OK, Det_ReportError(0x01U, 0U, 0x01U, 0x01U));
}

/** @req SWS_Det_00003 */
void test_Det_ReportError_AfterInit_ShouldReturnOk(void)
{
    Det_ConfigType cfg = {0};
    Det_Init(&cfg);
    Det_Start();
    TEST_ASSERT_EQUAL(E_OK, Det_ReportError(0x01U, 0U, 0x01U, 0x01U));
}

/** @req SWS_Det_00003 */
void test_Det_ReportError_BeforeInit_ShouldReturnOk(void)
{
    TEST_ASSERT_EQUAL(E_OK, Det_ReportError(0x01U, 0U, 0x01U, 0x01U));
}

/** @req SWS_Det_00004 */
void test_Det_ReportRuntimeError_AfterInit_ShouldReturnOk(void)
{
    Det_ConfigType cfg = {0};
    Det_Init(&cfg);
    Det_Start();
    TEST_ASSERT_EQUAL(E_OK, Det_ReportRuntimeError(0x01U, 0U, 0x01U, 0x01U));
}

/** @req SWS_Det_00005 */
void test_Det_ReportTransientFault_AfterInit_ShouldReturnOk(void)
{
    Det_ConfigType cfg = {0};
    Det_Init(&cfg);
    Det_Start();
    TEST_ASSERT_EQUAL(E_OK, Det_ReportTransientFault(0x01U, 0U, 0x01U, 0x01U));
}

/** @req SWS_Det_00006 */
void test_Det_GetVersionInfo_ValidPtr_ShouldSucceed(void)
{
    Std_VersionInfoType info;
    Det_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL(DET_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL(DET_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL(DET_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL(DET_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL(DET_SW_PATCH_VERSION, info.sw_patch_version);
}

/** @req SWS_Det_00006 */
void test_Det_GetVersionInfo_NullPtr_ShouldNotCrash(void)
{
    Det_GetVersionInfo(NULL_PTR);
    /* No crash; DET-enabled build reports internally. */
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_Det_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_Det_Init_NullPtr_ShouldNotCrash);
    RUN_TEST(test_Det_Init_DoubleInit_ShouldNotCrash);
    RUN_TEST(test_Det_Start_AfterInit_ShouldNotCrash);
    RUN_TEST(test_Det_ReportError_AfterInit_ShouldReturnOk);
    RUN_TEST(test_Det_ReportError_BeforeInit_ShouldReturnOk);
    RUN_TEST(test_Det_ReportRuntimeError_AfterInit_ShouldReturnOk);
    RUN_TEST(test_Det_ReportTransientFault_AfterInit_ShouldReturnOk);
    RUN_TEST(test_Det_GetVersionInfo_ValidPtr_ShouldSucceed);
    RUN_TEST(test_Det_GetVersionInfo_NullPtr_ShouldNotCrash);

    return UnityEnd();
}
