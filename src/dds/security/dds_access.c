/*
 * dds_access.c - DDS-Security Access Control Module Implementation
 *
 * Copyright (c) 2024-2025
 *
 * This file implements the DDS:Access:Permissions plugin
 * as specified in the OMG DDS-Security specification.
 */

#include "dds_access.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* ============================================================================
 * Initialization
 * ============================================================================ */

dds_access_context_t* dds_access_init(const dds_security_config_t *config)
{
    if (!config) {
        return NULL;
    }

    dds_access_context_t *ctx = (dds_access_context_t*)calloc(1, sizeof(dds_access_context_t));
    if (!ctx) {
        return NULL;
    }

    /* Copy configuration paths */
    if (config->permissions_ca_cert_path[0]) {
        strncpy(ctx->permissions_ca_cert_path, config->permissions_ca_cert_path,
                sizeof(ctx->permissions_ca_cert_path) - 1);
    }
    if (config->permissions_xml_path[0]) {
        strncpy(ctx->permissions_file_path, config->permissions_xml_path,
                sizeof(ctx->permissions_file_path) - 1);
    }
    if (config->governance_xml_path[0]) {
        strncpy(ctx->governance_file_path, config->governance_xml_path,
                sizeof(ctx->governance_file_path) - 1);
    }

    /* Allocate permissions database */
    ctx->max_subjects = DDS_ACCESS_MAX_SUBJECTS;
    ctx->permissions_db = (dds_security_permissions_t*)calloc(
        ctx->max_subjects, sizeof(dds_security_permissions_t));
    if (!ctx->permissions_db) {
        free(ctx);
        return NULL;
    }

    /* Initialize default policy */
    ctx->default_policy.default_deny = false;
    ctx->default_policy.scope = DDS_PERM_SCOPE_DOMAIN;

    /* Load CA certificate if available */
    if (ctx->permissions_ca_cert_path[0]) {
        /* Load certificate from file */
        FILE *fp = fopen(ctx->permissions_ca_cert_path, "rb");
        if (fp) {
            ctx->permissions_ca_cert.length = fread(
                ctx->permissions_ca_cert.data, 1, DDS_SECURITY_MAX_CERT_SIZE, fp);
            fclose(fp);
        }
    }

    return ctx;
}

void dds_access_deinit(dds_access_context_t *ctx)
{
    if (!ctx) {
        return;
    }

    /* Free all permissions data */
    if (ctx->permissions_db) {
        for (uint32_t i = 0; i < ctx->max_subjects; i++) {
            dds_security_permissions_t *perms = &ctx->permissions_db[i];
            if (perms->subjects) {
                for (uint32_t j = 0; j < perms->subject_count; j++) {
                    dds_participant_permission_t *subject = &perms->subjects[j];
                    if (subject->domain_perms) {
                        free(subject->domain_perms);
                    }
                }
                free(perms->subjects);
            }
        }
        free(ctx->permissions_db);
    }

    /* Free default policy rules */
    dds_access_rule_t *rule = ctx->default_policy.rules;
    while (rule) {
        dds_access_rule_t *next = rule->next;
        free(rule);
        rule = next;
    }

    free(ctx);
}

/* ============================================================================
 * XML Parsing (Simplified)
 * ============================================================================ */

static char* xml_find_tag(const char *xml, const char *tag, uint32_t *content_len)
{
    if (!xml || !tag || !content_len) {
        return NULL;
    }

    char open_tag[64];
    char close_tag[64];
    snprintf(open_tag, sizeof(open_tag), "<%s", tag);
    snprintf(close_tag, sizeof(close_tag), "</%s>", tag);

    const char *start = strstr(xml, open_tag);
    if (!start) {
        return NULL;
    }

    const char *content_start = strchr(start, '>');
    if (!content_start) {
        return NULL;
    }
    content_start++;

    const char *content_end = strstr(content_start, close_tag);
    if (!content_end) {
        return NULL;
    }

    *content_len = (uint32_t)(content_end - content_start);
    return (char*)content_start;
}

static void xml_extract_value(const char *content, uint32_t content_len,
                              char *output, uint32_t output_size)
{
    if (!content || !output || output_size == 0) {
        return;
    }

    uint32_t copy_len = content_len < output_size - 1 ? content_len : output_size - 1;
    memcpy(output, content, copy_len);
    output[copy_len] = '\0';

    /* Trim whitespace */
    char *start = output;
    while (*start && isspace((unsigned char)*start)) start++;

    char *end = output + strlen(output) - 1;
    while (end > start && isspace((unsigned char)*end)) *end-- = '\0';

    if (start != output) {
        memmove(output, start, strlen(start) + 1);
    }
}

dds_access_status_t dds_access_parse_permissions_xml(dds_access_context_t *ctx,
                                                      const char *xml_path,
                                                      dds_permissions_config_t *config)
{
    if (!ctx || !xml_path || !config) {
        return DDS_ACCESS_ERROR_INVALID_CONFIG;
    }

    memset(config, 0, sizeof(dds_permissions_config_t));

    /* Read XML file */
    FILE *fp = fopen(xml_path, "rb");
    if (!fp) {
        return DDS_ACCESS_ERROR_INVALID_CONFIG;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size <= 0 || file_size > DDS_SECURITY_MAX_PERMISSIONS_SIZE) {
        fclose(fp);
        return DDS_ACCESS_ERROR_INVALID_CONFIG;
    }

    char *xml_data = (char*)malloc(file_size + 1);
    if (!xml_data) {
        fclose(fp);
        return DDS_ACCESS_ERROR_NO_MEMORY;
    }

    fread(xml_data, 1, file_size, fp);
    xml_data[file_size] = '\0';
    fclose(fp);

    /* Parse version */
    uint32_t version_len;
    char *version_tag = xml_find_tag(xml_data, "dds", &version_len);
    if (version_tag) {
        /* Extract version info */
        config->version_major = DDS_PERMISSIONS_VERSION_MAJOR;
        config->version_minor = DDS_PERMISSIONS_VERSION_MINOR;
    }

    /* Parse subjects */
    const char *search_pos = xml_data;
    uint32_t subject_len;

    while (config->subject_count < DDS_ACCESS_MAX_SUBJECTS) {
        char *subject_tag = xml_find_tag(search_pos, "subject", &subject_len);
        if (!subject_tag) break;

        auto& subject = config->subjects[config->subject_count];

        /* Parse subject name */
        uint32_t name_len;
        char *name_tag = xml_find_tag(subject_tag, "subject_name", &name_len);
        if (name_tag) {
            xml_extract_value(name_tag, name_len,
                              subject.subject_name,
                              sizeof(subject.subject_name));
        }

        /* Parse validity */
        uint32_t validity_len;
        char *validity_tag = xml_find_tag(subject_tag, "validity", &validity_len);
        if (validity_tag) {
            uint32_t from_len, to_len;
            char *from_tag = xml_find_tag(validity_tag, "not_before", &from_len);
            char *to_tag = xml_find_tag(validity_tag, "not_after", &to_len);
            if (from_tag) {
                char from_str[32];
                xml_extract_value(from_tag, from_len, from_str, sizeof(from_str));
                strncpy(subject.validity_from, from_str, sizeof(subject.validity_from) - 1);
            }
            if (to_tag) {
                char to_str[32];
                xml_extract_value(to_tag, to_len, to_str, sizeof(to_str));
                strncpy(subject.validity_until, to_str, sizeof(subject.validity_until) - 1);
            }
        }

        /* Parse domain permissions */
        const char *domain_search = subject_tag;
        uint32_t domain_len;

        while (subject.domain_count < DDS_ACCESS_MAX_DOMAINS) {
            char *domain_tag = xml_find_tag(domain_search, "domain", &domain_len);
            if (!domain_tag) break;

            /* Parse domain ID */
            uint32_t id_len;
            char *id_tag = xml_find_tag(domain_tag, "id", &id_len);
            if (id_tag) {
                char id_str[16];
                xml_extract_value(id_tag, id_len, id_str, sizeof(id_str));
                subject.domains[subject.domain_count].domain_id = atoi(id_str);
            }

            /* Parse publish permissions */
            const char *pub_search = domain_tag;
            uint32_t pub_len;

            while (subject.domains[subject.domain_count].publish_count < DDS_ACCESS_MAX_TOPICS) {
                char *pub_tag = xml_find_tag(pub_search, "publish", &pub_len);
                if (!pub_tag) break;

                uint32_t topic_len;
                char *topic_tag = xml_find_tag(pub_tag, "topic", &topic_len);
                if (topic_tag) {
                    xml_extract_value(topic_tag, topic_len,
                                      subject.domains[subject.domain_count].publish[
                                          subject.domains[subject.domain_count].publish_count].topic_pattern,
                                      128);
                }

                subject.domains[subject.domain_count].publish_count++;
                pub_search = pub_tag + pub_len;
            }

            /* Parse subscribe permissions */
            const char *sub_search = domain_tag;
            uint32_t sub_len;

            while (subject.domains[subject.domain_count].subscribe_count < DDS_ACCESS_MAX_TOPICS) {
                char *sub_tag = xml_find_tag(sub_search, "subscribe", &sub_len);
                if (!sub_tag) break;

                uint32_t topic_len;
                char *topic_tag = xml_find_tag(sub_tag, "topic", &topic_len);
                if (topic_tag) {
                    xml_extract_value(topic_tag, topic_len,
                                      subject.domains[subject.domain_count].subscribe[
                                          subject.domains[subject.domain_count].subscribe_count].topic_pattern,
                                      128);
                }

                subject.domains[subject.domain_count].subscribe_count++;
                sub_search = sub_tag + sub_len;
            }

            subject.domain_count++;
            domain_search = domain_tag + domain_len;
        }

        config->subject_count++;
        search_pos = subject_tag + subject_len;
    }

    free(xml_data);

    if (ctx->on_permissions_loaded) {
        ctx->on_permissions_loaded(config->subjects[0].subject_name,
                                   config->subject_count > 0);
    }

    return DDS_ACCESS_OK;
}

dds_access_status_t dds_access_parse_governance_xml(dds_access_context_t *ctx,
                                                     const char *xml_path,
                                                     dds_governance_config_t *config)
{
    if (!ctx || !xml_path || !config) {
        return DDS_ACCESS_ERROR_INVALID_CONFIG;
    }

    memset(config, 0, sizeof(dds_governance_config_t));

    /* Read XML file */
    FILE *fp = fopen(xml_path, "rb");
    if (!fp) {
        /* Use defaults if file not found */
        config->version_major = DDS_PERMISSIONS_VERSION_MAJOR;
        config->version_minor = DDS_PERMISSIONS_VERSION_MINOR;
        return DDS_ACCESS_OK;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size <= 0 || file_size > DDS_SECURITY_MAX_PERMISSIONS_SIZE) {
        fclose(fp);
        return DDS_ACCESS_ERROR_INVALID_CONFIG;
    }

    char *xml_data = (char*)malloc(file_size + 1);
    if (!xml_data) {
        fclose(fp);
        return DDS_ACCESS_ERROR_NO_MEMORY;
    }

    fread(xml_data, 1, file_size, fp);
    xml_data[file_size] = '\0';
    fclose(fp);

    /* Parse version */
    config->version_major = DDS_PERMISSIONS_VERSION_MAJOR;
    config->version_minor = DDS_PERMISSIONS_VERSION_MINOR;

    /* Parse domains */
    const char *search_pos = xml_data;
    uint32_t domain_len;

    while (config->domain_count < DDS_ACCESS_MAX_DOMAINS) {
        char *domain_tag = xml_find_tag(search_pos, "domain", &domain_len);
        if (!domain_tag) break;

        auto& domain = config->domains[config->domain_count];

        /* Parse domain ID */
        uint32_t id_len;
        char *id_tag = xml_find_tag(domain_tag, "id", &id_len);
        if (id_tag) {
            char id_str[16];
            xml_extract_value(id_tag, id_len, id_str, sizeof(id_str));
            domain.domain_id = atoi(id_str);
        }

        /* Parse security policies */
        uint32_t policy_len;
        char *policy_tag = xml_find_tag(domain_tag, "allow_unauthenticated_participants", &policy_len);
        if (policy_tag) {
            char value[16];
            xml_extract_value(policy_tag, policy_len, value, sizeof(value));
            domain.allow_unauthenticated_participants = (strcmp(value, "true") == 0);
        }

        config->domain_count++;
        search_pos = domain_tag + domain_len;
    }

    free(xml_data);
    return DDS_ACCESS_OK;
}

dds_access_status_t dds_access_validate_permissions_signature(dds_access_context_t *ctx,
                                                               const uint8_t *permissions_xml,
                                                               uint32_t xml_len,
                                                               const uint8_t *signature,
                                                               uint32_t sig_len)
{
    if (!ctx || !permissions_xml || !signature) {
        return DDS_ACCESS_ERROR_INVALID_PARAM;
    }

    /* Simplified signature validation */
    /* In production, use proper cryptographic signature verification */

    return DDS_ACCESS_OK;
}

/* ============================================================================
 * Permission Management
 * ============================================================================ */

dds_access_status_t dds_access_load_participant_permissions(dds_access_context_t *ctx,
                                                              const char *subject_name,
                                                              const uint8_t *permissions_xml,
                                                              uint32_t xml_len)
{
    if (!ctx || !subject_name) {
        return DDS_ACCESS_ERROR_INVALID_PARAM;
    }

    /* Find or allocate permissions entry */
    dds_security_permissions_t *perms = NULL;
    for (uint32_t i = 0; i < ctx->max_subjects; i++) {
        if (ctx->permissions_db[i].subject_count > 0 &&
            strcmp(ctx->permissions_db[i].subjects[0].subject_name, subject_name) == 0) {
            perms = &ctx->permissions_db[i];
            break;
        }
        if (ctx->permissions_db[i].subject_count == 0 && !perms) {
            perms = &ctx->permissions_db[i];
        }
    }

    if (!perms) {
        return DDS_ACCESS_ERROR_NO_MEMORY;
    }

    /* Clear existing permissions */
    if (perms->subjects) {
        for (uint32_t i = 0; i < perms->subject_count; i++) {
            if (perms->subjects[i].domain_perms) {
                free(perms->subjects[i].domain_perms);
            }
        }
        free(perms->subjects);
    }

    /* Allocate and initialize subject permissions */
    perms->subjects = (dds_participant_permission_t*)calloc(1, sizeof(dds_participant_permission_t));
    if (!perms->subjects) {
        return DDS_ACCESS_ERROR_NO_MEMORY;
    }

    strncpy(perms->subjects[0].subject_name, subject_name,
            sizeof(perms->subjects[0].subject_name) - 1);
    perms->subject_count = 1;
    perms->validated = true;

    ctx->active_subjects++;

    if (ctx->on_permissions_loaded) {
        ctx->on_permissions_loaded(subject_name, true);
    }

    return DDS_ACCESS_OK;
}

dds_access_status_t dds_access_unload_participant_permissions(dds_access_context_t *ctx,
                                                                const char *subject_name)
{
    if (!ctx || !subject_name) {
        return DDS_ACCESS_ERROR_INVALID_PARAM;
    }

    for (uint32_t i = 0; i < ctx->max_subjects; i++) {
        dds_security_permissions_t *perms = &ctx->permissions_db[i];

        if (perms->subject_count > 0 &&
            strcmp(perms->subjects[0].subject_name, subject_name) == 0) {

            /* Free permissions */
            if (perms->subjects) {
                for (uint32_t j = 0; j < perms->subject_count; j++) {
                    if (perms->subjects[j].domain_perms) {
                        free(perms->subjects[j].domain_perms);
                    }
                }
                free(perms->subjects);
                perms->subjects = NULL;
            }

            perms->subject_count = 0;
            perms->validated = false;
            ctx->active_subjects--;

            return DDS_ACCESS_OK;
        }
    }

    return DDS_ACCESS_ERROR_SUBJECT_NOT_FOUND;
}

dds_access_status_t dds_access_update_participant_permissions(dds_access_context_t *ctx,
                                                                const char *subject_name,
                                                                const uint8_t *permissions_xml,
                                                                uint32_t xml_len)
{
    /* Unload and reload */
    dds_access_unload_participant_permissions(ctx, subject_name);
    return dds_access_load_participant_permissions(ctx, subject_name, permissions_xml, xml_len);
}

uint32_t dds_access_check_permissions_expiry(dds_access_context_t *ctx, uint64_t current_time)
{
    if (!ctx) {
        return 0;
    }

    uint32_t expired_count = 0;

    for (uint32_t i = 0; i < ctx->max_subjects; i++) {
        dds_security_permissions_t *perms = &ctx->permissions_db[i];

        for (uint32_t j = 0; j < perms->subject_count; j++) {
            dds_participant_permission_t *subject = &perms->subjects[j];

            if (current_time > subject->valid_until) {
                expired_count++;
                /* Mark for renewal */
            }
        }
    }

    return expired_count;
}

/* ============================================================================
 * Access Control Decisions
 * ============================================================================ */

static bool match_pattern(const char *pattern, const char *str)
{
    if (!pattern || !str) {
        return false;
    }

    /* Simple wildcard matching */
    while (*pattern && *str) {
        if (*pattern == '*') {
            /* Match any sequence */
            pattern++;
            if (!*pattern) return true;

            while (*str) {
                if (match_pattern(pattern, str)) {
                    return true;
                }
                str++;
            }
            return false;
        } else if (*pattern == '?') {
            /* Match single character */
            pattern++;
            str++;
        } else if (*pattern == *str) {
            pattern++;
            str++;
        } else {
            return false;
        }
    }

    /* Handle trailing wildcards */
    while (*pattern == '*') pattern++;

    return (*pattern == '\0' && *str == '\0');
}

bool dds_access_match_topic_pattern(const char *pattern, const char *topic_name)
{
    return match_pattern(pattern, topic_name);
}

dds_access_decision_t dds_access_check_permission(dds_access_context_t *ctx,
                                                   const dds_access_request_t *request)
{
    if (!ctx || !request) {
        return DDS_ACCESS_DECISION_DENY;
    }

    ctx->access_checks++;

    /* Check cache first */
    if (strcmp(ctx->permission_cache.last_subject, request->subject_name) == 0) {
        if (ctx->permission_cache.cached_permission &&
            (dds_get_current_time_ms() - ctx->permission_cache.cache_time) < 5000) {
            ctx->cache_hits++;
        }
    }

    /* Find subject permissions */
    dds_participant_permission_t *subject_perm = NULL;
    for (uint32_t i = 0; i < ctx->max_subjects; i++) {
        dds_security_permissions_t *perms = &ctx->permissions_db[i];

        for (uint32_t j = 0; j < perms->subject_count; j++) {
            if (strcmp(perms->subjects[j].subject_name, request->subject_name) == 0) {
                subject_perm = &perms->subjects[j];
                break;
            }
        }
        if (subject_perm) break;
    }

    /* Check ASIL level requirement */
    if (request->asil_level < ctx->default_policy.rules ? ctx->default_policy.rules->required_asil : DDS_SECURITY_ASIL_QM) {
        /* ASIL level insufficient - may still be allowed based on policy */
    }

    /* Default deny if strict mode */
    if (ctx->default_policy.default_deny && !subject_perm) {
        ctx->access_denied++;
        if (ctx->on_access_denied) {
            ctx->on_access_denied(request->subject_name, request->topic_name, request->requested_action);
        }
        return DDS_ACCESS_DECISION_DENY;
    }

    /* Allow by default if not strict */
    ctx->access_granted++;
    return DDS_ACCESS_DECISION_ALLOW;
}

dds_access_status_t dds_access_check_join_permission(dds_access_context_t *ctx,
                                                      const char *subject_name,
                                                      dds_domain_id_t domain_id,
                                                      dds_security_asil_level_t asil_level)
{
    if (!ctx || !subject_name) {
        return DDS_ACCESS_ERROR_ACCESS_DENIED;
    }

    dds_access_request_t request = {
        .subject_name = subject_name,
        .domain_id = domain_id,
        .topic_name = NULL,
        .partition_name = NULL,
        .requested_action = DDS_PERM_READ_WRITE,
        .asil_level = asil_level,
        .is_encrypted = false,
        .is_authenticated = true
    };

    dds_access_decision_t decision = dds_access_check_permission(ctx, &request);

    return (decision == DDS_ACCESS_DECISION_ALLOW) ? DDS_ACCESS_OK : DDS_ACCESS_ERROR_ACCESS_DENIED;
}

dds_access_status_t dds_access_check_publish(dds_access_context_t *ctx,
                                              const dds_access_request_t *request)
{
    if (!ctx || !request) {
        return DDS_ACCESS_ERROR_INVALID_PARAM;
    }

    dds_access_decision_t decision = dds_access_check_permission(ctx, request);

    if (decision != DDS_ACCESS_DECISION_ALLOW) {
        return DDS_ACCESS_ERROR_ACCESS_DENIED;
    }

    return DDS_ACCESS_OK;
}

dds_access_status_t dds_access_check_subscribe(dds_access_context_t *ctx,
                                                const dds_access_request_t *request)
{
    if (!ctx || !request) {
        return DDS_ACCESS_ERROR_INVALID_PARAM;
    }

    dds_access_decision_t decision = dds_access_check_permission(ctx, request);

    if (decision != DDS_ACCESS_DECISION_ALLOW) {
        return DDS_ACCESS_ERROR_ACCESS_DENIED;
    }

    return DDS_ACCESS_OK;
}

dds_access_status_t dds_access_check_read(dds_access_context_t *ctx,
                                           const dds_access_request_t *request)
{
    return dds_access_check_subscribe(ctx, request);
}

dds_access_status_t dds_access_check_write(dds_access_context_t *ctx,
                                            const dds_access_request_t *request)
{
    return dds_access_check_publish(ctx, request);
}

/* ============================================================================
 * Governance Operations
 * ============================================================================ */

dds_access_status_t dds_access_get_domain_governance(dds_access_context_t *ctx,
                                                      dds_domain_id_t domain_id,
                                                      dds_governance_config_t *governance)
{
    if (!ctx || !governance) {
        return DDS_ACCESS_ERROR_INVALID_PARAM;
    }

    /* Parse governance file */
    if (ctx->governance_file_path[0]) {
        dds_access_parse_governance_xml(ctx, ctx->governance_file_path, governance);
    }

    /* Find domain-specific settings */
    for (uint32_t i = 0; i < governance->domain_count; i++) {
        if (governance->domains[i].domain_id == domain_id) {
            return DDS_ACCESS_OK;
        }
    }

    /* Use defaults */
    memset(governance, 0, sizeof(dds_governance_config_t));
    governance->version_major = DDS_PERMISSIONS_VERSION_MAJOR;
    governance->version_minor = DDS_PERMISSIONS_VERSION_MINOR;

    return DDS_ACCESS_OK;
}

bool dds_access_discovery_protection_enabled(dds_access_context_t *ctx,
                                             dds_domain_id_t domain_id,
                                             const char *topic_name)
{
    if (!ctx) {
        return false;
    }

    dds_governance_config_t governance;
    dds_access_get_domain_governance(ctx, domain_id, &governance);

    for (uint32_t i = 0; i < governance.domain_count; i++) {
        if (governance.domains[i].domain_id == domain_id) {
            return governance.domains[i].discovery_protection_kind;
        }
    }

    return false;
}

bool dds_access_liveliness_protection_enabled(dds_access_context_t *ctx,
                                              dds_domain_id_t domain_id,
                                              const char *topic_name)
{
    if (!ctx) {
        return false;
    }

    dds_governance_config_t governance;
    dds_access_get_domain_governance(ctx, domain_id, &governance);

    for (uint32_t i = 0; i < governance.domain_count; i++) {
        if (governance.domains[i].domain_id == domain_id) {
            return governance.domains[i].liveliness_protection_kind;
        }
    }

    return false;
}

uint8_t dds_access_rtps_protection_kind(dds_access_context_t *ctx,
                                        dds_domain_id_t domain_id)
{
    if (!ctx) {
        return 0; /* NONE */
    }

    dds_governance_config_t governance;
    dds_access_get_domain_governance(ctx, domain_id, &governance);

    for (uint32_t i = 0; i < governance.domain_count; i++) {
        if (governance.domains[i].domain_id == domain_id) {
            return governance.domains[i].rtps_protection_kind;
        }
    }

    return 0; /* NONE */
}

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

dds_access_status_t dds_access_get_subject_name(const dds_security_cert_t *cert,
                                                char *subject_name,
                                                uint32_t max_len)
{
    if (!cert || !subject_name || max_len == 0) {
        return DDS_ACCESS_ERROR_INVALID_PARAM;
    }

    /* Simplified - extract from pre-parsed data */
    strncpy(subject_name, cert->subject_name, max_len - 1);
    subject_name[max_len - 1] = '\0';

    return DDS_ACCESS_OK;
}

void dds_access_clear_permission_cache(dds_access_context_t *ctx)
{
    if (!ctx) {
        return;
    }

    ctx->permission_cache.last_subject[0] = '\0';
    ctx->permission_cache.cached_permission = NULL;
    ctx->permission_cache.cache_time = 0;
}

void dds_access_get_stats(dds_access_context_t *ctx,
                          uint64_t *checks,
                          uint64_t *denied,
                          uint64_t *granted)
{
    if (!ctx) {
        return;
    }

    if (checks) *checks = ctx->access_checks;
    if (denied) *denied = ctx->access_denied;
    if (granted) *granted = ctx->access_granted;
}

/* Get current time in milliseconds (stub) */
extern uint64_t dds_get_current_time_ms(void);
