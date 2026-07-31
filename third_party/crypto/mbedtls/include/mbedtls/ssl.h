/*==================================================================================================
 * ssl.h - mbedTLS SSL/TLS stub (yuleASR)
 *
 * Minimal declarations matching the mbedTLS 2.28 TLS API surface used by
 * the yuleASR MQTT TLS layer. Definitions come from the real mbedTLS
 * library in production builds.
 *================================================================================================*/
#ifndef MBEDTLS_SSL_H
#define MBEDTLS_SSL_H

#include <stddef.h>
#include <stdint.h>
#include "mbedtls/x509.h"

#define MBEDTLS_SSL_VERIFY_NONE         0
#define MBEDTLS_SSL_VERIFY_OPTIONAL     1
#define MBEDTLS_SSL_VERIFY_REQUIRED     2

#define MBEDTLS_ERR_SSL_WANT_READ          -0x6900
#define MBEDTLS_ERR_SSL_WANT_WRITE         -0x6880
#define MBEDTLS_ERR_SSL_INTERNAL_ERROR     -0x6F00
#define MBEDTLS_ERR_NET_SEND_FAILED        -0x004C
#define MBEDTLS_ERR_NET_RECV_FAILED        -0x004E

void mbedtls_strerror(int ret, char *buf, size_t buflen);

#define MBEDTLS_SSL_MAJOR_VERSION_3     3
#define MBEDTLS_SSL_MINOR_VERSION_3     3

typedef struct mbedtls_ssl_context {
    int state;
    void *conf;
    void *session;
    void *p_bio;
    void *f_send;
    void *f_recv;
    void *f_recv_timeout;
    int verify_result;
} mbedtls_ssl_context;

typedef struct mbedtls_ssl_config {
    int endpoint;
    int min_major_ver;
    int min_minor_ver;
    int max_major_ver;
    int max_minor_ver;
    int authmode;
    mbedtls_x509_crt *ca_chain;
    mbedtls_x509_crl *ca_crl;
    mbedtls_x509_crt *own_cert;
    void *pk_key;
    void (*f_dbg)(void *, int, const char *, int, const char *);
    void *p_dbg;
} mbedtls_ssl_config;

typedef struct mbedtls_ssl_session {
    int ciphersuite;
    int compression;
    unsigned char id[32];
    size_t id_len;
} mbedtls_ssl_session;

typedef int mbedtls_ssl_send_t(void *ctx, const unsigned char *buf, size_t len);
typedef int mbedtls_ssl_recv_t(void *ctx, unsigned char *buf, size_t len);
typedef int mbedtls_ssl_recv_timeout_t(void *ctx, unsigned char *buf, size_t len, uint32_t timeout);

void mbedtls_ssl_init(mbedtls_ssl_context *ssl);
void mbedtls_ssl_free(mbedtls_ssl_context *ssl);
int  mbedtls_ssl_setup(mbedtls_ssl_context *ssl, const mbedtls_ssl_config *conf);
int  mbedtls_ssl_set_hostname(mbedtls_ssl_context *ssl, const char *hostname);
void mbedtls_ssl_set_bio(mbedtls_ssl_context *ssl, void *p_bio,
                         mbedtls_ssl_send_t *f_send, mbedtls_ssl_recv_t *f_recv,
                         mbedtls_ssl_recv_timeout_t *f_recv_timeout);
int  mbedtls_ssl_handshake(mbedtls_ssl_context *ssl);
int  mbedtls_ssl_read(mbedtls_ssl_context *ssl, unsigned char *buf, size_t len);
int  mbedtls_ssl_write(mbedtls_ssl_context *ssl, const unsigned char *buf, size_t len);
int  mbedtls_ssl_close_notify(mbedtls_ssl_context *ssl);

void mbedtls_ssl_config_init(mbedtls_ssl_config *conf);
void mbedtls_ssl_config_free(mbedtls_ssl_config *conf);
int  mbedtls_ssl_conf_min_version(mbedtls_ssl_config *conf, int major, int minor);
int  mbedtls_ssl_conf_max_version(mbedtls_ssl_config *conf, int major, int minor);
void mbedtls_ssl_conf_authmode(mbedtls_ssl_config *conf, int authmode);
void mbedtls_ssl_conf_ca_chain(mbedtls_ssl_config *conf, mbedtls_x509_crt *ca_chain, mbedtls_x509_crl *ca_crl);
int  mbedtls_ssl_conf_own_cert(mbedtls_ssl_config *conf, mbedtls_x509_crt *own_cert, void *pk_key);
void mbedtls_ssl_conf_dbg(mbedtls_ssl_config *conf,
                          void (*f_dbg)(void *, int, const char *, int, const char *),
                          void *p_dbg);

void mbedtls_ssl_session_init(mbedtls_ssl_session *session);
void mbedtls_ssl_session_free(mbedtls_ssl_session *session);
int  mbedtls_ssl_get_session(mbedtls_ssl_context *ssl, mbedtls_ssl_session *session);
int  mbedtls_ssl_get_verify_result(mbedtls_ssl_context *ssl);
const mbedtls_x509_crt *mbedtls_ssl_get_peer_cert(const mbedtls_ssl_context *ssl);
const char *mbedtls_ssl_get_ciphersuite(const mbedtls_ssl_context *ssl);
int  mbedtls_ssl_get_version_number(const mbedtls_ssl_context *ssl);

#endif /* MBEDTLS_SSL_H */
