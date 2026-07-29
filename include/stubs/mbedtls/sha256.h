/**
 * @file sha256.h
 * @brief mbedTLS SHA-256 wrapper - stub for compilation
 */
#ifndef MBEDTLS_SHA256_H
#define MBEDTLS_SHA256_H

#include "Std_Types.h"

#define MBEDTLS_ERR_SHA256_HW_ACCEL_FAILED      -0x0037

#define MBEDTLS_SHA256_BLOCK_SIZE  64
#define MBEDTLS_SHA256_DIGEST_SIZE 32

typedef struct mbedtls_sha256_context {
    uint32_t total[2];
    uint32_t state[8];
    unsigned char buffer[64];
    int is224;
} mbedtls_sha256_context;

extern void mbedtls_sha256_init(mbedtls_sha256_context* ctx);
extern void mbedtls_sha256_free(mbedtls_sha256_context* ctx);
extern void mbedtls_sha256_clone(mbedtls_sha256_context* dst, const mbedtls_sha256_context* src);
extern int mbedtls_sha256_starts(mbedtls_sha256_context* ctx, int is224);
extern int mbedtls_sha256_update(mbedtls_sha256_context* ctx, const unsigned char* input, size_t ilen);
extern int mbedtls_sha256_finish(mbedtls_sha256_context* ctx, unsigned char* output);
extern int mbedtls_sha256(const unsigned char* input, size_t ilen, unsigned char* output, int is224);

#endif /* MBEDTLS_SHA256_H */
