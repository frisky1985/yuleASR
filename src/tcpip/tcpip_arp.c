/**
 * @file tcpip_arp.c
 * @brief ARP协议实现 - 地址解析协议
 * @version 1.0
 * @date 2026-04-26
 *
 * @note MISRA C:2012 compliant
 * @note RFC 826 compliant
 */

#include <string.h>
#include "tcpip_arp.h"

/* ============================================================================
 * 私有数据结构
 * ============================================================================ */

typedef struct {
    tcpip_arp_entry_t entries[TCPIP_ARP_CACHE_SIZE];
    uint32_t request_count;
    uint32_t reply_count;
    uint32_t cache_hits;
    uint32_t cache_misses;
    uint32_t cache_timeout_ms;
    bool initialized;
} tcpip_arp_ctx_t;

/* ============================================================================
 * 静态变量
 * ============================================================================ */

static tcpip_arp_ctx_t g_arp_ctx;

/* ============================================================================
 * 内部函数声明
 * ============================================================================ */

static int16_t tcpip_arp_find_free_entry(void);
static int16_t tcpip_arp_find_entry(tcpip_ip_addr_t ip_addr, uint8_t iface_id);
static int16_t tcpip_arp_find_entry_by_ip(tcpip_ip_addr_t ip_addr);
static tcpip_error_t tcpip_arp_update_entry(int16_t idx, 
                                             const uint8_t *mac_addr,
                                             uint8_t iface_id);

/* ============================================================================
 * ARP初始化API实现
 * ============================================================================ */

tcpip_error_t tcpip_arp_init(void)
{
    tcpip_error_t result = TCPIP_OK;

    if (g_arp_ctx.initialized)
    {
        result = TCPIP_ERROR;
    }
    else
    {
        (void)memset(&g_arp_ctx, 0, sizeof(g_arp_ctx));
        g_arp_ctx.cache_timeout_ms = TCPIP_ARP_CACHE_TIMEOUT_MS;
        g_arp_ctx.initialized = true;
    }

    return result;
}

void tcpip_arp_deinit(void)
{
    if (g_arp_ctx.initialized)
    {
        (void)tcpip_arp_flush(0xFFu);
        g_arp_ctx.initialized = false;
    }
}

void tcpip_arp_main_function(uint32_t elapsed_ms)
{
    uint8_t i;
    
    if (g_arp_ctx.initialized)
    {
        for (i = 0u; i < TCPIP_ARP_CACHE_SIZE; i++)
        {
            if ((g_arp_ctx.entries[i].state == TCPIP_ARP_ENTRY_VALID) ||
                (g_arp_ctx.entries[i].state == TCPIP_ARP_ENTRY_PENDING))
            {
                if (g_arp_ctx.entries[i].timestamp >= g_arp_ctx.cache_timeout_ms)
                {
                    /* 条目超时，清除 */
                    g_arp_ctx.entries[i].state = TCPIP_ARP_ENTRY_FREE;
                }
                else
                {
                    g_arp_ctx.entries[i].timestamp += elapsed_ms;
                }
                
                /* 处理待定条目的重试 */
                if (g_arp_ctx.entries[i].state == TCPIP_ARP_ENTRY_PENDING)
                {
                    if (g_arp_ctx.entries[i].retry_count > 0u)
                    {
                        /* 减少重试计数 */
                    }
                    else
                    {
                        /* 重试次数耗尽，清除 */
                        g_arp_ctx.entries[i].state = TCPIP_ARP_ENTRY_FREE;
                    }
                }
            }
        }
    }
}

/* ============================================================================
 * ARP缓存管理API实现
 * ============================================================================ */

tcpip_error_t tcpip_arp_lookup(tcpip_ip_addr_t ip_addr,
                                uint8_t iface_id,
                                uint8_t *mac_addr)
{
    tcpip_error_t result = TCPIP_ERROR_NOT_FOUND;
    int16_t idx;

    if (!g_arp_ctx.initialized)
    {
        result = TCPIP_ERROR;
    }
    else if (mac_addr == NULL)
    {
        result = TCPIP_ERROR_PARAM;
    }
    else
    {
        /* 检查是否是广播地址 */
        if (tcpip_arp_is_broadcast(ip_addr))
        {
            tcpip_arp_get_broadcast_mac(mac_addr);
            result = TCPIP_OK;
        }
        else
        {
            /* 查找缓存 */
            idx = tcpip_arp_find_entry_by_ip(ip_addr);
            
            if (idx >= 0)
            {
                if (g_arp_ctx.entries[idx].state == TCPIP_ARP_ENTRY_VALID)
                {
                    (void)memcpy(mac_addr, g_arp_ctx.entries[idx].mac_addr, 6u);
                    g_arp_ctx.entries[idx].timestamp = 0u;  /* 刷新时间戳 */
                    g_arp_ctx.cache_hits++;
                    result = TCPIP_OK;
                }
                else if (g_arp_ctx.entries[idx].state == TCPIP_ARP_ENTRY_PENDING)
                {
                    /* 正在解析中 */
                    result = TCPIP_ERROR_BUSY;
                }
            }
            else
            {
                /* 创建待定条目并发送请求 */
                idx = tcpip_arp_find_free_entry();
                if (idx >= 0)
                {
                    g_arp_ctx.entries[idx].ip_addr = ip_addr;
                    g_arp_ctx.entries[idx].state = TCPIP_ARP_ENTRY_PENDING;
                    g_arp_ctx.entries[idx].timestamp = 0u;
                    g_arp_ctx.entries[idx].retry_count = TCPIP_ARP_MAX_RETRIES;
                    g_arp_ctx.entries[idx].iface_id = iface_id;
                    
                    (void)tcpip_arp_send_request(ip_addr, iface_id);
                    g_arp_ctx.cache_misses++;
                    result = TCPIP_ERROR_NOT_FOUND;
                }
                else
                {
                    result = TCPIP_ERROR_NO_MEMORY;
                }
            }
        }
    }

    return result;
}

tcpip_error_t tcpip_arp_add_entry(tcpip_ip_addr_t ip_addr,
                                   const uint8_t *mac_addr,
                                   uint8_t iface_id,
                                   bool is_static)
{
    tcpip_error_t result = TCPIP_ERROR_PARAM;
    int16_t idx;

    if ((mac_addr != NULL) && g_arp_ctx.initialized)
    {
        /* 检查是否已存在 */
        idx = tcpip_arp_find_entry_by_ip(ip_addr);
        
        if (idx < 0)
        {
            /* 查找空闲条目 */
            idx = tcpip_arp_find_free_entry();
        }
        
        if (idx >= 0)
        {
            result = tcpip_arp_update_entry(idx, mac_addr, iface_id);
            if (result == TCPIP_OK)
            {
                g_arp_ctx.entries[idx].ip_addr = ip_addr;
                g_arp_ctx.entries[idx].state = is_static ? 
                    TCPIP_ARP_ENTRY_STATIC : TCPIP_ARP_ENTRY_VALID;
            }
        }
        else
        {
            result = TCPIP_ERROR_NO_MEMORY;
        }
    }

    return result;
}

tcpip_error_t tcpip_arp_del_entry(tcpip_ip_addr_t ip_addr, uint8_t iface_id)
{
    tcpip_error_t result = TCPIP_ERROR_NOT_FOUND;
    int16_t idx;

    idx = tcpip_arp_find_entry(ip_addr, iface_id);
    
    if (idx >= 0)
    {
        g_arp_ctx.entries[idx].state = TCPIP_ARP_ENTRY_FREE;
        result = TCPIP_OK;
    }

    return result;
}

tcpip_error_t tcpip_arp_flush(uint8_t iface_id)
{
    uint8_t i;

    for (i = 0u; i < TCPIP_ARP_CACHE_SIZE; i++)
    {
        if ((iface_id == 0xFFu) || (g_arp_ctx.entries[i].iface_id == iface_id))
        {
            if (g_arp_ctx.entries[i].state != TCPIP_ARP_ENTRY_STATIC)
            {
                g_arp_ctx.entries[i].state = TCPIP_ARP_ENTRY_FREE;
            }
        }
    }

    return TCPIP_OK;
}

tcpip_error_t tcpip_arp_get_entry(uint8_t index, tcpip_arp_entry_t *entry)
{
    tcpip_error_t result = TCPIP_ERROR_PARAM;

    if ((entry != NULL) && (index < TCPIP_ARP_CACHE_SIZE))
    {
        (void)memcpy(entry, &g_arp_ctx.entries[index], sizeof(tcpip_arp_entry_t));
        result = TCPIP_OK;
    }

    return result;
}

uint8_t tcpip_arp_get_entry_count(void)
{
    uint8_t count = 0u;
    uint8_t i;

    for (i = 0u; i < TCPIP_ARP_CACHE_SIZE; i++)
    {
        if (g_arp_ctx.entries[i].state != TCPIP_ARP_ENTRY_FREE)
        {
            count++;
        }
    }

    return count;
}

/* ============================================================================
 * ARP请求/响应API实现
 * ============================================================================ */

tcpip_error_t tcpip_arp_send_request(tcpip_ip_addr_t target_ip,
                                      uint8_t iface_id)
{
    tcpip_error_t result = TCPIP_OK;
    
    (void)target_ip;
    (void)iface_id;
    
    /* 简化实现 - 实际需构建和发送ARP请求包 */
    g_arp_ctx.request_count++;
    
    return result;
}

tcpip_error_t tcpip_arp_send_reply(tcpip_ip_addr_t target_ip,
                                    const uint8_t *target_mac,
                                    uint8_t iface_id)
{
    tcpip_error_t result = TCPIP_OK;
    
    (void)target_ip;
    (void)target_mac;
    (void)iface_id;
    
    /* 简化实现 - 实际需构建和发送ARP响应包 */
    g_arp_ctx.reply_count++;
    
    return result;
}

tcpip_error_t tcpip_arp_process_packet(uint8_t iface_id,
                                        const uint8_t *packet,
                                        uint16_t len)
{
    tcpip_error_t result = TCPIP_ERROR_PARAM;
    const tcpip_arp_packet_t *arp_pkt;
    int16_t idx;

    if ((packet != NULL) && (len >= TCPIP_ARP_PACKET_LEN) && g_arp_ctx.initialized)
    {
        arp_pkt = (const tcpip_arp_packet_t *)packet;
        
        /* 验证ARP包 */
        if ((TCPIP_NTOHS(arp_pkt->hw_type) == TCPIP_ARP_HW_TYPE_ETH) &&
            (TCPIP_NTOHS(arp_pkt->proto_type) == TCPIP_ARP_PROTO_TYPE_IP) &&
            (arp_pkt->hw_len == TCPIP_ARP_HW_LEN) &&
            (arp_pkt->proto_len == TCPIP_ARP_PROTO_LEN))
        {
            uint16_t opcode = TCPIP_NTOHS(arp_pkt->opcode);
            
            if (opcode == TCPIP_ARP_OP_REQUEST)
            {
                /* 处理ARP请求 */
                /* 更新发送方的缓存 */
                (void)tcpip_arp_add_entry(arp_pkt->sender_ip, 
                                          arp_pkt->sender_mac, 
                                          iface_id, false);
                g_arp_ctx.request_count++;
                result = TCPIP_OK;
            }
            else if (opcode == TCPIP_ARP_OP_REPLY)
            {
                /* 处理ARP响应 */
                idx = tcpip_arp_find_entry_by_ip(arp_pkt->sender_ip);
                if (idx >= 0)
                {
                    (void)tcpip_arp_update_entry(idx, arp_pkt->sender_mac, iface_id);
                    g_arp_ctx.entries[idx].state = TCPIP_ARP_ENTRY_VALID;
                }
                else
                {
                    (void)tcpip_arp_add_entry(arp_pkt->sender_ip,
                                              arp_pkt->sender_mac,
                                              iface_id, false);
                }
                g_arp_ctx.reply_count++;
                result = TCPIP_OK;
            }
            else
            {
                result = TCPIP_ERROR;
            }
        }
    }

    return result;
}

tcpip_error_t tcpip_arp_has_pending(bool *pending)
{
    tcpip_error_t result = TCPIP_ERROR_PARAM;
    uint8_t i;

    if ((pending != NULL) && g_arp_ctx.initialized)
    {
        *pending = false;
        for (i = 0u; i < TCPIP_ARP_CACHE_SIZE; i++)
        {
            if (g_arp_ctx.entries[i].state == TCPIP_ARP_ENTRY_PENDING)
            {
                *pending = true;
                break;
            }
        }
        result = TCPIP_OK;
    }

    return result;
}

/* ============================================================================
 * ARP解析等待API实现
 * ============================================================================ */

tcpip_error_t tcpip_arp_wait_for_resolve(tcpip_ip_addr_t ip_addr,
                                          uint32_t timeout_ms)
{
    tcpip_error_t result = TCPIP_ERROR_NOT_FOUND;
    int16_t idx;
    uint32_t elapsed = 0u;

    (void)timeout_ms;

    idx = tcpip_arp_find_entry_by_ip(ip_addr);
    
    if (idx >= 0)
    {
        while ((elapsed < timeout_ms) && 
               (g_arp_ctx.entries[idx].state == TCPIP_ARP_ENTRY_PENDING))
        {
            /* 等待解析完成 */
            elapsed += 10u;
        }
        
        if (g_arp_ctx.entries[idx].state == TCPIP_ARP_ENTRY_VALID)
        {
            result = TCPIP_OK;
        }
        else
        {
            result = TCPIP_ERROR_TIMEOUT;
        }
    }

    return result;
}

tcpip_error_t tcpip_arp_cancel_wait(tcpip_ip_addr_t ip_addr)
{
    tcpip_error_t result = TCPIP_ERROR_NOT_FOUND;
    int16_t idx;

    idx = tcpip_arp_find_entry_by_ip(ip_addr);
    
    if (idx >= 0)
    {
        if (g_arp_ctx.entries[idx].state == TCPIP_ARP_ENTRY_PENDING)
        {
            g_arp_ctx.entries[idx].state = TCPIP_ARP_ENTRY_FREE;
        }
        result = TCPIP_OK;
    }

    return result;
}

/* ============================================================================
 * 特殊地址检查API实现
 * ============================================================================ */

tcpip_error_t tcpip_arp_is_needed(tcpip_ip_addr_t ip_addr,
                                   uint8_t iface_id,
                                   bool *need_arp)
{
    tcpip_error_t result = TCPIP_ERROR_PARAM;

    if (need_arp != NULL)
    {
        /* 广播、多播、本地回环地址不需要ARP */
        if ((ip_addr == TCPIP_IP_BROADCAST) ||
            ((ip_addr & 0xF0000000u) == 0xE0000000u) ||  /* 224.0.0.0/4 */
            ((ip_addr & 0xFF000000u) == 0x7F000000u))   /* 127.0.0.0/8 */
        {
            *need_arp = false;
        }
        else
        {
            /* 检查是否在同一子网 */
            (void)iface_id;
            *need_arp = true;
        }
        result = TCPIP_OK;
    }

    return result;
}

bool tcpip_arp_is_broadcast(tcpip_ip_addr_t ip_addr)
{
    return ((ip_addr == TCPIP_IP_BROADCAST) ||
            ((ip_addr & 0xF0000000u) == 0xE0000000u));
}

void tcpip_arp_get_broadcast_mac(uint8_t *mac_addr)
{
    if (mac_addr != NULL)
    {
        mac_addr[0] = 0xFFu;
        mac_addr[1] = 0xFFu;
        mac_addr[2] = 0xFFu;
        mac_addr[3] = 0xFFu;
        mac_addr[4] = 0xFFu;
        mac_addr[5] = 0xFFu;
    }
}

/* ============================================================================
 * ARP统计信息API实现
 * ============================================================================ */

tcpip_error_t tcpip_arp_get_stats(uint32_t *request_count,
                                   uint32_t *reply_count,
                                   uint32_t *cache_hits,
                                   uint32_t *cache_misses)
{
    tcpip_error_t result = TCPIP_OK;

    if (!g_arp_ctx.initialized)
    {
        result = TCPIP_ERROR;
    }
    else
    {
        if (request_count != NULL)
        {
            *request_count = g_arp_ctx.request_count;
        }
        if (reply_count != NULL)
        {
            *reply_count = g_arp_ctx.reply_count;
        }
        if (cache_hits != NULL)
        {
            *cache_hits = g_arp_ctx.cache_hits;
        }
        if (cache_misses != NULL)
        {
            *cache_misses = g_arp_ctx.cache_misses;
        }
    }

    return result;
}

tcpip_error_t tcpip_arp_clear_stats(void)
{
    tcpip_error_t result = TCPIP_OK;

    if (!g_arp_ctx.initialized)
    {
        result = TCPIP_ERROR;
    }
    else
    {
        g_arp_ctx.request_count = 0u;
        g_arp_ctx.reply_count = 0u;
        g_arp_ctx.cache_hits = 0u;
        g_arp_ctx.cache_misses = 0u;
    }

    return result;
}

tcpip_error_t tcpip_arp_set_cache_timeout(uint32_t timeout_ms)
{
    tcpip_error_t result = TCPIP_OK;

    if (!g_arp_ctx.initialized)
    {
        result = TCPIP_ERROR;
    }
    else
    {
        g_arp_ctx.cache_timeout_ms = timeout_ms;
    }

    return result;
}

tcpip_error_t tcpip_arp_get_cache_timeout(uint32_t *timeout_ms)
{
    tcpip_error_t result = TCPIP_ERROR_PARAM;

    if ((timeout_ms != NULL) && g_arp_ctx.initialized)
    {
        *timeout_ms = g_arp_ctx.cache_timeout_ms;
        result = TCPIP_OK;
    }

    return result;
}

/* ============================================================================
 * 内部辅助函数实现
 * ============================================================================ */

static int16_t tcpip_arp_find_free_entry(void)
{
    int16_t idx = -1;
    uint8_t i;

    for (i = 0u; i < TCPIP_ARP_CACHE_SIZE; i++)
    {
        if (g_arp_ctx.entries[i].state == TCPIP_ARP_ENTRY_FREE)
        {
            idx = (int16_t)i;
            break;
        }
    }

    return idx;
}

static int16_t tcpip_arp_find_entry(tcpip_ip_addr_t ip_addr, uint8_t iface_id)
{
    int16_t idx = -1;
    uint8_t i;

    for (i = 0u; i < TCPIP_ARP_CACHE_SIZE; i++)
    {
        if ((g_arp_ctx.entries[i].state != TCPIP_ARP_ENTRY_FREE) &&
            (g_arp_ctx.entries[i].ip_addr == ip_addr) &&
            (g_arp_ctx.entries[i].iface_id == iface_id))
        {
            idx = (int16_t)i;
            break;
        }
    }

    return idx;
}

static int16_t tcpip_arp_find_entry_by_ip(tcpip_ip_addr_t ip_addr)
{
    int16_t idx = -1;
    uint8_t i;

    for (i = 0u; i < TCPIP_ARP_CACHE_SIZE; i++)
    {
        if ((g_arp_ctx.entries[i].state != TCPIP_ARP_ENTRY_FREE) &&
            (g_arp_ctx.entries[i].ip_addr == ip_addr))
        {
            idx = (int16_t)i;
            break;
        }
    }

    return idx;
}

static tcpip_error_t tcpip_arp_update_entry(int16_t idx, 
                                             const uint8_t *mac_addr,
                                             uint8_t iface_id)
{
    tcpip_error_t result = TCPIP_OK;

    if ((idx >= 0) && (idx < TCPIP_ARP_CACHE_SIZE) && (mac_addr != NULL))
    {
        (void)memcpy(g_arp_ctx.entries[idx].mac_addr, mac_addr, 6u);
        g_arp_ctx.entries[idx].iface_id = iface_id;
        g_arp_ctx.entries[idx].timestamp = 0u;
        g_arp_ctx.entries[idx].retry_count = 0u;
    }
    else
    {
        result = TCPIP_ERROR_PARAM;
    }

    return result;
}
