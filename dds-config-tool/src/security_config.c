/*
 * DDS Configuration Tool - Security Configuration Implementation
 *
 * Copyright (c) 2024 DDS Config Tool Authors
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "security_config.h"
#include "dds_config_parser.h"

/* ============================================================================
 * Internal Constants
 * ============================================================================ */
#define SEC_CONFIG_MAGIC 0x53454343  /* "SECC" */

/* ============================================================================
 * Utility Macros
 * ============================================================================ */
#define SAFE_STRCPY(dest, src, max) do { \
    if ((src) != NULL) { \
        strncpy((dest), (src), (max) - 1); \
        (dest)[(max) - 1] = '\0'; \
    } else { \
        (dest)[0] = '\0'; \
    } \
} while(0)

#define FREE_PTR(ptr) do { \
    free(ptr); \
    (ptr) = NULL; \
} while(0)

/* ============================================================================
 * Certificate Utilities (Simplified - Real implementation would use OpenSSL)
 * ============================================================================ */

static bool file_exists(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (fp) {
        fclose(fp);
        return true;
    }
    return false;
}

static uint64_t get_current_time_seconds(void)
{
    return (uint64_t)time(NULL);
}

/* ============================================================================
 * Configuration Creation and Destruction
 * ============================================================================ */

dds_sec_full_config_t* dds_security_config_create(void)
{
    dds_sec_full_config_t *config = calloc(1, sizeof(dds_sec_full_config_t));
    if (!config) {
        return NULL;
    }

    /* Initialize defaults */
    config->enabled = false;
    config->allow_unauthenticated = false;
    config->authentication_timeout_ms = 10000;  /* 10 seconds */
    config->handshake_timeout_ms = 30000;       /* 30 seconds */
    
    /* Default plugin types */
    config->authentication.type = DDS_SEC_PLUGIN_AUTH_PKIDH;
    config->access_control.type = DDS_SEC_PLUGIN_ACCESS_PERMISSIONS;
    config->cryptography.type = DDS_SEC_PLUGIN_CRYPTO_AES_GCM_GMAC;
    
    /* Default crypto transforms */
    config->crypto.transforms.cipher_suite = DDS_SEC_CIPHER_AES256_GCM;
    config->crypto.transforms.encrypt_rtps = true;
    config->crypto.transforms.encrypt_payload = true;
    config->crypto.transforms.encrypt_submessage = true;
    config->crypto.transforms.sign_messages = true;
    
    /* Default crypto limits */
    config->crypto.limits.max_participants = 100;
    config->crypto.limits.max_keys_per_participant = 32;
    config->crypto.limits.key_derivation_iterations = 10000;
    config->crypto.limits.session_key_lifetime_secs = 3600;
    config->crypto.limits.enable_key_rotation = true;
    config->crypto.limits.key_rotation_interval_secs = 86400;  /* 24 hours */
    
    /* Default audit settings */
    config->audit.enabled = true;
    config->audit.level = DDS_SEC_LOG_FAILURES;
    config->audit.max_log_size_mb = 100;
    config->audit.max_log_files = 10;
    
    /* Default governance */
    config->access.governance.allow_unauthenticated_participants = false;
    config->access.governance.enable_join_access_control = true;
    config->access.governance.default_cipher_suite = DDS_SEC_CIPHER_AES256_GCM;
    
    return config;
}

void dds_security_config_destroy(dds_sec_full_config_t *config)
{
    if (!config) return;
    
    /* Free certificate chain in CA config */
    if (config->identity.ca.cert_chain) {
        free(config->identity.ca.cert_chain);
    }
    
    /* Free permissions rules */
    if (config->access.permissions.rules) {
        free(config->access.permissions.rules);
    }
    
    /* Free permissions CA certificate chain */
    if (config->access.permissions_ca.cert_chain) {
        free(config->access.permissions_ca.cert_chain);
    }
    
    free(config);
}

dds_sec_full_config_t* dds_security_config_clone(const dds_sec_full_config_t *config)
{
    if (!config) return NULL;
    
    dds_sec_full_config_t *clone = dds_security_config_create();
    if (!clone) return NULL;
    
    /* Copy main config */
    memcpy(clone, config, sizeof(dds_sec_full_config_t));
    
    /* Deep copy certificate chain */
    if (config->identity.ca.cert_chain_length > 0 && config->identity.ca.cert_chain) {
        clone->identity.ca.cert_chain = calloc(config->identity.ca.cert_chain_length,
                                                sizeof(dds_sec_certificate_t));
        if (clone->identity.ca.cert_chain) {
            memcpy(clone->identity.ca.cert_chain, config->identity.ca.cert_chain,
                   config->identity.ca.cert_chain_length * sizeof(dds_sec_certificate_t));
        }
    }
    
    /* Deep copy permissions rules */
    if (config->access.permissions.rule_count > 0 && config->access.permissions.rules) {
        clone->access.permissions.rules = calloc(config->access.permissions.rule_count,
                                                  sizeof(dds_sec_access_rule_t));
        if (clone->access.permissions.rules) {
            memcpy(clone->access.permissions.rules, config->access.permissions.rules,
                   config->access.permissions.rule_count * sizeof(dds_sec_access_rule_t));
        }
    }
    
    return clone;
}

/* ============================================================================
 * Configuration Loading
 * ============================================================================ */

dds_config_error_t dds_security_config_load(dds_sec_full_config_t *config,
                                            const dds_configuration_t *dds_config)
{
    if (!config || !dds_config) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    /* Copy security settings from DDS configuration */
    for (size_t i = 0; i < dds_config->domain_count; i++) {
        const dds_security_config_t *sec = &dds_config->domains[i].security;
        
        config->enabled = sec->enabled;
        if (sec->enabled) {
            config->authentication.type = sec->auth_kind;
            config->access_control.type = sec->access_kind;
            config->cryptography.type = sec->crypto_kind;
            
            SAFE_STRCPY(config->identity.ca.path, sec->identity_ca, 
                       DDS_SEC_MAX_CA_PATH_LEN);
            SAFE_STRCPY(config->identity.certificate.path, sec->identity_certificate,
                       DDS_SEC_MAX_CERT_PATH_LEN);
            SAFE_STRCPY(config->identity.private_key.path, sec->private_key,
                       DDS_SEC_MAX_KEY_PATH_LEN);
            SAFE_STRCPY(config->access.governance.path, sec->governance,
                       DDS_SEC_MAX_GOVERNANCE_PATH);
            SAFE_STRCPY(config->access.permissions.path, sec->permissions,
                       DDS_SEC_MAX_PERMISSION_PATH);
            
            config->crypto.transforms.encrypt_rtps = sec->encrypt_rtps;
            config->crypto.transforms.encrypt_payload = sec->encrypt_payload;
        }
    }
    
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_security_config_load_file(dds_sec_full_config_t *config,
                                                  const char *filename)
{
    if (!config || !filename) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    /* For now, just validate the file exists */
    if (!file_exists(filename)) {
        return DDS_CONFIG_ERR_FILE_IO;
    }
    
    /* TODO: Implement full YAML/XML security config parsing */
    /* This would parse a dedicated security configuration file */
    
    return DDS_CONFIG_OK;
}

/* ============================================================================
 * Configuration Validation
 * ============================================================================ */

dds_config_error_t dds_security_config_validate(const dds_sec_full_config_t *config)
{
    if (!config) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    if (!config->enabled) {
        return DDS_CONFIG_OK;  /* Disabled config is always valid */
    }
    
    /* Validate identity configuration */
    if (config->identity.certificate.path[0] == '\0') {
        return DDS_CONFIG_ERR_VALIDATION_FAILED;
    }
    
    if (!file_exists(config->identity.certificate.path)) {
        return DDS_CONFIG_ERR_FILE_IO;
    }
    
    if (config->identity.private_key.path[0] == '\0') {
        return DDS_CONFIG_ERR_VALIDATION_FAILED;
    }
    
    if (!file_exists(config->identity.private_key.path)) {
        return DDS_CONFIG_ERR_FILE_IO;
    }
    
    /* Validate CA */
    if (config->identity.ca.path[0] != '\0' && 
        !file_exists(config->identity.ca.path)) {
        return DDS_CONFIG_ERR_FILE_IO;
    }
    
    /* Validate access control configuration */
    if (config->access_control.type != DDS_SEC_PLUGIN_ACCESS_NONE) {
        if (config->access.governance.path[0] != '\0' &&
            !file_exists(config->access.governance.path)) {
            return DDS_CONFIG_ERR_FILE_IO;
        }
        
        if (config->access.permissions.path[0] != '\0' &&
            !file_exists(config->access.permissions.path)) {
            return DDS_CONFIG_ERR_FILE_IO;
        }
        
        if (config->access.permissions_ca.path[0] != '\0' &&
            !file_exists(config->access.permissions_ca.path)) {
            return DDS_CONFIG_ERR_FILE_IO;
        }
    }
    
    return DDS_CONFIG_OK;
}

bool dds_security_config_is_valid(const dds_sec_full_config_t *config)
{
    return dds_security_config_validate(config) == DDS_CONFIG_OK;
}

/* ============================================================================
 * Certificate Operations
 * ============================================================================ */

dds_config_error_t dds_sec_load_certificate(dds_sec_certificate_t *cert,
                                            const char *path)
{
    if (!cert || !path) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    memset(cert, 0, sizeof(dds_sec_certificate_t));
    SAFE_STRCPY(cert->path, path, DDS_SEC_MAX_CERT_PATH_LEN);
    
    /* TODO: Implement real certificate parsing using OpenSSL */
    /* For now, just validate file exists and set dummy values */
    if (!file_exists(path)) {
        return DDS_CONFIG_ERR_FILE_IO;
    }
    
    /* Set dummy values for testing */
    cert->valid_from = get_current_time_seconds();
    cert->valid_until = cert->valid_from + (365 * 24 * 3600);  /* 1 year */
    cert->is_ca = false;
    cert->is_self_signed = false;
    
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_sec_validate_certificate(const dds_sec_certificate_t *cert,
                                                const dds_sec_ca_config_t *ca)
{
    if (!cert) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    uint64_t now = get_current_time_seconds();
    
    /* Check validity period */
    if (now < cert->valid_from || now > cert->valid_until) {
        return DDS_CONFIG_ERR_VALIDATION_FAILED;
    }
    
    /* Check CA if provided */
    if (ca && ca->path[0] != '\0') {
        if (!file_exists(ca->path)) {
            return DDS_CONFIG_ERR_FILE_IO;
        }
        /* TODO: Implement certificate chain validation */
    }
    
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_sec_generate_csr(const dds_sec_identity_info_t *identity,
                                        const dds_sec_private_key_t *key,
                                        dds_sec_csr_request_t *csr)
{
    if (!identity || !key || !csr) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    memset(csr, 0, sizeof(dds_sec_csr_request_t));
    
    /* TODO: Implement real CSR generation using OpenSSL */
    /* For now, create a placeholder CSR structure */
    
    csr->csr_data = malloc(2048);
    if (!csr->csr_data) {
        return DDS_CONFIG_ERR_MEMORY;
    }
    
    /* Create a PEM-like CSR representation */
    int len = snprintf(csr->csr_data, 2048,
        "-----BEGIN CERTIFICATE REQUEST-----\n"
        "Subject: CN=%s, O=%s, OU=%s, C=%s\n"
        "Key Algorithm: %s\n"
        "Key Size: %u\n"
        "-----END CERTIFICATE REQUEST-----\n",
        identity->identity_name,
        identity->organization,
        identity->organizational_unit,
        identity->country,
        key->algorithm,
        key->key_size);
    
    csr->csr_length = (size_t)len;
    SAFE_STRCPY(csr->private_key_path, key->path, DDS_SEC_MAX_KEY_PATH_LEN);
    
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_sec_sign_csr(const dds_sec_csr_request_t *csr,
                                    const dds_sec_ca_config_t *ca,
                                    uint64_t valid_days,
                                    dds_sec_certificate_response_t *response)
{
    if (!csr || !ca || !response) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    memset(response, 0, sizeof(dds_sec_certificate_response_t));
    
    /* Validate CA exists */
    if (!file_exists(ca->path)) {
        return DDS_CONFIG_ERR_FILE_IO;
    }
    
    /* TODO: Implement real CSR signing using OpenSSL */
    
    response->certificate_data = malloc(4096);
    if (!response->certificate_data) {
        return DDS_CONFIG_ERR_MEMORY;
    }
    
    /* Create a PEM-like certificate representation */
    response->valid_from = get_current_time_seconds();
    response->valid_until = response->valid_from + (valid_days * 24 * 3600);
    
    int len = snprintf(response->certificate_data, 4096,
        "-----BEGIN CERTIFICATE-----\n"
        "Issuer: CA Certificate\n"
        "Valid From: %llu\n"
        "Valid Until: %llu\n"
        "Serial: 12345\n"
        "-----END CERTIFICATE-----\n",
        (unsigned long long)response->valid_from,
        (unsigned long long)response->valid_until);
    
    response->certificate_length = (size_t)len;
    SAFE_STRCPY(response->issuer, ca->path, DDS_SEC_MAX_SUBJECT_NAME);
    
    return DDS_CONFIG_OK;
}

/* ============================================================================
 * CRL Operations
 * ============================================================================ */

dds_config_error_t dds_sec_load_crl(dds_sec_crl_data_t *crl, const char *path)
{
    if (!crl || !path) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    memset(crl, 0, sizeof(dds_sec_crl_data_t));
    
    if (!file_exists(path)) {
        return DDS_CONFIG_ERR_FILE_IO;
    }
    
    /* TODO: Implement real CRL parsing */
    
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_sec_generate_crl(const dds_sec_ca_config_t *ca,
                                        const uint64_t *revoked_serials,
                                        size_t revoked_count,
                                        dds_sec_crl_data_t *crl)
{
    if (!ca || !crl) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    memset(crl, 0, sizeof(dds_sec_crl_data_t));
    
    if (!file_exists(ca->path)) {
        return DDS_CONFIG_ERR_FILE_IO;
    }
    
    /* TODO: Implement real CRL generation */
    
    crl->this_update = get_current_time_seconds();
    crl->next_update = crl->this_update + (7 * 24 * 3600);  /* 7 days */
    
    if (revoked_count > 0 && revoked_serials) {
        crl->revoked_serials = calloc(revoked_count, sizeof(uint64_t));
        if (!crl->revoked_serials) {
            return DDS_CONFIG_ERR_MEMORY;
        }
        memcpy(crl->revoked_serials, revoked_serials, revoked_count * sizeof(uint64_t));
        crl->revoked_count = revoked_count;
    }
    
    return DDS_CONFIG_OK;
}

void dds_sec_crl_data_free(dds_sec_crl_data_t *crl)
{
    if (!crl) return;
    
    FREE_PTR(crl->crl_data);
    FREE_PTR(crl->revoked_serials);
    crl->crl_length = 0;
    crl->revoked_count = 0;
}

/* ============================================================================
 * Governance and Permissions
 * ============================================================================ */

dds_config_error_t dds_sec_load_governance(dds_sec_governance_t *gov,
                                           const char *path)
{
    if (!gov || !path) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    memset(gov, 0, sizeof(dds_sec_governance_t));
    SAFE_STRCPY(gov->path, path, DDS_SEC_MAX_GOVERNANCE_PATH);
    
    if (!file_exists(path)) {
        return DDS_CONFIG_ERR_FILE_IO;
    }
    
    /* TODO: Implement real governance XML parsing */
    
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_sec_save_governance(const dds_sec_governance_t *gov,
                                           const char *path)
{
    if (!gov || !path) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    FILE *fp = fopen(path, "w");
    if (!fp) {
        return DDS_CONFIG_ERR_FILE_IO;
    }
    
    /* Generate governance XML document */
    fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(fp, "<dds xmlns=\"http://www.omg.org/dds/\"\n");
    fprintf(fp, "     xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n");
    fprintf(fp, "     xsi:schemaLocation=\"http://www.omg.org/dds/ DDS-Security.xsd\">\n");
    fprintf(fp, "  <domain_access_rules>\n");
    fprintf(fp, "    <domain_rule>\n");
    fprintf(fp, "      <domains>\n");
    fprintf(fp, "        <id_range>\n");
    fprintf(fp, "          <min>0</min>\n");
    fprintf(fp, "          <max>230</max>\n");
    fprintf(fp, "        </id_range>\n");
    fprintf(fp, "      </domains>\n");
    fprintf(fp, "      <allow_unauthenticated_participants>%s</allow_unauthenticated_participants>\n",
            gov->allow_unauthenticated_participants ? "true" : "false");
    fprintf(fp, "      <enable_join_access_control>%s</enable_join_access_control>\n",
            gov->enable_join_access_control ? "true" : "false");
    fprintf(fp, "      <discovery_protection_kind>ENCRYPT</discovery_protection_kind>\n");
    fprintf(fp, "      <liveliness_protection_kind>ENCRYPT</liveliness_protection_kind>\n");
    fprintf(fp, "      <rtps_protection_kind>SIGN</rtps_protection_kind>\n");
    fprintf(fp, "    </domain_rule>\n");
    fprintf(fp, "  </domain_access_rules>\n");
    fprintf(fp, "</dds>\n");
    
    fclose(fp);
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_sec_load_permissions(dds_sec_permissions_doc_t *perms,
                                            const char *path)
{
    if (!perms || !path) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    memset(perms, 0, sizeof(dds_sec_permissions_doc_t));
    SAFE_STRCPY(perms->path, path, DDS_SEC_MAX_PERMISSION_PATH);
    
    if (!file_exists(path)) {
        return DDS_CONFIG_ERR_FILE_IO;
    }
    
    /* TODO: Implement real permissions XML parsing */
    
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_sec_save_permissions(const dds_sec_permissions_doc_t *perms,
                                            const char *path)
{
    if (!perms || !path) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    FILE *fp = fopen(path, "w");
    if (!fp) {
        return DDS_CONFIG_ERR_FILE_IO;
    }
    
    /* Generate permissions XML document */
    fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(fp, "<dds xmlns=\"http://www.omg.org/dds/\"\n");
    fprintf(fp, "     xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n");
    fprintf(fp, "     xsi:schemaLocation=\"http://www.omg.org/dds/ DDS-Security.xsd\">\n");
    fprintf(fp, "  <permissions>\n");
    
    for (size_t i = 0; i < perms->rule_count; i++) {
        const dds_sec_access_rule_t *rule = &perms->rules[i];
        fprintf(fp, "    <grant name=\"Grant_%zu\">\n", i);
        fprintf(fp, "      <subject_name>%s</subject_name>\n", rule->subject_name);
        fprintf(fp, "      <validity>\n");
        fprintf(fp, "        <not_before>1970-01-01T00:00:00</not_before>\n");
        fprintf(fp, "        <not_after>2099-12-31T23:59:59</not_after>\n");
        fprintf(fp, "      </validity>\n");
        fprintf(fp, "      <allow_rule>\n");
        fprintf(fp, "        <domains>\n");
        fprintf(fp, "          <id>%u</id>\n", rule->domain_id);
        fprintf(fp, "        </domains>\n");
        fprintf(fp, "        <publish>\n");
        fprintf(fp, "          <topics>\n");
        fprintf(fp, "            <topic>%s</topic>\n", rule->topic_name);
        fprintf(fp, "          </topics>\n");
        fprintf(fp, "        </publish>\n");
        fprintf(fp, "        <subscribe>\n");
        fprintf(fp, "          <topics>\n");
        fprintf(fp, "            <topic>%s</topic>\n", rule->topic_name);
        fprintf(fp, "          </topics>\n");
        fprintf(fp, "        </subscribe>\n");
        fprintf(fp, "      </allow_rule>\n");
        fprintf(fp, "    </grant>\n");
    }
    
    fprintf(fp, "  </permissions>\n");
    fprintf(fp, "</dds>\n");
    
    fclose(fp);
    return DDS_CONFIG_OK;
}

void dds_sec_permissions_doc_free(dds_sec_permissions_doc_t *perms)
{
    if (!perms) return;
    
    FREE_PTR(perms->rules);
    perms->rule_count = 0;
}

/* ============================================================================
 * Access Control
 * ============================================================================ */

dds_config_error_t dds_sec_add_access_rule(dds_sec_permissions_doc_t *perms,
                                           const dds_sec_access_rule_t *rule)
{
    if (!perms || !rule) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    if (perms->rule_count >= DDS_SEC_MAX_ACCESS_RULES) {
        return DDS_CONFIG_ERR_MEMORY;
    }
    
    dds_sec_access_rule_t *new_rules = realloc(perms->rules,
                                               (perms->rule_count + 1) * sizeof(dds_sec_access_rule_t));
    if (!new_rules) {
        return DDS_CONFIG_ERR_MEMORY;
    }
    
    perms->rules = new_rules;
    memcpy(&perms->rules[perms->rule_count], rule, sizeof(dds_sec_access_rule_t));
    perms->rule_count++;
    
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_sec_remove_access_rule(dds_sec_permissions_doc_t *perms,
                                              const char *subject_name,
                                              const char *topic_name)
{
    if (!perms || !subject_name) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    for (size_t i = 0; i < perms->rule_count; i++) {
        if (strcmp(perms->rules[i].subject_name, subject_name) == 0 &&
            (topic_name == NULL || strcmp(perms->rules[i].topic_name, topic_name) == 0)) {
            /* Remove this rule by shifting remaining rules */
            if (i < perms->rule_count - 1) {
                memmove(&perms->rules[i], &perms->rules[i + 1],
                        (perms->rule_count - i - 1) * sizeof(dds_sec_access_rule_t));
            }
            perms->rule_count--;
            return DDS_CONFIG_OK;
        }
    }
    
    return DDS_CONFIG_ERR_NOT_FOUND;
}

bool dds_sec_check_permission(const dds_sec_permissions_doc_t *perms,
                              const char *subject_name,
                              const char *topic_name,
                              dds_sec_permission_t permission)
{
    if (!perms || !subject_name || !topic_name) {
        return false;
    }
    
    for (size_t i = 0; i < perms->rule_count; i++) {
        const dds_sec_access_rule_t *rule = &perms->rules[i];
        
        if (strcmp(rule->subject_name, subject_name) == 0 &&
            strcmp(rule->topic_name, topic_name) == 0) {
            return (rule->permissions & permission) != 0;
        }
    }
    
    return false;
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

const char* dds_sec_auth_plugin_to_string(dds_sec_auth_plugin_t plugin)
{
    switch (plugin) {
        case DDS_SEC_PLUGIN_AUTH_NONE: return "NONE";
        case DDS_SEC_PLUGIN_AUTH_PKIDH: return "PKI-DH";
        case DDS_SEC_PLUGIN_AUTH_DTLSPSK: return "DTLS-PSK";
        case DDS_SEC_PLUGIN_AUTH_CUSTOM: return "CUSTOM";
        default: return "UNKNOWN";
    }
}

const char* dds_sec_access_plugin_to_string(dds_sec_access_plugin_t plugin)
{
    switch (plugin) {
        case DDS_SEC_PLUGIN_ACCESS_NONE: return "NONE";
        case DDS_SEC_PLUGIN_ACCESS_PERMISSIONS: return "PERMISSIONS";
        case DDS_SEC_PLUGIN_ACCESS_ACL: return "ACL";
        case DDS_SEC_PLUGIN_ACCESS_XACML: return "XACML";
        case DDS_SEC_PLUGIN_ACCESS_CUSTOM: return "CUSTOM";
        default: return "UNKNOWN";
    }
}

const char* dds_sec_crypto_plugin_to_string(dds_sec_crypto_plugin_t plugin)
{
    switch (plugin) {
        case DDS_SEC_PLUGIN_CRYPTO_NONE: return "NONE";
        case DDS_SEC_PLUGIN_CRYPTO_AES_GCM_GMAC: return "AES-GCM-GMAC";
        case DDS_SEC_PLUGIN_CRYPTO_CUSTOM: return "CUSTOM";
        default: return "UNKNOWN";
    }
}

const char* dds_sec_cipher_suite_to_string(dds_sec_cipher_suite_t cipher)
{
    switch (cipher) {
        case DDS_SEC_CIPHER_AES128_GCM: return "AES128-GCM";
        case DDS_SEC_CIPHER_AES256_GCM: return "AES256-GCM";
        case DDS_SEC_CIPHER_CHACHA20_POLY1305: return "CHACHA20-POLY1305";
        default: return "UNKNOWN";
    }
}

const char* dds_sec_signature_algo_to_string(dds_sec_signature_algorithm_t algo)
{
    switch (algo) {
        case DDS_SEC_SIGNATURE_RSA: return "RSA";
        case DDS_SEC_SIGNATURE_ECDSA: return "ECDSA";
        case DDS_SEC_SIGNATURE_ED25519: return "Ed25519";
        default: return "UNKNOWN";
    }
}

dds_sec_auth_plugin_t dds_sec_auth_plugin_from_string(const char *str)
{
    if (!str) return DDS_SEC_PLUGIN_AUTH_NONE;
    
    if (strcasecmp(str, "PKI-DH") == 0 || strcasecmp(str, "PKIDH") == 0) {
        return DDS_SEC_PLUGIN_AUTH_PKIDH;
    } else if (strcasecmp(str, "DTLS-PSK") == 0 || strcasecmp(str, "DTLSPSK") == 0) {
        return DDS_SEC_PLUGIN_AUTH_DTLSPSK;
    } else if (strcasecmp(str, "CUSTOM") == 0) {
        return DDS_SEC_PLUGIN_AUTH_CUSTOM;
    }
    
    return DDS_SEC_PLUGIN_AUTH_NONE;
}

dds_sec_access_plugin_t dds_sec_access_plugin_from_string(const char *str)
{
    if (!str) return DDS_SEC_PLUGIN_ACCESS_NONE;
    
    if (strcasecmp(str, "PERMISSIONS") == 0) {
        return DDS_SEC_PLUGIN_ACCESS_PERMISSIONS;
    } else if (strcasecmp(str, "ACL") == 0) {
        return DDS_SEC_PLUGIN_ACCESS_ACL;
    } else if (strcasecmp(str, "XACML") == 0) {
        return DDS_SEC_PLUGIN_ACCESS_XACML;
    } else if (strcasecmp(str, "CUSTOM") == 0) {
        return DDS_SEC_PLUGIN_ACCESS_CUSTOM;
    }
    
    return DDS_SEC_PLUGIN_ACCESS_NONE;
}

dds_sec_crypto_plugin_t dds_sec_crypto_plugin_from_string(const char *str)
{
    if (!str) return DDS_SEC_PLUGIN_CRYPTO_NONE;
    
    if (strcasecmp(str, "AES-GCM-GMAC") == 0 || strcasecmp(str, "AESGCMGMAC") == 0) {
        return DDS_SEC_PLUGIN_CRYPTO_AES_GCM_GMAC;
    } else if (strcasecmp(str, "CUSTOM") == 0) {
        return DDS_SEC_PLUGIN_CRYPTO_CUSTOM;
    }
    
    return DDS_SEC_PLUGIN_CRYPTO_NONE;
}
