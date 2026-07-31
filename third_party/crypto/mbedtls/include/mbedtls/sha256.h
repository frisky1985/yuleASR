/*==================================================================================================
 * sha256.h - mbedTLS SHA-256 stub (yuleASR)
 *================================================================================================*/
#ifndef MBEDTLS_SHA256_H
#define MBEDTLS_SHA256_H

#include <stddef.h>
#include <stdint.h>

int mbedtls_sha256_ret(const unsigned char *input, size_t ilen,
                       unsigned char output[32], int is224);

#endif /* MBEDTLS_SHA256_H */
