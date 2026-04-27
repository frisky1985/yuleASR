/**
 * @file dlt_performance.h
 * @brief DLT Performance Optimization Module
 *
 * Zero-copy buffer management, batch processing, and memory pooling
 */

#ifndef DLT_PERFORMANCE_H
#define DLT_PERFORMANCE_H

#include "dlt.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Memory Pool Configuration                                                 */
/*===========================================================================*/

#define DLT_MEMPOOL_SMALL_SIZE    64
#define DLT_MEMPOOL_MEDIUM_SIZE   256
#define DLT_MEMPOOL_LARGE_SIZE    1024
#define DLT_MEMPOOL_XLARGE_SIZE   4096

#define DLT_MEMPOOL_SMALL_COUNT   32
#define DLT_MEMPOOL_MEDIUM_COUNT  16
#define DLT_MEMPOOL_LARGE_COUNT   8
#define DLT_MEMPOOL_XLARGE_COUNT  4

typedef enum {
    DLT_MEMPOOL_SMALL = 0,
    DLT_MEMPOOL_MEDIUM,
    DLT_MEMPOOL_LARGE,
    DLT_MEMPOOL_XLARGE,
    DLT_MEMPOOL_COUNT
} Dlt_MemPoolSizeType;

typedef struct {
    void *pool[DLT_MEMPOOL_COUNT];
    uint16_t available[DLT_MEMPOOL_COUNT];
    uint16_t total[DLT_MEMPOOL_COUNT];
    uint16_t allocations[DLT_MEMPOOL_COUNT];
} Dlt_MemPoolType;

/*===========================================================================*/
/* Zero-Copy Buffer                                                          */
/*===========================================================================*/

typedef struct {
    uint8_t *base;           /* Base memory address */
    uint16_t capacity;       /* Total buffer capacity */
    uint16_t head;           /* Write position */
    uint16_t tail;           /* Read position */
    uint16_t used;           /* Currently used bytes */
    bool wrapping;           /* Circular buffer wrapped */
} Dlt_ZeroCopyBufferType;

typedef struct {
    uint8_t *data;           /* Pointer to data (may be offset) */
    uint16_t length;         /* Data length */
    uint16_t offset;         /* Offset from base */
    bool contiguous;         /* True if data is contiguous */
} Dlt_BufferSliceType;

/*===========================================================================*/
/* Batch Processing                                                          */
/*===========================================================================*/

#define DLT_BATCH_MAX_MESSAGES 16
#define DLT_BATCH_TIMEOUT_MS   10

typedef struct {
    uint8_t *messages[DLT_BATCH_MAX_MESSAGES];
    uint16_t lengths[DLT_BATCH_MAX_MESSAGES];
    uint8_t count;
    uint32_t first_timestamp;
    bool enabled;
} Dlt_BatchQueueType;

/*===========================================================================*/
/* Lock-Free Queue (Single Producer - Single Consumer)                       */
/*===========================================================================*/

#define DLT_QUEUE_SIZE 64

typedef struct {
    volatile uint32_t head;
    volatile uint32_t tail;
    void *buffer[DLT_QUEUE_SIZE];
    volatile uint32_t dropped;
} Dlt_LockFreeQueueType;

/*===========================================================================*/
/* API Functions - Memory Pool                                               */
/*===========================================================================*/

/**
 * @brief Initialize memory pool
 */
Dlt_ReturnType Dlt_MemPool_Init(Dlt_MemPoolType *pool);

/**
 * @brief De-initialize memory pool
 */
void Dlt_MemPool_DeInit(Dlt_MemPoolType *pool);

/**
 * @brief Allocate memory from pool
 */
void* Dlt_MemPool_Allocate(Dlt_MemPoolType *pool, uint16_t size);

/**
 * @brief Free memory back to pool
 */
void Dlt_MemPool_Free(Dlt_MemPoolType *pool, void *ptr, uint16_t size);

/**
 * @brief Get memory pool statistics
 */
typedef struct {
    uint16_t used_small;
    uint16_t used_medium;
    uint16_t used_large;
    uint16_t used_xlarge;
    uint32_t total_allocations;
    uint32_t pool_misses;
} Dlt_MemPoolStatsType;

void Dlt_MemPool_GetStats(const Dlt_MemPoolType *pool, Dlt_MemPoolStatsType *stats);

/*===========================================================================*/
/* API Functions - Zero-Copy Buffer                                          */
/*===========================================================================*/

/**
 * @brief Initialize zero-copy circular buffer
 */
Dlt_ReturnType Dlt_ZeroCopyBuffer_Init(Dlt_ZeroCopyBufferType *buffer,
                                        uint8_t *memory,
                                        uint16_t capacity);

/**
 * @brief Allocate write space in buffer
 */
int16_t Dlt_ZeroCopyBuffer_Allocate(Dlt_ZeroCopyBufferType *buffer,
                                     uint16_t size,
                                     Dlt_BufferSliceType *slice);

/**
 * @brief Commit written data to buffer
 */
Dlt_ReturnType Dlt_ZeroCopyBuffer_Commit(Dlt_ZeroCopyBufferType *buffer,
                                          uint16_t actual_size);

/**
 * @brief Read data from buffer without copying
 */
Dlt_ReturnType Dlt_ZeroCopyBuffer_Peek(Dlt_ZeroCopyBufferType *buffer,
                                        Dlt_BufferSliceType *slice);

/**
 * @brief Consume (remove) data from buffer
 */
Dlt_ReturnType Dlt_ZeroCopyBuffer_Consume(Dlt_ZeroCopyBufferType *buffer,
                                           uint16_t size);

/**
 * @brief Get available space in buffer
 */
static inline uint16_t Dlt_ZeroCopyBuffer_Available(const Dlt_ZeroCopyBufferType *buffer) {
    return buffer->capacity - buffer->used;
}

/*===========================================================================*/
/* API Functions - Batch Processing                                          */
/*===========================================================================*/

/**
 * @brief Initialize batch queue
 */
Dlt_ReturnType Dlt_BatchQueue_Init(Dlt_BatchQueueType *queue, bool enabled);

/**
 * @brief Add message to batch
 */
Dlt_ReturnType Dlt_BatchQueue_Add(Dlt_BatchQueueType *queue,
                                   uint8_t *message,
                                   uint16_t length);

/**
 * @brief Flush batch queue (send all accumulated messages)
 */
Dlt_ReturnType Dlt_BatchQueue_Flush(Dlt_BatchQueueType *queue);

/**
 * @brief Check if batch needs flushing (timeout or full)
 */
bool Dlt_BatchQueue_ShouldFlush(const Dlt_BatchQueueType *queue);

/*===========================================================================*/
/* API Functions - Lock-Free Queue                                           */
/*===========================================================================*/

/**
 * @brief Initialize lock-free queue
 */
void Dlt_LockFreeQueue_Init(Dlt_LockFreeQueueType *queue);

/**
 * @brief Enqueue item (producer only)
 */
bool Dlt_LockFreeQueue_Enqueue(Dlt_LockFreeQueueType *queue, void *item);

/**
 * @brief Dequeue item (consumer only)
 */
void* Dlt_LockFreeQueue_Dequeue(Dlt_LockFreeQueueType *queue);

/**
 * @brief Check if queue is empty
 */
static inline bool Dlt_LockFreeQueue_IsEmpty(const Dlt_LockFreeQueueType *queue) {
    return queue->head == queue->tail;
}

/**
 * @brief Get queue fill count
 */
static inline uint32_t Dlt_LockFreeQueue_Count(const Dlt_LockFreeQueueType *queue) {
    return (queue->head - queue->tail) & (DLT_QUEUE_SIZE - 1);
}

/*===========================================================================*/
/* Performance Monitoring                                                    */
/*===========================================================================*/

typedef struct {
    uint32_t messages_logged;
    uint32_t messages_dropped;
    uint32_t bytes_written;
    uint32_t bytes_dropped;
    uint32_t avg_message_size;
    uint32_t max_message_size;
    uint32_t buffer_high_watermark;
    uint32_t context_switches;
} Dlt_PerformanceStatsType;

/**
 * @brief Get performance statistics
 */
void Dlt_Performance_GetStats(Dlt_PerformanceStatsType *stats);

/**
 * @brief Reset performance statistics
 */
void Dlt_Performance_ResetStats(void);

/**
 * @brief Enable/disable performance monitoring
 */
void Dlt_Performance_SetEnabled(bool enabled);

#ifdef __cplusplus
}
#endif

#endif /* DLT_PERFORMANCE_H"
}

#endif /* DLT_PERFORMANCE_H */
