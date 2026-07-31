/*==================================================================================================
 * aes.h - mbedTLS AES stub (yuleASR)
 *================================================================================================*/
#ifndef MBEDTLS_AES_H
#define MBEDTLS_AES_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    MBEDTLS_CIPHER_ID_NONE = 0,
    MBEDTLS_CIPHER_ID_NULL,
    MBEDTLS_CIPHER_ID_AES,
    MBEDTLS_CIPHER_ID_DES,
    MBEDTLS_CIPHER_ID_3DES,
    MBEDTLS_CIPHER_ID_CAMELLIA,
    MBEDTLS_CIPHER_ID_BLOWFISH,
    MBEDTLS_CIPHER_ID_ARC4,
    MBEDTLS_CIPHER_ID_CHACHA20
} mbedtls_cipher_id_t;

#endif /* MBEDTLS_AES_H */
