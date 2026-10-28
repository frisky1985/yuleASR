/**
 * @file test_dds_eth_transport.c
 * @brief DDS RTPS over ETH 传输层单元测试 (tests/unit/ddstrans)
 *
 * 被测: src/middleware/dds/transport/eth/dds_eth_transport.c (真实源码)
 * 桩:   tcpip_* 7 个函数按 dds_tcpip_compat.h 签名在文件内实现;
 *       tcpip_types.h 命中真实 src/tcpip/tcpip_types.h (仅类型, 自包含);
 *       eth_manager.h 命中最小桩 (被测代码未使用其符号)。
 */
#include <string.h>
#include "unity.h"
#include "dds_eth_transport.h"
#include "dds_tcpip_compat.h"

/* ============================================================================
 * tcpip 桩 — 签名与 dds_tcpip_compat.h 完全一致
 * ============================================================================ */
#define STUB_LOG_MAX   16
#define STUB_RXQ_MAX   8
#define STUB_SEND_MAX  64

typedef struct {
    tcpip_socket_id_t sock;
    tcpip_sockaddr_t  src;
    uint16_t          len;
    uint8_t           data[STUB_SEND_MAX];
} stub_rx_pkt_t;

static int               g_tc_create_fail;    /* 全部失败开关 */
static int               g_tc_create_fail_at; /* 第 N 次调用失败 (-1 = 不启用) */
static int               g_tc_create_calls;
static int               g_tc_bind_fail;
static int               g_tc_bind_fail_at;
static int               g_tc_bind_calls;
static int               g_tc_send_fail;
static int               g_tc_recv_fail;
static int               g_tc_mcast_fail;
static int               g_tc_leave_fail;
static tcpip_socket_id_t g_tc_created[STUB_LOG_MAX];
static int               g_tc_created_cnt;
static tcpip_socket_id_t g_tc_closed[STUB_LOG_MAX];
static int               g_tc_closed_cnt;
static tcpip_socket_id_t g_tc_bind_sock;
static tcpip_sockaddr_t  g_tc_bind_addr;
static int               g_tc_bind_cnt;
static tcpip_socket_id_t g_tc_send_sock;
static tcpip_sockaddr_t  g_tc_send_addr;
static uint8_t           g_tc_send_data[STUB_SEND_MAX];
static uint16_t          g_tc_send_len;
static int               g_tc_send_cnt;
static int               g_tc_join_cnt;
static int               g_tc_leave_cnt;
static tcpip_ip_addr_t   g_tc_last_mcast;
static stub_rx_pkt_t     g_tc_rxq[STUB_RXQ_MAX];
static int               g_tc_rxq_cnt;

static void stub_tcpip_reset(void)
{
    memset(&g_tc_created, 0, sizeof(g_tc_created));
    memset(&g_tc_closed, 0, sizeof(g_tc_closed));
    memset(&g_tc_rxq, 0, sizeof(g_tc_rxq));
    memset(&g_tc_send_data, 0, sizeof(g_tc_send_data));
    g_tc_create_fail = 0; g_tc_create_fail_at = -1; g_tc_create_calls = 0;
    g_tc_bind_fail = 0; g_tc_bind_fail_at = -1; g_tc_bind_calls = 0;
    g_tc_send_fail = 0;
    g_tc_recv_fail = 0; g_tc_mcast_fail = 0; g_tc_leave_fail = 0;
    g_tc_created_cnt = 0; g_tc_closed_cnt = 0;
    g_tc_bind_sock = 0; memset(&g_tc_bind_addr, 0, sizeof(g_tc_bind_addr)); g_tc_bind_cnt = 0;
    g_tc_send_sock = 0; memset(&g_tc_send_addr, 0, sizeof(g_tc_send_addr));
    g_tc_send_len = 0; g_tc_send_cnt = 0;
    g_tc_join_cnt = 0; g_tc_leave_cnt = 0; g_tc_last_mcast = 0;
    g_tc_rxq_cnt = 0;
}

static void stub_push_rx(tcpip_socket_id_t sock, const void *data, uint16_t len,
                         uint32_t src_ip, uint16_t src_port)
{
    TEST_ASSERT_TRUE(g_tc_rxq_cnt < STUB_RXQ_MAX);
    g_tc_rxq[g_tc_rxq_cnt].sock = sock;
    g_tc_rxq[g_tc_rxq_cnt].src.addr = (tcpip_ip_addr_t)src_ip;
    g_tc_rxq[g_tc_rxq_cnt].src.port = src_port;
    g_tc_rxq[g_tc_rxq_cnt].len = len;
    memcpy(g_tc_rxq[g_tc_rxq_cnt].data, data, len);
    g_tc_rxq_cnt++;
}

tcpip_error_t tcpip_socket_create(tcpip_socket_type_t type, tcpip_socket_id_t *socket_id)
{
    int idx = g_tc_create_calls++;
    (void)type;
    if ((g_tc_create_fail != 0) || (g_tc_create_fail_at == idx)) {
        return TCPIP_ERROR;
    }
    *socket_id = (tcpip_socket_id_t)g_tc_created_cnt; /* 0,1,2,... */
    if (g_tc_created_cnt < STUB_LOG_MAX) {
        g_tc_created[g_tc_created_cnt++] = *socket_id;
    }
    return TCPIP_OK;
}

tcpip_error_t tcpip_socket_close(tcpip_socket_id_t socket_id)
{
    if (g_tc_closed_cnt < STUB_LOG_MAX) {
        g_tc_closed[g_tc_closed_cnt++] = socket_id;
    }
    return TCPIP_OK;
}

tcpip_error_t tcpip_socket_bind(tcpip_socket_id_t socket_id, const tcpip_sockaddr_t *addr)
{
    int idx = g_tc_bind_calls++;
    if ((g_tc_bind_fail != 0) || (g_tc_bind_fail_at == idx)) {
        return TCPIP_ERROR;
    }
    g_tc_bind_sock = socket_id;
    g_tc_bind_addr = *addr;
    g_tc_bind_cnt++;
    return TCPIP_OK;
}

tcpip_error_t tcpip_udp_sendto(tcpip_socket_id_t socket_id, const tcpip_sockaddr_t *dest,
                               const uint8_t *data, uint16_t len)
{
    if (g_tc_send_fail != 0) {
        return TCPIP_ERROR;
    }
    g_tc_send_sock = socket_id;
    g_tc_send_addr = *dest;
    g_tc_send_len = len;
    if (len > STUB_SEND_MAX) {
        len = STUB_SEND_MAX;
    }
    memcpy(g_tc_send_data, data, len);
    g_tc_send_cnt++;
    return TCPIP_OK;
}

tcpip_error_t tcpip_udp_recvfrom(tcpip_socket_id_t socket_id, tcpip_sockaddr_t *src,
                                 uint8_t *data, uint16_t *len, uint32_t flags)
{
    int i;
    (void)flags;
    if (g_tc_recv_fail != 0) {
        return TCPIP_ERROR;
    }
    for (i = 0; i < g_tc_rxq_cnt; i++) {
        if (g_tc_rxq[i].sock == socket_id) {
            *src = g_tc_rxq[i].src;
            *len = g_tc_rxq[i].len;
            memcpy(data, g_tc_rxq[i].data, g_tc_rxq[i].len);
            memmove(&g_tc_rxq[i], &g_tc_rxq[i + 1],
                    (size_t)(g_tc_rxq_cnt - i - 1) * sizeof(stub_rx_pkt_t));
            g_tc_rxq_cnt--;
            return TCPIP_OK;
        }
    }
    return TCPIP_ERROR_TIMEOUT;
}

tcpip_error_t tcpip_udp_join_multicast(tcpip_socket_id_t socket_id, tcpip_ip_addr_t mcast_addr)
{
    (void)socket_id;
    if (g_tc_mcast_fail != 0) {
        return TCPIP_ERROR;
    }
    g_tc_join_cnt++;
    g_tc_last_mcast = mcast_addr;
    return TCPIP_OK;
}

tcpip_error_t tcpip_udp_leave_multicast(tcpip_socket_id_t socket_id, tcpip_ip_addr_t mcast_addr)
{
    (void)socket_id;
    if (g_tc_leave_fail != 0) {
        return TCPIP_ERROR;
    }
    g_tc_leave_cnt++;
    g_tc_last_mcast = mcast_addr;
    return TCPIP_OK;
}

/* ============================================================================
 * 测试夹具
 * ============================================================================ */
static const uint8_t MAC_LOCAL[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
static const uint8_t PREFIX_REMOTE[12] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
                                          0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
/* init(domain=0, MAC_LOCAL) 后生成的本地 GUID 前缀 */
static const uint8_t PREFIX_LOCAL[12] = {0x00, 0x01, 0x02, 0x05, 0x00, 0x00,
                                         0x00, 0x01, 0x22, 0x33, 0x44, 0x55};

static int g_data_cb_cnt;
static uint32_t g_data_cb_ip;
static uint16_t g_data_cb_port;
static uint32_t g_data_cb_len;
static uint8_t g_data_cb_first;
static int g_disc_cb_cnt;
static bool g_disc_cb_joined;

static void test_data_rx_cb(const uint8_t *data, uint32_t len,
                            const dds_endpoint_locator_t *locator, void *user_data)
{
    (void)user_data;
    g_data_cb_cnt++;
    g_data_cb_len = len;
    if ((data != NULL) && (len > 0U)) {
        g_data_cb_first = data[0];
    }
    if (locator != NULL) {
        g_data_cb_ip = locator->ip_addr;
        g_data_cb_port = locator->port;
    }
}

static void test_disc_cb(const dds_participant_info_t *participant, bool joined, void *user_data)
{
    (void)participant;
    (void)user_data;
    g_disc_cb_cnt++;
    g_disc_cb_joined = joined;
}

static void reset_cbs(void)
{
    g_data_cb_cnt = 0; g_data_cb_ip = 0; g_data_cb_port = 0;
    g_data_cb_len = 0; g_data_cb_first = 0;
    g_disc_cb_cnt = 0; g_disc_cb_joined = false;
}

static dds_eth_transport_config_t base_cfg(void)
{
    dds_eth_transport_config_t c;
    memset(&c, 0, sizeof(c));
    c.local_ip = ETH_IP_ADDR(192, 168, 0, 1);
    memcpy(c.local_mac, MAC_LOCAL, 6);
    c.domain_id = 0;
    c.base_port = 7400;
    c.port_offset = 0;
    c.enable_multicast = true;
    c.multicast_addr = ETH_IP_ADDR(239, 255, 0, 1);
    c.tx_buffer_size = DDS_ETH_TX_BUFFER_SIZE;
    c.rx_buffer_size = DDS_ETH_RX_BUFFER_SIZE;
    c.heartbeat_period_ms = 1000;
    c.discovery_period_ms = 3000;
    c.lease_duration_ms = 10000;
    c.enable_qos_mapping = true;
    return c;
}

static void do_init(void)
{
    dds_eth_transport_config_t cfg = base_cfg();
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_transport_init(&cfg));
}

static void do_init_started(void)
{
    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_register_data_callback(test_data_rx_cb, NULL));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_register_discovery_callback(test_disc_cb, NULL));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_transport_start());
}

static void put_u16le(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFU);
    p[1] = (uint8_t)(v >> 8);
}

static uint32_t rtps_put_header(uint8_t *buf, const uint8_t prefix[12])
{
    buf[0] = 'R'; buf[1] = 'T'; buf[2] = 'P'; buf[3] = 'S';
    buf[4] = 0x02; buf[5] = 0x05;
    buf[6] = 0x00; buf[7] = 0x01;
    memcpy(&buf[8], prefix, 12);
    return 20U;
}

static void rtps_put_submsg(uint8_t *buf, uint8_t id, uint8_t flags, uint16_t len)
{
    buf[0] = id;
    buf[1] = flags;
    put_u16le(&buf[2], len);
}

void setUp(void)
{
    stub_tcpip_reset();
    reset_cbs();
    dds_eth_transport_deinit(); /* 清理上一个测试可能遗留的产品状态 */
}

void tearDown(void)
{
    dds_eth_transport_deinit();
}

/* ============================================================================
 * 初始化 / 启停
 * ============================================================================ */
static void test_init_null_config(void)
{
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_transport_init(NULL));
}

static void test_init_success_and_double_init(void)
{
    dds_eth_transport_config_t cfg = base_cfg();
    do_init();
    TEST_ASSERT_EQUAL_INT(2, g_tc_created_cnt);          /* 单播+多播两个 socket */
    TEST_ASSERT_EQUAL_INT(ETH_BUSY, dds_eth_transport_init(&cfg));
    dds_eth_transport_deinit();
    do_init();                                           /* deinit 后可重新 init */
}

static void test_init_socket_create_fail(void)
{
    dds_eth_transport_config_t cfg = base_cfg();
    g_tc_create_fail = 1;
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_eth_transport_init(&cfg));
    TEST_ASSERT_EQUAL_INT(0, g_tc_created_cnt);
    TEST_ASSERT_EQUAL_INT(0, g_tc_closed_cnt);
}

static void test_init_second_create_fail_closes_first(void)
{
    /* 第 1 个 socket 成功, 第 2 个失败: 清理路径应关闭已创建的 socket */
    dds_eth_transport_config_t cfg = base_cfg();
    g_tc_create_fail_at = 1;
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_eth_transport_init(&cfg));
    TEST_ASSERT_EQUAL_INT(1, g_tc_created_cnt);
    TEST_ASSERT_EQUAL_INT(1, g_tc_closed_cnt);
    TEST_ASSERT_EQUAL_INT((int)g_tc_created[0], (int)g_tc_closed[0]);
}

static void test_start_stop_lifecycle(void)
{
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_eth_transport_start());
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_eth_transport_stop());
    do_init();

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_transport_start());
    /* NOTE(产品行为): start 依次绑定单播 socket 与多播 socket (共 2 次 bind) */
    TEST_ASSERT_EQUAL_INT(2, g_tc_bind_cnt);
    TEST_ASSERT_EQUAL_INT((int)g_tc_created[1], (int)g_tc_bind_sock); /* 最后绑定的是多播 socket */
    TEST_ASSERT_EQUAL_HEX32(ETH_IP_ADDR(239, 255, 0, 1), g_tc_bind_addr.addr);
    TEST_ASSERT_EQUAL_UINT16(7400, g_tc_bind_addr.port); /* base_port + 多播端口偏移(0) */
    TEST_ASSERT_EQUAL_INT(1, g_tc_join_cnt);             /* 多播加入 */
    TEST_ASSERT_EQUAL_HEX32(ETH_IP_ADDR(239, 255, 0, 1), g_tc_last_mcast);

    TEST_ASSERT_EQUAL_INT(ETH_BUSY, dds_eth_transport_start()); /* 重复 start */
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_transport_stop());
    TEST_ASSERT_EQUAL_INT(1, g_tc_leave_cnt);            /* stop 仅离开一次多播 */
    TEST_ASSERT_EQUAL_INT(2, g_tc_closed_cnt);           /* close 单播 + 多播两个 socket */
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_transport_stop()); /* 未运行再 stop 仍 OK */
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_eth_transport_process(0));
}

static void test_start_bind_fail(void)
{
    do_init();
    g_tc_bind_fail = 1;                                  /* 单播 bind 即失败 */
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_eth_transport_start());
    TEST_ASSERT_EQUAL_INT(0, g_tc_join_cnt);             /* 失败后不进入多播分支 */
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_eth_transport_process(0)); /* running 仍为 false */
}

static void test_start_multicast_join_fail(void)
{
    do_init();
    g_tc_mcast_fail = 1;
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_eth_transport_start());
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_eth_transport_process(0));
}

static void test_start_multicast_bind_fail(void)
{
    /* 第 1 次 bind(单播)成功, 第 2 次 bind(多播)失败: start 报 ETH_ERROR */
    do_init();
    g_tc_bind_fail_at = 1;
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_eth_transport_start());
    TEST_ASSERT_EQUAL_INT(1, g_tc_bind_cnt);             /* 只有单播 bind 成功 */
    TEST_ASSERT_EQUAL_INT(1, g_tc_join_cnt);             /* 多播已加入 (start 内部顺序) */
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_eth_transport_process(0));
}

static void test_register_callbacks_guards(void)
{
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_eth_register_data_callback(test_data_rx_cb, NULL));
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_eth_register_discovery_callback(test_disc_cb, NULL));
    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_register_data_callback(test_data_rx_cb, NULL));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_register_discovery_callback(test_disc_cb, NULL));
}

/* ============================================================================
 * 发送
 * ============================================================================ */
static void test_send_rtps_guards(void)
{
    uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    dds_endpoint_locator_t loc;
    memset(&loc, 0, sizeof(loc));
    loc.ip_addr = ETH_IP_ADDR(192, 168, 0, 2);
    loc.port = 7400;

    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_eth_send_rtps(data, 8, &loc, false));
    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_send_rtps(NULL, 8, &loc, false));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_send_rtps(data, 8, NULL, false));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_send_rtps(data, 0, &loc, false));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_send_rtps(data, DDS_ETH_MAX_RTPS_PACKET_SIZE + 1U, &loc, false));
    TEST_ASSERT_EQUAL_INT(0, g_tc_send_cnt);
}

static void test_send_rtps_success_unicast(void)
{
    uint8_t data[8] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80};
    dds_endpoint_locator_t loc;
    dds_eth_transport_stats_t stats;
    memset(&loc, 0, sizeof(loc));
    loc.ip_addr = ETH_IP_ADDR(192, 168, 0, 2);
    loc.port = 7400;
    loc.is_multicast = false;

    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_send_rtps(data, 8, &loc, false));
    TEST_ASSERT_EQUAL_INT(1, g_tc_send_cnt);
    TEST_ASSERT_EQUAL_INT((int)g_tc_created[0], (int)g_tc_send_sock); /* 单播 socket */
    TEST_ASSERT_EQUAL_HEX32(ETH_IP_ADDR(192, 168, 0, 2), g_tc_send_addr.addr);
    TEST_ASSERT_EQUAL_UINT16(7400, g_tc_send_addr.port);
    TEST_ASSERT_EQUAL_UINT16(8, g_tc_send_len);
    TEST_ASSERT_EQUAL_HEX8(0x10, g_tc_send_data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x80, g_tc_send_data[7]);

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(1, stats.tx_packets);
    TEST_ASSERT_EQUAL_UINT32(8, stats.tx_bytes);
    TEST_ASSERT_EQUAL_UINT32(0, stats.tx_errors);
}

static void test_send_rtps_multicast_locator(void)
{
    uint8_t data[4] = {1, 2, 3, 4};
    dds_endpoint_locator_t loc;
    memset(&loc, 0, sizeof(loc));
    loc.ip_addr = ETH_IP_ADDR(192, 168, 0, 2);      /* 应被忽略 */
    loc.multicast_addr = ETH_IP_ADDR(239, 255, 0, 1);
    loc.port = 7401;
    loc.is_multicast = true;

    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_send_rtps(data, 4, &loc, false));
    TEST_ASSERT_EQUAL_HEX32(ETH_IP_ADDR(239, 255, 0, 1), g_tc_send_addr.addr);
    TEST_ASSERT_EQUAL_UINT16(7401, g_tc_send_addr.port);
}

static void test_send_rtps_send_fail(void)
{
    uint8_t data[4] = {1, 2, 3, 4};
    dds_endpoint_locator_t loc;
    dds_eth_transport_stats_t stats;
    memset(&loc, 0, sizeof(loc));
    loc.ip_addr = ETH_IP_ADDR(192, 168, 0, 2);
    loc.port = 7400;

    do_init();
    g_tc_send_fail = 1;
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_eth_send_rtps(data, 4, &loc, false));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(1, stats.tx_errors);
    TEST_ASSERT_EQUAL_UINT32(0, stats.tx_packets);
}

static void test_send_rtps_reliable_unsupported(void)
{
    /* NOTE(产品行为): reliable=true 的 TCP 可靠路径未实现, 直接返回 TCPIP_ERROR -> ETH_ERROR */
    uint8_t data[4] = {1, 2, 3, 4};
    dds_endpoint_locator_t loc;
    memset(&loc, 0, sizeof(loc));
    loc.ip_addr = ETH_IP_ADDR(192, 168, 0, 2);
    loc.port = 7400;

    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_eth_send_rtps(data, 4, &loc, true));
}

/* ============================================================================
 * 统计
 * ============================================================================ */
static void test_stats_api(void)
{
    dds_eth_transport_stats_t stats;
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_eth_get_stats(&stats));
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_eth_clear_stats());

    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_get_stats(NULL));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(0, stats.tx_packets);
    TEST_ASSERT_EQUAL_UINT32(0, stats.active_endpoints);
    TEST_ASSERT_EQUAL_UINT32(0, stats.active_participants);

    stats.tx_packets = 99;
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_clear_stats());
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(0, stats.tx_packets);
}

/* ============================================================================
 * Endpoint 管理
 * ============================================================================ */
static dds_endpoint_config_t make_endpoint(uint8_t tag, dds_endpoint_type_t type)
{
    dds_endpoint_config_t e;
    memset(&e, 0, sizeof(e));
    memset(e.guid.prefix, 0xB0, 11);
    e.guid.prefix[11] = tag;
    e.guid.entity_id[0] = tag;
    e.type = type;
    e.locator.ip_addr = ETH_IP_ADDR(10, 0, 0, tag);
    e.locator.port = 7000U + tag;
    strcpy(e.topic_name, "TestTopic");
    return e;
}

static void test_endpoint_register_find_unregister(void)
{
    dds_endpoint_config_t e = make_endpoint(1, DDS_ENDPOINT_TYPE_WRITER);
    dds_endpoint_config_t out;
    dds_guid_t guid;
    uint32_t id = 0xFFFFFFFFU;
    dds_eth_transport_stats_t stats;

    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_register_endpoint(&e, &id));
    TEST_ASSERT_EQUAL_UINT32(0, id);

    memset(&out, 0, sizeof(out));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_find_endpoint(&e.guid, &out));
    TEST_ASSERT_EQUAL_UINT32(ETH_IP_ADDR(10, 0, 0, 1), out.locator.ip_addr);
    TEST_ASSERT_EQUAL_UINT16(7001, out.locator.port);
    TEST_ASSERT_EQUAL_INT((int)DDS_ENDPOINT_TYPE_WRITER, (int)out.type);
    TEST_ASSERT_EQUAL_STRING("TestTopic", out.topic_name);

    /* 未知 GUID */
    memset(&guid, 0, sizeof(guid));
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_eth_find_endpoint(&guid, &out));

    /* 注销后再查 */
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_unregister_endpoint(id));
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_eth_find_endpoint(&e.guid, &out));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(0, stats.active_endpoints);
}

static void test_endpoint_guards(void)
{
    dds_endpoint_config_t e = make_endpoint(2, DDS_ENDPOINT_TYPE_READER);
    dds_endpoint_config_t out;
    uint32_t id = 0;

    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_eth_register_endpoint(&e, &id));
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_eth_unregister_endpoint(0));
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_eth_find_endpoint(&e.guid, &out));
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_eth_get_active_endpoints(&out, 1, &id));

    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_register_endpoint(NULL, &id));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_register_endpoint(&e, NULL));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_unregister_endpoint(DDS_ETH_MAX_ENDPOINTS));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_unregister_endpoint(0)); /* 未使用 */
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_find_endpoint(NULL, &out));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_find_endpoint(&e.guid, NULL));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_get_active_endpoints(NULL, 1, &id));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_get_active_endpoints(&out, 1, NULL));
}

static void test_endpoint_table_full(void)
{
    dds_endpoint_config_t e;
    uint32_t id;
    int i;

    do_init();
    for (i = 0; i < (int)DDS_ETH_MAX_ENDPOINTS; i++) {
        e = make_endpoint((uint8_t)(i + 1U), DDS_ENDPOINT_TYPE_WRITER);
        TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_register_endpoint(&e, &id));
        TEST_ASSERT_EQUAL_UINT32((uint32_t)i, id);
    }
    e = make_endpoint(0xFF, DDS_ENDPOINT_TYPE_WRITER);
    TEST_ASSERT_EQUAL_INT(ETH_NO_MEMORY, dds_eth_register_endpoint(&e, &id));
}

static void test_endpoint_get_active(void)
{
    dds_endpoint_config_t e1 = make_endpoint(1, DDS_ENDPOINT_TYPE_WRITER);
    dds_endpoint_config_t e2 = make_endpoint(2, DDS_ENDPOINT_TYPE_READER);
    dds_endpoint_config_t outs[4];
    uint32_t actual = 0xFFFF;
    uint32_t id;

    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_register_endpoint(&e1, &id));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_register_endpoint(&e2, &id));

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_get_active_endpoints(outs, 4, &actual));
    TEST_ASSERT_EQUAL_UINT32(2, actual);
    TEST_ASSERT_EQUAL_UINT32(ETH_IP_ADDR(10, 0, 0, 1), outs[0].locator.ip_addr);
    TEST_ASSERT_EQUAL_UINT32(ETH_IP_ADDR(10, 0, 0, 2), outs[1].locator.ip_addr);

    actual = 0xFFFF;
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_get_active_endpoints(outs, 1, &actual));
    TEST_ASSERT_EQUAL_UINT32(1, actual);                 /* max_count 截断 */

    actual = 0xFFFF;
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_get_active_endpoints(outs, 0, &actual));
    TEST_ASSERT_EQUAL_UINT32(0, actual);
}

/* ============================================================================
 * Participant 管理
 * ============================================================================ */
static dds_participant_info_t make_participant(uint8_t tag)
{
    dds_participant_info_t p;
    memset(&p, 0, sizeof(p));
    memset(p.guid.prefix, 0xC0, 11);
    p.guid.prefix[11] = tag;
    p.guid.entity_id[3] = 0xC1;
    p.unicast_addr = ETH_IP_ADDR(192, 168, 1, tag);
    p.metatraffic_unicast_port = 7400;
    p.lease_duration_ms = 10000;
    return p;
}

static void test_participant_add_remove_callbacks(void)
{
    dds_participant_info_t p = make_participant(1);
    dds_participant_info_t got;
    dds_participant_info_t list[4];
    uint32_t actual = 0;
    dds_eth_transport_stats_t stats;

    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_register_discovery_callback(test_disc_cb, NULL));

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_add_participant(&p));
    TEST_ASSERT_EQUAL_INT(1, g_disc_cb_cnt);
    TEST_ASSERT_EQUAL_INT(1, (int)g_disc_cb_joined);     /* joined */

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_get_participant(&p.guid, &got));
    TEST_ASSERT_EQUAL_HEX32(ETH_IP_ADDR(192, 168, 1, 1), got.unicast_addr);
    TEST_ASSERT_EQUAL_UINT32(0, got.last_seen_ms);       /* 添加时强制归零 */

    actual = 0;
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_get_active_participants(list, 4, &actual));
    TEST_ASSERT_EQUAL_UINT32(1, actual);

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_refresh_participant(&p.guid)); /* 已知 GUID */
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_remove_participant(&p.guid));
    TEST_ASSERT_EQUAL_INT(2, g_disc_cb_cnt);
    TEST_ASSERT_EQUAL_INT(0, (int)g_disc_cb_joined);     /* left */

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(0, stats.active_participants);
}

static void test_participant_add_existing_updates(void)
{
    /* NOTE(产品行为): 重复添加相同 GUID 走更新路径, 不再触发发现回调 */
    dds_participant_info_t p = make_participant(1);
    dds_participant_info_t p2 = make_participant(1);
    dds_participant_info_t list[4];
    uint32_t actual;

    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_register_discovery_callback(test_disc_cb, NULL));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_add_participant(&p));
    p2.unicast_addr = ETH_IP_ADDR(192, 168, 9, 9);
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_add_participant(&p2));
    TEST_ASSERT_EQUAL_INT(1, g_disc_cb_cnt);             /* 无重复回调 */

    actual = 0;
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_get_active_participants(list, 4, &actual));
    TEST_ASSERT_EQUAL_UINT32(1, actual);                 /* 未新增条目 */
    TEST_ASSERT_EQUAL_HEX32(ETH_IP_ADDR(192, 168, 9, 9), list[0].unicast_addr);
}

static void test_participant_guards(void)
{
    dds_participant_info_t p = make_participant(3);
    dds_guid_t guid;
    dds_participant_info_t got;
    uint32_t actual;

    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_eth_add_participant(&p));
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_eth_remove_participant(&p.guid));
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_eth_get_participant(&p.guid, &got));
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_eth_refresh_participant(&p.guid));

    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_add_participant(NULL));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_remove_participant(NULL));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_get_participant(NULL, &got));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_get_participant(&p.guid, NULL));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_refresh_participant(NULL));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_get_active_participants(NULL, 1, &actual));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_get_active_participants(&got, 1, NULL));

    memset(&guid, 0, sizeof(guid));
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_eth_remove_participant(&guid));   /* 未知 */
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_eth_refresh_participant(&guid));  /* 未知 */
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_eth_get_participant(&guid, &got));/* 未知 */
}

static void test_participant_table_full(void)
{
    dds_participant_info_t p;
    int i;

    do_init();
    for (i = 0; i < (int)DDS_ETH_MAX_PARTICIPANTS; i++) {
        p = make_participant((uint8_t)(i + 1U));
        TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_add_participant(&p));
    }
    p = make_participant(0xFF);
    TEST_ASSERT_EQUAL_INT(ETH_NO_MEMORY, dds_eth_add_participant(&p));
}

/* ============================================================================
 * 多播地址计算
 * ============================================================================ */
static void test_calc_multicast_addr(void)
{
    eth_ip_addr_t addr = 0;

    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_calc_multicast_addr(0, 0, NULL));

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_calc_multicast_addr(0, 0, &addr));
    TEST_ASSERT_EQUAL_HEX32(ETH_IP_ADDR(239, 255, 0, 0), addr);

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_calc_multicast_addr(1, 1, &addr));
    TEST_ASSERT_EQUAL_HEX32(ETH_IP_ADDR(239, 255, 0, 2), addr);

    /* (domain + participant) % 256 */
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_calc_multicast_addr(300, 0, &addr));
    TEST_ASSERT_EQUAL_HEX32(ETH_IP_ADDR(239, 255, 0, 44), addr);
}

/* ============================================================================
 * RTPS 消息头
 * ============================================================================ */
static void test_rtps_header_create(void)
{
    dds_rtps_header_t h;
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_create_rtps_header(NULL, PREFIX_LOCAL));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_create_rtps_header(&h, NULL));

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_create_rtps_header(&h, PREFIX_LOCAL));
    TEST_ASSERT_EQUAL_HEX32(0x52545053U, h.protocol_id);
    TEST_ASSERT_EQUAL_HEX16(0x0205, h.protocol_version);
    TEST_ASSERT_EQUAL_HEX16(0x0001, h.vendor_id);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(PREFIX_LOCAL, h.guid_prefix, 12);
}

static void test_rtps_header_parse_roundtrip(void)
{
    uint8_t buf[32];
    dds_rtps_header_t h;
    uint32_t hlen = 0;

    (void)rtps_put_header(buf, PREFIX_REMOTE);
    memset(&buf[20], 0xEE, 12);                          /* 头后还有负载 */

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_parse_rtps_header(buf, 32, &h, &hlen));
    TEST_ASSERT_EQUAL_UINT32(20, hlen);
    TEST_ASSERT_EQUAL_HEX32(0x52545053U, h.protocol_id); /* 大端解析 "RTPS" */
    TEST_ASSERT_EQUAL_HEX16(0x0205, h.protocol_version);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(PREFIX_REMOTE, h.guid_prefix, 12);
}

static void test_rtps_header_parse_guards(void)
{
    uint8_t buf[24];
    dds_rtps_header_t h;
    uint32_t hlen = 0;

    (void)rtps_put_header(buf, PREFIX_REMOTE);

    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_parse_rtps_header(NULL, 24, &h, &hlen));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_parse_rtps_header(buf, 24, NULL, &hlen));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_parse_rtps_header(buf, 24, &h, NULL));
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_eth_parse_rtps_header(buf, sizeof(dds_rtps_header_t) - 1U, &h, &hlen));

    buf[0] = 'X';                                        /* 协议 ID 不符 */
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_eth_parse_rtps_header(buf, 24, &h, &hlen));
}

/* ============================================================================
 * DATA 子消息封装/解封
 * ============================================================================ */
static void test_data_encapsulate_decapsulate_roundtrip(void)
{
    uint8_t writer[4] = {1, 2, 3, 4};
    uint8_t reader[4] = {5, 6, 7, 8};
    uint8_t payload[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    uint8_t buf[128];
    uint32_t alen = 0;
    uint8_t w2[4], r2[4];
    uint64_t seq = 0;
    const uint8_t *pl = NULL;
    uint32_t plen = 0;
    dds_rtps_data_submsg_t *d;

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_encapsulate_data(buf, sizeof(buf), writer, reader,
                                                           0x123456789ABCDEF0ULL, payload, 10, &alen));
    TEST_ASSERT_EQUAL_UINT32(sizeof(dds_rtps_data_submsg_t) + 10U, alen);
    TEST_ASSERT_EQUAL_UINT32(34, alen);

    d = (dds_rtps_data_submsg_t *)buf;
    TEST_ASSERT_EQUAL_HEX8(DDS_RTPS_SUBMSG_DATA, d->header.submessage_id);
    TEST_ASSERT_EQUAL_HEX8(DDS_RTPS_SUBMSG_FLAG_ENDIANNESS | DDS_RTPS_SUBMSG_FLAG_DATA_PRESENT, d->header.flags);
    /* NOTE(产品行为): submessage_length = sizeof(submsg) - sizeof(submsg_hdr) + payload - 4
     * = 16 + payload_len (比 RTPS 规范少 4, 记录差异不改产品代码) */
    TEST_ASSERT_EQUAL_HEX16(16 + 10, d->header.submessage_length);
    TEST_ASSERT_EQUAL_UINT16(16, d->octets_to_inline_qos);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(reader, d->reader_id, 4);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(writer, d->writer_id, 4);
    TEST_ASSERT_EQUAL_HEX32(0x12345678U, d->writer_seq_num_high);
    TEST_ASSERT_EQUAL_HEX32(0x9ABCDEF0U, d->writer_seq_num_low);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, &buf[sizeof(dds_rtps_data_submsg_t)], 10);

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_decapsulate_data(buf, alen, w2, r2, &seq, &pl, &plen));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(writer, w2, 4);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(reader, r2, 4);
    TEST_ASSERT_EQUAL_HEX32(0x12345678U, (uint32_t)(seq >> 32));
    TEST_ASSERT_EQUAL_HEX32(0x9ABCDEF0U, (uint32_t)(seq & 0xFFFFFFFFULL));
    TEST_ASSERT_EQUAL_PTR(&buf[sizeof(dds_rtps_data_submsg_t)], pl);
    TEST_ASSERT_EQUAL_UINT32(10, plen);
}

static void test_data_encapsulate_guards(void)
{
    uint8_t writer[4] = {1, 2, 3, 4};
    uint8_t reader[4] = {5, 6, 7, 8};
    uint8_t payload[4] = {9, 9, 9, 9};
    uint8_t small[16];
    uint8_t buf[64];
    uint32_t alen = 0;

    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_encapsulate_data(NULL, 64, writer, reader, 1, payload, 4, &alen));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_encapsulate_data(buf, 64, NULL, reader, 1, payload, 4, &alen));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_encapsulate_data(buf, 64, writer, NULL, 1, payload, 4, &alen));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_encapsulate_data(buf, 64, writer, reader, 1, NULL, 4, &alen));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_encapsulate_data(buf, 64, writer, reader, 1, payload, 4, NULL));

    /* 缓冲区过小: 需要 24 + 4 = 28 */
    TEST_ASSERT_EQUAL_INT(ETH_NO_MEMORY, dds_eth_encapsulate_data(small, sizeof(small), writer, reader, 1, payload, 4, &alen));
}

static void test_data_decapsulate_guards(void)
{
    uint8_t writer[4] = {1, 2, 3, 4};
    uint8_t reader[4] = {5, 6, 7, 8};
    uint8_t payload[4] = {9, 9, 9, 9};
    uint8_t buf[64];
    uint32_t alen = 0;
    uint8_t w2[4], r2[4];
    uint64_t seq = 0;
    const uint8_t *pl = NULL;
    uint32_t plen = 0;

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_encapsulate_data(buf, sizeof(buf), writer, reader, 1, payload, 4, &alen));

    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_decapsulate_data(NULL, alen, w2, r2, &seq, &pl, &plen));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_decapsulate_data(buf, alen, NULL, r2, &seq, &pl, &plen));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_decapsulate_data(buf, alen, w2, NULL, &seq, &pl, &plen));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_decapsulate_data(buf, alen, w2, r2, NULL, &pl, &plen));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_decapsulate_data(buf, alen, w2, r2, &seq, NULL, &plen));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_decapsulate_data(buf, alen, w2, r2, &seq, &pl, NULL));

    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_eth_decapsulate_data(buf, sizeof(dds_rtps_data_submsg_t) - 1U, w2, r2, &seq, &pl, &plen));

    buf[0] = 0x07;                                       /* HEARTBEAT 而非 DATA */
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_eth_decapsulate_data(buf, alen, w2, r2, &seq, &pl, &plen));

    /* 长度恰等于头长: 无负载, payload=NULL/0 */
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_encapsulate_data(buf, sizeof(buf), writer, reader, 1, payload, 4, &alen));
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_decapsulate_data(buf, sizeof(dds_rtps_data_submsg_t), w2, r2, &seq, &pl, &plen));
    TEST_ASSERT_NULL(pl);
    TEST_ASSERT_EQUAL_UINT32(0, plen);
}

/* ============================================================================
 * HEARTBEAT / ACKNACK 子消息
 * ============================================================================ */
static void test_create_heartbeat(void)
{
    uint8_t writer[4] = {0x11, 0x22, 0x33, 0x44};
    uint8_t buf[64];
    uint32_t alen = 0;
    dds_rtps_heartbeat_submsg_t *hb;
    dds_eth_transport_stats_t stats;

    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_create_heartbeat(buf, sizeof(buf), writer, 5, 9, 42, &alen));
    TEST_ASSERT_EQUAL_UINT32(sizeof(dds_rtps_heartbeat_submsg_t), alen);
    TEST_ASSERT_EQUAL_UINT32(32, alen);

    hb = (dds_rtps_heartbeat_submsg_t *)buf;
    TEST_ASSERT_EQUAL_HEX8(DDS_RTPS_SUBMSG_HEARTBEAT, hb->header.submessage_id);
    TEST_ASSERT_EQUAL_HEX8(DDS_RTPS_SUBMSG_FLAG_ENDIANNESS, hb->header.flags);
    TEST_ASSERT_EQUAL_HEX16((uint16_t)(sizeof(dds_rtps_heartbeat_submsg_t) - sizeof(dds_rtps_submsg_header_t)), hb->header.submessage_length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(writer, hb->writer_id, 4);
    TEST_ASSERT_EQUAL_UINT32(0, hb->reader_id[0]);       /* reader 未知, 全 0 */
    TEST_ASSERT_EQUAL_HEX32(0, hb->first_seq_num_high);
    TEST_ASSERT_EQUAL_HEX32(5, hb->first_seq_num_low);
    TEST_ASSERT_EQUAL_HEX32(0, hb->last_seq_num_high);
    TEST_ASSERT_EQUAL_HEX32(9, hb->last_seq_num_low);
    TEST_ASSERT_EQUAL_HEX32(42, hb->count);

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(1, stats.heartbeat_sent);

    TEST_ASSERT_EQUAL_INT(ETH_NO_MEMORY, dds_eth_create_heartbeat(buf, 16, writer, 1, 1, 1, &alen));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_create_heartbeat(NULL, 64, writer, 1, 1, 1, &alen));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_create_heartbeat(buf, 64, NULL, 1, 1, 1, &alen));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_create_heartbeat(buf, 64, writer, 1, 1, 1, NULL));
}

static void test_create_acknack(void)
{
    uint8_t reader[4] = {0xA0, 0xA1, 0xA2, 0xA3};
    uint8_t writer[4] = {0xB0, 0xB1, 0xB2, 0xB3};
    uint8_t buf[64];
    uint32_t alen = 0;
    dds_rtps_acknack_submsg_t *ack;
    dds_eth_transport_stats_t stats;

    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_create_acknack(buf, sizeof(buf), reader, writer, 0x0000000FU, 7, &alen));
    TEST_ASSERT_EQUAL_UINT32(sizeof(dds_rtps_acknack_submsg_t), alen);
    TEST_ASSERT_EQUAL_UINT32(20, alen);

    ack = (dds_rtps_acknack_submsg_t *)buf;
    TEST_ASSERT_EQUAL_HEX8(DDS_RTPS_SUBMSG_ACKNACK, ack->header.submessage_id);
    TEST_ASSERT_EQUAL_HEX8(DDS_RTPS_SUBMSG_FLAG_ENDIANNESS, ack->header.flags);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(reader, ack->reader_id, 4);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(writer, ack->writer_id, 4);
    TEST_ASSERT_EQUAL_HEX32(0x0000000FU, ack->reader_sn_state);
    TEST_ASSERT_EQUAL_HEX32(7, ack->count);

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(1, stats.acknack_sent);

    TEST_ASSERT_EQUAL_INT(ETH_NO_MEMORY, dds_eth_create_acknack(buf, 8, reader, writer, 1, 1, &alen));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_create_acknack(NULL, 64, reader, writer, 1, 1, &alen));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_create_acknack(buf, 64, NULL, writer, 1, 1, &alen));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_create_acknack(buf, 64, reader, NULL, 1, 1, &alen));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_create_acknack(buf, 64, reader, writer, 1, 1, NULL));
}

/* ============================================================================
 * 接收处理 (dds_eth_transport_process)
 * ============================================================================ */
static void test_process_lifecycle(void)
{
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_eth_transport_process(0));

    do_init();                                           /* 未 start */
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_eth_transport_process(0));

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_transport_start());
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_transport_process(0)); /* 空队列 -> TIMEOUT 视为正常 */
}

static void test_process_rx_data_callback(void)
{
    uint8_t pkt[64];
    uint32_t off;

    do_init_started();
    off = rtps_put_header(pkt, PREFIX_REMOTE);
    rtps_put_submsg(&pkt[off], DDS_RTPS_SUBMSG_DATA, 0x05, 8);
    memset(&pkt[off + 4], 0x5A, 8);
    stub_push_rx(g_tc_created[0], pkt, off + 12U, ETH_IP_ADDR(192, 168, 5, 5), 7400);

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_transport_process(0));
    TEST_ASSERT_EQUAL_INT(1, g_data_cb_cnt);
    TEST_ASSERT_EQUAL_UINT32(12, g_data_cb_len);         /* DATA 子消息整体 */
    TEST_ASSERT_EQUAL_HEX8(DDS_RTPS_SUBMSG_DATA, pkt[20]);
    TEST_ASSERT_EQUAL_HEX32(ETH_IP_ADDR(192, 168, 5, 5), g_data_cb_ip);
    TEST_ASSERT_EQUAL_UINT16(7400, g_data_cb_port);
}

static void test_process_own_packet_ignored(void)
{
    /* 本地 GUID 前缀的包被静默丢弃 (环路抑制), 不触发回调 */
    uint8_t pkt[64];
    uint32_t off;
    dds_eth_transport_stats_t stats;

    do_init_started();
    off = rtps_put_header(pkt, PREFIX_LOCAL);
    rtps_put_submsg(&pkt[off], DDS_RTPS_SUBMSG_DATA, 0x05, 8);
    memset(&pkt[off + 4], 0x5A, 8);
    stub_push_rx(g_tc_created[0], pkt, off + 12U, ETH_IP_ADDR(192, 168, 0, 1), 7400);

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_transport_process(0));
    TEST_ASSERT_EQUAL_INT(0, g_data_cb_cnt);

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(1, stats.rx_packets);       /* 收包计数仍增加 */
}

static void test_process_heartbeat_acknack_stats(void)
{
    /* 一个包内同时携带 HEARTBEAT + ACKNACK 子消息 */
    uint8_t pkt[96];
    uint32_t off;
    dds_eth_transport_stats_t stats;

    do_init_started();
    off = rtps_put_header(pkt, PREFIX_REMOTE);
    rtps_put_submsg(&pkt[off], DDS_RTPS_SUBMSG_HEARTBEAT, 0x01, 28);
    memset(&pkt[off + 4], 0, 28);
    off += 4U + 28U;
    rtps_put_submsg(&pkt[off], DDS_RTPS_SUBMSG_ACKNACK, 0x01, 12);
    memset(&pkt[off + 4], 0, 12);
    off += 4U + 12U;

    stub_push_rx(g_tc_created[0], pkt, off, ETH_IP_ADDR(192, 168, 5, 6), 7400);
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_transport_process(0));
    TEST_ASSERT_EQUAL_INT(0, g_data_cb_cnt);

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(1, stats.heartbeat_received);
    TEST_ASSERT_EQUAL_UINT32(1, stats.acknack_received);
}

static void test_process_multicast_rx(void)
{
    /* 推到多播 socket 的包同样被处理 */
    uint8_t pkt[64];
    uint32_t off;

    do_init_started();
    off = rtps_put_header(pkt, PREFIX_REMOTE);
    rtps_put_submsg(&pkt[off], DDS_RTPS_SUBMSG_DATA, 0x05, 8);
    memset(&pkt[off + 4], 0x33, 8);
    stub_push_rx(g_tc_created[1], pkt, off + 12U, ETH_IP_ADDR(239, 255, 0, 1), 7400);

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_transport_process(0));
    TEST_ASSERT_EQUAL_INT(1, g_data_cb_cnt);
    /* NOTE(产品行为): 多播包上报的 locator.is_multicast 也为 false (未区分来源) */
    TEST_ASSERT_EQUAL_HEX32(ETH_IP_ADDR(239, 255, 0, 1), g_data_cb_ip);
}

static void test_process_recv_error_counts(void)
{
    /* NOTE(产品行为): recvfrom 报错时 process 仍返回 ETH_OK, 仅 rx_errors 计数 */
    uint8_t pkt[4] = {1, 2, 3, 4};
    dds_eth_transport_stats_t stats;

    do_init_started();
    stub_push_rx(g_tc_created[0], pkt, 4, 0, 0);
    g_tc_recv_fail = 1;

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_transport_process(0));
    TEST_ASSERT_EQUAL_INT(0, g_data_cb_cnt);
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_get_stats(&stats));
    /* NOTE(产品行为): process 同时轮询单播+多播 socket, recv 失败各计一次 → 2 */
    TEST_ASSERT_EQUAL_UINT32(2, stats.rx_errors);
    TEST_ASSERT_EQUAL_UINT32(0, stats.rx_packets);
}

static void test_process_non_rtps_packet_dropped(void)
{
    /* 非 RTPS 协议 ID 的包: 不触发回调, 不计 rx_errors */
    uint8_t pkt[32];
    dds_eth_transport_stats_t stats;

    do_init_started();
    memset(pkt, 0xEE, sizeof(pkt));
    stub_push_rx(g_tc_created[0], pkt, sizeof(pkt), ETH_IP_ADDR(10, 0, 0, 9), 1234);

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_transport_process(0));
    TEST_ASSERT_EQUAL_INT(0, g_data_cb_cnt);
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(1, stats.rx_packets);
    TEST_ASSERT_EQUAL_UINT32(0, stats.rx_errors);
}

/* ============================================================================
 * 多播组管理独立 API
 * ============================================================================ */
static void test_join_multicast_group_guards(void)
{
    eth_ip_addr_t addr = ETH_IP_ADDR(239, 255, 0, 1);
    eth_mac_addr_t mac = {0x01, 0x00, 0x5E, 0x00, 0x00, 0x01};

    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_eth_join_multicast_group(addr, mac));
    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_eth_leave_multicast_group(addr));

    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_join_multicast_group(addr, mac));
    TEST_ASSERT_EQUAL_INT(1, g_tc_join_cnt);             /* 独立 join (init 没调 start, 所以之前没 join) */

    g_tc_mcast_fail = 1;
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_eth_join_multicast_group(addr, mac));

    g_tc_mcast_fail = 0;
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_leave_multicast_group(addr));
    TEST_ASSERT_EQUAL_INT(1, g_tc_leave_cnt);

    /* leave_multicast 失败路径 */
    g_tc_leave_fail = 1;
    TEST_ASSERT_EQUAL_INT(ETH_ERROR, dds_eth_leave_multicast_group(addr));
    g_tc_leave_fail = 0;
}

/* ============================================================================
 * participant table 满查找失败 / 多播独立 API 校验
 * ============================================================================ */
static void test_participant_table_full_get_active(void)
{
    /* get_active_participants 在表满时 (DDS_ETH_MAX_PARTICIPANTS) 仍工作 */
    dds_participant_info_t p;
    dds_participant_info_t list[DDS_ETH_MAX_PARTICIPANTS + 4];
    uint32_t actual;
    int i;

    do_init();
    for (i = 0; i < (int)DDS_ETH_MAX_PARTICIPANTS; i++) {
        p = make_participant((uint8_t)(i + 1U));
        TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_add_participant(&p));
    }
    /* 表满后 get_active 应该返回全部 */
    actual = 0xFFFF;
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_get_active_participants(list, DDS_ETH_MAX_PARTICIPANTS, &actual));
    TEST_ASSERT_EQUAL_UINT32(DDS_ETH_MAX_PARTICIPANTS, actual);

    /* max_count 较小的情况 */
    actual = 0xFFFF;
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_get_active_participants(list, 5, &actual));
    TEST_ASSERT_EQUAL_UINT32(5, actual);
}

/* ============================================================================
 * RTPS 包处理 — 截断 / 对齐 / 未知子消息
 * ============================================================================ */
static void test_process_submsg_headers_unknown(void)
{
    /* 构造一个带 unknown submessage id 的包 */
    uint8_t pkt[64];
    uint32_t off;

    do_init_started();
    off = rtps_put_header(pkt, PREFIX_REMOTE);
    rtps_put_submsg(&pkt[off], 0x30, 0x01, 4);          /* SEC_BODY — 未知子消息类型 */
    memset(&pkt[off + 4], 0, 4);
    off += 8U;

    stub_push_rx(g_tc_created[0], pkt, off, ETH_IP_ADDR(10, 0, 0, 9), 1234);
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_transport_process(0));
    TEST_ASSERT_EQUAL_INT(0, g_data_cb_cnt);             /* 未触发 DATA 回调 */
}

static void test_process_packet_truncated_submsg(void)
{
    /* 截断的 RTPS 包 — 子消息头都不完整 */
    uint8_t pkt[24];
    uint32_t off;
    dds_eth_transport_stats_t stats;

    do_init_started();
    off = rtps_put_header(pkt, PREFIX_REMOTE);
    /* 只塞 2 字节子消息头 — 不够 sizeof(dds_rtps_submsg_header_t)=4 */
    pkt[off] = DDS_RTPS_SUBMSG_DATA;
    pkt[off + 1] = 0x01;
    stub_push_rx(g_tc_created[0], pkt, off + 2U, ETH_IP_ADDR(10, 0, 0, 8), 1234);
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_transport_process(0));
    TEST_ASSERT_EQUAL_INT(0, g_data_cb_cnt);

    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(1, stats.rx_packets);
}

static void test_process_submsg_alignment(void)
{
    /* 构造 DATA 子消息后 submessage_length 非 4 倍数, 触发对齐逻辑 */
    uint8_t pkt[80];
    uint32_t off;
    dds_eth_transport_stats_t stats;

    do_init_started();
    off = rtps_put_header(pkt, PREFIX_REMOTE);
    /* DATA 子消息 submessage_length=9 (非 4 倍数) */
    rtps_put_submsg(&pkt[off], DDS_RTPS_SUBMSG_DATA, 0x05, 9);
    memset(&pkt[off + 4], 0x5A, 9);
    off += 4U + 9U;
    /* 对齐补齐 */
    if ((off % 4U) != 0U) {
        uint32_t pad = 4U - (off % 4U);
        memset(&pkt[off], 0xEE, pad);
        off += pad;
    }
    /* 再一个 HEARTBEAT, 验证对齐后继续处理 */
    rtps_put_submsg(&pkt[off], DDS_RTPS_SUBMSG_HEARTBEAT, 0x01, 28);
    memset(&pkt[off + 4], 0, 28);
    off += 4U + 28U;

    stub_push_rx(g_tc_created[0], pkt, off, ETH_IP_ADDR(10, 0, 0, 7), 1234);
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_transport_process(0));
    TEST_ASSERT_EQUAL_INT(1, g_data_cb_cnt);
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_get_stats(&stats));
    TEST_ASSERT_EQUAL_UINT32(1, stats.heartbeat_received);
}

/* ============================================================================
 * Participant 租约超时路径 (需要 lease_duration=0 让 current_time-last_seen_ms 立即 > 0)
 * ============================================================================ */
static void test_participant_lease_expiry(void)
{
    /* add participant 时 lease_duration_ms=0: last_seen_ms=0, current_time=0,
     * 但代码检查 (0 - 0) > 0 为 false.
     * 我们用 refresh + 特殊方式触发: 让最后看到的时间为 1 (极小值)
     * 使 (0 - 1) 下溢为 UINT32_MAX > 0.
     *
     * refresh_participant 内部 last_seen_ms = 0, 无法设置为非 0.
     * 但添加时 info.lease_duration_ms 由调用方指定. 如果我们添加 lease=1 的 participant,
     * 然后 refresh 不更新 last_seen... 等等 refresh 总是归零.
     *
     * 实际上 dds_eth_add_participant 后 info.last_seen_ms = 0,
     * 再 refresh_participant 还是 0, 这意味着 current_time - 0 = 0 永远不超 lease.
     *
     * 此路径在产品实现中 current_time 固定为 0, 因此租约超时永远不会触发.
     * 我们通过手工构造 participant 让 last_seen_ms = 1,
     * 需找到能设置它的入口.
     */
    dds_participant_info_t p;
    dds_participant_info_t list[4];
    uint32_t actual;

    do_init_started();
    p = make_participant(7);
    p.lease_duration_ms = 1;                              /* 极短租约 */
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_add_participant(&p));

    /* current_time=0, last_seen_ms=0, 0-0=0, 0>1 是 false. 不触发.
     * 要触发需要 last_seen_ms >= 1. 由于 add 和 refresh 都归零,
     * 这条路走不通 — 属于产品缺陷 (current_time 未实现).
     *
     * 我们通过验证调用 process 后 participant 仍存在来锁定这一行为.
     */
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_transport_process(0));
    actual = 0;
    TEST_ASSERT_EQUAL_INT(ETH_OK, dds_eth_get_active_participants(list, 4, &actual));
    TEST_ASSERT_EQUAL_UINT32(1, actual);                 /* 仍然存在 — 租约超时代码未生效 */
}

/* ============================================================================
 * 额外的 get_active_participants 守卫
 * ============================================================================ */
static void test_get_active_participants_null_guards(void)
{
    dds_participant_info_t list[2];
    uint32_t actual;

    TEST_ASSERT_EQUAL_INT(ETH_NOT_INIT, dds_eth_get_active_participants(list, 2, &actual));
    do_init();
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_get_active_participants(NULL, 2, &actual));
    TEST_ASSERT_EQUAL_INT(ETH_INVALID_PARAM, dds_eth_get_active_participants(list, 2, NULL));
}

/* ============================================================================
 * main
 * ============================================================================ */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_null_config);
    RUN_TEST(test_init_success_and_double_init);
    RUN_TEST(test_init_socket_create_fail);
    RUN_TEST(test_init_second_create_fail_closes_first);
    RUN_TEST(test_start_stop_lifecycle);
    RUN_TEST(test_start_bind_fail);
    RUN_TEST(test_start_multicast_join_fail);
    RUN_TEST(test_start_multicast_bind_fail);
    RUN_TEST(test_register_callbacks_guards);
    RUN_TEST(test_send_rtps_guards);
    RUN_TEST(test_send_rtps_success_unicast);
    RUN_TEST(test_send_rtps_multicast_locator);
    RUN_TEST(test_send_rtps_send_fail);
    RUN_TEST(test_send_rtps_reliable_unsupported);
    RUN_TEST(test_stats_api);
    RUN_TEST(test_endpoint_register_find_unregister);
    RUN_TEST(test_endpoint_guards);
    RUN_TEST(test_endpoint_table_full);
    RUN_TEST(test_endpoint_get_active);
    RUN_TEST(test_participant_add_remove_callbacks);
    RUN_TEST(test_participant_add_existing_updates);
    RUN_TEST(test_participant_guards);
    RUN_TEST(test_participant_table_full);
    RUN_TEST(test_calc_multicast_addr);
    RUN_TEST(test_rtps_header_create);
    RUN_TEST(test_rtps_header_parse_roundtrip);
    RUN_TEST(test_rtps_header_parse_guards);
    RUN_TEST(test_data_encapsulate_decapsulate_roundtrip);
    RUN_TEST(test_data_encapsulate_guards);
    RUN_TEST(test_data_decapsulate_guards);
    RUN_TEST(test_create_heartbeat);
    RUN_TEST(test_create_acknack);
    RUN_TEST(test_process_lifecycle);
    RUN_TEST(test_process_rx_data_callback);
    RUN_TEST(test_process_own_packet_ignored);
    RUN_TEST(test_process_heartbeat_acknack_stats);
    RUN_TEST(test_process_multicast_rx);
    RUN_TEST(test_process_recv_error_counts);
    RUN_TEST(test_process_non_rtps_packet_dropped);
    /* 新增: 多播组独立 API + participant 边界 */
    RUN_TEST(test_join_multicast_group_guards);
    RUN_TEST(test_participant_table_full_get_active);
    RUN_TEST(test_get_active_participants_null_guards);
    /* 新增: RTPS 子消息未知 / 截断 / 对齐 */
    RUN_TEST(test_process_submsg_headers_unknown);
    RUN_TEST(test_process_packet_truncated_submsg);
    RUN_TEST(test_process_submsg_alignment);
    /* 新增: 租约超时路径 */
    RUN_TEST(test_participant_lease_expiry);
    return UNITY_END();
}
