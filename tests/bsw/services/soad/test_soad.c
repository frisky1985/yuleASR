/**
 * @file test_soad.c
 * @brief Unity-based unit tests for the AUTOSAR Socket Adapter (SoAd) module.
 *
 * Exercises the real src/bsw/services/soad/src/SoAd.c against the TcpIp / Det
 * stubs in stubs.c.  Target coverage: ≥ 70% line coverage across SoAd.c.
 */

#include "SoAd.h"
#include "SoAd_Cfg.h"
#include "TcpIp.h"
#include "Std_Types.h"
#include "ComStack_Types.h"

#include "unity.h"

#include <string.h>

/* Declared in stubs.c — counters that the tests assert on. */
extern uint32 stub_Det_ReportError_Count;
extern uint16 stub_Det_Last_ModuleId;
extern uint8  stub_Det_Last_ApiId;
extern uint8  stub_Det_Last_ErrorId;

extern uint32 stub_TcpIp_Create_Count;
extern uint32 stub_TcpIp_Bind_Count;
extern uint32 stub_TcpIp_Close_Count;
extern uint32 stub_TcpIp_Send_Count;
extern uint32 stub_TcpIp_Receive_Count;

extern void SoAdStub_Reset(void);
extern void SoAdStub_SetCreateFail(TcpIp_SocketIdType socketId, TcpIp_ReturnType ret);
extern void SoAdStub_SetSendFail(TcpIp_SocketIdType socketId, TcpIp_ReturnType ret);
extern void SoAdStub_SetReceiveFail(TcpIp_SocketIdType socketId, TcpIp_ReturnType ret);
extern void SoAdStub_SeedReceive(TcpIp_SocketIdType socketId, const uint8 *data, uint16 len);
extern const uint8 *SoAdStub_GetTxCapture(TcpIp_SocketIdType socketId, uint16 *lenOut);

/* ------------------------------------------------------------------------- */
/*  setUp / tearDown                                                         */
/* ------------------------------------------------------------------------- */

void setUp(void)
{
    /* 1) Best-effort teardown of SoAd — DeInit tolerates both INIT (clean
     *    teardown) and UNINIT (DET error that we discard). */
    SoAd_DeInit();

    /* 2) Wipe every stub counter / seed so the test starts from zero. */
    SoAdStub_Reset();

    /* 3) Extra assurance: reset the counter that DeInit may have just
     *    incremented (so the first assertion of every test starts at 0). */
    stub_Det_ReportError_Count = 0U;
}

void tearDown(void)
{
    /* Leave SoAd in UNINIT for the next setUp. */
    SoAd_DeInit();
}

/* ------------------------------------------------------------------------- */
/*  SoAd_Init                                                                */
/* ------------------------------------------------------------------------- */

void test_SoAd_Init_NullPointerTriggersDet(void)
{
    SoAd_Init(NULL_PTR);

    TEST_ASSERT_EQUAL_UINT32(1U, stub_Det_ReportError_Count);
    TEST_ASSERT_EQUAL_UINT8(SOAD_SID_INIT, stub_Det_Last_ApiId);
    TEST_ASSERT_EQUAL_UINT8(SOAD_E_PARAM_POINTER, stub_Det_Last_ErrorId);
}

void test_SoAd_Init_WithConfigInitializesAllConnections(void)
{
    SoAd_Init(&SoAd_Config);

    /* Init itself should not call TcpIp anything. */
    TEST_ASSERT_EQUAL_UINT32(0U, stub_TcpIp_Create_Count);
    TEST_ASSERT_EQUAL_UINT32(0U, stub_TcpIp_Close_Count);

    /* DET should be clean. */
    TEST_ASSERT_EQUAL_UINT32(0U, stub_Det_ReportError_Count);
}

void test_SoAd_Init_AlreadyInitializedTriggersDet(void)
{
    SoAd_Init(&SoAd_Config);
    SoAd_Init(&SoAd_Config);

    TEST_ASSERT_EQUAL_UINT32(1U, stub_Det_ReportError_Count);
    TEST_ASSERT_EQUAL_UINT8(SOAD_SID_INIT, stub_Det_Last_ApiId);
    TEST_ASSERT_EQUAL_UINT8(SOAD_E_ALREADY_INITIALIZED, stub_Det_Last_ErrorId);
}

/* ------------------------------------------------------------------------- */
/*  SoAd_DeInit                                                              */
/* ------------------------------------------------------------------------- */

void test_SoAd_DeInit_BeforeInitTriggersDet(void)
{
    SoAd_DeInit();
    TEST_ASSERT_EQUAL_UINT32(1U, stub_Det_ReportError_Count);
    TEST_ASSERT_EQUAL_UINT8(SOAD_SID_DEINIT, stub_Det_Last_ApiId);
    TEST_ASSERT_EQUAL_UINT8(SOAD_E_UNINIT, stub_Det_Last_ErrorId);
}

void test_SoAd_DeInit_AfterInitIsClean(void)
{
    SoAd_Init(&SoAd_Config);
    stub_Det_ReportError_Count = 0U;

    SoAd_DeInit();
    TEST_ASSERT_EQUAL_UINT32(0U, stub_Det_ReportError_Count);

    /* Second DeInit is an UNINIT error. */
    SoAd_DeInit();
    TEST_ASSERT_EQUAL_UINT32(1U, stub_Det_ReportError_Count);
    TEST_ASSERT_EQUAL_UINT8(SOAD_E_UNINIT, stub_Det_Last_ErrorId);
}

/* ------------------------------------------------------------------------- */
/*  SoAd_GetVersionInfo                                                      */
/* ------------------------------------------------------------------------- */

void test_SoAd_GetVersionInfo_NullPointer(void)
{
    Std_VersionInfoType vi;
    memset(&vi, 0, sizeof(vi));
    SoAd_GetVersionInfo(NULL_PTR);

    TEST_ASSERT_EQUAL_UINT32(1U, stub_Det_ReportError_Count);
    TEST_ASSERT_EQUAL_UINT8(SOAD_SID_GETVERSIONINFO, stub_Det_Last_ApiId);
    TEST_ASSERT_EQUAL_UINT8(SOAD_E_PARAM_POINTER, stub_Det_Last_ErrorId);

    SoAd_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL_UINT16(SOAD_MODULE_ID, vi.moduleID);
    TEST_ASSERT_EQUAL_UINT16(SOAD_VENDOR_ID, vi.vendorID);
    TEST_ASSERT_EQUAL_UINT8(SOAD_SW_MAJOR_VERSION, vi.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(SOAD_SW_MINOR_VERSION, vi.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(SOAD_SW_PATCH_VERSION, vi.sw_patch_version);
}

/* ------------------------------------------------------------------------- */
/*  SoAd_OpenTcpConnection                                                   */
/* ------------------------------------------------------------------------- */

void test_SoAd_OpenTcpConnection_NotInitialized(void)
{
    Std_ReturnType r = SoAd_OpenTcpConnection(0U);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, r);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_Det_ReportError_Count);
    TEST_ASSERT_EQUAL_UINT8(SOAD_SID_OPENTCPCONNECTION, stub_Det_Last_ApiId);
    TEST_ASSERT_EQUAL_UINT8(SOAD_E_UNINIT, stub_Det_Last_ErrorId);
}

void test_SoAd_OpenTcpConnection_InvalidConnId(void)
{
    SoAd_Init(&SoAd_Config);
    Std_ReturnType r = SoAd_OpenTcpConnection((uint16)SOAD_NUMBER_OF_CONNECTIONS);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, r);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_Det_ReportError_Count);
    TEST_ASSERT_EQUAL_UINT8(SOAD_E_INVALID_CONNID, stub_Det_Last_ErrorId);
}

void test_SoAd_OpenTcpConnection_NominalFlow(void)
{
    SoAd_Init(&SoAd_Config);

    Std_ReturnType r = SoAd_OpenTcpConnection(0U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, r);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_TcpIp_Create_Count);

    /* A second Open on the same ConId must fail (state != CLOSED). */
    r = SoAd_OpenTcpConnection(0U);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, r);

    SoAd_DeInit();
}

void test_SoAd_OpenTcpConnection_TcpIpCreateFailure(void)
{
    SoAd_Init(&SoAd_Config);
    /* Force TcpIp_Create to fail for slot 0. */
    SoAdStub_SetCreateFail(0U, TCPIP_E_NOT_OK);

    Std_ReturnType r = SoAd_OpenTcpConnection(0U);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, r);

    SoAd_DeInit();
}

/* ------------------------------------------------------------------------- */
/*  SoAd_OpenUdpConnection                                                   */
/* ------------------------------------------------------------------------- */

void test_SoAd_OpenUdpConnection_NominalFlow(void)
{
    SoAd_Init(&SoAd_Config);

    Std_ReturnType r = SoAd_OpenUdpConnection(1U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, r);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_TcpIp_Create_Count);

    /* UDP connection enters CONNECTED immediately — send should work. */
    PduInfoType info = { NULL_PTR, NULL_PTR, 0U };
    uint8 buf[4] = {0x11, 0x22, 0x33, 0x44};
    info.SduDataPtr = buf;
    info.SduLength  = 4U;
    r = SoAd_Send(1U, &info);
    TEST_ASSERT_EQUAL_UINT8(E_OK, r);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_TcpIp_Send_Count);

    SoAd_DeInit();
}

void test_SoAd_OpenUdpConnection_NotInitialized(void)
{
    Std_ReturnType r = SoAd_OpenUdpConnection(0U);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, r);
    TEST_ASSERT_EQUAL_UINT8(SOAD_SID_OPENUdpConnection, stub_Det_Last_ApiId);
}

/* ------------------------------------------------------------------------- */
/*  SoAd_CloseTcpConnection / SoAd_CloseUdpConnection                        */
/* ------------------------------------------------------------------------- */

void test_SoAd_CloseTcpConnection_NotInitialized(void)
{
    Std_ReturnType r = SoAd_CloseTcpConnection(0U, TRUE);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, r);
    TEST_ASSERT_EQUAL_UINT8(SOAD_SID_CLOSETCPConnection, stub_Det_Last_ApiId);
}

void test_SoAd_CloseTcpConnection_InvalidConnId(void)
{
    SoAd_Init(&SoAd_Config);
    Std_ReturnType r = SoAd_CloseTcpConnection((uint16)SOAD_NUMBER_OF_CONNECTIONS, TRUE);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, r);
    TEST_ASSERT_EQUAL_UINT8(SOAD_E_INVALID_CONNID, stub_Det_Last_ErrorId);
}

void test_SoAd_CloseTcpConnection_NominalFlow(void)
{
    SoAd_Init(&SoAd_Config);
    (void)SoAd_OpenTcpConnection(0U);

    Std_ReturnType r = SoAd_CloseTcpConnection(0U, TRUE);
    TEST_ASSERT_EQUAL_UINT8(E_OK, r);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_TcpIp_Close_Count);

    SoAd_DeInit();
}

void test_SoAd_CloseUdpConnection_DelegatesToCloseTcp(void)
{
    SoAd_Init(&SoAd_Config);
    (void)SoAd_OpenUdpConnection(0U);

    Std_ReturnType r = SoAd_CloseUdpConnection(0U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, r);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_TcpIp_Close_Count);
}

/* ------------------------------------------------------------------------- */
/*  SoAd_Send                                                                */
/* ------------------------------------------------------------------------- */

void test_SoAd_Send_NotInitialized(void)
{
    PduInfoType info = { NULL_PTR, NULL_PTR, 0U };
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, SoAd_Send(0U, &info));
    TEST_ASSERT_EQUAL_UINT8(SOAD_SID_SEND, stub_Det_Last_ApiId);
}

void test_SoAd_Send_NullPduInfo(void)
{
    SoAd_Init(&SoAd_Config);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, SoAd_Send(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(SOAD_SID_SEND, stub_Det_Last_ApiId);
    TEST_ASSERT_EQUAL_UINT8(SOAD_E_PARAM_POINTER, stub_Det_Last_ErrorId);
}

void test_SoAd_Send_InvalidConnId(void)
{
    SoAd_Init(&SoAd_Config);
    PduInfoType info = { NULL_PTR, NULL_PTR, 0U };
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, SoAd_Send((uint16)SOAD_NUMBER_OF_CONNECTIONS, &info));
    TEST_ASSERT_EQUAL_UINT8(SOAD_E_INVALID_CONNID, stub_Det_Last_ErrorId);
}

void test_SoAd_Send_ClosedConnectionRejects(void)
{
    SoAd_Init(&SoAd_Config);
    PduInfoType info = { NULL_PTR, NULL_PTR, 0U };
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, SoAd_Send(0U, &info));
    SoAd_DeInit();
}

void test_SoAd_Send_TcpIpSendFailureIsNotOk(void)
{
    SoAd_Init(&SoAd_Config);
    (void)SoAd_OpenUdpConnection(0U);

    /* OpenUDP created socket slot 0 — force Send on that socket to fail. */
    SoAdStub_SetSendFail(0U, TCPIP_E_NOT_OK);

    uint8 buf[3] = {1, 2, 3};
    PduInfoType info = { buf, NULL_PTR, 3U };
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, SoAd_Send(0U, &info));

    SoAd_DeInit();
}

/* ------------------------------------------------------------------------- */
/*  SoAd_Receive                                                             */
/* ------------------------------------------------------------------------- */

void test_SoAd_Receive_NotInitialized(void)
{
    PduInfoType info = { NULL_PTR, NULL_PTR, 0U };
    PduLengthType len = 0U;
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, SoAd_Receive(0U, &info, &len));
    TEST_ASSERT_EQUAL_UINT8(SOAD_SID_RECEIVE, stub_Det_Last_ApiId);
}

void test_SoAd_Receive_NullArgs(void)
{
    SoAd_Init(&SoAd_Config);
    PduLengthType len = 0U;
    PduInfoType info = { NULL_PTR, NULL_PTR, 0U };

    /* NULL PduInfoPtr */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, SoAd_Receive(0U, NULL_PTR, &len));
    TEST_ASSERT_EQUAL_UINT8(SOAD_SID_RECEIVE, stub_Det_Last_ApiId);

    /* NULL Length */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, SoAd_Receive(0U, &info, NULL_PTR));

    SoAd_DeInit();
}

void test_SoAd_Receive_ClosedConnectionRejects(void)
{
    SoAd_Init(&SoAd_Config);
    PduInfoType info = { NULL_PTR, NULL_PTR, 0U };
    PduLengthType len = 0U;
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, SoAd_Receive(0U, &info, &len));
    SoAd_DeInit();
}

void test_SoAd_Receive_NominalFlow(void)
{
    SoAd_Init(&SoAd_Config);
    /* UDP opens straight into CONNECTED. */
    (void)SoAd_OpenUdpConnection(0U);

    uint8 seed[5] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    SoAdStub_SeedReceive(0U, seed, sizeof(seed));

    uint8 rxBuf[16] = {0};
    PduInfoType info = { rxBuf, NULL_PTR, sizeof(rxBuf) };
    PduLengthType len = 0U;

    Std_ReturnType r = SoAd_Receive(0U, &info, &len);
    TEST_ASSERT_EQUAL_UINT8(E_OK, r);
    TEST_ASSERT_EQUAL_UINT32(sizeof(seed), len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(seed, rxBuf, sizeof(seed));

    SoAd_DeInit();
}

/* ------------------------------------------------------------------------- */
/*  SoAd_GetRemoteAddr / SoAd_SetRemoteAddr                                  */
/* ------------------------------------------------------------------------- */

void test_SoAd_GetSetRemoteAddr_NominalAndDet(void)
{
    SoAd_Init(&SoAd_Config);

    TcpIp_SockAddrType addr;
    memset(&addr, 0, sizeof(addr));
    addr.domain = TCPIP_AF_INET;
    addr.port   = 5000U;
    addr.addr[0] = 192U;
    addr.addr[1] = 168U;
    addr.addr[2] = 1U;
    addr.addr[3] = 42U;

    TEST_ASSERT_EQUAL_UINT8(E_OK, SoAd_SetRemoteAddr(0U, &addr));

    /* DET: NULL pointer. */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, SoAd_SetRemoteAddr(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(SOAD_SID_SETREMOTESADDR, stub_Det_Last_ApiId);
    TEST_ASSERT_EQUAL_UINT8(SOAD_E_PARAM_POINTER, stub_Det_Last_ErrorId);

    /* DET: GetRemoteAddr NULL pointers. */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, SoAd_GetRemoteAddr(0U, NULL_PTR, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(SOAD_SID_GETREMOTESADDR, stub_Det_Last_ApiId);
    TEST_ASSERT_EQUAL_UINT8(SOAD_E_PARAM_POINTER, stub_Det_Last_ErrorId);

    /* DET: GetRemoteAddr before init. */
    TcpIp_SockAddrType gotAddr;
    uint16 port;
    SoAd_DeInit();
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, SoAd_GetRemoteAddr(0U, &gotAddr, &port));
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, SoAd_SetRemoteAddr(0U, &addr));

    SoAd_Init(&SoAd_Config);
    TEST_ASSERT_EQUAL_UINT8(E_OK, SoAd_GetRemoteAddr(0U, &gotAddr, &port));
    TEST_ASSERT_EQUAL_UINT16(5000U, port);
    SoAd_DeInit();
}

/* ------------------------------------------------------------------------- */
/*  SoAd_RequestConnMode (always returns E_NOT_OK per product source)        */
/* ------------------------------------------------------------------------- */

void test_SoAd_RequestConnMode_IsStub(void)
{
    SoAd_Init(&SoAd_Config);
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, SoAd_RequestConnMode(0U, SOAD_CONNMODE_REQUESTED_UDP));
    SoAd_DeInit();
}

/* ------------------------------------------------------------------------- */
/*  SoAd_ReleaseIpAddrAssignment / SoAd_RequestIpAddrAssignment (stubs)      */
/* ------------------------------------------------------------------------- */

void test_SoAd_IpAddrAssignmentStubs(void)
{
    SoAd_Init(&SoAd_Config);
    TEST_ASSERT_EQUAL_UINT8(E_OK, SoAd_ReleaseIpAddrAssignment(0U));
    TEST_ASSERT_EQUAL_UINT8(E_OK, SoAd_RequestIpAddrAssignment(0U, TCPIP_IPADDR_ASSIGNMENT_STATIC));
    SoAd_DeInit();
}

/* ------------------------------------------------------------------------- */
/*  SoAd_MainFunction                                                        */
/* ------------------------------------------------------------------------- */

void test_SoAd_MainFunction_TcpConnectTimeoutExpires(void)
{
    SoAd_Init(&SoAd_Config);
    (void)SoAd_OpenTcpConnection(0U);

    /* MainFunction is a no-op before init, so confirm that path does
     * nothing weird: */
    SoAd_MainFunction(); /* in CONNECTING, ticks down once */

    /* Pump until ConnectTimeout drops to 0.  SOAD_CONNECT_TIMEOUT_MS /
     * SOAD_MAIN_FUNCTION_PERIOD_MS  == 5000 / 10 == 500 ticks. */
    uint32 ticks = 0U;
    while (ticks < 501U) {
        SoAd_MainFunction();
        ticks++;
    }

    /* The connection should now be closed and Close should have been called. */
    TEST_ASSERT_EQUAL_UINT32(1U, stub_TcpIp_Close_Count);

    /* Second Open + a MainFunction before connection is just idle for the
     * Closed state — coverage of the non-connecting branch. */
    (void)SoAd_OpenUdpConnection(1U);
    SoAd_MainFunction(); /* UDP -> CONNECTED, timer still starts but MainFunction
                         *   only decrements CONNECTING / DISCONNECTING states */

    SoAd_DeInit();
}

void test_SoAd_MainFunction_NotInitializedIsSilent(void)
{
    stub_Det_ReportError_Count = 0U;
    SoAd_MainFunction(); /* must not crash and must not DET */
    TEST_ASSERT_EQUAL_UINT32(0U, stub_Det_ReportError_Count);
}

/* ------------------------------------------------------------------------- */
/*  Callback surfaces                                                        */
/* ------------------------------------------------------------------------- */

void test_SoAd_Callbacks_DoNotCrashAndCoverFindConnection(void)
{
    SoAd_Init(&SoAd_Config);

    /* Open UDP so a socket slot exists and SoAd knows about a CONNECTED conn. */
    (void)SoAd_OpenUdpConnection(0U);
    /* Open TCP too — different slot, reaches CONNECTING state. */
    (void)SoAd_OpenTcpConnection(1U);

    TcpIp_SockAddrType remote = {0};
    uint8 data[4] = {0};

    /* Finds CONNECTED socket (slot 0), enters the inner if-block without
     * doing anything (RxIndication body is empty in SoAd.c). */
    SoAd_RxIndication(0U, &remote, data, sizeof(data));

    /* Finds CONNECTING socket (slot 1) — inner state check skips. */
    SoAd_RxIndication(1U, &remote, data, sizeof(data));

    /* Unknown socket id — FindConnectionBySocket returns E_NOT_OK. */
    SoAd_RxIndication(0xFFU, &remote, data, sizeof(data));

    /* TxConfirmation / TcpIpEvent / LocalIpAddrAssignmentChg are stubs that
     * discard all args — just verify they're callable after Init. */
    SoAd_TxConfirmation(0U, 4U);
    SoAd_TcpIpEvent(0U, TCPIP_EVENT_CONNECTED, TCPIP_OK);
    SoAd_LocalIpAddrAssignmentChg(0U, TCPIP_IPADDR_STATE_ASSIGNED);

    /* Triggers TcpIpEvent with other event values. */
    SoAd_TcpIpEvent(0U, TCPIP_EVENT_DISCONNECTED, TCPIP_OK);
    SoAd_TcpIpEvent(0U, TCPIP_EVENT_TIMEOUT, TCPIP_E_TIMEOUT_VAL);
    SoAd_TcpIpEvent(0U, TCPIP_EVENT_DATA_AVAILABLE, TCPIP_OK);
    SoAd_TcpIpEvent(0U, TCPIP_EVENT_ERROR, TCPIP_E_NOT_OK);

    SoAd_TxConfirmation(0xFFU, 0U);

    SoAd_DeInit();
}

/* ------------------------------------------------------------------------- */
/*  SoAd_DeInit closes all connections it can find                           */
/* ------------------------------------------------------------------------- */

void test_SoAd_DeInit_ClosesOpenConnections(void)
{
    SoAd_Init(&SoAd_Config);
    (void)SoAd_OpenTcpConnection(0U);   /* CONNECTING */
    (void)SoAd_OpenUdpConnection(1U);   /* CONNECTED */

    SoAd_DeInit();

    /* DeInit unconditionally calls SoAd_CloseTcpConnection(0, TRUE) — this
     * must result in one TcpIp_Close call. */
    TEST_ASSERT_EQUAL_UINT32(1U, stub_TcpIp_Close_Count);

    /* And DET-clean (not yet de-init error again — that comes after). */
    TEST_ASSERT_EQUAL_UINT32(0U, stub_Det_ReportError_Count);
}

/* ------------------------------------------------------------------------- */
/*  SoAd_OpenTcpConnection_BindWhenConfigured                                */
/*  (SoAd.c calls TcpIp_Bind if RemotePort > 0U.  Our SoAd_Config default
 *  connections have RemotePort == 0U, so to hit the bind path we use slot 2.)
/* ------------------------------------------------------------------------- */

void test_SoAd_OpenTcpConnection_BindWhenRemotePortNonZero(void)
{
    SoAd_Init(&SoAd_Config);
    /* Mutate the config on the fly — SoAd stores the pointer, so the live
     * array entries drive runtime behaviour. */
    extern SoAd_ConnectionConfigType g_conn_cfgs[SOAD_NUMBER_OF_CONNECTIONS];
    g_conn_cfgs[2].RemotePort = 6000U;

    Std_ReturnType r = SoAd_OpenTcpConnection(2U);
    TEST_ASSERT_EQUAL_UINT8(E_OK, r);
    TEST_ASSERT_EQUAL_UINT32(1U, stub_TcpIp_Bind_Count);

    SoAd_DeInit();
}

/* ------------------------------------------------------------------------- */
/*  main                                                                     */
/* ------------------------------------------------------------------------- */

int main(void)
{
    UNITY_BEGIN();

    /* Init / DeInit */
    RUN_TEST(test_SoAd_Init_NullPointerTriggersDet);
    RUN_TEST(test_SoAd_Init_WithConfigInitializesAllConnections);
    RUN_TEST(test_SoAd_Init_AlreadyInitializedTriggersDet);
    RUN_TEST(test_SoAd_DeInit_BeforeInitTriggersDet);
    RUN_TEST(test_SoAd_DeInit_AfterInitIsClean);
    RUN_TEST(test_SoAd_DeInit_ClosesOpenConnections);

    /* VersionInfo */
    RUN_TEST(test_SoAd_GetVersionInfo_NullPointer);

    /* OpenTcpConnection */
    RUN_TEST(test_SoAd_OpenTcpConnection_NotInitialized);
    RUN_TEST(test_SoAd_OpenTcpConnection_InvalidConnId);
    RUN_TEST(test_SoAd_OpenTcpConnection_NominalFlow);
    RUN_TEST(test_SoAd_OpenTcpConnection_TcpIpCreateFailure);
    RUN_TEST(test_SoAd_OpenTcpConnection_BindWhenRemotePortNonZero);

    /* OpenUdpConnection */
    RUN_TEST(test_SoAd_OpenUdpConnection_NominalFlow);
    RUN_TEST(test_SoAd_OpenUdpConnection_NotInitialized);

    /* Close */
    RUN_TEST(test_SoAd_CloseTcpConnection_NotInitialized);
    RUN_TEST(test_SoAd_CloseTcpConnection_InvalidConnId);
    RUN_TEST(test_SoAd_CloseTcpConnection_NominalFlow);
    RUN_TEST(test_SoAd_CloseUdpConnection_DelegatesToCloseTcp);

    /* Send */
    RUN_TEST(test_SoAd_Send_NotInitialized);
    RUN_TEST(test_SoAd_Send_NullPduInfo);
    RUN_TEST(test_SoAd_Send_InvalidConnId);
    RUN_TEST(test_SoAd_Send_ClosedConnectionRejects);
    RUN_TEST(test_SoAd_Send_TcpIpSendFailureIsNotOk);

    /* Receive */
    RUN_TEST(test_SoAd_Receive_NotInitialized);
    RUN_TEST(test_SoAd_Receive_NullArgs);
    RUN_TEST(test_SoAd_Receive_ClosedConnectionRejects);
    RUN_TEST(test_SoAd_Receive_NominalFlow);

    /* RemoteAddr */
    RUN_TEST(test_SoAd_GetSetRemoteAddr_NominalAndDet);

    /* ConnMode & IpAddr stub surfaces */
    RUN_TEST(test_SoAd_RequestConnMode_IsStub);
    RUN_TEST(test_SoAd_IpAddrAssignmentStubs);

    /* MainFunction */
    RUN_TEST(test_SoAd_MainFunction_TcpConnectTimeoutExpires);
    RUN_TEST(test_SoAd_MainFunction_NotInitializedIsSilent);

    /* Callbacks */
    RUN_TEST(test_SoAd_Callbacks_DoNotCrashAndCoverFindConnection);

    return UNITY_END();
}
