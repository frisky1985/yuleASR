/**
 * @file eth_dma.c
 * @brief 车载以太网DMA管理实现 - 环形描述符队列
 * @version 1.0
 * @date 2026-04-24
 *
 * @note 收发分离的环形缓冲区设计
 * @note 支持中断和轮询模式
 * @note 符合AUTOSAR MCAL规范
 */

#include "eth_dma.h"
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * 内部状态结构
 * ============================================================================ */

/** DMA环形缓冲区内部结构 */
typedef struct {
    eth_dma_config_t config;            /* DMA配置 */
    eth_dma_state_t state;              /* 当前状态 */
    eth_dma_error_t error;              /* 当前错误 */
    eth_dma_stats_t stats;              /* 统计信息 */

    /* 接收环形缓冲区 */
    eth_dma_rx_desc_t *rx_desc_ring;    /* 接收描述符环 */
    uint8_t **rx_buffers;               /* 接收缓冲区数组 */
    uint16_t rx_desc_head;              /* 接收描述符头指针 */
    uint16_t rx_desc_tail;              /* 接收描述符尾指针 */

    /* 发送环形缓冲区 */
    eth_dma_tx_desc_t *tx_desc_ring;    /* 发送描述符环 */
    uint8_t **tx_buffers;               /* 发送缓冲区数组 */
    uint16_t tx_desc_head;              /* 发送描述符头指针 */
    uint16_t tx_desc_tail;              /* 发送描述符尾指针 */

    /* 回调函数 */
    eth_dma_rx_complete_t rx_callback;
    eth_dma_tx_complete_t tx_callback;
    eth_dma_error_t_callback error_callback;
    void *rx_user_data;
    void *tx_user_data;
    void *error_user_data;

    /* 同步原语 */
    volatile uint32_t rx_in_progress;
    volatile uint32_t tx_in_progress;
} eth_dma_context_t;

/* 全局DMA上下文 */
static eth_dma_context_t g_dma_ctx = {0};

/* 对齐宏 */
#define ETH_ALIGN_UP(x, a)      (((x) + ((a) - 1)) & ~((a) - 1))
#define ETH_ALIGN_DOWN(x, a)    ((x) & ~((a) - 1))

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

/**
 * @brief 验证DMA配置参数
 */
static eth_status_t eth_dma_validate_config(const eth_dma_config_t *config)
{
    if (config == NULL) {
        return ETH_INVALID_PARAM;
    }

    /* 验证描述符数量 */
    if (config->rx_desc_count == 0 || config->rx_desc_count > ETH_DMA_MAX_RX_DESC_COUNT) {
        return ETH_INVALID_PARAM;
    }
    if (config->tx_desc_count == 0 || config->tx_desc_count > ETH_DMA_MAX_TX_DESC_COUNT) {
        return ETH_INVALID_PARAM;
    }

    /* 验证缓冲区大小 */
    if (config->buffer_size < 64) {
        return ETH_INVALID_PARAM;
    }
    if (config->enable_jumbo && config->buffer_size < ETH_DMA_JUMBO_BUFFER_SIZE) {
        return ETH_INVALID_PARAM;
    }

    return ETH_OK;
}

/**
 * @brief 计算下一个描述符索引(环形)
 */
static inline uint16_t eth_dma_next_rx_desc(uint16_t current, uint16_t count)
{
    return (current + 1) % count;
}

static inline uint16_t eth_dma_next_tx_desc(uint16_t current, uint16_t count)
{
    return (current + 1) % count;
}

/**
 * @brief 获取接收描述符所有权状态
 */
static inline bool eth_dma_rx_desc_owned_by_dma(const eth_dma_rx_desc_t *desc)
{
    return (desc->rd0 & ETH_RX_DESC_OWN) != 0;
}

/**
 * @brief 获取发送描述符所有权状态
 */
static inline bool eth_dma_tx_desc_owned_by_dma(const eth_dma_tx_desc_t *desc)
{
    return (desc->td0 & ETH_TX_DESC_OWN) != 0;
}

/**
 * @brief 初始化接收描述符环
 */
static eth_status_t eth_dma_init_rx_ring(void)
{
    uint16_t count = g_dma_ctx.config.rx_desc_count;
    uint32_t buffer_size = g_dma_ctx.config.buffer_size;

    /* 分配描述符环 */
    g_dma_ctx.rx_desc_ring = (eth_dma_rx_desc_t *)malloc(
        sizeof(eth_dma_rx_desc_t) * count);
    if (g_dma_ctx.rx_desc_ring == NULL) {
        return ETH_NO_MEMORY;
    }
    memset(g_dma_ctx.rx_desc_ring, 0, sizeof(eth_dma_rx_desc_t) * count);

    /* 分配接收缓冲区数组 */
    g_dma_ctx.rx_buffers = (uint8_t **)malloc(sizeof(uint8_t *) * count);
    if (g_dma_ctx.rx_buffers == NULL) {
        free(g_dma_ctx.rx_desc_ring);
        return ETH_NO_MEMORY;
    }

    /* 分配和初始化描述符 */
    for (uint16_t i = 0; i < count; i++) {
        /* 分配缓冲区 */
        g_dma_ctx.rx_buffers[i] = (uint8_t *)eth_dma_alloc_buffer(buffer_size);
        if (g_dma_ctx.rx_buffers[i] == NULL) {
            /* 清理已分配的缓冲区 */
            for (uint16_t j = 0; j < i; j++) {
                eth_dma_free_buffer(g_dma_ctx.rx_buffers[j]);
            }
            free(g_dma_ctx.rx_buffers);
            free(g_dma_ctx.rx_desc_ring);
            return ETH_NO_MEMORY;
        }

        /* 配置描述符 */
        eth_dma_rx_desc_t *desc = &g_dma_ctx.rx_desc_ring[i];
        desc->rd0 = 0;
        desc->rd1 = (buffer_size << ETH_RX_DESC_FL_SHIFT);
        desc->rd2 = (uint32_t)(uintptr_t)g_dma_ctx.rx_buffers[i];
        desc->rd3 = 0;

        /* 将描述符所有权交给DMA */
        desc->rd0 |= ETH_RX_DESC_OWN;
    }

    /* 设置环形链接(最后一个描述符指向第一个) */
    /* 实际硬件可能需要特殊配置 */

    g_dma_ctx.rx_desc_head = 0;
    g_dma_ctx.rx_desc_tail = 0;

    return ETH_OK;
}

/**
 * @brief 初始化发送描述符环
 */
static eth_status_t eth_dma_init_tx_ring(void)
{
    uint16_t count = g_dma_ctx.config.tx_desc_count;
    uint32_t buffer_size = g_dma_ctx.config.buffer_size;

    /* 分配描述符环 */
    g_dma_ctx.tx_desc_ring = (eth_dma_tx_desc_t *)malloc(
        sizeof(eth_dma_tx_desc_t) * count);
    if (g_dma_ctx.tx_desc_ring == NULL) {
        return ETH_NO_MEMORY;
    }
    memset(g_dma_ctx.tx_desc_ring, 0, sizeof(eth_dma_tx_desc_t) * count);

    /* 分配发送缓冲区数组 */
    g_dma_ctx.tx_buffers = (uint8_t **)malloc(sizeof(uint8_t *) * count);
    if (g_dma_ctx.tx_buffers == NULL) {
        free(g_dma_ctx.tx_desc_ring);
        return ETH_NO_MEMORY;
    }

    /* 分配和初始化描述符 */
    for (uint16_t i = 0; i < count; i++) {
        /* 分配缓冲区 */
        g_dma_ctx.tx_buffers[i] = (uint8_t *)eth_dma_alloc_buffer(buffer_size);
        if (g_dma_ctx.tx_buffers[i] == NULL) {
            /* 清理已分配的缓冲区 */
            for (uint16_t j = 0; j < i; j++) {
                eth_dma_free_buffer(g_dma_ctx.tx_buffers[j]);
            }
            free(g_dma_ctx.tx_buffers);
            free(g_dma_ctx.tx_desc_ring);
            return ETH_NO_MEMORY;
        }

        /* 配置描述符(初始状态由CPU拥有) */
        eth_dma_tx_desc_t *desc = &g_dma_ctx.tx_desc_ring[i];
        desc->td0 = ETH_TX_DESC_TCH;  /* 启用二级链接 */
        desc->td1 = 0;
        desc->td2 = (uint32_t)(uintptr_t)g_dma_ctx.tx_buffers[i];
        desc->td3 = 0;
    }

    g_dma_ctx.tx_desc_head = 0;
    g_dma_ctx.tx_desc_tail = 0;

    return ETH_OK;
}

/**
 * @brief 释放接收环形缓冲区
 */
static void eth_dma_deinit_rx_ring(void)
{
    if (g_dma_ctx.rx_desc_ring == NULL) {
        return;
    }

    uint16_t count = g_dma_ctx.config.rx_desc_count;

    /* 释放缓冲区 */
    if (g_dma_ctx.rx_buffers != NULL) {
        for (uint16_t i = 0; i < count; i++) {
            if (g_dma_ctx.rx_buffers[i] != NULL) {
                eth_dma_free_buffer(g_dma_ctx.rx_buffers[i]);
            }
        }
        free(g_dma_ctx.rx_buffers);
        g_dma_ctx.rx_buffers = NULL;
    }

    /* 释放描述符环 */
    free(g_dma_ctx.rx_desc_ring);
    g_dma_ctx.rx_desc_ring = NULL;
}

/**
 * @brief 释放发送环形缓冲区
 */
static void eth_dma_deinit_tx_ring(void)
{
    if (g_dma_ctx.tx_desc_ring == NULL) {
        return;
    }

    uint16_t count = g_dma_ctx.config.tx_desc_count;

    /* 释放缓冲区 */
    if (g_dma_ctx.tx_buffers != NULL) {
        for (uint16_t i = 0; i < count; i++) {
            if (g_dma_ctx.tx_buffers[i] != NULL) {
                eth_dma_free_buffer(g_dma_ctx.tx_buffers[i]);
            }
        }
        free(g_dma_ctx.tx_buffers);
        g_dma_ctx.tx_buffers = NULL;
    }

    /* 释放描述符环 */
    free(g_dma_ctx.tx_desc_ring);
    g_dma_ctx.tx_desc_ring = NULL;
}

/* ============================================================================
 * 公共API实现
 * ============================================================================ */

eth_status_t eth_dma_init(const eth_dma_config_t *config)
{
    eth_status_t status;

    /* 验证参数 */
    status = eth_dma_validate_config(config);
    if (status != ETH_OK) {
        return status;
    }

    /* 检查是否已初始化 */
    if (g_dma_ctx.state != ETH_DMA_STATE_UNINIT) {
        return ETH_ERROR;
    }

    /* 复制配置 */
    memcpy(&g_dma_ctx.config, config, sizeof(eth_dma_config_t));

    /* 初始化统计 */
    memset(&g_dma_ctx.stats, 0, sizeof(eth_dma_stats_t));

    /* 初始化接收环形缓冲区 */
    status = eth_dma_init_rx_ring();
    if (status != ETH_OK) {
        return status;
    }

    /* 初始化发送环形缓冲区 */
    status = eth_dma_init_tx_ring();
    if (status != ETH_OK) {
        eth_dma_deinit_rx_ring();
        return status;
    }

    /* 初始化回调 */
    g_dma_ctx.rx_callback = NULL;
    g_dma_ctx.tx_callback = NULL;
    g_dma_ctx.error_callback = NULL;
    g_dma_ctx.rx_in_progress = 0;
    g_dma_ctx.tx_in_progress = 0;

    /* 设置状态 */
    g_dma_ctx.state = ETH_DMA_STATE_INIT;
    g_dma_ctx.error = ETH_DMA_ERR_NONE;

    return ETH_OK;
}

void eth_dma_deinit(void)
{
    if (g_dma_ctx.state == ETH_DMA_STATE_UNINIT) {
        return;
    }

    /* 停止DMA */
    eth_dma_stop();

    /* 释放环形缓冲区 */
    eth_dma_deinit_tx_ring();
    eth_dma_deinit_rx_ring();

    /* 清除状态 */
    memset(&g_dma_ctx, 0, sizeof(eth_dma_context_t));
    g_dma_ctx.state = ETH_DMA_STATE_UNINIT;
}

eth_status_t eth_dma_start(void)
{
    if (g_dma_ctx.state != ETH_DMA_STATE_INIT &&
        g_dma_ctx.state != ETH_DMA_STATE_STOPPED) {
        return ETH_ERROR;
    }

    /* 启动DMA传输(实际实现需要操作硬件寄存器) */

    g_dma_ctx.state = ETH_DMA_STATE_RUNNING;
    return ETH_OK;
}

eth_status_t eth_dma_stop(void)
{
    if (g_dma_ctx.state != ETH_DMA_STATE_RUNNING) {
        return ETH_ERROR;
    }

    /* 停止DMA传输(实际实现) */

    g_dma_ctx.state = ETH_DMA_STATE_STOPPED;
    return ETH_OK;
}

eth_status_t eth_dma_reset(void)
{
    /* 停止DMA */
    eth_status_t status = eth_dma_stop();
    if (status != ETH_OK && status != ETH_ERROR) {
        return status;
    }

    /* 重置描述符环(实际实现) */

    /* 重置指针 */
    g_dma_ctx.rx_desc_head = 0;
    g_dma_ctx.rx_desc_tail = 0;
    g_dma_ctx.tx_desc_head = 0;
    g_dma_ctx.tx_desc_tail = 0;

    /* 清除错误状态 */
    g_dma_ctx.error = ETH_DMA_ERR_NONE;

    g_dma_ctx.state = ETH_DMA_STATE_INIT;
    return ETH_OK;
}

/* ============================================================================
 * 接收API实现
 * ============================================================================ */

eth_status_t eth_dma_rx_get_packet(eth_dma_rx_packet_t *packet, uint32_t timeout_ms)
{
    (void)timeout_ms;  /* 轮询模式暂时不使用超时 */

    if (g_dma_ctx.state != ETH_DMA_STATE_RUNNING) {
        return ETH_ERROR;
    }

    if (packet == NULL) {
        return ETH_INVALID_PARAM;
    }

    uint16_t head = g_dma_ctx.rx_desc_head;
    eth_dma_rx_desc_t *desc = &g_dma_ctx.rx_desc_ring[head];

    /* 检查描述符是否由CPU拥有(数据已到达) */
    if (eth_dma_rx_desc_owned_by_dma(desc)) {
        return ETH_TIMEOUT;  /* 还没有数据 */
    }

    /* 读取描述符状态 */
    uint32_t status = desc->rd0;

    /* 检查错误 */
    if (status & ETH_RX_DESC_ES) {
        g_dma_ctx.stats.rx_errors++;
        /* 重新设置描述符给DMA */
        desc->rd0 = ETH_RX_DESC_OWN;
        g_dma_ctx.rx_desc_head = eth_dma_next_rx_desc(head, g_dma_ctx.config.rx_desc_count);
        return ETH_ERROR;
    }

    /* 计算帧长度 */
    uint32_t length = (status & ETH_RX_DESC_FL_MASK) >> ETH_RX_DESC_FL_SHIFT;
    if (length > g_dma_ctx.config.buffer_size) {
        length = g_dma_ctx.config.buffer_size;
    }

    /* 填充输出结构 */
    packet->buffer = g_dma_ctx.rx_buffers[head];
    packet->length = length;
    packet->timestamp = 0;  /* 实际实现需要读取时间戳 */
    packet->status = status;
    packet->valid = true;

    /* 更新统计 */
    g_dma_ctx.stats.rx_packets++;
    g_dma_ctx.stats.rx_bytes += length;

    return ETH_OK;
}

eth_status_t eth_dma_rx_release_desc(uint16_t desc_index)
{
    if (g_dma_ctx.state != ETH_DMA_STATE_RUNNING) {
        return ETH_ERROR;
    }

    if (desc_index >= g_dma_ctx.config.rx_desc_count) {
        return ETH_INVALID_PARAM;
    }

    eth_dma_rx_desc_t *desc = &g_dma_ctx.rx_desc_ring[desc_index];

    /* 将描述符所有权交给DMA */
    desc->rd0 = ETH_RX_DESC_OWN;

    /* 更新头指针 */
    g_dma_ctx.rx_desc_head = eth_dma_next_rx_desc(desc_index, g_dma_ctx.config.rx_desc_count);

    return ETH_OK;
}

eth_status_t eth_dma_register_rx_callback(eth_dma_rx_complete_t callback, void *user_data)
{
    if (g_dma_ctx.state == ETH_DMA_STATE_UNINIT) {
        return ETH_ERROR;
    }

    g_dma_ctx.rx_callback = callback;
    g_dma_ctx.rx_user_data = user_data;

    return ETH_OK;
}

eth_status_t eth_dma_handle_rx_interrupt(void)
{
    if (g_dma_ctx.state != ETH_DMA_STATE_RUNNING) {
        return ETH_ERROR;
    }

    uint16_t count = 0;

    /* 处理所有已完成的接收描述符 */
    while (count < g_dma_ctx.config.rx_desc_count) {
        eth_dma_rx_packet_t packet;
        eth_status_t status = eth_dma_rx_get_packet(&packet, 0);

        if (status == ETH_TIMEOUT) {
            break;  /* 没有更多数据 */
        }

        if (status == ETH_OK && packet.valid) {
            /* 调用用户回调 */
            if (g_dma_ctx.rx_callback != NULL) {
                g_dma_ctx.rx_callback(&packet, g_dma_ctx.rx_user_data);
            }
            count++;
        }
    }

    return ETH_OK;
}

eth_status_t eth_dma_rx_check_available(uint16_t *available)
{
    if (g_dma_ctx.state != ETH_DMA_STATE_RUNNING) {
        return ETH_ERROR;
    }

    if (available == NULL) {
        return ETH_INVALID_PARAM;
    }

    uint16_t count = 0;
    uint16_t head = g_dma_ctx.rx_desc_head;
    uint16_t desc_count = g_dma_ctx.config.rx_desc_count;

    /* 计算可用的描述符数量 */
    for (uint16_t i = 0; i < desc_count; i++) {
        uint16_t idx = (head + i) % desc_count;
        if (!eth_dma_rx_desc_owned_by_dma(&g_dma_ctx.rx_desc_ring[idx])) {
            count++;
        } else {
            break;  /* 遇到DMA拥有的描述符，停止计数 */
        }
    }

    *available = count;
    return ETH_OK;
}

/* ============================================================================
 * 发送API实现
 * ============================================================================ */

eth_status_t eth_dma_tx_queue_packet(const eth_dma_tx_packet_t *packet, uint32_t timeout_ms)
{
    (void)timeout_ms;

    if (g_dma_ctx.state != ETH_DMA_STATE_RUNNING) {
        return ETH_ERROR;
    }

    if (packet == NULL || packet->buffer == NULL || packet->length == 0) {
        return ETH_INVALID_PARAM;
    }

    if (packet->length > g_dma_ctx.config.buffer_size) {
        return ETH_INVALID_PARAM;
    }

    uint16_t tail = g_dma_ctx.tx_desc_tail;
    uint16_t next_tail = eth_dma_next_tx_desc(tail, g_dma_ctx.config.tx_desc_count);

    /* 检查环形缓冲区是否满 */
    if (next_tail == g_dma_ctx.tx_desc_head) {
        g_dma_ctx.stats.tx_dropped++;
        return ETH_BUSY;
    }

    eth_dma_tx_desc_t *desc = &g_dma_ctx.tx_desc_ring[tail];

    /* 检查描述符是否可用(不应该由DMA拥有) */
    if (eth_dma_tx_desc_owned_by_dma(desc)) {
        g_dma_ctx.stats.tx_dropped++;
        return ETH_BUSY;
    }

    /* 复制数据到发送缓冲区 */
    memcpy(g_dma_ctx.tx_buffers[tail], packet->buffer, packet->length);

    /* 配置描述符 */
    desc->td1 = (packet->length & ETH_TX_DESC_TBS_MASK);
    desc->td0 = ETH_TX_DESC_OWN | ETH_TX_DESC_FS | ETH_TX_DESC_LS;

    /* 如果启用中断，添加中断位 */
    if (g_dma_ctx.config.tx_interrupt) {
        desc->td0 |= ETH_TX_DESC_IC;
    }

    /* 更新尾指针 */
    g_dma_ctx.tx_desc_tail = next_tail;

    /* 更新统计 */
    g_dma_ctx.stats.tx_packets++;
    g_dma_ctx.stats.tx_bytes += packet->length;

    return ETH_OK;
}

eth_status_t eth_dma_tx_trigger(void)
{
    if (g_dma_ctx.state != ETH_DMA_STATE_RUNNING) {
        return ETH_ERROR;
    }

    /* 触发DMA发送(实际实现需要操作硬件寄存器) */

    return ETH_OK;
}

eth_status_t eth_dma_tx_check_complete(uint16_t *completed)
{
    if (g_dma_ctx.state != ETH_DMA_STATE_RUNNING) {
        return ETH_ERROR;
    }

    if (completed == NULL) {
        return ETH_INVALID_PARAM;
    }

    uint16_t count = 0;
    uint16_t head = g_dma_ctx.tx_desc_head;
    uint16_t tail = g_dma_ctx.tx_desc_tail;

    /* 计算已完成的描述符 */
    while (head != tail) {
        eth_dma_tx_desc_t *desc = &g_dma_ctx.tx_desc_ring[head];

        if (eth_dma_tx_desc_owned_by_dma(desc)) {
            break;  /* 还在发送中 */
        }

        count++;
        head = eth_dma_next_tx_desc(head, g_dma_ctx.config.tx_desc_count);
    }

    *completed = count;
    return ETH_OK;
}

eth_status_t eth_dma_register_tx_callback(eth_dma_tx_complete_t callback, void *user_data)
{
    if (g_dma_ctx.state == ETH_DMA_STATE_UNINIT) {
        return ETH_ERROR;
    }

    g_dma_ctx.tx_callback = callback;
    g_dma_ctx.tx_user_data = user_data;

    return ETH_OK;
}

eth_status_t eth_dma_handle_tx_interrupt(void)
{
    if (g_dma_ctx.state != ETH_DMA_STATE_RUNNING) {
        return ETH_ERROR;
    }

    uint16_t completed = 0;
    uint16_t head = g_dma_ctx.tx_desc_head;
    uint16_t tail = g_dma_ctx.tx_desc_tail;

    /* 处理已完成的发送描述符 */
    while (head != tail) {
        eth_dma_tx_desc_t *desc = &g_dma_ctx.tx_desc_ring[head];

        if (eth_dma_tx_desc_owned_by_dma(desc)) {
            break;  /* 还在发送中 */
        }

        completed++;
        head = eth_dma_next_tx_desc(head, g_dma_ctx.config.tx_desc_count);
    }

    /* 更新头指针 */
    g_dma_ctx.tx_desc_head = head;

    /* 调用用户回调 */
    if (completed > 0 && g_dma_ctx.tx_callback != NULL) {
        g_dma_ctx.tx_callback(completed, g_dma_ctx.tx_user_data);
    }

    return ETH_OK;
}

eth_status_t eth_dma_tx_get_available(uint16_t *available)
{
    if (g_dma_ctx.state != ETH_DMA_STATE_RUNNING) {
        return ETH_ERROR;
    }

    if (available == NULL) {
        return ETH_INVALID_PARAM;
    }

    uint16_t used = 0;
    uint16_t head = g_dma_ctx.tx_desc_head;
    uint16_t tail = g_dma_ctx.tx_desc_tail;

    /* 计算已使用的描述符数量 */
    if (tail >= head) {
        used = tail - head;
    } else {
        used = g_dma_ctx.config.tx_desc_count - head + tail;
    }

    *available = g_dma_ctx.config.tx_desc_count - used - 1;
    return ETH_OK;
}

/* ============================================================================
 * 管理API实现
 * ============================================================================ */

eth_status_t eth_dma_get_stats(eth_dma_stats_t *stats)
{
    if (g_dma_ctx.state == ETH_DMA_STATE_UNINIT) {
        return ETH_ERROR;
    }

    if (stats == NULL) {
        return ETH_INVALID_PARAM;
    }

    memcpy(stats, &g_dma_ctx.stats, sizeof(eth_dma_stats_t));
    return ETH_OK;
}

eth_status_t eth_dma_clear_stats(void)
{
    if (g_dma_ctx.state == ETH_DMA_STATE_UNINIT) {
        return ETH_ERROR;
    }

    memset(&g_dma_ctx.stats, 0, sizeof(eth_dma_stats_t));
    return ETH_OK;
}

eth_status_t eth_dma_get_state(eth_dma_state_t *state)
{
    if (state == NULL) {
        return ETH_INVALID_PARAM;
    }

    *state = g_dma_ctx.state;
    return ETH_OK;
}

eth_status_t eth_dma_get_error(eth_dma_error_t *error)
{
    if (error == NULL) {
        return ETH_INVALID_PARAM;
    }

    *error = g_dma_ctx.error;
    return ETH_OK;
}

eth_status_t eth_dma_clear_error(void)
{
    g_dma_ctx.error = ETH_DMA_ERR_NONE;

    if (g_dma_ctx.state == ETH_DMA_STATE_ERROR) {
        g_dma_ctx.state = ETH_DMA_STATE_STOPPED;
    }

    return ETH_OK;
}

eth_status_t eth_dma_configure_interrupt(bool enable_rx, bool enable_tx, bool enable_err)
{
    if (g_dma_ctx.state == ETH_DMA_STATE_UNINIT) {
        return ETH_ERROR;
    }

    g_dma_ctx.config.rx_interrupt = enable_rx;
    g_dma_ctx.config.tx_interrupt = enable_tx;
    g_dma_ctx.config.error_interrupt = enable_err;

    /* 应用到硬件(实际实现) */

    return ETH_OK;
}

eth_status_t eth_dma_handle_error_interrupt(void)
{
    if (g_dma_ctx.state != ETH_DMA_STATE_RUNNING) {
        return ETH_ERROR;
    }

    /* 处理DMA错误(实际实现需要读取错误寄存器) */

    g_dma_ctx.error = ETH_DMA_ERR_BUS;  /* 模拟错误 */
    g_dma_ctx.state = ETH_DMA_STATE_ERROR;

    /* 调用错误回调 */
    if (g_dma_ctx.error_callback != NULL) {
        g_dma_ctx.error_callback(g_dma_ctx.error, g_dma_ctx.error_user_data);
    }

    return ETH_OK;
}

eth_status_t eth_dma_register_error_callback(eth_dma_error_t_callback callback, void *user_data)
{
    if (g_dma_ctx.state == ETH_DMA_STATE_UNINIT) {
        return ETH_ERROR;
    }

    g_dma_ctx.error_callback = callback;
    g_dma_ctx.error_user_data = user_data;

    return ETH_OK;
}

/* ============================================================================
 * 内存管理API实现
 * ============================================================================ */

void* eth_dma_alloc_buffer(uint32_t size)
{
    /* 对齐到ETH_DMA_ALIGN字节边界 */
    uint32_t aligned_size = ETH_ALIGN_UP(size, ETH_DMA_ALIGN);

    /* 使用对齐的内存分配 */
    void *buffer = aligned_alloc(ETH_DMA_ALIGN, aligned_size);

    return buffer;
}

void eth_dma_free_buffer(void *buffer)
{
    if (buffer != NULL) {
        free(buffer);
    }
}

eth_status_t eth_dma_invalidate_buffer(void *buffer, uint32_t size)
{
    if (buffer == NULL) {
        return ETH_INVALID_PARAM;
    }

    /* 使缓存无效(实际实现需要操作MMU/cache) */
    (void)size;

    return ETH_OK;
}

eth_status_t eth_dma_flush_buffer(void *buffer, uint32_t size)
{
    if (buffer == NULL) {
        return ETH_INVALID_PARAM;
    }

    /* 刷新缓存(实际实现需要操作MMU/cache) */
    (void)size;

    return ETH_OK;
}
