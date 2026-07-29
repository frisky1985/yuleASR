/**
 * @file ecdsa.h
 * @brief mbedTLS ECDSA wrapper - stub for compilation
 */
#ifndef MBEDTLS_ECDSA_H
#define MBEDTLS_ECDSA_H

#include "Std_Types.h"

/* ECDSA context */
typedef struct mbedtls_ecdsa_context {
    void* grp;
    void* Q;
    void* d;
} mbedtls_ecdsa_context;

/* ECDSA signature */
typedef struct mbedtls_ecdsa_signature {
    void* r;
    void* s;
} mbedtls_ecdsa_signature;

/* Function stubs */
extern void mbedtls_ecdsa_init(mbedtls_ecdsa_context* ctx);
extern void mbedtls_ecdsa_free(mbedtls_ecdsa_context* ctx);
extern int mbedtls_ecdsa_from_keypair(mbedtls_ecdsa_context* ctx, const void* key);
extern int mbedtls_ecdsa_sign(mbedtls_ecdsa_context* ctx, void* r, void* s,
    const unsigned char* hash, size_t hlen, void* f_rng, void* p_rng);
extern int mbedtls_ecdsa_verify(mbedtls_ecdsa_context* ctx, const unsigned char* hash,
    size_t hlen, const void* r, const void* s);
extern int mbedtls_ecdsa_write_signature(mbedtls_ecdsa_context* ctx, int md_alg,
    const unsigned char* hash, size_t hlen, unsigned char* sig, size_t* siglen,
    void* f_rng, void* p_rng);
extern int mbedtls_ecdsa_read_signature(mbedtls_ecdsa_context* ctx,
    const unsigned char* hash, size_t hlen, const unsigned char* sig, size_t siglen);

#endif /* MBEDTLS_ECDSA_H */
