/**
 * @file tcpip_socket.c
 * @brief Socket API implementation
 * @version 1.0
 * @date 2026-04-26
 *
 * @note MISRA C:2012 compliant
 */

#include <string.h>
#include "tcpip_socket.h"
#include "tcpip_tcp.h"
#include "tcpip_udp.h"

/* ============================================================================
 * Private data structures
 * ============================================================================ */

typedef struct {
    bool used;
    tcpip_socket_type_t type;
    tcpip_socket_state_t state;
    tcpip_sockaddr_t local_addr;
    tcpip_sockaddr_t remote_addr;
    tcpip_socket_event_cb_t event_cb;
    void *event_cb_data;
    bool nonblocking;
    uint16_t rx_buf_size;
    uint16_t tx_buf_size;
    uint32_t rx_timeout_ms;
    uint32_t tx_timeout_ms;
} tcpip_socket_internal_t;

/* ============================================================================
 * Static variables
 * ============================================================================ */

static tcpip_socket_internal_t g_sockets[TCPIP_MAX_SOCKETS];
static bool g_socket_initialized = false;

/* ============================================================================
 * Internal function declarations
 * ============================================================================ */

static int16_t tcpip_socket_find_free(void);
static tcpip_error_t tcpip_socket_validate_params(tcpip_socket_id_t socket_id);

/* ============================================================================
 * Socket lifecycle API implementation
 * ============================================================================ */

tcpip_error_t tcpip_socket_init(void)
{
    tcpip_error_t result = TCPIP_OK;

    if (g_socket_initialized)
    {
        result = TCPIP_ERROR;
    }
    else
    {
        (void)memset(g_sockets, 0, sizeof(g_sockets));
        g_socket_initialized = true;
    }

    return result;
}

void tcpip_socket_deinit(void)
{
    uint8_t i;

    if (g_socket_initialized)
    {
        for (i = 0u; i < TCPIP_MAX_SOCKETS; i++)
        {
            if (g_sockets[i].used)
            {
                (void)tcpip_socket_close((tcpip_socket_id_t)i);
            }
        }
        g_socket_initialized = false;
    }
}

void tcpip_socket_main_function(uint32_t elapsed_ms)
{
    (void)elapsed_ms;
    /* Socket main function processing */
}

tcpip_error_t tcpip_socket_create(tcpip_socket_type_t type,
                                   tcpip_socket_id_t *socket_id)
{
    tcpip_error_t result = TCPIP_ERROR_PARAM;
    int16_t idx;

    if ((socket_id != NULL) && g_socket_initialized)
    {
        if ((type == TCPIP_SOCK_STREAM) || (type == TCPIP_SOCK_DGRAM))
        {
            idx = tcpip_socket_find_free();
            
            if (idx >= 0)
            {
                (void)memset(&g_sockets[idx], 0, sizeof(tcpip_socket_internal_t));
                g_sockets[idx].used = true;
                g_sockets[idx].type = type;
                g_sockets[idx].state = TCPIP_SOCK_STATE_CREATED;
                g_sockets[idx].rx_buf_size = 2048u;
                g_sockets[idx].tx_buf_size = 2048u;
                g_sockets[idx].rx_timeout_ms = 0xFFFFFFFFu;
                g_sockets[idx].tx_timeout_ms = 0xFFFFFFFFu;
                
                *socket_id = (tcpip_socket_id_t)idx;
                result = TCPIP_OK;
                
                /* Initialize transport layer */
                if (type == TCPIP_SOCK_STREAM)
                {
                    result = tcpip_tcp_create(*socket_id);
                }
                else
                {
                    result = tcpip_udp_create(*socket_id);
                }
            }
            else
            {
                result = TCPIP_ERROR_NO_MEMORY;
            }
        }
    }

    return result;
}

tcpip_error_t tcpip_socket_close(tcpip_socket_id_t socket_id)
{
    tcpip_error_t result;

    result = tcpip_socket_validate_params(socket_id);
    
    if (result == TCPIP_OK)
    {
        /* Close transport layer */
        if (g_sockets[socket_id].type == TCPIP_SOCK_STREAM)
        {
            (void)tcpip_tcp_close(socket_id);
        }
        else
        {
            (void)tcpip_udp_close(socket_id);
        }
        
        g_sockets[socket_id].used = false;
        g_sockets[socket_id].state = TCPIP_SOCK_STATE_UNUSED;
    }

    return result;
}

tcpip_error_t tcpip_socket_abort(tcpip_socket_id_t socket_id)
{
    tcpip_error_t result;

    result = tcpip_socket_validate_params(socket_id);
    
    if (result == TCPIP_OK)
    {
        if (g_sockets[socket_id].type == TCPIP_SOCK_STREAM)
        {
            (void)tcpip_tcp_abort(socket_id);
        }
        
        g_sockets[socket_id].used = false;
        g_sockets[socket_id].state = TCPIP_SOCK_STATE_UNUSED;
    }

    return result;
}

tcpip_error_t tcpip_socket_bind(tcpip_socket_id_t socket_id,
                                 tcpip_ip_addr_t local_addr,
                                 tcpip_port_t local_port,
                                 tcpip_port_t *assigned_port)
{
    tcpip_error_t result;
    tcpip_port_t port;

    result = tcpip_socket_validate_params(socket_id);
    
    if ((result == TCPIP_OK) && (assigned_port != NULL))
    {
        port = local_port;
        
        if (port == 0u)
        {
            /* Auto-assign port (simplified) */
            port = (tcpip_port_t)(1024u + (uint16_t)socket_id + 10000u);
        }
        
        g_sockets[socket_id].local_addr.addr = local_addr;
        g_sockets[socket_id].local_addr.port = port;
        g_sockets[socket_id].state = TCPIP_SOCK_STATE_BOUND;
        *assigned_port = port;
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

/* ============================================================================
 * TCP Socket API implementation
 * ============================================================================ */

tcpip_error_t tcpip_socket_listen(tcpip_socket_id_t socket_id,
                                   uint8_t backlog)
{
    tcpip_error_t result;

    result = tcpip_socket_validate_params(socket_id);
    
    if (result == TCPIP_OK)
    {
        if (g_sockets[socket_id].type != TCPIP_SOCK_STREAM)
        {
            result = TCPIP_ERROR_PARAM;
        }
        else if (g_sockets[socket_id].state != TCPIP_SOCK_STATE_BOUND)
        {
            result = TCPIP_ERROR;
        }
        else
        {
            result = tcpip_tcp_listen(socket_id, backlog);
            if (result == TCPIP_OK)
            {
                g_sockets[socket_id].state = TCPIP_SOCK_STATE_LISTENING;
            }
        }
    }

    return result;
}

tcpip_error_t tcpip_socket_accept(tcpip_socket_id_t socket_id,
                                   tcpip_socket_id_t *new_socket,
                                   tcpip_sockaddr_t *remote_addr)
{
    tcpip_error_t result;

    result = tcpip_socket_validate_params(socket_id);
    
    if ((result == TCPIP_OK) && (new_socket != NULL) && (remote_addr != NULL))
    {
        if (g_sockets[socket_id].type != TCPIP_SOCK_STREAM)
        {
            result = TCPIP_ERROR_PARAM;
        }
        else if (g_sockets[socket_id].state != TCPIP_SOCK_STATE_LISTENING)
        {
            result = TCPIP_ERROR;
        }
        else
        {
            result = tcpip_tcp_accept(socket_id, new_socket, remote_addr);
            if (result == TCPIP_OK)
            {
                g_sockets[*new_socket].state = TCPIP_SOCK_STATE_CONNECTED;
                g_sockets[*new_socket].remote_addr = *remote_addr;
            }
        }
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

tcpip_error_t tcpip_socket_connect(tcpip_socket_id_t socket_id,
                                    const tcpip_sockaddr_t *remote_addr)
{
    tcpip_error_t result;

    result = tcpip_socket_validate_params(socket_id);
    
    if ((result == TCPIP_OK) && (remote_addr != NULL))
    {
        if (g_sockets[socket_id].type != TCPIP_SOCK_STREAM)
        {
            result = TCPIP_ERROR_PARAM;
        }
        else if ((g_sockets[socket_id].state != TCPIP_SOCK_STATE_CREATED) &&
                 (g_sockets[socket_id].state != TCPIP_SOCK_STATE_BOUND))
        {
            result = TCPIP_ERROR;
        }
        else
        {
            result = tcpip_tcp_connect(socket_id, remote_addr->addr, remote_addr->port);
            if (result == TCPIP_OK)
            {
                g_sockets[socket_id].state = TCPIP_SOCK_STATE_CONNECTED;
                g_sockets[socket_id].remote_addr = *remote_addr;
            }
        }
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

tcpip_error_t tcpip_socket_disconnect(tcpip_socket_id_t socket_id)
{
    tcpip_error_t result;

    result = tcpip_socket_validate_params(socket_id);
    
    if (result == TCPIP_OK)
    {
        if (g_sockets[socket_id].type != TCPIP_SOCK_STREAM)
        {
            result = TCPIP_ERROR_PARAM;
        }
        else
        {
            result = tcpip_tcp_shutdown(socket_id);
            g_sockets[socket_id].state = TCPIP_SOCK_STATE_CLOSING;
        }
    }

    return result;
}

tcpip_error_t tcpip_socket_is_connected(tcpip_socket_id_t socket_id,
                                         bool *connected)
{
    tcpip_error_t result;
    tcpip_tcp_state_t tcp_state;

    result = tcpip_socket_validate_params(socket_id);
    
    if ((result == TCPIP_OK) && (connected != NULL))
    {
        if (g_sockets[socket_id].type == TCPIP_SOCK_STREAM)
        {
            result = tcpip_tcp_get_state(socket_id, &tcp_state);
            if (result == TCPIP_OK)
            {
                *connected = (tcp_state == TCPIP_TCP_STATE_ESTABLISHED);
            }
        }
        else
        {
            *connected = true;  /* UDP is connectionless */
        }
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

/* ============================================================================
 * Data transfer API implementation
 * ============================================================================ */

tcpip_error_t tcpip_socket_send(tcpip_socket_id_t socket_id,
                                 const uint8_t *data,
                                 uint16_t len,
                                 uint16_t *sent_len)
{
    tcpip_error_t result;

    result = tcpip_socket_validate_params(socket_id);
    
    if ((result == TCPIP_OK) && (data != NULL) && (sent_len != NULL))
    {
        if (g_sockets[socket_id].type == TCPIP_SOCK_STREAM)
        {
            result = tcpip_tcp_send(socket_id, data, len, sent_len);
        }
        else
        {
            result = TCPIP_ERROR_PARAM;  /* UDP needs destination */
        }
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

tcpip_error_t tcpip_socket_recv(tcpip_socket_id_t socket_id,
                                 uint8_t *buffer,
                                 uint16_t buffer_size,
                                 uint16_t *received_len)
{
    tcpip_error_t result;

    result = tcpip_socket_validate_params(socket_id);
    
    if ((result == TCPIP_OK) && (buffer != NULL) && (received_len != NULL))
    {
        if (g_sockets[socket_id].type == TCPIP_SOCK_STREAM)
        {
            result = tcpip_tcp_recv(socket_id, buffer, buffer_size, received_len);
        }
        else
        {
            result = tcpip_udp_recv(socket_id, NULL, buffer, buffer_size, received_len);
        }
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

tcpip_error_t tcpip_socket_sendto(tcpip_socket_id_t socket_id,
                                   const tcpip_sockaddr_t *dest_addr,
                                   const uint8_t *data,
                                   uint16_t len)
{
    tcpip_error_t result;

    result = tcpip_socket_validate_params(socket_id);
    
    if ((result == TCPIP_OK) && (dest_addr != NULL) && (data != NULL))
    {
        if (g_sockets[socket_id].type == TCPIP_SOCK_DGRAM)
        {
            result = tcpip_udp_send(socket_id, dest_addr->addr, dest_addr->port, data, len);
        }
        else
        {
            result = TCPIP_ERROR_PARAM;
        }
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

tcpip_error_t tcpip_socket_recvfrom(tcpip_socket_id_t socket_id,
                                     tcpip_sockaddr_t *src_addr,
                                     uint8_t *buffer,
                                     uint16_t buffer_size,
                                     uint16_t *received_len)
{
    tcpip_error_t result;

    result = tcpip_socket_validate_params(socket_id);
    
    if ((result == TCPIP_OK) && (buffer != NULL) && (received_len != NULL))
    {
        if (g_sockets[socket_id].type == TCPIP_SOCK_DGRAM)
        {
            result = tcpip_udp_recv(socket_id, src_addr, buffer, buffer_size, received_len);
        }
        else
        {
            result = TCPIP_ERROR_PARAM;
        }
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

tcpip_error_t tcpip_socket_get_rx_avail(tcpip_socket_id_t socket_id,
                                         uint16_t *available_len)
{
    tcpip_error_t result;

    result = tcpip_socket_validate_params(socket_id);
    
    if ((result == TCPIP_OK) && (available_len != NULL))
    {
        if (g_sockets[socket_id].type == TCPIP_SOCK_STREAM)
        {
            result = tcpip_tcp_get_rx_available(socket_id, available_len);
        }
        else
        {
            result = tcpip_udp_get_rx_available(socket_id, available_len);
        }
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

tcpip_error_t tcpip_socket_get_tx_space(tcpip_socket_id_t socket_id,
                                         uint16_t *tx_space)
{
    tcpip_error_t result;

    result = tcpip_socket_validate_params(socket_id);
    
    if ((result == TCPIP_OK) && (tx_space != NULL))
    {
        *tx_space = g_sockets[socket_id].tx_buf_size;
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

/* ============================================================================
 * Socket option configuration API implementation
 * ============================================================================ */

tcpip_error_t tcpip_socket_setopt(tcpip_socket_id_t socket_id,
                                   uint8_t option,
                                   const void *value,
                                   uint8_t value_len)
{
    tcpip_error_t result;
    const bool *bool_val;
    const uint16_t *u16_val;
    const uint32_t *u32_val;

    result = tcpip_socket_validate_params(socket_id);
    
    if ((result == TCPIP_OK) && (value != NULL))
    {
        switch (option)
        {
            case TCPIP_SOCK_OPT_BROADCAST:
                if ((value_len == sizeof(bool)) && 
                    (g_sockets[socket_id].type == TCPIP_SOCK_DGRAM))
                {
                    bool_val = (const bool *)value;
                    result = tcpip_udp_set_broadcast(socket_id, *bool_val);
                }
                break;
                
            case TCPIP_SOCK_OPT_KEEPALIVE:
                if (value_len == sizeof(bool))
                {
                    bool_val = (const bool *)value;
                    (void)tcpip_tcp_set_keepalive(socket_id, *bool_val, 7200u, 75u, 9u);
                    result = TCPIP_OK;
                }
                break;
                
            case TCPIP_SOCK_OPT_RCVTIMEO:
                if (value_len == sizeof(uint32_t))
                {
                    u32_val = (const uint32_t *)value;
                    g_sockets[socket_id].rx_timeout_ms = *u32_val;
                    result = TCPIP_OK;
                }
                break;
                
            case TCPIP_SOCK_OPT_SNDTIMEO:
                if (value_len == sizeof(uint32_t))
                {
                    u32_val = (const uint32_t *)value;
                    g_sockets[socket_id].tx_timeout_ms = *u32_val;
                    result = TCPIP_OK;
                }
                break;
                
            case TCPIP_SOCK_OPT_RCVBUF:
                if (value_len == sizeof(uint16_t))
                {
                    u16_val = (const uint16_t *)value;
                    g_sockets[socket_id].rx_buf_size = *u16_val;
                    result = TCPIP_OK;
                }
                break;
                
            case TCPIP_SOCK_OPT_SNDBUF:
                if (value_len == sizeof(uint16_t))
                {
                    u16_val = (const uint16_t *)value;
                    g_sockets[socket_id].tx_buf_size = *u16_val;
                    result = TCPIP_OK;
                }
                break;
                
            case TCPIP_SOCK_OPT_NODELAY:
                if ((value_len == sizeof(bool)) && 
                    (g_sockets[socket_id].type == TCPIP_SOCK_STREAM))
                {
                    bool_val = (const bool *)value;
                    result = tcpip_tcp_set_nagle(socket_id, !(*bool_val));
                }
                break;
                
            case TCPIP_SOCK_OPT_MCAST_TTL:
                if ((value_len == sizeof(uint8_t)) &&
                    (g_sockets[socket_id].type == TCPIP_SOCK_DGRAM))
                {
                    result = tcpip_udp_set_mcast_ttl(socket_id, *((const uint8_t *)value));
                }
                break;
                
            default:
                result = TCPIP_ERROR_PARAM;
                break;
        }
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

tcpip_error_t tcpip_socket_getopt(tcpip_socket_id_t socket_id,
                                   uint8_t option,
                                   void *value,
                                   uint8_t *value_len)
{
    tcpip_error_t result;

    result = tcpip_socket_validate_params(socket_id);
    
    if ((result == TCPIP_OK) && (value != NULL) && (value_len != NULL))
    {
        switch (option)
        {
            case TCPIP_SOCK_OPT_RCVBUF:
                if (*value_len >= sizeof(uint16_t))
                {
                    *((uint16_t *)value) = g_sockets[socket_id].rx_buf_size;
                    *value_len = sizeof(uint16_t);
                    result = TCPIP_OK;
                }
                break;
                
            case TCPIP_SOCK_OPT_SNDBUF:
                if (*value_len >= sizeof(uint16_t))
                {
                    *((uint16_t *)value) = g_sockets[socket_id].tx_buf_size;
                    *value_len = sizeof(uint16_t);
                    result = TCPIP_OK;
                }
                break;
                
            default:
                result = TCPIP_ERROR_PARAM;
                break;
        }
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

/* ============================================================================
 * Event callback API implementation
 * ============================================================================ */

tcpip_error_t tcpip_socket_register_callback(tcpip_socket_id_t socket_id,
                                              tcpip_socket_event_cb_t callback,
                                              void *user_data)
{
    tcpip_error_t result;

    result = tcpip_socket_validate_params(socket_id);
    
    if (result == TCPIP_OK)
    {
        g_sockets[socket_id].event_cb = callback;
        g_sockets[socket_id].event_cb_data = user_data;
    }

    return result;
}

tcpip_error_t tcpip_socket_unregister_callback(tcpip_socket_id_t socket_id)
{
    tcpip_error_t result;

    result = tcpip_socket_validate_params(socket_id);
    
    if (result == TCPIP_OK)
    {
        g_sockets[socket_id].event_cb = NULL;
        g_sockets[socket_id].event_cb_data = NULL;
    }

    return result;
}

tcpip_error_t tcpip_socket_set_nonblocking(tcpip_socket_id_t socket_id,
                                            bool nonblock)
{
    tcpip_error_t result;

    result = tcpip_socket_validate_params(socket_id);
    
    if (result == TCPIP_OK)
    {
        g_sockets[socket_id].nonblocking = nonblock;
    }

    return result;
}

void tcpip_socket_trigger_event(tcpip_socket_id_t socket_id, uint8_t event)
{
    if ((socket_id < TCPIP_MAX_SOCKETS) && g_sockets[socket_id].used)
    {
        if (g_sockets[socket_id].event_cb != NULL)
        {
            g_sockets[socket_id].event_cb(socket_id, event, 
                                           g_sockets[socket_id].event_cb_data);
        }
    }
}

/* ============================================================================
 * Socket info query API implementation
 * ============================================================================ */

tcpip_error_t tcpip_socket_get_state(tcpip_socket_id_t socket_id,
                                      tcpip_socket_state_t *state)
{
    tcpip_error_t result;

    result = tcpip_socket_validate_params(socket_id);
    
    if ((result == TCPIP_OK) && (state != NULL))
    {
        *state = g_sockets[socket_id].state;
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

tcpip_error_t tcpip_socket_get_type(tcpip_socket_id_t socket_id,
                                     tcpip_socket_type_t *type)
{
    tcpip_error_t result;

    result = tcpip_socket_validate_params(socket_id);
    
    if ((result == TCPIP_OK) && (type != NULL))
    {
        *type = g_sockets[socket_id].type;
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

tcpip_error_t tcpip_socket_get_local_addr(tcpip_socket_id_t socket_id,
                                           tcpip_sockaddr_t *local_addr)
{
    tcpip_error_t result;

    result = tcpip_socket_validate_params(socket_id);
    
    if ((result == TCPIP_OK) && (local_addr != NULL))
    {
        *local_addr = g_sockets[socket_id].local_addr;
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

tcpip_error_t tcpip_socket_get_remote_addr(tcpip_socket_id_t socket_id,
                                            tcpip_sockaddr_t *remote_addr)
{
    tcpip_error_t result;

    result = tcpip_socket_validate_params(socket_id);
    
    if ((result == TCPIP_OK) && (remote_addr != NULL))
    {
        *remote_addr = g_sockets[socket_id].remote_addr;
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

tcpip_error_t tcpip_socket_get_config(tcpip_socket_id_t socket_id,
                                       tcpip_socket_config_t *config)
{
    tcpip_error_t result;

    result = tcpip_socket_validate_params(socket_id);
    
    if ((result == TCPIP_OK) && (config != NULL))
    {
        config->type = g_sockets[socket_id].type;
        config->local_addr = g_sockets[socket_id].local_addr.addr;
        config->local_port = g_sockets[socket_id].local_addr.port;
        config->rx_buffer_size = g_sockets[socket_id].rx_buf_size;
        config->tx_buffer_size = g_sockets[socket_id].tx_buf_size;
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}

tcpip_error_t tcpip_socket_validate(tcpip_socket_id_t socket_id)
{
    return tcpip_socket_validate_params(socket_id);
}

uint8_t tcpip_socket_get_free_count(void)
{
    uint8_t count = 0u;
    uint8_t i;

    for (i = 0u; i < TCPIP_MAX_SOCKETS; i++)
    {
        if (!g_sockets[i].used)
        {
            count++;
        }
    }

    return count;
}

uint8_t tcpip_socket_get_active_count(void)
{
    uint8_t count = 0u;
    uint8_t i;

    for (i = 0u; i < TCPIP_MAX_SOCKETS; i++)
    {
        if (g_sockets[i].used)
        {
            count++;
        }
    }

    return count;
}

/* ============================================================================
 * FD set operations implementation
 * ============================================================================ */

void tcpip_fd_zero(tcpip_fd_set_t *set)
{
    if (set != NULL)
    {
        set->bitmap = 0u;
    }
}

void tcpip_fd_set(tcpip_fd_set_t *set, tcpip_socket_id_t socket_id)
{
    if ((set != NULL) && (socket_id < 32u))
    {
        set->bitmap |= (1u << (uint32_t)socket_id);
    }
}

void tcpip_fd_clr(tcpip_fd_set_t *set, tcpip_socket_id_t socket_id)
{
    if ((set != NULL) && (socket_id < 32u))
    {
        set->bitmap &= ~(1u << (uint32_t)socket_id);
    }
}

bool tcpip_fd_isset(tcpip_fd_set_t *set, tcpip_socket_id_t socket_id)
{
    bool result = false;

    if ((set != NULL) && (socket_id < 32u))
    {
        result = ((set->bitmap & (1u << (uint32_t)socket_id)) != 0u);
    }

    return result;
}

tcpip_error_t tcpip_select(uint8_t nfds,
                            tcpip_fd_set_t *readfds,
                            tcpip_fd_set_t *writefds,
                            tcpip_fd_set_t *exceptfds,
                            uint32_t timeout_ms,
                            uint8_t *ready_count)
{
    tcpip_error_t result = TCPIP_OK;
    tcpip_socket_id_t i;
    uint8_t count = 0u;
    uint16_t avail;
    bool connected;

    (void)writefds;
    (void)exceptfds;
    (void)timeout_ms;

    if (readfds != NULL)
    {
        for (i = 0u; i < nfds; i++)
        {
            if (tcpip_fd_isset(readfds, i))
            {
                result = tcpip_socket_get_rx_avail(i, &avail);
                if ((result == TCPIP_OK) && (avail > 0u))
                {
                    count++;
                }
                else
                {
                    /* Check if connection established */
                    result = tcpip_socket_is_connected(i, &connected);
                    if ((result == TCPIP_OK) && connected &&
                        (g_sockets[i].state == TCPIP_SOCK_STATE_CONNECTED))
                    {
                        count++;
                    }
                    else
                    {
                        tcpip_fd_clr(readfds, i);
                    }
                }
            }
        }
    }

    if (ready_count != NULL)
    {
        *ready_count = count;
    }

    return result;
}

/* ============================================================================
 * Internal helper functions implementation
 * ============================================================================ */

static int16_t tcpip_socket_find_free(void)
{
    int16_t idx = -1;
    uint8_t i;

    for (i = 0u; i < TCPIP_MAX_SOCKETS; i++)
    {
        if (!g_sockets[i].used)
        {
            idx = (int16_t)i;
            break;
        }
    }

    return idx;
}

static tcpip_error_t tcpip_socket_validate_params(tcpip_socket_id_t socket_id)
{
    tcpip_error_t result;

    if (!g_socket_initialized)
    {
        result = TCPIP_ERROR;
    }
    else if (socket_id >= TCPIP_MAX_SOCKETS)
    {
        result = TCPIP_ERROR_PARAM;
    }
    else if (!g_sockets[socket_id].used)
    {
        result = TCPIP_ERROR_NOT_FOUND;
    }
    else
    {
        result = TCPIP_OK;
    }

    return result;
}
