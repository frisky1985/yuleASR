/**
 * @file asn1.h
 * @brief mbedTLS ASN.1 wrapper - stub for compilation
 */
#ifndef MBEDTLS_ASN1_H
#define MBEDTLS_ASN1_H

#include "Std_Types.h"

/* ASN.1 tag types */
#define MBEDTLS_ASN1_BOOLEAN                0x01
#define MBEDTLS_ASN1_INTEGER                0x02
#define MBEDTLS_ASN1_BIT_STRING             0x03
#define MBEDTLS_ASN1_OCTET_STRING           0x04
#define MBEDTLS_ASN1_NULL                   0x05
#define MBEDTLS_ASN1_OID                    0x06
#define MBEDTLS_ASN1_ENUMERATED             0x0A
#define MBEDTLS_ASN1_UTF8_STRING            0x0C
#define MBEDTLS_ASN1_SEQUENCE               0x10
#define MBEDTLS_ASN1_SET                    0x11
#define MBEDTLS_ASN1_PRINTABLE_STRING       0x13
#define MBEDTLS_ASN1_T61_STRING             0x14
#define MBEDTLS_ASN1_IA5_STRING             0x16
#define MBEDTLS_ASN1_UTC_TIME               0x17
#define MBEDTLS_ASN1_GENERALIZED_TIME       0x18
#define MBEDTLS_ASN1_UNIVERSAL_STRING       0x1C
#define MBEDTLS_ASN1_BMP_STRING             0x1E
#define MBEDTLS_ASN1_PRIMITIVE              0x00
#define MBEDTLS_ASN1_CONSTRUCTED            0x20
#define MBEDTLS_ASN1_CONTEXT_SPECIFIC       0x80

/* ASN.1 error codes */
#define MBEDTLS_ERR_ASN1_OUT_OF_DATA        -0x0060
#define MBEDTLS_ERR_ASN1_UNEXPECTED_TAG     -0x0062
#define MBEDTLS_ERR_ASN1_INVALID_LENGTH     -0x0064
#define MBEDTLS_ERR_ASN1_LENGTH_MISMATCH    -0x0066
#define MBEDTLS_ERR_ASN1_INVALID_DATA       -0x0068
#define MBEDTLS_ERR_ASN1_ALLOC_FAILED       -0x006A
#define MBEDTLS_ERR_ASN1_BUF_TOO_SMALL      -0x006C

/* ASN.1 buffer */
typedef struct mbedtls_asn1_buf {
    int tag;
    unsigned char* p;
    size_t len;
} mbedtls_asn1_buf;

/* ASN.1 sequence */
typedef struct mbedtls_asn1_sequence {
    mbedtls_asn1_buf buf;
    struct mbedtls_asn1_sequence* next;
} mbedtls_asn1_sequence;

/* ASN.1 named data */
typedef struct mbedtls_asn1_named_data {
    mbedtls_asn1_buf oid;
    mbedtls_asn1_buf val;
    struct mbedtls_asn1_named_data* next;
    unsigned char next_merged;
} mbedtls_asn1_named_data;

/* ASN.1 parsing functions */
extern int mbedtls_asn1_get_len(unsigned char** p, const unsigned char* end, size_t* len);
extern int mbedtls_asn1_get_tag(unsigned char** p, const unsigned char* end, size_t* len, int tag);
extern int mbedtls_asn1_get_bool(unsigned char** p, const unsigned char* end, int* val);
extern int mbedtls_asn1_get_int(unsigned char** p, const unsigned char* end, int* val);
extern int mbedtls_asn1_get_mpi(unsigned char** p, const unsigned char* end, void* X);
extern int mbedtls_asn1_get_bitstring(unsigned char** p, const unsigned char* end, mbedtls_asn1_buf* bs);
extern int mbedtls_asn1_get_bitstring_null(unsigned char** p, const unsigned char* end, size_t* len);
extern int mbedtls_asn1_get_sequence_of(unsigned char** p, const unsigned char* end,
    mbedtls_asn1_sequence* cur, int tag);
extern int mbedtls_asn1_get_alg(unsigned char** p, const unsigned char* end,
    mbedtls_asn1_buf* alg_oid, mbedtls_asn1_buf* alg_params);
extern int mbedtls_asn1_get_alg_null(unsigned char** p, const unsigned char* end,
    mbedtls_asn1_buf* alg_oid);

#endif /* MBEDTLS_ASN1_H */
