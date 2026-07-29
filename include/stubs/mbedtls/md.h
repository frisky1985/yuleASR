/**
 * @file md.h
 * @brief mbedTLS Message Digest wrapper - stub for compilation
 */
#ifndef MBEDTLS_MD_H
#define MBEDTLS_MD_H

#include "Std_Types.h"

/* Message digest types */
#define MBEDTLS_MD_NONE         0
#define MBEDTLS_MD_MD2          1
#define MBEDTLS_MD_MD4          2
#define MBEDTLS_MD_MD5          3
#define MBEDTLS_MD_SHA1         4
#define MBEDTLS_MD_SHA224       5
#define MBEDTLS_MD_SHA256       6
#define MBEDTLS_MD_SHA384       7
#define MBEDTLS_MD_SHA512       8
#define MBEDTLS_MD_RIPEMD160    9

/* Error codes */
#define MBEDTLS_ERR_MD_FEATURE_UNAVAILABLE  -0x5080
#define MBEDTLS_ERR_MD_BAD_INPUT_DATA       -0x5100
#define MBEDTLS_ERR_MD_ALLOC_FAILED         -0x5180
#define MBEDTLS_ERR_MD_FILE_IO_ERROR        -0x5200

/* MD info */
typedef struct mbedtls_md_info_t mbedtls_md_info_t;

/* MD context */
typedef struct mbedtls_md_context {
    const mbedtls_md_info_t* md_info;
    void* md_ctx;
    void* hmac_ctx;
} mbedtls_md_context;

extern const mbedtls_md_info_t* mbedtls_md_info_from_type(int md_type);
extern void mbedtls_md_init(mbedtls_md_context* ctx);
extern void mbedtls_md_free(mbedtls_md_context* ctx);
extern int mbedtls_md_setup(mbedtls_md_context* ctx, const mbedtls_md_info_t* md_info, int hmac);
extern int mbedtls_md_starts(mbedtls_md_context* ctx);
extern int mbedtls_md_update(mbedtls_md_context* ctx, const unsigned char* input, size_t ilen);
extern int mbedtls_md_finish(mbedtls_md_context* ctx, unsigned char* output);
extern int mbedtls_md(const mbedtls_md_info_t* md_info, const unsigned char* input, size_t ilen, unsigned char* output);
extern int mbedtls_md_hmac(mbedtls_md_context* ctx, const unsigned char* key, size_t keylen, const unsigned char* input, size_t ilen, unsigned char* output);

#endif /* MBEDTLS_MD_H */
