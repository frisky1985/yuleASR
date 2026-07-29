/**
 * @file oid.h
 * @brief mbedTLS Object Identifier wrapper - stub for compilation
 */
#ifndef MBEDTLS_OID_H
#define MBEDTLS_OID_H

#include "Std_Types.h"

/* OID structure */
typedef struct mbedtls_oid_descriptor_t {
    const char* asn1;       /*!< OID ASN.1 representation */
    size_t asn1_len;        /*!< length of the OID */
    const char* description; /*!< description (full name) */
} mbedtls_oid_descriptor_t;

/* OID lookup functions */
extern int mbedtls_oid_get_x509_ext_type(const char* oid, size_t len, const char** type);
extern int mbedtls_oid_get_attr_oid_desc(const char* oid, size_t len, const char** desc);
extern int mbedtls_oid_get_sig_alg_desc(const char* oid, size_t len, const char** desc);
extern int mbedtls_oid_get_pk_alg_desc(const char* oid, size_t len, const char** desc);
extern int mbedtls_oid_get_md_alg_desc(const char* oid, size_t len, const char** desc);

#endif /* MBEDTLS_OID_H */
