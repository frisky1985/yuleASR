/**
 * @file dlt_performance.c
 * @brief DLT Performance Optimization Implementation
 */

#include "dlt/dlt_performance.h"
#include <string.h>

/*===========================================================================*/
/* Internal State                                                            */
/*===========================================================================*/

static Dlt_PerformanceStatsType g_perf_stats = {0};
static bool g_perf_enabled = true;

/*===========================================================================*/
/* Memory Pool Implementation                                                */
/*===========================================================================*/

Dlt_ReturnType Dlt_MemPool_Init(Dlt_MemPoolType *pool) {
    if (pool == NULL) {
        return DLT_RETURN_WRONG_PARAMETER;
    }

    memset(pool, 0, sizeof(Dlt_MemPoolType));

    /* Allocate pools */
    pool->pool[DLT_MEMPOOL_SMALL] = calloc(DLT_MEMPOOL_SMALL_COUNT, DLT_MEMPOOL_SMALL_SIZE);
    pool->pool[DLT_MEMPOOL_MEDIUM] = calloc(DLT_MEMPOOL_MEDIUM_COUNT, DLT_MEMPOOL_MEDIUM_SIZE);
    pool->pool[DLT_MEMPOOL_LARGE] = calloc(DLT_MEMPOOL_LARGE_COUNT, DLT_MEMPOOL_LARGE_SIZE);
    pool->pool[DLT_MEMPOOL_XLARGE] = calloc(DLT_MEMPOOL_XLARGE_COUNT, DLT_MEMPOOL_XLARGE_SIZE);

    pool->total[DLT_MEMPOOL_SMALL] = DLT_MEMPOOL_SMALL_COUNT;
    pool->total[DLT_MEMPOOL_MEDIUM] = DLT_MEMPOOL_MEDIUM_COUNT;
    pool->total[DLT_MEMPOOL_LARGE] = DLT_MEMPOOL_LARGE_COUNT;
    pool->total[DLT_MEMPOOL_XLARGE] = DLT_MEMPOOL_XLARGE_COUNT;

    pool->available[DLT_MEMPOOL_SMALL] = DLT_MEMPOOL_SMALL_COUNT;
    pool->available[DLT_MEMPOOL_MEDIUM] = DLT_MEMPOOL_MEDIUM_COUNT;
    pool->available[DLT_MEMPOOL_LARGE] = DLT_MEMPOOL_LARGE_COUNT;
    pool->available[DLT_MEMPOOL_XLARGE] = DLT_MEMPOOL_XLARGE_COUNT;

    return DLT_RETURN_OK;
}

void Dlt_MemPool_DeInit(Dlt_MemPoolType *pool) {
    if (pool == NULL) {
        return;
    }

    for (int i = 0; i < DLT_MEMPOOL_COUNT; i++) {
        if (pool->pool[i] != NULL) {
            free(pool->pool[i]);
            pool->pool[i] = NULL;
        }
    }
}

void* Dlt_MemPool_Allocate(Dlt_MemPoolType *pool, uint16_t size) {
    if (pool == NULL || size == 0) {
        return NULL;
    }

    Dlt_MemPoolSizeType pool_type;
    uint16_t block_size;

    if (size <= DLT_MEMPOOL_SMALL_SIZE) {
        pool_type = DLT_MEMPOOL_SMALL;
        block_size = DLT_MEMPOOL_SMALL_SIZE;
    } else if (size <= DLT_MEMPOOL_MEDIUM_SIZE) {
        pool_type = DLT_MEMPOOL_MEDIUM;
        block_size = DLT_MEMPOOL_MEDIUM_SIZE;
    } else if (size <= DLT_MEMPOOL_LARGE_SIZE) {
        pool_type = DLT_MEMPOOL_LARGE;
        block_size = DLT_MEMPOOL_LARGE_SIZE;
    } else if (size <= DLT_MEMPOOL_XLARGE_SIZE) {
        pool_type = DLT_MEMPOOL_XLARGE;
        block_size = DLT_MEMPOOL_XLARGE_SIZE;
    } else {
        /* Too large for pool, use malloc */
        pool->pool_misses++;
        return malloc(size);
    }

    if (pool->available[pool_type] == 0) {
        pool->pool_misses++;
        return malloc(size);
    }

    /* Simple allocation - find first available block */
    uint8_t *base = (uint8_t *)pool->pool[pool_type];
    uint16_t idx = pool->total[pool_type] - pool->available[pool_type];
    
    pool->available[pool_type]--;
    pool->allocations[pool_type]++;

    return base + (idx * block_size);
}

void Dlt_MemPool_Free(Dlt_MemPoolType *pool, void *ptr, uint16_t size) {
    if (pool == NULL || ptr == NULL) {
        return;
    }

    /* Check if pointer belongs to any pool */
    for (int i = 0; i < DLT_MEMPOOL_COUNT; i++) {
        uint8_t *base = (uint8_t *)pool->pool[i];
        uint16_t block_size = 0;
        
        switch (i) {
            case DLT_MEMPOOL_SMALL: block_size = DLT_MEMPOOL_SMALL_SIZE; break;
            case DLT_MEMPOOL_MEDIUM: block_size = DLT_MEMPOOL_MEDIUM_SIZE; break;
            case DLT_MEMPOOL_LARGE: block_size = DLT_MEMPOOL_LARGE_SIZE; break;
            case DLT_MEMPOOL_XLARGE: block_size = DLT_MEMPOOL_XLARGE_SIZE; break;
        }

        if (ptr >= (void *)base && ptr < (void *)(base + pool->total[i] * block_size)) {
            pool->available[i]++;
            pool->allocations[i]--;
            return;
        }
    }

    /* Not from pool, use free */
    free(ptr);
}

void Dlt_MemPool_GetStats(const Dlt_MemPoolType *pool, Dlt_MemPoolStatsType *stats) {
    if (pool == NULL || stats == NULL) {
        return;
    }

    stats->used_small = pool->total[DLT_MEMPOOL_SMALL] - pool->available[DLT_MEMPOOL_SMALL];
    stats->used_medium = pool->total[DLT_MEMPOOL_MEDIUM] - pool->available[DLT_MEMPOOL_MEDIUM];
    stats->used_large = pool->total[DLT_MEMPOOL_LARGE] - pool->available[DLT_MEMPOOL_LARGE];
    stats->used_xlarge = pool->total[DLT_MEMPOOL_XLARGE] - pool->available[DLT_MEMPOOL_XLARGE];
    stats->total_allocations = 0;
    stats->pool_misses = pool->pool_misses;
}

/*===========================================================================*/
/* Zero-Copy Buffer Implementation                                           */
/*===========================================================================*/

Dlt_ReturnType Dlt_ZeroCopyBuffer_Init(Dlt_ZeroCopyBufferType *buffer,
                                        uint8_t *memory,
                                        uint16_t capacity) {
    if (buffer == NULL || memory == NULL || capacity == 0) {
        return DLT_RETURN_WRONG_PARAMETER;
    }

    buffer->base = memory;
    buffer->capacity = capacity;
    buffer->head = 0;
    buffer->tail = 0;
    buffer->used = 0;
    buffer->wrapping = false;

    return DLT_RETURN_OK;
}

int16_t Dlt_ZeroCopyBuffer_Allocate(Dlt_ZeroCopyBufferType *buffer,
                                     uint16_t size,
                                     Dlt_BufferSliceType *slice) {
    if (buffer == NULL || slice == NULL || size == 0) {
        return -1;
    }

    if (size > buffer->capacity - buffer->used) {
        return -1; /* Not enough space */
    }

    slice->data = &buffer->base[buffer->head];
    slice->offset = buffer->head;

    /* Check if we need to wrap */
    if (buffer->head + size > buffer->capacity) {
        /* Wrap around - data will be non-contiguous */
        slice->length = buffer->capacity - buffer->head;
        slice->contiguous = false;
    } else {
        slice->length = size;
        slice->contiguous = true;
    }

    return slice->length;
}

Dlt_ReturnType Dlt_ZeroCopyBuffer_Commit(Dlt_ZeroCopyBufferType *buffer,
                                          uint16_t actual_size) {
    if (buffer == NULL || actual_size == 0) {
        return DLT_RETURN_WRONG_PARAMETER;
    }

    buffer->head = (buffer->head + actual_size) % buffer->capacity;
    buffer->used += actual_size;

    if (buffer->head < buffer->tail) {
        buffer->wrapping = true;
    }

    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_ZeroCopyBuffer_Peek(Dlt_ZeroCopyBufferType *buffer,
                                        Dlt_BufferSliceType *slice) {
    if (buffer == NULL || slice == NULL) {
        return DLT_RETURN_WRONG_PARAMETER;
    }

    if (buffer->used == 0) {
        return DLT_RETURN_ERROR;
    }

    slice->data = &buffer->base[buffer->tail];
    slice->offset = buffer->tail;

    if (buffer->wrapping) {
        slice->length = buffer->capacity - buffer->tail;
        slice->contiguous = false;
    } else {
        slice->length = buffer->head - buffer->tail;
        slice->contiguous = true;
    }

    return DLT_RETURN_OK;
}

Dlt_ReturnType Dlt_ZeroCopyBuffer_Consume(Dlt_ZeroCopyBufferType *buffer,
                                           uint16_t size) {
    if (buffer == NULL || size == 0) {
        return DLT_RETURN_WRONG_PARAMETER;
    }

    if (size > buffer->used) {
        return DLT_RETURN_ERROR;
    }

    buffer->tail = (buffer->tail + size) % buffer->capacity;
    buffer->used -= size;

    if (buffer->tail <= buffer->head) {
        buffer->wrapping = false;
    }

    return DLT_RETURN_OK;
}

/*===========================================================================*/
/* Lock-Free Queue Implementation                                            */
/*===========================================================================*/

void Dlt_LockFreeQueue_Init(Dlt_LockFreeQueueType *queue) {
    if (queue == NULL) {
        return;
    }

    queue->head = 0;
    queue->tail = 0;
    queue->dropped = 0;
    memset((void *)queue->buffer, 0, sizeof(queue->buffer));
}

bool Dlt_LockFreeQueue_Enqueue(Dlt_LockFreeQueueType *queue, void *item) {
    if (queue == NULL || item == NULL) {
        return false;
    }

    uint32_t head = queue->head;
    uint32_t next = (head + 1) & (DLT_QUEUE_SIZE - 1);

    if (next == queue->tail) {
        /* Queue full */
        queue->dropped++;
        return false;
    }

    queue->buffer[head & (DLT_QUEUE_SIZE - 1)] = item;
    queue->head = next;

    return true;
}

void* Dlt_LockFreeQueue_Dequeue(Dlt_LockFreeQueueType *queue) {
    if (queue == NULL) {
        return NULL;
    }

    uint32_t tail = queue->tail;

    if (tail == queue->head) {
        /* Queue empty */
        return NULL;
    }

    void *item = queue->buffer[tail & (DLT_QUEUE_SIZE - 1)];
    queue->tail = (tail + 1) & (DLT_QUEUE_SIZE - 1);

    return item;
}

/*===========================================================================*/
/* Performance Statistics                                                    */
/*===========================================================================*/

void Dlt_Performance_GetStats(Dlt_PerformanceStatsType *stats) {
    if (stats == NULL) {
        return;
    }

    memcpy(stats, &g_perf_stats, sizeof(Dlt_PerformanceStatsType));
}

void Dlt_Performance_ResetStats(void) {
    memset(&g_perf_stats, 0, sizeof(g_perf_stats));
}

void Dlt_Performance_SetEnabled(bool enabled) {
    g_perf_enabled = enabled;
}
