/*
 * DDS Configuration Tool - Security Configuration
 *
 * Copyright (c) 2024 DDS Config Tool Authors
 * SPDX-License-Identifier: MIT
 */

#ifndef SECURITY_CONFIG_H
#define SECURITY_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "dds_config_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Security Constants
 * ============================================================================ */
#define DDS_SEC_MAX_CERT_PATH_LEN       1024
#define DDS_SEC_MAX_KEY_PATH_LEN        1024
#define DDS_SEC_MAX_CA_PATH_LEN         1024
#define DDS_SEC_MAX_PERMISSION_PATH     1024
#define DDS_SEC_MAX_GOVERNANCE_PATH     1024
#define DDS_SEC_MAX_IDENTITY_NAME       256
#define DDS_SEC_MAX_SUBJECT_NAME        512
#define DDS_SEC_MAX_CRL_PATH_LEN        1024
#define DDS_SEC_MAX_CIPHER_SUITE_LEN    128
#define DDS_SEC_MAX_ALGORITHM_LEN       64
#define DDS_SEC_MAX_PLUGIN_NAME         256
#define DDS_SEC_MAX_PROPERTY_NAME       256
#define DDS_SEC_MAX_PROPERTY_VALUE      1024
#define DDS_SEC_MAX_PROPERTIES          32
#define DDS_SEC_MAX_ACCESS_RULES        256
#define DDS_SEC_MAX_SUBJECTS            128

/* ============================================================================
 * Security Plugin Types
 * ============================================================================ */
typedef enum {
    DDS_SEC_PLUGIN_AUTH_NONE = 0,
    DDS_SEC_PLUGIN_AUTH_PKIDH,
    DDS_SEC_PLUGIN_AUTH_DTLSPSK,
    DDS_SEC_PLUGIN_AUTH_CUSTOM
} dds_sec_auth_plugin_t;

typedef enum {
    DDS_SEC_PLUGIN_ACCESS_NONE = 0,
    DDS_SEC_PLUGIN_ACCESS_PERMISSIONS,
    DDS_SEC_PLUGIN_ACCESS_ACL,
    DDS_SEC_PLUGIN_ACCESS_XACML,
    DDS_SEC_PLUGIN_ACCESS_CUSTOM
} dds_sec_access_plugin_t;

typedef enum {
    DDS_SEC_PLUGIN_CRYPTO_NONE = 0,
    DDS_SEC_PLUGIN_CRYPTO_AES_GCM_GMAC,
    DDS_SEC_PLUGIN_CRYPTO_CUSTOM
} dds_sec_crypto_plugin_t;

typedef enum {
    DDS_SEC_KEY_ESTABLISHMENT_DH = 0,
    DDS_SEC_KEY_ESTABLISHMENT_ECDH,
    DDS_SEC_KEY_ESTABLISHMENT_PSK
} dds_sec_key_establishment_t;

typedef enum {
    DDS_SEC_SIGNATURE_RSA = 0,
    DDS_SEC_SIGNATURE_ECDSA,
    DDS_SEC_SIGNATURE_ED25519
} dds_sec_signature_algorithm_t;

typedef enum {
    DDS_SEC_CIPHER_AES128_GCM = 0,
    DDS_SEC_CIPHER_AES256_GCM,
    DDS_SEC_CIPHER_CHACHA20_POLY1305
} dds_sec_cipher_suite_t;

typedef enum {
    DDS_SEC_HASH_SHA256 = 0,
    DDS_SEC_HASH_SHA384,
    DDS_SEC_HASH_SHA512
} dds_sec_hash_algorithm_t;

/* ============================================================================
 * Certificate Configuration
 * ============================================================================ */
typedef struct {
    char path[DDS_SEC_MAX_CERT_PATH_LEN];
    char subject_name[DDS_SEC_MAX_SUBJECT_NAME];
    char issuer_name[DDS_SEC_MAX_SUBJECT_NAME];
    uint64_t serial_number;
    uint64_t valid_from;
    uint64_t valid_until;
    bool is_ca;
    bool is_self_signed;
} dds_sec_certificate_t;

typedef struct {
    char path[DDS_SEC_MAX_KEY_PATH_LEN];
    char algorithm[DDS_SEC_MAX_ALGORITHM_LEN];
    uint32_t key_size;
    bool encrypted;
    char password_env[64];  /* Environment variable containing password */
} dds_sec_private_key_t;

typedef struct {
    char path[DDS_SEC_MAX_CA_PATH_LEN];
    dds_sec_certificate_t *cert_chain;
    size_t cert_chain_length;
    bool verify_depth;
    uint32_t max_verify_depth;
} dds_sec_ca_config_t;

typedef struct {
    char path[DDS_SEC_MAX_CRL_PATH_LEN];
    bool check_crl;
    bool require_crl;
} dds_sec_crl_config_t;

/* ============================================================================
 * Identity Configuration
 * ============================================================================ */
typedef struct {
    char identity_name[DDS_SEC_MAX_IDENTITY_NAME];
    char organization[256];
    char organizational_unit[256];
    char country[3];
    char state[128];
    char locality[128];
    char email[256];
} dds_sec_identity_info_t;

typedef struct {
    dds_sec_identity_info_t info;
    dds_sec_certificate_t certificate;
    dds_sec_private_key_t private_key;
    dds_sec_ca_config_t ca;
    dds_sec_crl_config_t crl;
} dds_sec_identity_config_t;

/* ============================================================================
 * Access Control Configuration
 * ============================================================================ */
typedef enum {
    DDS_SEC_PERM_PUBLISH = 0x01,
    DDS_SEC_PERM_SUBSCRIBE = 0x02,
    DDS_SEC_PERM_REGISTER_TYPE = 0x04,
    DDS_SEC_PERM_CREATE_PARTICIPANT = 0x08,
    DDS_SEC_PERM_CREATE_TOPIC = 0x10,
    DDS_SEC_PERM_CREATE_PUBLISHER = 0x20,
    DDS_SEC_PERM_CREATE_SUBSCRIBER = 0x40,
    DDS_SEC_PERM_ALL = 0xFF
} dds_sec_permission_t;

typedef struct {
    char subject_name[DDS_SEC_MAX_SUBJECT_NAME];
    char topic_name[DDS_CONFIG_MAX_NAME_LEN];
    uint32_t domain_id;
    uint32_t permissions;  /* Bitmask of dds_sec_permission_t */
    dds_sec_cipher_suite_t min_cipher_suite;
    bool time_based;
    uint64_t valid_from;
    uint64_t valid_until;
} dds_sec_access_rule_t;

typedef struct {
    char path[DDS_SEC_MAX_PERMISSION_PATH];
    dds_sec_access_rule_t *rules;
    size_t rule_count;
    char signature[512];
} dds_sec_permissions_doc_t;

typedef struct {
    char path[DDS_SEC_MAX_GOVERNANCE_PATH];
    bool allow_unauthenticated_participants;
    bool enable_join_access_control;
    bool discovery_protection_kind;
    bool liveliness_protection_kind;
    bool rtps_protection_kind;
    bool discovery_protection_encrypt;
    bool liveliness_protection_encrypt;
    dds_sec_cipher_suite_t default_cipher_suite;
    char signature[512];
} dds_sec_governance_t;

typedef struct {
    dds_sec_governance_t governance;
    dds_sec_permissions_doc_t permissions;
    dds_sec_ca_config_t permissions_ca;
} dds_sec_access_control_config_t;

/* ============================================================================
 * Cryptography Configuration
 * ============================================================================ */
typedef struct {
    dds_sec_cipher_suite_t cipher_suite;
    bool encrypt_rtps;
    bool encrypt_payload;
    bool encrypt_submessage;
    bool sign_messages;
    bool key_exchange_encryption;
} dds_sec_crypto_transforms_t;

typedef struct {
    uint32_t max_participants;
    uint32_t max_keys_per_participant;
    uint32_t key_derivation_iterations;
    uint32_t session_key_lifetime_secs;
    bool enable_key_rotation;
    uint32_t key_rotation_interval_secs;
} dds_sec_crypto_limits_t;

typedef struct {
    dds_sec_crypto_transforms_t transforms;
    dds_sec_crypto_limits_t limits;
    char master_key[64];  /* For testing only - use secure storage in production */
    bool use_kek;         /* Key Encryption Key */
    char kek_provider[64]; /* e.g., "pkcs11", "hsm", "file" */
} dds_sec_crypto_config_t;

/* ============================================================================
 * Plugin Configuration
 * ============================================================================ */
typedef struct {
    char name[DDS_SEC_MAX_PROPERTY_NAME];
    char value[DDS_SEC_MAX_PROPERTY_VALUE];
    bool propagate;
} dds_sec_property_t;

typedef struct {
    dds_sec_auth_plugin_t type;
    char library_path[512];
    char factory_function[128];
    dds_sec_property_t properties[DDS_SEC_MAX_PROPERTIES];
    size_t property_count;
    dds_sec_key_establishment_t key_establishment;
    dds_sec_signature_algorithm_t signature_algo;
} dds_sec_auth_plugin_config_t;

typedef struct {
    dds_sec_access_plugin_t type;
    char library_path[512];
    char factory_function[128];
    dds_sec_property_t properties[DDS_SEC_MAX_PROPERTIES];
    size_t property_count;
} dds_sec_access_plugin_config_t;

typedef struct {
    dds_sec_crypto_plugin_t type;
    char library_path[512];
    char factory_function[128];
    dds_sec_property_t properties[DDS_SEC_MAX_PROPERTIES];
    size_t property_count;
} dds_sec_crypto_plugin_config_t;

/* ============================================================================
 * Logging and Auditing
 * ============================================================================ */
typedef enum {
    DDS_SEC_LOG_NONE = 0,
    DDS_SEC_LOG_FAILURES,
    DDS_SEC_LOG_SUCCESSES,
    DDS_SEC_LOG_ALL
} dds_sec_log_level_t;

typedef struct {
    bool enabled;
    dds_sec_log_level_t level;
    char log_file[512];
    bool syslog_enabled;
    bool remote_audit;
    char remote_audit_endpoint[256];
    uint32_t max_log_size_mb;
    uint32_t max_log_files;
} dds_sec_audit_config_t;

/* ============================================================================
 * Main Security Configuration
/* ============================================================================
 * Main Security Configuration (Extended version)
 * ============================================================================ */
typedef struct {
    bool enabled;
    
    /* Plugin configuration */
    dds_sec_auth_plugin_config_t authentication;
    dds_sec_access_plugin_config_t access_control;
    dds_sec_crypto_plugin_config_t cryptography;
    
    /* Security settings */
    dds_sec_identity_config_t identity;
    dds_sec_access_control_config_t access;
    dds_sec_crypto_config_t crypto;
    dds_sec_audit_config_t audit;
    
    /* Compatibility settings */
    bool allow_unauthenticated;
    uint32_t authentication_timeout_ms;
    uint32_t handshake_timeout_ms;
} dds_sec_full_config_t;

/* ============================================================================
 * Certificate Management
 * ============================================================================ */
typedef struct {
    char *csr_data;
    size_t csr_length;
    char private_key_path[DDS_SEC_MAX_KEY_PATH_LEN];
} dds_sec_csr_request_t;

typedef struct {
    char *certificate_data;
    size_t certificate_length;
    char issuer[DDS_SEC_MAX_SUBJECT_NAME];
    uint64_t valid_from;
    uint64_t valid_until;
} dds_sec_certificate_response_t;

typedef struct {
    char *crl_data;
    size_t crl_length;
    uint64_t this_update;
    uint64_t next_update;
    uint64_t *revoked_serials;
    size_t revoked_count;
} dds_sec_crl_data_t;

/* ============================================================================
 * API Functions - Security Configuration
 * ============================================================================ */

/* Configuration creation and destruction */
dds_sec_full_config_t* dds_security_config_create(void);
void dds_security_config_destroy(dds_sec_full_config_t *config);
dds_sec_full_config_t* dds_security_config_clone(const dds_sec_full_config_t *config);

/* Configuration loading */
dds_config_error_t dds_security_config_load(dds_sec_full_config_t *config, 
                                            const dds_configuration_t *dds_config);
dds_config_error_t dds_security_config_load_file(dds_sec_full_config_t *config,
                                                  const char *filename);

/* Configuration validation */
dds_config_error_t dds_security_config_validate(const dds_sec_full_config_t *config);
bool dds_security_config_is_valid(const dds_sec_full_config_t *config);

/* Certificate operations */
dds_config_error_t dds_sec_load_certificate(dds_sec_certificate_t *cert, 
                                            const char *path);
dds_config_error_t dds_sec_validate_certificate(const dds_sec_certificate_t *cert,
                                                const dds_sec_ca_config_t *ca);
dds_config_error_t dds_sec_generate_csr(const dds_sec_identity_info_t *identity,
                                        const dds_sec_private_key_t *key,
                                        dds_sec_csr_request_t *csr);
dds_config_error_t dds_sec_sign_csr(const dds_sec_csr_request_t *csr,
                                    const dds_sec_ca_config_t *ca,
                                    uint64_t valid_days,
                                    dds_sec_certificate_response_t *response);

/* CRL operations */
dds_config_error_t dds_sec_load_crl(dds_sec_crl_data_t *crl, const char *path);
dds_config_error_t dds_sec_generate_crl(const dds_sec_ca_config_t *ca,
                                        const uint64_t *revoked_serials,
                                        size_t revoked_count,
                                        dds_sec_crl_data_t *crl);
void dds_sec_crl_data_free(dds_sec_crl_data_t *crl);

/* Governance and permissions */
dds_config_error_t dds_sec_load_governance(dds_sec_governance_t *gov, 
                                           const char *path);
dds_config_error_t dds_sec_save_governance(const dds_sec_governance_t *gov,
                                           const char *path);
dds_config_error_t dds_sec_load_permissions(dds_sec_permissions_doc_t *perms,
                                            const char *path);
dds_config_error_t dds_sec_save_permissions(const dds_sec_permissions_doc_t *perms,
                                            const char *path);
void dds_sec_permissions_doc_free(dds_sec_permissions_doc_t *perms);

/* Access control */
dds_config_error_t dds_sec_add_access_rule(dds_sec_permissions_doc_t *perms,
                                           const dds_sec_access_rule_t *rule);
dds_config_error_t dds_sec_remove_access_rule(dds_sec_permissions_doc_t *perms,
                                              const char *subject_name,
                                              const char *topic_name);
bool dds_sec_check_permission(const dds_sec_permissions_doc_t *perms,
                              const char *subject_name,
                              const char *topic_name,
                              dds_sec_permission_t permission);

/* Utilities */
const char* dds_sec_auth_plugin_to_string(dds_sec_auth_plugin_t plugin);
const char* dds_sec_access_plugin_to_string(dds_sec_access_plugin_t plugin);
const char* dds_sec_crypto_plugin_to_string(dds_sec_crypto_plugin_t plugin);
const char* dds_sec_cipher_suite_to_string(dds_sec_cipher_suite_t cipher);
const char* dds_sec_signature_algo_to_string(dds_sec_signature_algorithm_t algo);
dds_sec_auth_plugin_t dds_sec_auth_plugin_from_string(const char *str);
dds_sec_access_plugin_t dds_sec_access_plugin_from_string(const char *str);
dds_sec_crypto_plugin_t dds_sec_crypto_plugin_from_string(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_CONFIG_H */
