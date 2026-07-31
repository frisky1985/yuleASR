/*==================================================================================================
 * ecp.h - mbedTLS elliptic curve point/group stub (yuleASR)
 *================================================================================================*/
#ifndef MBEDTLS_ECP_H
#define MBEDTLS_ECP_H

#include <stddef.h>
#include "mbedtls/bignum.h"

typedef enum {
    MBEDTLS_ECP_DP_NONE = 0,
    MBEDTLS_ECP_DP_SECP192R1,
    MBEDTLS_ECP_DP_SECP224R1,
    MBEDTLS_ECP_DP_SECP256R1,
    MBEDTLS_ECP_DP_SECP384R1,
    MBEDTLS_ECP_DP_SECP521R1,
    MBEDTLS_ECP_DP_BP256R1,
    MBEDTLS_ECP_DP_BP384R1,
    MBEDTLS_ECP_DP_BP512R1,
    MBEDTLS_ECP_DP_SECP256K1,
    MBEDTLS_ECP_DP_SECP192K1,
    MBEDTLS_ECP_DP_SECP224K1,
    MBEDTLS_ECP_DP_CURVE25519,
    MBEDTLS_ECP_DP_CURVE448
} mbedtls_ecp_group_id;

typedef struct mbedtls_ecp_point {
    mbedtls_mpi X;
    mbedtls_mpi Y;
    mbedtls_mpi Z;
} mbedtls_ecp_point;

typedef struct mbedtls_ecp_group {
    mbedtls_ecp_group_id id;
    mbedtls_mpi P;
    mbedtls_mpi A;
    mbedtls_mpi B;
    mbedtls_mpi N;
    mbedtls_mpi GX;
    mbedtls_mpi GY;
} mbedtls_ecp_group;

void mbedtls_ecp_group_init(mbedtls_ecp_group *grp);
void mbedtls_ecp_group_free(mbedtls_ecp_group *grp);
int  mbedtls_ecp_group_load(mbedtls_ecp_group *grp, mbedtls_ecp_group_id id);
void mbedtls_ecp_point_init(mbedtls_ecp_point *pt);
void mbedtls_ecp_point_free(mbedtls_ecp_point *pt);
int  mbedtls_ecp_mul(mbedtls_ecp_group *grp, mbedtls_ecp_point *R,
                     const mbedtls_mpi *m, const mbedtls_ecp_point *P,
                     int (*f_rng)(void *, unsigned char *, size_t), void *p_rng);
int  mbedtls_ecp_gen_keypair(mbedtls_ecp_group *grp, mbedtls_mpi *d,
                             mbedtls_ecp_point *Q,
                             int (*f_rng)(void *, unsigned char *, size_t), void *p_rng);

#endif /* MBEDTLS_ECP_H */
