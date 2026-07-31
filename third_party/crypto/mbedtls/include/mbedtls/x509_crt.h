/*==================================================================================================
 * x509_crt.h - mbedTLS X.509 certificate API stub (yuleASR)
 *================================================================================================*/
#ifndef MBEDTLS_X509_CRT_H
#define MBEDTLS_X509_CRT_H

#include <stddef.h>
#include <stdint.h>
#include "mbedtls/x509.h"

void mbedtls_x509_crt_init(mbedtls_x509_crt *crt);
void mbedtls_x509_crt_free(mbedtls_x509_crt *crt);
int  mbedtls_x509_crt_parse(mbedtls_x509_crt *chain, const unsigned char *buf, size_t buflen);
int  mbedtls_x509_crt_parse_der(mbedtls_x509_crt *chain, const unsigned char *buf, size_t buflen);
int  mbedtls_x509_crt_verify(mbedtls_x509_crt *crt,
                             mbedtls_x509_crt *trust_ca,
                             mbedtls_x509_crl *ca_crl,
                             const char *cn,
                             uint32_t *flags,
                             int (*f_vrfy)(void *, mbedtls_x509_crt *, int, uint32_t *),
                             void *p_vrfy);

#endif /* MBEDTLS_X509_CRT_H */
