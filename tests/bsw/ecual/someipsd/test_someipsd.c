/**
 * @file test_someipsd.c
 * @brief SomeIpSd Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/ecual/someipsd/src/SomeIpSd.c  @tests src/bsw/ecual/someipsd/include/SomeIpSd.h

#include "unity.h"
#include "SomeIpSd.h"
#include <string.h>

/* Mock Det_ReportError.
 * NOTE: the production SomeIpSd.c never calls Det_ReportError even though
 * SOMEIPSD_DEV_ERROR_DETECT is STD_ON in SomeIpSd_Cfg.h. The mock is kept
 * so tests assert the *actual* behavior (0 DET calls) and would catch any
 * future wiring change. */
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

/* Test config: one server service offered from startup */
static const SomeIpSd_ServiceConfigType testServices[1] =
{
    { 0x1234U, 0x0001U, 3000U, TRUE, 0U, 0U, 1U, 0U }
};

static const SomeIpSd_ConfigType testConfig =
{
    1U,               /* NumServices */
    testServices      /* Services */
};

void setUp(void) {
    mock_Det_Reset();
    /* Force a clean uninitialized state for every test */
    SomeIpSd_DeInit();
}

void tearDown(void) {
    SomeIpSd_DeInit();
}


/** @req SWS_SomeIpSd_00001 */
void test_SomeIpSd_Init_NullPtr_ShouldBeSafeNoOp(void) {
    /* SUT: Init(NULL) is accepted (config pointer stored unchecked) */
    SomeIpSd_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    /* Module transits to initialized state: OfferService works */
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_OfferService(0x1234U, 0x0001U));
}

/** @req SWS_SomeIpSd_00001 */
void test_SomeIpSd_Init_ValidConfig_ShouldSucceed(void) {
    SomeIpSd_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    /* Initialized: service management APIs are operational */
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_FindService(0x4321U, 0x0002U));
    TEST_ASSERT_EQUAL_INT(SD_STATE_DOWN,
        SomeIpSd_GetServiceState(0x4321U, 0x0002U));
}

/** @req SWS_SomeIpSd_00001 */
void test_SomeIpSd_Init_DoubleInit_ShouldSucceed(void) {
    SomeIpSd_Init(&testConfig);
    SomeIpSd_Init(&testConfig);
    /* SUT: re-init resets the internal state silently (no DET) */
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    /* Service table was reset: unknown service starts at SD_STATE_DOWN */
    TEST_ASSERT_EQUAL_INT(SD_STATE_DOWN,
        SomeIpSd_GetServiceState(0x1234U, 0x0001U));
}

/** @req SWS_SomeIpSd_00002 */
void test_SomeIpSd_DeInit_Uninit_ShouldBeSafeNoOp(void) {
    /* SUT: DeInit on uninitialized module is silent */
    SomeIpSd_DeInit();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_SomeIpSd_00002 */
void test_SomeIpSd_DeInit_ValidCall_ShouldResetState(void) {
    SomeIpSd_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_OfferService(0x1234U, 0x0001U));
    SomeIpSd_DeInit();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    /* After DeInit the service table is empty: all APIs refuse work */
    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIpSd_OfferService(0x1234U, 0x0001U));
    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIpSd_FindService(0x1234U, 0x0001U));
    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIpSd_StopOffer(0x1234U, 0x0001U));
    TEST_ASSERT_EQUAL_INT(SD_STATE_DOWN,
        SomeIpSd_GetServiceState(0x1234U, 0x0001U));
}

/** @req SWS_SomeIpSd_00003 */
void test_SomeIpSd_GetVersionInfo_NullPtr_ShouldBeSafeNoOp(void) {
    /* SUT: NULL pointer is silently ignored (no DET report) */
    SomeIpSd_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_SomeIpSd_00003 */
void test_SomeIpSd_GetVersionInfo_ValidPtr_ShouldReturnVersion(void) {
    Std_VersionInfoType vi;
    SomeIpSd_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(SOMEIPSD_VENDOR_ID, vi.vendorID);
    TEST_ASSERT_EQUAL_UINT16(SOMEIPSD_MODULE_ID, vi.moduleID);
    TEST_ASSERT_EQUAL_UINT8(1U, vi.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(0U, vi.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(0U, vi.sw_patch_version);
}

/** @req SWS_SomeIpSd_00004 */
void test_SomeIpSd_MainFunction_Uninit_ShouldBeSafeNoOp(void) {
    /* SUT: uninitialized MainFunction returns immediately */
    SomeIpSd_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_SomeIpSd_00004 */
void test_SomeIpSd_MainFunction_TtlExpiry_ShouldMarkNotAvailable(void) {
    SomeIpSd_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_OfferService(0x1234U, 0x0001U));
    TEST_ASSERT_EQUAL_INT(SD_STATE_AVAILABLE,
        SomeIpSd_GetServiceState(0x1234U, 0x0001U));

    /* Drain the TTL: OfferService sets RemainingTTL = 3000 */
    uint32 i;
    for (i = 0U; i < 3000U; i++) {
        SomeIpSd_MainFunction();
    }
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_INT(SD_STATE_NOT_AVAILABLE,
        SomeIpSd_GetServiceState(0x1234U, 0x0001U));
}

/** @req SWS_SomeIpSd_00005 */
void test_SomeIpSd_FindService_Uninit_ShouldReturnNotOk(void) {
    Std_ReturnType ret = SomeIpSd_FindService(0x1234U, 0x0001U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_SomeIpSd_00005 */
void test_SomeIpSd_FindService_ValidCall_ShouldRegisterService(void) {
    SomeIpSd_Init(&testConfig);
    Std_ReturnType ret = SomeIpSd_FindService(0x5555U, 0x0001U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    /* Service registered in DOWN state (waiting for an offer) */
    TEST_ASSERT_EQUAL_INT(SD_STATE_DOWN,
        SomeIpSd_GetServiceState(0x5555U, 0x0001U));
    /* Second FindService on the same tuple returns E_OK without duplicating */
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_FindService(0x5555U, 0x0001U));
}

/** @req SWS_SomeIpSd_00006 */
void test_SomeIpSd_OfferService_Uninit_ShouldReturnNotOk(void) {
    Std_ReturnType ret = SomeIpSd_OfferService(0x1234U, 0x0001U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_SomeIpSd_00006 */
void test_SomeIpSd_OfferService_ValidCall_ShouldMarkAvailable(void) {
    SomeIpSd_Init(&testConfig);
    Std_ReturnType ret = SomeIpSd_OfferService(0x7777U, 0x0001U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_INT(SD_STATE_AVAILABLE,
        SomeIpSd_GetServiceState(0x7777U, 0x0001U));
    /* Re-offer keeps the service available */
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_OfferService(0x7777U, 0x0001U));
    TEST_ASSERT_EQUAL_INT(SD_STATE_AVAILABLE,
        SomeIpSd_GetServiceState(0x7777U, 0x0001U));
}

/** @req SWS_SomeIpSd_00007 */
void test_SomeIpSd_StopOffer_Uninit_ShouldReturnNotOk(void) {
    Std_ReturnType ret = SomeIpSd_StopOffer(0x1234U, 0x0001U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_SomeIpSd_00007 */
void test_SomeIpSd_StopOffer_UnknownService_ShouldReturnNotOk(void) {
    SomeIpSd_Init(&testConfig);
    Std_ReturnType ret = SomeIpSd_StopOffer(0x9999U, 0x0001U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_SomeIpSd_00007 */
void test_SomeIpSd_StopOffer_ValidCall_ShouldMarkNotAvailable(void) {
    SomeIpSd_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_OfferService(0x1234U, 0x0001U));
    Std_ReturnType ret = SomeIpSd_StopOffer(0x1234U, 0x0001U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_INT(SD_STATE_NOT_AVAILABLE,
        SomeIpSd_GetServiceState(0x1234U, 0x0001U));
}

/** @req SWS_SomeIpSd_00008 */
void test_SomeIpSd_SubscribeEventGroup_Uninit_ShouldReturnNotOk(void) {
    Std_ReturnType ret = SomeIpSd_SubscribeEventGroup(0x1234U, 0x0001U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_SomeIpSd_00008 */
void test_SomeIpSd_SubscribeEventGroup_UnknownService_ShouldReturnNotOk(void) {
    SomeIpSd_Init(&testConfig);
    Std_ReturnType ret = SomeIpSd_SubscribeEventGroup(0x9999U, 0x0001U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_SomeIpSd_00008 */
void test_SomeIpSd_SubscribeEventGroup_ValidCall_ShouldSucceed(void) {
    SomeIpSd_Init(&testConfig);
    /* The SUT looks up (ServiceId, InstanceId=0): register with 0 first */
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_FindService(0x1234U, 0x0000U));
    Std_ReturnType ret = SomeIpSd_SubscribeEventGroup(0x1234U, 0x0001U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_SomeIpSd_00009 */
void test_SomeIpSd_RxIndication_OfferMessage_ShouldMarkServiceAvailable(void) {
    SomeIpSd_Init(&testConfig);
    /* Register a known service in DOWN state */
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_FindService(0x1234U, 0x0001U));
    TEST_ASSERT_EQUAL_INT(SD_STATE_DOWN,
        SomeIpSd_GetServiceState(0x1234U, 0x0001U));

    /* Build a minimal SOME/IP SD offer message:
     * 16 byte SOME/IP header + 16 byte SD entry (>= 32 bytes required). */
    uint8 msg[32];
    memset(msg, 0, sizeof(msg));
    msg[12] = 0x01U;             /* protocol version */
    msg[13] = 0x01U;             /* interface version */
    msg[14] = 0x01U;             /* message type: OFFER_SERVICE */
    msg[16] = 0x01U;             /* entry type: OFFER */
    msg[20] = 0x12U;             /* serviceId high (0x1234) */
    msg[21] = 0x34U;             /* serviceId low  */
    msg[22] = 0x00U;             /* instanceId high (0x0001) */
    msg[23] = 0x01U;             /* instanceId low  */
    PduInfoType pdu = { msg, 32U, NULL_PTR };

    SomeIpSd_RxIndication(0U, &pdu);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    /* Offer received: state must transition DOWN -> AVAILABLE */
    TEST_ASSERT_EQUAL_INT(SD_STATE_AVAILABLE,
        SomeIpSd_GetServiceState(0x1234U, 0x0001U));
}

/** @req SWS_SomeIpSd_00009 */
void test_SomeIpSd_RxIndication_NullOrTruncated_ShouldBeIgnored(void) {
    SomeIpSd_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, SomeIpSd_FindService(0x1234U, 0x0001U));

    /* NULL PduInfoPtr */
    SomeIpSd_RxIndication(0U, NULL_PTR);
    /* NULL SduDataPtr */
    PduInfoType emptyPdu = { NULL_PTR, 32U, NULL_PTR };
    SomeIpSd_RxIndication(0U, &emptyPdu);
    /* Truncated message (< 16 + 16 bytes) */
    uint8 small[31];
    memset(small, 0, sizeof(small));
    PduInfoType smallPdu = { small, 31U, NULL_PTR };
    SomeIpSd_RxIndication(0U, &smallPdu);

    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    /* Service remains in DOWN state: nothing was applied */
    TEST_ASSERT_EQUAL_INT(SD_STATE_DOWN,
        SomeIpSd_GetServiceState(0x1234U, 0x0001U));
}

/** @req SWS_SomeIpSd_00009 */
void test_SomeIpSd_GetServiceState_UnknownService_ShouldReturnDown(void) {
    SomeIpSd_Init(&testConfig);
    TEST_ASSERT_EQUAL_INT(SD_STATE_DOWN,
        SomeIpSd_GetServiceState(0xFFFFU, 0xFFFFU));
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}
