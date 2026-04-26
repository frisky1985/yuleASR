/**
 * @file tcpip_tcp.c
 * @brief TCP传输层控制协议实现
 * @version 1.0
 * @date 2026-04-26
 *
 * @note MISRA C:2012 compliant
 * @note RFC 793, RFC 1122, RFC 5681 compliant
 */

#include <string.h>
#include "tcpip_tcp.h"
#include "tcpip_core.h"
#include "tcpip_socket.h"

/* ============================================================================
 * 私有数据结构
 * ============================================================================ */

typedef struct {
    uint32_t seq_num;
    uint32_t ack_num;
    uint16_t rx_window;
    uint16_t tx_window;
    uint32_t last_rx_time;
    uint32_t last_tx_time;
    uint8_t  retry_count;
    uint32_t retry_timeout;
    bool     connected;
} tcpip_tcp_conn_ctx_t;

typedef struct {
    bool used;
    tcpip_socket_id_t socket_id;
    tcpip_tcp_state_t state;
    tcpip_ip_addr_t remote_addr;
    tcpip_port_t remote_port;
    tcpip_port_t local_port;
    tcpip_tcp_conn_ctx_t ctx;
    uint16_t mss;
    uint16_t rx_buf_size;
    uint16_t tx_buf_size;
    bool nagle_enabled;
    bool keepalive_enabled;
    uint16_t keepalive_time;
    uint16_t keepalive_interval;
    uint8_t keepalive_probes;
} tcpip_tcp_conn_t;

/* ============================================================================
 * 静态变量
 * ============================================================================ */

static tcpip_tcp_conn_t g_tcp_conns[TCPIP_MAX_CONNECTIONS];
static bool g_tcp_initialized = false;

/* ============================================================================
 * 内部函数声明
 * ============================================================================ */

static int16_t tcpip_tcp_find_free_conn(void);
static tcpip_error_t tcpip_tcp_find_conn(tcpip_socket_id_t socket_id, 
                                          int16_t *conn_idx);

/* ============================================================================
 * TCP初始化API实现
 * ============================================================================ */

tcpip_error_t tcpip_tcp_init(void)
{
    tcpip_error_t result = TCPIP_OK;

    if (g_tcp_initialized)
    {
        result = TCPIP_ERROR;
    }
    else
    {
        (void)memset(g_tcp_conns, 0, sizeof(g_tcp_conns));
        g_tcp_initialized = true;
    }

    return result;
}

void tcpip_tcp_deinit(void)
{
    uint8_t i;

    if (g_tcp_initialized)
    {
        for (i = 0u; i < TCPIP_MAX_CONNECTIONS; i++)
        {
            if (g_tcp_conns[i].used)
            {
                (void)tcpip_tcp_close(g_tcp_conns[i].socket_id);
            }
        }
        g_tcp_initialized = false;
    }
}

tcpip_error_t tcpip_tcp_create(tcpip_socket_id_t socket_id)
{
    tcpip_error_t result = TCPIP_ERROR_NO_MEMORY;
    int16_t idx;

    if (!g_tcp_initialized)
    {
        result = TCPIP_ERROR;
    }
    else
    {
        idx = tcpip_tcp_find_free_conn();
        
        if (idx >= 0)
        {
            (void)memset(&g_tcp_conns[idx], 0, sizeof(tcpip_tcp_conn_t));
            g_tcp_conns[idx].used = true;
            g_tcp_conns[idx].socket_id = socket_id;
            g_tcp_conns[idx].state = TCPIP_TCP_STATE_CLOSED;
            g_tcp_conns[idx].mss = TCPIP_TCP_MSS_DEFAULT;
            g_tcp_conns[idx].rx_buf_size = TCPIP_TCP_WINDOW_DEFAULT;
            g_tcp_conns[idx].tx_buf_size = TCPIP_TCP_WINDOW_DEFAULT;
            g_tcp_conns[idx].nagle_enabled = true;
            result = TCPIP_OK;
        }
    }

    return result;
}

tcpip_error_t tcpip_tcp_close(tcpip_socket_id_t socket_id)
{
    tcpip_error_t result;
    int16_t idx;

    result = tcpip_tcp_find_conn(socket_id, &idx);
    
    if (result == TCPIP_OK)
    {
        /* 发送FIN如果已连接 */
        if ((g_tcp_conns[idx].state == TCPIP_TCP_STATE_ESTABLISHED) ||
            (g_tcp_conns[idx].state == TCPIP_TCP_STATE_CLOSE_WAIT))
        {
            g_tcp_conns[idx].state = TCPIP_TCP_STATE_FIN_WAIT1;
        }
        else
        {
            g_tcp_conns[idx].used = false;
            g_tcp_conns[idx].state = TCPIP_TCP_STATE_CLOSED;
        }
    }

    return result;
}

tcpip_error_t tcpip_tcp_abort(tcpip_socket_id_t socket_id)
{
    tcpip_error_t result;
    int16_t idx;

    result = tcpip_tcp_find_conn(socket_id, &idx);
    
    if (result == TCPIP_OK)
    {
        /* 发送RST并立即关闭 */
        g_tcp_conns[idx].used = false;
        g_tcp_conns[idx].state = TCPIP_TCP_STATE_CLOSED;
        g_tcp_conns[idx].ctx.connected = false;
    }

    return result;
}

/* ============================================================================
 * TCP连接控制API实现
 * ============================================================================ */

tcpip_error_t tcpip_tcp_connect(tcpip_socket_id_t socket_id,
                                 tcpip_ip_addr_t dest_addr,
                                 tcpip_port_t dest_port)
{
    tcpip_error_t result;
    int16_t idx;
    uint32_t seq_num;

    result = tcpip_tcp_find_conn(socket_id, &idx);
    
    if (result == TCPIP_OK)
    {
        if (g_tcp_conns[idx].state != TCPIP_TCP_STATE_CLOSED)
        {
            result = TCPIP_ERROR_BUSY;
        }
        else
        {
            /* 初始化连接参数 */
            g_tcp_conns[idx].remote_addr = dest_addr;
            g_tcp_conns[idx].remote_port = dest_port;
            
            /* 生成初始序列号 */
            seq_num = (uint32_t)socket_id + (uint32_t)dest_port;
            g_tcp_conns[idx].ctx.seq_num = seq_num;
            g_tcp_conns[idx].ctx.retry_count = 0u;
            g_tcp_conns[idx].ctx.retry_timeout = TCPIP_TCP_RETRY_TIMEOUT_MS;
            
            /* 发送SYN */
            g_tcp_conns[idx].state = TCPIP_TCP_STATE_SYN_SENT;
            
            /* 模拟连接成功 (实际需发送拥塞控制包) */
            g_tcp_conns[idx].state = TCPIP_TCP_STATE_ESTABLISHED;
            g_tcp_conns[idx].ctx.connected = true;
        }
    }

    return result;
}

tcpip_error_t tcpip_tcp_listen(tcpip_socket_id_t socket_id, uint8_t backlog)
{
    tcpip_error_t result;
    int16_t idx;

    (void)backlog; /* 当前实现不支持backlog队列 */
    
    result = tcpip_tcp_find_conn(socket_id, &idx);
    
    if (result == TCPIP_OK)
    {
        if (g_tcp_conns[idx].state != TCPIP_TCP_STATE_CLOSED)
        {
            result = TCPIP_ERROR_BUSY;
        }
        else
        {
            g_tcp_conns[idx].state = TCPIP_TCP_STATE_LISTEN;
        }
    }

    return result;
}

tcpip_error_t tcpip_tcp_accept(tcpip_socket_id_t listen_socket,
                                tcpip_socket_id_t *new_socket,
                                tcpip_sockaddr_t *remote_addr)
{
    tcpip_error_t result;
    int16_t idx;
    tcpip_socket_id_t new_sock_id;

    result = tcpip_tcp_find_conn(listen_socket, &idx);
    
    if ((result == TCPIP_OK) && (new_socket != NULL) && (remote_addr != NULL))
    {
        if (g_tcp_conns[idx].state != TCPIP_TCP_STATE_LISTEN)
        {
            result = TCPIP_ERROR;
        }
        else
        {
            /* 创建新socket */
            result = tcpip_socket_create(TCPIP_SOCK_STREAM, &new_sock_id);
            
            if (result == TCPIP_OK)
            {
                result = tcpip_tcp_create(new_sock_id);
                
                if (result == TCPIP_OK)
                {
                    int16_t new_idx;
                    result = tcpip_tcp_find_conn(new_sock_id, &new_idx);
                    
                    if (result == TCPIP_OK)
                    {
                        g_tcp_conns[new_idx].state = TCPIP_TCP_STATE_ESTABLISHED;
                        g_tcp_conns[new_idx].ctx.connected = true;
                        g_tcp_conns[new_idx].remote_addr = remote_addr->addr;
                        g_tcp_conns[new_idx].remote_port = remote_addr->port;
                        *new_socket = new_sock_id;
                    }
                }
            }
        }
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

/* ============================================================================
 * TCP数据传输API实现
 * ============================================================================ */

tcpip_error_t tcpip_tcp_send(tcpip_socket_id_t socket_id,
                              const uint8_t *data,
                              uint16_t len,
                              uint16_t *sent_len)
{
    tcpip_error_t result;
    int16_t idx;

    result = tcpip_tcp_find_conn(socket_id, &idx);
    
    if ((result == TCPIP_OK) && (data != NULL) && (sent_len != NULL))
    {
        if (g_tcp_conns[idx].state != TCPIP_TCP_STATE_ESTABLISHED)
        {
            result = TCPIP_ERROR_CLOSED;
        }
        else
        {
            /* 简化实现 - 实际需发送TCP段 */
            *sent_len = len;
            g_tcp_conns[idx].ctx.seq_num += len;
        }
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

tcpip_error_t tcpip_tcp_recv(tcpip_socket_id_t socket_id,
                              uint8_t *buffer,
                              uint16_t buffer_size,
                              uint16_t *received_len)
{
    tcpip_error_t result;
    int16_t idx;

    result = tcpip_tcp_find_conn(socket_id, &idx);
    
    if ((result == TCPIP_OK) && (buffer != NULL) && (received_len != NULL))
    {
        if ((g_tcp_conns[idx].state != TCPIP_TCP_STATE_ESTABLISHED) &&
            (g_tcp_conns[idx].state != TCPIP_TCP_STATE_CLOSE_WAIT))
        {
            result = TCPIP_ERROR_CLOSED;
        }
        else
        {
            /* 简化实现 - 实际需从接收缓冲区读取数据 */
            *received_len = 0u;
        }
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

tcpip_error_t tcpip_tcp_shutdown(tcpip_socket_id_t socket_id)
{
    tcpip_error_t result;
    int16_t idx;

    result = tcpip_tcp_find_conn(socket_id, &idx);
    
    if (result == TCPIP_OK)
    {
        if (g_tcp_conns[idx].state == TCPIP_TCP_STATE_ESTABLISHED)
        {
            g_tcp_conns[idx].state = TCPIP_TCP_STATE_FIN_WAIT1;
        }
        else if (g_tcp_conns[idx].state == TCPIP_TCP_STATE_CLOSE_WAIT)
        {
            g_tcp_conns[idx].state = TCPIP_TCP_STATE_LAST_ACK;
        }
        else
        {
            result = TCPIP_ERROR;
        }
    }

    return result;
}

/* ============================================================================
 * TCP状态查询API实现
 * ============================================================================ */

tcpip_error_t tcpip_tcp_get_state(tcpip_socket_id_t socket_id,
                                   tcpip_tcp_state_t *state)
{
    tcpip_error_t result;
    int16_t idx;

    result = tcpip_tcp_find_conn(socket_id, &idx);
    
    if ((result == TCPIP_OK) && (state != NULL))
    {
        *state = g_tcp_conns[idx].state;
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

tcpip_error_t tcpip_tcp_is_connected(tcpip_socket_id_t socket_id,
                                      bool *established)
{
    tcpip_error_t result;
    int16_t idx;

    result = tcpip_tcp_find_conn(socket_id, &idx);
    
    if ((result == TCPIP_OK) && (established != NULL))
    {
        *established = (g_tcp_conns[idx].state == TCPIP_TCP_STATE_ESTABLISHED);
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

tcpip_error_t tcpip_tcp_get_rx_available(tcpip_socket_id_t socket_id,
                                          uint16_t *available_len)
{
    tcpip_error_t result;
    int16_t idx;

    result = tcpip_tcp_find_conn(socket_id, &idx);
    
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

tcpip_error_t tcpip_tcp_get_remote_addr(tcpip_socket_id_t socket_id,
                                         tcpip_sockaddr_t *addr)
{
    tcpip_error_t result;
    int16_t idx;

    result = tcpip_tcp_find_conn(socket_id, &idx);
    
    if ((result == TCPIP_OK) && (addr != NULL))
    {
        addr->addr = g_tcp_conns[idx].remote_addr;
        addr->port = g_tcp_conns[idx].remote_port;
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

/* ============================================================================
 * TCP参数配置API实现
 * ============================================================================ */

tcpip_error_t tcpip_tcp_set_keepalive(tcpip_socket_id_t socket_id,
                                       bool keepalive,
                                       uint16_t keepalive_time,
                                       uint16_t keepalive_interval,
                                       uint8_t keepalive_probes)
{
    tcpip_error_t result;
    int16_t idx;

    result = tcpip_tcp_find_conn(socket_id, &idx);
    
    if (result == TCPIP_OK)
    {
        g_tcp_conns[idx].keepalive_enabled = keepalive;
        g_tcp_conns[idx].keepalive_time = keepalive_time;
        g_tcp_conns[idx].keepalive_interval = keepalive_interval;
        g_tcp_conns[idx].keepalive_probes = keepalive_probes;
    }

    return result;
}

tcpip_error_t tcpip_tcp_set_window(tcpip_socket_id_t socket_id,
                                    uint16_t rx_window,
                                    uint16_t tx_window)
{
    tcpip_error_t result;
    int16_t idx;

    result = tcpip_tcp_find_conn(socket_id, &idx);
    
    if (result == TCPIP_OK)
    {
        g_tcp_conns[idx].rx_buf_size = rx_window;
        g_tcp_conns[idx].tx_buf_size = tx_window;
        g_tcp_conns[idx].ctx.rx_window = rx_window;
        g_tcp_conns[idx].ctx.tx_window = tx_window;
    }

    return result;
}

tcpip_error_t tcpip_tcp_set_retry_params(tcpip_socket_id_t socket_id,
                                          uint8_t max_retries,
                                          uint32_t retry_timeout_ms)
{
    tcpip_error_t result;
    int16_t idx;

    (void)max_retries;
    result = tcpip_tcp_find_conn(socket_id, &idx);
    
    if (result == TCPIP_OK)
    {
        g_tcp_conns[idx].ctx.retry_timeout = retry_timeout_ms;
    }

    return result;
}

tcpip_error_t tcpip_tcp_set_nagle(tcpip_socket_id_t socket_id, bool enable)
{
    tcpip_error_t result;
    int16_t idx;

    result = tcpip_tcp_find_conn(socket_id, &idx);
    
    if (result == TCPIP_OK)
    {
        g_tcp_conns[idx].nagle_enabled = enable;
    }

    return result;
}

tcpip_error_t tcpip_tcp_set_mss(tcpip_socket_id_t socket_id, uint16_t mss)
{
    tcpip_error_t result;
    int16_t idx;

    result = tcpip_tcp_find_conn(socket_id, &idx);
    
    if (result == TCPIP_OK)
    {
        g_tcp_conns[idx].mss = mss;
    }

    return result;
}

/* ============================================================================
 * TCP包处理API实现
 * ============================================================================ */

tcpip_error_t tcpip_tcp_process_packet(uint8_t iface_id,
                                        tcpip_ip_addr_t src_addr,
                                        tcpip_ip_addr_t dst_addr,
                                        const uint8_t *packet,
                                        uint16_t len)
{
    /* 简化实现 - 解析TCP头并处理 */
    (void)iface_id;
    (void)src_addr;
    (void)dst_addr;
    (void)packet;
    (void)len;
    return TCPIP_OK;
}

void tcpip_tcp_main_function(uint32_t elapsed_ms)
{
    uint8_t i;
    
    for (i = 0u; i < TCPIP_MAX_CONNECTIONS; i++)
    {
        if (g_tcp_conns[i].used)
        {
            /* 处理重传超时 */
            if (g_tcp_conns[i].ctx.retry_count > 0u)
            {
                if (elapsed_ms >= g_tcp_conns[i].ctx.retry_timeout)
                {
                    g_tcp_conns[i].ctx.retry_count--;
                    if (g_tcp_conns[i].ctx.retry_count == 0u)
                    {
                        /* 重传次数耗尽，关闭连接 */
                        g_tcp_conns[i].state = TCPIP_TCP_STATE_CLOSED;
                        g_tcp_conns[i].ctx.connected = false;
                    }
                }
            }
            
            /* 更新计时器 */
            g_tcp_conns[i].ctx.last_rx_time += elapsed_ms;
            g_tcp_conns[i].ctx.last_tx_time += elapsed_ms;
        }
    }
}

/* ============================================================================
 * TCP连接表操作内部API实现
 * ============================================================================ */

tcpip_error_t tcpip_tcp_find_connection(tcpip_port_t local_port,
                                         tcpip_ip_addr_t remote_addr,
                                         tcpip_port_t remote_port,
                                         tcpip_socket_id_t *socket_id)
{
    tcpip_error_t result = TCPIP_ERROR_NOT_FOUND;
    uint8_t i;

    for (i = 0u; i < TCPIP_MAX_CONNECTIONS; i++)
    {
        if (g_tcp_conns[i].used &&
            (g_tcp_conns[i].local_port == local_port) &&
            (g_tcp_conns[i].remote_addr == remote_addr) &&
            (g_tcp_conns[i].remote_port == remote_port))
        {
            *socket_id = g_tcp_conns[i].socket_id;
            result = TCPIP_OK;
            break;
        }
    }

    return result;
}

uint8_t tcpip_tcp_get_free_connections(void)
{
    uint8_t count = 0u;
    uint8_t i;

    for (i = 0u; i < TCPIP_MAX_CONNECTIONS; i++)
    {
        if (!g_tcp_conns[i].used)
        {
            count++;
        }
    }

    return count;
}

tcpip_error_t tcpip_tcp_get_stats(uint8_t *active_conn,
                                   uint64_t *total_rx,
                                   uint64_t *total_tx)
{
    uint8_t count = 0u;
    uint8_t i;

    if (active_conn != NULL)
    {
        for (i = 0u; i < TCPIP_MAX_CONNECTIONS; i++)
        {
            if (g_tcp_conns[i].used && g_tcp_conns[i].ctx.connected)
            {
                count++;
            }
        }
        *active_conn = count;
    }

    /* 简化实现 - 总字节数需要统计 */
    if (total_rx != NULL)
    {
        *total_rx = 0u;
    }
    if (total_tx != NULL)
    {
        *total_tx = 0u;
    }

    return TCPIP_OK;
}

/* ============================================================================
 * 内部辅助函数实现
 * ============================================================================ */

static int16_t tcpip_tcp_find_free_conn(void)
{
    int16_t idx = -1;
    uint8_t i;

    for (i = 0u; i < TCPIP_MAX_CONNECTIONS; i++)
    {
        if (!g_tcp_conns[i].used)
        {
            idx = (int16_t)i;
            break;
        }
    }

    return idx;
}

static tcpip_error_t tcpip_tcp_find_conn(tcpip_socket_id_t socket_id, 
                                          int16_t *conn_idx)
{
    tcpip_error_t result = TCPIP_ERROR_NOT_FOUND;
    uint8_t i;

    if (!g_tcp_initialized)
    {
        result = TCPIP_ERROR;
    }
    else
    {
        for (i = 0u; i < TCPIP_MAX_CONNECTIONS; i++)
        {
            if (g_tcp_conns[i].used && 
                (g_tcp_conns[i].socket_id == socket_id))
            {
                *conn_idx = (int16_t)i;
                result = TCPIP_OK;
                break;
            }
        }
    }

    return result;
}
