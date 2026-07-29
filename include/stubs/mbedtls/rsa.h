/**
 * @file rsa.h
 * @brief mbedTLS RSA wrapper - stub for compilation
 */
#ifndef MBEDTLS_RSA_H
#define MBEDTLS_RSA_H

#include "Std_Types.h"

#define MBEDTLS_RSA_PUBLIC      0
#define MBEDTLS_RSA_PRIVATE     1

#define MBEDTLS_ERR_RSA_BAD_INPUT_DATA           -0x4080
#define MBEDTLS_ERR_RSA_INVALID_PADDING          -0x4100
#define MBEDTLS_ERR_RSA_KEY_GEN_FAILED           -0x4180
#define MBEDTLS_ERR_RSA_KEY_CHECK_FAILED         -0x4200
#define MBEDTLS_ERR_RSA_PUBLIC_FAILED            -0x4280
#define MBEDTLS_ERR_RSA_PRIVATE_FAILED           -0x4300
#define MBEDTLS_ERR_RSA_VERIFY_FAILED            -0x4400

struct mbedtls_rsa_context {
    int ver;
    size_t len;
    int padding;
    int hash_id;
};

extern void mbedtls_rsa_init(struct mbedtls_rsa_context* ctx, int padding, int hash_id);
extern void mbedtls_rsa_free(struct mbedtls_rsa_context* ctx);
extern int mbedtls_rsa_pkcs1_encrypt(struct mbedtls_rsa_context* ctx, void* f_rng, void* p_rng, int mode, size_t ilen, const unsigned char* input, unsigned char* output);
extern int mbedtls_rsa_pkcs1_decrypt(struct mbedtls_rsa_context* ctx, void* f_rng, void* p_rng, int mode, size_t* olen, const unsigned char* input, unsigned char* output, size_t output_max_len);
extern int mbedtls_rsa_pkcs1_sign(struct mbedtls_rsa_context* ctx, void* f_rng, void* p_rng, int mode, int md_alg, unsigned int hashlen, const unsigned char* hash, unsigned char* sig);
extern int mbedtls_rsa_pkcs1_verify(struct mbedtls_rsa_context* ctx, void* f_rng, void* p_rng, int mode, int md_alg, unsigned int hashlen, const unsigned char* hash, const unsigned char* sig);

#endif /* MBEDTLS_RSA_H */
