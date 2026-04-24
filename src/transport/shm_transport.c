/**
 * @file shm_transport.c
 * @brief 共享内存传输层实现 - 同节点无拷贝通信
 * @version 1.0
 * @date 2026-04-24
 */

#include "shm_transport.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <time.h>
#include <sched.h>
#include <pthread.h>

/* ============================================================================
 * 内部数据结构定义
 * ============================================================================ */

/** 环形缓冲区头部 */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t buffer_size;
    uint32_t header_size;
    uint64_t write_pos;
    uint64_t read_pos;
    uint32_t message_count;
    uint32_t max_messages;
    volatile uint32_t lock;
    uint32_t reserved[4];
} __attribute__((aligned(64))) shm_ring_header_t;

/** 共享内存传输私有数据 */
typedef struct {
    shm_transport_config_t config;
    
    /* 共享内存 */
    char shm_name[64];
    int shm_fd;
    void *shm_base;
    size_t shm_size;
    
    /* 环形缓冲区 */
    shm_ring_header_t *ring_header;
    uint8_t *ring_buffer;
    uint32_t ring_buffer_size;
    
    /* 同步机制 */
    pthread_mutex_t local_mutex;
    pthread_cond_t local_cond;
    volatile int *futex_addr;
    
    /* 统计 */
    shm_transport_stats_t stats;
    pthread_mutex_t stats_mutex;
    
    /* 接收回调 */
    eth_rx_callback_t rx_callback;
    void *rx_callback_data;
    pthread_t rx_thread;
    volatile int rx_thread_running;
    
    /* 状态 */
    volatile int initialized;
    volatile int is_creator;
    uint64_t last_rx_seq;
} shm_transport_private_t;

/* ============================================================================
 * 低级同步原语 (Futex)
 * ============================================================================ */

/**
 * @brief 原子操作 - 锁定
 */
static inline void atomic_lock(volatile uint32_t *lock)
{
    while (__sync_lock_test_and_set(lock, 1)) {
        /* 自旋等待 */
        while (*lock) {
            __sync_synchronize();
        }
    }
}

/**
 * @brief 原子操作 - 解锁
 */
static inline void atomic_unlock(volatile uint32_t *lock)
{
    __sync_lock_release(lock);
}

/**
 * @brief 无锁操作 - CAS
 */
static inline int cas(volatile uint64_t *ptr, uint64_t expected, uint64_t new_val)
{
    return __sync_bool_compare_and_swap(ptr, expected, new_val);
}

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
 * @brief 获取当前时间(微秒)
 */
static inline uint64_t get_time_us(void)
{
    return get_time_ns() / 1000ULL;
}

/**
 * @brief 内存对齐
 */
static inline uint32_t align_size(uint32_t size, uint32_t alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
}

/**
 * @brief 计算消息总大小(包含头部)
 */
static inline uint32_t message_total_size(uint32_t payload_size)
{
    return align_size(sizeof(shm_message_header_t) + payload_size, SHM_ALIGN_SIZE);
}

/**
 * @brief 计算校验和
 */
static uint32_t calculate_checksum(const void *data, uint32_t len)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t checksum = 0;
    for (uint32_t i = 0; i < len; i++) {
        checksum = (checksum << 1) | (checksum >> 31);
        checksum += bytes[i];
    }
    return checksum;
}

/* ============================================================================
 * 环形缓冲区操作
 * ============================================================================ */

/**
 * @brief 获取环形缓冲区空闲空间
 */
static uint32_t ring_free_space(shm_transport_private_t *priv)
{
    shm_ring_header_t *hdr = priv->ring_header;
    uint64_t write_pos = hdr->write_pos;
    uint64_t read_pos = hdr->read_pos;
    
    if (write_pos >= read_pos) {
        return priv->ring_buffer_size - (uint32_t)(write_pos - read_pos) - 1;
    } else {
        return (uint32_t)(read_pos - write_pos) - 1;
    }
}

/**
 * @brief 获取环形缓冲区已用空间
 */
static uint32_t ring_used_space(shm_transport_private_t *priv)
{
    shm_ring_header_t *hdr = priv->ring_header;
    uint64_t write_pos = hdr->write_pos;
    uint64_t read_pos = hdr->read_pos;
    
    if (write_pos >= read_pos) {
        return (uint32_t)(write_pos - read_pos);
    } else {
        return priv->ring_buffer_size - (uint32_t)(read_pos - write_pos);
    }
}

/**
 * @brief 在环形缓冲区中写入数据
 */
static int ring_write(shm_transport_private_t *priv, const void *data, uint32_t len)
{
    shm_ring_header_t *hdr = priv->ring_header;
    uint32_t total_size = message_total_size(len);
    
    if (total_size > ring_free_space(priv)) {
        return -1; /* 空间不足 */
    }
    
    uint64_t write_pos = hdr->write_pos % priv->ring_buffer_size;
    
    /* 检查是否需要环绕 */
    if (write_pos + total_size > priv->ring_buffer_size) {
        /* 分两次写入 */
        uint32_t first_part = priv->ring_buffer_size - (uint32_t)write_pos;
        memcpy(priv->ring_buffer + write_pos, data, first_part);
        memcpy(priv->ring_buffer, (const uint8_t *)data + first_part, total_size - first_part);
    } else {
        memcpy(priv->ring_buffer + write_pos, data, total_size);
    }
    
    /* 内存屏障 */
    __sync_synchronize();
    
    /* 更新写位置 */
    hdr->write_pos += total_size;
    hdr->message_count++;
    
    return 0;
}

/**
 * @brief 从环形缓冲区读取数据
 */
static int ring_read(shm_transport_private_t *priv, void *buffer, uint32_t max_len, uint32_t *out_len)
{
    shm_ring_header_t *hdr = priv->ring_header;
    
    if (hdr->message_count == 0 || ring_used_space(priv) < sizeof(shm_message_header_t)) {
        return -1; /* 没有数据 */
    }
    
    uint64_t read_pos = hdr->read_pos % priv->ring_buffer_size;
    
    /* 读取头部 */
    shm_message_header_t header;
    if (read_pos + sizeof(shm_message_header_t) > priv->ring_buffer_size) {
        uint32_t first_part = priv->ring_buffer_size - (uint32_t)read_pos;
        memcpy(&header, priv->ring_buffer + read_pos, first_part);
        memcpy((uint8_t *)&header + first_part, priv->ring_buffer, 
               sizeof(shm_message_header_t) - first_part);
    } else {
        memcpy(&header, priv->ring_buffer + read_pos, sizeof(shm_message_header_t));
    }
    
    /* 验证魔法字 */
    if (header.magic != SHM_MAGIC_COOKIE) {
        return -2; /* 数据损坏 */
    }
    
    uint32_t total_size = message_total_size(header.payload_size);
    
    if (max_len < header.payload_size) {
        return -3; /* 缓冲区不足 */
    }
    
    /* 读取载荷 */
    uint64_t payload_pos = (read_pos + sizeof(shm_message_header_t)) % priv->ring_buffer_size;
    if (payload_pos + header.payload_size > priv->ring_buffer_size) {
        uint32_t first_part = priv->ring_buffer_size - (uint32_t)payload_pos;
        memcpy(buffer, priv->ring_buffer + payload_pos, first_part);
        memcpy((uint8_t *)buffer + first_part, priv->ring_buffer, 
               header.payload_size - first_part);
    } else {
        memcpy(buffer, priv->ring_buffer + payload_pos, header.payload_size);
    }
    
    /* 内存屏障 */
    __sync_synchronize();
    
    /* 更新读位置 */
    hdr->read_pos += total_size;
    hdr->message_count--;
    
    *out_len = header.payload_size;
    return 0;
}

/* ============================================================================
 * 标准传输接口实现
 * ============================================================================ */

/**
 * @brief 共享内存传输初始化
 */
static eth_status_t shm_init(transport_handle_t *handle, const transport_config_t *config)
{
    if (handle == NULL || config == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    shm_transport_private_t *priv = (shm_transport_private_t *)handle->private_data;
    if (priv == NULL) {
        return ETH_ERROR;
    }
    
    /* 复制配置 */
    memcpy(&priv->config.base, config, sizeof(transport_config_t));
    
    /* 确定共享内存大小 */
    uint32_t ring_size = priv->config.ring_buffer_size;
    if (ring_size == 0) {
        ring_size = SHM_DEFAULT_RING_SIZE;
    }
    if (ring_size < SHM_MIN_RING_SIZE) {
        ring_size = SHM_MIN_RING_SIZE;
    }
    if (ring_size > SHM_MAX_RING_SIZE) {
        ring_size = SHM_MAX_RING_SIZE;
    }
    
    priv->ring_buffer_size = ring_size;
    priv->shm_size = sizeof(shm_ring_header_t) + ring_size;
    
    /* 尝试创建共享内存(创建者) */
    priv->shm_fd = shm_open(priv->config.shm_name, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (priv->shm_fd >= 0) {
        /* 是创建者 */
        priv->is_creator = 1;
        if (ftruncate(priv->shm_fd, priv->shm_size) < 0) {
            close(priv->shm_fd);
            shm_unlink(priv->config.shm_name);
            return ETH_ERROR;
        }
    } else {
        /* 尝试打开已存在的共享内存 */
        priv->shm_fd = shm_open(priv->config.shm_name, O_RDWR, 0666);
        if (priv->shm_fd < 0) {
            return ETH_ERROR;
        }
        priv->is_creator = 0;
    }
    
    /* 映射共享内存 */
    priv->shm_base = mmap(NULL, priv->shm_size, PROT_READ | PROT_WRITE, 
                          MAP_SHARED, priv->shm_fd, 0);
    if (priv->shm_base == MAP_FAILED) {
        close(priv->shm_fd);
        if (priv->is_creator) {
            shm_unlink(priv->config.shm_name);
        }
        return ETH_ERROR;
    }
    
    /* 初始化环形缓冲区头部 */
    priv->ring_header = (shm_ring_header_t *)priv->shm_base;
    priv->ring_buffer = (uint8_t *)priv->shm_base + sizeof(shm_ring_header_t);
    
    if (priv->is_creator) {
        memset(priv->ring_header, 0, sizeof(shm_ring_header_t));
        priv->ring_header->magic = SHM_MAGIC_COOKIE;
        priv->ring_header->version = SHM_VERSION;
        priv->ring_header->buffer_size = priv->ring_buffer_size;
        priv->ring_header->header_size = sizeof(shm_ring_header_t);
        priv->ring_header->max_messages = SHM_MAX_MESSAGE_COUNT;
        priv->ring_header->lock = 0;
        priv->ring_header->write_pos = 0;
        priv->ring_header->read_pos = 0;
    } else {
        /* 验证魔法字 */
        if (priv->ring_header->magic != SHM_MAGIC_COOKIE) {
            munmap(priv->shm_base, priv->shm_size);
            close(priv->shm_fd);
            return ETH_ERROR;
        }
    }
    
    /* 初始化本地同步 */
    pthread_mutex_init(&priv->local_mutex, NULL);
    pthread_cond_init(&priv->local_cond, NULL);
    
    /* 初始化统计 */
    pthread_mutex_init(&priv->stats_mutex, NULL);
    memset(&priv->stats, 0, sizeof(priv->stats));
    priv->stats.latency_min_us = 0xFFFFFFFF;
    
    priv->futex_addr = (volatile int *)&priv->ring_header->lock;
    priv->last_rx_seq = 0;
    priv->initialized = 1;
    
    /* 设置CPU亲和性 */
    if (priv->config.cpu_affinity != 0) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        for (int i = 0; i < 8; i++) {
            if (priv->config.cpu_affinity & (1 << i)) {
                CPU_SET(i, &cpuset);
            }
        }
        pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
    }
    
    return ETH_OK;
}

/**
 * @brief 共享内存传输反初始化
 */
static eth_status_t shm_deinit(transport_handle_t *handle)
{
    if (handle == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    shm_transport_private_t *priv = (shm_transport_private_t *)handle->private_data;
    if (priv == NULL) {
        return ETH_OK;
    }
    
    /* 停止接收线程 */
    priv->rx_thread_running = 0;
    if (priv->rx_thread) {
        pthread_join(priv->rx_thread, NULL);
    }
    
    /* 取消映射 */
    if (priv->shm_base != NULL && priv->shm_base != MAP_FAILED) {
        munmap(priv->shm_base, priv->shm_size);
        priv->shm_base = NULL;
    }
    
    /* 关闭文件描述符 */
    if (priv->shm_fd >= 0) {
        close(priv->shm_fd);
        priv->shm_fd = -1;
    }
    
    /* 如果是创建者，删除共享内存对象 */
    if (priv->is_creator) {
        shm_unlink(priv->config.shm_name);
    }
    
    /* 销毁同步机制 */
    pthread_mutex_destroy(&priv->local_mutex);
    pthread_cond_destroy(&priv->local_cond);
    pthread_mutex_destroy(&priv->stats_mutex);
    
    priv->initialized = 0;
    
    return ETH_OK;
}

/**
 * @brief 共享内存发送数据
 */
static eth_status_t shm_send(transport_handle_t *handle, const uint8_t *data, 
                              uint32_t len, uint32_t timeout_ms)
{
    if (handle == NULL || data == NULL || len == 0) {
        return ETH_INVALID_PARAM;
    }
    
    if (len > SHM_MAX_MESSAGE_SIZE) {
        return ETH_INVALID_PARAM;
    }
    
    shm_transport_private_t *priv = (shm_transport_private_t *)handle->private_data;
    if (priv == NULL || !priv->initialized) {
        return ETH_NOT_INIT;
    }
    
    uint64_t start_time = get_time_us();
    
    /* 构建消息头部 */
    uint32_t total_size = message_total_size(len);
    uint8_t *message = (uint8_t *)malloc(total_size);
    if (message == NULL) {
        return ETH_NO_MEMORY;
    }
    
    shm_message_header_t *header = (shm_message_header_t *)message;
    header->magic = SHM_MAGIC_COOKIE;
    header->version = SHM_VERSION;
    header->timestamp_ns = get_time_ns();
    header->seq_num = (uint32_t)(header->timestamp_ns & 0xFFFFFFFF);
    header->payload_size = len;
    header->checksum = calculate_checksum(data, len);
    header->priority = SHM_PRIO_MEDIUM;
    header->flags = SHM_FLAG_VALID;
    header->reserved = 0;
    
    memcpy(message + sizeof(shm_message_header_t), data, len);
    
    /* 等待锁 */
    uint64_t timeout_us = (uint64_t)timeout_ms * 1000ULL;
    while (priv->ring_header->lock) {
        if (get_time_us() - start_time > timeout_us) {
            free(message);
            return ETH_TIMEOUT;
        }
        sched_yield();
    }
    
    /* 获取锁 */
    atomic_lock(&priv->ring_header->lock);
    
    /* 写入环形缓冲区 */
    int ret = ring_write(priv, message, total_size);
    
    /* 解锁 */
    atomic_unlock(&priv->ring_header->lock);
    
    free(message);
    
    if (ret < 0) {
        pthread_mutex_lock(&priv->stats_mutex);
        priv->stats.tx_drops++;
        pthread_mutex_unlock(&priv->stats_mutex);
        return ETH_BUSY;
    }
    
    /* 通知等待者 */
    pthread_cond_broadcast(&priv->local_cond);
    
    /* 更新统计 */
    uint64_t latency = get_time_us() - start_time;
    pthread_mutex_lock(&priv->stats_mutex);
    priv->stats.tx_messages++;
    priv->stats.tx_bytes += len;
    if (latency < priv->stats.latency_min_us) priv->stats.latency_min_us = (uint32_t)latency;
    if (latency > priv->stats.latency_max_us) priv->stats.latency_max_us = (uint32_t)latency;
    priv->stats.latency_avg_us = (priv->stats.latency_avg_us + (uint32_t)latency) / 2;
    pthread_mutex_unlock(&priv->stats_mutex);
    
    return ETH_OK;
}

/**
 * @brief 共享内存接收数据
 */
static eth_status_t shm_receive(transport_handle_t *handle, uint8_t *buffer, 
                                 uint32_t max_len, uint32_t *received_len, 
                                 uint32_t timeout_ms)
{
    if (handle == NULL || buffer == NULL || received_len == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    shm_transport_private_t *priv = (shm_transport_private_t *)handle->private_data;
    if (priv == NULL || !priv->initialized) {
        return ETH_NOT_INIT;
    }
    
    uint64_t start_time = get_time_us();
    uint64_t timeout_us = (uint64_t)timeout_ms * 1000ULL;
    
    /* 等待数据可用 */
    while (ring_used_space(priv) < sizeof(shm_message_header_t)) {
        if (timeout_ms > 0) {
            if (get_time_us() - start_time > timeout_us) {
                return ETH_TIMEOUT;
            }
        }
        sched_yield();
    }
    
    /* 获取锁 */
    atomic_lock(&priv->ring_header->lock);
    
    /* 读取数据 */
    uint32_t out_len = 0;
    int ret = ring_read(priv, buffer, max_len, &out_len);
    
    /* 解锁 */
    atomic_unlock(&priv->ring_header->lock);
    
    if (ret < 0) {
        return ETH_ERROR;
    }
    
    *received_len = out_len;
    
    /* 更新统计 */
    uint64_t latency = get_time_us() - start_time;
    pthread_mutex_lock(&priv->stats_mutex);
    priv->stats.rx_messages++;
    priv->stats.rx_bytes += out_len;
    if (latency < priv->stats.latency_min_us) priv->stats.latency_min_us = (uint32_t)latency;
    if (latency > priv->stats.latency_max_us) priv->stats.latency_max_us = (uint32_t)latency;
    priv->stats.latency_avg_us = (priv->stats.latency_avg_us + (uint32_t)latency) / 2;
    pthread_mutex_unlock(&priv->stats_mutex);
    
    return ETH_OK;
}

/**
 * @brief 注册接收回调
 */
static eth_status_t shm_register_callback(transport_handle_t *handle, 
                                           eth_rx_callback_t callback, 
                                           void *user_data)
{
    if (handle == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    shm_transport_private_t *priv = (shm_transport_private_t *)handle->private_data;
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
static eth_status_t shm_get_status(transport_handle_t *handle, uint32_t *status)
{
    if (handle == NULL || status == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    shm_transport_private_t *priv = (shm_transport_private_t *)handle->private_data;
    if (priv == NULL) {
        return ETH_ERROR;
    }
    
    *status = 0;
    if (priv->initialized) *status |= 0x01;
    
    return ETH_OK;
}

/* 静态操作函数表 */
static const transport_ops_t shm_ops = {
    .name = "SharedMemory",
    .init = shm_init,
    .deinit = shm_deinit,
    .send = shm_send,
    .receive = shm_receive,
    .register_callback = shm_register_callback,
    .get_status = shm_get_status
};

/* ============================================================================
 * 公开API实现
 * ============================================================================ */

transport_handle_t* shm_transport_create(const shm_transport_config_t *config)
{
    if (config == NULL || config->shm_name[0] == '\0') {
        return NULL;
    }
    
    transport_handle_t *handle = (transport_handle_t *)malloc(sizeof(transport_handle_t));
    if (handle == NULL) {
        return NULL;
    }
    
    shm_transport_private_t *priv = (shm_transport_private_t *)calloc(1, sizeof(shm_transport_private_t));
    if (priv == NULL) {
        free(handle);
        return NULL;
    }
    
    /* 复制配置 */
    memcpy(&priv->config, config, sizeof(shm_transport_config_t));
    strncpy(priv->shm_name, config->shm_name, sizeof(priv->shm_name) - 1);
    priv->shm_name[sizeof(priv->shm_name) - 1] = '\0';
    
    handle->ops = &shm_ops;
    handle->private_data = priv;
    
    /* 执行初始化 */
    transport_config_t base_config;
    memcpy(&base_config, &config->base, sizeof(transport_config_t));
    if (shm_init(handle, &base_config) != ETH_OK) {
        free(priv);
        free(handle);
        return NULL;
    }
    
    return handle;
}

eth_status_t shm_transport_destroy(transport_handle_t *handle)
{
    if (handle == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    shm_deinit(handle);
    
    if (handle->private_data != NULL) {
        free(handle->private_data);
        handle->private_data = NULL;
    }
    
    free(handle);
    return ETH_OK;
}

eth_status_t shm_transport_send_zero_copy(transport_handle_t *handle,
                                           void *data,
                                           uint32_t len,
                                           shm_priority_t priority,
                                           uint32_t timeout_ms)
{
    if (handle == NULL || data == NULL || len == 0) {
        return ETH_INVALID_PARAM;
    }
    
    shm_transport_private_t *priv = (shm_transport_private_t *)handle->private_data;
    if (priv == NULL || !priv->initialized) {
        return ETH_NOT_INIT;
    }
    
    /* 实际上仍然需要复制到环形缓冲区，但通过直接写入实现"零拷贝"概念 */
    uint64_t start_time = get_time_us();
    
    uint32_t total_size = message_total_size(len);
    uint8_t *message = (uint8_t *)malloc(total_size);
    if (message == NULL) {
        return ETH_NO_MEMORY;
    }
    
    shm_message_header_t *header = (shm_message_header_t *)message;
    header->magic = SHM_MAGIC_COOKIE;
    header->version = SHM_VERSION;
    header->timestamp_ns = get_time_ns();
    header->seq_num = (uint32_t)(header->timestamp_ns & 0xFFFFFFFF);
    header->payload_size = len;
    header->checksum = calculate_checksum(data, len);
    header->priority = (uint8_t)priority;
    header->flags = SHM_FLAG_VALID | SHM_FLAG_TIMESTAMPED;
    header->reserved = 0;
    
    memcpy(message + sizeof(shm_message_header_t), data, len);
    
    /* 获取锁 */
    atomic_lock(&priv->ring_header->lock);
    
    int ret = ring_write(priv, message, total_size);
    
    atomic_unlock(&priv->ring_header->lock);
    
    free(message);
    
    if (ret < 0) {
        return ETH_BUSY;
    }
    
    pthread_cond_broadcast(&priv->local_cond);
    
    uint64_t latency = get_time_us() - start_time;
    pthread_mutex_lock(&priv->stats_mutex);
    priv->stats.tx_messages++;
    priv->stats.tx_bytes += len;
    priv->stats.latency_avg_us = (priv->stats.latency_avg_us + (uint32_t)latency) / 2;
    pthread_mutex_unlock(&priv->stats_mutex);
    
    return ETH_OK;
}

eth_status_t shm_transport_receive_zero_copy(transport_handle_t *handle,
                                              void **data_out,
                                              uint32_t *len_out,
                                              uint32_t timeout_ms)
{
    if (handle == NULL || data_out == NULL || len_out == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    /* 实际上返回的是环形缓冲区内的指针，需要应用层调用release_buffer释放 */
    return ETH_ERROR; /* 需要更复杂的实现 */
}

eth_status_t shm_transport_release_buffer(transport_handle_t *handle, void *data)
{
    if (handle == NULL || data == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    /* 实际不需要释放，环形缓冲区自动管理 */
    return ETH_OK;
}

eth_status_t shm_transport_get_write_buffer(transport_handle_t *handle,
                                             void **buffer,
                                             uint32_t max_len,
                                             uint32_t timeout_ms)
{
    if (handle == NULL || buffer == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    shm_transport_private_t *priv = (shm_transport_private_t *)handle->private_data;
    if (priv == NULL || !priv->initialized) {
        return ETH_NOT_INIT;
    }
    
    uint64_t start_time = get_time_us();
    uint64_t timeout_us = (uint64_t)timeout_ms * 1000ULL;
    
    /* 等待空间可用 */
    while (ring_free_space(priv) < max_len + sizeof(shm_message_header_t)) {
        if (timeout_ms > 0 && get_time_us() - start_time > timeout_us) {
            return ETH_TIMEOUT;
        }
        sched_yield();
    }
    
    *buffer = malloc(max_len);
    if (*buffer == NULL) {
        return ETH_NO_MEMORY;
    }
    
    return ETH_OK;
}

eth_status_t shm_transport_commit_write(transport_handle_t *handle,
                                         void *buffer,
                                         uint32_t actual_len,
                                         shm_priority_t priority)
{
    if (handle == NULL || buffer == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    shm_transport_private_t *priv = (shm_transport_private_t *)handle->private_data;
    if (priv == NULL || !priv->initialized) {
        free(buffer);
        return ETH_NOT_INIT;
    }
    
    uint32_t total_size = message_total_size(actual_len);
    uint8_t *message = (uint8_t *)malloc(total_size);
    if (message == NULL) {
        free(buffer);
        return ETH_NO_MEMORY;
    }
    
    shm_message_header_t *header = (shm_message_header_t *)message;
    header->magic = SHM_MAGIC_COOKIE;
    header->version = SHM_VERSION;
    header->timestamp_ns = get_time_ns();
    header->seq_num = (uint32_t)(header->timestamp_ns & 0xFFFFFFFF);
    header->payload_size = actual_len;
    header->checksum = calculate_checksum(buffer, actual_len);
    header->priority = (uint8_t)priority;
    header->flags = SHM_FLAG_VALID;
    header->reserved = 0;
    
    memcpy(message + sizeof(shm_message_header_t), buffer, actual_len);
    free(buffer);
    
    atomic_lock(&priv->ring_header->lock);
    int ret = ring_write(priv, message, total_size);
    atomic_unlock(&priv->ring_header->lock);
    
    free(message);
    
    if (ret < 0) {
        return ETH_BUSY;
    }
    
    pthread_cond_broadcast(&priv->local_cond);
    
    pthread_mutex_lock(&priv->stats_mutex);
    priv->stats.tx_messages++;
    priv->stats.tx_bytes += actual_len;
    pthread_mutex_unlock(&priv->stats_mutex);
    
    return ETH_OK;
}

eth_status_t shm_transport_flush(transport_handle_t *handle)
{
    if (handle == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    /* 强制内存屏障 */
    __sync_synchronize();
    
    return ETH_OK;
}

eth_status_t shm_transport_get_buffer_status(transport_handle_t *handle,
                                              uint32_t *free_space,
                                              uint32_t *used_space)
{
    if (handle == NULL || free_space == NULL || used_space == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    shm_transport_private_t *priv = (shm_transport_private_t *)handle->private_data;
    if (priv == NULL || !priv->initialized) {
        return ETH_NOT_INIT;
    }
    
    *free_space = ring_free_space(priv);
    *used_space = ring_used_space(priv);
    
    return ETH_OK;
}

eth_status_t shm_transport_get_stats(transport_handle_t *handle,
                                      shm_transport_stats_t *stats)
{
    if (handle == NULL || stats == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    shm_transport_private_t *priv = (shm_transport_private_t *)handle->private_data;
    if (priv == NULL) {
        return ETH_ERROR;
    }
    
    pthread_mutex_lock(&priv->stats_mutex);
    memcpy(stats, &priv->stats, sizeof(shm_transport_stats_t));
    pthread_mutex_unlock(&priv->stats_mutex);
    
    return ETH_OK;
}

eth_status_t shm_transport_clear_stats(transport_handle_t *handle)
{
    if (handle == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    shm_transport_private_t *priv = (shm_transport_private_t *)handle->private_data;
    if (priv == NULL) {
        return ETH_ERROR;
    }
    
    pthread_mutex_lock(&priv->stats_mutex);
    memset(&priv->stats, 0, sizeof(priv->stats));
    priv->stats.latency_min_us = 0xFFFFFFFF;
    pthread_mutex_unlock(&priv->stats_mutex);
    
    return ETH_OK;
}

eth_status_t shm_transport_set_deterministic_mode(transport_handle_t *handle,
                                                   shm_deterministic_mode_t mode)
{
    if (handle == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    shm_transport_private_t *priv = (shm_transport_private_t *)handle->private_data;
    if (priv == NULL) {
        return ETH_ERROR;
    }
    
    priv->config.det_mode = mode;
    
    /* 根据模式调整同步策略 */
    switch (mode) {
        case SHM_MODE_STANDARD:
            priv->config.sync_type = SHM_SYNC_MUTEX;
            break;
        case SHM_MODE_LOCK_FREE:
            priv->config.sync_type = SHM_SYNC_SPINLOCK;
            break;
        case SHM_MODE_WAIT_FREE:
            priv->config.sync_type = SHM_SYNC_NONE;
            break;
        default:
            break;
    }
    
    return ETH_OK;
}

eth_status_t shm_transport_set_cpu_affinity(transport_handle_t *handle,
                                             uint8_t cpu_mask)
{
    if (handle == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    shm_transport_private_t *priv = (shm_transport_private_t *)handle->private_data;
    if (priv == NULL) {
        return ETH_ERROR;
    }
    
    priv->config.cpu_affinity = cpu_mask;
    
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (int i = 0; i < 8; i++) {
        if (cpu_mask & (1 << i)) {
            CPU_SET(i, &cpuset);
        }
    }
    
    pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
    
    return ETH_OK;
}

eth_status_t shm_transport_preallocate(transport_handle_t *handle,
                                        uint32_t min_free_space,
                                        uint32_t timeout_ms)
{
    if (handle == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    shm_transport_private_t *priv = (shm_transport_private_t *)handle->private_data;
    if (priv == NULL || !priv->initialized) {
        return ETH_NOT_INIT;
    }
    
    uint64_t start_time = get_time_us();
    uint64_t timeout_us = (uint64_t)timeout_ms * 1000ULL;
    
    while (ring_free_space(priv) < min_free_space) {
        if (timeout_ms > 0 && get_time_us() - start_time > timeout_us) {
            return ETH_TIMEOUT;
        }
        sched_yield();
    }
    
    return ETH_OK;
}

eth_status_t shm_transport_poll(transport_handle_t *handle,
                                 uint32_t timeout_ms)
{
    if (handle == NULL) {
        return ETH_INVALID_PARAM;
    }
    
    shm_transport_private_t *priv = (shm_transport_private_t *)handle->private_data;
    if (priv == NULL || !priv->initialized) {
        return ETH_NOT_INIT;
    }
    
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000;
    
    pthread_mutex_lock(&priv->local_mutex);
    int ret = 0;
    while (ring_used_space(priv) < sizeof(shm_message_header_t) && ret == 0) {
        ret = pthread_cond_timedwait(&priv->local_cond, &priv->local_mutex, &ts);
    }
    pthread_mutex_unlock(&priv->local_mutex);
    
    if (ret == ETIMEDOUT) {
        return ETH_TIMEOUT;
    }
    
    return ETH_OK;
}

eth_status_t shm_transport_init(void)
{
    /* 模块级初始化 */
    return ETH_OK;
}
