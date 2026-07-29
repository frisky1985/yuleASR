/**
 * @file ssl.h
 * @brief mbedTLS SSL/TLS wrapper - stub for compilation
 */
#ifndef MBEDTLS_SSL_H
#define MBEDTLS_SSL_H

#include "Std_Types.h"
#include "mbedtls/x509_crt.h"

/* SSL context */
typedef struct mbedtls_ssl_context {
    int state;
    void* handshake;
    void* transform;
    void* session;
    void* session_negotiate;
    void* conf;
    void* hostname;
    void* p_timer;
} mbedtls_ssl_context;

/* SSL configuration */
typedef struct mbedtls_ssl_config {
    int endpoint;
    int transport;
    int min_version;
    int max_version;
    void* psk;
    size_t psk_len;
    void* psk_identity;
    size_t psk_identity_len;
    mbedtls_x509_crt* ca_chain;
    mbedtls_x509_crt* own_cert;
    void* pk_key;
} mbedtls_ssl_config;

/* SSL session */
typedef struct mbedtls_ssl_session {
    mbedtls_x509_crt* peer_cert;
} mbedtls_ssl_session;

/* SSL cipher suite */
#define MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA    0x002F
#define MBEDTLS_TLS_RSA_WITH_AES_256_CBC_SHA    0x0035
#define MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA 0xC013
#define MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256 0xC02B

/* SSL endpoint types */
#define MBEDTLS_SSL_IS_CLIENT   0
#define MBEDTLS_SSL_IS_SERVER   1

/* SSL transport types */
#define MBEDTLS_SSL_TRANSPORT_STREAM   0
#define MBEDTLS_SSL_TRANSPORT_DATAGRAM 1

/* SSL states */
#define MBEDTLS_SSL_HANDSHAKE_OVER      1

/* Function stubs */
extern void mbedtls_ssl_init(mbedtls_ssl_context* ssl);
extern void mbedtls_ssl_free(mbedtls_ssl_context* ssl);
extern int mbedtls_ssl_setup(mbedtls_ssl_context* ssl, const mbedtls_ssl_config* conf);
extern int mbedtls_ssl_handshake(mbedtls_ssl_context* ssl);
extern int mbedtls_ssl_read(mbedtls_ssl_context* ssl, unsigned char* buf, size_t len);
extern int mbedtls_ssl_write(mbedtls_ssl_context* ssl, const unsigned char* buf, size_t len);

#endif /* MBEDTLS_SSL_H */
