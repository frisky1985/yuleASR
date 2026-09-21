/**
 * @file test_ecuc.c
 * @brief EcuC Unit Tests — Substantiated
 * @version 1.0.0
 * @date 2026-08-25
 *
 * Substantiation: rewritten against the real EcuC API (EcuC.h). All
 * assertions verify return values, config get/set round trips and exact
 * DET mock arguments (module/service/error IDs).
 */

// @tests src/bsw/services/ecuC/src/EcuC.c  @tests src/bsw/services/ecuC/include/EcuC.h

#include "unity.h"
#include "EcuC.h"

/* Service IDs and error codes from EcuC.c / EcuC_Cfg.h */
#define ECUC_SID_INIT               0x00U
#define ECUC_SID_DEINIT             0x01U
#define ECUC_SID_GET_CONFIG         0x02U
#define ECUC_SID_SET_CONFIG         0x03U
#define ECUC_SID_GET_VERSION_INFO   0x04U
#define ECUC_E_UNINIT               0x20U
#define ECUC_E_INVALID_CONFIG_ID    0xFFFFU

/* Mock Det_ReportError — records full argument set for verification */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint16 mock_DetLastModuleId = 0U;
static uint8 mock_DetCallCount = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetLastModuleId = 0U;
    mock_DetCallCount = 0U;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)InstanceId;
    mock_DetLastModuleId = ModuleId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

/* Test config */
static EcuC_ConfigType testConfig;
static void test_EcuC_SetupDefaultConfig(void) {
    testConfig.CoreFrequency  = 160000000UL;
    testConfig.BusFrequency   = 80000000UL;
    testConfig.RamSize        = 0x200000UL;
    testConfig.FlashSize      = 0x400000UL;
    testConfig.EepromSize     = 0x8000UL;
    testConfig.CanBaudrate    = 500000UL;
    testConfig.LinBaudrate    = 19200UL;
    testConfig.SpiFrequency   = 1000000UL;
    testConfig.PduCount       = 0U;
    testConfig.SignalCount    = 0U;
    testConfig.RoutingPathCount = 0U;
    testConfig.Pdus           = NULL_PTR;
    testConfig.Signals        = NULL_PTR;
    testConfig.RoutingPaths   = NULL_PTR;
}

void setUp(void) {
    mock_Det_Reset();
    test_EcuC_SetupDefaultConfig();
    /* Force a known UNINIT state before every test (EcuC_State is file-static). */
    EcuC_DeInit();
}

void tearDown(void) {
}

/** @req SWS_EcuC_00001 */
void test_EcuC_Init_NullPtr_ShouldReportDet(void) {
    EcuC_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(ECUC_MODULE_ID, mock_DetLastModuleId);
    TEST_ASSERT_EQUAL_UINT8(ECUC_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ECUC_E_PARAM_POINTER, mock_DetLastErrorId);
    /* Still uninitialized: config read must fail. */
    uint32 value = 0U;
    TEST_ASSERT_EQUAL(E_NOT_OK, EcuC_GetConfigValue(ECUC_CONFIG_ID_CORE_FREQ, &value));
}

/** @req SWS_EcuC_00001 */
void test_EcuC_Init_ValidConfig_ShouldSucceed(void) {
    EcuC_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    uint32 value = 0U;
    TEST_ASSERT_EQUAL(E_OK, EcuC_GetConfigValue(ECUC_CONFIG_ID_CORE_FREQ, &value));
    TEST_ASSERT_EQUAL_UINT32(160000000UL, value);
}

/** @req SWS_EcuC_00001 */
void test_EcuC_Init_DoubleInit_ShouldAdoptLatestConfig(void) {
    EcuC_Init(&testConfig);
    EcuC_ConfigType otherConfig = testConfig;
    otherConfig.CoreFrequency = 1UL;
    /* Production Init is not guarded: the second init silently replaces the config. */
    EcuC_Init(&otherConfig);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    uint32 value = 0U;
    TEST_ASSERT_EQUAL(E_OK, EcuC_GetConfigValue(ECUC_CONFIG_ID_CORE_FREQ, &value));
    TEST_ASSERT_EQUAL_UINT32(1UL, value);
}

/** @req SWS_EcuC_00002 */
void test_EcuC_DeInit_Uninit_ShouldNotReportDet(void) {
    /* Production EcuC_DeInit() has no DET guard — it silently resets. */
    EcuC_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_EcuC_00002 */
void test_EcuC_DeInit_ValidCall_ShouldReturnToUninit(void) {
    EcuC_Init(&testConfig);
    EcuC_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    uint32 value = 0U;
    TEST_ASSERT_EQUAL(E_NOT_OK, EcuC_GetConfigValue(ECUC_CONFIG_ID_CORE_FREQ, &value));
    TEST_ASSERT_EQUAL_UINT8(ECUC_SID_GET_CONFIG, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ECUC_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_EcuC_00004 */
void test_EcuC_GetConfigValue_Uninit_ShouldReportDet(void) {
    uint32 value = 0U;
    TEST_ASSERT_EQUAL(E_NOT_OK, EcuC_GetConfigValue(ECUC_CONFIG_ID_CORE_FREQ, &value));
    TEST_ASSERT_EQUAL_UINT8(ECUC_SID_GET_CONFIG, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ECUC_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_EcuC_00004 */
void test_EcuC_GetConfigValue_NullPtr_ShouldReportDet(void) {
    EcuC_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, EcuC_GetConfigValue(ECUC_CONFIG_ID_CORE_FREQ, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(ECUC_SID_GET_CONFIG, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ECUC_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_EcuC_00004 */
void test_EcuC_GetConfigValue_InvalidId_ShouldReturnNotOk(void) {
    EcuC_Init(&testConfig);
    uint32 value = 0U;
    /* Unknown ConfigId hits the default branch: E_NOT_OK without any DET call. */
    TEST_ASSERT_EQUAL(E_NOT_OK, EcuC_GetConfigValue(ECUC_E_INVALID_CONFIG_ID, &value));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_EcuC_00005 */
void test_EcuC_SetConfigValue_ValidId_ShouldUpdateAndReadBack(void) {
    EcuC_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, EcuC_SetConfigValue(ECUC_CONFIG_ID_RAM_SIZE, 0x100000UL));
    uint32 value = 0U;
    TEST_ASSERT_EQUAL(E_OK, EcuC_GetConfigValue(ECUC_CONFIG_ID_RAM_SIZE, &value));
    TEST_ASSERT_EQUAL_UINT32(0x100000UL, value);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_EcuC_00005 */
void test_EcuC_SetConfigValue_Uninit_ShouldReportDet(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, EcuC_SetConfigValue(ECUC_CONFIG_ID_CORE_FREQ, 0UL));
    TEST_ASSERT_EQUAL_UINT8(ECUC_SID_SET_CONFIG, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ECUC_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_EcuC_00003 */
void test_EcuC_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    EcuC_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(ECUC_MODULE_ID, mock_DetLastModuleId);
    TEST_ASSERT_EQUAL_UINT8(ECUC_SID_GET_VERSION_INFO, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(ECUC_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_EcuC_00003 */
void test_EcuC_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType info = {0U, 0U, 0U, 0U, 0U};
    EcuC_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(ECUC_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT8(ECUC_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT8(1U, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(0U, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(0U, info.sw_patch_version);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_EcuC_Init_NullPtr_ShouldReportDet);
    RUN_TEST(test_EcuC_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_EcuC_Init_DoubleInit_ShouldAdoptLatestConfig);
    RUN_TEST(test_EcuC_DeInit_Uninit_ShouldNotReportDet);
    RUN_TEST(test_EcuC_DeInit_ValidCall_ShouldReturnToUninit);
    RUN_TEST(test_EcuC_GetConfigValue_Uninit_ShouldReportDet);
    RUN_TEST(test_EcuC_GetConfigValue_NullPtr_ShouldReportDet);
    RUN_TEST(test_EcuC_GetConfigValue_InvalidId_ShouldReturnNotOk);
    RUN_TEST(test_EcuC_SetConfigValue_ValidId_ShouldUpdateAndReadBack);
    RUN_TEST(test_EcuC_SetConfigValue_Uninit_ShouldReportDet);
    RUN_TEST(test_EcuC_GetVersionInfo_NullPtr_ShouldReportDet);
    RUN_TEST(test_EcuC_GetVersionInfo_ValidPtr_ShouldSucceed);

    return UnityEnd();
}
