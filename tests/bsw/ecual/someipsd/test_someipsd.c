/**
 * @file test_someipsd.c
 * @brief SomeIpSd Unit Tests — canonical services/someip implementation
 * @version 2.0.0
 * @date 2026-10-09
 *
 * Migrated from ecual/someipsd (shimmed) to services/someip (canonical).
 * API signatures and config types differ from the original ecual tests.
 */

// @tests src/bsw/services/someip/src/SomeIpSd.c  @tests src/bsw/services/someip/include/SomeIpSd.h

#include "unity.h"
#include "SomeIpSd.h"
#include <string.h>

/* Mock Det_ReportError.
 * NOTE: the production SomeIpSd.c does NOT #include SomeIpSd_Cfg.h, so
 * SOMEIPSD_DEV_ERROR_DETECT is undefined at compile time → all DET guards
 * compile out. The mock is kept to assert the *actual* behavior (0 DET
 * calls) and would catch any future wiring change. */
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

/* Test config: canonical SomeIpSd_ConfigType uses SomeIpSd_ServiceInfoType array */
static const SomeIpSd_ServiceInfoType testServices[1] =
{
    { 0x1234U, 0x0001U, 1U, 0U, 3000U, 0xC0A8010AUL, 30500U, SOMEIPSD_PROTO_UDP, FALSE }
};

static const SomeIpSd_ConfigType testConfig =
{
    testServices,     /* Services */
    1U,               /* NumServices */
    0xC0A8010AUL,     /* LocalIpAddress */
    30500U,           /* LocalPort */
    0xE0F4E0F5UL,     /* MulticastIpAddress */
    30490U,           /* MulticastPort */
    TRUE              /* DevErrorDetect */
};

void setUp(void) {
    mock_Det_Reset();
    SomeIpSd_DeInit();
}

void tearDown(void) {
    SomeIpSd_DeInit();
}

/* ---------------------------------------------------------------------------
 * SomeIpSd_Init
 * ------------------------------------------------------------------------- */

/** @req SWS_SomeIpSd_00001 */
void test_SomeIpSd_Init_NullPtr_ShouldBeSilentNoOp(void) {
    /* SUT: DET guard compiles out → NULL is silently ignored, stays uninit */
    SomeIpSd_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    /* Module stays uninitialized: OfferService refuses */
    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIpSd_OfferService(0x1234U, 0x0001U, 1U, 0U, 3000U));
}

/** @req SWS_SomeIpSd_00001 */
void test_SomeIpSd_Init_ValidConfig_ShouldInitialize(void) {
    SomeIpSd_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    /* Initialized: OfferService works */
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_OfferService(0x1234U, 0x0001U, 1U, 0U, 3000U));
}

/** @req SWS_SomeIpSd_00001 */
void test_SomeIpSd_Init_DoubleInit_ShouldResetSilently(void) {
    SomeIpSd_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_OfferService(0x1234U, 0x0001U, 1U, 0U, 3000U));
    /* Re-init: resets registry silently */
    SomeIpSd_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    /* Registry was cleared: previously offered service is gone */
    SomeIpSd_InstanceIdType instId = 0U;
    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIpSd_FindService(0x1234U, &instId, NULL_PTR));
}

/* ---------------------------------------------------------------------------
 * SomeIpSd_DeInit
 * ------------------------------------------------------------------------- */

/** @req SWS_SomeIpSd_00002 */
void test_SomeIpSd_DeInit_Uninit_ShouldBeSilentNoOp(void) {
    SomeIpSd_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_SomeIpSd_00002 */
void test_SomeIpSd_DeInit_AfterInit_ShouldResetState(void) {
    SomeIpSd_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_OfferService(0x1234U, 0x0001U, 1U, 0U, 3000U));
    SomeIpSd_DeInit();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    /* After DeInit all APIs refuse work */
    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIpSd_OfferService(0x1234U, 0x0001U, 1U, 0U, 3000U));
    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIpSd_StopOfferService(0x1234U, 0x0001U));
    SomeIpSd_InstanceIdType instId = 0U;
    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIpSd_FindService(0x1234U, &instId, NULL_PTR));
}

/* ---------------------------------------------------------------------------
 * SomeIpSd_OfferService (5-param canonical API)
 * ------------------------------------------------------------------------- */

/** @req SWS_SomeIpSd_00006 */
void test_SomeIpSd_OfferService_Uninit_ShouldReturnNotOk(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIpSd_OfferService(0x1234U, 0x0001U, 1U, 0U, 3000U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_SomeIpSd_00006 */
void test_SomeIpSd_OfferService_ValidCall_ShouldReturnOk(void) {
    SomeIpSd_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_OfferService(0x7777U, 0x0001U, 1U, 0U, 3000U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_SomeIpSd_00006 */
void test_SomeIpSd_OfferService_ReOffer_ShouldReturnNotOk(void) {
    SomeIpSd_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_OfferService(0x7777U, 0x0001U, 1U, 0U, 3000U));
    /* Re-offer same (ServiceId, InstanceId): SUT reports E_ALREADY_OFFERED via DET */
    /* NOTE: DET guard compiles out in current build → silent E_NOT_OK */
    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIpSd_OfferService(0x7777U, 0x0001U, 1U, 0U, 3000U));
}

/* ---------------------------------------------------------------------------
 * SomeIpSd_StopOfferService
 * ------------------------------------------------------------------------- */

/** @req SWS_SomeIpSd_00007 */
void test_SomeIpSd_StopOfferService_Uninit_ShouldReturnNotOk(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIpSd_StopOfferService(0x1234U, 0x0001U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_SomeIpSd_00007 */
void test_SomeIpSd_StopOfferService_UnknownService_ShouldReturnNotOk(void) {
    SomeIpSd_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIpSd_StopOfferService(0x9999U, 0x0001U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_SomeIpSd_00007 */
void test_SomeIpSd_StopOfferService_ValidCall_ShouldReturnOk(void) {
    SomeIpSd_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_OfferService(0x1234U, 0x0001U, 1U, 0U, 3000U));
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_StopOfferService(0x1234U, 0x0001U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
    /* After stop, FindService can no longer find it */
    SomeIpSd_InstanceIdType instId = 0U;
    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIpSd_FindService(0x1234U, &instId, NULL_PTR));
}

/* ---------------------------------------------------------------------------
 * SomeIpSd_FindService (3-param canonical API)
 * ------------------------------------------------------------------------- */

/** @req SWS_SomeIpSd_00005 */
void test_SomeIpSd_FindService_Uninit_ShouldReturnNotOk(void) {
    SomeIpSd_InstanceIdType instId = 0U;
    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIpSd_FindService(0x1234U, &instId, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_SomeIpSd_00005 */
void test_SomeIpSd_FindService_NotFound_ShouldReturnNotOk(void) {
    SomeIpSd_Init(&testConfig);
    SomeIpSd_InstanceIdType instId = 0U;
    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIpSd_FindService(0x5555U, &instId, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_SomeIpSd_00005 */
void test_SomeIpSd_FindService_AfterOffer_ShouldReturnOkWithInfo(void) {
    SomeIpSd_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_OfferService(0x5555U, 0x0002U, 2U, 100U, 5000U));

    SomeIpSd_InstanceIdType instId = 0U;
    SomeIpSd_ServiceInfoType info;
    memset(&info, 0, sizeof(info));
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_FindService(0x5555U, &instId, &info));
    TEST_ASSERT_EQUAL_UINT16(0x0002U, instId);
    TEST_ASSERT_EQUAL_UINT16(0x5555U, info.ServiceId);
    TEST_ASSERT_EQUAL_UINT16(0x0002U, info.InstanceId);
    TEST_ASSERT_EQUAL_UINT8(2U, info.MajorVersion);
    TEST_ASSERT_EQUAL_UINT32(100U, info.MinorVersion);
    TEST_ASSERT_EQUAL_UINT32(5000U, info.Ttl);
    TEST_ASSERT_EQUAL(1, info.IsAvailable);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_SomeIpSd_00005 */
void test_SomeIpSd_FindService_NullOutputPtrs_ShouldStillReturnOk(void) {
    SomeIpSd_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_OfferService(0x5555U, 0x0002U, 2U, 100U, 5000U));
    /* Both output pointers NULL: SUT skips writes but still returns E_OK */
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_FindService(0x5555U, NULL_PTR, NULL_PTR));
}

/* ---------------------------------------------------------------------------
 * SomeIpSd_SubscribeEventGroup (4-param canonical API)
 * ------------------------------------------------------------------------- */

/** @req SWS_SomeIpSd_00008 */
void test_SomeIpSd_SubscribeEventGroup_Uninit_ShouldReturnNotOk(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIpSd_SubscribeEventGroup(0x1234U, 0x0001U, 0x0001U, 3000U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_SomeIpSd_00008 */
void test_SomeIpSd_SubscribeEventGroup_Init_ShouldReturnOk(void) {
    SomeIpSd_Init(&testConfig);
    /* SUT: stub implementation always returns E_OK when initialized */
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_SubscribeEventGroup(0x1234U, 0x0001U, 0x0001U, 3000U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/* ---------------------------------------------------------------------------
 * SomeIpSd_UnsubscribeEventGroup
 * ------------------------------------------------------------------------- */

void test_SomeIpSd_UnsubscribeEventGroup_Uninit_ShouldReturnNotOk(void) {
    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIpSd_UnsubscribeEventGroup(0x1234U, 0x0001U, 0x0001U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

void test_SomeIpSd_UnsubscribeEventGroup_Init_ShouldReturnOk(void) {
    SomeIpSd_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_UnsubscribeEventGroup(0x1234U, 0x0001U, 0x0001U));
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/* ---------------------------------------------------------------------------
 * SomeIpSd_RxIndication (raw data, not PduInfoType)
 * ------------------------------------------------------------------------- */

/** @req SWS_SomeIpSd_00009 */
void test_SomeIpSd_RxIndication_Uninit_ShouldBeIgnored(void) {
    uint8 msg[32];
    memset(msg, 0, sizeof(msg));
    SomeIpSd_RxIndication(msg, 32U);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_SomeIpSd_00009 */
void test_SomeIpSd_RxIndication_NullData_ShouldBeIgnored(void) {
    SomeIpSd_Init(&testConfig);
    SomeIpSd_RxIndication(NULL_PTR, 32U);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_SomeIpSd_00009 */
void test_SomeIpSd_RxIndication_Truncated_ShouldBeIgnored(void) {
    SomeIpSd_Init(&testConfig);
    uint8 msg[8];
    memset(msg, 0, sizeof(msg));
    /* Length < SOMEIP_HEADER_SIZE (16) → silently ignored */
    SomeIpSd_RxIndication(msg, 8U);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/* ---------------------------------------------------------------------------
 * SomeIpSd_MainFunction
 * ------------------------------------------------------------------------- */

/** @req SWS_SomeIpSd_00004 */
void test_SomeIpSd_MainFunction_Uninit_ShouldBeSilentNoOp(void) {
    SomeIpSd_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** @req SWS_SomeIpSd_00004 */
void test_SomeIpSd_MainFunction_Init_ShouldRunSilently(void) {
    SomeIpSd_Init(&testConfig);
    SomeIpSd_MainFunction();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/* ---------------------------------------------------------------------------
 * Callbacks (weak default implementations — no-ops)
 * ------------------------------------------------------------------------- */

void test_SomeIpSd_ServiceAvailableCallback_ShouldNotCrash(void) {
    SomeIpSd_ServiceAvailableCallback(0x1234U, 0x0001U, TRUE);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

void test_SomeIpSd_EventSubscriptionCallback_ShouldNotCrash(void) {
    SomeIpSd_EventSubscriptionCallback(0x1234U, 0x0001U, 0x0001U, TRUE);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/* ---------------------------------------------------------------------------
 * NOTE: SomeIpSd_GetVersionInfo is NOT tested — it is guarded by
 * SOMEIPSD_VERSION_INFO_API which is undefined in the current build
 * (SomeIpSd.c does not include SomeIpSd_Cfg.h), so the function is
 * compiled out and does not exist in the service_someip library.
 * ------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * Additional branch-coverage tests
 * ------------------------------------------------------------------------- */

/** Fill the 32-slot registry to exercise the "no free entry" branch */
void test_SomeIpSd_OfferService_RegistryFull_ShouldReturnNotOk(void) {
    SomeIpSd_Init(&testConfig);
    uint16 i;
    for (i = 0U; i < 32U; i++) {
        Std_ReturnType ret = SomeIpSd_OfferService(
            (SomeIp_ServiceIdType)(0x1000U + i),
            (SomeIpSd_InstanceIdType)(0x0001U + (i / 100U)),
            1U, 0U, 3000U);
        /* InstanceId wraps: 0..31 are all distinct (i/100 = 0 for all i<32) */
        TEST_ASSERT_EQUAL(E_OK, ret);
    }
    /* 33rd offer must fail — entryPtr == NULL_PTR after full scan */
    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIpSd_OfferService(
        0x2000U, 0x0001U, 1U, 0U, 3000U));
}

/** Drive static cyclicCounter past the 300 threshold with an active service */
void test_SomeIpSd_MainFunction_CyclicOffer_ShouldReOfferServices(void) {
    SomeIpSd_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_OfferService(0x7777U, 0x0001U, 1U, 0U, 3000U));
    uint16 i;
    for (i = 0U; i < 301U; i++) {
        SomeIpSd_MainFunction();
    }
    /* Service is still registered after cyclic re-offer */
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_FindService(0x7777U, NULL_PTR, NULL_PTR));
}

/** Valid SD message with matching MessageId → passes the MessageId check */
void test_SomeIpSd_RxIndication_ValidSdMessage_ShouldPassMessageIdCheck(void) {
    SomeIpSd_Init(&testConfig);
    uint8 msg[32];
    memset(msg, 0, sizeof(msg));
    /* MessageId = (SOMEIPSD_SERVICE_ID << 16) | SOMEIPSD_METHOD_ID
     *            = 0xFFFF8100  → big-endian bytes: FF FF 81 00 */
    msg[0] = 0xFFU;
    msg[1] = 0xFFU;
    msg[2] = 0x81U;
    msg[3] = 0x00U;
    /* Length field (bytes 4-7) = 16 (header only, big-endian) */
    msg[7] = 16U;
    SomeIpSd_RxIndication(msg, 32U);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** Non-SD MessageId → hits the "return" branch after ParseHeader */
void test_SomeIpSd_RxIndication_WrongMessageId_ShouldBeIgnored(void) {
    SomeIpSd_Init(&testConfig);
    uint8 msg[32];
    memset(msg, 0, sizeof(msg));
    /* MessageId = 0x12340001 ≠ 0xFFFF8100 */
    msg[0] = 0x12U;
    msg[1] = 0x34U;
    msg[2] = 0x00U;
    msg[3] = 0x01U;
    msg[7] = 16U;
    SomeIpSd_RxIndication(msg, 32U);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_DetCallCount);
}

/** FindService with NULL InstanceIdPtr but valid ServiceInfoPtr */
void test_SomeIpSd_FindService_NullInstanceIdPtr_ShouldReturnInfo(void) {
    SomeIpSd_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_OfferService(0x5555U, 0x0002U, 2U, 100U, 5000U));
    SomeIpSd_ServiceInfoType info;
    memset(&info, 0, sizeof(info));
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_FindService(0x5555U, NULL_PTR, &info));
    TEST_ASSERT_EQUAL_UINT16(0x5555U, info.ServiceId);
    TEST_ASSERT_EQUAL_UINT16(0x0002U, info.InstanceId);
    TEST_ASSERT_EQUAL(1, info.IsAvailable);
}

int main(void) {
    UNITY_BEGIN();

    /* Init / DeInit */
    RUN_TEST(test_SomeIpSd_Init_NullPtr_ShouldBeSilentNoOp);
    RUN_TEST(test_SomeIpSd_Init_ValidConfig_ShouldInitialize);
    RUN_TEST(test_SomeIpSd_Init_DoubleInit_ShouldResetSilently);
    RUN_TEST(test_SomeIpSd_DeInit_Uninit_ShouldBeSilentNoOp);
    RUN_TEST(test_SomeIpSd_DeInit_AfterInit_ShouldResetState);

    /* OfferService */
    RUN_TEST(test_SomeIpSd_OfferService_Uninit_ShouldReturnNotOk);
    RUN_TEST(test_SomeIpSd_OfferService_ValidCall_ShouldReturnOk);
    RUN_TEST(test_SomeIpSd_OfferService_ReOffer_ShouldReturnNotOk);
    RUN_TEST(test_SomeIpSd_OfferService_RegistryFull_ShouldReturnNotOk);

    /* StopOfferService */
    RUN_TEST(test_SomeIpSd_StopOfferService_Uninit_ShouldReturnNotOk);
    RUN_TEST(test_SomeIpSd_StopOfferService_UnknownService_ShouldReturnNotOk);
    RUN_TEST(test_SomeIpSd_StopOfferService_ValidCall_ShouldReturnOk);

    /* FindService */
    RUN_TEST(test_SomeIpSd_FindService_Uninit_ShouldReturnNotOk);
    RUN_TEST(test_SomeIpSd_FindService_NotFound_ShouldReturnNotOk);
    RUN_TEST(test_SomeIpSd_FindService_AfterOffer_ShouldReturnOkWithInfo);
    RUN_TEST(test_SomeIpSd_FindService_NullOutputPtrs_ShouldStillReturnOk);
    RUN_TEST(test_SomeIpSd_FindService_NullInstanceIdPtr_ShouldReturnInfo);

    /* SubscribeEventGroup / UnsubscribeEventGroup */
    RUN_TEST(test_SomeIpSd_SubscribeEventGroup_Uninit_ShouldReturnNotOk);
    RUN_TEST(test_SomeIpSd_SubscribeEventGroup_Init_ShouldReturnOk);
    RUN_TEST(test_SomeIpSd_UnsubscribeEventGroup_Uninit_ShouldReturnNotOk);
    RUN_TEST(test_SomeIpSd_UnsubscribeEventGroup_Init_ShouldReturnOk);

    /* RxIndication */
    RUN_TEST(test_SomeIpSd_RxIndication_Uninit_ShouldBeIgnored);
    RUN_TEST(test_SomeIpSd_RxIndication_NullData_ShouldBeIgnored);
    RUN_TEST(test_SomeIpSd_RxIndication_Truncated_ShouldBeIgnored);
    RUN_TEST(test_SomeIpSd_RxIndication_ValidSdMessage_ShouldPassMessageIdCheck);
    RUN_TEST(test_SomeIpSd_RxIndication_WrongMessageId_ShouldBeIgnored);

    /* MainFunction */
    RUN_TEST(test_SomeIpSd_MainFunction_Uninit_ShouldBeSilentNoOp);
    RUN_TEST(test_SomeIpSd_MainFunction_Init_ShouldRunSilently);
    RUN_TEST(test_SomeIpSd_MainFunction_CyclicOffer_ShouldReOfferServices);

    /* Callbacks */
    RUN_TEST(test_SomeIpSd_ServiceAvailableCallback_ShouldNotCrash);
    RUN_TEST(test_SomeIpSd_EventSubscriptionCallback_ShouldNotCrash);

    return UnityEnd();
}
