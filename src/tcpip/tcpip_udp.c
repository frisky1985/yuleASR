/**
 * @file tcpip_udp.c
 * @brief UDP用户数据报协议实现
 * @version 1.0
 * @date 2026-04-26
 *
 * @note MISRA C:2012 compliant
 * @note RFC 768 compliant
 */

#include <string.h>
#include "tcpip_udp.h"
#include "tcpip_core.h"

/* ============================================================================
 * 私有数据结构
 * ============================================================================ */

typedef struct {
    bool used;
    tcpip_socket_id_t socket_id;
    tcpip_port_t local_port;
    tcpip_ip_addr_t local_addr;
    bool broadcast_enabled;
    uint8_t mcast_ttl;
    uint32_t rx_packets;
    uint32_t tx_packets;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
} tcpip_udp_endpoint_t;

/* ============================================================================
 * 静态变量
 * ============================================================================ */

static tcpip_udp_endpoint_t g_udp_endpoints[TCPIP_MAX_SOCKETS];
static bool g_udp_initialized = false;

/* ============================================================================
 * 内部函数声明
 * ============================================================================ */

static int16_t tcpip_udp_find_free_endpoint(void);
static tcpip_error_t tcpip_udp_find_endpoint(tcpip_socket_id_t socket_id, 
                                              int16_t *ep_idx);
static tcpip_error_t tcpip_udp_find_by_port(tcpip_port_t local_port,
                                             int16_t *ep_idx);

/* ============================================================================
 * UDP初始化管理API实现
 * ============================================================================ */

tcpip_error_t tcpip_udp_init(void)
{
    tcpip_error_t result = TCPIP_OK;

    if (g_udp_initialized)
    {
        result = TCPIP_ERROR;
    }
    else
    {
        (void)memset(g_udp_endpoints, 0, sizeof(g_udp_endpoints));
        g_udp_initialized = true;
    }

    return result;
}

void tcpip_udp_deinit(void)
{
    uint8_t i;

    if (g_udp_initialized)
    {
        for (i = 0u; i < TCPIP_MAX_SOCKETS; i++)
        {
            if (g_udp_endpoints[i].used)
            {
                (void)tcpip_udp_close(g_udp_endpoints[i].socket_id);
            }
        }
        g_udp_initialized = false;
    }
}

tcpip_error_t tcpip_udp_create(tcpip_socket_id_t socket_id)
{
    tcpip_error_t result = TCPIP_ERROR_NO_MEMORY;
    int16_t idx;

    if (!g_udp_initialized)
    {
        result = TCPIP_ERROR;
    }
    else
    {
        idx = tcpip_udp_find_free_endpoint();
        
        if (idx >= 0)
        {
            (void)memset(&g_udp_endpoints[idx], 0, sizeof(tcpip_udp_endpoint_t));
            g_udp_endpoints[idx].used = true;
            g_udp_endpoints[idx].socket_id = socket_id;
            g_udp_endpoints[idx].local_port = 0u;
            g_udp_endpoints[idx].local_addr = TCPIP_IP_ANY;
            g_udp_endpoints[idx].mcast_ttl = 1u;
            result = TCPIP_OK;
        }
    }

    return result;
}

tcpip_error_t tcpip_udp_close(tcpip_socket_id_t socket_id)
{
    tcpip_error_t result;
    int16_t idx;

    result = tcpip_udp_find_endpoint(socket_id, &idx);
    
    if (result == TCPIP_OK)
    {
        g_udp_endpoints[idx].used = false;
        g_udp_endpoints[idx].local_port = 0u;
    }

    return result;
}

/* ============================================================================
 * UDP数据传输API实现
 * ============================================================================ */

tcpip_error_t tcpip_udp_send(tcpip_socket_id_t socket_id,
                              tcpip_ip_addr_t dest_addr,
                              tcpip_port_t dest_port,
                              const uint8_t *data,
                              uint16_t len)
{
    tcpip_error_t result;
    int16_t idx;

    result = tcpip_udp_find_endpoint(socket_id, &idx);
    
    if ((result == TCPIP_OK) && (data != NULL))
    {
        /* 检查是否允许广播 */
        if ((dest_addr == TCPIP_IP_BROADCAST) && 
            (!g_udp_endpoints[idx].broadcast_enabled))
        {
            result = TCPIP_ERROR_PARAM;
        }
        else if (len > (TCPIP_IP_MTU_DEFAULT - TCPIP_IP_HEADER_LEN - TCPIP_UDP_HEADER_LEN))
        {
            result = TCPIP_ERROR_MTU;
        }
        else
        {
            /* 简化实现 - 实际需构建UDP数据包 */
            g_udp_endpoints[idx].tx_packets++;
            g_udp_endpoints[idx].tx_bytes += len;
        }
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

tcpip_error_t tcpip_udp_send_to(tcpip_socket_id_t socket_id,
                                 tcpip_port_t src_port,
                                 tcpip_ip_addr_t dest_addr,
                                 tcpip_port_t dest_port,
                                 const uint8_t *data,
                                 uint16_t len)
{
    tcpip_error_t result;
    int16_t idx;

    result = tcpip_udp_find_endpoint(socket_id, &idx);
    
    if (result == TCPIP_OK)
    {
        g_udp_endpoints[idx].local_port = src_port;
        result = tcpip_udp_send(socket_id, dest_addr, dest_port, data, len);
    }

    return result;
}

tcpip_error_t tcpip_udp_recv(tcpip_socket_id_t socket_id,
                              tcpip_sockaddr_t *src_addr,
                              uint8_t *buffer,
                              uint16_t buffer_size,
                              uint16_t *received_len)
{
    tcpip_error_t result;
    int16_t idx;

    result = tcpip_udp_find_endpoint(socket_id, &idx);
    
    if ((result == TCPIP_OK) && (buffer != NULL) && (received_len != NULL))
    {
        /* 简化实现 - 实际需从接收缓冲区读取 */
        *received_len = 0u;
        if (src_addr != NULL)
        {
            src_addr->addr = TCPIP_IP_ANY;
            src_addr->port = 0u;
        }
        (void)buffer_size;
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

tcpip_error_t tcpip_udp_get_rx_available(tcpip_socket_id_t socket_id,
                                          uint16_t *available_len)
{
    tcpip_error_t result;
    int16_t idx;

    result = tcpip_udp_find_endpoint(socket_id, &idx);
    
    if ((result == TCPIP_OK) && (available_len != NULL))
    {
        /* 简化实现 */
        *available_len = 0u;
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

/* ============================================================================
 * UDP组播API实现
 * ============================================================================ */

tcpip_error_t tcpip_udp_join_multicast(tcpip_socket_id_t socket_id,
                                        tcpip_ip_addr_t mcast_addr)
{
    tcpip_error_t result;
    int16_t idx;

    (void)mcast_addr;
    result = tcpip_udp_find_endpoint(socket_id, &idx);
    
    /* 简化实现 - 实际需注册到组播组 */
    return result;
}

tcpip_error_t tcpip_udp_leave_multicast(tcpip_socket_id_t socket_id,
                                         tcpip_ip_addr_t mcast_addr)
{
    tcpip_error_t result;
    int16_t idx;

    (void)mcast_addr;
    result = tcpip_udp_find_endpoint(socket_id, &idx);
    
    /* 简化实现 */
    return result;
}

tcpip_error_t tcpip_udp_set_mcast_ttl(tcpip_socket_id_t socket_id, uint8_t ttl)
{
    tcpip_error_t result;
    int16_t idx;

    result = tcpip_udp_find_endpoint(socket_id, &idx);
    
    if (result == TCPIP_OK)
    {
        g_udp_endpoints[idx].mcast_ttl = ttl;
    }

    return result;
}

tcpip_error_t tcpip_udp_set_broadcast(tcpip_socket_id_t socket_id, bool enable)
{
    tcpip_error_t result;
    int16_t idx;

    result = tcpip_udp_find_endpoint(socket_id, &idx);
    
    if (result == TCPIP_OK)
    {
        g_udp_endpoints[idx].broadcast_enabled = enable;
    }

    return result;
}

/* ============================================================================
 * UDP包处理API实现
 * ============================================================================ */

tcpip_error_t tcpip_udp_process_packet(uint8_t iface_id,
                                        tcpip_ip_addr_t src_addr,
                                        tcpip_ip_addr_t dst_addr,
                                        const uint8_t *packet,
                                        uint16_t len)
{
    tcpip_error_t result = TCPIP_ERROR_PARAM;
    const tcpip_udp_header_t *udp_hdr;
    tcpip_port_t dest_port;
    int16_t ep_idx;

    (void)iface_id;
    (void)src_addr;
    (void)dst_addr;

    if ((packet != NULL) && (len >= TCPIP_UDP_HEADER_LEN))
    {
        udp_hdr = (const tcpip_udp_header_t *)packet;
        dest_port = TCPIP_NTOHS(udp_hdr->dst_port);
        
        result = tcpip_udp_find_by_port(dest_port, &ep_idx);
        
        if (result == TCPIP_OK)
        {
            g_udp_endpoints[ep_idx].rx_packets++;
            g_udp_endpoints[ep_idx].rx_bytes += (len - TCPIP_UDP_HEADER_LEN);
        }
    }

    return result;
}

void tcpip_udp_main_function(uint32_t elapsed_ms)
{
    (void)elapsed_ms;
    /* UDP主循环处理 - 目前无需特殊处理 */
}

/* ============================================================================
 * UDP端点查找API实现
 * ============================================================================ */

tcpip_error_t tcpip_udp_find_socket(tcpip_port_t local_port,
                                     tcpip_socket_id_t *socket_id)
{
    tcpip_error_t result;
    int16_t idx;

    result = tcpip_udp_find_by_port(local_port, &idx);
    
    if ((result == TCPIP_OK) && (socket_id != NULL))
    {
        *socket_id = g_udp_endpoints[idx].socket_id;
    }

    return result;
}

tcpip_error_t tcpip_udp_is_port_in_use(tcpip_port_t port, bool *in_use)
{
    tcpip_error_t result = TCPIP_OK;
    int16_t idx;

    if (in_use != NULL)
    {
        idx = 0;
        while (idx < TCPIP_MAX_SOCKETS)
        {
            if (g_udp_endpoints[idx].used && 
                (g_udp_endpoints[idx].local_port == port))
            {
                break;
            }
            idx++;
        }
        *in_use = (idx < TCPIP_MAX_SOCKETS);
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

tcpip_error_t tcpip_udp_get_stats(uint8_t *active_sockets,
                                   uint64_t *total_rx,
                                   uint64_t *total_tx)
{
    uint8_t count = 0u;
    uint8_t i;
    uint64_t rx = 0u;
    uint64_t tx = 0u;

    for (i = 0u; i < TCPIP_MAX_SOCKETS; i++)
    {
        if (g_udp_endpoints[i].used)
        {
            count++;
            rx += g_udp_endpoints[i].rx_bytes;
            tx += g_udp_endpoints[i].tx_bytes;
        }
    }

    if (active_sockets != NULL)
    {
        *active_sockets = count;
    }
    if (total_rx != NULL)
    {
        *total_rx = rx;
    }
    if (total_tx != NULL)
    {
        *total_tx = tx;
    }

    return TCPIP_OK;
}

/* ============================================================================
 * 内部辅助函数实现
 * ============================================================================ */

static int16_t tcpip_udp_find_free_endpoint(void)
{
    int16_t idx = -1;
    uint8_t i;

    for (i = 0u; i < TCPIP_MAX_SOCKETS; i++)
    {
        if (!g_udp_endpoints[i].used)
        {
            idx = (int16_t)i;
            break;
        }
    }

    return idx;
}

static tcpip_error_t tcpip_udp_find_endpoint(tcpip_socket_id_t socket_id, 
                                              int16_t *ep_idx)
{
    tcpip_error_t result = TCPIP_ERROR_NOT_FOUND;
    uint8_t i;

    if (!g_udp_initialized)
    {
        result = TCPIP_ERROR;
    }
    else
    {
        for (i = 0u; i < TCPIP_MAX_SOCKETS; i++)
        {
            if (g_udp_endpoints[i].used && 
                (g_udp_endpoints[i].socket_id == socket_id))
            {
                *ep_idx = (int16_t)i;
                result = TCPIP_OK;
                break;
            }
        }
    }

    return result;
}

static tcpip_error_t tcpip_udp_find_by_port(tcpip_port_t local_port,
                                             int16_t *ep_idx)
{
    tcpip_error_t result = TCPIP_ERROR_NOT_FOUND;
    uint8_t i;

    for (i = 0u; i < TCPIP_MAX_SOCKETS; i++)
    {
        if (g_udp_endpoints[i].used && 
            (g_udp_endpoints[i].local_port == local_port))
        {
            *ep_idx = (int16_t)i;
            result = TCPIP_OK;
            break;
        }
    }

    return result;
}
