/*==================================================================================================
 * ecdsa.h - mbedTLS ECDSA stub (yuleASR)
 *================================================================================================*/
#ifndef MBEDTLS_ECDSA_H
#define MBEDTLS_ECDSA_H

#include <stddef.h>
#include "mbedtls/ecp.h"
#include "mbedtls/md.h"

typedef struct mbedtls_ecdsa_context {
    mbedtls_ecp_group grp;
    mbedtls_mpi d;
} mbedtls_ecdsa_context;

void mbedtls_ecdsa_init(mbedtls_ecdsa_context *ctx);
void mbedtls_ecdsa_free(mbedtls_ecdsa_context *ctx);
int  mbedtls_ecdsa_setup(mbedtls_ecdsa_context *ctx, mbedtls_ecp_group_id group_id);
int  mbedtls_ecdsa_sign(mbedtls_ecp_group *grp, mbedtls_mpi *r, mbedtls_mpi *s,
                        const mbedtls_mpi *d, const unsigned char *buf, size_t blen,
                        int (*f_rng)(void *, unsigned char *, size_t), void *p_rng);
int  mbedtls_ecdsa_verify(mbedtls_ecp_group *grp, const unsigned char *buf, size_t blen,
                          const mbedtls_ecp_point *Q, const mbedtls_mpi *r, const mbedtls_mpi *s);

#endif /* MBEDTLS_ECDSA_H */
