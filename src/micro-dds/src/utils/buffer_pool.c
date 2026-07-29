/** @file buffer_pool.c
 * @brief 静态缓冲区池实现
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 *
 * 使用静态内存分配，避免malloc/free
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* ============================================================================
 * 配置定义
 * ============================================================================ */

#ifndef MICRODDS_BUFFER_POOL_SIZE
#define MICRODDS_BUFFER_POOL_SIZE 8U  /* 缓冲区数量 */
#endif

#ifndef MICRODDS_BUFFER_SIZE
#define MICRODDS_BUFFER_SIZE 512U  /* 每个缓冲区大小 */
#endif

/* ============================================================================
 * 内部数据结构
 * ============================================================================ */

typedef struct {
    uint8_t data[MICRODDS_BUFFER_SIZE];
    bool in_use;
    uint16_t used_size;
} Buffer_Entry;

typedef struct {
    Buffer_Entry buffers[MICRODDS_BUFFER_POOL_SIZE];
    bool initialized;
} Buffer_Pool;

/* ============================================================================
 * 全局状态
 * ============================================================================ */

static Buffer_Pool g_buffer_pool;

/* ============================================================================
 * 公共API实现
 * ============================================================================ */

/**
 * @brief 初始化缓冲区池
 * @return true 成功
 */
bool MicroDDS_BufferPool_Init(void) {
    if (g_buffer_pool.initialized) {
        return true;
    }

    for (uint32_t i = 0U; i < MICRODDS_BUFFER_POOL_SIZE; i++) {
        g_buffer_pool.buffers[i].in_use = false;
        g_buffer_pool.buffers[i].used_size = 0U;
        /* 数据区域不需要清零 */
    }

    g_buffer_pool.initialized = true;
    return true;
}

/**
 * @brief 关闭缓冲区池
 */
void MicroDDS_BufferPool_Shutdown(void) {
    if (!g_buffer_pool.initialized) {
        return;
    }

    for (uint32_t i = 0U; i < MICRODDS_BUFFER_POOL_SIZE; i++) {
        g_buffer_pool.buffers[i].in_use = false;
    }

    g_buffer_pool.initialized = false;
}

/**
 * @brief 分配缓冲区
 * @return 缓冲区指针，失败返回NULL_PTR
 */
void* MicroDDS_BufferPool_Alloc(void) {
    if (!g_buffer_pool.initialized) {
        return NULL_PTR;
    }

    for (uint32_t i = 0U; i < MICRODDS_BUFFER_POOL_SIZE; i++) {
        if (!g_buffer_pool.buffers[i].in_use) {
            g_buffer_pool.buffers[i].in_use = true;
            g_buffer_pool.buffers[i].used_size = 0U;
            return g_buffer_pool.buffers[i].data;
        }
    }

    return NULL_PTR;  /* 没有可用缓冲区 */
}

/**
 * @brief 释放缓冲区
 * @param buffer 缓冲区指针
 */
void MicroDDS_BufferPool_Free(void* buffer) {
    if (buffer == NULL_PTR) {
        return;
    }

    for (uint32_t i = 0U; i < MICRODDS_BUFFER_POOL_SIZE; i++) {
        if (g_buffer_pool.buffers[i].data == buffer) {
            g_buffer_pool.buffers[i].in_use = false;
            g_buffer_pool.buffers[i].used_size = 0U;
            return;
        }
    }
}

/**
 * @brief 获取缓冲区大小
 * @return 缓冲区大小
 */
uint16_t MicroDDS_BufferPool_GetBufferSize(void) {
    return MICRODDS_BUFFER_SIZE;
}

/**
 * @brief 获取可用缓冲区数量
 * @return 可用缓冲区数量
 */
uint32_t MicroDDS_BufferPool_GetAvailableCount(void) {
    if (!g_buffer_pool.initialized) {
        return 0U;
    }

    uint32_t count = 0U;
    for (uint32_t i = 0U; i < MICRODDS_BUFFER_POOL_SIZE; i++) {
        if (!g_buffer_pool.buffers[i].in_use) {
            count++;
        }
    }

    return count;
}
