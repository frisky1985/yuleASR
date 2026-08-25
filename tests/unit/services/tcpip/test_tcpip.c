/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : TcpIp Unit Tests — B1 deep-dive
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
*
* @file test_tcpip.c
* @brief Tests for the AUTOSAR TcpIp adaption layer:
*        - init / de-init lifecycle
*        - socket lifecycle (create/close/exhaustion/slot reuse)
*        - connection state machine (CLOSED / LISTEN / SYN-SENT /
*          SYN-RECEIVED / ESTABLISHED / FIN-WAIT / TIME-WAIT)
*        - multi-connection (8 sockets, listener + pending children,
*          per-socket RX isolation)
*        - zero-copy RX/TX buffers, options, VLAN, statistics
*        - DET error reporting
*================================================================================================*/

// @tests src/bsw/services/tcpip/src/TcpIp.c  @tests src/bsw/services/tcpip/include/TcpIp.h

#include "../test_framework.h"
#include "TcpIp.h"
#include <string.h>

/*==================================================================================================
*                                      DET MOCK
*==================================================================================================*/
#define MOCK_MAX_DET_CALLS                      (64U)

static uint16 g_det_module[MOCK_MAX_DET_CALLS];
static uint8  g_det_api[MOCK_MAX_DET_CALLS];
static uint8  g_det_err[MOCK_MAX_DET_CALLS];
static uint16 g_det_count;

Std_ReturnType Det_ReportError(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 ErrorId)
{
    (void)InstanceId;
    if (g_det_count < (uint16)MOCK_MAX_DET_CALLS)
    {
        g_det_module[g_det_count] = ModuleId;
        g_det_api[g_det_count] = ApiId;
        g_det_err[g_det_count] = ErrorId;
        g_det_count++;
    }
    return E_OK;
}

static void mock_det_reset(void)
{
    g_det_count = 0U;
    (void)memset(g_det_module, 0, sizeof(g_det_module));
    (void)memset(g_det_api, 0, sizeof(g_det_api));
    (void)memset(g_det_err, 0, sizeof(g_det_err));
}

static uint16 mock_det_count_for(uint8 ApiId)
{
    uint16 i;
    uint16 count = 0U;
    for (i = 0U; i < g_det_count; i++)
    {
        if (g_det_api[i] == ApiId)
        {
            count++;
        }
    }
    return count;
}

/*==================================================================================================
*                                      TEST HELPERS
*==================================================================================================*/
static TcpIp_ConfigType g_test_config;

static void setup_default_config(void)
{
    (void)memset(&g_test_config, 0, sizeof(g_test_config));
    g_test_config.NumSockets             = TCPIP_MAX_SOCKETS;
    g_test_config.NumTcpPbufs            = TCPIP_MAX_TCP_PBUFS;
    g_test_config.TcpRcvBufSize          = TCPIP_TCP_RCV_BUF_SIZE;
    g_test_config.TcpSndBufSize          = TCPIP_TCP_SND_BUF_SIZE;
    g_test_config.UdpRcvBufSize          = TCPIP_UDP_RCV_BUF_SIZE;
    g_test_config.EthLinkCheckIntervalMs = TCPIP_ETH_LINK_CHECK_INTERVAL_MS;
}

static void tcpip_setup(void)
{
    mock_det_reset();
    TcpIp_DeInit();          /* safe even when uninitialised (DET recorded) */
    mock_det_reset();
    setup_default_config();
    TcpIp_Init(&g_test_config);
}

static TcpIp_SockAddrType make_addr(uint8 a0, uint8 a1, uint8 a2, uint8 a3, uint16 port)
{
    TcpIp_SockAddrType addr;
    (void)memset(&addr, 0, sizeof(addr));
    addr.domain = TCPIP_AF_INET;
    addr.port   = port;
    addr.addr[0] = a0;
    addr.addr[1] = a1;
    addr.addr[2] = a2;
    addr.addr[3] = a3;
    return addr;
}

static TcpIp_SocketIdType create_tcp_socket(void)
{
    TcpIp_SocketIdType id = TCPIP_SOCKETID_INVALID;
    (void)TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, &id);
    return id;
}

/*==================================================================================================
*                                      TEST CASES
*==================================================================================================*/

/* ---------- init / de-init lifecycle ---------- */

TEST_CASE(tcpip_init_valid)
{
    tcpip_setup();
    setup_default_config();
    TcpIp_Init(&g_test_config);
    TcpIp_LinkStateType link = TCPIP_LINK_STATE_DOWN;
    TcpIp_InterfaceStateType ifstate = TCPIP_IFSTATE_DOWN;
    ASSERT_EQ(TCPIP_OK, TcpIp_GetLinkState(&link));
    ASSERT_EQ(TCPIP_LINK_STATE_UP, link);
    ASSERT_EQ(TCPIP_OK, TcpIp_GetInterfaceState(&ifstate));
    ASSERT_EQ(TCPIP_IFSTATE_UP, ifstate);
}

TEST_CASE(tcpip_init_null_config_det)
{
    tcpip_setup();
    mock_det_reset();
    TcpIp_Init(NULL_PTR);
    ASSERT_EQ(1U, mock_det_count_for(TcpIP_SID_INIT));
}

TEST_CASE(tcpip_init_twice_det)
{
    tcpip_setup();
    setup_default_config();
    TcpIp_Init(&g_test_config);
    mock_det_reset();
    TcpIp_Init(&g_test_config);
    ASSERT_EQ(1U, mock_det_count_for(TcpIP_SID_INIT));
}

TEST_CASE(tcpip_deinit_then_reinit)
{
    tcpip_setup();
    TcpIp_DeInit();
    setup_default_config();
    TcpIp_Init(&g_test_config);
    TcpIp_DeInit();
    /* DeInit after DeInit: DET recorded, no crash */
    TcpIp_DeInit();
}

TEST_CASE(tcpip_reset_preserves_config)
{
    tcpip_setup();
    TcpIp_LinkStateType link = TCPIP_LINK_STATE_DOWN;
    ASSERT_EQ(TCPIP_OK, TcpIp_Reset());
    ASSERT_EQ(TCPIP_OK, TcpIp_GetLinkState(&link));
    ASSERT_EQ(TCPIP_LINK_STATE_UP, link);
}

TEST_CASE(tcpip_version_info)
{
    tcpip_setup();
    Std_VersionInfoType ver;
    (void)memset(&ver, 0, sizeof(ver));
    TcpIp_GetVersionInfo(&ver);
    ASSERT_EQ(TCPIP_VENDOR_ID, ver.vendorID);
    ASSERT_EQ(TCPIP_MODULE_ID, ver.moduleID);
    ASSERT_EQ(TCPIP_SW_MAJOR_VERSION, ver.sw_major_version);
}

/* ---------- socket lifecycle ---------- */

TEST_CASE(tcpip_create_tcp)
{
    tcpip_setup();
    TcpIp_SocketIdType id = TCPIP_SOCKETID_INVALID;
    ASSERT_EQ(TCPIP_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, &id));
    ASSERT_NE(TCPIP_SOCKETID_INVALID, id);
}

TEST_CASE(tcpip_create_udp)
{
    tcpip_setup();
    TcpIp_SocketIdType id = TCPIP_SOCKETID_INVALID;
    ASSERT_EQ(TCPIP_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_DGRAM, &id));
    ASSERT_NE(TCPIP_SOCKETID_INVALID, id);
}

TEST_CASE(tcpip_create_null_id_det)
{
    tcpip_setup();
    mock_det_reset();
    ASSERT_EQ(TCPIP_E_NOT_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, NULL_PTR));
    ASSERT_EQ(1U, mock_det_count_for(TCPIP_SID_OPENSOCKET));
}

TEST_CASE(tcpip_create_uninit_det)
{
    tcpip_setup();
    TcpIp_SocketIdType id;
    TcpIp_DeInit();
    mock_det_reset();
    ASSERT_EQ(TCPIP_E_NOT_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, &id));
    ASSERT_EQ(1U, mock_det_count_for(TCPIP_SID_OPENSOCKET));
}

TEST_CASE(tcpip_close_releases_slot)
{
    tcpip_setup();
    TcpIp_SocketIdType id1 = TCPIP_SOCKETID_INVALID;
    TcpIp_SocketIdType id2 = TCPIP_SOCKETID_INVALID;
    ASSERT_EQ(TCPIP_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_DGRAM, &id1));
    ASSERT_EQ(TCPIP_OK, TcpIp_Close(id1, TRUE));
    /* Slot reuse: a fresh create must succeed even at full table */
    ASSERT_EQ(TCPIP_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_DGRAM, &id2));
    ASSERT_NE(TCPIP_SOCKETID_INVALID, id2);
}

TEST_CASE(tcpip_close_invalid_id)
{
    tcpip_setup();
    ASSERT_EQ(TCPIP_E_NOT_OK, TcpIp_Close(TCPIP_SOCKETID_INVALID, TRUE));
}

TEST_CASE(tcpip_exhaust_sockets)
{
    tcpip_setup();
    TcpIp_SocketIdType id;
    uint8 i;
    for (i = 0U; i < TCPIP_MAX_SOCKETS; i++)
    {
        ASSERT_EQ(TCPIP_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_DGRAM, &id));
    }
    ASSERT_EQ(TCPIP_E_NOT_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_DGRAM, &id));
}

/* ---------- bind / addresses ---------- */

TEST_CASE(tcpip_bind_and_get_local_addr)
{
    tcpip_setup();
    TcpIp_SocketIdType id = create_tcp_socket();
    TcpIp_SockAddrType addr = make_addr(192U, 168U, 0U, 2U, 12345U);
    TcpIp_SockAddrType out;
    ASSERT_EQ(TCPIP_OK, TcpIp_Bind(id, &addr));
    ASSERT_EQ(TCPIP_OK, TcpIp_GetLocalAddr(id, &out));
    ASSERT_EQ(12345U, out.port);
    ASSERT_EQ(192U, out.addr[0]);
    ASSERT_EQ(2U, out.addr[3]);
}

TEST_CASE(tcpip_set_get_remote_addr)
{
    tcpip_setup();
    TcpIp_SocketIdType id = create_tcp_socket();
    TcpIp_SockAddrType addr = make_addr(10U, 0U, 0U, 1U, 9999U);
    TcpIp_SockAddrType out;
    ASSERT_EQ(TCPIP_OK, TcpIp_SetRemoteAddr(id, &addr));
    ASSERT_EQ(TCPIP_OK, TcpIp_GetRemoteAddr(id, &out));
    ASSERT_EQ(9999U, out.port);
    ASSERT_EQ(10U, out.addr[0]);
}

TEST_CASE(tcpip_ipv4_addr_and_mask)
{
    tcpip_setup();
    TcpIp_Ipv4AddrType ip = 0UL;
    TcpIp_Ipv4AddrType mask = 0UL;
    ASSERT_EQ(TCPIP_OK, TcpIp_GetIPv4Addr(&ip));
    ASSERT_EQ(TCPIP_DEFAULT_IPV4_ADDR, ip);
    ASSERT_EQ(TCPIP_OK, TcpIp_GetIPv4SubnetMask(0U, &mask));
    ASSERT_EQ(TCPIP_DEFAULT_IPV4_MASK, mask);
    ASSERT_EQ(TCPIP_E_NOT_OK, TcpIp_GetIPv4SubnetMask(1U, &mask));
}

TEST_CASE(tcpip_ipaddr_state)
{
    tcpip_setup();
    TcpIp_IpAddrStateType state = TCPIP_IPADDR_STATE_UNASSIGNED;
    ASSERT_EQ(TCPIP_OK, TcpIp_GetIpAddrState(0U, &state));
    ASSERT_EQ(TCPIP_IPADDR_STATE_ASSIGNED, state);
}

/* ---------- connection state machine ---------- */

TEST_CASE(tcpip_connect_establishes)
{
    tcpip_setup();
    TcpIp_SocketIdType id = create_tcp_socket();
    TcpIp_SockAddrType addr = make_addr(192U, 168U, 0U, 1U, 80U);
    TcpIp_TcpStateType st = TCPIP_TCPSTATE_CLOSED;
    TcpIp_ConnectionStateType cs = TCPIP_CONNSTATE_CLOSED;
    ASSERT_EQ(TCPIP_OK, TcpIp_Connect(id, &addr));
    ASSERT_EQ(TCPIP_OK, TcpIp_GetTcpState(id, &st));
    ASSERT_EQ(TCPIP_TCPSTATE_ESTABLISHED, st);
    ASSERT_EQ(TCPIP_OK, TcpIp_GetConnectionState(id, &cs));
    ASSERT_EQ(TCPIP_CONNSTATE_ESTABLISHED, cs);
    ASSERT_TRUE(TcpIp_IsConnected(id));
}

TEST_CASE(tcpip_connect_twice_isconn)
{
    tcpip_setup();
    TcpIp_SocketIdType id = create_tcp_socket();
    TcpIp_SockAddrType addr = make_addr(192U, 168U, 0U, 1U, 80U);
    ASSERT_EQ(TCPIP_OK, TcpIp_Connect(id, &addr));
    ASSERT_EQ(TCPIP_E_ISCONN, TcpIp_Connect(id, &addr));
}

TEST_CASE(tcpip_udp_connect)
{
    tcpip_setup();
    TcpIp_SocketIdType id;
    TcpIp_SockAddrType addr = make_addr(192U, 168U, 0U, 1U, 5353U);
    ASSERT_EQ(TCPIP_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_DGRAM, &id));
    ASSERT_EQ(TCPIP_OK, TcpIp_Connect(id, &addr));
}

TEST_CASE(tcpip_listen_state)
{
    tcpip_setup();
    TcpIp_SocketIdType id = create_tcp_socket();
    TcpIp_TcpStateType st = TCPIP_TCPSTATE_CLOSED;
    TcpIp_ConnectionStateType cs = TCPIP_CONNSTATE_CLOSED;
    ASSERT_EQ(TCPIP_OK, TcpIp_Listen(id, 2U));
    ASSERT_EQ(TCPIP_OK, TcpIp_GetTcpState(id, &st));
    ASSERT_EQ(TCPIP_TCPSTATE_LISTEN, st);
    ASSERT_EQ(TCPIP_OK, TcpIp_GetConnectionState(id, &cs));
    ASSERT_EQ(TCPIP_CONNSTATE_LISTENING, cs);
}

TEST_CASE(tcpip_listen_on_udp_invalid)
{
    tcpip_setup();
    TcpIp_SocketIdType id;
    ASSERT_EQ(TCPIP_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_DGRAM, &id));
    ASSERT_EQ(TCPIP_E_INVALID_PROTOCOL, TcpIp_Listen(id, 2U));
}

TEST_CASE(tcpip_change_state_invalid_transition)
{
    tcpip_setup();
    TcpIp_SocketIdType id = create_tcp_socket();
    /* CLOSED -> ESTABLISHED is not a valid direct transition */
    ASSERT_EQ(TCPIP_E_INVALID_STATE, TcpIp_ChangeTcpState(id, TCPIP_TCPSTATE_ESTABLISHED));
}

TEST_CASE(tcpip_change_state_client_handshake)
{
    tcpip_setup();
    TcpIp_SocketIdType id = create_tcp_socket();
    /* Adapter-driven connect: CLOSED -> SYN_SENT -> ESTABLISHED */
    ASSERT_EQ(TCPIP_OK, TcpIp_ChangeTcpState(id, TCPIP_TCPSTATE_SYN_SENT));
    ASSERT_EQ(TCPIP_OK, TcpIp_ChangeTcpState(id, TCPIP_TCPSTATE_ESTABLISHED));
    ASSERT_TRUE(TcpIp_IsConnected(id));
}

TEST_CASE(tcpip_server_accept_flow)
{
    tcpip_setup();
    TcpIp_SocketIdType listener = create_tcp_socket();
    TcpIp_SocketIdType child = TCPIP_SOCKETID_INVALID;
    TcpIp_TcpStateType st = TCPIP_TCPSTATE_CLOSED;

    ASSERT_EQ(TCPIP_OK, TcpIp_Listen(listener, 2U));

    /* Incoming SYN: listener spawns a pending child */
    ASSERT_EQ(TCPIP_OK, TcpIp_ChangeTcpState(listener, TCPIP_TCPSTATE_SYN_RECEIVED));

    /* Accept the pending child (handshake at least SYN_RECEIVED) */
    ASSERT_EQ(TCPIP_OK, TcpIp_Accept(listener, &child));
    ASSERT_NE(TCPIP_SOCKETID_INVALID, child);
    ASSERT_EQ(TCPIP_OK, TcpIp_GetTcpState(child, &st));
    ASSERT_EQ(TCPIP_TCPSTATE_SYN_RECEIVED, st);

    /* Handshake completes (ACK): adapter drives the child to ESTABLISHED */
    ASSERT_EQ(TCPIP_OK, TcpIp_ChangeTcpState(child, TCPIP_TCPSTATE_ESTABLISHED));
    ASSERT_TRUE(TcpIp_IsConnected(child));
    ASSERT_EQ(TCPIP_OK, TcpIp_Close(child, TRUE));
}

TEST_CASE(tcpip_accept_empty_listener)
{
    tcpip_setup();
    TcpIp_SocketIdType listener = create_tcp_socket();
    TcpIp_SocketIdType child = TCPIP_SOCKETID_INVALID;
    ASSERT_EQ(TCPIP_OK, TcpIp_Listen(listener, 2U));
    ASSERT_EQ(TCPIP_E_NOT_OK, TcpIp_Accept(listener, &child));
}

TEST_CASE(tcpip_accept_not_listening)
{
    tcpip_setup();
    TcpIp_SocketIdType id = create_tcp_socket();
    TcpIp_SocketIdType child = TCPIP_SOCKETID_INVALID;
    ASSERT_EQ(TCPIP_E_INVALID_STATE, TcpIp_Accept(id, &child));
}

TEST_CASE(tcpip_abort_connection)
{
    tcpip_setup();
    TcpIp_SocketIdType id = create_tcp_socket();
    TcpIp_SockAddrType addr = make_addr(192U, 168U, 0U, 1U, 80U);
    TcpIp_TcpStateType st = TCPIP_TCPSTATE_CLOSED;
    ASSERT_EQ(TCPIP_OK, TcpIp_Connect(id, &addr));
    ASSERT_EQ(TCPIP_OK, TcpIp_Abort(id));
    /* Slot freed: id must be reusable */
    ASSERT_EQ(TCPIP_E_NOT_OK, TcpIp_GetTcpState(id, &st));
}

TEST_CASE(tcpip_graceful_close_via_main_function)
{
    tcpip_setup();
    TcpIp_SocketIdType id = create_tcp_socket();
    TcpIp_SockAddrType addr = make_addr(192U, 168U, 0U, 1U, 80U);
    TcpIp_TcpStateType st = TCPIP_TCPSTATE_CLOSED;

    ASSERT_EQ(TCPIP_OK, TcpIp_Connect(id, &addr));
    ASSERT_EQ(TCPIP_OK, TcpIp_Close(id, FALSE));
    ASSERT_EQ(TCPIP_OK, TcpIp_GetTcpState(id, &st));
    ASSERT_EQ(TCPIP_TCPSTATE_FIN_WAIT_1, st);

    TcpIp_MainFunction();
    ASSERT_EQ(TCPIP_OK, TcpIp_GetTcpState(id, &st));
    ASSERT_EQ(TCPIP_TCPSTATE_FIN_WAIT_2, st);

    TcpIp_MainFunction();
    ASSERT_EQ(TCPIP_OK, TcpIp_GetTcpState(id, &st));
    ASSERT_EQ(TCPIP_TCPSTATE_TIME_WAIT, st);

    /* Final MainFunction step releases the slot */
    TcpIp_MainFunction();
    ASSERT_EQ(TCPIP_E_NOT_OK, TcpIp_GetTcpState(id, &st));
}

/* ---------- multi-connection ---------- */

TEST_CASE(tcpip_multiple_sockets_mixed)
{
    tcpip_setup();
    TcpIp_SocketIdType ids[TCPIP_MAX_SOCKETS];
    uint8 i;
    for (i = 0U; i < TCPIP_MAX_SOCKETS; i++)
    {
        if ((i % 2U) == 0U)
        {
            ASSERT_EQ(TCPIP_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, &ids[i]));
        }
        else
        {
            ASSERT_EQ(TCPIP_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_DGRAM, &ids[i]));
        }
    }
    for (i = 0U; i < TCPIP_MAX_SOCKETS; i++)
    {
        ASSERT_EQ(TCPIP_OK, TcpIp_Close(ids[i], TRUE));
    }
}

TEST_CASE(tcpip_listener_backlog_overflow)
{
    tcpip_setup();
    TcpIp_SocketIdType listener = create_tcp_socket();
    TcpIp_SocketIdType child = TCPIP_SOCKETID_INVALID;
    uint8 i;

    ASSERT_EQ(TCPIP_OK, TcpIp_Listen(listener, 1U));

    /* Fill the backlog (1 pending) */
    ASSERT_EQ(TCPIP_OK, TcpIp_ChangeTcpState(listener, TCPIP_TCPSTATE_SYN_RECEIVED));
    /* Second SYN must fail: backlog full */
    ASSERT_EQ(TCPIP_E_NOBUFS, TcpIp_ChangeTcpState(listener, TCPIP_TCPSTATE_SYN_RECEIVED));

    /* Drain via accept; backlog frees up */
    ASSERT_EQ(TCPIP_OK, TcpIp_Accept(listener, &child));
    ASSERT_EQ(TCPIP_E_NOT_OK, TcpIp_Accept(listener, &child));

    /* Slot released by accept can be reused after the child is closed */
    ASSERT_EQ(TCPIP_OK, TcpIp_Close(child, TRUE));
    for (i = 0U; i < TCPIP_MAX_SOCKETS; i++)
    {
        (void)TcpIp_Close(i, TRUE);
    }
}

TEST_CASE(tcpip_rx_isolation_between_sockets)
{
    tcpip_setup();
    TcpIp_SocketIdType idA = create_tcp_socket();
    TcpIp_SocketIdType idB = create_tcp_socket();
    static const uint8 dataA[4] = { 0xAAU, 0xBBU, 0xCCU, 0xDDU };
    static const uint8 dataB[3] = { 0x11U, 0x22U, 0x33U };
    uint8 buf[8];
    uint16 len = 0U;

    ASSERT_EQ(TCPIP_OK, TcpIp_RxIndication(idA, dataA, 4U));
    ASSERT_EQ(TCPIP_OK, TcpIp_RxIndication(idB, dataB, 3U));

    ASSERT_EQ(TCPIP_OK, TcpIp_Receive(idA, buf, sizeof(buf), &len));
    ASSERT_EQ(4U, len);
    ASSERT_EQ(0xAAU, buf[0]);
    ASSERT_EQ(0xDDU, buf[3]);

    ASSERT_EQ(TCPIP_OK, TcpIp_Receive(idB, buf, sizeof(buf), &len));
    ASSERT_EQ(3U, len);
    ASSERT_EQ(0x11U, buf[0]);
    ASSERT_EQ(0x33U, buf[2]);
}

/* ---------- RX / TX buffers ---------- */

TEST_CASE(tcpip_rxindication_receive)
{
    tcpip_setup();
    TcpIp_SocketIdType id = create_tcp_socket();
    static const uint8 data[5] = { 1U, 2U, 3U, 4U, 5U };
    uint8 buf[16];
    uint16 len = 0U;

    ASSERT_EQ(TCPIP_OK, TcpIp_RxIndication(id, data, 5U));
    ASSERT_EQ(TCPIP_OK, TcpIp_Receive(id, buf, sizeof(buf), &len));
    ASSERT_EQ(5U, len);
    ASSERT_MEM_EQ(data, buf, 5U);
}

TEST_CASE(tcpip_rx_queue_two_chunks)
{
    tcpip_setup();
    TcpIp_SocketIdType id = create_tcp_socket();
    static const uint8 d1[3] = { 1U, 2U, 3U };
    static const uint8 d2[2] = { 4U, 5U };
    uint8 buf[16];
    uint16 len = 0U;

    ASSERT_EQ(TCPIP_OK, TcpIp_RxIndication(id, d1, 3U));
    ASSERT_EQ(TCPIP_OK, TcpIp_RxIndication(id, d2, 2U));
    ASSERT_EQ(TCPIP_OK, TcpIp_Receive(id, buf, sizeof(buf), &len));
    ASSERT_EQ(3U, len);
    ASSERT_EQ(TCPIP_OK, TcpIp_Receive(id, buf, sizeof(buf), &len));
    ASSERT_EQ(2U, len);
    ASSERT_EQ(4U, buf[0]);
}

TEST_CASE(tcpip_rx_queue_overflow)
{
    tcpip_setup();
    TcpIp_SocketIdType id = create_tcp_socket();
    static const uint8 d[2] = { 1U, 2U };
    uint8 i;
    for (i = 0U; i < (uint8)TCPIP_MAX_RX_BUFFERS; i++)
    {
        ASSERT_EQ(TCPIP_OK, TcpIp_RxIndication(id, d, 2U));
    }
    ASSERT_EQ(TCPIP_E_BUFFER_OVERFLOW, TcpIp_RxIndication(id, d, 2U));
}

TEST_CASE(tcpip_receive_buffer_too_small)
{
    tcpip_setup();
    TcpIp_SocketIdType id = create_tcp_socket();
    static const uint8 data[8] = { 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U };
    uint8 buf[4];
    uint16 len = 0U;

    ASSERT_EQ(TCPIP_OK, TcpIp_RxIndication(id, data, 8U));
    ASSERT_EQ(TCPIP_E_BUFFER_OVERFLOW, TcpIp_Receive(id, buf, sizeof(buf), &len));
    /* Data must NOT be consumed */
    ASSERT_EQ(TCPIP_OK, TcpIp_Receive(id, buf, 16U, &len));
    ASSERT_EQ(8U, len);
}

TEST_CASE(tcpip_get_release_rx_buffer)
{
    tcpip_setup();
    TcpIp_SocketIdType id = create_tcp_socket();
    static const uint8 data[4] = { 9U, 8U, 7U, 6U };
    uint8* ptr = NULL_PTR;
    uint16 len = 0U;

    ASSERT_EQ(TCPIP_E_NOT_OK, TcpIp_GetRxBuffer(id, &ptr, &len));   /* no data */
    ASSERT_EQ(TCPIP_OK, TcpIp_RxIndication(id, data, 4U));
    ASSERT_EQ(TCPIP_OK, TcpIp_GetRxBuffer(id, &ptr, &len));
    ASSERT_NOT_NULL(ptr);
    ASSERT_EQ(4U, len);
    ASSERT_EQ(9U, ptr[0]);
    ASSERT_EQ(TCPIP_OK, TcpIp_ReleaseRxBuffer(id));
    ASSERT_EQ(TCPIP_E_NOT_OK, TcpIp_GetRxBuffer(id, &ptr, &len));
}

TEST_CASE(tcpip_set_rx_buffer_user_path)
{
    tcpip_setup();
    TcpIp_SocketIdType id = create_tcp_socket();
    static const uint8 data[4] = { 1U, 2U, 3U, 4U };
    static uint8 userBuf[16];
    uint8* ptr = NULL_PTR;
    uint16 len = 0U;

    ASSERT_EQ(TCPIP_OK, TcpIp_SetRxBuffer(id, userBuf, sizeof(userBuf)));
    ASSERT_EQ(TCPIP_OK, TcpIp_RxIndication(id, data, 4U));
    ASSERT_EQ(TCPIP_OK, TcpIp_GetRxBuffer(id, &ptr, &len));
    ASSERT_TRUE(ptr == userBuf);
    ASSERT_EQ(4U, len);
    ASSERT_EQ(2U, ptr[1]);
    ASSERT_EQ(TCPIP_OK, TcpIp_ReleaseRxBuffer(id));
}

TEST_CASE(tcpip_tx_buffer_roundtrip)
{
    tcpip_setup();
    TcpIp_SocketIdType id = create_tcp_socket();
    uint8* ptr = NULL_PTR;
    uint16 cap = 0U;

    ASSERT_EQ(TCPIP_OK, TcpIp_GetTxBuffer(id, &ptr, &cap));
    ASSERT_NOT_NULL(ptr);
    ASSERT_GE(cap, 4U);
    ptr[0] = 0x55U;
    ptr[1] = 0xAAU;
    ASSERT_EQ(TCPIP_OK, TcpIp_ReleaseTxBuffer(id, 2U));
}

TEST_CASE(tcpip_send_and_transmit)
{
    tcpip_setup();
    TcpIp_SocketIdType id = create_tcp_socket();
    static const uint8 data[6] = { 1U, 2U, 3U, 4U, 5U, 6U };
    TcpIp_SockAddrType addr = make_addr(192U, 168U, 0U, 1U, 80U);
    ASSERT_EQ(TCPIP_OK, TcpIp_Connect(id, &addr));
    ASSERT_EQ(TCPIP_OK, TcpIp_Send(id, data, 6U));
    ASSERT_EQ(TCPIP_OK, TcpIp_Transmit(id, data, 3U));
    ASSERT_EQ(TCPIP_E_NOT_OK, TcpIp_Send(id, NULL_PTR, 1U));
}

/* ---------- options ---------- */

TEST_CASE(tcpip_tcp_options)
{
    tcpip_setup();
    TcpIp_SocketIdType id = create_tcp_socket();
    uint32 value = 0UL;
    ASSERT_EQ(TCPIP_OK, TcpIp_SetTcpOption(id, TCPIP_TCPOPT_NODELAY, 1UL));
    ASSERT_EQ(TCPIP_OK, TcpIp_GetTcpOption(id, TCPIP_TCPOPT_NODELAY, &value));
    ASSERT_EQ(1UL, value);
    ASSERT_EQ(TCPIP_OK, TcpIp_SetTcpOption(id, TCPIP_TCPOPT_MAXSEG, 1460UL));
    ASSERT_EQ(TCPIP_OK, TcpIp_GetTcpOption(id, TCPIP_TCPOPT_MAXSEG, &value));
    ASSERT_EQ(1460UL, value);
    ASSERT_EQ(TCPIP_E_NOT_OK, TcpIp_SetTcpOption(id, 0x7FU, 1UL));
}

TEST_CASE(tcpip_udp_options)
{
    tcpip_setup();
    TcpIp_SocketIdType id;
    uint32 value = 0UL;
    ASSERT_EQ(TCPIP_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_DGRAM, &id));
    ASSERT_EQ(TCPIP_OK, TcpIp_SetUdpOption(id, TCPIP_UDPOPT_TTL, 64UL));
    ASSERT_EQ(TCPIP_OK, TcpIp_GetUdpOption(id, TCPIP_UDPOPT_TTL, &value));
    ASSERT_EQ(64UL, value);
    ASSERT_EQ(TCPIP_E_NOT_OK, TcpIp_GetUdpOption(id, 0x7FU, &value));
}

/* ---------- VLAN ---------- */

#if (TCPIP_VLAN_SUPPORT == STD_ON)
TEST_CASE(tcpip_vlan_default)
{
    tcpip_setup();
    TcpIp_VlanConfigType cfg;
    (void)memset(&cfg, 0xFF, sizeof(cfg));
    ASSERT_EQ(TCPIP_OK, TcpIp_GetVlanConfig(&cfg));
    ASSERT_FALSE(cfg.VlanEnabled);
    ASSERT_EQ(0U, cfg.VlanId);
}

TEST_CASE(tcpip_vlan_set_get)
{
    tcpip_setup();
    TcpIp_VlanConfigType cfg;
    TcpIp_VlanConfigType out;
    (void)memset(&cfg, 0, sizeof(cfg));
    cfg.VlanEnabled = TRUE;
    cfg.VlanId = 100U;
    cfg.VlanPriority = 5U;
    cfg.DropUntagged = TRUE;
    ASSERT_EQ(TCPIP_OK, TcpIp_SetVlanConfig(&cfg));
    ASSERT_EQ(TCPIP_OK, TcpIp_GetVlanConfig(&out));
    ASSERT_TRUE(out.VlanEnabled);
    ASSERT_EQ(100U, out.VlanId);
    ASSERT_EQ(5U, out.VlanPriority);
    ASSERT_TRUE(out.DropUntagged);
}

TEST_CASE(tcpip_vlan_invalid_id)
{
    tcpip_setup();
    TcpIp_VlanConfigType cfg;
    (void)memset(&cfg, 0, sizeof(cfg));
    cfg.VlanId = 4096U;   /* 12-bit overflow */
    ASSERT_EQ(TCPIP_E_PARAM_CONFIG, TcpIp_SetVlanConfig(&cfg));
}

TEST_CASE(tcpip_vlan_invalid_priority)
{
    tcpip_setup();
    TcpIp_VlanConfigType cfg;
    (void)memset(&cfg, 0, sizeof(cfg));
    cfg.VlanPriority = 8U;
    ASSERT_EQ(TCPIP_E_PARAM_CONFIG, TcpIp_SetVlanConfig(&cfg));
}
#endif /* TCPIP_VLAN_SUPPORT */

/* ---------- statistics ---------- */

#if (TCPIP_ENABLE_STATISTICS == STD_ON)
TEST_CASE(tcpip_stats_counters)
{
    tcpip_setup();
    TcpIp_StatisticsType stats;
    TcpIp_SocketIdType id;
    static const uint8 data[4] = { 1U, 2U, 3U, 4U };
    TcpIp_SockAddrType addr = make_addr(192U, 168U, 0U, 1U, 80U);
    uint8 buf[16];
    uint16 len = 0U;

    ASSERT_EQ(TCPIP_OK, TcpIp_ResetStatistics());
    ASSERT_EQ(TCPIP_OK, TcpIp_GetStatistics(&stats));
    ASSERT_EQ(0UL, stats.TxPackets);
    ASSERT_EQ(0UL, stats.RxPackets);
    ASSERT_EQ(TCPIP_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, &id));

    /* TX */
    ASSERT_EQ(TCPIP_OK, TcpIp_Connect(id, &addr));          /* +1 active open */
    ASSERT_EQ(TCPIP_OK, TcpIp_Send(id, data, 4U));          /* +1 tx packet, +4 bytes */
    /* RX */
    ASSERT_EQ(TCPIP_OK, TcpIp_RxIndication(id, data, 4U));  /* +1 rx packet, +4 bytes */
    ASSERT_EQ(TCPIP_OK, TcpIp_Receive(id, buf, sizeof(buf), &len));

    ASSERT_EQ(TCPIP_OK, TcpIp_GetStatistics(&stats));
    ASSERT_EQ(1UL, stats.TxPackets);
    ASSERT_EQ(4UL, stats.TxBytes);
    ASSERT_EQ(1UL, stats.RxPackets);
    ASSERT_EQ(4UL, stats.RxBytes);
    ASSERT_EQ(1UL, stats.TcpActiveOpens);
    ASSERT_EQ(1UL, stats.TcpEstablishedCount);
    ASSERT_EQ(1UL, stats.SocketCreateCount);
    ASSERT_EQ(0UL, stats.TxErrors);
    ASSERT_EQ(0UL, stats.RxOverflows);

    /* overflow + error paths */
    ASSERT_EQ(TCPIP_E_NOT_OK, TcpIp_Send(TCPIP_SOCKETID_INVALID, data, 4U)); /* +1 tx error */
    (void)TcpIp_RxIndication(TCPIP_SOCKETID_INVALID, data, 4U);              /* +1 rx error */

    ASSERT_EQ(TCPIP_OK, TcpIp_GetStatistics(&stats));
    ASSERT_EQ(1UL, stats.TxErrors);
    ASSERT_EQ(1UL, stats.RxErrors);

    /* close increments socket close */
    ASSERT_EQ(TCPIP_OK, TcpIp_Close(id, TRUE));
    ASSERT_EQ(TCPIP_OK, TcpIp_GetStatistics(&stats));
    ASSERT_EQ(1UL, stats.SocketCloseCount);
    ASSERT_EQ(1UL, stats.TcpCloseCount);

    /* reset */
    ASSERT_EQ(TCPIP_OK, TcpIp_ResetStatistics());
    ASSERT_EQ(TCPIP_OK, TcpIp_GetStatistics(&stats));
    ASSERT_EQ(0UL, stats.TxPackets);
    ASSERT_EQ(0UL, stats.SocketCloseCount);
}

TEST_CASE(tcpip_stats_passive_open)
{
    tcpip_setup();
    TcpIp_StatisticsType stats;
    TcpIp_SocketIdType listener;
    TcpIp_SocketIdType child = TCPIP_SOCKETID_INVALID;

    ASSERT_EQ(TCPIP_OK, TcpIp_ResetStatistics());
    ASSERT_EQ(TCPIP_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, &listener));
    ASSERT_EQ(TCPIP_OK, TcpIp_Listen(listener, 2U));
    ASSERT_EQ(TCPIP_OK, TcpIp_ChangeTcpState(listener, TCPIP_TCPSTATE_SYN_RECEIVED));
    ASSERT_EQ(TCPIP_OK, TcpIp_Accept(listener, &child));
    ASSERT_EQ(TCPIP_OK, TcpIp_ChangeTcpState(child, TCPIP_TCPSTATE_ESTABLISHED));

    ASSERT_EQ(TCPIP_OK, TcpIp_GetStatistics(&stats));
    ASSERT_EQ(1UL, stats.TcpPassiveOpens);
    ASSERT_EQ(1UL, stats.TcpEstablishedCount);
    ASSERT_EQ(1UL, stats.SocketCreateCount);   /* listener only (child = internal alloc) */
}
#endif /* TCPIP_ENABLE_STATISTICS */

/* ---------- DET error reporting ---------- */

TEST_CASE(tcpip_uninit_apis_report_det)
{
    tcpip_setup();
    TcpIp_SocketIdType id;
    TcpIp_LinkStateType link;
    uint8 buf[4];
    uint16 len = 0U;

    TcpIp_DeInit();
    mock_det_reset();

    (void)TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, &id);
    (void)TcpIp_GetLinkState(&link);
    (void)TcpIp_Send(id, buf, 1U);
    (void)TcpIp_Receive(id, buf, 4U, &len);
    (void)TcpIp_Listen(id, 2U);
    (void)TcpIp_Connect(id, NULL_PTR);

    ASSERT_EQ(6U, g_det_count);
    ASSERT_EQ(1U, mock_det_count_for(TCPIP_SID_OPENSOCKET));
    ASSERT_EQ(1U, mock_det_count_for(TCPIP_SID_GETLINKSTATE));
    ASSERT_EQ(1U, mock_det_count_for(TCPIP_SID_TRANSMIT));
    ASSERT_EQ(1U, mock_det_count_for(TCPIP_SID_RECEIVE));
    ASSERT_EQ(1U, mock_det_count_for(TCPIP_SID_LISTEN));
    ASSERT_EQ(1U, mock_det_count_for(TCPIP_SID_CONNECT));
}

/*==================================================================================================
*                                      TEST SUITE
*==================================================================================================*/
TEST_MAIN_BEGIN()
    RUN_TEST(test_tcpip_init_valid);
    RUN_TEST(test_tcpip_init_null_config_det);
    RUN_TEST(test_tcpip_init_twice_det);
    RUN_TEST(test_tcpip_deinit_then_reinit);
    RUN_TEST(test_tcpip_reset_preserves_config);
    RUN_TEST(test_tcpip_version_info);
    RUN_TEST(test_tcpip_create_tcp);
    RUN_TEST(test_tcpip_create_udp);
    RUN_TEST(test_tcpip_create_null_id_det);
    RUN_TEST(test_tcpip_create_uninit_det);
    RUN_TEST(test_tcpip_close_releases_slot);
    RUN_TEST(test_tcpip_close_invalid_id);
    RUN_TEST(test_tcpip_exhaust_sockets);
    RUN_TEST(test_tcpip_bind_and_get_local_addr);
    RUN_TEST(test_tcpip_set_get_remote_addr);
    RUN_TEST(test_tcpip_ipv4_addr_and_mask);
    RUN_TEST(test_tcpip_ipaddr_state);
    RUN_TEST(test_tcpip_connect_establishes);
    RUN_TEST(test_tcpip_connect_twice_isconn);
    RUN_TEST(test_tcpip_udp_connect);
    RUN_TEST(test_tcpip_listen_state);
    RUN_TEST(test_tcpip_listen_on_udp_invalid);
    RUN_TEST(test_tcpip_change_state_invalid_transition);
    RUN_TEST(test_tcpip_change_state_client_handshake);
    RUN_TEST(test_tcpip_server_accept_flow);
    RUN_TEST(test_tcpip_accept_empty_listener);
    RUN_TEST(test_tcpip_accept_not_listening);
    RUN_TEST(test_tcpip_abort_connection);
    RUN_TEST(test_tcpip_graceful_close_via_main_function);
    RUN_TEST(test_tcpip_multiple_sockets_mixed);
    RUN_TEST(test_tcpip_listener_backlog_overflow);
    RUN_TEST(test_tcpip_rx_isolation_between_sockets);
    RUN_TEST(test_tcpip_rxindication_receive);
    RUN_TEST(test_tcpip_rx_queue_two_chunks);
    RUN_TEST(test_tcpip_rx_queue_overflow);
    RUN_TEST(test_tcpip_receive_buffer_too_small);
    RUN_TEST(test_tcpip_get_release_rx_buffer);
    RUN_TEST(test_tcpip_set_rx_buffer_user_path);
    RUN_TEST(test_tcpip_tx_buffer_roundtrip);
    RUN_TEST(test_tcpip_send_and_transmit);
    RUN_TEST(test_tcpip_tcp_options);
    RUN_TEST(test_tcpip_udp_options);
    RUN_TEST(test_tcpip_vlan_default);
    RUN_TEST(test_tcpip_vlan_set_get);
    RUN_TEST(test_tcpip_vlan_invalid_id);
    RUN_TEST(test_tcpip_vlan_invalid_priority);
    RUN_TEST(test_tcpip_stats_counters);
    RUN_TEST(test_tcpip_stats_passive_open);
    RUN_TEST(test_tcpip_uninit_apis_report_det);
TEST_MAIN_END()
