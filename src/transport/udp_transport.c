/**
 * @file udp_transport.c
 * @brief UDP/RTPS传输层实现 - 车载以太网优化
 * @version 1.0
 * @date 2026-04-24
 */

#include "udp_transport.h"
#include "../common/utils/eth_utils.h"

/* 系统头文件 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/sockios.h>
#include <linux/net_tstamp.h>
#include <time.h>
#include <pthread.h>

/* ============================================================================
 * 私有数据结构
 * ============================================================================ */

/** RTPS头部结构 */
typedef struct __attribute__((packed)) {
    uint32_t magic;             /* RTPS Magic: "RTPS" */
    uint16_t version;           /* 协议版本 */
    uint16_t vendor_id;         /* 厂商ID */
    uint8_t  guid_prefix[12];   /* GUID前缀 */
} rtps_header_t;

/** 优先级队列条目 */
typedef struct {
    uint8_t *data;
    uint32_t len;
    uint32_t max_len;
    uint64_t timestamp_ns;
    uint8_t priority;
    uint8_t in_use;
} priority_queue_entry_t;

/** UDP传输实例私有数据 */
typedef struct {
    udp_transport_config_t config;
    int socket_fd;
    int socket_mc;              /* 组播socket */
    struct sockaddr_in local_addr;
    struct sockaddr_in remote_addr;
    struct sockaddr_in mc_addr;
    
    /* 优先级队列 */
    priority_queue_entry_t tx_queues[UDP_PRIORITY_QUEUES][32];
    uint32_t tx_queue_head[UDP_PRIORITY_QUEUES];
    uint32_t tx_queue_tail[UDP_PRIORITY_QUEUES];
    pthread_mutex_t tx_queue_mutex;
    
    /* 统计信息 */
    udp_transport_stats_t stats;
    pthread_mutex_t stats_mutex;
    
    /* 接收回调 */
    eth_rx_callback_t rx_callback;
    void *rx_callback_data;
    pthread_t rx_thread;
    volatile int rx_thread_running;
    
    /* TSN/TTE */
    volatile uint64_t tte_next_tx_time_ns;
    volatile uint64_t tte_cycle_time_ns;
    
    /* 状态 */
    volatile int initialized;
    volatile int connected;
} udp_transport_private_t;

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

/**
 * @brief 获取当前时间(纳秒)
 */
static inline uint64_t get_time_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

/**
 * @brief 计算差异时间(微秒)
 */
static inline uint32_t elapsed_us(struct timeval *start, struct timeval *end)
{
    return (uint32_t)((end->tv_sec - start->tv_sec) * 1000000ULL + 
                      (end->tv_usec - start->tv_usec));
}

/**
 * @brief 创建RTPS头部
 */
static void create_rtps_header(rtps_header_t *header, uint8_t priority)
{
    header->magic = htonl(RTPS_MAGIC_COOKIE);
    header->version = htons(0x0200);    /* v2.0 */
    header->vendor_id = htons(0x0100);  /* Vendor ID */
    memset(header->guid_prefix, 0, 12);
    header->guid_prefix[11] = priority; /* 嵌入优先级 */
}

/**
 * @brief 设置套接字优先级(QoS)
 */
static int set_socket_priority(int sockfd, uint8_t priority)
{
    int tos = 0;
    
    /* 映射到IP ToS/DSCP */
    switch (priority) {
        case 0: tos = 0x2E; break;  /* EF - 最高优先级 */
        case 1: tos = 0x0A; break;  /* AF11 */
        case 2: tos = 0x12; break;  /* AF21 */
        case 3: tos = 0x00; break;  /* BE - 默认 */
        default: tos = 0x00;
    }
    
    if (setsockopt(sockfd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos)) < 0) {
        return -1;
    }
    
    return 0;
}

/**
 * @brief 设置汽车以太网参数
 */
static int set_automotive_eth_params(int sockfd, automotive_eth_speed_t speed)
{
    int ret = 0;
    
    /* 启用SO_TIMESTAMPING用于精确时间戳 */
    int ts_flags = SOF_TIMESTAMPING_TX_HARDWARE | 
                   SOF_TIMESTAMPING_RX_HARDWARE |
                   SOF_TIMESTAMPING_TX_SOFTWARE |
                   SOF_TIMESTAMPING_RX_SOFTWARE |
                   SOF_TIMESTAMPING_SOFTWARE;
    ret |= setsockopt(sockfd, SOL_SOCKET, SO_TIMESTAMPING, 
                      &ts_flags, sizeof(ts_flags));
    
    /* 启用包追踪 */
    int enable_pktinfo = 1;
    ret |= setsockopt(sockfd, IPPROTO_IP, IP_PKTINFO, 
                      &enable_pktinfo, sizeof(enable_pktinfo));
    
    /* 根据以太网速率设置缓冲区大小 */
    int sndbuf = UDP_TX_BUFFER_SIZE;
    int rcvbuf = UDP_RX_BUFFER_SIZE;
    
    if (speed == AUTOMOTIVE_ETH_100BASE_T1) {
        sndbuf = 65536;
        rcvbuf = 65536;
    } else if (speed == AUTOMOTIVE_ETH_1000BASE_T1) {
        sndbuf = 262144;
        rcvbuf = 262144;
    }
    
    ret |= setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf));
    ret |= setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));
    
    /* 非阻塞模式 */
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    
    return ret;
}

/**
 * @brief 组播接收线程
 */
static void* multicast_rx_thread(void *arg)
{
    udp_transport_private_t *priv = (udp_transport_private_t *)arg;
    uint8_t rx_buffer[UDP_RX_BUFFER_SIZE];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    
    while (priv->rx_thread_running) {
        ssize_t received = recvfrom(priv->socket_mc, rx_buffer, sizeof(rx_buffer), 0,
                                     (struct sockaddr *)&from_addr, &from_len);
        
        if (received > 0) {
            pthread_mutex_lock(&priv->stats_mutex);
            priv->stats.rx_packets++;
            priv->stats.rx_bytes += received;
            pthread_mutex_unlock(&priv->stats_mutex);
            
            if (priv->rx_callback != NULL) {
                eth_buffer_t buffer = {
                    .data = rx_buffer,
                    .len = (uint32_t)received,
                    .max_len = sizeof(rx_buffer)
                };
                priv->rx_callback(&buffer, priv->rx_callback_data);
            }
        } else if (received < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            /* 处理错误 */
            usleep(1000);
        } else {
            usleep(100);
        }
    }
    
    return NULL;
}

/* ============================================================================
 * 标准传输接口实现
 * ============================================================================ */

/**
 * @brief UDP传输初始化
 */
static eth_status_t udp_init(transport_handle_t *handle, const transport_config_t *config)
{
    if (handle == NULL || config == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    udp_transport_private_t *priv = (udp_transport_private_t *)handle->private_data;
    if (priv == NULL) {
        return ETH_ERROR;
    }
    
    /* 复制配置 */
    memcpy(&priv->config.base, config, sizeof(transport_config_t));
    
    /* 创建UDP socket */
    priv->socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (priv->socket_fd < 0) {
        return ETH_ERROR;
    }
    
    /* 设置地址复用 */
    int reuse = 1;
    setsockopt(priv->socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    /* 设置本地地址 */
    memset(&priv->local_addr, 0, sizeof(priv->local_addr));
    priv->local_addr.sin_family = AF_INET;
    priv->local_addr.sin_addr.s_addr = htonl(priv->config.base.local_addr);
    priv->local_addr.sin_port = htons(priv->config.base.local_port);
    
    if (bind(priv->socket_fd, (struct sockaddr *)&priv->local_addr, 
             sizeof(priv->local_addr)) < 0) {
        close(priv->socket_fd);
        priv->socket_fd = -1;
        return ETH_ERROR;
    }
    
    /* 设置目标地址 */
    memset(&priv->remote_addr, 0, sizeof(priv->remote_addr));
    priv->remote_addr.sin_family = AF_INET;
    priv->remote_addr.sin_addr.s_addr = htonl(priv->config.base.remote_addr);
    priv->remote_addr.sin_port = htons(priv->config.base.remote_port);
    
    /* 汽车以太网优化 */
    if (set_automotive_eth_params(priv->socket_fd, priv->config.eth_speed) < 0) {
        /* 非致命错误，继续 */
    }
    
    /* 初始化优先级队列 */
    pthread_mutex_init(&priv->tx_queue_mutex, NULL);
    for (int i = 0; i < UDP_PRIORITY_QUEUES; i++) {
        priv->tx_queue_head[i] = 0;
        priv->tx_queue_tail[i] = 0;
    }
    
    /* 初始化统计 */
    pthread_mutex_init(&priv->stats_mutex, NULL);
    memset(&priv->stats, 0, sizeof(priv->stats));
    priv->stats.latency_min_us = 0xFFFFFFFF;
    
    /* 设置默认优先级映射 */
    for (int i = 0; i < UDP_PRIORITY_QUEUES; i++) {
        priv->config.priority_queue_map[i] = i;
        priv->config.queue_weights[i] = (UDP_PRIORITY_QUEUES - i) * 10;
    }
    
    /* TSN配置初始化 */
    priv->config.tsn.enable_tsn = false;
    priv->config.tsn.traffic_class = TSN_TC_BULK;
    priv->config.tsn.time_slot_us = 1000;
    priv->config.tsn.bandwidth_reservation = 50;
    priv->config.tsn.max_latency_us = 1000;
    
    priv->tte_cycle_time_ns = 0;
    priv->tte_next_tx_time_ns = 0;
    
    priv->initialized = 1;
    priv->connected = 1;
    
    return ETH_OK;
}

/**
 * @brief UDP传输反初始化
 */
static eth_status_t udp_deinit(transport_handle_t *handle)
{
    if (handle == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    udp_transport_private_t *priv = (udp_transport_private_t *)handle->private_data;
    if (priv == NULL) {
        return ETH_OK;
    }
    
    /* 停止组播接收线程 */
    if (priv->rx_thread_running) {
        priv->rx_thread_running = 0;
        pthread_join(priv->rx_thread, NULL);
    }
    
    /* 关闭socket */
    if (priv->socket_fd >= 0) {
        close(priv->socket_fd);
        priv->socket_fd = -1;
    }
    if (priv->socket_mc >= 0) {
        close(priv->socket_mc);
        priv->socket_mc = -1;
    }
    
    /* 销毁互斥锁 */
    pthread_mutex_destroy(&priv->tx_queue_mutex);
    pthread_mutex_destroy(&priv->stats_mutex);
    
    priv->initialized = 0;
    priv->connected = 0;
    
    return ETH_OK;
}

/**
 * @brief UDP发送数据
 */
static eth_status_t udp_send(transport_handle_t *handle, const uint8_t *data, 
                              uint32_t len, uint32_t timeout_ms)
{
    if (handle == NULL || data == NULL || len == 0) {
        return ETH_INVALID_PARAM;
    }
    
    udp_transport_private_t *priv = (udp_transport_private_t *)handle->private_data;
    if (priv == NULL || !priv->initialized) {
        return ETH_NOT_INIT;
    }
    
    /* 检查数据大小 */
    if (len + RTPS_HEADER_SIZE > RTPS_MAX_PAYLOAD) {
        return ETH_INVALID_PARAM;
    }
    
    /* 构建RTPS数据包 */
    uint8_t tx_buffer[RTPS_MAX_PAYLOAD];
    rtps_header_t *header = (rtps_header_t *)tx_buffer;
    create_rtps_header(header, 0); /* 默认优先级 */
    
    memcpy(tx_buffer + RTPS_HEADER_SIZE, data, len);
    uint32_t total_len = RTPS_HEADER_SIZE + len;
    
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    /* TTE检查 - 确定性调度 */
    if (priv->config.enable_tte && priv->tte_cycle_time_ns > 0) {
        uint64_t now_ns = get_time_ns();
        if (now_ns < priv->tte_next_tx_time_ns) {
            uint64_t wait_ns = priv->tte_next_tx_time_ns - now_ns;
            struct timespec ts;
            ts.tv_sec = wait_ns / 1000000000ULL;
            ts.tv_nsec = wait_ns % 1000000000ULL;
            nanosleep(&ts, NULL);
        }
    }
    
    /* 发送数据 */
    ssize_t sent = sendto(priv->socket_fd, tx_buffer, total_len, 0,
                          (struct sockaddr *)&priv->remote_addr,
                          sizeof(priv->remote_addr));
    
    if (sent < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return ETH_TIMEOUT;
        }
        return ETH_ERROR;
    }
    
    gettimeofday(&end, NULL);
    uint32_t latency = elapsed_us(&start, &end);
    
    /* 更新统计 */
    pthread_mutex_lock(&priv->stats_mutex);
    priv->stats.tx_packets++;
    priv->stats.tx_bytes += sent;
    if (latency < priv->stats.latency_min_us) priv->stats.latency_min_us = latency;
    if (latency > priv->stats.latency_max_us) priv->stats.latency_max_us = latency;
    priv->stats.latency_avg_us = (priv->stats.latency_avg_us + latency) / 2;
    pthread_mutex_unlock(&priv->stats_mutex);
    
    /* 更新TTE下一发送时间 */
    if (priv->config.enable_tte && priv->tte_cycle_time_ns > 0) {
        priv->tte_next_tx_time_ns += priv->tte_cycle_time_ns;
    }
    
    return ETH_OK;
}

/**
 * @brief UDP接收数据
 */
static eth_status_t udp_receive(transport_handle_t *handle, uint8_t *buffer, 
                                 uint32_t max_len, uint32_t *received_len, 
                                 uint32_t timeout_ms)
{
    if (handle == NULL || buffer == NULL || received_len == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    udp_transport_private_t *priv = (udp_transport_private_t *)handle->private_data;
    if (priv == NULL || !priv->initialized) {
        return ETH_NOT_INIT;
    }
    
    /* 设置超时 */
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(priv->socket_fd, &read_fds);
    
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    int ret = select(priv->socket_fd + 1, &read_fds, NULL, NULL, &tv);
    if (ret < 0) {
        return ETH_ERROR;
    } else if (ret == 0) {
        return ETH_TIMEOUT;
    }
    
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    ssize_t received = recvfrom(priv->socket_fd, buffer, max_len, 0,
                                 (struct sockaddr *)&from_addr, &from_len);
    
    if (received < 0) {
        return ETH_ERROR;
    }
    
    /* 检查RTPS头部 */
    if (received >= (ssize_t)RTPS_HEADER_SIZE) {
        rtps_header_t *header = (rtps_header_t *)buffer;
        if (ntohl(header->magic) == RTPS_MAGIC_COOKIE) {
            /* 有效RTPS包，移除头部 */
            memmove(buffer, buffer + RTPS_HEADER_SIZE, received - RTPS_HEADER_SIZE);
            *received_len = received - RTPS_HEADER_SIZE;
        } else {
            *received_len = received;
        }
    } else {
        *received_len = received;
    }
    
    pthread_mutex_lock(&priv->stats_mutex);
    priv->stats.rx_packets++;
    priv->stats.rx_bytes += received;
    pthread_mutex_unlock(&priv->stats_mutex);
    
    return ETH_OK;
}

/**
 * @brief 注册接收回调
 */
static eth_status_t udp_register_callback(transport_handle_t *handle, 
                                           eth_rx_callback_t callback, 
                                           void *user_data)
{
    if (handle == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    udp_transport_private_t *priv = (udp_transport_private_t *)handle->private_data;
    if (priv == NULL) {
        return ETH_ERROR;
    }
    
    priv->rx_callback = callback;
    priv->rx_callback_data = user_data;
    
    return ETH_OK;
}

/**
 * @brief 获取传输状态
 */
static eth_status_t udp_get_status(transport_handle_t *handle, uint32_t *status)
{
    if (handle == NULL || status == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    udp_transport_private_t *priv = (udp_transport_private_t *)handle->private_data;
    if (priv == NULL) {
        return ETH_ERROR;
    }
    
    *status = 0;
    if (priv->initialized) *status |= 0x01;
    if (priv->connected) *status |= 0x02;
    
    return ETH_OK;
}

/* 静态操作函数表 */
static const transport_ops_t udp_ops = {
    .name = "UDP/RTPS",
    .init = udp_init,
    .deinit = udp_deinit,
    .send = udp_send,
    .receive = udp_receive,
    .register_callback = udp_register_callback,
    .get_status = udp_get_status
};

/* ============================================================================
 * 公开API实现
 * ============================================================================ */

transport_handle_t* udp_transport_create(const udp_transport_config_t *config)
{
    if (config == NULL) {
        return NULL;
    }
    
    transport_handle_t *handle = (transport_handle_t *)malloc(sizeof(transport_handle_t));
    if (handle == NULL) {
        return NULL;
    }
    
    udp_transport_private_t *priv = (udp_transport_private_t *)calloc(1, sizeof(udp_transport_private_t));
    if (priv == NULL) {
        free(handle);
        return NULL;
    }
    
    /* 复制配置 */
    memcpy(&priv->config, config, sizeof(udp_transport_config_t));
    
    handle->ops = &udp_ops;
    handle->private_data = priv;
    
    /* 执行初始化 */
    if (udp_init(handle, &config->base) != ETH_OK) {
        free(priv);
        free(handle);
        return NULL;
    }
    
    return handle;
}

eth_status_t udp_transport_destroy(transport_handle_t *handle)
{
    if (handle == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    udp_deinit(handle);
    
    if (handle->private_data != NULL) {
        free(handle->private_data);
        handle->private_data = NULL;
    }
    
    free(handle);
    return ETH_OK;
}

eth_status_t udp_transport_send_priority(transport_handle_t *handle,
                                          const uint8_t *data,
                                          uint32_t len,
                                          uint8_t priority,
                                          uint32_t timeout_ms)
{
    if (handle == NULL || data == NULL || len == 0) {
        return ETH_INVALID_PARAM;
    }
    
    if (priority >= UDP_PRIORITY_QUEUES) {
        priority = UDP_PRIORITY_QUEUES - 1;
    }
    
    udp_transport_private_t *priv = (udp_transport_private_t *)handle->private_data;
    if (priv == NULL || !priv->initialized) {
        return ETH_NOT_INIT;
    }
    
    /* 设置套接字优先级 */
    set_socket_priority(priv->socket_fd, priority);
    
    /* 构建RTPS数据包 */
    uint8_t tx_buffer[RTPS_MAX_PAYLOAD];
    rtps_header_t *header = (rtps_header_t *)tx_buffer;
    create_rtps_header(header, priority);
    
    memcpy(tx_buffer + RTPS_HEADER_SIZE, data, len);
    uint32_t total_len = RTPS_HEADER_SIZE + len;
    
    /* 发送数据 */
    ssize_t sent = sendto(priv->socket_fd, tx_buffer, total_len, 0,
                          (struct sockaddr *)&priv->remote_addr,
                          sizeof(priv->remote_addr));
    
    if (sent < 0) {
        return ETH_ERROR;
    }
    
    pthread_mutex_lock(&priv->stats_mutex);
    priv->stats.tx_packets++;
    priv->stats.tx_bytes += sent;
    pthread_mutex_unlock(&priv->stats_mutex);
    
    return ETH_OK;
}

eth_status_t udp_transport_join_multicast(transport_handle_t *handle,
                                           eth_ip_addr_t multicast_addr)
{
    if (handle == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    udp_transport_private_t *priv = (udp_transport_private_t *)handle->private_data;
    if (priv == NULL || !priv->initialized) {
        return ETH_NOT_INIT;
    }
    
    /* 创建组播socket */
    priv->socket_mc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (priv->socket_mc < 0) {
        return ETH_ERROR;
    }
    
    /* 允许地址复用 */
    int reuse = 1;
    setsockopt(priv->socket_mc, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    /* 绑定到组播端口 */
    struct sockaddr_in mc_bind_addr;
    memset(&mc_bind_addr, 0, sizeof(mc_bind_addr));
    mc_bind_addr.sin_family = AF_INET;
    mc_bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    mc_bind_addr.sin_port = htons(priv->config.base.local_port);
    
    if (bind(priv->socket_mc, (struct sockaddr *)&mc_bind_addr, sizeof(mc_bind_addr)) < 0) {
        close(priv->socket_mc);
        priv->socket_mc = -1;
        return ETH_ERROR;
    }
    
    /* 加入组播组 */
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = htonl(multicast_addr);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    
    if (setsockopt(priv->socket_mc, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
                   &mreq, sizeof(mreq)) < 0) {
        close(priv->socket_mc);
        priv->socket_mc = -1;
        return ETH_ERROR;
    }
    
    /* 设置TTL */
    uint8_t ttl = priv->config.multicast_ttl;
    setsockopt(priv->socket_mc, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
    
    /* 保存组播地址 */
    priv->config.multicast_addr = multicast_addr;
    memset(&priv->mc_addr, 0, sizeof(priv->mc_addr));
    priv->mc_addr.sin_family = AF_INET;
    priv->mc_addr.sin_addr.s_addr = htonl(multicast_addr);
    priv->mc_addr.sin_port = htons(priv->config.base.local_port);
    
    /* 启动组播接收线程 */
    priv->rx_thread_running = 1;
    pthread_create(&priv->rx_thread, NULL, multicast_rx_thread, priv);
    
    return ETH_OK;
}

eth_status_t udp_transport_leave_multicast(transport_handle_t *handle,
                                            eth_ip_addr_t multicast_addr)
{
    if (handle == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    udp_transport_private_t *priv = (udp_transport_private_t *)handle->private_data;
    if (priv == NULL || !priv->initialized) {
        return ETH_NOT_INIT;
    }
    
    /* 停止线程 */
    priv->rx_thread_running = 0;
    pthread_join(priv->rx_thread, NULL);
    
    /* 离开组播组 */
    if (priv->socket_mc >= 0) {
        struct ip_mreq mreq;
        mreq.imr_multiaddr.s_addr = htonl(multicast_addr);
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        setsockopt(priv->socket_mc, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
        
        close(priv->socket_mc);
        priv->socket_mc = -1;
    }
    
    return ETH_OK;
}

eth_status_t udp_transport_set_tsn_shaping(transport_handle_t *handle,
                                            tsn_traffic_class_t traffic_class,
                                            const tsn_config_t *config)
{
    if (handle == NULL || config == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    udp_transport_private_t *priv = (udp_transport_private_t *)handle->private_data;
    if (priv == NULL || !priv->initialized) {
        return ETH_NOT_INIT;
    }
    
    /* 保存TSN配置 */
    memcpy(&priv->config.tsn, config, sizeof(tsn_config_t));
    
    /* 根据流量类别设置优先级 */
    int socket_prio = 0;
    switch (traffic_class) {
        case TSN_TC_CRITICAL:    socket_prio = 7; break;
        case TSN_TC_MEDIA:       socket_prio = 5; break;
        case TSN_TC_DIAGNOSTICS: socket_prio = 3; break;
        case TSN_TC_BULK:        socket_prio = 0; break;
        default:                 socket_prio = 0;
    }
    
    if (setsockopt(priv->socket_fd, SOL_SOCKET, SO_PRIORITY, 
                   &socket_prio, sizeof(socket_prio)) < 0) {
        return ETH_ERROR;
    }
    
    return ETH_OK;
}

eth_status_t udp_transport_set_time_triggered(transport_handle_t *handle,
                                               bool enable,
                                               uint64_t cycle_time_ns,
                                               uint64_t transmit_window_ns)
{
    if (handle == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    udp_transport_private_t *priv = (udp_transport_private_t *)handle->private_data;
    if (priv == NULL || !priv->initialized) {
        return ETH_NOT_INIT;
    }
    
    priv->config.enable_tte = enable;
    priv->tte_cycle_time_ns = cycle_time_ns;
    
    if (enable) {
        priv->tte_next_tx_time_ns = get_time_ns();
    }
    
    (void)transmit_window_ns; /* 保留用于未来扩展 */
    
    return ETH_OK;
}

eth_status_t udp_transport_get_stats(transport_handle_t *handle,
                                      udp_transport_stats_t *stats)
{
    if (handle == NULL || stats == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    udp_transport_private_t *priv = (udp_transport_private_t *)handle->private_data;
    if (priv == NULL) {
        return ETH_ERROR;
    }
    
    pthread_mutex_lock(&priv->stats_mutex);
    memcpy(stats, &priv->stats, sizeof(udp_transport_stats_t));
    pthread_mutex_unlock(&priv->stats_mutex);
    
    return ETH_OK;
}

eth_status_t udp_transport_clear_stats(transport_handle_t *handle)
{
    if (handle == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    udp_transport_private_t *priv = (udp_transport_private_t *)handle->private_data;
    if (priv == NULL) {
        return ETH_ERROR;
    }
    
    pthread_mutex_lock(&priv->stats_mutex);
    memset(&priv->stats, 0, sizeof(priv->stats));
    priv->stats.latency_min_us = 0xFFFFFFFF;
    pthread_mutex_unlock(&priv->stats_mutex);
    
    return ETH_OK;
}

eth_status_t udp_transport_set_automotive_eth(transport_handle_t *handle,
                                               automotive_eth_speed_t speed)
{
    if (handle == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    udp_transport_private_t *priv = (udp_transport_private_t *)handle->private_data;
    if (priv == NULL || !priv->initialized) {
        return ETH_NOT_INIT;
    }
    
    priv->config.eth_speed = speed;
    return set_automotive_eth_params(priv->socket_fd, speed) == 0 ? ETH_OK : ETH_ERROR;
}

eth_status_t udp_transport_set_deterministic(transport_handle_t *handle,
                                              uint32_t deadline_us,
                                              uint32_t offset_us)
{
    if (handle == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    udp_transport_private_t *priv = (udp_transport_private_t *)handle->private_data;
    if (priv == NULL || !priv->initialized) {
        return ETH_NOT_INIT;
    }
    
    priv->config.deadline_us = deadline_us;
    priv->config.offset_us = offset_us;
    priv->config.enable_deadline = true;
    
    return ETH_OK;
}

eth_status_t udp_transport_get_latency(transport_handle_t *handle,
                                        uint32_t *latency_us)
{
    if (handle == NULL || latency_us == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    udp_transport_private_t *priv = (udp_transport_private_t *)handle->private_data;
    if (priv == NULL) {
        return ETH_ERROR;
    }
    
    pthread_mutex_lock(&priv->stats_mutex);
    *latency_us = priv->stats.latency_avg_us;
    pthread_mutex_unlock(&priv->stats_mutex);
    
    return ETH_OK;
}

eth_status_t udp_transport_init(void)
{
    /* 模块级初始化 */
    return ETH_OK;
}
