/**
 * @file ecp.h
 * @brief mbedTLS Elliptic Curve Point wrapper - stub for compilation
 */
#ifndef MBEDTLS_ECP_H
#define MBEDTLS_ECP_H

#include "Std_Types.h"

/* ECP group */
typedef struct mbedtls_ecp_group {
    int id;
    uint32 nbits;
} mbedtls_ecp_group;

/* ECP point */
typedef struct mbedtls_ecp_point {
    uint32 x;
} mbedtls_ecp_point;

/* ECP keypair */
typedef struct mbedtls_ecp_keypair {
    mbedtls_ecp_group grp;
    mbedtls_ecp_point Q;
    mbedtls_mpi d;
} mbedtls_ecp_keypair;

/* ECP curve identifiers */
#define MBEDTLS_ECP_DP_SECP256R1     0x0017
#define MBEDTLS_ECP_DP_SECP384R1     0x0018
#define MBEDTLS_ECP_DP_SECP521R1     0x0019
#define MBEDTLS_ECP_DP_BP256R1       0x001A
#define MBEDTLS_ECP_DP_CURVE25519    0x001D

/* Function stubs */
extern void mbedtls_ecp_keypair_init(mbedtls_ecp_keypair* key);
extern void mbedtls_ecp_keypair_free(mbedtls_ecp_keypair* key);
extern int mbedtls_ecp_read_key(int grp_id, mbedtls_ecp_keypair* key, const unsigned char* buf, size_t buflen);
extern int mbedtls_ecp_mul(mbedtls_ecp_group* grp, mbedtls_ecp_point* R, const mbedtls_mpi* m, const mbedtls_ecp_point* P, void* f_rng, void* p_rng);
extern int mbedtls_ecp_point_read_binary(mbedtls_ecp_group* grp, mbedtls_ecp_point* P, const unsigned char* buf, size_t ilen);
extern int mbedtls_ecp_tls_read_group(mbedtls_ecp_group* grp, const unsigned char** buf, size_t len);
extern int mbedtls_ecp_check_pub_priv(const mbedtls_ecp_keypair* pub, const mbedtls_ecp_keypair* prv);

#endif /* MBEDTLS_ECP_H */
