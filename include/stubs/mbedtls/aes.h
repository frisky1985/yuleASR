/**
 * @file aes.h
 * @brief mbedTLS AES wrapper - stub for compilation
 */
#ifndef MBEDTLS_AES_H
#define MBEDTLS_AES_H

#include "Std_Types.h"

#define MBEDTLS_AES_ENCRYPT     1
#define MBEDTLS_AES_DECRYPT     0

#define MBEDTLS_ERR_AES_INVALID_KEY_LENGTH    -0x0020
#define MBEDTLS_ERR_AES_INVALID_INPUT_LENGTH  -0x0022
#define MBEDTLS_ERR_AES_BAD_INPUT_DATA        -0x0024
#define MBEDTLS_ERR_AES_FEATURE_UNAVAILABLE   -0x0026
#define MBEDTLS_ERR_AES_HW_ACCEL_FAILED       -0x0028

/* AES context types */
typedef struct mbedtls_aes_context {
    int nr;                 /* number of rounds */
    uint32_t* rk;           /* AES round keys */
    uint32_t buf[68];       /* unaligned data */
} mbedtls_aes_context;

typedef struct mbedtls_aes_xts_context {
    mbedtls_aes_context tweak;
    mbedtls_aes_context decrypt;
} mbedtls_aes_xts_context;

/* Function stubs */
extern void mbedtls_aes_init(mbedtls_aes_context* ctx);
extern void mbedtls_aes_free(mbedtls_aes_context* ctx);
extern int mbedtls_aes_setkey_enc(mbedtls_aes_context* ctx, const unsigned char* key, unsigned int keybits);
extern int mbedtls_aes_setkey_dec(mbedtls_aes_context* ctx, const unsigned char* key, unsigned int keybits);
extern int mbedtls_aes_crypt_ecb(mbedtls_aes_context* ctx, int mode,
    const unsigned char input[16], unsigned char output[16]);
extern int mbedtls_aes_crypt_cbc(mbedtls_aes_context* ctx, int mode, size_t length,
    unsigned char iv[16], const unsigned char* input, unsigned char* output);
extern int mbedtls_aes_crypt_cfb128(mbedtls_aes_context* ctx, int mode, size_t length,
    size_t* iv_off, unsigned char iv[16], const unsigned char* input, unsigned char* output);
extern int mbedtls_aes_crypt_ctr(mbedtls_aes_context* ctx, size_t length,
    size_t* nc_off, unsigned char nonce_counter[16], unsigned char stream_block[16],
    const unsigned char* input, unsigned char* output);
extern int mbedtls_aes_crypt_gcm(mbedtls_aes_context* ctx, int mode, size_t length,
    const unsigned char* iv, size_t iv_len, const unsigned char* add, size_t add_len,
    const unsigned char* input, unsigned char* output, unsigned char* tag, size_t tag_len);

#endif /* MBEDTLS_AES_H */
