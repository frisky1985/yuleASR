/** @file test_microdds_udp.c
 * @brief microdds UDP 传输层 (src/transport/udp.c) 单元测试
 *
 * 测试策略:
 *  - udp.c 在 macOS/Linux 上走 MICRODDS_PLATFORM_POSIX 路径, 本测试直接使用
 *    真实 127.0.0.1 回环 socket 实测收发往返, 不打 socket 桩。
 *  - 被测模块是单一全局实例 (g_udp_transport), 每个用例使用独立 domain_id
 *    (30000+), 端口 = 7400+domain / 7410+domain, 用例间互不冲突;
 *    tearDown 统一 Shutdown + 复位平台钩子保证用例独立。
 *  - PDP 发现报文按 udp.c 内部结构 (16B header + 16B payload) 逐字节构造。
 *
 * NOTE(产品缺陷记录, 只记录不修改产品代码):
 *  1. udp.c 未 include 任何定义 NULL_PTR 的头文件, 无法独立编译;
 *     测试构建用 -DNULL_PTR=((void*)0) 注入。
 *  2. udp.c SendMulticast 的默认多播地址宏 0xEFFF0001U 未做主机序→网络序
 *     转换 (见 test_send_multicast), 小端机上实际发往 1.0.255.239 而非
 *     239.255.0.1; 且 transport.h 中同名宏是字符串, 两处语义不一致。
 *  3. udp.c MicroDDS_UDP_Send 用 (unsigned int)(sent) > 0 判定成功, sendto
 *     返回 -1 时被误判为成功并计入 bytes_sent。
 *  4. MicroDDS_UDP_Receive 缓冲区不足时按 recvfrom 语义截断 (未用 MSG_TRUNC),
 *     调用方无法得知原始包长 (见 test_receive_truncates_to_buffer_size)。
 *  5. MicroDDS_UDP_AddPeer 不校验地址/端口, 0.0.0.0:0 也接受入表
 *     (见 test_peer_management)。
 *  6. udp.c 的 sleep_ms/platform_sleep_ms 钩子在模块内从未被调用 (死代码)。
 */

#include "unity.h"
#include "microdds/transport.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

/* ============================================================================
 * 与 udp.c 内部约定对齐的常量
 * ============================================================================ */

#define UDP_PORT_BASE      7400U  /* udp.c MICRODDS_UDP_PORT_BASE */
#define USER_PORT_OFFSET   10U    /* udp.c MICRODDS_UDP_USER_PORT_OFFSET */
#define MAX_PAYLOAD        1472U  /* udp.c MICRODDS_UDP_MAX_PAYLOAD */
#define MAX_PEERS          16U    /* udp.c MICRODDS_MAX_PEERS */
#define PDP_TOTAL_LEN      32U    /* sizeof(PDP_Header) + sizeof(PDP_AnnouncePayload) */
#define PDP_HDR_LEN        16U    /* sizeof(PDP_Header) */
#define PDP_MSG_ANNOUNCE   0x01U
#define LOOPBACK_STR       "127.0.0.1"

static uint16_t discovery_port_of(uint32_t domain)
{
    return (uint16_t)(UDP_PORT_BASE + domain);
}

static uint16_t user_port_of(uint32_t domain)
{
    return (uint16_t)(UDP_PORT_BASE + USER_PORT_OFFSET + domain);
}

/* ============================================================================
 * 测试辅助: 真实回环 socket
 * ============================================================================ */

static int make_udp_socket(void)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    TEST_ASSERT_TRUE(fd >= 0);
    /* 接收辅助 socket 阻塞等待上限 1s, 防止测试悬挂 */
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    (void)setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    return fd;
}

static uint16_t get_bound_port(int fd)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    TEST_ASSERT_TRUE(getsockname(fd, (struct sockaddr*)&addr, &len) == 0);
    return ntohs(addr.sin_port);
}

/* 绑定到指定地址/端口 (port=0 时系统分配临时端口), 返回实际端口 */
static uint16_t bind_udp(int fd, uint32_t ip_net_order, uint16_t port)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip_net_order;
    addr.sin_port = htons(port);
    TEST_ASSERT_TRUE(bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0);
    if (port != 0U) {
        return port;
    }
    return get_bound_port(fd);
}

/* 从测试 socket 发送数据到 127.0.0.1:dest_port */
static void send_raw_to(int fd, const uint8_t* data, uint16_t len, uint16_t dest_port)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(LOOPBACK_STR);
    addr.sin_port = htons(dest_port);
    ssize_t n = sendto(fd, data, len, 0, (struct sockaddr*)&addr, sizeof(addr));
    TEST_ASSERT_EQUAL_INT((int)len, (int)n);
}

/* 循环调用被测 Receive 直到用户套接字收到数据 (防回环投递竞态) */
static uint16_t wait_user_packet(uint8_t* buf, uint16_t size,
                                 uint32_t* src, uint16_t* port, int budget_ms)
{
    int waited = 0;
    while (waited <= budget_ms) {
        uint16_t n = MicroDDS_UDP_Receive(buf, size, src, port, 20U);
        if (n > 0U) {
            return n;
        }
        waited += 20;
    }
    return 0U;
}

/* 循环调用被测 Receive 直到活跃参与者数达到 target (PDP 消费路径) */
static void pump_pdp_until_count(uint32_t target, int max_tries)
{
    uint8_t scratch[64];
    int i = 0;
    while ((i++ < max_tries) &&
           (MicroDDS_UDP_GetDiscoveredParticipantCount() < target)) {
        (void)MicroDDS_UDP_Receive(scratch, sizeof(scratch), NULL, NULL, 20U);
    }
}

/* 固定次数空转 Receive, 用于消化发现套接字上的报文 */
static void pump_receive(int times, uint32_t timeout_each)
{
    uint8_t scratch[64];
    for (int i = 0; i < times; i++) {
        (void)MicroDDS_UDP_Receive(scratch, sizeof(scratch), NULL, NULL, timeout_each);
    }
}

/* ============================================================================
 * 测试辅助: PDP 宣告报文逐字节构造 (与 udp.c 内部布局一致)
 * ============================================================================ */

static void put_be16(uint8_t* p, uint16_t v)
{
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)(v & 0xFFU);
}

static void put_be32(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)((v >> 16) & 0xFFU);
    p[2] = (uint8_t)((v >> 8) & 0xFFU);
    p[3] = (uint8_t)(v & 0xFFU);
}

static uint16_t build_pdp_announce(uint8_t* out, uint32_t participant_id,
                                   uint32_t domain_id, uint16_t disc_port,
                                   uint16_t user_port, const char* payload_ip)
{
    memset(out, 0, PDP_TOTAL_LEN);
    out[0] = 'P';
    out[1] = 'D';
    out[2] = 'P';
    out[3] = 0;
    out[4] = 1U;                       /* version */
    out[5] = (uint8_t)PDP_MSG_ANNOUNCE;
    put_be16(&out[6], 16U);            /* payload_length */
    put_be32(&out[8], participant_id);
    put_be32(&out[12], 0U);            /* timestamp */
    /* payload.address: 按网络序 4 字节写入 (与发送方 s_addr 内存表示一致) */
    struct in_addr a;
    TEST_ASSERT_TRUE(inet_aton(payload_ip, &a) != 0);
    memcpy(&out[16], &a.s_addr, 4U);
    put_be16(&out[20], disc_port);
    put_be16(&out[22], user_port);
    put_be32(&out[24], domain_id);
    put_be32(&out[28], 10000U);        /* lease_duration */
    return (uint16_t)PDP_TOTAL_LEN;
}

/* ============================================================================
 * 平台钩子桩 (可注入固定时间)
 * ============================================================================ */

static uint32_t g_fake_now_ms;

static uint32_t fake_get_time_ms(void)
{
    return g_fake_now_ms;
}

static void fake_sleep_ms(uint32_t ms)
{
    (void)ms;  /* NOTE: udp.c 从不调用 sleep 钩子 (死代码), 此处仅为签名完整 */
}

/* ============================================================================
 * Unity 钩子: 每个用例后强制复位全局传输状态
 * ============================================================================ */

void setUp(void)
{
}

void tearDown(void)
{
    MicroDDS_UDP_SetPlatformHooks(NULL, NULL);
    MicroDDS_UDP_Shutdown();
}

/* ============================================================================
 * 用例
 * ============================================================================ */

/* 未初始化状态下所有 API 的守卫行为 */
static void test_uninit_api_guards(void)
{
    MicroDDS_UDP_Shutdown();  /* 确保干净状态 (幂等空操作) */
    uint8_t buf[64];
    uint32_t src = 0;
    uint16_t port = 0;

    TEST_ASSERT_FALSE(MicroDDS_UDP_IsInitialized());
    TEST_ASSERT_FALSE(MicroDDS_UDP_JoinMulticast("239.255.0.1"));
    TEST_ASSERT_FALSE(MicroDDS_UDP_LeaveMulticast("239.255.0.1"));
    TEST_ASSERT_FALSE(MicroDDS_UDP_AddPeer(0x7F000001U, 9000));
    TEST_ASSERT_FALSE(MicroDDS_UDP_RemovePeer(0x7F000001U, 9000));
    TEST_ASSERT_FALSE(MicroDDS_UDP_SendTo(buf, 4, inet_addr(LOOPBACK_STR), 9000));
    TEST_ASSERT_FALSE(MicroDDS_UDP_Send(buf, 4));
    TEST_ASSERT_FALSE(MicroDDS_UDP_SendMulticast(buf, 4, "239.255.0.1"));
    TEST_ASSERT_EQUAL_UINT(0, MicroDDS_UDP_Receive(buf, sizeof(buf), &src, &port, 0));
    TEST_ASSERT_EQUAL_UINT(0, MicroDDS_UDP_GetPeerCount());
    TEST_ASSERT_EQUAL_UINT(0, MicroDDS_UDP_GetDiscoveryPort());
    TEST_ASSERT_EQUAL_UINT(0, MicroDDS_UDP_GetUserPort());
    TEST_ASSERT_EQUAL_UINT(0, MicroDDS_UDP_GetParticipantId());
    TEST_ASSERT_EQUAL_UINT(0, MicroDDS_UDP_GetDiscoveredParticipantCount());
    TEST_ASSERT_FALSE(MicroDDS_UDP_GetDiscoveredParticipant(0, NULL, NULL, NULL));

    uint32_t s1, s2, s3, s4, s5;
    MicroDDS_UDP_GetStats(&s1, &s2, &s3, &s4, &s5);
    TEST_ASSERT_EQUAL_UINT32(0, s1);
    TEST_ASSERT_EQUAL_UINT32(0, s2);
    TEST_ASSERT_EQUAL_UINT32(0, s3);
    TEST_ASSERT_EQUAL_UINT32(0, s4);
    TEST_ASSERT_EQUAL_UINT32(0, s5);
    MicroDDS_UDP_GetStats(NULL, NULL, NULL, NULL, NULL);  /* 全 NULL 不崩溃 */
}

/* Init/Shutdown 生命周期 + 重复 Init 幂等 */
static void test_init_shutdown_lifecycle(void)
{
    TEST_ASSERT_TRUE(MicroDDS_UDP_Init(30000));
    TEST_ASSERT_TRUE(MicroDDS_UDP_IsInitialized());
    TEST_ASSERT_EQUAL_UINT(discovery_port_of(30000), MicroDDS_UDP_GetDiscoveryPort());
    TEST_ASSERT_EQUAL_UINT(user_port_of(30000), MicroDDS_UDP_GetUserPort());
    TEST_ASSERT_TRUE(MicroDDS_UDP_GetParticipantId() != 0U);

    /* 已初始化时再次 Init 幂等返回 true */
    TEST_ASSERT_TRUE(MicroDDS_UDP_Init(30000));
    TEST_ASSERT_TRUE(MicroDDS_UDP_IsInitialized());

    MicroDDS_UDP_Shutdown();
    TEST_ASSERT_FALSE(MicroDDS_UDP_IsInitialized());
    MicroDDS_UDP_Shutdown();  /* 未初始化 Shutdown 空操作, 不崩溃 */
}

/* Shutdown 后同 domain 重新 Init 成功 (端口已释放) */
static void test_reinit_after_shutdown(void)
{
    TEST_ASSERT_TRUE(MicroDDS_UDP_Init(30001));
    MicroDDS_UDP_Shutdown();
    TEST_ASSERT_TRUE(MicroDDS_UDP_Init(30001));
    TEST_ASSERT_TRUE(MicroDDS_UDP_IsInitialized());
    MicroDDS_UDP_Shutdown();
}

/* 发现端口被占用 → Init 失败且状态保持未初始化 */
static void test_init_bind_conflict_discovery_port(void)
{
    int blocker = make_udp_socket();
    /* INADDR_ANY 绑定且不设 SO_REUSEADDR, 模拟端口占用 */
    (void)bind_udp(blocker, htonl(INADDR_ANY), discovery_port_of(30002));

    TEST_ASSERT_FALSE(MicroDDS_UDP_Init(30002));
    TEST_ASSERT_FALSE(MicroDDS_UDP_IsInitialized());

    TEST_ASSERT_TRUE(close(blocker) == 0);
    TEST_ASSERT_TRUE(MicroDDS_UDP_Init(30002));  /* 释放后可成功 */
    MicroDDS_UDP_Shutdown();
}

/* 用户端口占用导致 Init 在第二步失败 → 失败路径必须关闭已绑定的发现套接字 */
static void test_init_failure_cleans_sockets(void)
{
    int blocker = make_udp_socket();
    (void)bind_udp(blocker, htonl(INADDR_ANY), user_port_of(30012));

    /* 发现端口绑定成功、用户端口绑定失败 → Init 返回 false */
    TEST_ASSERT_FALSE(MicroDDS_UDP_Init(30012));
    TEST_ASSERT_FALSE(MicroDDS_UDP_IsInitialized());
    TEST_ASSERT_TRUE(close(blocker) == 0);

    /* NOTE(实测): 若失败路径未关闭已绑定 7400+30012 的发现套接字,
     * 此处重新 Init 会因端口占用而失败 — 以此证明清理逻辑生效 */
    TEST_ASSERT_TRUE(MicroDDS_UDP_Init(30012));
    MicroDDS_UDP_Shutdown();
}

/* 平台时间钩子: participant_id = get_time_ms() ^ (domain_id << 16) */
static void test_platform_hooks_participant_id(void)
{
    g_fake_now_ms = 123456U;
    MicroDDS_UDP_SetPlatformHooks(fake_get_time_ms, fake_sleep_ms);
    TEST_ASSERT_TRUE(MicroDDS_UDP_Init(30090));
    TEST_ASSERT_EQUAL_UINT32(123456U ^ (30090U << 16),
                             MicroDDS_UDP_GetParticipantId());
    MicroDDS_UDP_Shutdown();

    /* 恢复真实时钟后仍可正常初始化 */
    MicroDDS_UDP_SetPlatformHooks(NULL, NULL);
    TEST_ASSERT_TRUE(MicroDDS_UDP_Init(30091));
    TEST_ASSERT_TRUE(MicroDDS_UDP_GetParticipantId() != 0U);
    MicroDDS_UDP_Shutdown();
}

/* SendTo 真实回环往返 + NULL/超长守卫 + 统计 */
static void test_sendto_loopback_roundtrip(void)
{
    TEST_ASSERT_TRUE(MicroDDS_UDP_Init(30010));
    int rx = make_udp_socket();
    uint16_t rx_port = bind_udp(rx, inet_addr(LOOPBACK_STR), 0);
    MicroDDS_UDP_ClearStats();

    /* 守卫: NULL 数据 */
    TEST_ASSERT_FALSE(MicroDDS_UDP_SendTo(NULL, 4, inet_addr(LOOPBACK_STR), rx_port));

    /* 守卫: 负载超过 1472 → 拒绝并计入 dropped */
    uint8_t big[MAX_PAYLOAD + 1U];
    memset(big, 0xA5, sizeof(big));
    TEST_ASSERT_FALSE(MicroDDS_UDP_SendTo(big, (uint16_t)sizeof(big),
                                          inet_addr(LOOPBACK_STR), rx_port));
    uint32_t s1, s2, s3, s4, s5;
    MicroDDS_UDP_GetStats(&s1, &s2, &s3, &s4, &s5);
    TEST_ASSERT_EQUAL_UINT32(0, s1);
    TEST_ASSERT_EQUAL_UINT32(1, s3);

    /* 正常发送 → 回环接收 */
    uint8_t payload[5] = {'U', 'D', 'P', '!', '?'};
    TEST_ASSERT_TRUE(MicroDDS_UDP_SendTo(payload, 5, inet_addr(LOOPBACK_STR), rx_port));

    uint8_t rbuf[64];
    struct sockaddr_in from;
    socklen_t flen = sizeof(from);
    ssize_t n = recvfrom(rx, rbuf, sizeof(rbuf), 0, (struct sockaddr*)&from, &flen);
    TEST_ASSERT_EQUAL_INT(5, (int)n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, rbuf, 5);
    /* 发送套接字已绑定用户端口 → 回环包源端口即用户端口 */
    TEST_ASSERT_EQUAL_UINT(user_port_of(30010), ntohs(from.sin_port));

    MicroDDS_UDP_GetStats(&s1, &s2, &s3, &s4, &s5);
    /* NOTE(实测): 仅 1 次真实成功发送计数; NULL/超长守卫不计入 packets_sent */
    TEST_ASSERT_EQUAL_UINT32(1, s1);
    TEST_ASSERT_EQUAL_UINT32(0, s2);
    TEST_ASSERT_EQUAL_UINT32(1, s3);
    TEST_ASSERT_EQUAL_UINT32(5, s4);

    TEST_ASSERT_TRUE(close(rx) == 0);
    MicroDDS_UDP_Shutdown();
}

/* Receive 真实回环往返: 载荷/源地址(网络序)/源端口(主机序)/统计/NULL 输出参数 */
static void test_receive_loopback(void)
{
    TEST_ASSERT_TRUE(MicroDDS_UDP_Init(30020));
    int tx = make_udp_socket();
    uint16_t tx_port = bind_udp(tx, inet_addr(LOOPBACK_STR), 0);

    uint8_t payload[8] = {'R', 'X', 'T', 'E', 'S', 'T', '0', '1'};
    send_raw_to(tx, payload, 8, user_port_of(30020));

    uint8_t rbuf[64];
    uint32_t src = 0;
    uint16_t sport = 0;
    uint16_t n = wait_user_packet(rbuf, sizeof(rbuf), &src, &sport, 200);
    TEST_ASSERT_EQUAL_UINT(8, n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, rbuf, 8);
    TEST_ASSERT_EQUAL_UINT32(inet_addr(LOOPBACK_STR), src);  /* 源地址为网络序 */
    TEST_ASSERT_EQUAL_UINT(tx_port, sport);                  /* 源端口为主机序 */

    uint32_t s1, s2, s3, s4, s5;
    MicroDDS_UDP_GetStats(&s1, &s2, &s3, &s4, &s5);
    TEST_ASSERT_EQUAL_UINT32(1, s2);
    TEST_ASSERT_EQUAL_UINT32(8, s5);

    /* src/port 输出参数允许为 NULL */
    send_raw_to(tx, payload, 8, user_port_of(30020));
    n = wait_user_packet(rbuf, sizeof(rbuf), NULL, NULL, 200);
    TEST_ASSERT_EQUAL_UINT(8, n);

    /* 守卫: NULL 缓冲区 */
    TEST_ASSERT_EQUAL_UINT(0, MicroDDS_UDP_Receive(NULL, sizeof(rbuf), &src, &sport, 0));

    TEST_ASSERT_TRUE(close(tx) == 0);
    MicroDDS_UDP_Shutdown();
}

/* 无数据时的超时行为: timeout=0 非阻塞, timeout=60 走 select 超时路径 */
static void test_receive_timeout_behavior(void)
{
    TEST_ASSERT_TRUE(MicroDDS_UDP_Init(30021));
    uint8_t buf[64];
    uint32_t src = 0;
    uint16_t sport = 0;

    /* timeout=0: 无数据立即返回 0 */
    TEST_ASSERT_EQUAL_UINT(0, MicroDDS_UDP_Receive(buf, sizeof(buf), &src, &sport, 0));

    /* timeout=60: 阻塞约 60ms 后返回 0 */
    struct timeval t0, t1;
    gettimeofday(&t0, NULL);
    TEST_ASSERT_EQUAL_UINT(0, MicroDDS_UDP_Receive(buf, sizeof(buf), &src, &sport, 60U));
    gettimeofday(&t1, NULL);
    long elapsed_ms = (t1.tv_sec - t0.tv_sec) * 1000L +
                      (t1.tv_usec - t0.tv_usec) / 1000L;
    /* select 至少等待 timeout_ms (留 5ms 调度余量), 上限放宽防 CI 抖动 */
    TEST_ASSERT_TRUE(elapsed_ms >= 55);
    TEST_ASSERT_TRUE(elapsed_ms < 1000);

    MicroDDS_UDP_Shutdown();
}

/* 缓冲区边界: 接收缓冲区不足时按 recvfrom 语义截断 */
static void test_receive_truncates_to_buffer_size(void)
{
    TEST_ASSERT_TRUE(MicroDDS_UDP_Init(30022));
    int tx = make_udp_socket();
    (void)bind_udp(tx, inet_addr(LOOPBACK_STR), 0);

    uint8_t payload[64];
    memset(payload, 0x5A, sizeof(payload));
    send_raw_to(tx, payload, 64, user_port_of(30022));

    uint8_t rbuf[16];
    uint16_t n = wait_user_packet(rbuf, sizeof(rbuf), NULL, NULL, 200);
    /* NOTE(实测): 返回截断后的 16 字节, 剩余 48 字节被丢弃,
     * 调用方无法得知原始包长 (缺陷记录 4) */
    TEST_ASSERT_EQUAL_UINT(16, n);
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT_EQUAL_UINT8(0x5A, rbuf[i]);
    }

    TEST_ASSERT_TRUE(close(tx) == 0);
    MicroDDS_UDP_Shutdown();
}

/* 对端管理: 去重/移除/不存在/容量上限/Shutdown 清空 */
static void test_peer_management(void)
{
    TEST_ASSERT_TRUE(MicroDDS_UDP_Init(30030));
    uint32_t host_ip = ntohl(inet_addr(LOOPBACK_STR));  /* AddPeer 使用主机序 */

    TEST_ASSERT_TRUE(MicroDDS_UDP_AddPeer(host_ip, 9000));
    TEST_ASSERT_EQUAL_UINT(1, MicroDDS_UDP_GetPeerCount());
    /* 重复添加: 返回 true 且不重复计数 */
    TEST_ASSERT_TRUE(MicroDDS_UDP_AddPeer(host_ip, 9000));
    TEST_ASSERT_EQUAL_UINT(1, MicroDDS_UDP_GetPeerCount());

    TEST_ASSERT_TRUE(MicroDDS_UDP_AddPeer(host_ip, 9001));
    TEST_ASSERT_EQUAL_UINT(2, MicroDDS_UDP_GetPeerCount());

    /* NOTE(实测): 无地址/端口合法性校验, 0.0.0.0:0 也接受入表 (缺陷记录 5) */
    TEST_ASSERT_TRUE(MicroDDS_UDP_AddPeer(0U, 0U));
    TEST_ASSERT_EQUAL_UINT(3, MicroDDS_UDP_GetPeerCount());
    TEST_ASSERT_TRUE(MicroDDS_UDP_RemovePeer(0U, 0U));
    TEST_ASSERT_EQUAL_UINT(2, MicroDDS_UDP_GetPeerCount());

    /* 移除不存在的对端 */
    TEST_ASSERT_FALSE(MicroDDS_UDP_RemovePeer(0x0A000001U, 1234));
    TEST_ASSERT_EQUAL_UINT(2, MicroDDS_UDP_GetPeerCount());

    TEST_ASSERT_TRUE(MicroDDS_UDP_RemovePeer(host_ip, 9000));
    TEST_ASSERT_TRUE(MicroDDS_UDP_RemovePeer(host_ip, 9001));
    TEST_ASSERT_EQUAL_UINT(0, MicroDDS_UDP_GetPeerCount());

    /* 容量上限 16, 第 17 个被拒绝 */
    for (uint32_t i = 0; i < MAX_PEERS; i++) {
        TEST_ASSERT_TRUE(MicroDDS_UDP_AddPeer(host_ip, (uint16_t)(20000U + i)));
    }
    TEST_ASSERT_EQUAL_UINT(MAX_PEERS, MicroDDS_UDP_GetPeerCount());
    TEST_ASSERT_FALSE(MicroDDS_UDP_AddPeer(host_ip, 30000));
    TEST_ASSERT_EQUAL_UINT(MAX_PEERS, MicroDDS_UDP_GetPeerCount());

    MicroDDS_UDP_Shutdown();
    /* Shutdown 后对端表清零, API 拒绝 */
    TEST_ASSERT_EQUAL_UINT(0, MicroDDS_UDP_GetPeerCount());
    TEST_ASSERT_FALSE(MicroDDS_UDP_AddPeer(host_ip, 1));
}

/* Send 组播到所有对端: 无对端 false / 真实回环往返 */
static void test_send_to_peers_loopback(void)
{
    TEST_ASSERT_TRUE(MicroDDS_UDP_Init(30040));
    uint8_t payload[6] = {'P', 'E', 'E', 'R', 'S', '1'};

    /* 无对端时 Send 返回 false */
    TEST_ASSERT_FALSE(MicroDDS_UDP_Send(payload, 6));

    int rx = make_udp_socket();
    uint16_t rx_port = bind_udp(rx, inet_addr(LOOPBACK_STR), 0);
    TEST_ASSERT_TRUE(MicroDDS_UDP_AddPeer(ntohl(inet_addr(LOOPBACK_STR)), rx_port));
    TEST_ASSERT_TRUE(MicroDDS_UDP_Send(payload, 6));

    uint8_t rbuf[32];
    ssize_t n = recv(rx, rbuf, sizeof(rbuf), 0);
    TEST_ASSERT_EQUAL_INT(6, (int)n);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, rbuf, 6);

    TEST_ASSERT_TRUE(close(rx) == 0);
    MicroDDS_UDP_Shutdown();
}

/* 多播组加入/离开: 守卫/去重/容量 4/重复离开/未加入离开 */
static void test_multicast_join_leave(void)
{
    TEST_ASSERT_TRUE(MicroDDS_UDP_Init(30050));

    /* 守卫 */
    TEST_ASSERT_FALSE(MicroDDS_UDP_JoinMulticast(NULL));
    TEST_ASSERT_FALSE(MicroDDS_UDP_JoinMulticast("not-an-ip"));
    TEST_ASSERT_FALSE(MicroDDS_UDP_JoinMulticast("300.1.1.1"));
    /* 非多播地址被拒绝 */
    TEST_ASSERT_FALSE(MicroDDS_UDP_JoinMulticast("127.0.0.1"));

    /* 加入 4 个多播组 (容量上限 4) */
    TEST_ASSERT_TRUE(MicroDDS_UDP_JoinMulticast("239.255.0.1"));
    /* 重复加入: 返回 true 且不占新槽位 */
    TEST_ASSERT_TRUE(MicroDDS_UDP_JoinMulticast("239.255.0.1"));
    TEST_ASSERT_TRUE(MicroDDS_UDP_JoinMulticast("239.255.0.2"));
    TEST_ASSERT_TRUE(MicroDDS_UDP_JoinMulticast("239.255.0.3"));
    TEST_ASSERT_TRUE(MicroDDS_UDP_JoinMulticast("239.255.0.4"));
    /* 第 5 组超出容量 */
    TEST_ASSERT_FALSE(MicroDDS_UDP_JoinMulticast("239.255.0.5"));

    /* 离开未加入的组 */
    TEST_ASSERT_FALSE(MicroDDS_UDP_LeaveMulticast("239.255.0.5"));
    TEST_ASSERT_FALSE(MicroDDS_UDP_LeaveMulticast("224.0.0.99"));

    /* 正常离开后槽位释放, 可再加入 */
    TEST_ASSERT_TRUE(MicroDDS_UDP_LeaveMulticast("239.255.0.2"));
    TEST_ASSERT_FALSE(MicroDDS_UDP_LeaveMulticast("239.255.0.2"));  /* 重复离开 */
    TEST_ASSERT_TRUE(MicroDDS_UDP_JoinMulticast("239.255.0.5"));

    MicroDDS_UDP_Shutdown();
    TEST_ASSERT_FALSE(MicroDDS_UDP_JoinMulticast("239.255.0.1"));
}

/* 多播发送: 守卫路径 + 显式多播组发送 + 默认地址 (缺陷记录 2) */
static void test_send_multicast(void)
{
    TEST_ASSERT_TRUE(MicroDDS_UDP_Init(30060));
    uint8_t payload[4] = {1, 2, 3, 4};

    /* 守卫 */
    TEST_ASSERT_FALSE(MicroDDS_UDP_SendMulticast(NULL, 4, "239.255.0.1"));
    TEST_ASSERT_FALSE(MicroDDS_UDP_SendMulticast(payload, 4, "not-an-ip"));

    /* 显式合法多播组: sendto 结果取决于宿主机多播路由。
     * NOTE(实测): 本沙箱 en0 无有效链路, sendto 239.255.0.7 返回
     * EHOSTUNREACH(65) → 函数返回 false; 在有活动多播路由的宿主机上
     * 返回 true。两种结果均为产品对 sendto 的忠实转发, 不做硬断言,
     * 仅记录结果并验证调用不崩溃 (缺陷记录 2 仍独立成立)。 */
    bool explicit_group_result = MicroDDS_UDP_SendMulticast(payload, 4, "239.255.0.7");
    printf("      [info] SendMulticast(239.255.0.7) returned %s "
           "(true if host has active multicast route)\n",
           explicit_group_result ? "true" : "false");

    /* NULL 组地址 → 使用内部默认地址 0xEFFF0001U。
     * NOTE(缺陷记录 2): 该常量未做主机序→网络序转换, 小端机上实际发往
     * 1.0.255.239 而非 239.255.0.1; 能否发送成功取决于宿主机路由,
     * 因此这里只验证函数可调用且不崩溃 (结果记录于测试日志)。 */
    bool default_addr_result = MicroDDS_UDP_SendMulticast(payload, 4, NULL);
    printf("      [info] SendMulticast(NULL group) returned %s "
           "(default addr is byte-swapped on LE, see defect note 2)\n",
           default_addr_result ? "true" : "false");

    MicroDDS_UDP_Shutdown();
    TEST_ASSERT_FALSE(MicroDDS_UDP_SendMulticast(payload, 4, "239.255.0.1"));
}

/* PDP 发现: 宣告处理/字段解析/自动加对端/条目更新/非法报文过滤 */
static void test_pdp_announce_discovery(void)
{
    const uint32_t dom = 30070;
    TEST_ASSERT_TRUE(MicroDDS_UDP_Init(dom));
    uint32_t local_pid = MicroDDS_UDP_GetParticipantId();
    TEST_ASSERT_EQUAL_UINT(0, MicroDDS_UDP_GetDiscoveredParticipantCount());

    int tx = make_udp_socket();
    (void)bind_udp(tx, inet_addr(LOOPBACK_STR), 0);

    uint8_t pkt[PDP_TOTAL_LEN];
    uint32_t pid = 0, addr = 0;
    uint16_t port = 0;

    /* 1. 合法宣告 → 参与者表新增 + 自动加入对端表 */
    (void)build_pdp_announce(pkt, 0x11112222U, dom,
                             discovery_port_of(dom), 9100, LOOPBACK_STR);
    send_raw_to(tx, pkt, PDP_TOTAL_LEN, discovery_port_of(dom));
    pump_pdp_until_count(1, 50);
    TEST_ASSERT_EQUAL_UINT(1, MicroDDS_UDP_GetDiscoveredParticipantCount());
    TEST_ASSERT_TRUE(MicroDDS_UDP_GetDiscoveredParticipant(0, &pid, &addr, &port));
    TEST_ASSERT_EQUAL_UINT32(0x11112222U, pid);
    TEST_ASSERT_EQUAL_UINT32(inet_addr(LOOPBACK_STR), addr);  /* 网络序原样返回 */
    TEST_ASSERT_EQUAL_UINT(9100, port);                       /* 主机序 user_port */
    TEST_ASSERT_EQUAL_UINT(1, MicroDDS_UDP_GetPeerCount());

    /* 2. 相同 participant_id 的第二次宣告: 更新条目而非新增 */
    (void)build_pdp_announce(pkt, 0x11112222U, dom,
                             discovery_port_of(dom), 9101, LOOPBACK_STR);
    send_raw_to(tx, pkt, PDP_TOTAL_LEN, discovery_port_of(dom));
    pump_receive(5, 20U);
    TEST_ASSERT_EQUAL_UINT(1, MicroDDS_UDP_GetDiscoveredParticipantCount());
    TEST_ASSERT_TRUE(MicroDDS_UDP_GetDiscoveredParticipant(0, &pid, &addr, &port));
    TEST_ASSERT_EQUAL_UINT(9101, port);
    /* NOTE(实测): 更新后的 user_port 会作为新对端 (127.0.0.1:9101) 入表 */
    TEST_ASSERT_EQUAL_UINT(2, MicroDDS_UDP_GetPeerCount());

    /* 3. 过滤: 本地 participant_id 的宣告被忽略 */
    (void)build_pdp_announce(pkt, local_pid, dom,
                             discovery_port_of(dom), 9200, LOOPBACK_STR);
    send_raw_to(tx, pkt, PDP_TOTAL_LEN, discovery_port_of(dom));
    pump_receive(5, 20U);
    TEST_ASSERT_EQUAL_UINT(1, MicroDDS_UDP_GetDiscoveredParticipantCount());
    /* 管道有效性验证: 随后合法宣告仍能入表, 证明上面是过滤而非通道失效 */
    (void)build_pdp_announce(pkt, 0x22223333U, dom,
                             discovery_port_of(dom), 9203, LOOPBACK_STR);
    send_raw_to(tx, pkt, PDP_TOTAL_LEN, discovery_port_of(dom));
    pump_pdp_until_count(2, 50);
    TEST_ASSERT_EQUAL_UINT(2, MicroDDS_UDP_GetDiscoveredParticipantCount());

    /* 4. 过滤: 其他 domain 的宣告被忽略 */
    (void)build_pdp_announce(pkt, 0x33334444U, dom + 1U,
                             discovery_port_of(dom), 9201, LOOPBACK_STR);
    send_raw_to(tx, pkt, PDP_TOTAL_LEN, discovery_port_of(dom));
    pump_receive(5, 20U);
    TEST_ASSERT_EQUAL_UINT(2, MicroDDS_UDP_GetDiscoveredParticipantCount());
    (void)build_pdp_announce(pkt, 0x44445555U, dom,
                             discovery_port_of(dom), 9204, LOOPBACK_STR);
    send_raw_to(tx, pkt, PDP_TOTAL_LEN, discovery_port_of(dom));
    pump_pdp_until_count(3, 50);
    TEST_ASSERT_EQUAL_UINT(3, MicroDDS_UDP_GetDiscoveredParticipantCount());

    /* 5. 过滤: 魔数错误的宣告被忽略 */
    (void)build_pdp_announce(pkt, 0x55556666U, dom,
                             discovery_port_of(dom), 9202, LOOPBACK_STR);
    pkt[0] = 'X';
    send_raw_to(tx, pkt, PDP_TOTAL_LEN, discovery_port_of(dom));
    pump_receive(5, 20U);
    TEST_ASSERT_EQUAL_UINT(3, MicroDDS_UDP_GetDiscoveredParticipantCount());

    /* 6. 过滤: 长度不足 (只有 16B header, 缺 announce payload) */
    (void)build_pdp_announce(pkt, 0x66667777U, dom,
                             discovery_port_of(dom), 9205, LOOPBACK_STR);
    send_raw_to(tx, pkt, (uint16_t)PDP_HDR_LEN, discovery_port_of(dom));
    pump_receive(5, 20U);
    TEST_ASSERT_EQUAL_UINT(3, MicroDDS_UDP_GetDiscoveredParticipantCount());

    /* 7. 边界: 越界索引 / NULL 输出参数 */
    TEST_ASSERT_FALSE(MicroDDS_UDP_GetDiscoveredParticipant(3, &pid, &addr, &port));
    TEST_ASSERT_TRUE(MicroDDS_UDP_GetDiscoveredParticipant(0, NULL, NULL, NULL));

    TEST_ASSERT_TRUE(close(tx) == 0);
    MicroDDS_UDP_Shutdown();
}

/* PDP 租约: 时间推进超过 10s 后参与者被标记不活跃 (借平台时间钩子注入) */
static void test_pdp_lease_expiry(void)
{
    const uint32_t dom = 30085;
    g_fake_now_ms = 1000U;
    MicroDDS_UDP_SetPlatformHooks(fake_get_time_ms, fake_sleep_ms);
    TEST_ASSERT_TRUE(MicroDDS_UDP_Init(dom));

    int tx = make_udp_socket();
    (void)bind_udp(tx, inet_addr(LOOPBACK_STR), 0);

    uint8_t pkt[PDP_TOTAL_LEN];
    (void)build_pdp_announce(pkt, 0x77778888U, dom,
                             discovery_port_of(dom), 9300, LOOPBACK_STR);
    send_raw_to(tx, pkt, PDP_TOTAL_LEN, discovery_port_of(dom));
    pump_pdp_until_count(1, 50);
    TEST_ASSERT_EQUAL_UINT(1, MicroDDS_UDP_GetDiscoveredParticipantCount());

    /* 假时钟推进 10.5s (> 租约 10s): 下一次 Receive 的 PDP 维护将其置为不活跃 */
    g_fake_now_ms = 1000U + 10500U;
    uint8_t scratch[64];
    (void)MicroDDS_UDP_Receive(scratch, sizeof(scratch), NULL, NULL, 0);
    TEST_ASSERT_EQUAL_UINT(0, MicroDDS_UDP_GetDiscoveredParticipantCount());
    /* NOTE(实测): 条目仍占用表槽位 (is_active=false), 按 index 访问返回 false */
    TEST_ASSERT_FALSE(MicroDDS_UDP_GetDiscoveredParticipant(0, NULL, NULL, NULL));

    TEST_ASSERT_TRUE(close(tx) == 0);
    MicroDDS_UDP_Shutdown();
}

/* 统计: 收发/字节累计 + ClearStats + 超长丢弃计数 */
static void test_stats_and_clear(void)
{
    TEST_ASSERT_TRUE(MicroDDS_UDP_Init(30080));
    MicroDDS_UDP_ClearStats();

    int rx = make_udp_socket();
    uint16_t rx_port = bind_udp(rx, inet_addr(LOOPBACK_STR), 0);

    uint8_t p10[10];
    uint8_t p20[20];
    memset(p10, 1, sizeof(p10));
    memset(p20, 2, sizeof(p20));
    TEST_ASSERT_TRUE(MicroDDS_UDP_SendTo(p10, 10, inet_addr(LOOPBACK_STR), rx_port));
    TEST_ASSERT_TRUE(MicroDDS_UDP_SendTo(p20, 20, inet_addr(LOOPBACK_STR), rx_port));

    uint32_t s1, s2, s3, s4, s5;
    MicroDDS_UDP_GetStats(&s1, &s2, &s3, &s4, &s5);
    TEST_ASSERT_EQUAL_UINT32(2, s1);
    TEST_ASSERT_EQUAL_UINT32(0, s2);
    TEST_ASSERT_EQUAL_UINT32(0, s3);
    TEST_ASSERT_EQUAL_UINT32(30, s4);
    TEST_ASSERT_EQUAL_UINT32(0, s5);

    /* 接收两包: 从测试 socket 注入到被测传输层的用户端口 */
    int tx = make_udp_socket();
    (void)bind_udp(tx, inet_addr(LOOPBACK_STR), 0);
    uint8_t rbuf[64];
    send_raw_to(tx, p10, 10, user_port_of(30080));
    (void)wait_user_packet(rbuf, sizeof(rbuf), NULL, NULL, 200);
    send_raw_to(tx, p20, 20, user_port_of(30080));
    (void)wait_user_packet(rbuf, sizeof(rbuf), NULL, NULL, 200);
    TEST_ASSERT_TRUE(close(tx) == 0);
    MicroDDS_UDP_GetStats(&s1, &s2, &s3, &s4, &s5);
    TEST_ASSERT_EQUAL_UINT32(2, s2);
    TEST_ASSERT_EQUAL_UINT32(30, s5);

    /* 超长 → dropped 计数 */
    uint8_t big[MAX_PAYLOAD + 1U];
    memset(big, 0, sizeof(big));
    TEST_ASSERT_FALSE(MicroDDS_UDP_SendTo(big, (uint16_t)sizeof(big),
                                          inet_addr(LOOPBACK_STR), rx_port));
    s3 = 0;
    MicroDDS_UDP_GetStats(NULL, NULL, &s3, NULL, NULL);
    TEST_ASSERT_EQUAL_UINT32(1, s3);

    /* ClearStats 归零 */
    MicroDDS_UDP_ClearStats();
    MicroDDS_UDP_GetStats(&s1, &s2, &s3, &s4, &s5);
    TEST_ASSERT_EQUAL_UINT32(0, s1);
    TEST_ASSERT_EQUAL_UINT32(0, s2);
    TEST_ASSERT_EQUAL_UINT32(0, s3);
    TEST_ASSERT_EQUAL_UINT32(0, s4);
    TEST_ASSERT_EQUAL_UINT32(0, s5);

    TEST_ASSERT_TRUE(close(rx) == 0);
    MicroDDS_UDP_Shutdown();
}

/* ============================================================================
 * main
 * ============================================================================ */

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_uninit_api_guards);
    RUN_TEST(test_init_shutdown_lifecycle);
    RUN_TEST(test_reinit_after_shutdown);
    RUN_TEST(test_init_bind_conflict_discovery_port);
    RUN_TEST(test_init_failure_cleans_sockets);
    RUN_TEST(test_platform_hooks_participant_id);
    RUN_TEST(test_sendto_loopback_roundtrip);
    RUN_TEST(test_receive_loopback);
    RUN_TEST(test_receive_timeout_behavior);
    RUN_TEST(test_receive_truncates_to_buffer_size);
    RUN_TEST(test_peer_management);
    RUN_TEST(test_send_to_peers_loopback);
    RUN_TEST(test_multicast_join_leave);
    RUN_TEST(test_send_multicast);
    RUN_TEST(test_pdp_announce_discovery);
    RUN_TEST(test_pdp_lease_expiry);
    RUN_TEST(test_stats_and_clear);
    return UNITY_END();
}
