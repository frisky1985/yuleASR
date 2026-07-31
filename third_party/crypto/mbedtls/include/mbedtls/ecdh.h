/*==================================================================================================
 * ecdh.h - mbedTLS ECDH stub (yuleASR)
 *================================================================================================*/
#ifndef MBEDTLS_ECDH_H
#define MBEDTLS_ECDH_H

#include <stddef.h>
#include "mbedtls/ecp.h"

typedef struct mbedtls_ecdh_context {
    mbedtls_ecp_group grp;
    mbedtls_ecp_point Qp;
    mbedtls_mpi d;
    mbedtls_ecp_point Q;
    mbedtls_ecp_point z;
} mbedtls_ecdh_context;

#endif /* MBEDTLS_ECDH_H */
