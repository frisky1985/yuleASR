/**
 * @file pk.h
 * @brief mbedTLS Public Key wrapper - stub for compilation
 */
#ifndef MBEDTLS_PK_H
#define MBEDTLS_PK_H

#include "Std_Types.h"

#define MBEDTLS_ERR_PK_ALLOC_FAILED             -0x3F80
#define MBEDTLS_ERR_PK_TYPE_MISMATCH            -0x3F00
#define MBEDTLS_ERR_PK_BAD_INPUT_DATA           -0x3E80
#define MBEDTLS_ERR_PK_FILE_IO_ERROR            -0x3E00
#define MBEDTLS_ERR_PK_KEY_INVALID_VERSION      -0x3D80
#define MBEDTLS_ERR_PK_KEY_INVALID_FORMAT       -0x3D00
#define MBEDTLS_ERR_PK_UNKNOWN_PK_ALG           -0x3C80
#define MBEDTLS_ERR_PK_INVALID_PUBKEY           -0x3C00
#define MBEDTLS_ERR_PK_INVALID_ALG              -0x3B80
#define MBEDTLS_ERR_PK_SIG_LEN_MISMATCH         -0x3B00

/* PK types */
#define MBEDTLS_PK_RSA          0
#define MBEDTLS_PK_ECKEY        1
#define MBEDTLS_PK_ECKEY_DH     2
#define MBEDTLS_PK_ECDSA        3
#define MBEDTLS_PK_RSA_ALT      4
#define MBEDTLS_PK_RSASSA_PSS   5

/* PK info */
typedef struct mbedtls_pk_info_t mbedtls_pk_info_t;

/* PK context */
typedef struct mbedtls_pk_context {
    const mbedtls_pk_info_t* pk_info;
    void* pk_ctx;
} mbedtls_pk_context;

/* PK RSA alt */
typedef struct mbedtls_rsa_context mbedtls_rsa_context;

/* PK functions */
extern void mbedtls_pk_init(mbedtls_pk_context* ctx);
extern void mbedtls_pk_free(mbedtls_pk_context* ctx);
extern int mbedtls_pk_setup(mbedtls_pk_context* ctx, const mbedtls_pk_info_t* info);
extern int mbedtls_pk_parse_key(mbedtls_pk_context* ctx, const unsigned char* key, size_t keylen, const unsigned char* pwd, size_t pwdlen);
extern int mbedtls_pk_parse_public_key(mbedtls_pk_context* ctx, const unsigned char* key, size_t keylen);
extern int mbedtls_pk_sign(mbedtls_pk_context* ctx, int md_alg, const unsigned char* hash, size_t hashlen, unsigned char* sig, size_t* siglen, void* f_rng, void* p_rng);
extern int mbedtls_pk_verify(mbedtls_pk_context* ctx, int md_alg, const unsigned char* hash, size_t hashlen, const unsigned char* sig, size_t siglen);
extern int mbedtls_pk_write_pubkey_der(mbedtls_pk_context* ctx, unsigned char* buf, size_t size);

#endif /* MBEDTLS_PK_H */
