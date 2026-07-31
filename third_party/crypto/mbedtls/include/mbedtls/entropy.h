/*==================================================================================================
 * entropy.h - mbedTLS entropy stub (yuleASR)
 *================================================================================================*/
#ifndef MBEDTLS_ENTROPY_H
#define MBEDTLS_ENTROPY_H

#include <stddef.h>

typedef struct mbedtls_entropy_context {
    unsigned char initialized;
} mbedtls_entropy_context;

void mbedtls_entropy_init(mbedtls_entropy_context *ctx);
int  mbedtls_entropy_func(void *data, unsigned char *output, size_t len);
void mbedtls_entropy_free(mbedtls_entropy_context *ctx);

#endif /* MBEDTLS_ENTROPY_H */
