#ifndef MBEDTLS_HMAC_DRBG_H
#define MBEDTLS_HMAC_DRBG_H
#include "Std_Types.h"
typedef struct mbedtls_hmac_drbg_context { int md_type; } mbedtls_hmac_drbg_context;
extern void mbedtls_hmac_drbg_init(mbedtls_hmac_drbg_context* ctx);
extern void mbedtls_hmac_drbg_free(mbedtls_hmac_drbg_context* ctx);
extern int mbedtls_hmac_drbg_seed(mbedtls_hmac_drbg_context* ctx, int md_type, const unsigned char* custom, size_t len);
extern int mbedtls_hmac_drbg_random(void* p_rng, unsigned char* output, size_t out_len);
#endif
