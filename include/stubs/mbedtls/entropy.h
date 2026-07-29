/**
 * @file entropy.h
 * @brief mbedTLS Entropy wrapper - stub for compilation
 */
#ifndef MBEDTLS_ENTROPY_H
#define MBEDTLS_ENTROPY_H

#include "Std_Types.h"

#define MBEDTLS_ERR_ENTROPY_NO_SOURCES        -0x3C80
#define MBEDTLS_ERR_ENTROPY_SOURCE_FAILED     -0x3E80

typedef struct mbedtls_entropy_context {
    int source_count;
    int error;
} mbedtls_entropy_context;

extern void mbedtls_entropy_init(mbedtls_entropy_context* ctx);
extern void mbedtls_entropy_free(mbedtls_entropy_context* ctx);
extern int mbedtls_entropy_func(void* data, unsigned char* output, size_t len);
extern int mbedtls_entropy_add_source(mbedtls_entropy_context* ctx,
    int (*f_source)(void*, unsigned char*, size_t, size_t*),
    void* p_source, size_t threshold, int strong);

#endif /* MBEDTLS_ENTROPY_H */
