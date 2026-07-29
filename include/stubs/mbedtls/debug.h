/**
 * @file debug.h
 * @brief mbedTLS Debug wrapper - stub for compilation
 */
#ifndef MBEDTLS_DEBUG_H
#define MBEDTLS_DEBUG_H

#include "Std_Types.h"

/* Debug threshold levels */
#define MBEDTLS_DEBUG_LEVEL_NONE    0
#define MBEDTLS_DEBUG_LEVEL_ERROR   1
#define MBEDTLS_DEBUG_LEVEL_WARN    2
#define MBEDTLS_DEBUG_LEVEL_INFO    3
#define MBEDTLS_DEBUG_LEVEL_VERBOSE 4

/* Debug function type */
typedef void (*mbedtls_debug_print_func)(void* ctx, int level, const char* file, int line, const char* str);

/* Debug functions */
extern void mbedtls_debug_set_threshold(int threshold);

static inline void mbedtls_debug_print_msg(void* ctx, int level, const char* file, int line, const char* format, ...) {
    (void)ctx; (void)level; (void)file; (void)line; (void)format;
}

static inline void mbedtls_debug_print_ret(void* ctx, int level, const char* file, int line, const char* text, int ret) {
    (void)ctx; (void)level; (void)file; (void)line; (void)text; (void)ret;
}

static inline void mbedtls_debug_print_buf(void* ctx, int level, const char* file, int line, const char* text, const unsigned char* buf, size_t len) {
    (void)ctx; (void)level; (void)file; (void)line; (void)text; (void)buf; (void)len;
}

/* Debug macro - disabled by default */
#ifndef MBEDTLS_DEBUG_C
#define DEBUG(...)
#else
#define DEBUG(...) mbedtls_debug_print_msg(__VA_ARGS__)
#endif

#endif /* MBEDTLS_DEBUG_H */
