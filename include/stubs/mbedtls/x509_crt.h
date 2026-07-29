/**
 * @file x509_crt.h
 * @brief mbedTLS X.509 Certificate wrapper - stub for compilation
 */
#ifndef MBEDTLS_X509_CRT_H
#define MBEDTLS_X509_CRT_H

#include "Std_Types.h"

/* mbedTLS X.509 certificate types */
typedef struct mbedtls_x509_crt {
    void* opaque;
} mbedtls_x509_crt;

typedef struct mbedtls_x509_crt_profile {
    uint32 allowed_mds;
    uint32 allowed_pks;
    uint32 allowed_curves;
    uint32 rsa_min_bit_size;
} mbedtls_x509_crt_profile;

typedef struct mbedtls_mpi {
    void* p;
} mbedtls_mpi;

typedef struct mbedtls_pk_context {
    void* pk_info;
    void* pk_ctx;
} mbedtls_pk_context;

/* Function stubs */
extern void mbedtls_x509_crt_init(mbedtls_x509_crt* crt);
extern void mbedtls_x509_crt_free(mbedtls_x509_crt* crt);
extern int mbedtls_x509_crt_parse(mbedtls_x509_crt* crt, const unsigned char* buf, size_t buflen);
extern int mbedtls_x509_crt_parse_der(mbedtls_x509_crt* crt, const unsigned char* buf, size_t buflen);
extern int mbedtls_x509_crt_parse_file(mbedtls_x509_crt* crt, const char* path);

#endif /* MBEDTLS_X509_CRT_H */
