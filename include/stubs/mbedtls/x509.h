/**
 * @file x509.h
 * @brief mbedTLS X.509 wrapper - stub for compilation
 */
#ifndef MBEDTLS_X509_H
#define MBEDTLS_X509_H

#include "Std_Types.h"

/* X.509 types */
typedef struct mbedtls_x509_buf {
    int tag;
    unsigned char* p;
    size_t len;
} mbedtls_x509_buf;

typedef struct mbedtls_x509_name {
    mbedtls_x509_buf oid;
    mbedtls_x509_buf val;
    struct mbedtls_x509_name* next;
} mbedtls_x509_name;

typedef struct mbedtls_x509_time {
    int year, mon, day;
    int hour, min, sec;
} mbedtls_x509_time;

typedef struct mbedtls_x509_sequence {
    mbedtls_x509_buf buf;
    struct mbedtls_x509_sequence* next;
} mbedtls_x509_sequence;

/* X.509 extension types */
#define MBEDTLS_X509_EXT_BASIC_CONSTRAINTS      0
#define MBEDTLS_X509_EXT_KEY_USAGE              1
#define MBEDTLS_X509_EXT_EXTENDED_KEY_USAGE     2
#define MBEDTLS_X509_EXT_SUBJECT_ALT_NAME       3
#define MBEDTLS_X509_EXT_NS_CERT_TYPE           4

/* OID arc */
#define MBEDTLS_OID_X509_EXT_BASIC_CONSTRAINTS  "\x55\x1D\x13"
#define MBEDTLS_OID_X509_EXT_KEY_USAGE          "\x55\x1D\x0F"

#endif /* MBEDTLS_X509_H */
