#ifndef MBEDTLS_CIPHER_H
#define MBEDTLS_CIPHER_H
#include "Std_Types.h"
#define MBEDTLS_CIPHER_AES_128_ECB 1
#define MBEDTLS_CIPHER_AES_128_CBC 2
#define MBEDTLS_CIPHER_AES_128_GCM 3
typedef struct mbedtls_cipher_context_t { void* cipher_info; } mbedtls_cipher_context_t;
extern void mbedtls_cipher_init(mbedtls_cipher_context_t* ctx);
extern void mbedtls_cipher_free(mbedtls_cipher_context_t* ctx);
extern int mbedtls_cipher_setup(mbedtls_cipher_context_t* ctx, int info);
extern int mbedtls_cipher_setkey(mbedtls_cipher_context_t* ctx, const unsigned char* key, int key_bitlen, int direction);
extern int mbedtls_cipher_crypt(mbedtls_cipher_context_t* ctx, const unsigned char* iv, size_t iv_len, const unsigned char* input, size_t ilen, unsigned char* output, size_t* olen);
#endif
