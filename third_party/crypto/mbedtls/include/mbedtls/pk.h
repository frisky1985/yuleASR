/*==================================================================================================
 * pk.h - mbedTLS public key stub (yuleASR)
 *================================================================================================*/
#ifndef MBEDTLS_PK_H
#define MBEDTLS_PK_H

#include <stddef.h>
#include "mbedtls/x509.h"

typedef enum {
    MBEDTLS_PK_NONE = 0,
    MBEDTLS_PK_RSA,
    MBEDTLS_PK_ECKEY,
    MBEDTLS_PK_ECKEY_DH,
    MBEDTLS_PK_ECDSA,
    MBEDTLS_PK_RSA_ALT,
    MBEDTLS_PK_RSASSA_PSS
} mbedtls_pk_type_t;

typedef struct mbedtls_pk_context {
    mbedtls_pk_type_t pk_type;
    void *pk_ctx;
} mbedtls_pk_context;

void mbedtls_pk_init(mbedtls_pk_context *ctx);
void mbedtls_pk_free(mbedtls_pk_context *ctx);
int  mbedtls_pk_parse_key(mbedtls_pk_context *ctx,
                          const unsigned char *key, size_t keylen,
                          const unsigned char *pwd, size_t pwdlen,
                          int (*f_rng)(void *, unsigned char *, size_t),
                          void *p_rng);

#endif /* MBEDTLS_PK_H */
