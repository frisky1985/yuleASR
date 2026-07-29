#ifndef MBEDTLS_CHECK_CONFIG_H
#define MBEDTLS_CHECK_CONFIG_H
#if defined(MBEDTLS_AES_C) && !defined(MBEDTLS_CIPHER_C)
#error "MBEDTLS_AES_C defined but MBEDTLS_CIPHER_C not defined"
#endif
#endif
