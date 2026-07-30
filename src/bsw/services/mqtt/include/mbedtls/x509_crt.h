/*
 * mbedtls/x509_crt.h - Stub for mbedTLS X.509 certificate handling
 * This is a minimal stub for compilation purposes only.
 * Install mbedTLS for the full implementation.
 */
#ifndef MBEDTLS_X509_CRT_H
#define MBEDTLS_X509_CRT_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

/* Minimal types for compilation */
typedef struct mbedtls_x509_crt {
    int version;
    unsigned char serial[32];
    size_t serial_len;
    struct mbedtls_x509_crt *next;
    struct mbedtls_x509_crt *prev;
} mbedtls_x509_crt;

typedef struct mbedtls_x509_crt_profile {
    uint32_t allowed_mds;
    uint32_t allowed_pks;
    uint32_t allowed_curves;
    uint32_t rsa_min_bitlen;
} mbedtls_x509_crt_profile;

typedef struct mbedtls_md_info_t mbedtls_md_info_t;
typedef struct mbedtls_pk_context mbedtls_pk_context;

/* Certificate verification flags */
#define MBEDTLS_X509_BADCERT_EXPIRED          0x000001
#define MBEDTLS_X509_BADCERT_REVOKED          0x000002
#define MBEDTLS_X509_BADCERT_CN_MISMATCH      0x000004
#define MBEDTLS_X509_BADCERT_NOT_TRUSTED       0x000008
#define MBEDTLS_X509_BADCERT_BAD_KEY           0x000010
#define MBEDTLS_X509_BADCERT_BAD_MD            0x000020
#define MBEDTLS_X509_BADCERT_FUTURE            0x000040
#define MBEDTLS_X509_BADCERT_OTHER             0x000100

/* Profile constants */
extern const mbedtls_x509_crt_profile mbedtls_x509_crt_profile_default;

/* Function stubs */
static inline void mbedtls_x509_crt_init(mbedtls_x509_crt *crt) { (void)crt; }
static inline void mbedtls_x509_crt_free(mbedtls_x509_crt *crt) { (void)crt; }

#endif /* MBEDTLS_X509_CRT_H */
