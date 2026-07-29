/**
 * @file gcm.h
 * @brief mbedTLS GCM mode wrapper - stub for compilation
 */
#ifndef MBEDTLS_GCM_H
#define MBEDTLS_GCM_H

#include "Std_Types.h"

#define MBEDTLS_GCM_ENCRYPT     1
#define MBEDTLS_GCM_DECRYPT     0

#define MBEDTLS_ERR_GCM_AUTH_FAILED               -0x0012
#define MBEDTLS_ERR_GCM_BAD_INPUT                 -0x0014
#define MBEDTLS_ERR_GCM_HW_ACCEL_FAILED           -0x0016

typedef struct mbedtls_gcm_context {
    void* cipher_ctx;
    uint64_t len;
    uint64_t add_len;
    unsigned char base_ectr[16];
    unsigned char y[16];
    unsigned char buf[16];
    int mode;
} mbedtls_gcm_context;

extern void mbedtls_gcm_init(mbedtls_gcm_context* ctx);
extern void mbedtls_gcm_free(mbedtls_gcm_context* ctx);
extern int mbedtls_gcm_setkey(mbedtls_gcm_context* ctx, int cipher,
    const unsigned char* key, unsigned int keybits);
extern int mbedtls_gcm_crypt_and_tag(mbedtls_gcm_context* ctx, int mode, size_t length,
    const unsigned char* iv, size_t iv_len, const unsigned char* add, size_t add_len,
    const unsigned char* input, unsigned char* output, size_t tag_len, unsigned char* tag);
extern int mbedtls_gcm_auth_decrypt(mbedtls_gcm_context* ctx, size_t length,
    const unsigned char* iv, size_t iv_len, const unsigned char* add, size_t add_len,
    const unsigned char* tag, size_t tag_len, const unsigned char* input, unsigned char* output);
extern int mbedtls_gcm_starts(mbedtls_gcm_context* ctx, int mode,
    const unsigned char* iv, size_t iv_len);
extern int mbedtls_gcm_update(mbedtls_gcm_context* ctx, size_t length,
    const unsigned char* input, unsigned char* output);
extern int mbedtls_gcm_finish(mbedtls_gcm_context* ctx, unsigned char* tag, size_t tag_len);

#endif /* MBEDTLS_GCM_H */
