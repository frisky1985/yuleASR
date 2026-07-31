/*==================================================================================================
 * md.h - mbedTLS message digest stub (yuleASR)
 *================================================================================================*/
#ifndef MBEDTLS_MD_H
#define MBEDTLS_MD_H

#include <stddef.h>

typedef enum {
    MBEDTLS_MD_NONE = 0,
    MBEDTLS_MD_MD2,
    MBEDTLS_MD_MD4,
    MBEDTLS_MD_MD5,
    MBEDTLS_MD_SHA1,
    MBEDTLS_MD_SHA224,
    MBEDTLS_MD_SHA256,
    MBEDTLS_MD_SHA384,
    MBEDTLS_MD_SHA512,
    MBEDTLS_MD_RIPEMD160
} mbedtls_md_type_t;

typedef struct mbedtls_md_info_t {
    mbedtls_md_type_t type;
    unsigned char size;
} mbedtls_md_info_t;

const mbedtls_md_info_t *mbedtls_md_info_from_type(mbedtls_md_type_t md_type);
int mbedtls_md_hmac(const mbedtls_md_info_t *md_info,
                    const unsigned char *key, size_t keylen,
                    const unsigned char *input, size_t ilen,
                    unsigned char *output);

#endif /* MBEDTLS_MD_H */
