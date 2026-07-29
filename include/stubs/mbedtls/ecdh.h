/**
 * @file ecdh.h
 * @brief mbedTLS ECDH wrapper - stub for compilation
 */
#ifndef MBEDTLS_ECDH_H
#define MBEDTLS_ECDH_H

#include "Std_Types.h"

/* ECDH context */
typedef struct mbedtls_ecdh_context {
    void* grp;
    void* d;
    void* Q;
    void* Qp;
    void* z;
    int point_format;
} mbedtls_ecdh_context;

/* Key agreement roles */
#define MBEDTLS_ECDH_ROLE_NONE      0
#define MBEDTLS_ECDH_ROLE_CLIENT    1
#define MBEDTLS_ECDH_ROLE_SERVER    2

/* Function stubs */
extern void mbedtls_ecdh_init(mbedtls_ecdh_context* ctx);
extern void mbedtls_ecdh_free(mbedtls_ecdh_context* ctx);
extern int mbedtls_ecdh_make_params(mbedtls_ecdh_context* ctx, size_t* olen,
    unsigned char* buf, size_t blen, void* f_rng, void* p_rng);
extern int mbedtls_ecdh_read_params(mbedtls_ecdh_context* ctx,
    const unsigned char** buf, const unsigned char* end);
extern int mbedtls_ecdh_make_public(mbedtls_ecdh_context* ctx, size_t* olen,
    unsigned char* buf, size_t blen, void* f_rng, void* p_rng);
extern int mbedtls_ecdh_read_public(mbedtls_ecdh_context* ctx,
    const unsigned char* buf, size_t blen);
extern int mbedtls_ecdh_calc_secret(mbedtls_ecdh_context* ctx, size_t* olen,
    unsigned char* buf, size_t blen, void* f_rng, void* p_rng);

#endif /* MBEDTLS_ECDH_H */
