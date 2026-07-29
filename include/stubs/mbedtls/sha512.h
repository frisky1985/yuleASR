#ifndef MBEDTLS_SHA512_H
#define MBEDTLS_SHA512_H
#include "Std_Types.h"
typedef struct mbedtls_sha512_context { int is384; } mbedtls_sha512_context;
extern void mbedtls_sha512_init(mbedtls_sha512_context* ctx);
extern void mbedtls_sha512_free(mbedtls_sha512_context* ctx);
extern int mbedtls_sha512_starts(mbedtls_sha512_context* ctx, int is384);
extern int mbedtls_sha512_update(mbedtls_sha512_context* ctx, const unsigned char* input, size_t ilen);
extern int mbedtls_sha512_finish(mbedtls_sha512_context* ctx, unsigned char* output);
extern int mbedtls_sha512(const unsigned char* input, size_t ilen, unsigned char* output, int is384);
#endif
