/*==================================================================================================
 * x509.h - mbedTLS X.509 stub (yuleASR)
 *================================================================================================*/
#ifndef MBEDTLS_X509_H
#define MBEDTLS_X509_H

#include <stddef.h>
#include <stdint.h>
#include "mbedtls/bignum.h"
#include "mbedtls/oid.h"
#include "mbedtls/asn1.h"

/* Certificate verification flags */
#define MBEDTLS_X509_BADCERT_EXPIRED        0x01
#define MBEDTLS_X509_BADCERT_REVOKED        0x04
#define MBEDTLS_X509_BADCERT_NOT_TRUSTED    0x08

typedef struct mbedtls_x509_buf {
    unsigned char *p;
    size_t         len;
    int            tag;
} mbedtls_x509_buf;

typedef struct mbedtls_x509_name {
    mbedtls_x509_buf oid;
    mbedtls_x509_buf val;
    struct mbedtls_x509_name *next;
} mbedtls_x509_name;

typedef struct mbedtls_x509_time {
    int year;
    int mon;
    int day;
    int hour;
    int min;
    int sec;
} mbedtls_x509_time;

typedef struct mbedtls_x509_crt {
    mbedtls_x509_buf raw;
    mbedtls_x509_buf tbs;
    mbedtls_x509_buf serial;
    mbedtls_x509_buf sig;
    mbedtls_x509_buf sig_oid;
    mbedtls_x509_name issuer;
    mbedtls_x509_name subject;
    mbedtls_x509_time valid_from;
    mbedtls_x509_time valid_to;
    unsigned char ca_istrue;
    unsigned char key_usage;
    unsigned char ext_key_usage;
    struct mbedtls_x509_crt *next;
} mbedtls_x509_crt;

#endif /* MBEDTLS_X509_H */
