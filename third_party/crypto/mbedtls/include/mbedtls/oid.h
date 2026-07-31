/*==================================================================================================
 * oid.h - mbedTLS object identifiers stub (yuleASR)
 *================================================================================================*/
#ifndef MBEDTLS_OID_H
#define MBEDTLS_OID_H

#include <stddef.h>
#include <string.h>

/* Compare a static OID array against an mbedtls_x509_buf */
#define MBEDTLS_OID_CMP(oid1, oid2) \
    ( sizeof(oid1) == (oid2)->len && \
      memcmp( oid1, (oid2)->p, (oid2)->len ) == 0 )

/* X.520 distinguished name attribute OIDs (2.5.4.x) — compound literals so
 * sizeof() works inside MBEDTLS_OID_CMP. */
#define MBEDTLS_OID_AT_CN                       ((const unsigned char[]){ 0x55, 0x04, 0x03 })
#define MBEDTLS_OID_AT_COUNTRY                  ((const unsigned char[]){ 0x55, 0x04, 0x06 })
#define MBEDTLS_OID_AT_LOCALITY                 ((const unsigned char[]){ 0x55, 0x04, 0x07 })
#define MBEDTLS_OID_AT_STATE_PROVINCE           ((const unsigned char[]){ 0x55, 0x04, 0x08 })
#define MBEDTLS_OID_AT_ORGANIZATION_NAME        ((const unsigned char[]){ 0x55, 0x04, 0x0A })
#define MBEDTLS_OID_AT_ORG_UNIT                 ((const unsigned char[]){ 0x55, 0x04, 0x0B })

/* PKCS#9 email address OID (1.2.840.113549.1.9.1) */
#define MBEDTLS_OID_PKCS9_EMAIL                 ((const unsigned char[]){ 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x09, 0x01, 0x16 })

#endif /* MBEDTLS_OID_H */
