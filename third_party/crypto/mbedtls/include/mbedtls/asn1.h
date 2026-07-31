/*==================================================================================================
 * asn1.h - mbedTLS ASN.1 stub (yuleASR)
 *================================================================================================*/
#ifndef MBEDTLS_ASN1_H
#define MBEDTLS_ASN1_H

#include <stddef.h>
#include <stdint.h>

typedef struct mbedtls_asn1_buf {
    int tag;
    unsigned char *p;
    size_t len;
} mbedtls_asn1_buf;

typedef struct mbedtls_asn1_sequence {
    mbedtls_asn1_buf buf;
    struct mbedtls_asn1_sequence *next;
} mbedtls_asn1_sequence;

typedef struct mbedtls_x509_crl {
    unsigned char *p;
    size_t len;
} mbedtls_x509_crl;

#endif /* MBEDTLS_ASN1_H */
