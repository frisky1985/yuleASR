/*
 * DDS Configuration Parser - Implementation
 *
 * Copyright (c) 2024 DDS Config Tool Authors
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "dds_config_types.h"
#include "dds_config_parser.h"

/* ============================================================================
 * Utility Functions Implementation (from dds_config_types.h)
 * ============================================================================ */

const char* dds_config_error_string(dds_config_error_t error)
{
    switch (error) {
        case DDS_CONFIG_OK: return "Success";
        case DDS_CONFIG_ERR_INVALID_FORMAT: return "Invalid format";
        case DDS_CONFIG_ERR_SCHEMA_VIOLATION: return "Schema violation";
        case DDS_CONFIG_ERR_INVALID_DOMAIN: return "Invalid domain";
        case DDS_CONFIG_ERR_DUPLICATE_TOPIC: return "Duplicate topic";
        case DDS_CONFIG_ERR_INVALID_QOS: return "Invalid QoS";
        case DDS_CONFIG_ERR_FILE_IO: return "File I/O error";
        case DDS_CONFIG_ERR_MEMORY: return "Memory allocation error";
        case DDS_CONFIG_ERR_NOT_FOUND: return "Not found";
        case DDS_CONFIG_ERR_INVALID_ARGUMENT: return "Invalid argument";
        case DDS_CONFIG_ERR_PARSE_ERROR: return "Parse error";
        case DDS_CONFIG_ERR_VALIDATION_FAILED: return "Validation failed";
        case DDS_CONFIG_ERR_INTERNAL: return "Internal error";
        default: return "Unknown error";
    }
}

dds_duration_t dds_duration_from_string(const char *str)
{
    dds_duration_t dur = {0, 0, false};
    if (!str || str[0] == '\0') return dur;

    if (strcmp(str, "infinite") == 0 || strcmp(str, "INF") == 0) {
        dur.infinite = true;
        return dur;
    }

    char *endptr;
    double val = strtod(str, &endptr);

    if (endptr == str) return dur;

    if (strcmp(endptr, "ns") == 0) {
        dur.sec = 0;
        dur.nanosec = (uint32_t)(val);
    } else if (strcmp(endptr, "us") == 0) {
        dur.sec = 0;
        dur.nanosec = (uint32_t)(val * 1000);
    } else if (strcmp(endptr, "ms") == 0) {
        dur.sec = (int64_t)(val / 1000);
        dur.nanosec = (uint32_t)((val - dur.sec * 1000) * 1000000);
    } else if (strcmp(endptr, "s") == 0) {
        dur.sec = (int64_t)val;
        dur.nanosec = (uint32_t)((val - dur.sec) * 1000000000);
    } else if (strcmp(endptr, "m") == 0) {
        dur.sec = (int64_t)(val * 60);
    } else if (strcmp(endptr, "h") == 0) {
        dur.sec = (int64_t)(val * 3600);
    } else {
        dur.sec = (int64_t)val;
    }

    return dur;
}

const char* dds_qos_reliability_to_string(dds_reliability_kind_t kind)
{
    switch (kind) {
        case DDS_RELIABILITY_BEST_EFFORT: return "BEST_EFFORT";
        case DDS_RELIABILITY_RELIABLE: return "RELIABLE";
        default: return "UNKNOWN";
    }
}

const char* dds_qos_durability_to_string(dds_durability_kind_t kind)
{
    switch (kind) {
        case DDS_DURABILITY_VOLATILE: return "VOLATILE";
        case DDS_DURABILITY_TRANSIENT_LOCAL: return "TRANSIENT_LOCAL";
        case DDS_DURABILITY_TRANSIENT: return "TRANSIENT";
        case DDS_DURABILITY_PERSISTENT: return "PERSISTENT";
        default: return "UNKNOWN";
    }
}

const char* dds_qos_history_to_string(dds_history_kind_t kind)
{
    switch (kind) {
        case DDS_HISTORY_KEEP_LAST: return "KEEP_LAST";
        case DDS_HISTORY_KEEP_ALL: return "KEEP_ALL";
        default: return "UNKNOWN";
    }
}

const char* dds_qos_ownership_to_string(dds_ownership_kind_t kind)
{
    switch (kind) {
        case DDS_OWNERSHIP_SHARED: return "SHARED";
        case DDS_OWNERSHIP_EXCLUSIVE: return "EXCLUSIVE";
        default: return "UNKNOWN";
    }
}

const char* dds_qos_liveliness_to_string(dds_liveliness_kind_t kind)
{
    switch (kind) {
        case DDS_LIVELINESS_AUTOMATIC: return "AUTOMATIC";
        case DDS_LIVELINESS_MANUAL_BY_PARTICIPANT: return "MANUAL_BY_PARTICIPANT";
        case DDS_LIVELINESS_MANUAL_BY_TOPIC: return "MANUAL_BY_TOPIC";
        default: return "UNKNOWN";
    }
}

/* Parser context structure */
struct dds_config_parser {
    dds_config_parser_callbacks_t callbacks;
    void *userdata;
    char *error_msg;
    int error_line;
    int error_column;
};

/* Utility macros */
#define SAFE_STRCPY(dest, src, max) do { \
    strncpy((dest), (src), (max) - 1); \
    (dest)[(max) - 1] = '\0'; \
} while(0)

#define TRIM_NEWLINE(str) do { \
    size_t len = strlen(str); \
    while (len > 0 && ((str)[len-1] == '\n' || (str)[len-1] == '\r')) { \
        (str)[--len] = '\0'; \
    } \
} while(0)

/* Forward declarations */
static dds_config_error_t parse_yaml_file(dds_config_parser_t *parser, FILE *fp, dds_configuration_t **config);
static dds_config_error_t parse_json_file(dds_config_parser_t *parser, FILE *fp, dds_configuration_t **config);
static void skip_whitespace(const char **ptr);
static char* read_line(FILE *fp, char *buf, size_t size);
static int count_indent(const char *line);
static char* trim(char *str);
static char* parse_quoted_string(const char **ptr);

/* ============================================================================
 * Parser Creation and Destruction
 * ============================================================================ */

dds_config_parser_t* dds_config_parser_create(
    const dds_config_parser_callbacks_t *callbacks,
    void *userdata)
{
    dds_config_parser_t *parser = calloc(1, sizeof(dds_config_parser_t));
    if (!parser) {
        return NULL;
    }

    if (callbacks) {
        parser->callbacks = *callbacks;
    }
    parser->userdata = userdata;
    parser->error_line = -1;
    parser->error_column = -1;

    return parser;
}

void dds_config_parser_destroy(dds_config_parser_t *parser)
{
    if (!parser) return;

    free(parser->error_msg);
    free(parser);
}

/* ============================================================================
 * File Loading
 * ============================================================================ */

dds_config_error_t dds_config_parser_load_file(
    dds_config_parser_t *parser,
    const char *filename,
    dds_config_format_t format,
    dds_configuration_t **config)
{
    if (!parser || !filename || !config) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }

    /* Auto-detect format if needed */
    if (format == DDS_CONFIG_FORMAT_AUTO) {
        format = dds_config_detect_format(filename);
        if (format == DDS_CONFIG_FORMAT_AUTO) {
            return DDS_CONFIG_ERR_INVALID_FORMAT;
        }
    }

    /* Open file */
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        return DDS_CONFIG_ERR_FILE_IO;
    }

    dds_config_error_t err;
    switch (format) {
        case DDS_CONFIG_FORMAT_YAML:
            err = parse_yaml_file(parser, fp, config);
            break;
        case DDS_CONFIG_FORMAT_JSON:
            err = parse_json_file(parser, fp, config);
            break;
        default:
            err = DDS_CONFIG_ERR_INVALID_FORMAT;
            break;
    }

    fclose(fp);
    return err;
}

dds_config_error_t dds_config_parser_load_string(
    dds_config_parser_t *parser,
    const char *data,
    size_t length,
    dds_config_format_t format,
    dds_configuration_t **config)
{
    if (!parser || !data || !config) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }

    /* Write to temp file for simplicity */
    FILE *fp = tmpfile();
    if (!fp) {
        return DDS_CONFIG_ERR_FILE_IO;
    }

    size_t to_write = length > 0 ? length : strlen(data);
    if (fwrite(data, 1, to_write, fp) != to_write) {
        fclose(fp);
        return DDS_CONFIG_ERR_FILE_IO;
    }

    rewind(fp);

    dds_config_error_t err;
    switch (format) {
        case DDS_CONFIG_FORMAT_YAML:
            err = parse_yaml_file(parser, fp, config);
            break;
        case DDS_CONFIG_FORMAT_JSON:
            err = parse_json_file(parser, fp, config);
            break;
        default:
            err = DDS_CONFIG_ERR_INVALID_FORMAT;
            break;
    }

    fclose(fp);
    return err;
}

/* ============================================================================
 * Configuration Memory Management
 * ============================================================================ */

dds_configuration_t* dds_config_create(void)
{
    dds_configuration_t *config = calloc(1, sizeof(dds_configuration_t));
    if (!config) {
        return NULL;
    }

    /* Initialize default values */
    strncpy(config->version, "1.0.0", sizeof(config->version) - 1);
    config->logging.level = DDS_LOG_INFO;
    config->logging.output = DDS_LOG_OUTPUT_STDOUT;
    config->logging.enabled = true;

    return config;
}

void dds_config_destroy(dds_configuration_t *config)
{
    if (!config) return;

    /* Free domains */
    for (size_t i = 0; i < config->domain_count; i++) {
        dds_domain_config_t *d = &config->domains[i];
        for (size_t j = 0; j < d->discovery.initial_peer_count; j++) {
            free(d->discovery.initial_peers[j]);
        }
        free(d->discovery.initial_peers);
        for (size_t j = 0; j < d->transport.unicast_address_count; j++) {
            free(d->transport.unicast_addresses[j]);
        }
        free(d->transport.unicast_addresses);
    }
    free(config->domains);

    /* Free topics */
    for (size_t i = 0; i < config->topic_count; i++) {
        dds_topic_config_t *t = &config->topics[i];
        for (size_t j = 0; j < t->partition_count; j++) {
            free(t->partitions[j]);
        }
        free(t->partitions);
    }
    free(config->topics);

    /* Free QoS profiles */
    free(config->qos_profiles);

    /* Free participants */
    free(config->participants);

    /* Free types */
    free(config->types);

    /* Free logging categories */
    for (size_t i = 0; i < config->logging.category_count; i++) {
        free(config->logging.categories);
    }

    free(config);
}

dds_configuration_t* dds_config_clone(const dds_configuration_t *config)
{
    if (!config) return NULL;

    dds_configuration_t *clone = dds_config_create();
    if (!clone) return NULL;

    /* Copy version and system info */
    memcpy(clone->version, config->version, sizeof(clone->version));
    memcpy(&clone->system, &config->system, sizeof(clone->system));

    /* Copy domains */
    if (config->domain_count > 0) {
        clone->domains = calloc(config->domain_count, sizeof(dds_domain_config_t));
        if (clone->domains) {
            clone->domain_count = config->domain_count;
            memcpy(clone->domains, config->domains,
                   config->domain_count * sizeof(dds_domain_config_t));
        }
    }

    /* Copy topics */
    if (config->topic_count > 0) {
        clone->topics = calloc(config->topic_count, sizeof(dds_topic_config_t));
        if (clone->topics) {
            clone->topic_count = config->topic_count;
            memcpy(clone->topics, config->topics,
                   config->topic_count * sizeof(dds_topic_config_t));
        }
    }

    /* Copy QoS profiles */
    if (config->qos_profile_count > 0) {
        clone->qos_profiles = calloc(config->qos_profile_count, sizeof(dds_qos_profile_t));
        if (clone->qos_profiles) {
            clone->qos_profile_count = config->qos_profile_count;
            memcpy(clone->qos_profiles, config->qos_profiles,
                   config->qos_profile_count * sizeof(dds_qos_profile_t));
        }
    }

    /* Copy participants */
    if (config->participant_count > 0) {
        clone->participants = calloc(config->participant_count, sizeof(dds_participant_config_t));
        if (clone->participants) {
            clone->participant_count = config->participant_count;
            memcpy(clone->participants, config->participants,
                   config->participant_count * sizeof(dds_participant_config_t));
        }
    }

    /* Copy logging and monitoring */
    memcpy(&clone->logging, &config->logging, sizeof(clone->logging));
    memcpy(&clone->monitoring, &config->monitoring, sizeof(clone->monitoring));

    return clone;
}

/* ============================================================================
 * Format Detection
 * ============================================================================ */

dds_config_format_t dds_config_detect_format(const char *filename)
{
    if (!filename) return DDS_CONFIG_FORMAT_AUTO;

    const char *ext = strrchr(filename, '.');
    if (!ext) return DDS_CONFIG_FORMAT_AUTO;

    if (strcasecmp(ext, ".yaml") == 0 || strcasecmp(ext, ".yml") == 0) {
        return DDS_CONFIG_FORMAT_YAML;
    } else if (strcasecmp(ext, ".json") == 0) {
        return DDS_CONFIG_FORMAT_JSON;
    } else if (strcasecmp(ext, ".xml") == 0) {
        return DDS_CONFIG_FORMAT_XML;
    }

    return DDS_CONFIG_FORMAT_AUTO;
}

const char* dds_config_format_extension(dds_config_format_t format)
{
    switch (format) {
        case DDS_CONFIG_FORMAT_YAML: return ".yaml";
        case DDS_CONFIG_FORMAT_JSON: return ".json";
        case DDS_CONFIG_FORMAT_XML:  return ".xml";
        default: return "";
    }
}

/* ============================================================================
 * YAML Parser (Simplified)
 * ============================================================================ */

typedef struct {
    char *key;
    char *value;
    int indent;
    int line_num;
} yaml_token_t;

typedef struct {
    yaml_token_t *tokens;
    size_t count;
    size_t capacity;
    int current;
} yaml_parser_state_t;

static int yaml_tokenize(FILE *fp, yaml_parser_state_t *state)
{
    char line[1024];
    int line_num = 0;

    while (read_line(fp, line, sizeof(line))) {
        line_num++;

        /* Skip empty lines and comments */
        char *trimmed = trim(line);
        if (trimmed[0] == '\0' || trimmed[0] == '#') {
            continue;
        }

        /* Calculate indent */
        int indent = count_indent(line);

        /* Find key/value separator */
        char *colon = strchr(trimmed, ':');
        if (!colon) continue;

        *colon = '\0';
        char *key = trim(trimmed);
        char *value = trim(colon + 1);

        /* Remove quotes from key if present */
        size_t key_len = strlen(key);
        if (key_len >= 2 && ((key[0] == '"' && key[key_len-1] == '"') ||
                             (key[0] == '\'' && key[key_len-1] == '\''))) {
            key[key_len-1] = '\0';
            key++;
        }

        /* Remove quotes from value if present */
        size_t val_len = strlen(value);
        if (val_len >= 2 && ((value[0] == '"' && value[val_len-1] == '"') ||
                             (value[0] == '\'' && value[val_len-1] == '\''))) {
            value[val_len-1] = '\0';
            value++;
        }

        /* Grow token array if needed */
        if (state->count >= state->capacity) {
            size_t new_cap = state->capacity == 0 ? 256 : state->capacity * 2;
            yaml_token_t *new_tokens = realloc(state->tokens, new_cap * sizeof(yaml_token_t));
            if (!new_tokens) return -1;
            state->tokens = new_tokens;
            state->capacity = new_cap;
        }

        /* Store token */
        yaml_token_t *tok = &state->tokens[state->count++];
        tok->key = strdup(key);
        tok->value = strdup(value);
        tok->indent = indent;
        tok->line_num = line_num;
    }

    return 0;
}

static void yaml_free_tokens(yaml_parser_state_t *state)
{
    for (size_t i = 0; i < state->count; i++) {
        free(state->tokens[i].key);
        free(state->tokens[i].value);
    }
    free(state->tokens);
    state->tokens = NULL;
    state->count = 0;
    state->capacity = 0;
}

/* Helper: Parse boolean string */
static bool parse_bool(const char *str)
{
    if (!str) return false;
    return (strcasecmp(str, "true") == 0 ||
            strcasecmp(str, "yes") == 0 ||
            strcasecmp(str, "1") == 0);
}

/* Helper: Parse duration string (e.g., "100ms", "30s", "5m") */
static dds_duration_t parse_duration(const char *str)
{
    dds_duration_t dur = {0, 0, false};
    if (!str || str[0] == '\0') return dur;

    if (strcmp(str, "infinite") == 0 || strcmp(str, "INF") == 0) {
        dur.infinite = true;
        return dur;
    }

    char *endptr;
    double val = strtod(str, &endptr);

    if (endptr == str) return dur;

    if (strcmp(endptr, "ns") == 0) {
        dur.sec = 0;
        dur.nanosec = (uint32_t)(val);
    } else if (strcmp(endptr, "us") == 0) {
        dur.sec = 0;
        dur.nanosec = (uint32_t)(val * 1000);
    } else if (strcmp(endptr, "ms") == 0) {
        dur.sec = (int64_t)(val / 1000);
        dur.nanosec = (uint32_t)((val - dur.sec * 1000) * 1000000);
    } else if (strcmp(endptr, "s") == 0) {
        dur.sec = (int64_t)val;
        dur.nanosec = (uint32_t)((val - dur.sec) * 1000000000);
    } else if (strcmp(endptr, "m") == 0) {
        dur.sec = (int64_t)(val * 60);
    } else if (strcmp(endptr, "h") == 0) {
        dur.sec = (int64_t)(val * 3600);
    } else {
        /* Assume seconds */
        dur.sec = (int64_t)val;
    }

    return dur;
}

/* Helper: Parse reliability kind */
static dds_reliability_kind_t parse_reliability(const char *str)
{
    if (!str) return DDS_RELIABILITY_BEST_EFFORT;
    if (strcasecmp(str, "RELIABLE") == 0) return DDS_RELIABILITY_RELIABLE;
    return DDS_RELIABILITY_BEST_EFFORT;
}

/* Helper: Parse durability kind */
static dds_durability_kind_t parse_durability(const char *str)
{
    if (!str) return DDS_DURABILITY_VOLATILE;
    if (strcasecmp(str, "TRANSIENT_LOCAL") == 0) return DDS_DURABILITY_TRANSIENT_LOCAL;
    if (strcasecmp(str, "TRANSIENT") == 0) return DDS_DURABILITY_TRANSIENT;
    if (strcasecmp(str, "PERSISTENT") == 0) return DDS_DURABILITY_PERSISTENT;
    return DDS_DURABILITY_VOLATILE;
}

/* Helper: Parse history kind */
static dds_history_kind_t parse_history(const char *str)
{
    if (!str) return DDS_HISTORY_KEEP_LAST;
    if (strcasecmp(str, "KEEP_ALL") == 0) return DDS_HISTORY_KEEP_ALL;
    return DDS_HISTORY_KEEP_LAST;
}

/* Helper: Parse ownership kind */
static dds_ownership_kind_t parse_ownership(const char *str)
{
    if (!str) return DDS_OWNERSHIP_SHARED;
    if (strcasecmp(str, "EXCLUSIVE") == 0) return DDS_OWNERSHIP_EXCLUSIVE;
    return DDS_OWNERSHIP_SHARED;
}

/* Helper: Parse liveliness kind */
static dds_liveliness_kind_t parse_liveliness(const char *str)
{
    if (!str) return DDS_LIVELINESS_AUTOMATIC;
    if (strcasecmp(str, "MANUAL_BY_PARTICIPANT") == 0) return DDS_LIVELINESS_MANUAL_BY_PARTICIPANT;
    if (strcasecmp(str, "MANUAL_BY_TOPIC") == 0) return DDS_LIVELINESS_MANUAL_BY_TOPIC;
    return DDS_LIVELINESS_AUTOMATIC;
}

/* Helper: Parse destination order kind */
static dds_destination_order_kind_t parse_destination_order(const char *str)
{
    if (!str) return DDS_DESTINATION_ORDER_BY_RECEPTION_TIMESTAMP;
    if (strcasecmp(str, "BY_SOURCE_TIMESTAMP") == 0) return DDS_DESTINATION_ORDER_BY_SOURCE_TIMESTAMP;
    return DDS_DESTINATION_ORDER_BY_RECEPTION_TIMESTAMP;
}

/* Helper: Parse transport protocol */
static dds_transport_protocol_t parse_transport(const char *str)
{
    if (!str) return DDS_TRANSPORT_UDPv4;
    if (strcasecmp(str, "UDPv6") == 0) return DDS_TRANSPORT_UDPv6;
    if (strcasecmp(str, "SHMEM") == 0) return DDS_TRANSPORT_SHMEM;
    if (strcasecmp(str, "TCPv4") == 0) return DDS_TRANSPORT_TCPv4;
    if (strcasecmp(str, "TCPv6") == 0) return DDS_TRANSPORT_TCPv6;
    return DDS_TRANSPORT_UDPv4;
}

/* Helper: Parse log level */
static dds_log_level_t parse_log_level(const char *str)
{
    if (!str) return DDS_LOG_INFO;
    if (strcasecmp(str, "FATAL") == 0) return DDS_LOG_FATAL;
    if (strcasecmp(str, "ERROR") == 0) return DDS_LOG_ERROR;
    if (strcasecmp(str, "WARN") == 0) return DDS_LOG_WARN;
    if (strcasecmp(str, "DEBUG") == 0) return DDS_LOG_DEBUG;
    if (strcasecmp(str, "TRACE") == 0) return DDS_LOG_TRACE;
    return DDS_LOG_INFO;
}

/* Helper: Parse log output */
static dds_log_output_t parse_log_output(const char *str)
{
    if (!str) return DDS_LOG_OUTPUT_STDOUT;
    if (strcasecmp(str, "stderr") == 0) return DDS_LOG_OUTPUT_STDERR;
    if (strcasecmp(str, "file") == 0) return DDS_LOG_OUTPUT_FILE;
    if (strcasecmp(str, "syslog") == 0) return DDS_LOG_OUTPUT_SYSLOG;
    return DDS_LOG_OUTPUT_STDOUT;
}

static dds_config_error_t parse_yaml_file(dds_config_parser_t *parser, FILE *fp, dds_configuration_t **config)
{
    (void)parser;

    yaml_parser_state_t state = {0};
    if (yaml_tokenize(fp, &state) != 0) {
        return DDS_CONFIG_ERR_PARSE_ERROR;
    }

    *config = dds_config_create();
    if (!*config) {
        yaml_free_tokens(&state);
        return DDS_CONFIG_ERR_MEMORY;
    }

    dds_configuration_t *cfg = *config;

    /* Simplified parser - processes tokens sequentially */
    enum {
        PARSE_ROOT,
        PARSE_SYSTEM,
        PARSE_DOMAINS,
        PARSE_DOMAIN,
        PARSE_DOMAIN_DISCOVERY,
        PARSE_DOMAIN_TRANSPORT,
        PARSE_DOMAIN_SECURITY,
        PARSE_QOS_PROFILES,
        PARSE_QOS_PROFILE,
        PARSE_TOPICS,
        PARSE_TOPIC,
        PARSE_PARTICIPANTS,
        PARSE_PARTICIPANT,
        PARSE_LOGGING,
        PARSE_MONITORING
    } parse_state = PARSE_ROOT;

    int current_domain = -1;
    int current_qos = -1;
    int current_topic = -1;
    int current_participant = -1;

    for (size_t i = 0; i < state.count; i++) {
        yaml_token_t *tok = &state.tokens[i];
        const char *key = tok->key;
        const char *val = tok->value;

        /* State transitions based on key */
        if (strcmp(key, "version") == 0 && tok->indent == 0) {
            SAFE_STRCPY(cfg->version, val, sizeof(cfg->version));
            continue;
        }

        if (strcmp(key, "system") == 0 && tok->indent == 0) {
            parse_state = PARSE_SYSTEM;
            continue;
        }

        if (strcmp(key, "domains") == 0 && tok->indent == 0) {
            parse_state = PARSE_DOMAINS;
            continue;
        }

        if (strcmp(key, "qos_profiles") == 0 && tok->indent == 0) {
            parse_state = PARSE_QOS_PROFILES;
            continue;
        }

        if (strcmp(key, "topics") == 0 && tok->indent == 0) {
            parse_state = PARSE_TOPICS;
            continue;
        }

        if (strcmp(key, "participants") == 0 && tok->indent == 0) {
            parse_state = PARSE_PARTICIPANTS;
            continue;
        }

        if (strcmp(key, "logging") == 0 && tok->indent == 0) {
            parse_state = PARSE_LOGGING;
            continue;
        }

        if (strcmp(key, "monitoring") == 0 && tok->indent == 0) {
            parse_state = PARSE_MONITORING;
            continue;
        }

        /* List item detection (starts with - ) */
        bool is_list_item = (strncmp(key, "- ", 2) == 0);
        if (is_list_item) {
            const char *item_key = key + 2;

            if (parse_state == PARSE_DOMAINS) {
                /* New domain */
                cfg->domains = realloc(cfg->domains, (cfg->domain_count + 1) * sizeof(dds_domain_config_t));
                if (cfg->domains) {
                    memset(&cfg->domains[cfg->domain_count], 0, sizeof(dds_domain_config_t));
                    current_domain = (int)cfg->domain_count++;
                    parse_state = PARSE_DOMAIN;

                    /* Check if it's "id: N" format */
                    if (strcmp(item_key, "id") == 0 && val[0]) {
                        cfg->domains[current_domain].domain_id = (uint32_t)atoi(val);
                    }
                }
                continue;
            }

            if (parse_state == PARSE_QOS_PROFILES) {
                /* New QoS profile */
                cfg->qos_profiles = realloc(cfg->qos_profiles, (cfg->qos_profile_count + 1) * sizeof(dds_qos_profile_t));
                if (cfg->qos_profiles) {
                    memset(&cfg->qos_profiles[cfg->qos_profile_count], 0, sizeof(dds_qos_profile_t));
                    current_qos = (int)cfg->qos_profile_count++;
                    parse_state = PARSE_QOS_PROFILE;

                    if (strcmp(item_key, "name") == 0 && val[0]) {
                        SAFE_STRCPY(cfg->qos_profiles[current_qos].name, val, DDS_CONFIG_MAX_NAME_LEN);
                    }
                }
                continue;
            }

            if (parse_state == PARSE_TOPICS) {
                /* New topic */
                cfg->topics = realloc(cfg->topics, (cfg->topic_count + 1) * sizeof(dds_topic_config_t));
                if (cfg->topics) {
                    memset(&cfg->topics[cfg->topic_count], 0, sizeof(dds_topic_config_t));
                    current_topic = (int)cfg->topic_count++;
                    parse_state = PARSE_TOPIC;

                    if (strcmp(item_key, "name") == 0 && val[0]) {
                        SAFE_STRCPY(cfg->topics[current_topic].name, val, DDS_CONFIG_MAX_NAME_LEN);
                    }
                }
                continue;
            }

            if (parse_state == PARSE_PARTICIPANTS) {
                /* New participant */
                cfg->participants = realloc(cfg->participants, (cfg->participant_count + 1) * sizeof(dds_participant_config_t));
                if (cfg->participants) {
                    memset(&cfg->participants[cfg->participant_count], 0, sizeof(dds_participant_config_t));
                    current_participant = (int)cfg->participant_count++;
                    parse_state = PARSE_PARTICIPANT;

                    if (strcmp(item_key, "name") == 0 && val[0]) {
                        SAFE_STRCPY(cfg->participants[current_participant].name, val, DDS_CONFIG_MAX_NAME_LEN);
                    }
                }
                continue;
            }
        }

        /* State-specific parsing */
        switch (parse_state) {
            case PARSE_SYSTEM:
                if (strcmp(key, "name") == 0) {
                    SAFE_STRCPY(cfg->system.name, val, DDS_CONFIG_MAX_NAME_LEN);
                } else if (strcmp(key, "description") == 0) {
                    SAFE_STRCPY(cfg->system.description, val, DDS_CONFIG_MAX_DESCRIPTION_LEN);
                } else if (strcmp(key, "version") == 0) {
                    SAFE_STRCPY(cfg->system.version, val, sizeof(cfg->system.version));
                }
                break;

            case PARSE_DOMAIN:
                if (current_domain >= 0 && current_domain < (int)cfg->domain_count) {
                    dds_domain_config_t *d = &cfg->domains[current_domain];
                    if (strcmp(key, "id") == 0) {
                        d->domain_id = (uint32_t)atoi(val);
                    } else if (strcmp(key, "name") == 0) {
                        SAFE_STRCPY(d->name, val, DDS_CONFIG_MAX_NAME_LEN);
                    } else if (strcmp(key, "description") == 0) {
                        SAFE_STRCPY(d->description, val, DDS_CONFIG_MAX_DESCRIPTION_LEN);
                    } else if (strcmp(key, "discovery") == 0) {
                        parse_state = PARSE_DOMAIN_DISCOVERY;
                    } else if (strcmp(key, "transport") == 0) {
                        parse_state = PARSE_DOMAIN_TRANSPORT;
                    } else if (strcmp(key, "security") == 0) {
                        parse_state = PARSE_DOMAIN_SECURITY;
                    }
                }
                break;

            case PARSE_DOMAIN_DISCOVERY:
                if (current_domain >= 0 && current_domain < (int)cfg->domain_count) {
                    dds_discovery_config_t *disc = &cfg->domains[current_domain].discovery;
                    if (strcmp(key, "enabled") == 0) {
                        disc->enabled = parse_bool(val);
                    } else if (strcmp(key, "protocol") == 0) {
                        disc->protocol = parse_transport(val);
                    } else if (strcmp(key, "lease_duration") == 0) {
                        disc->lease_duration = parse_duration(val);
                    } else if (strcmp(key, "liveliness_check_interval") == 0) {
                        disc->liveliness_check_interval = parse_duration(val);
                    }
                }
                /* Return to domain state when indent decreases */
                if (i + 1 < state.count && state.tokens[i+1].indent <= 2) {
                    parse_state = PARSE_DOMAIN;
                }
                break;

            case PARSE_DOMAIN_TRANSPORT:
                if (current_domain >= 0 && current_domain < (int)cfg->domain_count) {
                    dds_transport_config_t *trans = &cfg->domains[current_domain].transport;
                    if (strcmp(key, "multicast") == 0 || strcmp(key, "multicast_enabled") == 0) {
                        trans->multicast_enabled = parse_bool(val);
                    } else if (strcmp(key, "multicast_address") == 0) {
                        SAFE_STRCPY(trans->multicast_address, val, 64);
                    }
                }
                if (i + 1 < state.count && state.tokens[i+1].indent <= 2) {
                    parse_state = PARSE_DOMAIN;
                }
                break;

            case PARSE_DOMAIN_SECURITY:
                if (current_domain >= 0 && current_domain < (int)cfg->domain_count) {
                    dds_security_config_t *sec = &cfg->domains[current_domain].security;
                    if (strcmp(key, "enabled") == 0) {
                        sec->enabled = parse_bool(val);
                    } else if (strcmp(key, "auth_kind") == 0) {
                        if (strcasecmp(val, "PKI_DH") == 0) sec->auth_kind = DDS_AUTH_PKI_DH;
                        else if (strcasecmp(val, "PSK") == 0) sec->auth_kind = DDS_AUTH_PSK;
                        else sec->auth_kind = DDS_AUTH_NONE;
                    } else if (strcmp(key, "identity_ca") == 0) {
                        SAFE_STRCPY(sec->identity_ca, val, 512);
                    } else if (strcmp(key, "identity_certificate") == 0) {
                        SAFE_STRCPY(sec->identity_certificate, val, 512);
                    } else if (strcmp(key, "private_key") == 0) {
                        SAFE_STRCPY(sec->private_key, val, 512);
                    } else if (strcmp(key, "permissions") == 0) {
                        SAFE_STRCPY(sec->permissions, val, 512);
                    } else if (strcmp(key, "governance") == 0) {
                        SAFE_STRCPY(sec->governance, val, 512);
                    }
                }
                if (i + 1 < state.count && state.tokens[i+1].indent <= 2) {
                    parse_state = PARSE_DOMAIN;
                }
                break;

            case PARSE_QOS_PROFILE:
                if (current_qos >= 0 && current_qos < (int)cfg->qos_profile_count) {
                    dds_qos_profile_t *q = &cfg->qos_profiles[current_qos];
                    if (strcmp(key, "name") == 0) {
                        SAFE_STRCPY(q->name, val, DDS_CONFIG_MAX_NAME_LEN);
                    } else if (strcmp(key, "description") == 0) {
                        SAFE_STRCPY(q->description, val, DDS_CONFIG_MAX_DESCRIPTION_LEN);
                    } else if (strcmp(key, "base_profile") == 0) {
                        SAFE_STRCPY(q->base_profile, val, DDS_CONFIG_MAX_NAME_LEN);
                    } else if (strcmp(key, "reliability") == 0) {
                        parse_state = PARSE_QOS_PROFILE; /* Will handle nested in next iteration */
                    } else if (strcmp(key, "kind") == 0 && tok->indent == 6) {
                        /* Nested under reliability/durability/etc */
                        /* Check context from previous tokens */
                    }
                }
                break;

            case PARSE_TOPIC:
                if (current_topic >= 0 && current_topic < (int)cfg->topic_count) {
                    dds_topic_config_t *t = &cfg->topics[current_topic];
                    if (strcmp(key, "name") == 0) {
                        SAFE_STRCPY(t->name, val, DDS_CONFIG_MAX_NAME_LEN);
                    } else if (strcmp(key, "type") == 0 || strcmp(key, "type_name") == 0) {
                        SAFE_STRCPY(t->type_name, val, DDS_CONFIG_MAX_NAME_LEN);
                    } else if (strcmp(key, "domain_id") == 0) {
                        t->domain_id = (uint32_t)atoi(val);
                    } else if (strcmp(key, "qos_profile") == 0) {
                        SAFE_STRCPY(t->qos_profile, val, DDS_CONFIG_MAX_NAME_LEN);
                    }
                }
                break;

            case PARSE_PARTICIPANT:
                if (current_participant >= 0 && current_participant < (int)cfg->participant_count) {
                    dds_participant_config_t *p = &cfg->participants[current_participant];
                    if (strcmp(key, "name") == 0) {
                        SAFE_STRCPY(p->name, val, DDS_CONFIG_MAX_NAME_LEN);
                    } else if (strcmp(key, "domain_id") == 0) {
                        p->domain_id = (uint32_t)atoi(val);
                    } else if (strcmp(key, "qos_profile") == 0) {
                        SAFE_STRCPY(p->qos_profile, val, DDS_CONFIG_MAX_NAME_LEN);
                    }
                }
                break;

            case PARSE_LOGGING:
                if (strcmp(key, "enabled") == 0) {
                    cfg->logging.enabled = parse_bool(val);
                } else if (strcmp(key, "level") == 0) {
                    cfg->logging.level = parse_log_level(val);
                } else if (strcmp(key, "output") == 0) {
                    cfg->logging.output = parse_log_output(val);
                } else if (strcmp(key, "log_file") == 0) {
                    SAFE_STRCPY(cfg->logging.log_file, val, 512);
                }
                break;

            case PARSE_MONITORING:
                if (strcmp(key, "enabled") == 0) {
                    cfg->monitoring.enabled = parse_bool(val);
                } else if (strcmp(key, "statistics_enabled") == 0) {
                    cfg->monitoring.statistics_enabled = parse_bool(val);
                } else if (strcmp(key, "statistics_interval") == 0) {
                    cfg->monitoring.statistics_interval = parse_duration(val);
                }
                break;

            default:
                break;
        }
    }

    yaml_free_tokens(&state);
    return DDS_CONFIG_OK;
}

/* ============================================================================
 * JSON Parser (Simplified)
 * ============================================================================ */

static dds_config_error_t parse_json_file(dds_config_parser_t *parser, FILE *fp, dds_configuration_t **config)
{
    (void)parser;
    (void)fp;

    /* For simplicity, JSON parsing is implemented by converting to YAML-like tokens */
    /* Full implementation would use a proper JSON parser */

    *config = dds_config_create();
    if (!*config) {
        return DDS_CONFIG_ERR_MEMORY;
    }

    /* Placeholder - would need a proper JSON parser library */
    /* For now, return unsupported format error */
    dds_config_destroy(*config);
    *config = NULL;
    return DDS_CONFIG_ERR_INVALID_FORMAT;
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

static char* read_line(FILE *fp, char *buf, size_t size)
{
    if (!fgets(buf, (int)size, fp)) {
        return NULL;
    }
    return buf;
}

static int count_indent(const char *line)
{
    int indent = 0;
    while (*line == ' ' || *line == '\t') {
        indent++;
        line++;
    }
    return indent;
}

static char* trim(char *str)
{
    if (!str) return str;

    /* Skip leading whitespace */
    while (isspace((unsigned char)*str)) str++;

    /* Trim trailing whitespace */
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';

    return str;
}

static void skip_whitespace(const char **ptr)
{
    while (isspace((unsigned char)**ptr)) (*ptr)++;
}

static char* parse_quoted_string(const char **ptr)
{
    skip_whitespace(ptr);

    if (**ptr != '"' && **ptr != '\'') {
        return NULL;
    }

    char quote = **ptr;
    (*ptr)++;

    const char *start = *ptr;
    while (**ptr && **ptr != quote) {
        (*ptr)++;
    }

    size_t len = *ptr - start;
    char *result = malloc(len + 1);
    if (result) {
        memcpy(result, start, len);
        result[len] = '\0';
    }

    if (**ptr == quote) {
        (*ptr)++;
    }

    return result;
}
