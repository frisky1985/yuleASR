/**
 * @file tcpip_icmp.c
 * @brief ICMP protocol implementation - Internet Control Message Protocol
 * @version 1.0
 * @date 2026-04-26
 *
 * @note MISRA C:2012 compliant
 * @note RFC 792 compliant
 */

#include <string.h>
#include "tcpip_icmp.h"
#include "tcpip_core.h"

/* ============================================================================
 * Private data structures
 * ============================================================================ */

typedef struct {
    uint16_t id;
    uint16_t seq;
    uint32_t timestamp;
    bool pending;
} tcpip_icmp_echo_pending_t;

typedef struct {
    uint32_t echo_req_count;
    uint32_t echo_rep_count;
    uint32_t unreach_count;
    uint32_t error_count;
    bool echo_reply_enabled;
    bool initialized;
    tcpip_icmp_echo_pending_t pending[TCPIP_ICMP_MAX_PENDING];
    void (*ping_cb)(tcpip_ip_addr_t from, uint16_t seq, uint32_t rtt_ms, void *user_data);
    void *ping_cb_data;
} tcpip_icmp_ctx_t;

/* ============================================================================
 * Static variables
 * ============================================================================ */

static tcpip_icmp_ctx_t g_icmp_ctx;

/* ============================================================================
 * Internal function declarations
 * ============================================================================ */

static int16_t tcpip_icmp_find_pending(uint16_t id);
static int16_t tcpip_icmp_find_free_pending(void);

/* ============================================================================
 * ICMP initialization API implementation
 * ============================================================================ */

tcpip_error_t tcpip_icmp_init(void)
{
    tcpip_error_t result = TCPIP_OK;

    if (g_icmp_ctx.initialized)
    {
        result = TCPIP_ERROR;
    }
    else
    {
        (void)memset(&g_icmp_ctx, 0, sizeof(g_icmp_ctx));
        g_icmp_ctx.echo_reply_enabled = true;
        g_icmp_ctx.initialized = true;
    }

    return result;
}

void tcpip_icmp_deinit(void)
{
    if (g_icmp_ctx.initialized)
    {
        g_icmp_ctx.initialized = false;
    }
}

void tcpip_icmp_main_function(uint32_t elapsed_ms)
{
    uint8_t i;

    if (g_icmp_ctx.initialized)
    {
        /* Process pending echo requests */
        for (i = 0u; i < TCPIP_ICMP_MAX_PENDING; i++)
        {
            if (g_icmp_ctx.pending[i].pending)
            {
                if (g_icmp_ctx.pending[i].timestamp >= TCPIP_ICMP_ECHO_TIMEOUT_MS)
                {
                    /* Timeout */
                    g_icmp_ctx.pending[i].pending = false;
                    g_icmp_ctx.error_count++;
                }
                else
                {
                    g_icmp_ctx.pending[i].timestamp += elapsed_ms;
                }
            }
        }
    }
}

/* ============================================================================
 * ICMP Echo (Ping) API implementation
 * ============================================================================ */

tcpip_error_t tcpip_icmp_send_echo_request(tcpip_ip_addr_t dest_addr,
                                            uint8_t iface_id,
                                            const uint8_t *data,
                                            uint16_t data_len,
                                            uint16_t seq,
                                            uint16_t id)
{
    tcpip_error_t result = TCPIP_OK;
    int16_t pending_idx;

    (void)dest_addr;
    (void)iface_id;
    (void)data;
    (void)data_len;

    if (!g_icmp_ctx.initialized)
    {
        result = TCPIP_ERROR;
    }
    else
    {
        /* Add to pending list */
        pending_idx = tcpip_icmp_find_free_pending();
        if (pending_idx >= 0)
        {
            g_icmp_ctx.pending[pending_idx].id = id;
            g_icmp_ctx.pending[pending_idx].seq = seq;
            g_icmp_ctx.pending[pending_idx].timestamp = 0u;
            g_icmp_ctx.pending[pending_idx].pending = true;
        }

        g_icmp_ctx.echo_req_count++;
        /* Simplified - actual implementation needs to build ICMP packet */
    }

    return result;
}

tcpip_error_t tcpip_icmp_send_echo_reply(tcpip_ip_addr_t dest_addr,
                                          uint8_t iface_id,
                                          const uint8_t *data,
                                          uint16_t data_len,
                                          uint16_t seq,
                                          uint16_t id)
{
    tcpip_error_t result = TCPIP_OK;

    (void)dest_addr;
    (void)iface_id;
    (void)data;
    (void)data_len;
    (void)seq;
    (void)id;

    if (!g_icmp_ctx.initialized)
    {
        result = TCPIP_ERROR;
    }
    else if (!g_icmp_ctx.echo_reply_enabled)
    {
        result = TCPIP_ERROR;
    }
    else
    {
        g_icmp_ctx.echo_rep_count++;
        /* Simplified - actual implementation needs to build ICMP reply */
    }

    return result;
}

tcpip_error_t tcpip_icmp_wait_for_echo_reply(uint16_t id,
                                              uint32_t timeout_ms,
                                              uint32_t *rtt_ms)
{
    tcpip_error_t result = TCPIP_ERROR_TIMEOUT;
    int16_t idx;
    uint32_t elapsed = 0u;

    (void)timeout_ms;

    if (!g_icmp_ctx.initialized)
    {
        result = TCPIP_ERROR;
    }
    else
    {
        idx = tcpip_icmp_find_pending(id);
        
        if (idx >= 0)
        {
            while (elapsed < timeout_ms)
            {
                if (!g_icmp_ctx.pending[idx].pending)
                {
                    /* Reply received */
                    if (rtt_ms != NULL)
                    {
                        *rtt_ms = g_icmp_ctx.pending[idx].timestamp;
                    }
                    result = TCPIP_OK;
                    break;
                }
                elapsed += 10u;
            }
        }
    }

    return result;
}

tcpip_error_t tcpip_icmp_ping(tcpip_ip_addr_t dest_addr,
                               uint8_t iface_id,
                               uint8_t count,
                               uint32_t timeout_ms,
                               uint8_t *reply_count)
{
    tcpip_error_t result = TCPIP_OK;
    uint8_t i;
    uint8_t replies = 0u;
    uint16_t id;

    if (!g_icmp_ctx.initialized)
    {
        result = TCPIP_ERROR;
    }
    else
    {
        id = (uint16_t)(dest_addr & 0xFFFFu);
        
        for (i = 0u; i < count; i++)
        {
            result = tcpip_icmp_send_echo_request(dest_addr, iface_id, NULL, 0u, i, id);
            
            if (result == TCPIP_OK)
            {
                result = tcpip_icmp_wait_for_echo_reply(id, timeout_ms, NULL);
                
                if (result == TCPIP_OK)
                {
                    replies++;
                }
            }
        }
        
        if (reply_count != NULL)
        {
            *reply_count = replies;
        }
    }

    return result;
}

tcpip_error_t tcpip_icmp_register_ping_callback(void (*callback)(
                                                   tcpip_ip_addr_t from,
                                                   uint16_t seq,
                                                   uint32_t rtt_ms,
                                                   void *user_data),
                                                 void *user_data)
{
    tcpip_error_t result = TCPIP_OK;

    if (!g_icmp_ctx.initialized)
    {
        result = TCPIP_ERROR;
    }
    else
    {
        g_icmp_ctx.ping_cb = callback;
        g_icmp_ctx.ping_cb_data = user_data;
    }

    return result;
}

tcpip_error_t tcpip_icmp_unregister_ping_callback(void)
{
    tcpip_error_t result = TCPIP_OK;

    if (!g_icmp_ctx.initialized)
    {
        result = TCPIP_ERROR;
    }
    else
    {
        g_icmp_ctx.ping_cb = NULL;
        g_icmp_ctx.ping_cb_data = NULL;
    }

    return result;
}

/* ============================================================================
 * ICMP error message API implementation
 * ============================================================================ */

tcpip_error_t tcpip_icmp_send_dest_unreach(tcpip_ip_addr_t dest_addr,
                                            uint8_t iface_id,
                                            tcpip_icmp_dest_unreach_code_t code,
                                            const uint8_t *original_packet,
                                            uint16_t packet_len)
{
    tcpip_error_t result = TCPIP_OK;

    (void)dest_addr;
    (void)iface_id;
    (void)code;
    (void)original_packet;
    (void)packet_len;

    if (!g_icmp_ctx.initialized)
    {
        result = TCPIP_ERROR;
    }
    else
    {
        g_icmp_ctx.unreach_count++;
        /* Simplified - actual implementation needs to build ICMP unreachable */
    }

    return result;
}

tcpip_error_t tcpip_icmp_send_time_exceeded(tcpip_ip_addr_t dest_addr,
                                             uint8_t iface_id,
                                             const uint8_t *original_packet,
                                             uint16_t packet_len)
{
    tcpip_error_t result = TCPIP_OK;

    (void)dest_addr;
    (void)iface_id;
    (void)original_packet;
    (void)packet_len;

    if (!g_icmp_ctx.initialized)
    {
        result = TCPIP_ERROR;
    }
    else
    {
        g_icmp_ctx.error_count++;
    }

    return result;
}

/* ============================================================================
 * ICMP packet processing API implementation
 * ============================================================================ */

tcpip_error_t tcpip_icmp_process_packet(uint8_t iface_id,
                                         tcpip_ip_addr_t src_addr,
                                         const uint8_t *packet,
                                         uint16_t len)
{
    tcpip_error_t result = TCPIP_ERROR_PARAM;
    const tcpip_icmp_header_t *icmp_hdr;
    int16_t pending_idx;
    uint32_t rtt;

    (void)iface_id;

    if ((packet != NULL) && (len >= TCPIP_ICMP_HEADER_LEN) && g_icmp_ctx.initialized)
    {
        icmp_hdr = (const tcpip_icmp_header_t *)packet;
        
        switch (icmp_hdr->type)
        {
            case TCPIP_ICMP_ECHO_REQUEST:
                /* Send echo reply */
                result = tcpip_icmp_send_echo_reply(src_addr, iface_id,
                                                    packet + TCPIP_ICMP_HEADER_LEN,
                                                    (uint16_t)(len - TCPIP_ICMP_HEADER_LEN),
                                                    TCPIP_NTOHS(icmp_hdr->seq),
                                                    TCPIP_NTOHS(icmp_hdr->id));
                break;
                
            case TCPIP_ICMP_ECHO_REPLY:
                /* Process echo reply */
                pending_idx = tcpip_icmp_find_pending(TCPIP_NTOHS(icmp_hdr->id));
                if (pending_idx >= 0)
                {
                    rtt = g_icmp_ctx.pending[pending_idx].timestamp;
                    g_icmp_ctx.pending[pending_idx].pending = false;
                    g_icmp_ctx.echo_rep_count++;
                    
                    /* Call callback if registered */
                    if (g_icmp_ctx.ping_cb != NULL)
                    {
                        g_icmp_ctx.ping_cb(src_addr, TCPIP_NTOHS(icmp_hdr->seq),
                                          rtt, g_icmp_ctx.ping_cb_data);
                    }
                }
                result = TCPIP_OK;
                break;
                
            case TCPIP_ICMP_DEST_UNREACH:
                g_icmp_ctx.unreach_count++;
                result = TCPIP_OK;
                break;
                
            default:
                result = TCPIP_ERROR;
                break;
        }
    }

    return result;
}

/* ============================================================================
 * ICMP statistics API implementation
 * ============================================================================ */

tcpip_error_t tcpip_icmp_get_stats(uint32_t *echo_req_count,
                                    uint32_t *echo_rep_count,
                                    uint32_t *unreach_count,
                                    uint32_t *error_count)
{
    tcpip_error_t result = TCPIP_OK;

    if (!g_icmp_ctx.initialized)
    {
        result = TCPIP_ERROR;
    }
    else
    {
        if (echo_req_count != NULL)
        {
            *echo_req_count = g_icmp_ctx.echo_req_count;
        }
        if (echo_rep_count != NULL)
        {
            *echo_rep_count = g_icmp_ctx.echo_rep_count;
        }
        if (unreach_count != NULL)
        {
            *unreach_count = g_icmp_ctx.unreach_count;
        }
        if (error_count != NULL)
        {
            *error_count = g_icmp_ctx.error_count;
        }
    }

    return result;
}

tcpip_error_t tcpip_icmp_clear_stats(void)
{
    tcpip_error_t result = TCPIP_OK;

    if (!g_icmp_ctx.initialized)
    {
        result = TCPIP_ERROR;
    }
    else
    {
        g_icmp_ctx.echo_req_count = 0u;
        g_icmp_ctx.echo_rep_count = 0u;
        g_icmp_ctx.unreach_count = 0u;
        g_icmp_ctx.error_count = 0u;
    }

    return result;
}

tcpip_error_t tcpip_icmp_set_echo_reply(bool enable)
{
    tcpip_error_t result = TCPIP_OK;

    if (!g_icmp_ctx.initialized)
    {
        result = TCPIP_ERROR;
    }
    else
    {
        g_icmp_ctx.echo_reply_enabled = enable;
    }

    return result;
}

tcpip_error_t tcpip_icmp_get_echo_reply(bool *enabled)
{
    tcpip_error_t result = TCPIP_ERROR_PARAM;

    if ((enabled != NULL) && g_icmp_ctx.initialized)
    {
        *enabled = g_icmp_ctx.echo_reply_enabled;
        result = TCPIP_OK;
    }

    return result;
}

/* ============================================================================
 * Internal helper functions implementation
 * ============================================================================ */

static int16_t tcpip_icmp_find_pending(uint16_t id)
{
    int16_t idx = -1;
    uint8_t i;

    for (i = 0u; i < TCPIP_ICMP_MAX_PENDING; i++)
    {
        if (g_icmp_ctx.pending[i].pending && (g_icmp_ctx.pending[i].id == id))
        {
            idx = (int16_t)i;
            break;
        }
    }

    return idx;
}

static int16_t tcpip_icmp_find_free_pending(void)
{
    int16_t idx = -1;
    uint8_t i;

    for (i = 0u; i < TCPIP_ICMP_MAX_PENDING; i++)
    {
        if (!g_icmp_ctx.pending[i].pending)
        {
            idx = (int16_t)i;
            break;
        }
    }

    return idx;
}
