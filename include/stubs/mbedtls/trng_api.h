#ifndef MBEDTLS_TRNG_API_H
#define MBEDTLS_TRNG_API_H
#include "Std_Types.h"
typedef struct mbedtls_trng_context { int initialized; } mbedtls_trng_context;
extern int mbedtls_trng_init(mbedtls_trng_context* ctx);
extern void mbedtls_trng_free(mbedtls_trng_context* ctx);
extern int mbedtls_trng_read(mbedtls_trng_context* ctx, unsigned char* output, size_t len);
#endif
