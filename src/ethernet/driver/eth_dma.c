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
/* @req SHALL_DRIVER */


#include "eth_dma.h"
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * 静态存储 (ISO 26262 / AUTOSAR R21-11 BSW 禁止动态内存)
 * ============================================================================ */

/* 描述符环: 编译期按最大配置固定分配 */
static eth_dma_rx_desc_t s_rx_desc_ring_storage[ETH_DMA_MAX_RX_DESC_COUNT];
static eth_dma_tx_desc_t s_tx_desc_ring_storage[ETH_DMA_MAX_TX_DESC_COUNT];

/* 缓冲区池: 按最大描述符数 x 巨型帧大小固定分配 */
static uint8_t s_rx_buf_pool[ETH_DMA_MAX_RX_DESC_COUNT][ETH_DMA_JUMBO_BUFFER_SIZE + ETH_DMA_ALIGN];
static uint8_t s_tx_buf_pool[ETH_DMA_MAX_TX_DESC_COUNT][ETH_DMA_JUMBO_BUFFER_SIZE + ETH_DMA_ALIGN];

/* 缓冲区指针数组 (描述符 -> 池内缓冲区) */
static uint8_t *s_rx_buf_ptrs[ETH_DMA_MAX_RX_DESC_COUNT];
static uint8_t *s_tx_buf_ptrs[ETH_DMA_MAX_TX_DESC_COUNT];

/* 缓冲区使用位图 (1=已分配) */
static uint32_t s_rx_buf_used[(ETH_DMA_MAX_RX_DESC_COUNT + 31U) / 32U];
static uint32_t s_tx_buf_used[(ETH_DMA_MAX_TX_DESC_COUNT + 31U) / 32U];

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
#define ETH_ALIGN_UP(x, a)      (((x) + ((a) - 1U)) & ~((a) - 1U))
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
    if ((config->rx_desc_count == 0U) || (config->rx_desc_count > ETH_DMA_MAX_RX_DESC_COUNT)) {
        return ETH_INVALID_PARAM;
    }
    if ((config->tx_desc_count == 0U) || (config->tx_desc_count > ETH_DMA_MAX_TX_DESC_COUNT)) {
        return ETH_INVALID_PARAM;
    }

    /* 验证缓冲区大小 */
    if (config->buffer_size < 64U) {
        return ETH_INVALID_PARAM;
    }
    if (config->enable_jumbo && (config->buffer_size < ETH_DMA_JUMBO_BUFFER_SIZE)) {
        return ETH_INVALID_PARAM;
    }

    return ETH_OK;
}

/**
 * @brief 计算下一个描述符索引(环形)
 */
static inline uint16_t eth_dma_next_rx_desc(uint16_t current, uint16_t count)
{
    return (current + 1U) % count;
}

static inline uint16_t eth_dma_next_tx_desc(uint16_t current, uint16_t count)
{
    return (current + 1U) % count;
}

/**
 * @brief 获取接收描述符所有权状态
 */
static inline bool eth_dma_rx_desc_owned_by_dma(const eth_dma_rx_desc_t *desc)
{
    return (desc->rd0 & ETH_RX_DESC_OWN) != 0U;
}

/**
 * @brief 获取发送描述符所有权状态
 */
static inline bool eth_dma_tx_desc_owned_by_dma(const eth_dma_tx_desc_t *desc)
{
    return (desc->td0 & ETH_TX_DESC_OWN) != 0U;
}

/**
 * @brief 初始化接收描述符环 (静态池, 无动态分配)
 */
static eth_status_t eth_dma_init_rx_ring(void)
{
    uint16_t count = g_dma_ctx.config.rx_desc_count;
    uint32_t buffer_size = g_dma_ctx.config.buffer_size;

    /* 使用静态描述符环 (编译期最大配置) */
    g_dma_ctx.rx_desc_ring = s_rx_desc_ring_storage;
    memset(g_dma_ctx.rx_desc_ring, 0, sizeof(eth_dma_rx_desc_t) * ETH_DMA_MAX_RX_DESC_COUNT);

    /* 使用静态缓冲区指针数组 */
    g_dma_ctx.rx_buffers = s_rx_buf_ptrs;
    memset(g_dma_ctx.rx_buffers, 0, sizeof(uint8_t *) * ETH_DMA_MAX_RX_DESC_COUNT);

    /* 分配和初始化描述符 */
    for (uint16_t i = 0; i < count; i++) {
        /* 从静态池分配缓冲区 (越界检查: count <= MAX 已由校验保证) */
        g_dma_ctx.rx_buffers[i] = (uint8_t *)eth_dma_alloc_buffer(buffer_size);
        if (g_dma_ctx.rx_buffers[i] == NULL) {
            /* 清理已分配的缓冲区 (静态池回收) */
            for (uint16_t j = 0; j < i; j++) {
                eth_dma_free_buffer(g_dma_ctx.rx_buffers[j]);
            }
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
 * @brief 初始化发送描述符环 (静态池, 无动态分配)
 */
static eth_status_t eth_dma_init_tx_ring(void)
{
    uint16_t count = g_dma_ctx.config.tx_desc_count;
    uint32_t buffer_size = g_dma_ctx.config.buffer_size;

    /* 使用静态描述符环 (编译期最大配置) */
    g_dma_ctx.tx_desc_ring = s_tx_desc_ring_storage;
    memset(g_dma_ctx.tx_desc_ring, 0, sizeof(eth_dma_tx_desc_t) * ETH_DMA_MAX_TX_DESC_COUNT);

    /* 使用静态缓冲区指针数组 */
    g_dma_ctx.tx_buffers = s_tx_buf_ptrs;
    memset(g_dma_ctx.tx_buffers, 0, sizeof(uint8_t *) * ETH_DMA_MAX_TX_DESC_COUNT);

    /* 分配和初始化描述符 */
    for (uint16_t i = 0; i < count; i++) {
        /* 从静态池分配缓冲区 */
        g_dma_ctx.tx_buffers[i] = (uint8_t *)eth_dma_alloc_buffer(buffer_size);
        if (g_dma_ctx.tx_buffers[i] == NULL) {
            /* 清理已分配的缓冲区 (静态池回收) */
            for (uint16_t j = 0; j < i; j++) {
                eth_dma_free_buffer(g_dma_ctx.tx_buffers[j]);
            }
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
 * @brief 释放接收环形缓冲区 (静态池回收)
 */
static void eth_dma_deinit_rx_ring(void)
{
    if (g_dma_ctx.rx_desc_ring == NULL) {
        return;
    }

    uint16_t count = g_dma_ctx.config.rx_desc_count;

    /* 回收缓冲区 */
    if (g_dma_ctx.rx_buffers != NULL) {
        for (uint16_t i = 0; i < count; i++) {
            if (g_dma_ctx.rx_buffers[i] != NULL) {
                eth_dma_free_buffer(g_dma_ctx.rx_buffers[i]);
            }
        }
        g_dma_ctx.rx_buffers = NULL;
    }

    /* 静态描述符环无需释放 */
    g_dma_ctx.rx_desc_ring = NULL;
}

/**
 * @brief 释放发送环形缓冲区 (静态池回收)
 */
static void eth_dma_deinit_tx_ring(void)
{
    if (g_dma_ctx.tx_desc_ring == NULL) {
        return;
    }

    uint16_t count = g_dma_ctx.config.tx_desc_count;

    /* 回收缓冲区 */
    if (g_dma_ctx.tx_buffers != NULL) {
        for (uint16_t i = 0; i < count; i++) {
            if (g_dma_ctx.tx_buffers[i] != NULL) {
                eth_dma_free_buffer(g_dma_ctx.tx_buffers[i]);
            }
        }
        g_dma_ctx.tx_buffers = NULL;
    }

    /* 静态描述符环无需释放 */
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
    if ((g_dma_ctx.state != ETH_DMA_STATE_INIT) &&
        (g_dma_ctx.state != ETH_DMA_STATE_STOPPED)) {
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
    if ((status != ETH_OK) && (status != ETH_ERROR)) {
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
    if ((status & ETH_RX_DESC_ES) != 0U) {
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

        if ((status == ETH_OK) && packet.valid) {
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

    if ((packet == NULL) || (packet->buffer == NULL) || (packet->length == 0U)) {
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
    if ((completed > 0U) && (g_dma_ctx.tx_callback != NULL)) {
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

    *available = g_dma_ctx.config.tx_desc_count - used - 1U;
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
 * 内存管理API实现 (静态池)
 * ============================================================================ */

void* eth_dma_alloc_buffer(uint32_t size)
{
    /* 越界检查: 仅支持静态池覆盖范围 (巨型帧上限) */
    if (size > ETH_DMA_JUMBO_BUFFER_SIZE) {
        return NULL;
    }

    /* 接收池优先, 其次发送池 */
    for (uint32_t i = 0; i < ETH_DMA_MAX_RX_DESC_COUNT; i++) {
        uint32_t word = i / 32U;
        uint32_t bit = i % 32U;
        if ((s_rx_buf_used[word] & (1U << bit)) == 0U) {
            s_rx_buf_used[word] |= (1U << bit);
            /* 对齐到 ETH_DMA_ALIGN 字节边界 */
            uintptr_t base = (uintptr_t)&s_rx_buf_pool[i][0];
            uintptr_t aligned = (base + (uintptr_t)ETH_DMA_ALIGN - 1U) &
                                ~((uintptr_t)ETH_DMA_ALIGN - 1U);
            return (void *)aligned;
        }
    }
    for (uint32_t i = 0; i < ETH_DMA_MAX_TX_DESC_COUNT; i++) {
        uint32_t word = i / 32U;
        uint32_t bit = i % 32U;
        if ((s_tx_buf_used[word] & (1U << bit)) == 0U) {
            s_tx_buf_used[word] |= (1U << bit);
            uintptr_t base = (uintptr_t)&s_tx_buf_pool[i][0];
            uintptr_t aligned = (base + (uintptr_t)ETH_DMA_ALIGN - 1U) &
                                ~((uintptr_t)ETH_DMA_ALIGN - 1U);
            return (void *)aligned;
        }
    }

    return NULL; /* 池耗尽 */
}

void eth_dma_free_buffer(void *buffer)
{
    if (buffer == NULL) {
        return;
    }

    /* 判定缓冲区所属池并回收 (静态池, 无需 free) */
    uintptr_t ptr = (uintptr_t)buffer;
    uintptr_t rx_base = (uintptr_t)&s_rx_buf_pool[0][0];
    uintptr_t rx_end = rx_base + (uintptr_t)(sizeof(s_rx_buf_pool));
    uintptr_t tx_base = (uintptr_t)&s_tx_buf_pool[0][0];
    uintptr_t tx_end = tx_base + (uintptr_t)(sizeof(s_tx_buf_pool));

    if ((ptr >= rx_base) && (ptr < rx_end)) {
        uint32_t row = (uint32_t)((ptr - rx_base) / (uintptr_t)(sizeof(s_rx_buf_pool[0])));
        if (row < ETH_DMA_MAX_RX_DESC_COUNT) {
            s_rx_buf_used[row / 32U] &= ~(1U << (row % 32U));
        }
    } else if ((ptr >= tx_base) && (ptr < tx_end)) {
        uint32_t row = (uint32_t)((ptr - tx_base) / (uintptr_t)(sizeof(s_tx_buf_pool[0])));
        if (row < ETH_DMA_MAX_TX_DESC_COUNT) {
            s_tx_buf_used[row / 32U] &= ~(1U << (row % 32U));
        }
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
