/**
 * @file memory_leak_detector.c
 * @brief 内存泄漏检测器
 * @version 1.0
 * @date 2026-04-25
 * 
 * 用于检测测试过程中的内存泄漏
 * 包含完整的分配/释放追踪和报告功能
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <execinfo.h>
#include "memory_leak_detector.h"

/*==============================================================================
 * 配置宏
 *==============================================================================*/

#define MLD_MAX_ALLOCATIONS     10000
#define MLD_BACKTRACE_DEPTH     10
#define MLD_GUARD_SIZE          8
#define MLD_GUARD_PATTERN       0xDEADBEEF

/*==============================================================================
 * 内部数据结构
 *==============================================================================*/

typedef struct allocation_info {
    void* ptr;                          /* 用户指向的内存地址 */
    void* actual_ptr;                   /* 实际分配地址（包含头部） */
    size_t size;                        /* 请求的大小 */
    size_t actual_size;                 /* 实际分配大小 */
    const char* file;                   /* 分配所在文件 */
    int line;                           /* 分配所在行号 */
    const char* func;                   /* 分配所在函数 */
    void* backtrace[MLD_BACKTRACE_DEPTH]; /* 调用栈 */
    int backtrace_size;
    uint64_t timestamp;                 /* 分配时间戳 */
    pthread_t thread_id;                /* 分配线程ID */
    bool freed;                         /* 是否已释放 */
    struct allocation_info* next;
    struct allocation_info* prev;
} allocation_info_t;

/*==============================================================================
 * 全局状态
 *==============================================================================*/

static struct {
    bool initialized;
    bool active;
    pthread_mutex_t mutex;
    allocation_info_t* allocations;     /* 已分配列表头 */
    allocation_info_t* allocations_tail; /* 已分配列表尾 */
    allocation_info_t* free_list;       /* 空闲节点列表 */
    allocation_info_t pool[MLD_MAX_ALLOCATIONS]; /* 预分配的节点池 */
    
    /* 统计信息 */
    uint64_t total_allocations;
    uint64_t total_frees;
    uint64_t total_bytes_allocated;
    uint64_t total_bytes_freed;
    uint64_t current_bytes_allocated;
    uint64_t peak_bytes_allocated;
    uint64_t failed_allocations;
    
    /* 配置 */
    bool detect_double_free;
    bool detect_buffer_overflow;
    bool collect_backtrace;
    size_t report_threshold;            /* 报告阈值（字节） */
} mld_state = {
    .initialized = false,
    .active = false,
    .detect_double_free = true,
    .detect_buffer_overflow = true,
    .collect_backtrace = true,
    .report_threshold = 0
};

/*==============================================================================
 * 内部函数
 *==============================================================================*/

static uint64_t get_timestamp(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static void write_guard_bytes(void* ptr, size_t size) {
    uint32_t* guard = (uint32_t*)((char*)ptr + size);
    for (size_t i = 0; i < MLD_GUARD_SIZE / sizeof(uint32_t); i++) {
        guard[i] = MLD_GUARD_PATTERN;
    }
}

static bool check_guard_bytes(void* ptr, size_t size) {
    uint32_t* guard = (uint32_t*)((char*)ptr + size);
    for (size_t i = 0; i < MLD_GUARD_SIZE / sizeof(uint32_t); i++) {
        if (guard[i] != MLD_GUARD_PATTERN) {
            return false;
        }
    }
    return true;
}

static allocation_info_t* alloc_info_node(void) {
    if (mld_state.free_list != NULL) {
        allocation_info_t* node = mld_state.free_list;
        mld_state.free_list = node->next;
        memset(node, 0, sizeof(allocation_info_t));
        return node;
    }
    
    /* 从池中查找空闲节点 */
    for (int i = 0; i < MLD_MAX_ALLOCATIONS; i++) {
        if (mld_state.pool[i].ptr == NULL && !mld_state.pool[i].freed) {
            return &mld_state.pool[i];
        }
    }
    
    return NULL;
}

static void free_info_node(allocation_info_t* node) {
    node->next = mld_state.free_list;
    mld_state.free_list = node;
}

static void add_to_allocations(allocation_info_t* info) {
    info->prev = mld_state.allocations_tail;
    info->next = NULL;
    
    if (mld_state.allocations_tail != NULL) {
        mld_state.allocations_tail->next = info;
    } else {
        mld_state.allocations = info;
    }
    mld_state.allocations_tail = info;
}

static void remove_from_allocations(allocation_info_t* info) {
    if (info->prev != NULL) {
        info->prev->next = info->next;
    } else {
        mld_state.allocations = info->next;
    }
    
    if (info->next != NULL) {
        info->next->prev = info->prev;
    } else {
        mld_state.allocations_tail = info->prev;
    }
}

static allocation_info_t* find_allocation(void* ptr) {
    allocation_info_t* curr = mld_state.allocations;
    while (curr != NULL) {
        if (curr->ptr == ptr && !curr->freed) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

/*==============================================================================
 * API实现
 *==============================================================================*/

void mld_init(void) {
    if (mld_state.initialized) {
        return;
    }
    
    pthread_mutex_init(&mld_state.mutex, NULL);
    memset(mld_state.pool, 0, sizeof(mld_state.pool));
    mld_state.allocations = NULL;
    mld_state.allocations_tail = NULL;
    mld_state.free_list = NULL;
    mld_state.initialized = true;
    mld_state.active = true;
}

void mld_deinit(void) {
    if (!mld_state.initialized) {
        return;
    }
    
    mld_active(false);
    pthread_mutex_destroy(&mld_state.mutex);
    mld_state.initialized = false;
}

void mld_active(bool active) {
    pthread_mutex_lock(&mld_state.mutex);
    mld_state.active = active;
    pthread_mutex_unlock(&mld_state.mutex);
}

void mld_configure(bool detect_double_free,
                    bool detect_buffer_overflow,
                    bool collect_backtrace,
                    size_t report_threshold) {
    pthread_mutex_lock(&mld_state.mutex);
    mld_state.detect_double_free = detect_double_free;
    mld_state.detect_buffer_overflow = detect_buffer_overflow;
    mld_state.collect_backtrace = collect_backtrace;
    mld_state.report_threshold = report_threshold;
    pthread_mutex_unlock(&mld_state.mutex);
}

void* mld_malloc(size_t size, const char* file, int line, const char* func) {
    if (!mld_state.active) {
        return malloc(size);
    }
    
    pthread_mutex_lock(&mld_state.mutex);
    
    /* 分配额外内存用于头部和守护字节 */
    size_t actual_size = size + sizeof(allocation_info_t*) + MLD_GUARD_SIZE;
    void* actual_ptr = malloc(actual_size);
    
    if (actual_ptr == NULL) {
        mld_state.failed_allocations++;
        pthread_mutex_unlock(&mld_state.mutex);
        return NULL;
    }
    
    /* 计算用户可用的内存地址 */
    void* user_ptr = (char*)actual_ptr + sizeof(allocation_info_t*);
    
    /* 在实际指针前存储allocation_info指针 */
    allocation_info_t** info_ptr = (allocation_info_t**)actual_ptr;
    
    /* 创建追踪信息 */
    allocation_info_t* info = alloc_info_node();
    if (info == NULL) {
        free(actual_ptr);
        mld_state.failed_allocations++;
        pthread_mutex_unlock(&mld_state.mutex);
        return NULL;
    }
    
    info->ptr = user_ptr;
    info->actual_ptr = actual_ptr;
    info->size = size;
    info->actual_size = actual_size;
    info->file = file;
    info->line = line;
    info->func = func;
    info->timestamp = get_timestamp();
    info->thread_id = pthread_self();
    info->freed = false;
    
    if (mld_state.collect_backtrace) {
        info->backtrace_size = backtrace(info->backtrace, MLD_BACKTRACE_DEPTH);
    }
    
    *info_ptr = info;
    
    /* 写入守护字节 */
    if (mld_state.detect_buffer_overflow) {
        write_guard_bytes(user_ptr, size);
    }
    
    /* 添加到分配列表 */
    add_to_allocations(info);
    
    /* 更新统计 */
    mld_state.total_allocations++;
    mld_state.total_bytes_allocated += size;
    mld_state.current_bytes_allocated += size;
    if (mld_state.current_bytes_allocated > mld_state.peak_bytes_allocated) {
        mld_state.peak_bytes_allocated = mld_state.current_bytes_allocated;
    }
    
    pthread_mutex_unlock(&mld_state.mutex);
    
    return user_ptr;
}

void* mld_calloc(size_t num, size_t size, const char* file, int line, const char* func) {
    size_t total = num * size;
    void* ptr = mld_malloc(total, file, line, func);
    if (ptr != NULL) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void* mld_realloc(void* ptr, size_t new_size, const char* file, int line, const char* func) {
    if (ptr == NULL) {
        return mld_malloc(new_size, file, line, func);
    }
    
    if (new_size == 0) {
        mld_free(ptr, file, line, func);
        return NULL;
    }
    
    /* 获取原有大小 */
    allocation_info_t** info_ptr = (allocation_info_t**)((char*)ptr - sizeof(allocation_info_t*));
    allocation_info_t* info = *info_ptr;
    size_t old_size = info->size;
    
    /* 分配新内存 */
    void* new_ptr = mld_malloc(new_size, file, line, func);
    if (new_ptr == NULL) {
        return NULL;
    }
    
    /* 复制数据 */
    size_t copy_size = (old_size < new_size) ? old_size : new_size;
    memcpy(new_ptr, ptr, copy_size);
    
    /* 释放旧内存 */
    mld_free(ptr, file, line, func);
    
    return new_ptr;
}

void mld_free(void* ptr, const char* file, int line, const char* func) {
    if (ptr == NULL) {
        return;
    }
    
    if (!mld_state.active) {
        free(ptr);
        return;
    }
    
    pthread_mutex_lock(&mld_state.mutex);
    
    /* 获取allocation_info */
    allocation_info_t** info_ptr = (allocation_info_t**)((char*)ptr - sizeof(allocation_info_t*));
    allocation_info_t* info = *info_ptr;
    
    if (info == NULL || info->ptr != ptr) {
        fprintf(stderr, "[MLD] Error: Attempt to free invalid pointer %p at %s:%d\n",
                ptr, file, line);
        pthread_mutex_unlock(&mld_state.mutex);
        return;
    }
    
    if (info->freed) {
        if (mld_state.detect_double_free) {
            fprintf(stderr, "[MLD] Error: Double free detected for %p at %s:%d\n",
                    ptr, file, line);
            fprintf(stderr, "       Originally freed at %s:%d\n",
                    info->file, info->line);
        }
        pthread_mutex_unlock(&mld_state.mutex);
        return;
    }
    
    /* 检查缓冲区溢出 */
    if (mld_state.detect_buffer_overflow) {
        if (!check_guard_bytes(ptr, info->size)) {
            fprintf(stderr, "[MLD] Error: Buffer overflow detected for allocation at %s:%d\n",
                    info->file, info->line);
        }
    }
    
    /* 标记为已释放 */
    info->freed = true;
    info->file = file;  /* 记录释放位置 */
    info->line = line;
    
    /* 从分配列表移除 */
    remove_from_allocations(info);
    free_info_node(info);
    
    /* 更新统计 */
    mld_state.total_frees++;
    mld_state.total_bytes_freed += info->size;
    mld_state.current_bytes_allocated -= info->size;
    
    /* 释放实际内存 */
    free(info->actual_ptr);
    
    pthread_mutex_unlock(&mld_state.mutex);
}

void mld_check_leaks(void) {
    pthread_mutex_lock(&mld_state.mutex);
    
    int leak_count = 0;
    size_t leak_bytes = 0;
    
    allocation_info_t* curr = mld_state.allocations;
    while (curr != NULL) {
        if (!curr->freed) {
            leak_count++;
            leak_bytes += curr->size;
        }
        curr = curr->next;
    }
    
    pthread_mutex_unlock(&mld_state.mutex);
    
    if (leak_count > 0) {
        printf("\n[MLD] WARNING: %d memory leaks detected (%zu bytes)\n", 
               leak_count, leak_bytes);
    }
}

void mld_print_report(FILE* fp) {
    if (fp == NULL) {
        fp = stdout;
    }
    
    pthread_mutex_lock(&mld_state.mutex);
    
    fprintf(fp, "\n");
    fprintf(fp, "============================================================\n");
    fprintf(fp, "           Memory Leak Detector Report                      \n");
    fprintf(fp, "============================================================\n");
    
    fprintf(fp, "\n--- Statistics ---\n");
    fprintf(fp, "Total Allocations:      %lu\n", (unsigned long)mld_state.total_allocations);
    fprintf(fp, "Total Frees:            %lu\n", (unsigned long)mld_state.total_frees);
    fprintf(fp, "Total Bytes Allocated:  %lu\n", (unsigned long)mld_state.total_bytes_allocated);
    fprintf(fp, "Total Bytes Freed:      %lu\n", (unsigned long)mld_state.total_bytes_freed);
    fprintf(fp, "Current Bytes Used:     %lu\n", (unsigned long)mld_state.current_bytes_allocated);
    fprintf(fp, "Peak Bytes Used:        %lu\n", (unsigned long)mld_state.peak_bytes_allocated);
    fprintf(fp, "Failed Allocations:     %lu\n", (unsigned long)mld_state.failed_allocations);
    
    int leak_count = 0;
    size_t leak_bytes = 0;
    
    allocation_info_t* curr = mld_state.allocations;
    while (curr != NULL) {
        if (!curr->freed) {
            leak_count++;
            leak_bytes += curr->size;
        }
        curr = curr->next;
    }
    
    fprintf(fp, "\n--- Leak Summary ---\n");
    fprintf(fp, "Active Allocations:     %d\n", leak_count);
    fprintf(fp, "Leaked Bytes:           %zu\n", leak_bytes);
    
    if (leak_count > 0) {
        fprintf(fp, "\n--- Leak Details ---\n");
        curr = mld_state.allocations;
        int shown = 0;
        while (curr != NULL && shown < 20) {
            if (!curr->freed) {
                fprintf(fp, "\n[%d] %p (%zu bytes)\n", shown + 1, curr->ptr, curr->size);
                fprintf(fp, "    Allocated at: %s:%d (%s)\n",
                        curr->file ? curr->file : "unknown",
                        curr->line,
                        curr->func ? curr->func : "unknown");
                
                if (mld_state.collect_backtrace && curr->backtrace_size > 0) {
                    fprintf(fp, "    Backtrace:\n");
                    char** symbols = backtrace_symbols(curr->backtrace, curr->backtrace_size);
                    for (int i = 0; i < curr->backtrace_size && i < 5; i++) {
                        fprintf(fp, "        %s\n", symbols[i]);
                    }
                    free(symbols);
                }
                shown++;
            }
            curr = curr->next;
        }
        
        if (leak_count > 20) {
            fprintf(fp, "\n... and %d more leaks\n", leak_count - 20);
        }
    }
    
    fprintf(fp, "\n============================================================\n");
    
    pthread_mutex_unlock(&mld_state.mutex);
}

void mld_reset(void) {
    pthread_mutex_lock(&mld_state.mutex);
    
    /* 释放所有活动分配 */
    allocation_info_t* curr = mld_state.allocations;
    while (curr != NULL) {
        if (!curr->freed) {
            free(curr->actual_ptr);
        }
        curr = curr->next;
    }
    
    /* 重置状态 */
    memset(mld_state.pool, 0, sizeof(mld_state.pool));
    mld_state.allocations = NULL;
    mld_state.allocations_tail = NULL;
    mld_state.free_list = NULL;
    mld_state.total_allocations = 0;
    mld_state.total_frees = 0;
    mld_state.total_bytes_allocated = 0;
    mld_state.total_bytes_freed = 0;
    mld_state.current_bytes_allocated = 0;
    mld_state.peak_bytes_allocated = 0;
    mld_state.failed_allocations = 0;
    
    pthread_mutex_unlock(&mld_state.mutex);
}

mld_stats_t mld_get_stats(void) {
    mld_stats_t stats;
    pthread_mutex_lock(&mld_state.mutex);
    stats.total_allocations = mld_state.total_allocations;
    stats.total_frees = mld_state.total_frees;
    stats.total_bytes_allocated = mld_state.total_bytes_allocated;
    stats.total_bytes_freed = mld_state.total_bytes_freed;
    stats.current_bytes_allocated = mld_state.current_bytes_allocated;
    stats.peak_bytes_allocated = mld_state.peak_bytes_allocated;
    stats.failed_allocations = mld_state.failed_allocations;
    pthread_mutex_unlock(&mld_state.mutex);
    return stats;
}

/*==============================================================================
 * 使用宏实现
 *==============================================================================*/

void* test_malloc(size_t size) {
    return mld_malloc(size, "unknown", 0, "test_malloc");
}

void test_free(void* ptr) {
    mld_free(ptr, "unknown", 0, "test_free");
}

void* test_calloc(size_t num, size_t size) {
    return mld_calloc(num, size, "unknown", 0, "test_calloc");
}

void* test_realloc(void* ptr, size_t size) {
    return mld_realloc(ptr, size, "unknown", 0, "test_realloc");
}
