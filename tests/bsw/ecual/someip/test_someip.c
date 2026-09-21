/**
 * @file test_someip.c
 * @brief SomeIp Unit Tests
 * @version 1.0.0
 * @date 2026-08-25
 */

// @tests src/bsw/services/someip/src/SomeIp.c  @tests src/bsw/services/someip/include/SomeIp.h

#include "unity.h"
#include "SomeIp.h"

/* Mock Det_ReportError */
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

/* Test config: one service entry, one client entry */
static const SomeIp_ServiceConfigType testServices[1] =
{
    { 0x1234U, 0x0001U, SOMEIP_MSG_REQUEST, NULL_PTR }
};

static const SomeIp_ClientConfigType testClients[1] =
{
    { 0x0001U, 0x0000U }
};

static const SomeIp_ConfigType testConfig =
{
    testServices,   /* Services */
    1U,             /* NumServices */
    testClients,    /* Clients */
    1U,             /* NumClients */
    TRUE,           /* DevErrorDetect */
    TRUE            /* VersionInfoApi */
};

void setUp(void) {
    mock_Det_Reset();
    /* Force a clean uninitialized state for every test */
    SomeIp_DeInit();
}

void tearDown(void) {
    SomeIp_DeInit();
}


/** @req SWS_SomeIp_00001 */
void test_SomeIp_Init_NullPtr_ShouldReportError(void) {
    SomeIp_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(SOMEIP_INIT_SID, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(SOMEIP_E_INVALID_POINTER, mock_DetLastErrorId);
    /* Uninitialized: any Tx API must fail */
    TEST_ASSERT_EQUAL(E_NOT_OK,
        SomeIp_SendRequest(1U, 2U, 3U, NULL_PTR, 0U));
}

/** @req SWS_SomeIp_00001 */
void test_SomeIp_Init_ValidConfig_ShouldSucceed(void) {
    SomeIp_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    /* Initialized: Tx APIs accept calls */
    TEST_ASSERT_EQUAL(E_OK,
        SomeIp_SendRequest(1U, 2U, 3U, NULL_PTR, 0U));
}

/** @req SWS_SomeIp_00001 */
void test_SomeIp_Init_DoubleInit_ShouldSucceed(void) {
    SomeIp_Init(&testConfig);
    SomeIp_Init(&testConfig);
    /* SUT: no ALREADY_INITIALIZED guard; re-init accepted silently */
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL(E_OK,
        SomeIp_SendRequest(1U, 2U, 3U, NULL_PTR, 0U));
}

/** @req SWS_SomeIp_00002 */
void test_SomeIp_DeInit_Uninit_ShouldBeSilentNoOp(void) {
    /* SUT: DeInit on uninitialized module returns without DET report */
    SomeIp_DeInit();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    /* Still uninitialized afterwards */
    TEST_ASSERT_EQUAL(E_NOT_OK,
        SomeIp_SendNotification(1U, 2U, NULL_PTR, 0U));
}

/** @req SWS_SomeIp_00002 */
void test_SomeIp_DeInit_ValidCall_ShouldResetState(void) {
    SomeIp_Init(&testConfig);
    SomeIp_DeInit();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    /* After DeInit all Tx APIs reject calls again */
    TEST_ASSERT_EQUAL(E_NOT_OK,
        SomeIp_SendRequest(1U, 2U, 3U, NULL_PTR, 0U));
    TEST_ASSERT_EQUAL(E_NOT_OK,
        SomeIp_SendResponse(0x00010002U, NULL_PTR, 0U, SOMEIP_RET_OK));
    TEST_ASSERT_EQUAL(E_NOT_OK,
        SomeIp_SendNotification(1U, 2U, NULL_PTR, 0U));
}

/** @req SWS_SomeIp_00003 */
void test_SomeIp_GetVersionInfo_NullPtr_ShouldBeSilentNoOp(void) {
    /* SUT: NULL pointer is silently ignored (no DET report) */
    SomeIp_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_SomeIp_00003 */
void test_SomeIp_GetVersionInfo_ValidPtr_ShouldReturnVersion(void) {
    Std_VersionInfoType vi;
    SomeIp_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT16(0x0001U, vi.vendorID);          /* SOMEIP_VENDOR_ID */
    TEST_ASSERT_EQUAL_UINT16(MODULE_ID_SOMEIP, vi.moduleID);
    TEST_ASSERT_EQUAL_UINT8(SOMEIP_SW_MAJOR_VERSION, vi.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(SOMEIP_SW_MINOR_VERSION, vi.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(SOMEIP_SW_PATCH_VERSION, vi.sw_patch_version);
}

/** @req SWS_SomeIp_00004 */
void test_SomeIp_SendRequest_Uninit_ShouldReturnNotOk(void) {
    Std_ReturnType ret = SomeIp_SendRequest(0x0102U, 0x1234U, 0x0001U, NULL_PTR, 0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_SomeIp_00004 */
void test_SomeIp_SendRequest_ValidCall_ShouldReturnOk(void) {
    SomeIp_Init(&testConfig);
    Std_ReturnType ret = SomeIp_SendRequest(0x0102U, 0x1234U, 0x0001U, NULL_PTR, 0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_SomeIp_00005 */
void test_SomeIp_SendResponse_Uninit_ShouldReturnNotOk(void) {
    Std_ReturnType ret = SomeIp_SendResponse(0x00010002U, NULL_PTR, 0U, SOMEIP_RET_OK);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_SomeIp_00005 */
void test_SomeIp_SendResponse_ValidCall_ShouldReturnOk(void) {
    SomeIp_Init(&testConfig);
    Std_ReturnType ret = SomeIp_SendResponse(0x00010002U, NULL_PTR, 0U, SOMEIP_RET_OK);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_SomeIp_00006 */
void test_SomeIp_SendNotification_Uninit_ShouldReturnNotOk(void) {
    Std_ReturnType ret = SomeIp_SendNotification(0x1234U, 0x8001U, NULL_PTR, 0U);
    TEST_ASSERT_EQUAL(E_NOT_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_SomeIp_00006 */
void test_SomeIp_SendNotification_ValidCall_ShouldReturnOk(void) {
    SomeIp_Init(&testConfig);
    Std_ReturnType ret = SomeIp_SendNotification(0x1234U, 0x8001U, NULL_PTR, 0U);
    TEST_ASSERT_EQUAL(E_OK, ret);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_SomeIp_00007 */
void test_SomeIp_RxIndication_UninitOrBadInput_ShouldBeIgnored(void) {
    /* Uninitialized: call is silently ignored */
    uint8 data[SOMEIP_HEADER_SIZE] = { 0 };
    SomeIp_RxIndication(data, SOMEIP_HEADER_SIZE);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    /* NULL data */
    SomeIp_Init(&testConfig);
    SomeIp_RxIndication(NULL_PTR, SOMEIP_HEADER_SIZE);
    /* Truncated length (< SOMEIP_HEADER_SIZE) */
    SomeIp_RxIndication(data, SOMEIP_HEADER_SIZE - 1U);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_SomeIp_00007 */
void test_SomeIp_RxIndication_ValidMessage_ShouldProcess(void) {
    SomeIp_Init(&testConfig);
    /* Build a valid SOME/IP request header via the serializer itself */
    SomeIp_HeaderType hdr;
    hdr.MessageId = SomeIp_CreateMessageId(0x1234U, 0x0001U);
    hdr.Length = 8U;
    hdr.RequestId = SomeIp_CreateRequestId(0x0001U, 0x0001U);
    hdr.ProtocolVersion = SOMEIP_PROTOCOL_VERSION;
    hdr.InterfaceVersion = SOMEIP_INTERFACE_VERSION;
    hdr.MessageType = SOMEIP_MSG_REQUEST;
    hdr.ReturnCode = SOMEIP_RET_OK;

    uint8 buf[SOMEIP_HEADER_SIZE];
    TEST_ASSERT_EQUAL(E_OK, SomeIp_SerializeHeader(&hdr, buf));
    SomeIp_RxIndication(buf, SOMEIP_HEADER_SIZE);
    /* Valid request is accepted (no DET, no crash) */
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

/** @req SWS_SomeIp_00009 */
void test_SomeIp_ProcessMessage_InvalidType_ShouldReturnNotOk(void) {
    SomeIp_MessageType msg;
    msg.Header.MessageId = SomeIp_CreateMessageId(0x1234U, 0x0001U);
    msg.Header.Length = 8U;
    msg.Header.RequestId = 0U;
    msg.Header.ProtocolVersion = SOMEIP_PROTOCOL_VERSION;
    msg.Header.InterfaceVersion = SOMEIP_INTERFACE_VERSION;
    msg.Header.MessageType = 0x55U;  /* not a valid SOME/IP message type */
    msg.Header.ReturnCode = SOMEIP_RET_OK;
    msg.Payload = NULL_PTR;
    msg.PayloadLength = 0U;

    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIp_ProcessMessage(&msg));
}

/** @req SWS_SomeIp_00009 */
void test_SomeIp_ProcessMessage_ValidTypes_ShouldReturnOk(void) {
    SomeIp_MessageType msg;
    msg.Header.MessageId = SomeIp_CreateMessageId(0x1234U, 0x0001U);
    msg.Header.Length = 8U;
    msg.Header.RequestId = 0U;
    msg.Header.ProtocolVersion = SOMEIP_PROTOCOL_VERSION;
    msg.Header.InterfaceVersion = SOMEIP_INTERFACE_VERSION;
    msg.Header.MessageType = SOMEIP_MSG_REQUEST;
    msg.Header.ReturnCode = SOMEIP_RET_OK;
    msg.Payload = NULL_PTR;
    msg.PayloadLength = 0U;
    TEST_ASSERT_EQUAL(E_OK, SomeIp_ProcessMessage(&msg));

    msg.Header.MessageType = SOMEIP_MSG_NOTIFICATION;
    TEST_ASSERT_EQUAL(E_OK, SomeIp_ProcessMessage(&msg));

    msg.Header.MessageType = SOMEIP_MSG_RESPONSE;
    TEST_ASSERT_EQUAL(E_OK, SomeIp_ProcessMessage(&msg));

    /* NULL message pointer is rejected */
    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIp_ProcessMessage(NULL_PTR));
}

/** @req SWS_SomeIp_00010 */
void test_SomeIp_ParseHeader_NullArgs_ShouldReturnNotOk(void) {
    uint8 data[SOMEIP_HEADER_SIZE] = { 0 };
    SomeIp_HeaderType hdr;
    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIp_ParseHeader(NULL_PTR, &hdr));
    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIp_ParseHeader(data, NULL_PTR));
}

/** @req SWS_SomeIp_00010 */
void test_SomeIp_ParseHeader_WrongProtocolVersion_ShouldReturnNotOk(void) {
    uint8 data[SOMEIP_HEADER_SIZE] = { 0 };
    data[12] = 0x02U;  /* protocol version != SOMEIP_PROTOCOL_VERSION */
    SomeIp_HeaderType hdr;
    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIp_ParseHeader(data, &hdr));
}

/** @req SWS_SomeIp_00010 */
void test_SomeIp_ParseHeader_ValidBuffer_ShouldReturnBigEndianFields(void) {
    SomeIp_HeaderType hdrIn;
    hdrIn.MessageId = SomeIp_CreateMessageId(0x1234U, 0x0005U);
    hdrIn.Length = 0x00000108U;
    hdrIn.RequestId = SomeIp_CreateRequestId(0x0A0BU, 0x0C0DU);
    hdrIn.ProtocolVersion = SOMEIP_PROTOCOL_VERSION;
    hdrIn.InterfaceVersion = SOMEIP_INTERFACE_VERSION;
    hdrIn.MessageType = SOMEIP_MSG_RESPONSE;
    hdrIn.ReturnCode = SOMEIP_RET_OK;

    uint8 buf[SOMEIP_HEADER_SIZE];
    TEST_ASSERT_EQUAL(E_OK, SomeIp_SerializeHeader(&hdrIn, buf));

    SomeIp_HeaderType hdrOut;
    TEST_ASSERT_EQUAL(E_OK, SomeIp_ParseHeader(buf, &hdrOut));
    TEST_ASSERT_EQUAL_UINT32(hdrIn.MessageId, hdrOut.MessageId);
    TEST_ASSERT_EQUAL_UINT32(hdrIn.Length, hdrOut.Length);
    TEST_ASSERT_EQUAL_UINT32(hdrIn.RequestId, hdrOut.RequestId);
    TEST_ASSERT_EQUAL_UINT8(hdrIn.ProtocolVersion, hdrOut.ProtocolVersion);
    TEST_ASSERT_EQUAL_UINT8(hdrIn.InterfaceVersion, hdrOut.InterfaceVersion);
    TEST_ASSERT_EQUAL_UINT8(hdrIn.MessageType, hdrOut.MessageType);
    TEST_ASSERT_EQUAL_UINT8(hdrIn.ReturnCode, hdrOut.ReturnCode);
}

/** @req SWS_SomeIp_00011 */
void test_SomeIp_SerializeHeader_NullArgs_ShouldReturnNotOk(void) {
    SomeIp_HeaderType hdr;
    uint8 buf[SOMEIP_HEADER_SIZE];
    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIp_SerializeHeader(NULL_PTR, buf));
    TEST_ASSERT_EQUAL(E_NOT_OK, SomeIp_SerializeHeader(&hdr, NULL_PTR));
}

/** @req SWS_SomeIp_00011 */
void test_SomeIp_SerializeHeader_Valid_ShouldWriteBigEndianBytes(void) {
    SomeIp_HeaderType hdr;
    hdr.MessageId = SomeIp_CreateMessageId(0x1234U, 0x0005U);
    hdr.Length = 8U;
    hdr.RequestId = SomeIp_CreateRequestId(0x0001U, 0x0002U);
    hdr.ProtocolVersion = SOMEIP_PROTOCOL_VERSION;
    hdr.InterfaceVersion = SOMEIP_INTERFACE_VERSION;
    hdr.MessageType = SOMEIP_MSG_REQUEST;
    hdr.ReturnCode = SOMEIP_RET_OK;

    uint8 buf[SOMEIP_HEADER_SIZE];
    TEST_ASSERT_EQUAL(E_OK, SomeIp_SerializeHeader(&hdr, buf));
    /* Big-endian encoding of the 4-byte MessageId 0x12340005 */
    TEST_ASSERT_EQUAL_UINT8(0x12U, buf[0]);
    TEST_ASSERT_EQUAL_UINT8(0x34U, buf[1]);
    TEST_ASSERT_EQUAL_UINT8(0x00U, buf[2]);
    TEST_ASSERT_EQUAL_UINT8(0x05U, buf[3]);
}

/** @req SWS_SomeIp_00012 */
void test_SomeIp_ExtractIds_ValidMessageId_ShouldSplitFields(void) {
    SomeIp_MessageIdType msgId = SomeIp_CreateMessageId(0x1234U, 0x0005U);
    SomeIp_ServiceIdType serviceId = 0U;
    SomeIp_MethodIdType methodId = 0U;
    SomeIp_ExtractIds(msgId, &serviceId, &methodId);
    TEST_ASSERT_EQUAL_UINT16(0x1234U, serviceId);
    TEST_ASSERT_EQUAL_UINT16(0x0005U, methodId);
}

/** @req SWS_SomeIp_00012 */
void test_SomeIp_CreateMessageIdAndRequestId_ShouldCombineFields(void) {
    TEST_ASSERT_EQUAL_UINT32(0x12340005U,
        SomeIp_CreateMessageId(0x1234U, 0x0005U));
    TEST_ASSERT_EQUAL_UINT32(0x00010002U,
        SomeIp_CreateRequestId(0x0001U, 0x0002U));
}

/** @req SWS_SomeIp_00008 */
void test_SomeIp_TxConfirmation_ShouldBeSafeNoOp(void) {
    /* Confirmation callback: must be callable in any state without crash */
    SomeIp_TxConfirmation(0x00010002U);
    SomeIp_Init(&testConfig);
    SomeIp_TxConfirmation(0x00010002U);
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}
