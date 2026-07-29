/**
 * @file ctr_drbg.h
 * @brief mbedTLS CTR_DRBG wrapper - stub for compilation
 */
#ifndef MBEDTLS_CTR_DRBG_H
#define MBEDTLS_CTR_DRBG_H

#include "Std_Types.h"
#include "mbedtls/aes.h"

#define MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED   -0x0034
#define MBEDTLS_ERR_CTR_DRBG_REQUEST_TOO_BIG         -0x0036
#define MBEDTLS_ERR_CTR_DRBG_INPUT_TOO_BIG           -0x0038
#define MBEDTLS_ERR_CTR_DRBG_FILE_IO_ERROR           -0x003A

#define MBEDTLS_CTR_DRBG_BLOCKSIZE           16
#define MBEDTLS_CTR_DRBG_KEYSIZE             32
#define MBEDTLS_CTR_DRBG_KEYBITS             (MBEDTLS_CTR_DRBG_KEYSIZE * 8)
#define MBEDTLS_CTR_DRBG_SEEDLEN             (MBEDTLS_CTR_DRBG_KEYSIZE + MBEDTLS_CTR_DRBG_BLOCKSIZE)
#define MBEDTLS_CTR_DRBG_MAX_REQUEST         1024
#define MBEDTLS_CTR_DRBG_MAX_INPUT           384
#define MBEDTLS_CTR_DRBG_MAX_SEED_INPUT      384

typedef struct mbedtls_ctr_drbg_context {
    unsigned char counter[16];
    int reseed_counter;
    int prediction_resistance;
    size_t entropy_len;
    int reseed_interval;
    mbedtls_aes_context aes_ctx;
    int (*f_entropy)(void*, unsigned char*, size_t);
    void* p_entropy;
} mbedtls_ctr_drbg_context;

extern void mbedtls_ctr_drbg_init(mbedtls_ctr_drbg_context* ctx);
extern void mbedtls_ctr_drbg_free(mbedtls_ctr_drbg_context* ctx);
extern int mbedtls_ctr_drbg_seed(mbedtls_ctr_drbg_context* ctx,
    int (*f_entropy)(void*, unsigned char*, size_t), void* p_entropy,
    const unsigned char* custom, size_t len);
extern int mbedtls_ctr_drbg_random(void* p_rng, unsigned char* output, size_t output_len);
extern int mbedtls_ctr_drbg_reseed(mbedtls_ctr_drbg_context* ctx,
    const unsigned char* additional, size_t len);
extern int mbedtls_ctr_drbg_update_ret(mbedtls_ctr_drbg_context* ctx,
    const unsigned char* additional, size_t add_len);

#endif /* MBEDTLS_CTR_DRBG_H */
