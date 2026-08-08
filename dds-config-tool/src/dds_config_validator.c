/*
 * DDS Configuration Validator - Implementation
 *
 * Copyright (c) 2024 DDS Config Tool Authors
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dds_config_types.h"
#include "dds_config_validator.h"

/* ============================================================================
 * Default Validation Options
 * ============================================================================ */

const dds_validation_options_t DDS_VALIDATION_DEFAULT_OPTIONS = {
    .strict_mode = false,
    .check_consistency = true,
    .check_performance = true,
    .check_security = true
};

const dds_validation_options_t DDS_VALIDATION_STRICT_OPTIONS = {
    .strict_mode = true,
    .check_consistency = true,
    .check_performance = true,
    .check_security = true
};

/* ============================================================================
 * Validation Result Management
 * ============================================================================ */

dds_validation_result_t* dds_validation_result_create(void)
{
    dds_validation_result_t *result = calloc(1, sizeof(dds_validation_result_t));
    if (!result) {
        return NULL;
    }

    result->entries = NULL;
    result->count = 0;
    result->capacity = 0;
    result->error_count = 0;
    result->warning_count = 0;
    result->info_count = 0;

    return result;
}

void dds_validation_result_destroy(dds_validation_result_t *result)
{
    if (!result) return;

    free(result->entries);
    free(result);
}

void dds_validation_result_clear(dds_validation_result_t *result)
{
    if (!result) return;

    result->count = 0;
    result->error_count = 0;
    result->warning_count = 0;
    result->info_count = 0;
}

bool dds_validation_result_is_valid(const dds_validation_result_t *result)
{
    if (!result) return false;
    return result->error_count == 0;
}

void dds_validation_result_print(const dds_validation_result_t *result)
{
    if (!result) {
        printf("Validation result: NULL\n");
        return;
    }

    printf("\n=== Validation Results ===\n");
    printf("Total: %zu entries\n", result->count);
    printf("Errors: %zu, Warnings: %zu, Info: %zu\n",
           result->error_count, result->warning_count, result->info_count);

    if (result->count == 0) {
        printf("Configuration is valid.\n");
        return;
    }

    printf("\nDetails:\n");
    for (size_t i = 0; i < result->count; i++) {
        const char *severity;
        switch (result->entries[i].severity) {
            case 0: severity = "ERROR"; break;
            case 1: severity = "WARN"; break;
            case 2: severity = "INFO"; break;
            default: severity = "UNKNOWN"; break;
        }
        printf("[%s] %s: %s\n",
               severity,
               result->entries[i].path,
               result->entries[i].message);
    }
    printf("\n");
}

/* ============================================================================
 * Internal Helper Functions
 * ============================================================================ */

static bool add_entry(dds_validation_result_t *result, const char *path,
                      const char *message, int severity)
{
    if (!result) return false;

    /* Grow array if needed */
    if (result->count >= result->capacity) {
        size_t new_cap = result->capacity == 0 ? 16 : result->capacity * 2;
        dds_validation_entry_t *new_entries = realloc(result->entries,
                                                      new_cap * sizeof(dds_validation_entry_t));
        if (!new_entries) return false;
        result->entries = new_entries;
        result->capacity = new_cap;
    }

    /* Add entry */
    dds_validation_entry_t *entry = &result->entries[result->count++];
    strncpy(entry->path, path ? path : "", sizeof(entry->path) - 1);
    entry->path[sizeof(entry->path) - 1] = '\0';
    strncpy(entry->message, message ? message : "", sizeof(entry->message) - 1);
    entry->message[sizeof(entry->message) - 1] = '\0';
    entry->severity = severity;

    /* Update counts */
    switch (severity) {
        case 0: result->error_count++; break;
        case 1: result->warning_count++; break;
        case 2: result->info_count++; break;
    }

    return true;
}

static bool is_valid_name(const char *name, size_t max_len)
{
    if (!name || name[0] == '\0') return false;
    if (strlen(name) > max_len) return false;

    /* Must start with letter or underscore */
    if (!isalpha((unsigned char)name[0]) && name[0] != '_') return false;

    /* Rest can be alphanumeric or underscore */
    for (size_t i = 1; name[i]; i++) {
        if (!isalnum((unsigned char)name[i]) && name[i] != '_') return false;
    }

    return true;
}

static bool is_valid_topic_name_char(char c)
{
    /* Topic names can contain letters, digits, underscores, and forward slashes */
    return isalnum((unsigned char)c) || c == '_' || c == '/';
}

/* ============================================================================
 * Validation Functions
 * ============================================================================ */

dds_validation_result_t* dds_config_validate(
    const dds_configuration_t *config,
    const dds_validation_options_t *options)
{
    dds_validation_result_t *result = dds_validation_result_create();
    if (!result) return NULL;

    dds_validation_options_t opts = options ? *options : DDS_VALIDATION_DEFAULT_OPTIONS;

    if (!config) {
        add_entry(result, "", "Configuration is NULL", 0);
        return result;
    }

    /* Validate version */
    if (config->version[0] == '\0') {
        add_entry(result, "version", "Version is required", 1);
    }

    /* Validate domains */
    for (size_t i = 0; i < config->domain_count; i++) {
        char path[256];
        snprintf(path, sizeof(path), "domains[%zu]", i);
        dds_validation_result_t *domain_result = dds_config_validate_domain(
            &config->domains[i], &opts);
        if (domain_result) {
            for (size_t j = 0; j < domain_result->count; j++) {
                char full_path[512];
                snprintf(full_path, sizeof(full_path), "%s.%s",
                         path, domain_result->entries[j].path);
                add_entry(result, full_path,
                          domain_result->entries[j].message,
                          domain_result->entries[j].severity);
            }
            dds_validation_result_destroy(domain_result);
        }
    }

    /* Validate QoS profiles */
    for (size_t i = 0; i < config->qos_profile_count; i++) {
        char path[256];
        snprintf(path, sizeof(path), "qos_profiles[%zu]", i);
        dds_validation_result_t *qos_result = dds_config_validate_qos_profile(
            &config->qos_profiles[i], &opts);
        if (qos_result) {
            for (size_t j = 0; j < qos_result->count; j++) {
                char full_path[512];
                snprintf(full_path, sizeof(full_path), "%s.%s",
                         path, qos_result->entries[j].path);
                add_entry(result, full_path,
                          qos_result->entries[j].message,
                          qos_result->entries[j].severity);
            }
            dds_validation_result_destroy(qos_result);
        }
    }

    /* Validate topics */
    for (size_t i = 0; i < config->topic_count; i++) {
        char path[256];
        snprintf(path, sizeof(path), "topics[%zu]", i);
        dds_validation_result_t *topic_result = dds_config_validate_topic(
            &config->topics[i], config, &opts);
        if (topic_result) {
            for (size_t j = 0; j < topic_result->count; j++) {
                char full_path[512];
                snprintf(full_path, sizeof(full_path), "%s.%s",
                         path, topic_result->entries[j].path);
                add_entry(result, full_path,
                          topic_result->entries[j].message,
                          topic_result->entries[j].severity);
            }
            dds_validation_result_destroy(topic_result);
        }
    }

    /* Validate participants */
    for (size_t i = 0; i < config->participant_count; i++) {
        char path[256];
        snprintf(path, sizeof(path), "participants[%zu]", i);
        dds_validation_result_t *part_result = dds_config_validate_participant(
            &config->participants[i], config, &opts);
        if (part_result) {
            for (size_t j = 0; j < part_result->count; j++) {
                char full_path[512];
                snprintf(full_path, sizeof(full_path), "%s.%s",
                         path, part_result->entries[j].path);
                add_entry(result, full_path,
                          part_result->entries[j].message,
                          part_result->entries[j].severity);
            }
            dds_validation_result_destroy(part_result);
        }
    }

    /* Consistency checks */
    if (opts.check_consistency) {
        dds_validation_result_t *ref_result = dds_config_check_references(config);
        if (ref_result) {
            for (size_t j = 0; j < ref_result->count; j++) {
                add_entry(result, ref_result->entries[j].path,
                          ref_result->entries[j].message,
                          ref_result->entries[j].severity);
            }
            dds_validation_result_destroy(ref_result);
        }
    }

    /* QoS consistency checks */
    if (opts.check_consistency) {
        dds_validation_result_t *qos_cons_result = dds_config_check_qos_consistency(config);
        if (qos_cons_result) {
            for (size_t j = 0; j < qos_cons_result->count; j++) {
                add_entry(result, qos_cons_result->entries[j].path,
                          qos_cons_result->entries[j].message,
                          qos_cons_result->entries[j].severity);
            }
            dds_validation_result_destroy(qos_cons_result);
        }
    }

    return result;
}

dds_validation_result_t* dds_config_validate_domain(
    const dds_domain_config_t *domain,
    const dds_validation_options_t *options)
{
    dds_validation_result_t *result = dds_validation_result_create();
    if (!result) return NULL;

    if (!domain) {
        add_entry(result, "", "Domain is NULL", 0);
        return result;
    }

    /* Validate domain ID */
    if (!dds_config_is_valid_domain_id(domain->domain_id)) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Invalid domain ID: %u (valid range: 0-230)",
                 domain->domain_id);
        add_entry(result, "domain_id", msg, 0);
    }

    /* Validate name */
    if (domain->name[0] == '\0') {
        add_entry(result, "name", "Domain name is empty", 1);
    } else if (!is_valid_name(domain->name, DDS_CONFIG_MAX_NAME_LEN)) {
        add_entry(result, "name", "Domain name contains invalid characters", 1);
    }

    /* Validate transport configuration */
    if (options && options->check_performance) {
        if (domain->transport.multicast_enabled && domain->transport.multicast_address[0] == '\0') {
            add_entry(result, "transport.multicast_address",
                      "Multicast enabled but no multicast address specified", 1);
        }
    }

    /* Validate security configuration */
    if (options && options->check_security) {
        if (domain->security.enabled) {
            if (domain->security.auth_kind == DDS_AUTH_PKI_DH) {
                if (domain->security.identity_certificate[0] == '\0') {
                    add_entry(result, "security.identity_certificate",
                              "Identity certificate required when using PKI_DH authentication", 1);
                }
                if (domain->security.private_key[0] == '\0') {
                    add_entry(result, "security.private_key",
                              "Private key required when using PKI_DH authentication", 1);
                }
            }
        }
    }

    return result;
}

dds_validation_result_t* dds_config_validate_topic(
    const dds_topic_config_t *topic,
    const dds_configuration_t *context,
    const dds_validation_options_t *options)
{
    dds_validation_result_t *result = dds_validation_result_create();
    if (!result) return NULL;

    (void)options;

    if (!topic) {
        add_entry(result, "", "Topic is NULL", 0);
        return result;
    }

    /* Validate topic name */
    if (topic->name[0] == '\0') {
        add_entry(result, "name", "Topic name is required", 0);
    } else if (!dds_config_is_valid_topic_name(topic->name)) {
        add_entry(result, "name", "Topic name contains invalid characters", 0);
    }

    /* Validate type name */
    if (topic->type_name[0] == '\0') {
        add_entry(result, "type_name", "Type name is required", 0);
    } else if (!dds_config_is_valid_type_name(topic->type_name)) {
        add_entry(result, "type_name", "Type name contains invalid characters", 0);
    }

    /* Validate domain ID reference */
    if (context) {
        bool domain_found = false;
        for (size_t i = 0; i < context->domain_count; i++) {
            if (context->domains[i].domain_id == topic->domain_id) {
                domain_found = true;
                break;
            }
        }
        if (!domain_found) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Referenced domain ID %u does not exist",
                     topic->domain_id);
            add_entry(result, "domain_id", msg, 1);
        }
    }

    /* Validate QoS profile reference */
    if (topic->qos_profile[0] != '\0' && context) {
        bool profile_found = false;
        for (size_t i = 0; i < context->qos_profile_count; i++) {
            if (strcmp(context->qos_profiles[i].name, topic->qos_profile) == 0) {
                profile_found = true;
                break;
            }
        }
        if (!profile_found) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Referenced QoS profile '%s' does not exist",
                     topic->qos_profile);
            add_entry(result, "qos_profile", msg, 1);
        }
    }

    return result;
}

dds_validation_result_t* dds_config_validate_qos_profile(
    const dds_qos_profile_t *profile,
    const dds_validation_options_t *options)
{
    dds_validation_result_t *result = dds_validation_result_create();
    if (!result) return NULL;

    if (!profile) {
        add_entry(result, "", "QoS profile is NULL", 0);
        return result;
    }

    /* Validate name */
    if (profile->name[0] == '\0') {
        add_entry(result, "name", "QoS profile name is required", 0);
    }

    /* Validate QoS combinations */
    char error_msg[512];
    if (!dds_config_is_valid_qos_combination(profile, error_msg, sizeof(error_msg))) {
        add_entry(result, "", error_msg, 1);
    }

    /* Performance checks */
    if (options && options->check_performance) {
        /* Check for potentially problematic QoS settings */
        if (profile->has_history && profile->history.kind == DDS_HISTORY_KEEP_ALL) {
            if (!profile->has_resource_limits) {
                add_entry(result, "resource_limits",
                          "KEEP_ALL history without resource limits may cause memory issues", 1);
            }
        }

        if (profile->has_durability &&
            (profile->durability.kind == DDS_DURABILITY_TRANSIENT ||
             profile->durability.kind == DDS_DURABILITY_PERSISTENT)) {
            add_entry(result, "durability",
                      "TRANSIENT/PERSISTENT durability may impact performance", 2);
        }
    }

    return result;
}

dds_validation_result_t* dds_config_validate_participant(
    const dds_participant_config_t *participant,
    const dds_configuration_t *context,
    const dds_validation_options_t *options)
{
    dds_validation_result_t *result = dds_validation_result_create();
    if (!result) return NULL;

    (void)options;

    if (!participant) {
        add_entry(result, "", "Participant is NULL", 0);
        return result;
    }

    /* Validate name */
    if (participant->name[0] == '\0') {
        add_entry(result, "name", "Participant name is required", 0);
    }

    /* Validate domain ID */
    if (!dds_config_is_valid_domain_id(participant->domain_id)) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Invalid domain ID: %u", participant->domain_id);
        add_entry(result, "domain_id", msg, 0);
    }

    /* Validate domain reference */
    if (context) {
        bool domain_found = false;
        for (size_t i = 0; i < context->domain_count; i++) {
            if (context->domains[i].domain_id == participant->domain_id) {
                domain_found = true;
                break;
            }
        }
        if (!domain_found) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Referenced domain ID %u does not exist",
                     participant->domain_id);
            add_entry(result, "domain_id", msg, 1);
        }
    }

    /* Validate QoS profile reference */
    if (participant->qos_profile[0] != '\0' && context) {
        bool profile_found = false;
        for (size_t i = 0; i < context->qos_profile_count; i++) {
            if (strcmp(context->qos_profiles[i].name, participant->qos_profile) == 0) {
                profile_found = true;
                break;
            }
        }
        if (!profile_found) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Referenced QoS profile '%s' does not exist",
                     participant->qos_profile);
            add_entry(result, "qos_profile", msg, 1);
        }
    }

    return result;
}

/* ============================================================================
 * Schema Validation
 * ============================================================================ */

dds_validation_result_t* dds_config_validate_against_schema(
    const dds_configuration_t *config,
    const char *schema_file)
{
    dds_validation_result_t *result = dds_validation_result_create();
    if (!result) return NULL;

    (void)config;
    (void)schema_file;

    /* Schema validation not implemented - would need a JSON Schema parser */
    add_entry(result, "", "Schema validation not implemented", 2);

    return result;
}

/* ============================================================================
 * Specific Validation Checks
 * ============================================================================ */

bool dds_config_is_valid_domain_id(uint32_t domain_id)
{
    return domain_id <= DDS_DOMAIN_ID_MAX;
}

bool dds_config_is_valid_topic_name(const char *name)
{
    if (!name || name[0] == '\0') return false;

    /* Check for reserved names */
    if (strcmp(name, "DCPSParticipant") == 0 ||
        strcmp(name, "DCPSTopic") == 0 ||
        strcmp(name, "DCPSPublication") == 0 ||
        strcmp(name, "DCPSSubscription") == 0) {
        return false;
    }

    /* Topic names can contain letters, digits, underscores, and forward slashes */
    /* Must not start with a digit */
    if (isdigit((unsigned char)name[0])) return false;

    for (size_t i = 0; name[i]; i++) {
        if (!is_valid_topic_name_char(name[i])) return false;
    }

    return true;
}

bool dds_config_is_valid_type_name(const char *name)
{
    /* Type names follow similar rules to topic names */
    return is_valid_name(name, DDS_CONFIG_MAX_NAME_LEN);
}

bool dds_config_is_valid_qos_combination(
    const dds_qos_profile_t *profile,
    char *error_msg,
    size_t error_msg_size)
{
    if (!profile) {
        snprintf(error_msg, error_msg_size, "Profile is NULL");
        return false;
    }

    /* Check deadline and time-based filter compatibility */
    if (profile->has_deadline) {
        /* Deadline is valid with any reliability */
    }

    /* Check ownership and ownership strength */
    if (profile->has_ownership) {
        if (profile->ownership.kind == DDS_OWNERSHIP_EXCLUSIVE) {
            /* Ownership strength should be specified for exclusive ownership */
            if (!profile->has_ownership_strength) {
                snprintf(error_msg, error_msg_size,
                         "EXCLUSIVE ownership without ownership_strength may cause issues");
                return false;
            }
        }
    }

    /* Check durability and history compatibility */
    if (profile->has_durability && profile->has_history) {
        if (profile->durability.kind == DDS_DURABILITY_VOLATILE &&
            profile->history.kind == DDS_HISTORY_KEEP_ALL) {
            /* This combination is allowed but may not be useful */
        }
    }

    error_msg[0] = '\0';
    return true;
}

/* ============================================================================
 * Consistency Checks
 * ============================================================================ */

dds_validation_result_t* dds_config_check_references(
    const dds_configuration_t *config)
{
    dds_validation_result_t *result = dds_validation_result_create();
    if (!result) return NULL;

    if (!config) {
        add_entry(result, "", "Configuration is NULL", 0);
        return result;
    }

    /* Check for duplicate domain IDs */
    for (size_t i = 0; i < config->domain_count; i++) {
        for (size_t j = i + 1; j < config->domain_count; j++) {
            if (config->domains[i].domain_id == config->domains[j].domain_id) {
                char msg[256];
                snprintf(msg, sizeof(msg),
                         "Duplicate domain ID: %u", config->domains[i].domain_id);
                add_entry(result, "domains", msg, 0);
            }
        }
    }

    /* Check for duplicate QoS profile names */
    for (size_t i = 0; i < config->qos_profile_count; i++) {
        for (size_t j = i + 1; j < config->qos_profile_count; j++) {
            if (strcmp(config->qos_profiles[i].name,
                       config->qos_profiles[j].name) == 0) {
                char msg[256];
                snprintf(msg, sizeof(msg),
                         "Duplicate QoS profile name: %s",
                         config->qos_profiles[i].name);
                add_entry(result, "qos_profiles", msg, 0);
            }
        }
    }

    /* Check for duplicate topic names (within same domain) */
    for (size_t i = 0; i < config->topic_count; i++) {
        for (size_t j = i + 1; j < config->topic_count; j++) {
            if (strcmp(config->topics[i].name, config->topics[j].name) == 0 &&
                config->topics[i].domain_id == config->topics[j].domain_id) {
                char msg[256];
                snprintf(msg, sizeof(msg),
                         "Duplicate topic name '%s' in domain %u",
                         config->topics[i].name, config->topics[i].domain_id);
                add_entry(result, "topics", msg, 0);
            }
        }
    }

    /* Check for duplicate participant names */
    for (size_t i = 0; i < config->participant_count; i++) {
        for (size_t j = i + 1; j < config->participant_count; j++) {
            if (strcmp(config->participants[i].name,
                       config->participants[j].name) == 0) {
                char msg[256];
                snprintf(msg, sizeof(msg),
                         "Duplicate participant name: %s",
                         config->participants[i].name);
                add_entry(result, "participants", msg, 1);
            }
        }
    }

    return result;
}

dds_validation_result_t* dds_config_check_qos_consistency(
    const dds_configuration_t *config)
{
    dds_validation_result_t *result = dds_validation_result_create();
    if (!result) return NULL;

    if (!config) {
        add_entry(result, "", "Configuration is NULL", 0);
        return result;
    }

    /* Check for topics using QoS profiles that don't exist */
    for (size_t i = 0; i < config->topic_count; i++) {
        if (config->topics[i].qos_profile[0] != '\0') {
            bool found = false;
            for (size_t j = 0; j < config->qos_profile_count; j++) {
                if (strcmp(config->topics[i].qos_profile,
                           config->qos_profiles[j].name) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                char msg[256];
                snprintf(msg, sizeof(msg),
                         "Topic '%s' references non-existent QoS profile '%s'",
                         config->topics[i].name, config->topics[i].qos_profile);
                add_entry(result, "topics", msg, 0);
            }
        }
    }

    /* Check for participants using QoS profiles that don't exist */
    for (size_t i = 0; i < config->participant_count; i++) {
        if (config->participants[i].qos_profile[0] != '\0') {
            bool found = false;
            for (size_t j = 0; j < config->qos_profile_count; j++) {
                if (strcmp(config->participants[i].qos_profile,
                           config->qos_profiles[j].name) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                char msg[256];
                snprintf(msg, sizeof(msg),
                         "Participant '%s' references non-existent QoS profile '%s'",
                         config->participants[i].name, config->participants[i].qos_profile);
                add_entry(result, "participants", msg, 0);
            }
        }
    }

    return result;
}

dds_validation_result_t* dds_config_check_domain_references(
    const dds_configuration_t *config)
{
    dds_validation_result_t *result = dds_validation_result_create();
    if (!result) return NULL;

    if (!config) {
        add_entry(result, "", "Configuration is NULL", 0);
        return result;
    }

    /* Check that all referenced domain IDs exist */
    for (size_t i = 0; i < config->topic_count; i++) {
        bool found = false;
        for (size_t j = 0; j < config->domain_count; j++) {
            if (config->topics[i].domain_id == config->domains[j].domain_id) {
                found = true;
                break;
            }
        }
        if (!found) {
            char msg[256];
            snprintf(msg, sizeof(msg),
                     "Topic '%s' references non-existent domain ID %u",
                     config->topics[i].name, config->topics[i].domain_id);
            add_entry(result, "topics", msg, 0);
        }
    }

    for (size_t i = 0; i < config->participant_count; i++) {
        bool found = false;
        for (size_t j = 0; j < config->domain_count; j++) {
            if (config->participants[i].domain_id == config->domains[j].domain_id) {
                found = true;
                break;
            }
        }
        if (!found) {
            char msg[256];
            snprintf(msg, sizeof(msg),
                     "Participant '%s' references non-existent domain ID %u",
                     config->participants[i].name, config->participants[i].domain_id);
            add_entry(result, "participants", msg, 0);
        }
    }

    return result;
}
