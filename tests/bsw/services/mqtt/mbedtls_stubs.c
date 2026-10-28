/**
 * @file mbedtls_stubs.c
 * @brief Configurable mbedTLS stubs for the Mqtt_Tls / Mqtt_CertMgr unit tests.
 *
 * The test targets compile the production sources (Mqtt_Tls.c / Mqtt_CertMgr.c)
 * directly against the REAL mbedtls headers (for true struct layouts, constants
 * and the inline mbedtls_ssl_get_version_number), but replace every mbedtls
 * function with these controllable stubs instead of linking the real library.
 * This lets the tests drive both the success and the failure paths of the
 * production code deterministically.
 *
 * Control knobs (non-static globals, extern-declared by the test files):
 *  - stub_x509_parse_mode   0=auto (data starting "FAKE" parses, else fails)
 *                           1=always succeed, 2=always fail
 *  - stub_cert_valid_from_year / stub_cert_valid_to_year  template cert dates
 *  - stub_x509_verify_ret / stub_x509_verify_flags         x509_crt_verify out
 *  - stub_ssl_setup_ret / stub_pk_parse_key_ret /
 *    stub_ssl_conf_own_cert_ret / stub_ssl_set_hostname_ret
 *  - stub_ssl_handshake_ret  0 -> success, else returned as failure
 *  - stub_ssl_bio_trigger    when != 0, ssl_handshake first invokes the
 *                            registered BIO callbacks (send/recv/recv_timeout)
 *  - stub_ssl_write_ret / stub_ssl_read_ret
 *  - stub_ssl_verify_flags   value returned by mbedtls_ssl_get_verify_result
 *  - stub_ssl_peer_cert      pointer returned by mbedtls_ssl_get_peer_cert
 *  - stub_ssl_ciphersuite    string returned by mbedtls_ssl_get_ciphersuite
 *  - stub_mem_init_ret       Crypto_MbedTLS_MemInit result
 *  - stub_strerror_empty     !=0 -> mbedtls_strerror writes an empty string
 */

#include "Std_Types.h"
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <limits.h>

#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/x509.h"
#include "mbedtls/pk.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"

/*============================================================================
 * Configurable knobs
 *===========================================================================*/
int stub_x509_parse_mode = 0;
int stub_cert_valid_from_year = 2023;
int stub_cert_valid_to_year = 2033;
int stub_x509_verify_ret = 0;
uint32_t stub_x509_verify_flags = 0U;

int stub_ssl_setup_ret = 0;
int stub_pk_parse_key_ret = 0;
int stub_ssl_conf_own_cert_ret = 0;
int stub_ssl_set_hostname_ret = 0;
int stub_ssl_handshake_ret = -1;
int stub_ssl_bio_trigger = 1;
int stub_ssl_write_ret = 10;
int stub_ssl_read_ret = 5;
uint32_t stub_ssl_verify_flags = 0U;
const mbedtls_x509_crt* stub_ssl_peer_cert = NULL_PTR;
const char* stub_ssl_ciphersuite = "TLS-STUB-SUITE";
int stub_mem_init_ret = 0;
int stub_strerror_empty = 0;

/*============================================================================
 * Call counters / capture
 *===========================================================================*/
int stub_parse_calls = 0;
int stub_parse_der_calls = 0;
int stub_ca_chain_calls = 0;
int stub_own_cert_calls = 0;
int stub_set_hostname_calls = 0;
int stub_handshake_calls = 0;
int stub_get_session_calls = 0;
int stub_close_notify_calls = 0;
int stub_ssl_write_calls = 0;
int stub_ssl_read_calls = 0;
int stub_bio_send_calls = 0;
int stub_bio_recv_calls = 0;
int stub_bio_recv_timeout_calls = 0;
int stub_last_authmode = -1;

/* Most recent ssl context passed to mbedtls_ssl_init (tests use it to poke
 * the inline-only tls_version field for Mqtt_Tls_GetVersion). */
mbedtls_ssl_context* stub_last_ssl = NULL_PTR;

/* BIO registered by mbedtls_ssl_set_bio. */
static void* stub_bio_ctx = NULL_PTR;
static mbedtls_ssl_send_t* stub_bio_send = NULL_PTR;
static mbedtls_ssl_recv_t* stub_bio_recv = NULL_PTR;
static mbedtls_ssl_recv_timeout_t* stub_bio_recv_timeout = NULL_PTR;

/*============================================================================
 * X.509 template data ("FAKE" parse fills the target with this)
 *===========================================================================*/
static unsigned char oid_cn[]  = { 0x55, 0x04, 0x03 };
static unsigned char oid_o[]   = { 0x55, 0x04, 0x0A };
static unsigned char oid_ou[]  = { 0x55, 0x04, 0x0B };
static unsigned char oid_c[]   = { 0x55, 0x04, 0x06 };
static unsigned char oid_st[]  = { 0x55, 0x04, 0x08 };
static unsigned char oid_l[]   = { 0x55, 0x04, 0x07 };
static unsigned char oid_mail[]= { 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x09, 0x01 };

static char val_cn[]   = "yule-client";
static char val_o[]    = "YuleTech";
static char val_ou[]   = "BSW Team";
static char val_c[]    = "CN";
static char val_st[]   = "Shanghai";
static char val_l[]    = "Pudong";
static char val_mail[] = "bsw@yuletech.example";
/* Second-level issuer value used by Mqtt_Tls_VerifyCertificate (issuer.next->val) */
static char val_issuer2[] = "Test Issuer CA";

/* mbedtls_asn1_buf field order in the real header: { int tag; size_t len; unsigned char *p } */
#define STUB_BUF_OID(a) { 0x06, sizeof(a), (unsigned char*)(a) }
#define STUB_BUF_VAL(a) { 0x0C, sizeof(a) - 1U, (unsigned char*)(a) }

static mbedtls_x509_buf stub_oid_cn  = STUB_BUF_OID(oid_cn);
static mbedtls_x509_buf stub_oid_o   = STUB_BUF_OID(oid_o);
static mbedtls_x509_buf stub_oid_ou  = STUB_BUF_OID(oid_ou);
static mbedtls_x509_buf stub_oid_c   = STUB_BUF_OID(oid_c);
static mbedtls_x509_buf stub_oid_st  = STUB_BUF_OID(oid_st);
static mbedtls_x509_buf stub_oid_l   = STUB_BUF_OID(oid_l);
static mbedtls_x509_buf stub_oid_mail= STUB_BUF_OID(oid_mail);
static mbedtls_x509_buf stub_val_cn  = STUB_BUF_VAL(val_cn);
static mbedtls_x509_buf stub_val_o   = STUB_BUF_VAL(val_o);
static mbedtls_x509_buf stub_val_ou  = STUB_BUF_VAL(val_ou);
static mbedtls_x509_buf stub_val_c   = STUB_BUF_VAL(val_c);
static mbedtls_x509_buf stub_val_st  = STUB_BUF_VAL(val_st);
static mbedtls_x509_buf stub_val_l   = STUB_BUF_VAL(val_l);
static mbedtls_x509_buf stub_val_mail= STUB_BUF_VAL(val_mail);
static mbedtls_x509_buf stub_val_issuer2 = STUB_BUF_VAL(val_issuer2);

/* Subject chain: CN -> O -> OU -> C -> ST -> L -> email (7 nodes, loop-free) */
static mbedtls_x509_name stub_subject_nodes[7];
/* Issuer chain: two nodes; node[1] carries the value VerifyCertificate copies */
static mbedtls_x509_name stub_issuer_nodes[2];
static int stub_names_built = 0;

static unsigned char stub_serial_byte = 0xABU;

static mbedtls_x509_crt stub_parsed_template;
static mbedtls_x509_crt stub_peer_cert_obj;

static void stub_build_names(void) {
    int i;
    if (stub_names_built != 0) {
        return;
    }
    memset(stub_subject_nodes, 0, sizeof(stub_subject_nodes));
    memset(stub_issuer_nodes, 0, sizeof(stub_issuer_nodes));

    stub_subject_nodes[0].oid = stub_oid_cn;   stub_subject_nodes[0].val = stub_val_cn;
    stub_subject_nodes[1].oid = stub_oid_o;    stub_subject_nodes[1].val = stub_val_o;
    stub_subject_nodes[2].oid = stub_oid_ou;   stub_subject_nodes[2].val = stub_val_ou;
    stub_subject_nodes[3].oid = stub_oid_c;    stub_subject_nodes[3].val = stub_val_c;
    stub_subject_nodes[4].oid = stub_oid_st;   stub_subject_nodes[4].val = stub_val_st;
    stub_subject_nodes[5].oid = stub_oid_l;    stub_subject_nodes[5].val = stub_val_l;
    stub_subject_nodes[6].oid = stub_oid_mail; stub_subject_nodes[6].val = stub_val_mail;
    for (i = 0; i < 6; i++) {
        stub_subject_nodes[i].next = &stub_subject_nodes[i + 1];
    }

    stub_issuer_nodes[0].oid = stub_oid_cn;
    stub_issuer_nodes[0].val = stub_val_issuer2;
    stub_issuer_nodes[0].next = &stub_issuer_nodes[1];
    stub_issuer_nodes[1].oid = stub_oid_cn;
    stub_issuer_nodes[1].val = stub_val_issuer2;

    stub_names_built = 1;
}

static void stub_fill_cert_template(void) {
    stub_build_names();
    memset(&stub_parsed_template, 0, sizeof(stub_parsed_template));
    stub_parsed_template.version = 3;
    stub_parsed_template.serial.p = &stub_serial_byte;
    stub_parsed_template.serial.len = 1U;
    stub_parsed_template.subject = stub_subject_nodes[0];
    stub_parsed_template.issuer = stub_issuer_nodes[0];
    stub_parsed_template.valid_from.year = stub_cert_valid_from_year;
    stub_parsed_template.valid_from.mon = 1;
    stub_parsed_template.valid_from.day = 1;
    stub_parsed_template.valid_to.year = stub_cert_valid_to_year;
    stub_parsed_template.valid_to.mon = 1;
    stub_parsed_template.valid_to.day = 1;
    stub_parsed_template.ca_istrue = 1;
    stub_parsed_template.key_usage = 0x86U; /* keyCertSign|cRLSign|digitalSignature */
    stub_parsed_template.next = NULL_PTR;
}

/** Prepare the object returned by mbedtls_ssl_get_peer_cert(). */
void stub_peer_cert_setup(void) {
    stub_build_names();
    memset(&stub_peer_cert_obj, 0, sizeof(stub_peer_cert_obj));
    stub_peer_cert_obj.version = 3;
    stub_peer_cert_obj.subject = stub_subject_nodes[0];
    stub_peer_cert_obj.issuer = stub_issuer_nodes[0];
    stub_peer_cert_obj.next = NULL_PTR; /* -> notExpired=TRUE branch */
    stub_ssl_peer_cert = &stub_peer_cert_obj;
}

void stub_peer_cert_clear(void) {
    stub_ssl_peer_cert = NULL_PTR;
}

void stub_all_reset(void) {
    stub_x509_parse_mode = 0;
    stub_cert_valid_from_year = 2023;
    stub_cert_valid_to_year = 2033;
    stub_x509_verify_ret = 0;
    stub_x509_verify_flags = 0U;
    stub_ssl_setup_ret = 0;
    stub_pk_parse_key_ret = 0;
    stub_ssl_conf_own_cert_ret = 0;
    stub_ssl_set_hostname_ret = 0;
    stub_ssl_handshake_ret = -1;
    stub_ssl_bio_trigger = 1;
    stub_ssl_write_ret = 10;
    stub_ssl_read_ret = 5;
    stub_ssl_verify_flags = 0U;
    stub_ssl_peer_cert = NULL_PTR;
    stub_ssl_ciphersuite = "TLS-STUB-SUITE";
    stub_mem_init_ret = 0;
    stub_strerror_empty = 0;
    stub_parse_calls = 0;
    stub_parse_der_calls = 0;
    stub_ca_chain_calls = 0;
    stub_own_cert_calls = 0;
    stub_set_hostname_calls = 0;
    stub_handshake_calls = 0;
    stub_get_session_calls = 0;
    stub_close_notify_calls = 0;
    stub_ssl_write_calls = 0;
    stub_ssl_read_calls = 0;
    stub_bio_send_calls = 0;
    stub_bio_recv_calls = 0;
    stub_bio_recv_timeout_calls = 0;
    stub_last_authmode = -1;
    stub_last_ssl = NULL_PTR;
    stub_bio_ctx = NULL_PTR;
    stub_bio_send = NULL_PTR;
    stub_bio_recv = NULL_PTR;
    stub_bio_recv_timeout = NULL_PTR;
}

/*============================================================================
 * Crypto_MbedTLS_Mem stub
 *===========================================================================*/
Std_ReturnType Crypto_MbedTLS_MemInit(void) {
    return (stub_mem_init_ret == 0) ? E_OK : E_NOT_OK;
}

/*============================================================================
 * X.509 stubs
 *===========================================================================*/
void mbedtls_x509_crt_init(mbedtls_x509_crt *crt) {
    if (crt != NULL_PTR) {
        memset(crt, 0, sizeof(*crt));
    }
}

void mbedtls_x509_crt_free(mbedtls_x509_crt *crt) {
    (void)crt;
}

static int stub_x509_parse_common(mbedtls_x509_crt *chain, const unsigned char *buf, size_t buflen) {
    if (chain == NULL_PTR) {
        return -1;
    }
    if (stub_x509_parse_mode == 2) {
        return -0x2700;
    }
    if ((stub_x509_parse_mode == 1) ||
        ((buflen >= 4U) && (buf != NULL_PTR) &&
         (buf[0] == 'F') && (buf[1] == 'A') && (buf[2] == 'K') && (buf[3] == 'E'))) {
        stub_fill_cert_template();
        *chain = stub_parsed_template;
        return 0;
    }
    return -0x2700;
}

int mbedtls_x509_crt_parse(mbedtls_x509_crt *chain, const unsigned char *buf, size_t buflen) {
    stub_parse_calls++;
    return stub_x509_parse_common(chain, buf, buflen);
}

int mbedtls_x509_crt_parse_der(mbedtls_x509_crt *chain, const unsigned char *buf, size_t buflen) {
    stub_parse_der_calls++;
    return stub_x509_parse_common(chain, buf, buflen);
}

int mbedtls_x509_crt_verify(mbedtls_x509_crt *chain, mbedtls_x509_crt *trust_ca,
                            mbedtls_x509_crl *ca_crl, const char *cn, uint32_t *flags,
                            int (*f_vrfy)(void *, mbedtls_x509_crt *, int, uint32_t *),
                            void *p_vrfy) {
    (void)chain;
    (void)trust_ca;
    (void)ca_crl;
    (void)cn;
    (void)f_vrfy;
    (void)p_vrfy;
    if (flags != NULL_PTR) {
        *flags = stub_x509_verify_flags;
    }
    return stub_x509_verify_ret;
}

/*============================================================================
 * PK stubs
 *===========================================================================*/
void mbedtls_pk_init(mbedtls_pk_context *ctx) {
    if (ctx != NULL_PTR) {
        memset(ctx, 0, sizeof(*ctx));
    }
}

void mbedtls_pk_free(mbedtls_pk_context *ctx) {
    (void)ctx;
}

int mbedtls_pk_parse_key(mbedtls_pk_context *ctx, const unsigned char *key, size_t keylen,
                         const unsigned char *pwd, size_t pwdlen,
                         int (*f_rng)(void *, unsigned char *, size_t), void *p_rng) {
    (void)ctx; (void)key; (void)keylen; (void)pwd; (void)pwdlen;
    if (f_rng != NULL_PTR) {
        /* The production code passes mbedtls_ctr_drbg_random as f_rng; call
         * it once so the (stubbed) RNG path is exercised too. */
        unsigned char seed[4] = { 0x12U, 0x34U, 0x56U, 0x78U };
        (void)f_rng(p_rng, seed, sizeof(seed));
    }
    return stub_pk_parse_key_ret;
}

/* Referenced by function pointer in Mqtt_Tls.c (ssl RNG callback). */
int mbedtls_ctr_drbg_random(void *p_rng, unsigned char *output, size_t output_len) {
    (void)p_rng;
    if (output != NULL_PTR) {
        memset(output, 0xA5U, output_len);
    }
    return 0;
}

/*============================================================================
 * SSL stubs
 *===========================================================================*/
void mbedtls_ssl_init(mbedtls_ssl_context *ssl) {
    if (ssl != NULL_PTR) {
        memset(ssl, 0, sizeof(*ssl));
        stub_last_ssl = ssl;
    }
}

void mbedtls_ssl_free(mbedtls_ssl_context *ssl) {
    (void)ssl;
}

void mbedtls_ssl_config_init(mbedtls_ssl_config *conf) {
    if (conf != NULL_PTR) {
        memset(conf, 0, sizeof(*conf));
    }
}

void mbedtls_ssl_config_free(mbedtls_ssl_config *conf) {
    (void)conf;
}

void mbedtls_ssl_session_init(mbedtls_ssl_session *session) {
    if (session != NULL_PTR) {
        memset(session, 0, sizeof(*session));
    }
}

void mbedtls_ssl_session_free(mbedtls_ssl_session *session) {
    (void)session;
}

void mbedtls_ssl_conf_min_version(mbedtls_ssl_config *conf, int minor, int major) {
    (void)conf; (void)minor; (void)major;
}

void mbedtls_ssl_conf_max_version(mbedtls_ssl_config *conf, int minor, int major) {
    (void)conf; (void)minor; (void)major;
}

void mbedtls_ssl_conf_authmode(mbedtls_ssl_config *conf, int authmode) {
    (void)conf;
    stub_last_authmode = authmode;
}

void mbedtls_ssl_conf_ca_chain(mbedtls_ssl_config *conf, mbedtls_x509_crt *ca_chain,
                               mbedtls_x509_crl *ca_crl) {
    (void)conf; (void)ca_chain; (void)ca_crl;
    stub_ca_chain_calls++;
}

int mbedtls_ssl_conf_own_cert(mbedtls_ssl_config *conf, mbedtls_x509_crt *own_cert,
                              mbedtls_pk_context *pk_key) {
    (void)conf; (void)own_cert; (void)pk_key;
    stub_own_cert_calls++;
    return stub_ssl_conf_own_cert_ret;
}

int mbedtls_ssl_set_hostname(mbedtls_ssl_context *ssl, const char *hostname) {
    (void)ssl; (void)hostname;
    stub_set_hostname_calls++;
    return stub_ssl_set_hostname_ret;
}

void mbedtls_ssl_conf_dbg(mbedtls_ssl_config *conf,
                          void (*f_dbg)(void *, int, const char *, int, const char *),
                          void *p_dbg) {
    (void)conf; (void)f_dbg; (void)p_dbg;
}

void mbedtls_debug_set_threshold(int threshold) {
    (void)threshold;
}

int mbedtls_ssl_setup(mbedtls_ssl_context *ssl, const mbedtls_ssl_config *conf) {
    (void)ssl; (void)conf;
    return stub_ssl_setup_ret;
}

void mbedtls_ssl_set_bio(mbedtls_ssl_context *ssl, void *p_bio,
                         mbedtls_ssl_send_t *f_send, mbedtls_ssl_recv_t *f_recv,
                         mbedtls_ssl_recv_timeout_t *f_recv_timeout) {
    (void)ssl;
    stub_bio_ctx = p_bio;
    stub_bio_send = f_send;
    stub_bio_recv = f_recv;
    stub_bio_recv_timeout = f_recv_timeout;
}

int mbedtls_ssl_handshake(mbedtls_ssl_context *ssl) {
    unsigned char tmp[8];
    stub_handshake_calls++;
    if ((stub_ssl_bio_trigger != 0) && (ssl != NULL_PTR)) {
        /* Drive the registered BIO callbacks once so the production
         * Mqtt_Tls_SendCallback / RecvCallback / RecvTimeoutCallback bodies run
         * against the TcpIp stubs (recv returns 0 -> WANT_READ path). */
        if (stub_bio_recv != NULL_PTR) {
            stub_bio_recv_calls++;
            (void)stub_bio_recv(stub_bio_ctx, tmp, 1U);
        }
        if (stub_bio_recv_timeout != NULL_PTR) {
            stub_bio_recv_timeout_calls++;
            (void)stub_bio_recv_timeout(stub_bio_ctx, tmp, 1U, 0U);
        }
        if (stub_bio_send != NULL_PTR) {
            stub_bio_send_calls++;
            (void)stub_bio_send(stub_bio_ctx, (const unsigned char*)"HI", 2U);
        }
    }
    return stub_ssl_handshake_ret;
}

int mbedtls_ssl_get_session(const mbedtls_ssl_context *ssl, mbedtls_ssl_session *session) {
    (void)ssl; (void)session;
    stub_get_session_calls++;
    return 0;
}

int mbedtls_ssl_write(mbedtls_ssl_context *ssl, const unsigned char *buf, size_t len) {
    (void)ssl; (void)buf; (void)len;
    stub_ssl_write_calls++;
    return stub_ssl_write_ret;
}

int mbedtls_ssl_read(mbedtls_ssl_context *ssl, unsigned char *buf, size_t len) {
    (void)ssl; (void)buf; (void)len;
    stub_ssl_read_calls++;
    return stub_ssl_read_ret;
}

int mbedtls_ssl_close_notify(mbedtls_ssl_context *ssl) {
    (void)ssl;
    stub_close_notify_calls++;
    return 0;
}

uint32_t mbedtls_ssl_get_verify_result(const mbedtls_ssl_context *ssl) {
    (void)ssl;
    return stub_ssl_verify_flags;
}

const mbedtls_x509_crt *mbedtls_ssl_get_peer_cert(const mbedtls_ssl_context *ssl) {
    (void)ssl;
    return stub_ssl_peer_cert;
}

const char *mbedtls_ssl_get_ciphersuite(const mbedtls_ssl_context *ssl) {
    (void)ssl;
    return stub_ssl_ciphersuite;
}

void mbedtls_strerror(int errnum, char *buffer, size_t buflen) {
    (void)errnum;
    if ((buffer != NULL_PTR) && (buflen > 0U)) {
        if (stub_strerror_empty != 0) {
            buffer[0] = '\0';
        } else {
            (void)snprintf(buffer, buflen, "stub tls error %d", errnum);
        }
    }
}
