#ifndef MBEDTLS_BIGNUM_H
#define MBEDTLS_BIGNUM_H
#include "Std_Types.h"
typedef struct mbedtls_mpi { uint32_t* p; size_t n; } mbedtls_mpi;
#define MBEDTLS_ERR_MPI_FILE_IO_ERROR -0x0002
extern void mbedtls_mpi_init(mbedtls_mpi* X);
extern void mbedtls_mpi_free(mbedtls_mpi* X);
extern int mbedtls_mpi_read_binary(mbedtls_mpi* X, const unsigned char* buf, size_t buflen);
extern int mbedtls_mpi_write_binary(const mbedtls_mpi* X, unsigned char* buf, size_t buflen);
extern int mbedtls_mpi_read_string(mbedtls_mpi* X, int radix, const char* s);
#endif
