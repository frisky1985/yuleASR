/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Module               : TCP/IP Stack Unit Tests
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#include "../test_framework.h"
#include "TcpIp.h"

/*==================================================================================================
*                                      TEST GLOBALS
==================================================================================================*/
static TcpIp_ConfigType g_test_config;

static void setup_default_config(void)
{
    g_test_config.NumSockets            = 8;
    g_test_config.NumTcpPbufs           = 16;
    g_test_config.TcpRcvBufSize         = 4096;
    g_test_config.TcpSndBufSize         = 4096;
    g_test_config.UdpRcvBufSize         = 2048;
    g_test_config.EthLinkCheckIntervalMs = 100;
}

/*==================================================================================================
*                                      TEST CASES
==================================================================================================*/

TEST_CASE(tcpip_init_valid_config)
{
    setup_default_config();
    TcpIp_Init(&g_test_config);
    /* No assert — smoke test */
}

TEST_CASE(tcpip_init_null_config)
{
    TcpIp_Init(NULL_PTR);
    /* Should not crash, DET should report */
}

TEST_CASE(tcpip_init_twice)
{
    setup_default_config();
    TcpIp_Init(&g_test_config);
    TcpIp_Init(&g_test_config);  /* Should report already-init */
}

TEST_CASE(tcpip_deinit)
{
    setup_default_config();
    TcpIp_Init(&g_test_config);
    TcpIp_DeInit();
}

TEST_CASE(tcpip_deinit_uninit)
{
    TcpIp_DeInit();  /* Should not crash */
}

TEST_CASE(tcpip_create_tcp_socket)
{
    TcpIp_SocketIdType sockId;
    setup_default_config();
    TcpIp_Init(&g_test_config);

    ASSERT_EQ(TCPIP_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, &sockId));
    ASSERT_NE(TCPIP_SOCKETID_INVALID, sockId);
}

TEST_CASE(tcpip_create_udp_socket)
{
    TcpIp_SocketIdType sockId;
    setup_default_config();
    TcpIp_Init(&g_test_config);

    ASSERT_EQ(TCPIP_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_DGRAM, &sockId));
    ASSERT_NE(TCPIP_SOCKETID_INVALID, sockId);
}

TEST_CASE(tcpip_create_null_id)
{
    setup_default_config();
    TcpIp_Init(&g_test_config);
    ASSERT_EQ(TCPIP_E_NOT_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, NULL_PTR));
}

TEST_CASE(tcpip_create_uninit)
{
    TcpIp_SocketIdType sockId;
    ASSERT_EQ(TCPIP_E_NOT_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, &sockId));
}

TEST_CASE(tcpip_close_socket)
{
    TcpIp_SocketIdType sockId;
    setup_default_config();
    TcpIp_Init(&g_test_config);
    TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, &sockId);
    ASSERT_EQ(TCPIP_OK, TcpIp_Close(sockId, TRUE));
}

TEST_CASE(tcpip_close_invalid)
{
    setup_default_config();
    TcpIp_Init(&g_test_config);
    ASSERT_EQ(TCPIP_E_NOT_OK, TcpIp_Close(TCPIP_SOCKETID_INVALID, TRUE));
}

TEST_CASE(tcpip_bind)
{
    TcpIp_SocketIdType sockId;
    TcpIp_SockAddrType addr;
    setup_default_config();
    TcpIp_Init(&g_test_config);
    TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_DGRAM, &sockId);

    addr.domain = TCPIP_AF_INET;
    addr.port   = 30490;
    addr.addr[0] = 0; addr.addr[1] = 0; addr.addr[2] = 0; addr.addr[3] = 0;

    ASSERT_EQ(TCPIP_OK, TcpIp_Bind(sockId, &addr));
}

TEST_CASE(tcpip_bind_null)
{
    TcpIp_SocketIdType sockId;
    setup_default_config();
    TcpIp_Init(&g_test_config);
    TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_DGRAM, &sockId);

    ASSERT_EQ(TCPIP_E_NOT_OK, TcpIp_Bind(sockId, NULL_PTR));
}

TEST_CASE(tcpip_send)
{
    TcpIp_SocketIdType sockId;
    uint8 data[] = {0x01, 0x02, 0x03, 0x04};
    setup_default_config();
    TcpIp_Init(&g_test_config);
    TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, &sockId);

    ASSERT_EQ(TCPIP_OK, TcpIp_Send(sockId, data, sizeof(data)));
}

TEST_CASE(tcpip_transmit)
{
    TcpIp_SocketIdType sockId;
    uint8 data[] = {0x01, 0x02, 0x03, 0x04};
    setup_default_config();
    TcpIp_Init(&g_test_config);
    TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, &sockId);

    ASSERT_EQ(TCPIP_OK, TcpIp_Transmit(sockId, data, sizeof(data)));
}

TEST_CASE(tcpip_send_null)
{
    TcpIp_SocketIdType sockId;
    setup_default_config();
    TcpIp_Init(&g_test_config);
    TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, &sockId);

    ASSERT_EQ(TCPIP_E_NOT_OK, TcpIp_Send(sockId, NULL_PTR, 0));
}

TEST_CASE(tcpip_receive)
{
    TcpIp_SocketIdType sockId;
    uint8 buf[64];
    uint16 recvLen = 0;
    setup_default_config();
    TcpIp_Init(&g_test_config);
    TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, &sockId);

    ASSERT_EQ(TCPIP_OK, TcpIp_Receive(sockId, buf, sizeof(buf), &recvLen));
}

TEST_CASE(tcpip_receive_null_buf)
{
    TcpIp_SocketIdType sockId;
    uint16 recvLen = 0;
    setup_default_config();
    TcpIp_Init(&g_test_config);
    TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, &sockId);

    ASSERT_EQ(TCPIP_E_NOT_OK, TcpIp_Receive(sockId, NULL_PTR, 0, &recvLen));
}

TEST_CASE(tcpip_open_socket_tcp)
{
    TcpIp_SocketIdType sockId;
    setup_default_config();
    TcpIp_Init(&g_test_config);

    ASSERT_EQ(TCPIP_OK, TcpIp_OpenSocket(TCPIP_IPPROTO_TCP, 30490, &sockId));
    ASSERT_NE(TCPIP_SOCKETID_INVALID, sockId);
}

TEST_CASE(tcpip_open_socket_udp)
{
    TcpIp_SocketIdType sockId;
    setup_default_config();
    TcpIp_Init(&g_test_config);

    ASSERT_EQ(TCPIP_OK, TcpIp_OpenSocket(TCPIP_IPPROTO_UDP, 30491, &sockId));
    ASSERT_NE(TCPIP_SOCKETID_INVALID, sockId);
}

TEST_CASE(tcpip_open_socket_invalid_proto)
{
    TcpIp_SocketIdType sockId;
    setup_default_config();
    TcpIp_Init(&g_test_config);

    ASSERT_EQ(TCPIP_E_NOT_OK, TcpIp_OpenSocket(0xFF, 30490, &sockId));
}

TEST_CASE(tcpip_close_socket_api)
{
    TcpIp_SocketIdType sockId;
    setup_default_config();
    TcpIp_Init(&g_test_config);
    TcpIp_OpenSocket(TCPIP_IPPROTO_TCP, 30490, &sockId);
    ASSERT_EQ(TCPIP_OK, TcpIp_CloseSocket(sockId));
}

TEST_CASE(tcpip_get_ipv4_addr)
{
    TcpIp_Ipv4AddrType addr;
    setup_default_config();
    TcpIp_Init(&g_test_config);
    ASSERT_EQ(TCPIP_OK, TcpIp_GetIPv4Addr(&addr));
    ASSERT_EQ(TCPIP_DEFAULT_IPV4_ADDR, addr);
}

TEST_CASE(tcpip_get_ipv4_addr_null)
{
    setup_default_config();
    TcpIp_Init(&g_test_config);
    ASSERT_EQ(TCPIP_E_NOT_OK, TcpIp_GetIPv4Addr(NULL_PTR));
}

TEST_CASE(tcpip_get_ipv6_addr)
{
    TcpIp_Ipv6AddrType addr;
    setup_default_config();
    TcpIp_Init(&g_test_config);
    ASSERT_EQ(TCPIP_E_NOT_SUPPORTED, TcpIp_GetIPv6Addr(&addr));
}

TEST_CASE(tcpip_get_link_state)
{
    TcpIp_LinkStateType state;
    setup_default_config();
    TcpIp_Init(&g_test_config);
    ASSERT_EQ(TCPIP_OK, TcpIp_GetLinkState(&state));
}

TEST_CASE(tcpip_get_link_state_null)
{
    setup_default_config();
    TcpIp_Init(&g_test_config);
    ASSERT_EQ(TCPIP_E_NOT_OK, TcpIp_GetLinkState(NULL_PTR));
}

TEST_CASE(tcpip_reset)
{
    setup_default_config();
    TcpIp_Init(&g_test_config);
    ASSERT_EQ(TCPIP_OK, TcpIp_Reset());
}

TEST_CASE(tcpip_main_function)
{
    setup_default_config();
    TcpIp_Init(&g_test_config);
    TcpIp_MainFunction();
    /* Smoke test — should not crash */
}

TEST_CASE(tcpip_main_function_uninit)
{
    TcpIp_MainFunction();
    /* Should not crash */
}

TEST_CASE(tcpip_multiple_sockets)
{
    TcpIp_SocketIdType sockets[4];
    uint8 i;
    setup_default_config();
    TcpIp_Init(&g_test_config);

    for (i = 0U; i < 4U; i++)
    {
        ASSERT_EQ(TCPIP_OK, TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_DGRAM, &sockets[i]));
        ASSERT_NE(TCPIP_SOCKETID_INVALID, sockets[i]);
    }

    for (i = 0U; i < 4U; i++)
    {
        ASSERT_EQ(TCPIP_OK, TcpIp_Close(sockets[i], TRUE));
    }
}

TEST_CASE(tcpip_exhaust_sockets)
{
    TcpIp_SocketIdType sockId;
    uint8 i;
    TcpIp_ReturnType ret;
    setup_default_config();
    TcpIp_Init(&g_test_config);

    /* Create max sockets */
    for (i = 0U; i < TCPIP_MAX_SOCKETS; i++)
    {
        ret = TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, &sockId);
        ASSERT_EQ(TCPIP_OK, ret);
    }

    /* Next create should fail */
    ret = TcpIp_Create(TCPIP_AF_INET, TCPIP_SOCK_STREAM, &sockId);
    ASSERT_EQ(TCPIP_E_NOT_OK, ret);
}

TEST_CASE(tcpip_get_version_info)
{
    Std_VersionInfoType ver;
    setup_default_config();
    TcpIp_Init(&g_test_config);
    TcpIp_GetVersionInfo(&ver);
    ASSERT_EQ(TCPIP_VENDOR_ID, ver.vendorID);
    ASSERT_EQ(TCPIP_MODULE_ID, ver.moduleID);
}

/*==================================================================================================
*                                      TEST SUITE
==================================================================================================*/
TEST_SUITE_SETUP(tcpip) { }

TEST_SUITE_TEARDOWN(tcpip) { }

TEST_SUITE(tcpip)
{
    RUN_TEST(tcpip_init_valid_config);
    RUN_TEST(tcpip_init_null_config);
    RUN_TEST(tcpip_init_twice);
    RUN_TEST(tcpip_deinit);
    RUN_TEST(tcpip_deinit_uninit);
    RUN_TEST(tcpip_create_tcp_socket);
    RUN_TEST(tcpip_create_udp_socket);
    RUN_TEST(tcpip_create_null_id);
    RUN_TEST(tcpip_create_uninit);
    RUN_TEST(tcpip_close_socket);
    RUN_TEST(tcpip_close_invalid);
    RUN_TEST(tcpip_bind);
    RUN_TEST(tcpip_bind_null);
    RUN_TEST(tcpip_send);
    RUN_TEST(tcpip_transmit);
    RUN_TEST(tcpip_send_null);
    RUN_TEST(tcpip_receive);
    RUN_TEST(tcpip_receive_null_buf);
    RUN_TEST(tcpip_open_socket_tcp);
    RUN_TEST(tcpip_open_socket_udp);
    RUN_TEST(tcpip_open_socket_invalid_proto);
    RUN_TEST(tcpip_close_socket_api);
    RUN_TEST(tcpip_get_ipv4_addr);
    RUN_TEST(tcpip_get_ipv4_addr_null);
    RUN_TEST(tcpip_get_ipv6_addr);
    RUN_TEST(tcpip_get_link_state);
    RUN_TEST(tcpip_get_link_state_null);
    RUN_TEST(tcpip_reset);
    RUN_TEST(tcpip_main_function);
    RUN_TEST(tcpip_main_function_uninit);
    RUN_TEST(tcpip_multiple_sockets);
    RUN_TEST(tcpip_exhaust_sockets);
    RUN_TEST(tcpip_get_version_info);
}

/*==================================================================================================
*                                      MAIN
==================================================================================================*/
TEST_MAIN_BEGIN()
    RUN_TEST_SUITE(tcpip);
TEST_MAIN_END()
