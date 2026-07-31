/*==================================================================================================
 * bignum.h - mbedTLS multi-precision integer stub (yuleASR)
 *
 * Minimal declarations matching the mbedTLS 2.28 API surface used by
 * yuleASR crypto backends. Definitions are provided by the mbedTLS
 * library when linked in production builds.
 *================================================================================================*/
#ifndef MBEDTLS_BIGNUM_H
#define MBEDTLS_BIGNUM_H

#include <stddef.h>
#include <stdint.h>

#define MBEDTLS_ERR_MPI_ALLOC_FAILED             -0x0010
#define MBEDTLS_ERR_MPI_BAD_INPUT_DATA           -0x0012
#define MBEDTLS_ERR_MPI_INVALID_CHARACTER        -0x0014
#define MBEDTLS_ERR_MPI_BUFFER_TOO_SMALL         -0x0016
#define MBEDTLS_ERR_MPI_NEGATIVE_VALUE           -0x0018
#define MBEDTLS_ERR_MPI_DIVISION_BY_ZERO         -0x001A
#define MBEDTLS_ERR_MPI_NOT_ACCEPTABLE           -0x001C
#define MBEDTLS_ERR_MPI_FILE_IO_ERROR            -0x001E

typedef int32_t mbedtls_mpi_sint;
typedef struct mbedtls_mpi {
    int32_t  s;
    size_t   n;
    unsigned char *p;
} mbedtls_mpi;

void mbedtls_mpi_init(mbedtls_mpi *X);
void mbedtls_mpi_free(mbedtls_mpi *X);
int  mbedtls_mpi_lset(mbedtls_mpi *X, mbedtls_mpi_sint z);
int  mbedtls_mpi_read_binary(mbedtls_mpi *X, const unsigned char *buf, size_t buflen);
int  mbedtls_mpi_write_binary(const mbedtls_mpi *X, unsigned char *buf, size_t buflen);

#endif /* MBEDTLS_BIGNUM_H */
