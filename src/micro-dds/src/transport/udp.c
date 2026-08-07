/** @file udp.c
 * @brief UDP传输层完整实现
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 *
 * 完整的UDP传输层实现，包括:
 * - UDP套接字初始化/关闭
 * - 多播组加入/离开
 * - 数据包发送 (支持单播和组播)
 * - 数据包接收 (带超时机制)
 * - DDS发现协议基础 (简单PDP)
 * - 地址解析和转换函数
 * - 平台抽象层 (FreeRTOS/POSIX兼容)
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * 平台检测与抽象
 * ============================================================================ */

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
    #define MICRODDS_PLATFORM_POSIX
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <sys/select.h>
    #include <sys/time.h>
#elif defined(FREERTOS) || defined(__FreeRTOS__)
    #define MICRODDS_PLATFORM_FREERTOS
    #include "FreeRTOS.h"
    #include "task.h"
    #include "lwip/sockets.h"
    #include "lwip/netdb.h"
    #include "lwip/inet.h"
#else
    #define MICRODDS_PLATFORM_GENERIC
    #warning "Unknown platform, using generic stub implementation"
#endif

/* ============================================================================
 * 配置定义
 * ============================================================================ */

#ifndef MICRODDS_UDP_PORT_BASE
#define MICRODDS_UDP_PORT_BASE 7400U  /* DDS默认端口基础值 */
#endif

#ifndef MICRODDS_UDP_USER_PORT_OFFSET
#define MICRODDS_UDP_USER_PORT_OFFSET 10U  /* 用户数据端口偏移 */
#endif

#ifndef MICRODDS_UDP_MAX_PAYLOAD
#define MICRODDS_UDP_MAX_PAYLOAD 1472U  /* 典型的以太网MTU减去UDP/IP头部 */
#endif

#ifndef MICRODDS_MAX_PEERS
#define MICRODDS_MAX_PEERS 16U
#endif

#ifndef MICRODDS_MAX_MULTICAST_GROUPS
#define MICRODDS_MAX_MULTICAST_GROUPS 4U
#endif

#ifndef MICRODDS_PDP_ANNOUNCE_PERIOD_MS
#define MICRODDS_PDP_ANNOUNCE_PERIOD_MS 3000U  /* PDP宣告周期 */
#endif

#ifndef MICRODDS_PDP_LEASE_DURATION_MS
#define MICRODDS_PDP_LEASE_DURATION_MS 10000U  /* PDP租约持续时间 */
#endif

/* 默认多播地址: 239.255.0.1 (DDS标准发现多播地址) */
#ifndef MICRODDS_DEFAULT_MULTICAST_ADDR
#define MICRODDS_DEFAULT_MULTICAST_ADDR 0xEFFF0001U  /* 239.255.0.1 */
#endif

/* ============================================================================
 * 内部数据类型
 * ============================================================================ */

/** @brief IPv4地址结构 */
typedef struct {
    uint32_t address;  /* IPv4地址，网络字节序 */
    uint16_t port;     /* 端口号，网络字节序 */
} UDP_Address;

/** @brief 数据包结构 */
typedef struct {
    uint8_t data[MICRODDS_UDP_MAX_PAYLOAD];
    uint16_t length;
    UDP_Address source;
    bool is_valid;
} UDP_Packet;

/** @brief PDP参与者信息 */
typedef struct {
    uint32_t participant_id;
    uint32_t address;       /* 网络字节序 */
    uint16_t discovery_port;
    uint16_t user_port;
    uint32_t last_seen_ms;
    bool is_active;
} PDP_Participant;

/** @brief 多播组信息 */
typedef struct {
    uint32_t group_address;  /* 网络字节序 */
    bool is_joined;
} MulticastGroup;

/** @brief UDP传输层状态 */
typedef struct {
    /* 套接字句柄 (平台相关) */
#if defined(MICRODDS_PLATFORM_POSIX) || defined(MICRODDS_PLATFORM_FREERTOS)
    int discovery_socket;
    int user_socket;
#else
    int discovery_socket;
    int user_socket;
#endif
    
    /* 地址信息 */
    uint16_t discovery_port;  /* 主机字节序 */
    uint16_t user_port;       /* 主机字节序 */
    uint32_t local_address;   /* 网络字节序 */
    
    /* 对端管理 */
    UDP_Address peers[MICRODDS_MAX_PEERS];
    uint32_t peer_count;
    
    /* 多播组管理 */
    MulticastGroup multicast_groups[MICRODDS_MAX_MULTICAST_GROUPS];
    uint32_t multicast_group_count;
    
    /* PDP发现 */
    PDP_Participant discovered_participants[MICRODDS_MAX_PEERS];
    uint32_t participant_count;
    uint32_t local_participant_id;
    uint32_t last_announce_ms;
    bool pdp_enabled;
    
    /* 状态标志 */
    bool initialized;
    uint32_t domain_id;
    
    /* 统计信息 */
    uint32_t packets_sent;
    uint32_t packets_received;
    uint32_t packets_dropped;
    uint32_t bytes_sent;
    uint32_t bytes_received;
} UDP_Transport;

/** @brief PDP消息类型 */
typedef enum {
    PDP_MSG_ANNOUNCE = 0x01,  /* 参与者宣告 */
    PDP_MSG_HEARTBEAT = 0x02, /* 心跳 */
    PDP_MSG_LEAVE = 0x03      /* 离开 */
} PDP_MessageType;

/** @brief PDP消息头部 */
typedef struct {
    uint8_t magic[4];         /* "PDP\0" */
    uint8_t version;          /* 协议版本 */
    uint8_t msg_type;         /* 消息类型 */
    uint16_t payload_length;  /* 负载长度 */
    uint32_t participant_id;  /* 参与者ID */
    uint32_t timestamp;       /* 时间戳 */
} PDP_Header;

/** @brief PDP宣告负载 */
typedef struct {
    uint32_t address;         /* IP地址 */
    uint16_t discovery_port;  /* 发现端口 */
    uint16_t user_port;       /* 用户数据端口 */
    uint32_t domain_id;       /* 域ID */
    uint32_t lease_duration;  /* 租约持续时间(ms) */
} PDP_AnnouncePayload;

/* ============================================================================
 * 全局状态
 * ============================================================================ */

static UDP_Transport g_udp_transport;
static uint32_t g_system_time_ms = 0U;

/* 平台抽象函数指针 (可在初始化时覆盖) */
static uint32_t (*platform_get_time_ms)(void) = NULL_PTR;
static void (*platform_sleep_ms)(uint32_t ms) = NULL_PTR;

/* ============================================================================
 * 字节序转换函数
 * ============================================================================ */

static uint16_t host_to_network_16(uint16_t value) {
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    return (uint16_t)(((value & 0xFFU) << 8) | ((value >> 8) & 0xFFU));
#else
    return value;
#endif
}

static uint32_t host_to_network_32(uint32_t value) {
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    return ((value & 0xFFUL) << 24) |
           ((value & 0xFF00UL) << 8) |
           ((value & 0xFF0000UL) >> 8) |
           ((value & 0xFF000000UL) >> 24);
#else
    return value;
#endif
}

static uint16_t network_to_host_16(uint16_t value) {
    return host_to_network_16(value);  /* 相同操作 */
}

static uint32_t network_to_host_32(uint32_t value) {
    return host_to_network_32(value);  /* 相同操作 */
}

/* ============================================================================
 * 平台抽象层实现
 * ============================================================================ */

#if defined(MICRODDS_PLATFORM_POSIX)

static uint32_t posix_get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL_PTR);
    return (uint32_t)((tv.tv_sec * 1000ULL) + (tv.tv_usec / 1000ULL));
}

static void posix_sleep_ms(uint32_t ms) {
    usleep(ms * 1000U);
}

#elif defined(MICRODDS_PLATFORM_FREERTOS)

static uint32_t freertos_get_time_ms(void) {
    return xTaskGetTickCount() * (1000U / configTICK_RATE_HZ);
}

static void freertos_sleep_ms(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

#else

/* 通用桩实现 */
static uint32_t generic_get_time_ms(void) {
    return g_system_time_ms;
}

static void generic_sleep_ms(uint32_t ms) {
    (void)ms;
    /* 空实现 */
}

#endif

/**
 * @brief 获取当前时间(毫秒)
 */
static uint32_t get_time_ms(void) {
    if (platform_get_time_ms != NULL_PTR) {
        return platform_get_time_ms();
    }
#if defined(MICRODDS_PLATFORM_POSIX)
    return posix_get_time_ms();
#elif defined(MICRODDS_PLATFORM_FREERTOS)
    return freertos_get_time_ms();
#else
    return generic_get_time_ms();
#endif
}

/**
 * @brief 睡眠指定毫秒
 */
static void sleep_ms(uint32_t ms) {
    if (platform_sleep_ms != NULL_PTR) {
        platform_sleep_ms(ms);
        return;
    }
#if defined(MICRODDS_PLATFORM_POSIX)
    posix_sleep_ms(ms);
#elif defined(MICRODDS_PLATFORM_FREERTOS)
    freertos_sleep_ms(ms);
#else
    generic_sleep_ms(ms);
#endif
}

/* ============================================================================
 * 地址转换函数
 * ============================================================================ */

/**
 * @brief 将点分十进制字符串转换为32位网络字节序地址
 * @param ip_str IP地址字符串 (如 "192.168.1.1")
 * @return 网络字节序的32位地址，失败返回0
 */
static uint32_t UDP_Address_from_string(const char* ip_str) {
    if (ip_str == NULL_PTR) {
        return 0U;
    }
    
#if defined(MICRODDS_PLATFORM_POSIX) || defined(MICRODDS_PLATFORM_FREERTOS)
    struct in_addr addr;
    if (inet_aton(ip_str, &addr) == 0U ) {
        return 0U;
    }
    return addr.s_addr;
#else
    /* 简单解析实现 */
    uint32_t ip = 0U;
    uint32_t octet = 0U;
    uint32_t shift = 24U;
    
    while (*ip_str != '\0') {
        if ((*ip_str >= '0') && (*ip_str <= '9')) {
            octet = (octet * 10U) + (uint32_t)(*ip_str - '0');
        } else if (*ip_str == '.') {
            ip |= (octet << shift);
            octet = 0U;
            shift -= 8U;
        } else {
            return 0U;
        }
        ip_str++;
    }
    ip |= octet;
    return host_to_network_32(ip);
#endif
}

/**
 * @brief 将32位网络字节序地址转换为点分十进制字符串
 * @param address 网络字节序的32位地址
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return true 成功
 */
static bool UDP_Address_to_string(uint32_t address, char* buffer, uint32_t buffer_size) {
    if ((buffer == NULL_PTR) || (buffer_size < 16U)) {
        return false;
    }
    
    uint32_t addr_host = network_to_host_32(address);
    uint8_t octets[4];
    octets[0] = (uint8_t)((addr_host >> 24) & 0xFFU);
    octets[1] = (uint8_t)((addr_host >> 16) & 0xFFU);
    octets[2] = (uint8_t)((addr_host >> 8) & 0xFFU);
    octets[3] = (uint8_t)(addr_host & 0xFFU);
    
    (void)snprintf(buffer, buffer_size, "%u.%u.%u.%u",
                   octets[0], octets[1], octets[2], octets[3]);
    return true;
}

/**
 * @brief 检查地址是否为多播地址
 * @param address 网络字节序的32位地址
 * @return true 是多播地址
 */
static bool UDP_Address_is_multicast(uint32_t address) {
    /* 多播地址范围: 224.0.0.0 到 239.255.255.255 */
    uint32_t addr_host = network_to_host_32(address);
    return ((addr_host & 0xF0000000U) == 0xE0000000U);
}

/**
 * @brief 获取广播地址
 * @return 广播地址 (255.255.255.255)
 */
static uint32_t UDP_Address_get_broadcast(void) {
    return 0xFFFFFFFFU;  /* 255.255.255.255 */
}

/* ============================================================================
 * 套接字操作函数 (平台抽象)
 * ============================================================================ */

#if defined(MICRODDS_PLATFORM_POSIX) || defined(MICRODDS_PLATFORM_FREERTOS)

/**
 * @brief 创建UDP套接字
 * @return 套接字描述符，失败返回-1
 */
static int socket_create(void) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return -1;
    }
    
    /* 设置地址重用 */
    int reuse = 1;
    (void)setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    return sock;
}

/**
 * @brief 绑定套接字到指定端口
 * @param sock 套接字描述符
 * @param port 端口号 (主机字节序)
 * @return true 成功
 */
static bool socket_bind(int sock, uint16_t port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = host_to_network_16(port);
    
    return (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0U );
}

/**
 * @brief 设置套接字为非阻塞模式
 * @param sock 套接字描述符
 * @param non_blocking true 非阻塞
 * @return true 成功
 */
static bool socket_set_nonblocking(int sock, bool non_blocking) {
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }
    
    if (non_blocking) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    
    return (fcntl(sock, F_SETFL, flags) == 0U );
}

/**
 * @brief 设置套接字接收超时
 * @param sock 套接字描述符
 * @param timeout_ms 超时时间(毫秒)
 * @return true 成功
 */
static bool socket_set_recv_timeout(int sock, uint32_t timeout_ms) {
    struct timeval tv;
    tv.tv_sec = (time_t)(timeout_ms / 1000U);
    tv.tv_usec = (suseconds_t)((timeout_ms % 1000U) * 1000U);
    return (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0U );
}

/**
 * @brief 关闭套接字
 * @param sock 套接字描述符
 */
static void socket_close(int sock) {
    if (sock >= 0) {
        (void)close(sock);
    }
}

/**
 * @brief 加入多播组
 * @param sock 套接字描述符
 * @param group_addr 多播组地址 (网络字节序)
 * @param local_addr 本地地址 (网络字节序)
 * @return true 成功
 */
static bool socket_join_multicast(int sock, uint32_t group_addr, uint32_t local_addr) {
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = group_addr;
    mreq.imr_interface.s_addr = local_addr;
    
    return (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == 0U );
}

/**
 * @brief 离开多播组
 * @param sock 套接字描述符
 * @param group_addr 多播组地址 (网络字节序)
 * @param local_addr 本地地址 (网络字节序)
 * @return true 成功
 */
static bool socket_leave_multicast(int sock, uint32_t group_addr, uint32_t local_addr) {
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = group_addr;
    mreq.imr_interface.s_addr = local_addr;
    
    return (setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) == 0U );
}

/**
 * @brief 设置多播TTL
 * @param sock 套接字描述符
 * @param ttl TTL值
 * @return true 成功
 */
static bool socket_set_multicast_ttl(int sock, uint8_t ttl) {
    return (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) == 0U );
}

/**
 * @brief 设置多播环回
 * @param sock 套接字描述符
 * @param enable 是否启用
 * @return true 成功
 */
static bool socket_set_multicast_loop(int sock, bool enable) {
    uint8_t loop = enable ? 1U : 0U;
    return (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) == 0U );
}

/**
 * @brief 发送数据
 * @param sock 套接字描述符
 * @param data 数据指针
 * @param length 数据长度
 * @param dest_addr 目标地址
 * @param dest_port 目标端口
 * @return 发送的字节数，失败返回-1
 */
static int socket_sendto(int sock, const uint8_t* data, uint16_t length,
                         uint32_t dest_addr, uint16_t dest_port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = dest_addr;
    addr.sin_port = host_to_network_16(dest_port);
    
    ssize_t sent = sendto(sock, data, length, 0,
                          (struct sockaddr*)&addr, sizeof(addr));
    return (int)sent;
}

/**
 * @brief 接收数据
 * @param sock 套接字描述符
 * @param buffer 缓冲区
 * @param buffer_size 缓冲区大小
 * @param src_addr 输出源地址
 * @param src_port 输出源端口
 * @return 接收的字节数，失败返回-1
 */
static int socket_recvfrom(int sock, uint8_t* buffer, uint16_t buffer_size,
                           uint32_t* src_addr, uint16_t* src_port) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    
    ssize_t received = recvfrom(sock, buffer, buffer_size, 0,
                                (struct sockaddr*)&addr, &addr_len);
    
    if (received > 0U ) {
        if (src_addr != NULL_PTR) {
            *src_addr = addr.sin_addr.s_addr;
        }
        if (src_port != NULL_PTR) {
            *src_port = network_to_host_16(addr.sin_port);
        }
    }
    
    return (int)received;
}

/**
 * @brief 检查套接字是否有数据可读
 * @param sock 套接字描述符
 * @param timeout_ms 超时时间
 * @return true 有数据可读
 */
static bool socket_select_read(int sock, uint32_t timeout_ms) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sock, &read_fds);
    
    struct timeval tv;
    tv.tv_sec = (time_t)(timeout_ms / 1000U);
    tv.tv_usec = (suseconds_t)((timeout_ms % 1000U) * 1000U);
    
    int result = select(sock + 1, &read_fds, NULL_PTR, NULL_PTR, &tv);
    return (result > 0U ) && FD_ISSET(sock, &read_fds);
}

#else

/* 通用平台桩实现 */
static int socket_create(void) { return -1; }
static bool socket_bind(int sock, uint16_t port) { (void)sock; (void)port; return false; }
static bool socket_set_nonblocking(int sock, bool non_blocking) { (void)sock; (void)non_blocking; return false; }
static bool socket_set_recv_timeout(int sock, uint32_t timeout_ms) { (void)sock; (void)timeout_ms; return false; }
static void socket_close(int sock) { (void)sock; }
static bool socket_join_multicast(int sock, uint32_t group_addr, uint32_t local_addr) { (void)sock; (void)group_addr; (void)local_addr; return false; }
static bool socket_leave_multicast(int sock, uint32_t group_addr, uint32_t local_addr) { (void)sock; (void)group_addr; (void)local_addr; return false; }
static bool socket_set_multicast_ttl(int sock, uint8_t ttl) { (void)sock; (void)ttl; return false; }
static bool socket_set_multicast_loop(int sock, bool enable) { (void)sock; (void)enable; return false; }
static int socket_sendto(int sock, const uint8_t* data, uint16_t length, uint32_t dest_addr, uint16_t dest_port) { (void)sock; (void)data; (void)length; (void)dest_addr; (void)dest_port; return -1; }
static int socket_recvfrom(int sock, uint8_t* buffer, uint16_t buffer_size, uint32_t* src_addr, uint16_t* src_port) { (void)sock; (void)buffer; (void)buffer_size; (void)src_addr; (void)src_port; return -1; }
static bool socket_select_read(int sock, uint32_t timeout_ms) { (void)sock; (void)timeout_ms; return false; }

#endif

/* ============================================================================
 * PDP (Participant Discovery Protocol) 实现
 * ============================================================================ */

/**
 * @brief 初始化PDP头部
 * @param header PDP头部指针
 * @param msg_type 消息类型
 * @param participant_id 参与者ID
 */
static void PDP_init_header(PDP_Header* header, uint8_t msg_type, uint32_t participant_id) {
    header->magic[0] = 'P';
    header->magic[1] = 'D';
    header->magic[2] = 'P';
    header->magic[3] = 0;
    header->version = 1;
    header->msg_type = msg_type;
    header->payload_length = 0;
    header->participant_id = host_to_network_32(participant_id);
    header->timestamp = host_to_network_32(get_time_ms());
}

/**
 * @brief 验证PDP消息
 * @param data 数据指针
 * @param length 数据长度
 * @return true 是有效的PDP消息
 */
static bool PDP_verify_message(const uint8_t* data, uint16_t length) {
    if (length < sizeof(PDP_Header)) {
        return false;
    }
    
    const PDP_Header* header = (const PDP_Header*)data;
    return ((header->magic[0U] == 'P') &&
            ((header->magic[1U] == 'D')) &&
            ((header->magic[2U] == 'P')) &&
            ((header->magic[3] == 0U)) &&
            (header->version == 1U));
}

/**
 * @brief 发送PDP宣告消息
 */
static void PDP_send_announce(void) {
    if (!g_udp_transport.pdp_enabled) {
        return;
    }
    
    uint8_t buffer[sizeof(PDP_Header) + sizeof(PDP_AnnouncePayload)];
    PDP_Header* header = (PDP_Header*)buffer;
    PDP_AnnouncePayload* payload = (PDP_AnnouncePayload*)(buffer + sizeof(PDP_Header));
    
    PDP_init_header(header, PDP_MSG_ANNOUNCE, g_udp_transport.local_participant_id);
    header->payload_length = host_to_network_16((uint16_t)sizeof(PDP_AnnouncePayload));
    
    payload->address = g_udp_transport.local_address;
    payload->discovery_port = host_to_network_16(g_udp_transport.discovery_port);
    payload->user_port = host_to_network_16(g_udp_transport.user_port);
    payload->domain_id = host_to_network_32(g_udp_transport.domain_id);
    payload->lease_duration = host_to_network_32(MICRODDS_PDP_LEASE_DURATION_MS);
    
    uint16_t total_length = sizeof(PDP_Header) + sizeof(PDP_AnnouncePayload);
    
    /* 发送到多播组 */
    for (uint32_t i = 0U; i < g_udp_transport.multicast_group_count; i++) {
        if (g_udp_transport.multicast_groups[i].is_joined) {
            (void)socket_sendto(g_udp_transport.discovery_socket,
                               buffer, total_length,
                               g_udp_transport.multicast_groups[i].group_address,
                               g_udp_transport.discovery_port);
        }
    }
    
    /* 广播到局域网 */
    (void)socket_sendto(g_udp_transport.discovery_socket,
                       buffer, total_length,
                       UDP_Address_get_broadcast(),
                       g_udp_transport.discovery_port);
    
    g_udp_transport.last_announce_ms = get_time_ms();
}

/**
 * @brief 处理PDP宣告消息
 * @param data 数据指针
 * @param length 数据长度
 * @param src_addr 源地址
 */
static void PDP_handle_announce(const uint8_t* data, uint16_t length, uint32_t src_addr) {
    if (length < (sizeof(PDP_Header) + sizeof(PDP_AnnouncePayload))) {
        return;
    }
    
    const PDP_Header* header = (const PDP_Header*)data;
    const PDP_AnnouncePayload* payload = (const PDP_AnnouncePayload*)(data + sizeof(PDP_Header));
    
    uint32_t participant_id = network_to_host_32(header->participant_id);
    uint32_t domain_id = network_to_host_32(payload->domain_id);
    
    /* 忽略自己和其他域的消息 */
    if ((participant_id == g_udp_transport.local_participant_id) ||
        (domain_id != g_udp_transport.domain_id)) {
        return;
    }
    
    /* 查找或创建参与者条目 */
    PDP_Participant* participant = NULL_PTR;
    for (uint32_t i = 0U; i < g_udp_transport.participant_count; i++) {
        if (g_udp_transport.discovered_participants[i].participant_id == participant_id) {
            participant = &g_udp_transport.discovered_participants[i];
            break;
        }
    }
    
    /* 新参与者 */
    if (participant == NULL_PTR) {
        if (g_udp_transport.participant_count >= MICRODDS_MAX_PEERS) {
            return;  /* 参与者表满 */
        }
        participant = &g_udp_transport.discovered_participants[g_udp_transport.participant_count];
        g_udp_transport.participant_count++;
    }
    
    /* 更新参与者信息 */
    participant->participant_id = participant_id;
    participant->address = payload->address;
    participant->discovery_port = network_to_host_16(payload->discovery_port);
    participant->user_port = network_to_host_16(payload->user_port);
    participant->last_seen_ms = get_time_ms();
    participant->is_active = true;
    
    /* 同时添加到对端列表 */
    (void)MicroDDS_UDP_AddPeer(network_to_host_32(src_addr), participant->user_port);
}

/**
 * @brief 清理过期的参与者
 */
static void PDP_cleanup_participants(void) {
    uint32_t current_time = get_time_ms();
    
    for (uint32_t i = 0U; i < g_udp_transport.participant_count; i++) {
        PDP_Participant* p = &g_udp_transport.discovered_participants[i];
        if (p->is_active) {
            uint32_t elapsed = current_time - p->last_seen_ms;
            if (elapsed > MICRODDS_PDP_LEASE_DURATION_MS) {
                p->is_active = false;
            }
        }
    }
}

/**
 * @brief PDP维护任务 (周期性调用)
 */
static void PDP_maintain(void) {
    if (!g_udp_transport.pdp_enabled) {
        return;
    }
    
    uint32_t current_time = get_time_ms();
    
    /* 定期发送宣告 */
    if ((current_time - g_udp_transport.last_announce_ms) >= MICRODDS_PDP_ANNOUNCE_PERIOD_MS) {
        PDP_send_announce();
    }
    
    /* 清理过期参与者 */
    PDP_cleanup_participants();
}

/* ============================================================================
 * 公共API实现
 * ============================================================================ */

/**
 * @brief 初始化UDP传输层
 * @param domain_id DDS域ID
 * @return true 成功
 */
bool MicroDDS_UDP_Init(uint32_t domain_id) {
    if (g_udp_transport.initialized) {
        return true;
    }
    
    memset(&g_udp_transport, 0, sizeof(g_udp_transport));
    
    /* 计算端口号 */
    g_udp_transport.domain_id = domain_id;
    g_udp_transport.discovery_port = (uint16_t)(MICRODDS_UDP_PORT_BASE + domain_id);
    g_udp_transport.user_port = (uint16_t)(MICRODDS_UDP_PORT_BASE + MICRODDS_UDP_USER_PORT_OFFSET + domain_id);
    
    /* 生成唯一的参与者ID */
    g_udp_transport.local_participant_id = (uint32_t)(get_time_ms() ^ (domain_id << 16));
    
    /* 创建发现套接字 */
    g_udp_transport.discovery_socket = socket_create();
    if (g_udp_transport.discovery_socket < 0) {
        return false;
    }
    
    if (!socket_bind(g_udp_transport.discovery_socket, g_udp_transport.discovery_port)) {
        socket_close(g_udp_transport.discovery_socket);
        return false;
    }
    
    /* 创建用户数据套接字 */
    g_udp_transport.user_socket = socket_create();
    if (g_udp_transport.user_socket < 0) {
        socket_close(g_udp_transport.discovery_socket);
        return false;
    }
    
    if (!socket_bind(g_udp_transport.user_socket, g_udp_transport.user_port)) {
        socket_close(g_udp_transport.discovery_socket);
        socket_close(g_udp_transport.user_socket);
        return false;
    }
    
    /* 启用多播环回 */
    (void)socket_set_multicast_loop(g_udp_transport.discovery_socket, true);
    (void)socket_set_multicast_loop(g_udp_transport.user_socket, true);
    
    /* 设置多播TTL */
    (void)socket_set_multicast_ttl(g_udp_transport.discovery_socket, 1);
    (void)socket_set_multicast_ttl(g_udp_transport.user_socket, 1);
    
    g_udp_transport.initialized = true;
    g_udp_transport.pdp_enabled = true;
    
    /* 发送初始宣告 */
    PDP_send_announce();
    
    return true;
}

/**
 * @brief 关闭UDP传输层
 */
void MicroDDS_UDP_Shutdown(void) {
    if (!g_udp_transport.initialized) {
        return;
    }
    
    /* 离开所有多播组 */
    for (uint32_t i = 0U; i < g_udp_transport.multicast_group_count; i++) {
        if (g_udp_transport.multicast_groups[i].is_joined) {
            (void)socket_leave_multicast(g_udp_transport.discovery_socket,
                                        g_udp_transport.multicast_groups[i].group_address,
                                        g_udp_transport.local_address);
        }
    }
    
    /* 关闭套接字 */
    socket_close(g_udp_transport.discovery_socket);
    socket_close(g_udp_transport.user_socket);
    
    memset(&g_udp_transport, 0, sizeof(g_udp_transport));
}

/**
 * @brief 加入多播组
 * @param group_address 多播组地址字符串 (如 "239.255.0.1")
 * @return true 成功
 */
bool MicroDDS_UDP_JoinMulticast(const char* group_address) {
    if (!g_udp_transport.initialized || (group_address == NULL_PTR)) {
        return false;
    }
    
    if (g_udp_transport.multicast_group_count >= MICRODDS_MAX_MULTICAST_GROUPS) {
        return false;
    }
    
    uint32_t group_addr = UDP_Address_from_string(group_address);
    if (group_addr == 0U) {
        return false;
    }
    
    if (!UDP_Address_is_multicast(group_addr)) {
        return false;
    }
    
    /* 检查是否已加入 */
    for (uint32_t i = 0U; i < g_udp_transport.multicast_group_count; i++) {
        if (g_udp_transport.multicast_groups[i].group_address == group_addr) {
            return true;  /* 已加入 */
        }
    }
    
    /* 加入多播组 */
    if (!socket_join_multicast(g_udp_transport.discovery_socket, group_addr, g_udp_transport.local_address)) {
        return false;
    }
    
    /* 添加到列表 */
    MulticastGroup* group = &g_udp_transport.multicast_groups[g_udp_transport.multicast_group_count];
    group->group_address = group_addr;
    group->is_joined = true;
    g_udp_transport.multicast_group_count++;
    
    return true;
}

/**
 * @brief 离开多播组
 * @param group_address 多播组地址字符串
 * @return true 成功
 */
bool MicroDDS_UDP_LeaveMulticast(const char* group_address) {
    if (!g_udp_transport.initialized || (group_address == NULL_PTR)) {
        return false;
    }
    
    uint32_t group_addr = UDP_Address_from_string(group_address);
    if (group_addr == 0U) {
        return false;
    }
    
    /* 查找并移除 */
    for (uint32_t i = 0U; i < g_udp_transport.multicast_group_count; i++) {
        if (g_udp_transport.multicast_groups[i].group_address == group_addr) {
            (void)socket_leave_multicast(g_udp_transport.discovery_socket, group_addr, g_udp_transport.local_address);
            
            /* 从列表中移除 */
            for (uint32_t j = i; j < (g_udp_transport.multicast_group_count - 1U); j++) {
                g_udp_transport.multicast_groups[j] = g_udp_transport.multicast_groups[j + 1U];
            }
            g_udp_transport.multicast_group_count--;
            return true;
        }
    }
    
    return false;  /* 未找到 */
}

/**
 * @brief 添加对端节点
 * @param address IPv4地址（主机字节序）
 * @param port 端口号（主机字节序）
 * @return true 成功
 */
bool MicroDDS_UDP_AddPeer(uint32_t address, uint16_t port) {
    if (!g_udp_transport.initialized) {
        return false;
    }
    
    if (g_udp_transport.peer_count >= MICRODDS_MAX_PEERS) {
        return false;
    }
    
    /* 检查是否已存在 */
    for (uint32_t i = 0U; i < g_udp_transport.peer_count; i++) {
        if ((network_to_host_32(g_udp_transport.peers[i].address) == address) &&
            (network_to_host_16(g_udp_transport.peers[i].port) == port)) {
            return true;  /* 已存在 */
        }
    }
    
    UDP_Address* peer = &g_udp_transport.peers[g_udp_transport.peer_count];
    peer->address = host_to_network_32(address);
    peer->port = host_to_network_16(port);
    
    g_udp_transport.peer_count++;
    return true;
}

/**
 * @brief 移除对端节点
 * @param address IPv4地址（主机字节序）
 * @param port 端口号（主机字节序）
 * @return true 成功
 */
bool MicroDDS_UDP_RemovePeer(uint32_t address, uint16_t port) {
    if (!g_udp_transport.initialized) {
        return false;
    }
    
    for (uint32_t i = 0U; i < g_udp_transport.peer_count; i++) {
        if ((network_to_host_32(g_udp_transport.peers[i].address) == address) &&
            (network_to_host_16(g_udp_transport.peers[i].port) == port)) {
            /* 从列表中移除 */
            for (uint32_t j = i; j < (g_udp_transport.peer_count - 1U); j++) {
                g_udp_transport.peers[j] = g_udp_transport.peers[j + 1U];
            }
            g_udp_transport.peer_count--;
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 发送数据包到指定地址
 * @param data 数据指针
 * @param length 数据长度
 * @param dest_address 目标地址 (网络字节序)
 * @param dest_port 目标端口 (主机字节序)
 * @return true 成功
 */
bool MicroDDS_UDP_SendTo(const uint8_t* data, uint16_t length, uint32_t dest_address, uint16_t dest_port) {
    if (!g_udp_transport.initialized || (data == NULL_PTR)) {
        return false;
    }
    
    if (length > MICRODDS_UDP_MAX_PAYLOAD) {
        g_udp_transport.packets_dropped++;
        return false;
    }
    
    int sent = socket_sendto(g_udp_transport.user_socket, data, length, dest_address, dest_port);
    
    if (sent < 0) {
        return false;
    }
    
    g_udp_transport.packets_sent++;
    g_udp_transport.bytes_sent += (uint32_t)sent;
    
    return true;
}

/**
 * @brief 发送数据包到所有对端
 * @param data 数据指针
 * @param length 数据长度
 * @return true 成功
 */
bool MicroDDS_UDP_Send(const uint8_t* data, uint16_t length) {
    if (!g_udp_transport.initialized || (data == NULL_PTR)) {
        return false;
    }
    
    if (length > MICRODDS_UDP_MAX_PAYLOAD) {
        g_udp_transport.packets_dropped++;
        return false;
    }
    
    bool any_sent = false;
    
    /* 发送到所有对端 */
    for (uint32_t i = 0U; i < g_udp_transport.peer_count; i++) {
        int sent = socket_sendto(g_udp_transport.user_socket, data, length,
                                g_udp_transport.peers[i].address,
                                network_to_host_16(g_udp_transport.peers[i].port));
        if ((unsigned int)(sent) > 0U ) {
            any_sent = true;
            g_udp_transport.bytes_sent += (uint32_t)sent;
        }
    }
    
    if (any_sent) {
        g_udp_transport.packets_sent++;
    }
    
    return any_sent;
}

/**
 * @brief 发送数据包到多播组
 * @param data 数据指针
 * @param length 数据长度
 * @param group_address 多播组地址字符串
 * @return true 成功
 */
bool MicroDDS_UDP_SendMulticast(const uint8_t* data, uint16_t length, const char* group_address) {
    if (!g_udp_transport.initialized || (data == NULL_PTR)) {
        return false;
    }
    
    uint32_t group_addr = MICRODDS_DEFAULT_MULTICAST_ADDR;
    if (group_address != NULL_PTR) {
        group_addr = UDP_Address_from_string(group_address);
        if (group_addr == 0U) {
            return false;
        }
    }
    
    int sent = socket_sendto(g_udp_transport.user_socket, data, length,
                            group_addr, g_udp_transport.user_port);
    
    if (sent < 0) {
        return false;
    }
    
    g_udp_transport.packets_sent++;
    g_udp_transport.bytes_sent += (uint32_t)sent;
    
    return true;
}

/**
 * @brief 接收数据包
 * @param buffer 数据缓冲区
 * @param buffer_size 缓冲区大小
 * @param src_address 输出源地址 (网络字节序，可为NULL_PTR)
 * @param src_port 输出源端口 (主机字节序，可为NULL_PTR)
 * @param timeout_ms 超时时间（毫秒）
 * @return 接收的字节数，失败或超时返回0
 */
uint16_t MicroDDS_UDP_Receive(uint8_t* buffer, uint16_t buffer_size,
                               uint32_t* src_address, uint16_t* src_port,
                               uint32_t timeout_ms) {
    if (!g_udp_transport.initialized || (buffer == NULL_PTR)) {
        return 0U;
    }
    
    /* 先进行PDP维护 */
    PDP_maintain();
    
    /* 检查用户数据套接字 */
    if (socket_select_read(g_udp_transport.user_socket, timeout_ms)) {
        uint32_t addr = 0U;
        uint16_t port = 0U;
        
        int received = socket_recvfrom(g_udp_transport.user_socket, buffer, buffer_size, &addr, &port);
        
        if ((unsigned int)(received) > 0U ) {
            g_udp_transport.packets_received++;
            g_udp_transport.bytes_received += (uint32_t)received;
            
            if (src_address != NULL_PTR) {
                *src_address = addr;
            }
            if (src_port != NULL_PTR) {
                *src_port = port;
            }
            
            return (uint16_t)received;
        }
    }
    
    /* 检查发现套接字 (PDP消息) */
    if (socket_select_read(g_udp_transport.discovery_socket, 0U)) {
        uint8_t pdp_buffer[512];
        uint32_t src_addr = 0U;
        
        int received = socket_recvfrom(g_udp_transport.discovery_socket, pdp_buffer, sizeof(pdp_buffer), &src_addr, NULL_PTR);
        
        if ((unsigned int)(received) > 0U ) {
            /* 处理PDP消息 */
            if (PDP_verify_message(pdp_buffer, (uint16_t)received)) {
                const PDP_Header* header = (const PDP_Header*)pdp_buffer;
                
                switch (header->msg_type) {
                    case PDP_MSG_ANNOUNCE:
                        PDP_handle_announce(pdp_buffer, (uint16_t)received, src_addr);
                        break;
                    case PDP_MSG_HEARTBEAT:
                        /* 处理心跳 */
                        break;
                    case PDP_MSG_LEAVE:
                        /* 处理离开消息 */
                        break;
                    default:
                        break;
                }
            }
        }
    }
    
    return 0U;
}

/**
 * @brief 获取本地发现端口号
 * @return 本地发现端口号（主机字节序）
 */
uint16_t MicroDDS_UDP_GetDiscoveryPort(void) {
    return g_udp_transport.discovery_port;
}

/**
 * @brief 获取本地用户数据端口号
 * @return 本地用户数据端口号（主机字节序）
 */
uint16_t MicroDDS_UDP_GetUserPort(void) {
    return g_udp_transport.user_port;
}

/**
 * @brief 获取本地参与者ID
 * @return 参与者ID
 */
uint32_t MicroDDS_UDP_GetParticipantId(void) {
    return g_udp_transport.local_participant_id;
}

/**
 * @brief 获取发现的参与者数量
 * @return 参与者数量
 */
uint32_t MicroDDS_UDP_GetDiscoveredParticipantCount(void) {
    uint32_t count = 0U;
    for (uint32_t i = 0U; i < g_udp_transport.participant_count; i++) {
        if (g_udp_transport.discovered_participants[i].is_active) {
            count++;
        }
    }
    return count;
}

/**
 * @brief 获取发现的参与者信息
 * @param index 参与者索引
 * @param participant_id 输出参与者ID
 * @param address 输出地址
 * @param port 输出端口
 * @return true 成功
 */
bool MicroDDS_UDP_GetDiscoveredParticipant(uint32_t index, uint32_t* participant_id,
                                           uint32_t* address, uint16_t* port) {
    if (index >= g_udp_transport.participant_count) {
        return false;
    }
    
    PDP_Participant* p = &g_udp_transport.discovered_participants[index];
    if (!p->is_active) {
        return false;
    }
    
    if (participant_id != NULL_PTR) {
        *participant_id = p->participant_id;
    }
    if (address != NULL_PTR) {
        *address = p->address;
    }
    if (port != NULL_PTR) {
        *port = p->user_port;
    }
    
    return true;
}

/**
 * @brief 设置平台抽象函数
 * @param get_time_fn 获取时间函数指针
 * @param sleep_fn 睡眠函数指针
 */
void MicroDDS_UDP_SetPlatformHooks(uint32_t (*get_time_fn)(void), void (*sleep_fn)(uint32_t)) {
    platform_get_time_ms = get_time_fn;
    platform_sleep_ms = sleep_fn;
}

/**
 * @brief 获取传输统计信息
 * @param packets_sent 输出发送包数
 * @param packets_received 输出接收包数
 * @param packets_dropped 输出丢弃包数
 * @param bytes_sent 输出发送字节数
 * @param bytes_received 输出接收字节数
 */
void MicroDDS_UDP_GetStats(uint32_t* packets_sent, uint32_t* packets_received,
                           uint32_t* packets_dropped, uint32_t* bytes_sent,
                           uint32_t* bytes_received) {
    if (packets_sent != NULL_PTR) {
        *packets_sent = g_udp_transport.packets_sent;
    }
    if (packets_received != NULL_PTR) {
        *packets_received = g_udp_transport.packets_received;
    }
    if (packets_dropped != NULL_PTR) {
        *packets_dropped = g_udp_transport.packets_dropped;
    }
    if (bytes_sent != NULL_PTR) {
        *bytes_sent = g_udp_transport.bytes_sent;
    }
    if (bytes_received != NULL_PTR) {
        *bytes_received = g_udp_transport.bytes_received;
    }
}

/**
 * @brief 清除统计信息
 */
void MicroDDS_UDP_ClearStats(void) {
    g_udp_transport.packets_sent = 0U;
    g_udp_transport.packets_received = 0U;
    g_udp_transport.packets_dropped = 0U;
    g_udp_transport.bytes_sent = 0U;
    g_udp_transport.bytes_received = 0U;
}

/**
 * @brief 检查传输层是否已初始化
 * @return true 已初始化
 */
bool MicroDDS_UDP_IsInitialized(void) {
    return g_udp_transport.initialized;
}

/**
 * @brief 获取对端数量
 * @return 对端数量
 */
uint32_t MicroDDS_UDP_GetPeerCount(void) {
    return g_udp_transport.peer_count;
}
