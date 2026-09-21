/**
 * @file test_sd.c
 * @brief Sd (Service Discovery) Unit Tests — substantiated
 * @version 1.0.0
 * @date 2026-08-25
 *
 * NOTE: The production SUT (src/bsw/services/sd) does not provide
 * Sd_RequestService/Sd_ReleaseService/Sd_SubscribeEventgroup/
 * Sd_UnsubscribeEventgroup/Sd_GetServiceState. Those legacy test names are
 * remapped onto the real APIs as follows:
 *   RequestService      -> Sd_OfferService
 *   ReleaseService      -> Sd_StopService
 *   SubscribeEventgroup -> Sd_SubscribeEventGroup
 *   UnsubscribeEventgroup -> Sd_UnsubscribeEventGroup
 *   GetServiceState     -> Sd_FindService (+ Sd_HandleMessage/Sd_SetEventStatus)
 * The legacy config field NumServices does not exist; Sd_ConfigType carries
 * MaxServices/MaxSubscriptions/OfferCycleTimeMs/FindCycleTimeMs/TtlDefault/
 * DevErrorDetect/VersionInfoApi.
 */

// @tests src/bsw/services/sd/src/Sd.c  @tests src/bsw/services/sd/include/Sd.h

#include "unity.h"
#include "Sd.h"
#include <string.h>

/* Mock Det_ReportError — records ApiId/ErrorId/call count */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint8 mock_DetCallCount = 0U;

static void mock_Det_Reset(void) {
    mock_DetLastApiId = 0xFFU;
    mock_DetLastErrorId = 0xFFU;
    mock_DetCallCount = 0U;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId) {
    (void)ModuleId;
    (void)InstanceId;
    mock_DetLastApiId = ApiId;
    mock_DetLastErrorId = ErrorId;
    mock_DetCallCount++;
    return E_OK;
}

/* Test config — uses the real Sd_ConfigType fields */
static Sd_ConfigType testConfig;
static void test_Sd_SetupDefaultConfig(void) {
    memset(&testConfig, 0, sizeof(testConfig));
    testConfig.MaxServices = 4U;
    testConfig.MaxSubscriptions = 4U;
    testConfig.OfferCycleTimeMs = 1000U;
    testConfig.FindCycleTimeMs = 500U;
    testConfig.TtlDefault = 3U;
    testConfig.DevErrorDetect = TRUE;
    testConfig.VersionInfoApi = TRUE;
}

static void test_Sd_DoInit(void) {
    test_Sd_SetupDefaultConfig();
    Sd_Init(&testConfig);
}

/* Force the module back to the uninitialized state regardless of the
 * previous test outcome, then clear the DET log (a spurious DeInit on an
 * already-uninitialized SUT only produces a DET report). */
void setUp(void) {
    Sd_DeInit();
    mock_Det_Reset();
}

void tearDown(void) {
}

/* Test endpoint used by Offer/Find tests */
static const Sd_Ipv4EndpointType testEndpoint = {
    0xC0A80001UL,   /* Addr 192.168.0.1 */
    30509U,         /* Port */
    SD_PROTO_UDP
};

/** @req SWS_Sd_00001 */
void test_Sd_Init_NullPtr_ShouldNotCrash(void) {
    Sd_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SD_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SD_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Sd_00001 */
void test_Sd_Init_ValidConfig_ShouldSucceed(void) {
    Std_ReturnType ret;
    test_Sd_SetupDefaultConfig();
    Sd_Init(&testConfig);
    /* Initialization is observable: a previously-rejected API now succeeds */
    ret = Sd_HandleMessage(NULL_PTR, 0U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Sd_00001 */
void test_Sd_Init_DoubleInit_ShouldReportError(void) {
    test_Sd_DoInit();
    Sd_Init(&testConfig);   /* second init */
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SD_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SD_E_ALREADY_INITIALIZED, mock_DetLastErrorId);
    /* Module remains initialized and usable after the rejected re-init */
    TEST_ASSERT_EQUAL_UINT8(E_OK, Sd_HandleMessage(NULL_PTR, 0U));
}

/** @req SWS_Sd_00002 */
void test_Sd_DeInit_Uninit_ShouldReportError(void) {
    /* Not initialized */
    Sd_DeInit();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SD_SID_DEINIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SD_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Sd_00002 */
void test_Sd_DeInit_ValidCall_ShouldSucceed(void) {
    Std_ReturnType ret;
    test_Sd_DoInit();
    Sd_DeInit();
    /* After DeInit the module is uninitialized again: HandleMessage rejects */
    ret = Sd_HandleMessage(NULL_PTR, 0U);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
}

/** @req SWS_Sd_00003 */
void test_Sd_GetVersionInfo_NullPtr_ShouldReportError(void) {
    Sd_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SD_SID_GETVERSIONINFO, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SD_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Sd_00003 */
void test_Sd_GetVersionInfo_ValidPtr_ShouldSucceed(void) {
    Std_VersionInfoType versionInfo;
    memset(&versionInfo, 0, sizeof(versionInfo));
    Sd_GetVersionInfo(&versionInfo);
    TEST_ASSERT_EQUAL_UINT16(SD_VENDOR_ID, versionInfo.vendorID);
    TEST_ASSERT_EQUAL_UINT8(SD_MODULE_ID, versionInfo.moduleID);
    TEST_ASSERT_EQUAL_UINT8(SD_SW_MAJOR_VERSION, versionInfo.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(SD_SW_MINOR_VERSION, versionInfo.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(SD_SW_PATCH_VERSION, versionInfo.sw_patch_version);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Sd_00004 */
void test_Sd_MainFunction_Uninit_ShouldNotCrash(void) {
    /* Not initialized — the SUT returns silently without reporting to DET
     * (unlike the other Sd APIs). Documented SUT behaviour. */
    Sd_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Sd_00004 */
void test_Sd_MainFunction_ValidCall_ShouldSucceed(void) {
    Std_ReturnType ret;
    test_Sd_DoInit();
    Sd_MainFunction();
    Sd_MainFunction();
    /* MainFunction is void; observable effect: module still fully operative */
    ret = Sd_OfferService(0x1001U, 0x0001U, 1U, 0U, &testEndpoint);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Sd_00005 — remapped to Sd_OfferService */
void test_Sd_OfferService_Uninit_ShouldReportError(void) {
    Std_ReturnType ret;
    /* Not initialized */
    ret = Sd_OfferService(0x1001U, 0x0001U, 1U, 0U, &testEndpoint);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SD_SID_OFFERSERVICE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SD_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Sd_00005 — remapped to Sd_OfferService (invalid = NULL endpoint) */
void test_Sd_OfferService_NullEndpoint_ShouldReportError(void) {
    Std_ReturnType ret;
    test_Sd_DoInit();
    ret = Sd_OfferService(0x1001U, 0x0001U, 1U, 0U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(SD_SID_OFFERSERVICE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SD_E_PARAM_POINTER, mock_DetLastErrorId);
}

/** @req SWS_Sd_00005 — remapped to Sd_OfferService */
void test_Sd_OfferService_ValidCall_ShouldSucceed(void) {
    Std_ReturnType ret;
    test_Sd_DoInit();
    ret = Sd_OfferService(0x1001U, 0x0001U, 1U, 0U, &testEndpoint);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    /* Offering the same (ServiceId, InstanceId) again updates the entry */
    ret = Sd_OfferService(0x1001U, 0x0001U, 2U, 1U, &testEndpoint);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Sd_00006 — remapped to Sd_StopService */
void test_Sd_StopService_Uninit_ShouldReportError(void) {
    Std_ReturnType ret;
    /* Not initialized */
    ret = Sd_StopService(0x1001U, 0x0001U);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SD_SID_STOPSERVICE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SD_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Sd_00006 — remapped to Sd_StopService (unknown service: silent E_NOT_OK) */
void test_Sd_StopService_UnknownService_ShouldReturnNotOk(void) {
    Std_ReturnType ret;
    test_Sd_DoInit();
    ret = Sd_StopService(0x7777U, 0x0001U);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Sd_00006 — remapped to Sd_StopService */
void test_Sd_StopService_ValidCall_ShouldSucceed(void) {
    Std_ReturnType ret;
    test_Sd_DoInit();
    (void)Sd_OfferService(0x1001U, 0x0001U, 1U, 0U, &testEndpoint);
    ret = Sd_StopService(0x1001U, 0x0001U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    /* Stopping again: entry was removed, so E_NOT_OK without DET */
    ret = Sd_StopService(0x1001U, 0x0001U);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Sd_00007 — remapped to Sd_SubscribeEventGroup */
void test_Sd_SubscribeEventGroup_Uninit_ShouldReportError(void) {
    Std_ReturnType ret;
    /* Not initialized */
    ret = Sd_SubscribeEventGroup(0x1001U, 0x0001U, 3U);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SD_SID_SUBSCRIBEEVENTGROUP, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SD_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Sd_00007 — duplicate subscription is accepted silently (no DET) */
void test_Sd_SubscribeEventGroup_Duplicate_ShouldReturnOk(void) {
    Std_ReturnType ret;
    test_Sd_DoInit();
    ret = Sd_SubscribeEventGroup(0x1001U, 0x0001U, 3U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    ret = Sd_SubscribeEventGroup(0x1001U, 0x0001U, 3U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Sd_00007 — remapped to Sd_SubscribeEventGroup */
void test_Sd_SubscribeEventGroup_ValidCall_ShouldSucceed(void) {
    Std_ReturnType ret;
    test_Sd_DoInit();
    ret = Sd_SubscribeEventGroup(0x1001U, 0x0001U, 3U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Sd_00008 — remapped to Sd_UnsubscribeEventGroup */
void test_Sd_UnsubscribeEventGroup_Uninit_ShouldReportError(void) {
    Std_ReturnType ret;
    /* Not initialized */
    ret = Sd_UnsubscribeEventGroup(0x1001U, 0x0001U, 3U);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SD_SID_UNSUBSCRIBEEVENTGROUP, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SD_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Sd_00008 — unknown event group: silent E_NOT_OK */
void test_Sd_UnsubscribeEventGroup_Unknown_ShouldReturnNotOk(void) {
    Std_ReturnType ret;
    test_Sd_DoInit();
    ret = Sd_UnsubscribeEventGroup(0x1001U, 0x0001U, 9U);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Sd_00008 — remapped to Sd_UnsubscribeEventGroup */
void test_Sd_UnsubscribeEventGroup_ValidCall_ShouldSucceed(void) {
    Std_ReturnType ret;
    test_Sd_DoInit();
    (void)Sd_SubscribeEventGroup(0x1001U, 0x0001U, 3U);
    ret = Sd_UnsubscribeEventGroup(0x1001U, 0x0001U, 3U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    /* Entry removed: unsubscribing again fails silently */
    ret = Sd_UnsubscribeEventGroup(0x1001U, 0x0001U, 3U);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Sd_00009 — remapped to Sd_FindService */
void test_Sd_FindService_Uninit_ShouldReportError(void) {
    Std_ReturnType ret;
    Sd_Ipv4EndpointType endpoint;
    /* Not initialized */
    ret = Sd_FindService(0x1001U, 0x0001U, &endpoint);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SD_SID_FINDSERVICE, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SD_E_UNINIT, mock_DetLastErrorId);
}

/** @req SWS_Sd_00009 — unknown service in the found-registry: silent E_NOT_OK */
void test_Sd_FindService_UnknownService_ShouldReturnNotOk(void) {
    Std_ReturnType ret;
    Sd_Ipv4EndpointType endpoint;
    test_Sd_DoInit();
    ret = Sd_FindService(0x7777U, 0xFFFFU, &endpoint);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_Sd_00009 — service state via HandleMessage + SetEventStatus lifecycle */
void test_Sd_SetEventStatus_ValidCall_ShouldSucceed(void) {
    Std_ReturnType ret;
    test_Sd_DoInit();
    /* HandleMessage is a stub that accepts any buffer once initialized */
    ret = Sd_HandleMessage(NULL_PTR, 0U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    /* Subscribe an event group (PENDING), then mark it READY -> SUBSCRIBED */
    ret = Sd_SubscribeEventGroup(0x1001U, 0x0001U, 3U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    ret = Sd_SetEventStatus(0x1001U, 0x0001U, 3U, SD_EVENTGROUP_READY);
    TEST_ASSERT_EQUAL_UINT8(E_OK, ret);
    /* Unknown event group: silent E_NOT_OK */
    ret = Sd_SetEventStatus(0x1001U, 0x0001U, 9U, SD_EVENTGROUP_READY);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_Sd_Init_NullPtr_ShouldNotCrash);
    RUN_TEST(test_Sd_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_Sd_Init_DoubleInit_ShouldReportError);
    RUN_TEST(test_Sd_DeInit_Uninit_ShouldReportError);
    RUN_TEST(test_Sd_DeInit_ValidCall_ShouldSucceed);
    RUN_TEST(test_Sd_GetVersionInfo_NullPtr_ShouldReportError);
    RUN_TEST(test_Sd_GetVersionInfo_ValidPtr_ShouldSucceed);
    RUN_TEST(test_Sd_MainFunction_Uninit_ShouldNotCrash);
    RUN_TEST(test_Sd_MainFunction_ValidCall_ShouldSucceed);
    RUN_TEST(test_Sd_OfferService_Uninit_ShouldReportError);
    RUN_TEST(test_Sd_OfferService_NullEndpoint_ShouldReportError);
    RUN_TEST(test_Sd_OfferService_ValidCall_ShouldSucceed);
    RUN_TEST(test_Sd_StopService_Uninit_ShouldReportError);
    RUN_TEST(test_Sd_StopService_UnknownService_ShouldReturnNotOk);
    RUN_TEST(test_Sd_StopService_ValidCall_ShouldSucceed);
    RUN_TEST(test_Sd_SubscribeEventGroup_Uninit_ShouldReportError);
    RUN_TEST(test_Sd_SubscribeEventGroup_Duplicate_ShouldReturnOk);
    RUN_TEST(test_Sd_SubscribeEventGroup_ValidCall_ShouldSucceed);
    RUN_TEST(test_Sd_UnsubscribeEventGroup_Uninit_ShouldReportError);
    RUN_TEST(test_Sd_UnsubscribeEventGroup_Unknown_ShouldReturnNotOk);
    RUN_TEST(test_Sd_UnsubscribeEventGroup_ValidCall_ShouldSucceed);
    RUN_TEST(test_Sd_FindService_Uninit_ShouldReportError);
    RUN_TEST(test_Sd_FindService_UnknownService_ShouldReturnNotOk);
    RUN_TEST(test_Sd_SetEventStatus_ValidCall_ShouldSucceed);
    return UnityEnd();
}
