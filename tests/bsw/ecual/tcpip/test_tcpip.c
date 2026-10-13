/**
 * @file test_tcpip.c
 * @brief TcpIp Unit Tests — comprehensive coverage for branch coverage >= 90%
 * @version 3.0.0
 * @date 2026-10-20
 */

// @tests src/bsw/services/tcpip/src/TcpIp.c  @tests src/bsw/services/tcpip/include/TcpIp.h

#include "unity.h"
#include "TcpIp.h"
#include <string.h>

/* Mock Det_ReportError */
static uint8 mock_DetLastApiId = 0xFFU;
static uint8 mock_DetLastErrorId = 0xFFU;
static uint32 mock_DetCallCount = 0U;

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

/* Test config */
static const TcpIp_ConfigType testConfig = {
    TCPIP_MAX_SOCKETS,       /* NumSockets */
    TCPIP_MAX_TCP_PBUFS,     /* NumTcpPbufs */
    TCPIP_TCP_RCV_BUF_SIZE,  /* TcpRcvBufSize */
    TCPIP_TCP_SND_BUF_SIZE,  /* TcpSndBufSize */
    TCPIP_UDP_RCV_BUF_SIZE,  /* UdpRcvBufSize */
    TCPIP_ETH_LINK_CHECK_INTERVAL_MS /* EthLinkCheckIntervalMs */
};

static boolean tcpip_initialized = FALSE;

void setUp(void) {
    mock_Det_Reset();
    /* Force UNINIT state: DeInit reports DET when uninit, so reset after */
    TcpIp_DeInit();
    mock_Det_Reset();
    tcpip_initialized = FALSE;
}

void tearDown(void) {
}

/* Helper: init and create a TCP socket */
static TcpIp_SocketIdType helper_CreateTcpSocket(void) {
    TcpIp_SocketIdType sid = TCPIP_SOCKETID_INVALID;
    TcpIp_Init(&testConfig);
    tcpip_initialized = TRUE;
    TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, &sid);
    return sid;
}

/* Helper: init and create a UDP socket */
static TcpIp_SocketIdType helper_CreateUdpSocket(void) {
    TcpIp_SocketIdType sid = TCPIP_SOCKETID_INVALID;
    TcpIp_Init(&testConfig);
    tcpip_initialized = TRUE;
    TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_DGRAM, &sid);
    return sid;
}

/* =========================================================================
 * Init / DeInit / GetVersionInfo
 * ========================================================================= */

void test_TcpIp_Init_NullPtr_ShouldReportError(void) {
    TcpIp_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(TcpIP_SID_INIT, mock_DetLastApiId);
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_PARAM_POINTER, mock_DetLastErrorId);
}

void test_TcpIp_Init_ValidConfig_ShouldSucceed(void) {
    TcpIp_Init(&testConfig);
    tcpip_initialized = TRUE;
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

void test_TcpIp_Init_DoubleInit_ShouldReportError(void) {
    TcpIp_Init(&testConfig);
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_ALREADY_INITIALIZED, mock_DetLastErrorId);
}

void test_TcpIp_DeInit_Uninit_ShouldReportError(void) {
    TcpIp_DeInit();
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_UNINIT, mock_DetLastErrorId);
}

void test_TcpIp_DeInit_Valid_ShouldResetState(void) {
    TcpIp_Init(&testConfig);
    tcpip_initialized = TRUE;
    TcpIp_DeInit();
    tcpip_initialized = FALSE;
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
    /* After deinit, calls should fail with uninit */
    TcpIp_LinkStateType ls;
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetLinkState(&ls));
}

void test_TcpIp_GetVersionInfo_NullPtr_ShouldReportError(void) {
    TcpIp_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT32(1U, mock_DetCallCount);
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_PARAM_POINTER, mock_DetLastErrorId);
}

void test_TcpIp_GetVersionInfo_ValidPtr_ShouldReturnIds(void) {
    Std_VersionInfoType vi;
    memset(&vi, 0, sizeof(vi));
    TcpIp_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL_UINT16(TCPIP_VENDOR_ID, vi.vendorID);
    TEST_ASSERT_EQUAL_UINT16(TCPIP_MODULE_ID, vi.moduleID);
    TEST_ASSERT_EQUAL_UINT8(TCPIP_SW_MAJOR_VERSION, vi.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(TCPIP_SW_MINOR_VERSION, vi.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(TCPIP_SW_PATCH_VERSION, vi.sw_patch_version);
}

/* =========================================================================
 * Create
 * ========================================================================= */

void test_TcpIp_Create_Uninit_ShouldReportError(void) {
    TcpIp_SocketIdType sid;
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, &sid));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_UNINIT, mock_DetLastErrorId);
}

void test_TcpIp_Create_NullPtr_ShouldReportError(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_PARAM_POINTER, mock_DetLastErrorId);
}

void test_TcpIp_Create_TcpSocket_ShouldSucceed(void) {
    TcpIp_SocketIdType sid = TCPIP_SOCKETID_INVALID;
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, &sid));
    TEST_ASSERT_NOT_EQUAL(TCPIP_SOCKETID_INVALID, sid);
}

void test_TcpIp_Create_UdpSocket_ShouldSucceed(void) {
    TcpIp_SocketIdType sid = TCPIP_SOCKETID_INVALID;
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_DGRAM, &sid));
    TEST_ASSERT_NOT_EQUAL(TCPIP_SOCKETID_INVALID, sid);
}

void test_TcpIp_Create_SocketExhaustion_ShouldFail(void) {
    TcpIp_SocketIdType sid;
    uint8 i;
    TcpIp_Init(&testConfig);
    for (i = 0U; i < TCPIP_MAX_SOCKETS; i++) {
        TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, &sid));
    }
    /* All slots used */
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, &sid));
}

/* =========================================================================
 * Bind
 * ========================================================================= */

void test_TcpIp_Bind_Uninit_ShouldReportError(void) {
    TcpIp_SockAddrType addr;
    memset(&addr, 0, sizeof(addr));
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_Bind(0U, &addr));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_UNINIT, mock_DetLastErrorId);
}

void test_TcpIp_Bind_NullPtr_ShouldReportError(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_Bind(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_PARAM_POINTER, mock_DetLastErrorId);
}

void test_TcpIp_Bind_InvalidSocket_ShouldFail(void) {
    TcpIp_SockAddrType addr;
    memset(&addr, 0, sizeof(addr));
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_Bind(0xFFU, &addr));
}

void test_TcpIp_Bind_ValidCall_ShouldSucceed(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TcpIp_SockAddrType addr;
    memset(&addr, 0, sizeof(addr));
    addr.domain = TCPIP_AF_INET;
    addr.port = 8080U;
    addr.addr[0] = 192U; addr.addr[1] = 168U; addr.addr[2] = 1U; addr.addr[3] = 100U;
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_Bind(sid, &addr));
}

/* =========================================================================
 * Listen
 * ========================================================================= */

void test_TcpIp_Listen_Uninit_ShouldReportError(void) {
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_Listen(0U, 1U));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_UNINIT, mock_DetLastErrorId);
}

void test_TcpIp_Listen_InvalidSocket_ShouldFail(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_Listen(0xFFU, 1U));
}

void test_TcpIp_Listen_UdpSocket_ShouldReturnInvalidProtocol(void) {
    TcpIp_SocketIdType sid = helper_CreateUdpSocket();
    TEST_ASSERT_EQUAL(TCPIP_E_INVALID_PROTOCOL, TcpIp_Listen(sid, 1U));
}

void test_TcpIp_Listen_ValidCall_ShouldSucceed(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_Listen(sid, 2U));
    TcpIp_TcpStateType st;
    TcpIp_GetTcpState(sid, &st);
    TEST_ASSERT_EQUAL(TCPIP_TCPSTATE_LISTEN, st);
}

void test_TcpIp_Listen_ReListen_ShouldUpdateBacklog(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_Listen(sid, 1U));
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_Listen(sid, 3U));
}

void test_TcpIp_Listen_InvalidState_ShouldFail(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    /* Connect to move to ESTABLISHED */
    TcpIp_SockAddrType raddr;
    memset(&raddr, 0, sizeof(raddr));
    raddr.domain = TCPIP_AF_INET;
    raddr.port = 80U;
    TcpIp_Connect(sid, &raddr); /* native sim -> ESTABLISHED */
    TEST_ASSERT_EQUAL(TCPIP_E_INVALID_STATE, TcpIp_Listen(sid, 1U));
}

void test_TcpIp_Listen_BacklogClamped_ShouldSucceed(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    /* Backlog > TCPIP_MAX_PENDING_CONNECTIONS should be clamped */
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_Listen(sid, 255U));
}

/* =========================================================================
 * Connect
 * ========================================================================= */

void test_TcpIp_Connect_Uninit_ShouldReportError(void) {
    TcpIp_SockAddrType addr;
    memset(&addr, 0, sizeof(addr));
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_Connect(0U, &addr));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_UNINIT, mock_DetLastErrorId);
}

void test_TcpIp_Connect_NullPtr_ShouldReportError(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_Connect(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_PARAM_POINTER, mock_DetLastErrorId);
}

void test_TcpIp_Connect_InvalidSocket_ShouldFail(void) {
    TcpIp_SockAddrType addr;
    memset(&addr, 0, sizeof(addr));
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_Connect(0xFFU, &addr));
}

void test_TcpIp_Connect_TcpNativeSim_ShouldReachEstablished(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TcpIp_SockAddrType raddr;
    memset(&raddr, 0, sizeof(raddr));
    raddr.domain = TCPIP_AF_INET;
    raddr.port = 80U;
    raddr.addr[0] = 10U; raddr.addr[1] = 0U; raddr.addr[2] = 0U; raddr.addr[3] = 1U;
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_Connect(sid, &raddr));
    TcpIp_TcpStateType st;
    TcpIp_GetTcpState(sid, &st);
    TEST_ASSERT_EQUAL(TCPIP_TCPSTATE_ESTABLISHED, st);
}

void test_TcpIp_Connect_AlreadyConnected_ShouldReturnIsConn(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TcpIp_SockAddrType raddr;
    memset(&raddr, 0, sizeof(raddr));
    raddr.port = 80U;
    TcpIp_Connect(sid, &raddr); /* -> ESTABLISHED */
    TEST_ASSERT_EQUAL(TCPIP_E_ISCONN, TcpIp_Connect(sid, &raddr));
}

void test_TcpIp_Connect_UdpSocket_ShouldSucceed(void) {
    TcpIp_SocketIdType sid = helper_CreateUdpSocket();
    TcpIp_SockAddrType raddr;
    memset(&raddr, 0, sizeof(raddr));
    raddr.port = 5683U;
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_Connect(sid, &raddr));
}

/* =========================================================================
 * Accept
 * ========================================================================= */

void test_TcpIp_Accept_Uninit_ShouldReportError(void) {
    TcpIp_SocketIdType nsid;
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_Accept(0U, &nsid));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_UNINIT, mock_DetLastErrorId);
}

void test_TcpIp_Accept_NullPtr_ShouldReportError(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_Accept(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_PARAM_POINTER, mock_DetLastErrorId);
}

void test_TcpIp_Accept_NotListening_ShouldFail(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TcpIp_SocketIdType nsid;
    TEST_ASSERT_EQUAL(TCPIP_E_INVALID_STATE, TcpIp_Accept(sid, &nsid));
}

void test_TcpIp_Accept_NoPending_ShouldFail(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TcpIp_Listen(sid, 2U);
    TcpIp_SocketIdType nsid;
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_Accept(sid, &nsid));
}

void test_TcpIp_Accept_WithPending_ShouldSucceed(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TcpIp_Listen(sid, 2U);
    /* Simulate incoming SYN: LISTEN -> SYN_RECEIVED allocates child */
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_SYN_RECEIVED));
    TcpIp_SocketIdType nsid = TCPIP_SOCKETID_INVALID;
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_Accept(sid, &nsid));
    TEST_ASSERT_NOT_EQUAL(TCPIP_SOCKETID_INVALID, nsid);
}

/* =========================================================================
 * Abort
 * ========================================================================= */

void test_TcpIp_Abort_Uninit_ShouldReportError(void) {
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_Abort(0U));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_UNINIT, mock_DetLastErrorId);
}

void test_TcpIp_Abort_InvalidSocket_ShouldFail(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_Abort(0xFFU));
}

void test_TcpIp_Abort_ValidSocket_ShouldSucceed(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_Abort(sid));
    /* Socket should be freed now */
    TcpIp_TcpStateType st;
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetTcpState(sid, &st));
}

/* =========================================================================
 * Close
 * ========================================================================= */

void test_TcpIp_Close_Uninit_ShouldReportError(void) {
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_Close(0U, TRUE));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_UNINIT, mock_DetLastErrorId);
}

void test_TcpIp_Close_InvalidSocket_ShouldFail(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_Close(0xFFU, TRUE));
}

void test_TcpIp_Close_ForceClose_ShouldFreeSlot(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_Close(sid, TRUE));
    TcpIp_TcpStateType st;
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetTcpState(sid, &st));
}

void test_TcpIp_Close_GracefulClose_Established_ShouldStartClosing(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TcpIp_SockAddrType raddr;
    memset(&raddr, 0, sizeof(raddr));
    raddr.port = 80U;
    TcpIp_Connect(sid, &raddr); /* -> ESTABLISHED */
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_Close(sid, FALSE));
    TcpIp_TcpStateType st;
    TcpIp_GetTcpState(sid, &st);
    TEST_ASSERT_EQUAL(TCPIP_TCPSTATE_FIN_WAIT_1, st);
}

void test_TcpIp_Close_GracefulClose_AlreadyClosing_ShouldReturnOk(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TcpIp_SockAddrType raddr;
    memset(&raddr, 0, sizeof(raddr));
    raddr.port = 80U;
    TcpIp_Connect(sid, &raddr);
    TcpIp_Close(sid, FALSE); /* -> FIN_WAIT_1 */
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_Close(sid, FALSE)); /* already closing */
}

void test_TcpIp_Close_GracefulClose_NotConnected_ShouldFreeSlot(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    /* CLOSED state, not connected -> free immediately */
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_Close(sid, FALSE));
    TcpIp_TcpStateType st;
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetTcpState(sid, &st));
}

/* =========================================================================
 * Send / Transmit
 * ========================================================================= */

void test_TcpIp_Send_Uninit_ShouldReportError(void) {
    uint8 data[4] = {0};
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_Send(0U, data, 4U));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_UNINIT, mock_DetLastErrorId);
}

void test_TcpIp_Send_NullPtr_ShouldReportError(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_Send(0U, NULL_PTR, 4U));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_PARAM_POINTER, mock_DetLastErrorId);
}

void test_TcpIp_Send_InvalidSocket_ShouldFail(void) {
    uint8 data[4] = {0};
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_Send(0xFFU, data, 4U));
}

void test_TcpIp_Send_ValidCall_ShouldSucceed(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    uint8 data[4] = {0x01, 0x02, 0x03, 0x04};
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_Send(sid, data, 4U));
}

void test_TcpIp_Transmit_ShouldAliasSend(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    uint8 data[2] = {0xAA, 0xBB};
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_Transmit(sid, data, 2U));
}

/* =========================================================================
 * Receive
 * ========================================================================= */

void test_TcpIp_Receive_Uninit_ShouldReportError(void) {
    uint8 buf[16]; uint16 len;
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_Receive(0U, buf, 16U, &len));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_UNINIT, mock_DetLastErrorId);
}

void test_TcpIp_Receive_NullBufPtr_ShouldReportError(void) {
    uint16 len;
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_Receive(0U, NULL_PTR, 16U, &len));
}

void test_TcpIp_Receive_NullLenPtr_ShouldReportError(void) {
    uint8 buf[16];
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_Receive(0U, buf, 16U, NULL_PTR));
}

void test_TcpIp_Receive_NoData_ShouldReturnOkWithZeroLen(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    uint8 buf[16]; uint16 len = 0xFFFFU;
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_Receive(sid, buf, 16U, &len));
    TEST_ASSERT_EQUAL_UINT16(0U, len);
}

void test_TcpIp_Receive_WithData_ShouldCopyAndDrain(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    uint8 txData[4] = {0x01, 0x02, 0x03, 0x04};
    uint8 rxData[16] = {0}; uint16 rxLen = 0U;
    /* Inject data via RxIndication (pool path) */
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_RxIndication(sid, txData, 4U));
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_Receive(sid, rxData, 16U, &rxLen));
    TEST_ASSERT_EQUAL_UINT16(4U, rxLen);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(txData, rxData, 4U);
}

void test_TcpIp_Receive_BufferTooSmall_ShouldOverflow(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    uint8 txData[16] = {0};
    uint8 rxData[4] = {0}; uint16 rxLen;
    TcpIp_RxIndication(sid, txData, 16U);
    TEST_ASSERT_EQUAL(TCPIP_E_BUFFER_OVERFLOW, TcpIp_Receive(sid, rxData, 4U, &rxLen));
}

/* =========================================================================
 * ChangeTcpState
 * ========================================================================= */

void test_TcpIp_ChangeTcpState_Uninit_ShouldReportError(void) {
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_ChangeTcpState(0U, TCPIP_TCPSTATE_LISTEN));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_UNINIT, mock_DetLastErrorId);
}

void test_TcpIp_ChangeTcpState_InvalidSocket_ShouldFail(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_ChangeTcpState(0xFFU, TCPIP_TCPSTATE_LISTEN));
}

void test_TcpIp_ChangeTcpState_ClosedToListen_ShouldSucceed(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_LISTEN));
    TcpIp_TcpStateType st;
    TcpIp_GetTcpState(sid, &st);
    TEST_ASSERT_EQUAL(TCPIP_TCPSTATE_LISTEN, st);
}

void test_TcpIp_ChangeTcpState_InvalidTransition_ShouldFail(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    /* CLOSED -> ESTABLISHED is invalid */
    TEST_ASSERT_EQUAL(TCPIP_E_INVALID_STATE, TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_ESTABLISHED));
}

void test_TcpIp_ChangeTcpState_Idempotent_ShouldSucceed(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    /* Same state is always valid */
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_CLOSED));
}

void test_TcpIp_ChangeTcpState_ListenerSynReceived_ShouldAllocChild(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TcpIp_Listen(sid, 2U);
    /* LISTEN -> SYN_RECEIVED: allocates child */
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_SYN_RECEIVED));
}

void test_TcpIp_ChangeTcpState_ListenerBacklogFull_ShouldReturnNoBufs(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TcpIp_Listen(sid, 1U);
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_SYN_RECEIVED));
    /* Second SYN: backlog=1 is full */
    TEST_ASSERT_EQUAL(TCPIP_E_NOBUFS, TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_SYN_RECEIVED));
}

void test_TcpIp_ChangeTcpState_ToEstablished_ShouldIncStats(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_LISTEN);
    TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_SYN_RECEIVED);
    /* Accept the child */
    TcpIp_SocketIdType nsid;
    TcpIp_Accept(sid, &nsid);
    /* Move child to ESTABLISHED */
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_ChangeTcpState(nsid, TCPIP_TCPSTATE_ESTABLISHED));
}

void test_TcpIp_ChangeTcpState_FullTcpLifecycle(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    /* CLOSED -> LISTEN -> SYN_RECEIVED -> ESTABLISHED -> FIN_WAIT_1 -> FIN_WAIT_2 -> TIME_WAIT -> CLOSED */
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_LISTEN));
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_SYN_RECEIVED));
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_ESTABLISHED));
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_FIN_WAIT_1));
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_FIN_WAIT_2));
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_TIME_WAIT));
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_CLOSED));
}

void test_TcpIp_ChangeTcpState_CloseWaitLastAck(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_LISTEN);
    TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_SYN_RECEIVED);
    TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_ESTABLISHED);
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_CLOSE_WAIT));
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_LAST_ACK));
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_CLOSED));
}

void test_TcpIp_ChangeTcpState_Closing(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_LISTEN);
    TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_SYN_RECEIVED);
    TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_ESTABLISHED);
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_FIN_WAIT_1));
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_CLOSING));
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_ChangeTcpState(sid, TCPIP_TCPSTATE_TIME_WAIT));
}

/* =========================================================================
 * RxIndication / TxConfirmation
 * ========================================================================= */

void test_TcpIp_RxIndication_Uninit_ShouldReportError(void) {
    uint8 data[4] = {0};
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_RxIndication(0U, data, 4U));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_UNINIT, mock_DetLastErrorId);
}

void test_TcpIp_RxIndication_NullPtr_ShouldReportError(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_RxIndication(0U, NULL_PTR, 4U));
}

void test_TcpIp_RxIndication_InvalidSocket_ShouldFail(void) {
    uint8 data[4] = {0};
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_RxIndication(0xFFU, data, 4U));
}

void test_TcpIp_RxIndication_ZeroLength_ShouldReturnOk(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    uint8 data[1] = {0};
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_RxIndication(sid, data, 0U));
}

void test_TcpIp_RxIndication_UserBuffer_ShouldCopyToUserBuf(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    uint8 userBuf[64];
    uint8 data[8] = {1,2,3,4,5,6,7,8};
    TcpIp_SetRxBuffer(sid, userBuf, 64U);
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_RxIndication(sid, data, 8U));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(data, userBuf, 8U);
}

void test_TcpIp_RxIndication_PoolBuffer_ShouldQueueData(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    uint8 data[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0x11, 0x22, 0x33, 0x44};
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_RxIndication(sid, data, 8U));
    /* Receive it back */
    uint8 rxData[16]; uint16 rxLen;
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_Receive(sid, rxData, 16U, &rxLen));
    TEST_ASSERT_EQUAL_UINT16(8U, rxLen);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(data, rxData, 8U);
}

void test_TcpIp_RxIndication_Overflow_ShouldReturnBufferOverflow(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    uint8 data[8] = {0};
    /* Fill pool: TCPIP_MAX_RX_BUFFERS = 2 */
    TcpIp_RxIndication(sid, data, 8U);
    TcpIp_RxIndication(sid, data, 8U);
    TEST_ASSERT_EQUAL(TCPIP_E_BUFFER_OVERFLOW, TcpIp_RxIndication(sid, data, 8U));
}

void test_TcpIp_RxIndication_TooLarge_ShouldReturnBufferOverflow(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    uint8 data[1]; 
    /* Length > TCPIP_PBUF_POOL_BUF_SIZE */
    TEST_ASSERT_EQUAL(TCPIP_E_BUFFER_OVERFLOW, TcpIp_RxIndication(sid, data, TCPIP_PBUF_POOL_BUF_SIZE + 1U));
}

void test_TcpIp_TxConfirmation_Uninit_ShouldReportError(void) {
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_TxConfirmation(0U, TRUE));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_UNINIT, mock_DetLastErrorId);
}

void test_TcpIp_TxConfirmation_InvalidSocket_ShouldFail(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_TxConfirmation(0xFFU, TRUE));
}

void test_TcpIp_TxConfirmation_ValidCall_ShouldSucceed(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_TxConfirmation(sid, TRUE));
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_TxConfirmation(sid, FALSE));
}

/* =========================================================================
 * GetRxBuffer / ReleaseRxBuffer / GetTxBuffer / ReleaseTxBuffer
 * ========================================================================= */

void test_TcpIp_GetRxBuffer_Uninit_ShouldReportError(void) {
    uint8* dp; uint16 len;
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetRxBuffer(0U, &dp, &len));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_UNINIT, mock_DetLastErrorId);
}

void test_TcpIp_GetRxBuffer_NullPtr_ShouldReportError(void) {
    uint16 len;
    uint8* dp;
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetRxBuffer(0U, NULL_PTR, &len));
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetRxBuffer(0U, &dp, NULL_PTR));
}

void test_TcpIp_GetRxBuffer_NoData_ShouldFail(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    uint8* dp; uint16 len;
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetRxBuffer(sid, &dp, &len));
}

void test_TcpIp_GetRxBuffer_WithData_ShouldReturnData(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    uint8 data[4] = {1,2,3,4};
    TcpIp_RxIndication(sid, data, 4U);
    uint8* dp = NULL_PTR; uint16 len = 0U;
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_GetRxBuffer(sid, &dp, &len));
    TEST_ASSERT_NOT_NULL(dp);
    TEST_ASSERT_EQUAL_UINT16(4U, len);
}

void test_TcpIp_GetRxBuffer_ChunkInUse_ShouldReturnNoBufs(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    uint8 data[4] = {1,2,3,4};
    TcpIp_RxIndication(sid, data, 4U);
    uint8* dp; uint16 len;
    TcpIp_GetRxBuffer(sid, &dp, &len); /* first call: OK */
    TEST_ASSERT_EQUAL(TCPIP_E_NOBUFS, TcpIp_GetRxBuffer(sid, &dp, &len)); /* second: in use */
}

void test_TcpIp_ReleaseRxBuffer_Uninit_ShouldReportError(void) {
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_ReleaseRxBuffer(0U));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_UNINIT, mock_DetLastErrorId);
}

void test_TcpIp_ReleaseRxBuffer_NoData_ShouldFail(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_ReleaseRxBuffer(sid));
}

void test_TcpIp_ReleaseRxBuffer_AfterGet_ShouldReleaseChunk(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    uint8 data[4] = {1,2,3,4};
    TcpIp_RxIndication(sid, data, 4U);
    uint8* dp; uint16 len;
    TcpIp_GetRxBuffer(sid, &dp, &len);
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_ReleaseRxBuffer(sid));
    /* After release, can get again if more data available */
}

void test_TcpIp_GetTxBuffer_Uninit_ShouldReportError(void) {
    uint8* dp; uint16 len;
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetTxBuffer(0U, &dp, &len));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_UNINIT, mock_DetLastErrorId);
}

void test_TcpIp_GetTxBuffer_NullPtr_ShouldReportError(void) {
    uint16 len; uint8* dp;
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetTxBuffer(0U, NULL_PTR, &len));
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetTxBuffer(0U, &dp, NULL_PTR));
}

void test_TcpIp_GetTxBuffer_ValidCall_ShouldReturnBuffer(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    uint8* dp = NULL_PTR; uint16 len = 0U;
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_GetTxBuffer(sid, &dp, &len));
    TEST_ASSERT_NOT_NULL(dp);
    TEST_ASSERT_EQUAL_UINT16(TCPIP_PBUF_POOL_BUF_SIZE, len);
}

void test_TcpIp_ReleaseTxBuffer_Uninit_ShouldReportError(void) {
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_ReleaseTxBuffer(0U, 4U));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_UNINIT, mock_DetLastErrorId);
}

void test_TcpIp_ReleaseTxBuffer_InvalidSocket_ShouldFail(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_ReleaseTxBuffer(0xFFU, 4U));
}

void test_TcpIp_ReleaseTxBuffer_Overflow_ShouldFail(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TEST_ASSERT_EQUAL(TCPIP_E_BUFFER_OVERFLOW, TcpIp_ReleaseTxBuffer(sid, TCPIP_PBUF_POOL_BUF_SIZE + 1U));
}

void test_TcpIp_ReleaseTxBuffer_ValidCall_ShouldSucceed(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    uint8* dp; uint16 len;
    TcpIp_GetTxBuffer(sid, &dp, &len);
    dp[0] = 0xAA; dp[1] = 0xBB;
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_ReleaseTxBuffer(sid, 2U));
}

/* =========================================================================
 * SetRxBuffer
 * ========================================================================= */

void test_TcpIp_SetRxBuffer_Uninit_ShouldReportError(void) {
    uint8 buf[16];
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_SetRxBuffer(0U, buf, 16U));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_UNINIT, mock_DetLastErrorId);
}

void test_TcpIp_SetRxBuffer_NullPtr_ShouldReportError(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_SetRxBuffer(0U, NULL_PTR, 16U));
}

void test_TcpIp_SetRxBuffer_ValidCall_ShouldSucceed(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    uint8 buf[64];
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_SetRxBuffer(sid, buf, 64U));
}

/* =========================================================================
 * TCP Options
 * ========================================================================= */

void test_TcpIp_SetTcpOption_AllOptions_ShouldSucceed(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_SetTcpOption(sid, TCPIP_TCPOPT_REUSEADDR, 1U));
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_SetTcpOption(sid, TCPIP_TCPOPT_KEEPALIVE, 1U));
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_SetTcpOption(sid, TCPIP_TCPOPT_NODELAY, 1U));
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_SetTcpOption(sid, TCPIP_TCPOPT_MAXSEG, 1460U));
}

void test_TcpIp_SetTcpOption_InvalidOption_ShouldFail(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_SetTcpOption(sid, 0xFFU, 1U));
}

void test_TcpIp_SetTcpOption_InvalidSocket_ShouldFail(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_SetTcpOption(0xFFU, TCPIP_TCPOPT_REUSEADDR, 1U));
}

void test_TcpIp_SetTcpOption_Uninit_ShouldReportError(void) {
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_SetTcpOption(0U, TCPIP_TCPOPT_REUSEADDR, 1U));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_UNINIT, mock_DetLastErrorId);
}

void test_TcpIp_GetTcpOption_NullPtr_ShouldReportError(void) {
    uint32 val;
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetTcpOption(0U, TCPIP_TCPOPT_REUSEADDR, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_PARAM_POINTER, mock_DetLastErrorId);
    (void)val;
}

void test_TcpIp_GetTcpOption_AllOptions_ShouldReturnValues(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    uint32 val = 0U;
    TcpIp_SetTcpOption(sid, TCPIP_TCPOPT_REUSEADDR, 1U);
    TcpIp_SetTcpOption(sid, TCPIP_TCPOPT_KEEPALIVE, 1U);
    TcpIp_SetTcpOption(sid, TCPIP_TCPOPT_NODELAY, 1U);
    TcpIp_SetTcpOption(sid, TCPIP_TCPOPT_MAXSEG, 1460U);
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_GetTcpOption(sid, TCPIP_TCPOPT_REUSEADDR, &val));
    TEST_ASSERT_EQUAL_UINT32(1U, val);
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_GetTcpOption(sid, TCPIP_TCPOPT_KEEPALIVE, &val));
    TEST_ASSERT_EQUAL_UINT32(1U, val);
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_GetTcpOption(sid, TCPIP_TCPOPT_NODELAY, &val));
    TEST_ASSERT_EQUAL_UINT32(1U, val);
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_GetTcpOption(sid, TCPIP_TCPOPT_MAXSEG, &val));
    TEST_ASSERT_EQUAL_UINT32(1460U, val);
}

void test_TcpIp_GetTcpOption_InvalidOption_ShouldFail(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    uint32 val;
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetTcpOption(sid, 0xFFU, &val));
}

/* =========================================================================
 * UDP Options
 * ========================================================================= */

void test_TcpIp_SetUdpOption_AllOptions_ShouldSucceed(void) {
    TcpIp_SocketIdType sid = helper_CreateUdpSocket();
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_SetUdpOption(sid, TCPIP_UDPOPT_REUSEADDR, 1U));
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_SetUdpOption(sid, TCPIP_UDPOPT_TTL, 64U));
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_SetUdpOption(sid, TCPIP_UDPOPT_TOS, 0x10U));
}

void test_TcpIp_SetUdpOption_InvalidOption_ShouldFail(void) {
    TcpIp_SocketIdType sid = helper_CreateUdpSocket();
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_SetUdpOption(sid, 0xFFU, 1U));
}

void test_TcpIp_SetUdpOption_Uninit_ShouldReportError(void) {
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_SetUdpOption(0U, TCPIP_UDPOPT_TTL, 64U));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_UNINIT, mock_DetLastErrorId);
}

void test_TcpIp_SetUdpOption_InvalidSocket_ShouldFail(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_SetUdpOption(0xFFU, TCPIP_UDPOPT_TTL, 64U));
}

void test_TcpIp_GetUdpOption_NullPtr_ShouldReportError(void) {
    uint32 val;
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetUdpOption(0U, TCPIP_UDPOPT_TTL, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_PARAM_POINTER, mock_DetLastErrorId);
    (void)val;
}

void test_TcpIp_GetUdpOption_AllOptions_ShouldReturnValues(void) {
    TcpIp_SocketIdType sid = helper_CreateUdpSocket();
    uint32 val = 0U;
    TcpIp_SetUdpOption(sid, TCPIP_UDPOPT_TTL, 64U);
    TcpIp_SetUdpOption(sid, TCPIP_UDPOPT_TOS, 0x10U);
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_GetUdpOption(sid, TCPIP_UDPOPT_REUSEADDR, &val));
    TEST_ASSERT_EQUAL_UINT32(0U, val);
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_GetUdpOption(sid, TCPIP_UDPOPT_TTL, &val));
    TEST_ASSERT_EQUAL_UINT32(64U, val);
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_GetUdpOption(sid, TCPIP_UDPOPT_TOS, &val));
    TEST_ASSERT_EQUAL_UINT32(0x10U, val);
}

void test_TcpIp_GetUdpOption_InvalidOption_ShouldFail(void) {
    TcpIp_SocketIdType sid = helper_CreateUdpSocket();
    uint32 val;
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetUdpOption(sid, 0xFFU, &val));
}

/* =========================================================================
 * Address functions
 * ========================================================================= */

void test_TcpIp_GetIPv4Addr_Uninit_ShouldReportError(void) {
    TcpIp_Ipv4AddrType addr;
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetIPv4Addr(&addr));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_UNINIT, mock_DetLastErrorId);
}

void test_TcpIp_GetIPv4Addr_NullPtr_ShouldReportError(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetIPv4Addr(NULL_PTR));
}

void test_TcpIp_GetIPv4Addr_ValidCall_ShouldReturnDefault(void) {
    TcpIp_Ipv4AddrType addr = 0U;
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_GetIPv4Addr(&addr));
    TEST_ASSERT_EQUAL_UINT32(TCPIP_DEFAULT_IPV4_ADDR, addr);
}

void test_TcpIp_GetIPv6Addr_ValidCall_ShouldReturnNotSupported(void) {
    TcpIp_Ipv6AddrType addr;
    TcpIp_Init(&testConfig);
    /* IPV6 is OFF in config */
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_SUPPORTED, TcpIp_GetIPv6Addr(&addr));
}

void test_TcpIp_GetIPv6Addr_NullPtr_ShouldReportError(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetIPv6Addr(NULL_PTR));
}

void test_TcpIp_GetIPv4SubnetMask_ValidCall_ShouldSucceed(void) {
    TcpIp_Ipv4AddrType mask = 0U;
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_GetIPv4SubnetMask(0U, &mask));
    TEST_ASSERT_EQUAL_UINT32(TCPIP_DEFAULT_IPV4_MASK, mask);
}

void test_TcpIp_GetIPv4SubnetMask_InvalidIfIdx_ShouldFail(void) {
    TcpIp_Ipv4AddrType mask;
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetIPv4SubnetMask(1U, &mask));
}

void test_TcpIp_GetIPv4SubnetMask_NullPtr_ShouldReportError(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetIPv4SubnetMask(0U, NULL_PTR));
}

void test_TcpIp_GetLinkState_NullPtr_ShouldReportError(void) {
    TcpIp_LinkStateType ls;
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetLinkState(NULL_PTR));
    (void)ls;
}

void test_TcpIp_GetLinkState_ValidCall_ShouldReturnUp(void) {
    TcpIp_LinkStateType ls = TCPIP_LINK_STATE_DOWN;
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_GetLinkState(&ls));
    TEST_ASSERT_EQUAL(TCPIP_LINK_STATE_UP, ls);
}

void test_TcpIp_GetInterfaceState_ValidCall_ShouldSucceed(void) {
    TcpIp_InterfaceStateType ifs;
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_GetInterfaceState(&ifs));
    TEST_ASSERT_EQUAL(TCPIP_IFSTATE_UP, ifs);
}

void test_TcpIp_GetInterfaceState_NullPtr_ShouldReportError(void) {
    TcpIp_InterfaceStateType ifs;
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetInterfaceState(NULL_PTR));
    (void)ifs;
}

void test_TcpIp_GetIpAddrState_ValidCall_ShouldSucceed(void) {
    TcpIp_IpAddrStateType as;
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_GetIpAddrState(0U, &as));
    TEST_ASSERT_EQUAL(TCPIP_IPADDR_STATE_ASSIGNED, as);
}

void test_TcpIp_GetIpAddrState_InvalidIfIdx_ShouldFail(void) {
    TcpIp_IpAddrStateType as;
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetIpAddrState(1U, &as));
}

void test_TcpIp_GetIpAddrState_NullPtr_ShouldReportError(void) {
    TcpIp_IpAddrStateType as;
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetIpAddrState(0U, NULL_PTR));
    (void)as;
}

/* =========================================================================
 * Set/Get Remote/Local Addr
 * ========================================================================= */

void test_TcpIp_SetRemoteAddr_ValidCall_ShouldSucceed(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TcpIp_SockAddrType raddr;
    memset(&raddr, 0, sizeof(raddr));
    raddr.port = 443U;
    raddr.addr[0] = 10U; raddr.addr[1] = 0U; raddr.addr[2] = 0U; raddr.addr[3] = 1U;
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_SetRemoteAddr(sid, &raddr));
}

void test_TcpIp_SetRemoteAddr_NullPtr_ShouldReportError(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_SetRemoteAddr(0U, NULL_PTR));
}

void test_TcpIp_SetLocalAddr_ValidCall_ShouldSucceed(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TcpIp_SockAddrType laddr;
    memset(&laddr, 0, sizeof(laddr));
    laddr.port = 8080U;
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_SetLocalAddr(sid, &laddr));
}

void test_TcpIp_SetLocalAddr_NullPtr_ShouldReportError(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_SetLocalAddr(0U, NULL_PTR));
}

void test_TcpIp_GetLocalAddr_ValidCall_ShouldSucceed(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TcpIp_SockAddrType laddr;
    memset(&laddr, 0, sizeof(laddr));
    laddr.port = 9090U;
    TcpIp_Bind(sid, &laddr);
    TcpIp_SockAddrType result;
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_GetLocalAddr(sid, &result));
    TEST_ASSERT_EQUAL_UINT16(9090U, result.port);
}

void test_TcpIp_GetLocalAddr_NullPtr_ShouldReportError(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetLocalAddr(0U, NULL_PTR));
}

void test_TcpIp_GetRemoteAddr_ValidCall_ShouldSucceed(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TcpIp_SockAddrType raddr;
    memset(&raddr, 0, sizeof(raddr));
    raddr.port = 443U;
    raddr.addr[0] = 10U;
    TcpIp_Connect(sid, &raddr);
    TcpIp_SockAddrType result;
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_GetRemoteAddr(sid, &result));
    TEST_ASSERT_EQUAL_UINT16(443U, result.port);
}

void test_TcpIp_GetRemoteAddr_NullPtr_ShouldReportError(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetRemoteAddr(0U, NULL_PTR));
}

/* =========================================================================
 * GetConnectionState / GetTcpState
 * ========================================================================= */

void test_TcpIp_GetConnectionState_ValidCall_ShouldSucceed(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TcpIp_ConnectionStateType cs;
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_GetConnectionState(sid, &cs));
    TEST_ASSERT_EQUAL(TCPIP_CONNSTATE_CLOSED, cs);
}

void test_TcpIp_GetConnectionState_NullPtr_ShouldReportError(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetConnectionState(0U, NULL_PTR));
}

void test_TcpIp_GetTcpState_ValidCall_ShouldSucceed(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TcpIp_TcpStateType st;
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_GetTcpState(sid, &st));
    TEST_ASSERT_EQUAL(TCPIP_TCPSTATE_CLOSED, st);
}

void test_TcpIp_GetTcpState_NullPtr_ShouldReportError(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetTcpState(0U, NULL_PTR));
}

/* =========================================================================
 * MainFunction
 * ========================================================================= */

void test_TcpIp_MainFunction_Uninit_ShouldNotCrash(void) {
    TcpIp_MainFunction();
    TEST_ASSERT_EQUAL_UINT32(0U, mock_DetCallCount);
}

void test_TcpIp_MainFunction_GracefulCloseAdvancement(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TcpIp_SockAddrType raddr;
    memset(&raddr, 0, sizeof(raddr));
    raddr.port = 80U;
    TcpIp_Connect(sid, &raddr); /* -> ESTABLISHED */
    TcpIp_Close(sid, FALSE);    /* -> FIN_WAIT_1 */
    /* MainFunction advances: FIN_WAIT_1 -> FIN_WAIT_2 */
    TcpIp_MainFunction();
    TcpIp_TcpStateType st;
    TcpIp_GetTcpState(sid, &st);
    TEST_ASSERT_EQUAL(TCPIP_TCPSTATE_FIN_WAIT_2, st);
    /* FIN_WAIT_2 -> TIME_WAIT */
    TcpIp_MainFunction();
    TcpIp_GetTcpState(sid, &st);
    TEST_ASSERT_EQUAL(TCPIP_TCPSTATE_TIME_WAIT, st);
    /* TIME_WAIT -> force close (slot freed) */
    TcpIp_MainFunction();
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetTcpState(sid, &st));
}

/* =========================================================================
 * Reset
 * ========================================================================= */

void test_TcpIp_Reset_Uninit_ShouldFail(void) {
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_Reset());
}

void test_TcpIp_Reset_ValidCall_ShouldReinit(void) {
    TcpIp_Init(&testConfig);
    tcpip_initialized = TRUE;
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_Reset());
    /* After reset, should still be operational */
    TcpIp_LinkStateType ls;
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_GetLinkState(&ls));
}

/* =========================================================================
 * OpenSocket / CloseSocket / convenience wrappers
 * ========================================================================= */

void test_TcpIp_OpenSocket_Tcp_ShouldSucceed(void) {
    TcpIp_SocketIdType sid;
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_OpenSocket(TCPIP_IPPROTO_TCP, 8080U, &sid));
}

void test_TcpIp_OpenSocket_Udp_ShouldSucceed(void) {
    TcpIp_SocketIdType sid;
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_OpenSocket(TCPIP_IPPROTO_UDP, 5683U, &sid));
}

void test_TcpIp_OpenSocket_InvalidProtocol_ShouldFail(void) {
    TcpIp_SocketIdType sid;
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_OpenSocket(0xFFU, 80U, &sid));
}

void test_TcpIp_CloseSocket_ShouldFreeSlot(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_CloseSocket(sid));
    TcpIp_TcpStateType st;
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetTcpState(sid, &st));
}

void test_TcpIp_SocketCreate_NullPtr_ShouldFail(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_NOT_OK, TcpIp_SocketCreate(NULL_PTR));
}

void test_TcpIp_SocketCreate_ValidPtr_ShouldSucceed(void) {
    TcpIp_SocketIdType sid;
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(E_OK, TcpIp_SocketCreate(&sid));
}

void test_TcpIp_IsConnected_NotConnected_ShouldReturnFalse(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TEST_ASSERT_FALSE(TcpIp_IsConnected(sid));
}

void test_TcpIp_IsConnected_Connected_ShouldReturnTrue(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TcpIp_SockAddrType raddr;
    memset(&raddr, 0, sizeof(raddr));
    raddr.port = 80U;
    TcpIp_Connect(sid, &raddr); /* native sim -> ESTABLISHED */
    TEST_ASSERT_TRUE(TcpIp_IsConnected(sid));
}

void test_TcpIp_IsConnected_InvalidSocket_ShouldReturnFalse(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_FALSE(TcpIp_IsConnected(0xFFU));
}

void test_TcpIp_SocketClose_ShouldNotCrash(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TcpIp_SocketClose(sid);
}

void test_TcpIp_BindLocalAddr_ShouldAliasBind(void) {
    TcpIp_SocketIdType sid = helper_CreateTcpSocket();
    TcpIp_SockAddrType addr;
    memset(&addr, 0, sizeof(addr));
    addr.port = 1234U;
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_BindLocalAddr(sid, &addr));
}

/* =========================================================================
 * VLAN Config
 * ========================================================================= */

void test_TcpIp_SetVlanConfig_Uninit_ShouldReportError(void) {
    TcpIp_VlanConfigType vc = {TRUE, 100U, 3U, FALSE};
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_SetVlanConfig(&vc));
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_UNINIT, mock_DetLastErrorId);
}

void test_TcpIp_SetVlanConfig_NullPtr_ShouldReportError(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_SetVlanConfig(NULL_PTR));
}

void test_TcpIp_SetVlanConfig_ValidCall_ShouldSucceed(void) {
    TcpIp_Init(&testConfig);
    TcpIp_VlanConfigType vc = {TRUE, 100U, 3U, FALSE};
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_SetVlanConfig(&vc));
}

void test_TcpIp_SetVlanConfig_InvalidVid_ShouldFail(void) {
    TcpIp_Init(&testConfig);
    TcpIp_VlanConfigType vc = {TRUE, 4096U, 0U, FALSE};
    TEST_ASSERT_EQUAL(TCPIP_E_PARAM_CONFIG, TcpIp_SetVlanConfig(&vc));
}

void test_TcpIp_SetVlanConfig_InvalidPriority_ShouldFail(void) {
    TcpIp_Init(&testConfig);
    TcpIp_VlanConfigType vc = {TRUE, 100U, 8U, FALSE};
    TEST_ASSERT_EQUAL(TCPIP_E_PARAM_CONFIG, TcpIp_SetVlanConfig(&vc));
}

void test_TcpIp_GetVlanConfig_ValidCall_ShouldReturnConfig(void) {
    TcpIp_Init(&testConfig);
    TcpIp_VlanConfigType vcSet = {TRUE, 200U, 5U, TRUE};
    TcpIp_SetVlanConfig(&vcSet);
    TcpIp_VlanConfigType vcGet;
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_GetVlanConfig(&vcGet));
    TEST_ASSERT_TRUE(vcGet.VlanEnabled);
    TEST_ASSERT_EQUAL_UINT16(200U, vcGet.VlanId);
    TEST_ASSERT_EQUAL_UINT8(5U, vcGet.VlanPriority);
    TEST_ASSERT_TRUE(vcGet.DropUntagged);
}

void test_TcpIp_GetVlanConfig_NullPtr_ShouldReportError(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetVlanConfig(NULL_PTR));
}

/* =========================================================================
 * Statistics
 * ========================================================================= */

void test_TcpIp_GetStatistics_NullPtr_ShouldReportError(void) {
    TcpIp_StatisticsType stats;
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_GetStatistics(NULL_PTR));
    (void)stats;
}

void test_TcpIp_GetStatistics_ValidCall_ShouldSucceed(void) {
    TcpIp_StatisticsType stats;
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_GetStatistics(&stats));
}

void test_TcpIp_ResetStatistics_ValidCall_ShouldSucceed(void) {
    TcpIp_Init(&testConfig);
    TEST_ASSERT_EQUAL(TCPIP_OK, TcpIp_ResetStatistics());
}

void test_TcpIp_ResetStatistics_Uninit_ShouldReportError(void) {
    TEST_ASSERT_EQUAL(TCPIP_E_NOT_OK, TcpIp_ResetStatistics());
    TEST_ASSERT_EQUAL_UINT8(TCPIP_E_UNINIT, mock_DetLastErrorId);
}

/* =========================================================================
 * Runner
 * ========================================================================= */

int main(void) {
    UNITY_BEGIN();

    /* Init/DeInit/Version */
    RUN_TEST(test_TcpIp_Init_NullPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_Init_ValidConfig_ShouldSucceed);
    RUN_TEST(test_TcpIp_Init_DoubleInit_ShouldReportError);
    RUN_TEST(test_TcpIp_DeInit_Uninit_ShouldReportError);
    RUN_TEST(test_TcpIp_DeInit_Valid_ShouldResetState);
    RUN_TEST(test_TcpIp_GetVersionInfo_NullPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_GetVersionInfo_ValidPtr_ShouldReturnIds);

    /* Create */
    RUN_TEST(test_TcpIp_Create_Uninit_ShouldReportError);
    RUN_TEST(test_TcpIp_Create_NullPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_Create_TcpSocket_ShouldSucceed);
    RUN_TEST(test_TcpIp_Create_UdpSocket_ShouldSucceed);
    RUN_TEST(test_TcpIp_Create_SocketExhaustion_ShouldFail);

    /* Bind */
    RUN_TEST(test_TcpIp_Bind_Uninit_ShouldReportError);
    RUN_TEST(test_TcpIp_Bind_NullPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_Bind_InvalidSocket_ShouldFail);
    RUN_TEST(test_TcpIp_Bind_ValidCall_ShouldSucceed);

    /* Listen */
    RUN_TEST(test_TcpIp_Listen_Uninit_ShouldReportError);
    RUN_TEST(test_TcpIp_Listen_InvalidSocket_ShouldFail);
    RUN_TEST(test_TcpIp_Listen_UdpSocket_ShouldReturnInvalidProtocol);
    RUN_TEST(test_TcpIp_Listen_ValidCall_ShouldSucceed);
    RUN_TEST(test_TcpIp_Listen_ReListen_ShouldUpdateBacklog);
    RUN_TEST(test_TcpIp_Listen_InvalidState_ShouldFail);
    RUN_TEST(test_TcpIp_Listen_BacklogClamped_ShouldSucceed);

    /* Connect */
    RUN_TEST(test_TcpIp_Connect_Uninit_ShouldReportError);
    RUN_TEST(test_TcpIp_Connect_NullPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_Connect_InvalidSocket_ShouldFail);
    RUN_TEST(test_TcpIp_Connect_TcpNativeSim_ShouldReachEstablished);
    RUN_TEST(test_TcpIp_Connect_AlreadyConnected_ShouldReturnIsConn);
    RUN_TEST(test_TcpIp_Connect_UdpSocket_ShouldSucceed);

    /* Accept */
    RUN_TEST(test_TcpIp_Accept_Uninit_ShouldReportError);
    RUN_TEST(test_TcpIp_Accept_NullPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_Accept_NotListening_ShouldFail);
    RUN_TEST(test_TcpIp_Accept_NoPending_ShouldFail);
    RUN_TEST(test_TcpIp_Accept_WithPending_ShouldSucceed);

    /* Abort */
    RUN_TEST(test_TcpIp_Abort_Uninit_ShouldReportError);
    RUN_TEST(test_TcpIp_Abort_InvalidSocket_ShouldFail);
    RUN_TEST(test_TcpIp_Abort_ValidSocket_ShouldSucceed);

    /* Close */
    RUN_TEST(test_TcpIp_Close_Uninit_ShouldReportError);
    RUN_TEST(test_TcpIp_Close_InvalidSocket_ShouldFail);
    RUN_TEST(test_TcpIp_Close_ForceClose_ShouldFreeSlot);
    RUN_TEST(test_TcpIp_Close_GracefulClose_Established_ShouldStartClosing);
    RUN_TEST(test_TcpIp_Close_GracefulClose_AlreadyClosing_ShouldReturnOk);
    RUN_TEST(test_TcpIp_Close_GracefulClose_NotConnected_ShouldFreeSlot);

    /* Send/Transmit */
    RUN_TEST(test_TcpIp_Send_Uninit_ShouldReportError);
    RUN_TEST(test_TcpIp_Send_NullPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_Send_InvalidSocket_ShouldFail);
    RUN_TEST(test_TcpIp_Send_ValidCall_ShouldSucceed);
    RUN_TEST(test_TcpIp_Transmit_ShouldAliasSend);

    /* Receive */
    RUN_TEST(test_TcpIp_Receive_Uninit_ShouldReportError);
    RUN_TEST(test_TcpIp_Receive_NullBufPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_Receive_NullLenPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_Receive_NoData_ShouldReturnOkWithZeroLen);
    RUN_TEST(test_TcpIp_Receive_WithData_ShouldCopyAndDrain);
    RUN_TEST(test_TcpIp_Receive_BufferTooSmall_ShouldOverflow);

    /* ChangeTcpState */
    RUN_TEST(test_TcpIp_ChangeTcpState_Uninit_ShouldReportError);
    RUN_TEST(test_TcpIp_ChangeTcpState_InvalidSocket_ShouldFail);
    RUN_TEST(test_TcpIp_ChangeTcpState_ClosedToListen_ShouldSucceed);
    RUN_TEST(test_TcpIp_ChangeTcpState_InvalidTransition_ShouldFail);
    RUN_TEST(test_TcpIp_ChangeTcpState_Idempotent_ShouldSucceed);
    RUN_TEST(test_TcpIp_ChangeTcpState_ListenerSynReceived_ShouldAllocChild);
    RUN_TEST(test_TcpIp_ChangeTcpState_ListenerBacklogFull_ShouldReturnNoBufs);
    RUN_TEST(test_TcpIp_ChangeTcpState_ToEstablished_ShouldIncStats);
    RUN_TEST(test_TcpIp_ChangeTcpState_FullTcpLifecycle);
    RUN_TEST(test_TcpIp_ChangeTcpState_CloseWaitLastAck);
    RUN_TEST(test_TcpIp_ChangeTcpState_Closing);

    /* RxIndication/TxConfirmation */
    RUN_TEST(test_TcpIp_RxIndication_Uninit_ShouldReportError);
    RUN_TEST(test_TcpIp_RxIndication_NullPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_RxIndication_InvalidSocket_ShouldFail);
    RUN_TEST(test_TcpIp_RxIndication_ZeroLength_ShouldReturnOk);
    RUN_TEST(test_TcpIp_RxIndication_UserBuffer_ShouldCopyToUserBuf);
    RUN_TEST(test_TcpIp_RxIndication_PoolBuffer_ShouldQueueData);
    RUN_TEST(test_TcpIp_RxIndication_Overflow_ShouldReturnBufferOverflow);
    RUN_TEST(test_TcpIp_RxIndication_TooLarge_ShouldReturnBufferOverflow);
    RUN_TEST(test_TcpIp_TxConfirmation_Uninit_ShouldReportError);
    RUN_TEST(test_TcpIp_TxConfirmation_InvalidSocket_ShouldFail);
    RUN_TEST(test_TcpIp_TxConfirmation_ValidCall_ShouldSucceed);

    /* GetRxBuffer/ReleaseRxBuffer/GetTxBuffer/ReleaseTxBuffer */
    RUN_TEST(test_TcpIp_GetRxBuffer_Uninit_ShouldReportError);
    RUN_TEST(test_TcpIp_GetRxBuffer_NullPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_GetRxBuffer_NoData_ShouldFail);
    RUN_TEST(test_TcpIp_GetRxBuffer_WithData_ShouldReturnData);
    RUN_TEST(test_TcpIp_GetRxBuffer_ChunkInUse_ShouldReturnNoBufs);
    RUN_TEST(test_TcpIp_ReleaseRxBuffer_Uninit_ShouldReportError);
    RUN_TEST(test_TcpIp_ReleaseRxBuffer_NoData_ShouldFail);
    RUN_TEST(test_TcpIp_ReleaseRxBuffer_AfterGet_ShouldReleaseChunk);
    RUN_TEST(test_TcpIp_GetTxBuffer_Uninit_ShouldReportError);
    RUN_TEST(test_TcpIp_GetTxBuffer_NullPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_GetTxBuffer_ValidCall_ShouldReturnBuffer);
    RUN_TEST(test_TcpIp_ReleaseTxBuffer_Uninit_ShouldReportError);
    RUN_TEST(test_TcpIp_ReleaseTxBuffer_InvalidSocket_ShouldFail);
    RUN_TEST(test_TcpIp_ReleaseTxBuffer_Overflow_ShouldFail);
    RUN_TEST(test_TcpIp_ReleaseTxBuffer_ValidCall_ShouldSucceed);

    /* SetRxBuffer */
    RUN_TEST(test_TcpIp_SetRxBuffer_Uninit_ShouldReportError);
    RUN_TEST(test_TcpIp_SetRxBuffer_NullPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_SetRxBuffer_ValidCall_ShouldSucceed);

    /* TCP Options */
    RUN_TEST(test_TcpIp_SetTcpOption_AllOptions_ShouldSucceed);
    RUN_TEST(test_TcpIp_SetTcpOption_InvalidOption_ShouldFail);
    RUN_TEST(test_TcpIp_SetTcpOption_InvalidSocket_ShouldFail);
    RUN_TEST(test_TcpIp_SetTcpOption_Uninit_ShouldReportError);
    RUN_TEST(test_TcpIp_GetTcpOption_NullPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_GetTcpOption_AllOptions_ShouldReturnValues);
    RUN_TEST(test_TcpIp_GetTcpOption_InvalidOption_ShouldFail);

    /* UDP Options */
    RUN_TEST(test_TcpIp_SetUdpOption_AllOptions_ShouldSucceed);
    RUN_TEST(test_TcpIp_SetUdpOption_InvalidOption_ShouldFail);
    RUN_TEST(test_TcpIp_SetUdpOption_Uninit_ShouldReportError);
    RUN_TEST(test_TcpIp_SetUdpOption_InvalidSocket_ShouldFail);
    RUN_TEST(test_TcpIp_GetUdpOption_NullPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_GetUdpOption_AllOptions_ShouldReturnValues);
    RUN_TEST(test_TcpIp_GetUdpOption_InvalidOption_ShouldFail);

    /* Address functions */
    RUN_TEST(test_TcpIp_GetIPv4Addr_Uninit_ShouldReportError);
    RUN_TEST(test_TcpIp_GetIPv4Addr_NullPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_GetIPv4Addr_ValidCall_ShouldReturnDefault);
    RUN_TEST(test_TcpIp_GetIPv6Addr_ValidCall_ShouldReturnNotSupported);
    RUN_TEST(test_TcpIp_GetIPv6Addr_NullPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_GetIPv4SubnetMask_ValidCall_ShouldSucceed);
    RUN_TEST(test_TcpIp_GetIPv4SubnetMask_InvalidIfIdx_ShouldFail);
    RUN_TEST(test_TcpIp_GetIPv4SubnetMask_NullPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_GetLinkState_NullPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_GetLinkState_ValidCall_ShouldReturnUp);
    RUN_TEST(test_TcpIp_GetInterfaceState_ValidCall_ShouldSucceed);
    RUN_TEST(test_TcpIp_GetInterfaceState_NullPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_GetIpAddrState_ValidCall_ShouldSucceed);
    RUN_TEST(test_TcpIp_GetIpAddrState_InvalidIfIdx_ShouldFail);
    RUN_TEST(test_TcpIp_GetIpAddrState_NullPtr_ShouldReportError);

    /* Set/Get Remote/Local Addr */
    RUN_TEST(test_TcpIp_SetRemoteAddr_ValidCall_ShouldSucceed);
    RUN_TEST(test_TcpIp_SetRemoteAddr_NullPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_SetLocalAddr_ValidCall_ShouldSucceed);
    RUN_TEST(test_TcpIp_SetLocalAddr_NullPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_GetLocalAddr_ValidCall_ShouldSucceed);
    RUN_TEST(test_TcpIp_GetLocalAddr_NullPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_GetRemoteAddr_ValidCall_ShouldSucceed);
    RUN_TEST(test_TcpIp_GetRemoteAddr_NullPtr_ShouldReportError);

    /* Connection/TCP state */
    RUN_TEST(test_TcpIp_GetConnectionState_ValidCall_ShouldSucceed);
    RUN_TEST(test_TcpIp_GetConnectionState_NullPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_GetTcpState_ValidCall_ShouldSucceed);
    RUN_TEST(test_TcpIp_GetTcpState_NullPtr_ShouldReportError);

    /* MainFunction */
    RUN_TEST(test_TcpIp_MainFunction_Uninit_ShouldNotCrash);
    RUN_TEST(test_TcpIp_MainFunction_GracefulCloseAdvancement);

    /* Reset */
    RUN_TEST(test_TcpIp_Reset_Uninit_ShouldFail);
    RUN_TEST(test_TcpIp_Reset_ValidCall_ShouldReinit);

    /* OpenSocket/CloseSocket/convenience */
    RUN_TEST(test_TcpIp_OpenSocket_Tcp_ShouldSucceed);
    RUN_TEST(test_TcpIp_OpenSocket_Udp_ShouldSucceed);
    RUN_TEST(test_TcpIp_OpenSocket_InvalidProtocol_ShouldFail);
    RUN_TEST(test_TcpIp_CloseSocket_ShouldFreeSlot);
    RUN_TEST(test_TcpIp_SocketCreate_NullPtr_ShouldFail);
    RUN_TEST(test_TcpIp_SocketCreate_ValidPtr_ShouldSucceed);
    RUN_TEST(test_TcpIp_IsConnected_NotConnected_ShouldReturnFalse);
    RUN_TEST(test_TcpIp_IsConnected_Connected_ShouldReturnTrue);
    RUN_TEST(test_TcpIp_IsConnected_InvalidSocket_ShouldReturnFalse);
    RUN_TEST(test_TcpIp_SocketClose_ShouldNotCrash);
    RUN_TEST(test_TcpIp_BindLocalAddr_ShouldAliasBind);

    /* VLAN */
    RUN_TEST(test_TcpIp_SetVlanConfig_Uninit_ShouldReportError);
    RUN_TEST(test_TcpIp_SetVlanConfig_NullPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_SetVlanConfig_ValidCall_ShouldSucceed);
    RUN_TEST(test_TcpIp_SetVlanConfig_InvalidVid_ShouldFail);
    RUN_TEST(test_TcpIp_SetVlanConfig_InvalidPriority_ShouldFail);
    RUN_TEST(test_TcpIp_GetVlanConfig_ValidCall_ShouldReturnConfig);
    RUN_TEST(test_TcpIp_GetVlanConfig_NullPtr_ShouldReportError);

    /* Statistics */
    RUN_TEST(test_TcpIp_GetStatistics_NullPtr_ShouldReportError);
    RUN_TEST(test_TcpIp_GetStatistics_ValidCall_ShouldSucceed);
    RUN_TEST(test_TcpIp_ResetStatistics_ValidCall_ShouldSucceed);
    RUN_TEST(test_TcpIp_ResetStatistics_Uninit_ShouldReportError);

    return UNITY_END();
}
