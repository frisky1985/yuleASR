/*==================================================================================================
 * ctr_drbg.h - mbedTLS CTR_DRBG stub (yuleASR)
 *================================================================================================*/
#ifndef MBEDTLS_CTR_DRBG_H
#define MBEDTLS_CTR_DRBG_H

#include <stddef.h>
#include <stdint.h>

typedef struct mbedtls_ctr_drbg_context {
    unsigned char counter[16];
    unsigned char reseed_counter;
    int (*f_entropy)(void *, unsigned char *, size_t);
    void *p_entropy;
} mbedtls_ctr_drbg_context;

void mbedtls_ctr_drbg_init(mbedtls_ctr_drbg_context *ctx);
int  mbedtls_ctr_drbg_seed(mbedtls_ctr_drbg_context *ctx,
                           int (*f_entropy)(void *, unsigned char *, size_t),
                           void *p_entropy, const unsigned char *custom, size_t len);
int  mbedtls_ctr_drbg_random(void *p_rng, unsigned char *output, size_t output_len);
void mbedtls_ctr_drbg_free(mbedtls_ctr_drbg_context *ctx);

#endif /* MBEDTLS_CTR_DRBG_H */
