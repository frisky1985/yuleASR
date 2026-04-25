/**
 * @file memory_leak_detector.h
 * @brief 内存泄漏检测器头文件
 * @version 1.0
 * @date 2026-04-25
 */

#ifndef MEMORY_LEAK_DETECTOR_H
#define MEMORY_LEAK_DETECTOR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 类型定义
 *==============================================================================*/

typedef struct {
    uint64_t total_allocations;
    uint64_t total_frees;
    uint64_t total_bytes_allocated;
    uint64_t total_bytes_freed;
    uint64_t current_bytes_allocated;
    uint64_t peak_bytes_allocated;
    uint64_t failed_allocations;
} mld_stats_t;

/*==============================================================================
 * API声明
 *==============================================================================*/

/* 初始化和终止 */
void mld_init(void);
void mld_deinit(void);
void mld_active(bool active);
void mld_configure(bool detect_double_free,
                    bool detect_buffer_overflow,
                    bool collect_backtrace,
                    size_t report_threshold);

/* 内存管理API（带追踪） */
void* mld_malloc(size_t size, const char* file, int line, const char* func);
void* mld_calloc(size_t num, size_t size, const char* file, int line, const char* func);
void* mld_realloc(void* ptr, size_t new_size, const char* file, int line, const char* func);
void mld_free(void* ptr, const char* file, int line, const char* func);

/* 检查和报告 */
void mld_check_leaks(void);
void mld_print_report(FILE* fp);
void mld_reset(void);
mld_stats_t mld_get_stats(void);

/* 使用宏 */
#define MLD_MALLOC(size)        mld_malloc(size, __FILE__, __LINE__, __func__)
#define MLD_CALLOC(num, size)   mld_calloc(num, size, __FILE__, __LINE__, __func__)
#define MLD_REALLOC(ptr, size)  mld_realloc(ptr, size, __FILE__, __LINE__, __func__)
#define MLD_FREE(ptr)           mld_free(ptr, __FILE__, __LINE__, __func__)

/* Unity兼容函数 */
void* test_malloc(size_t size);
void test_free(void* ptr);
void* test_calloc(size_t num, size_t size);
void* test_realloc(void* ptr, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* MEMORY_LEAK_DETECTOR_H */
