/*
 * DDS Configuration Validator - Header
 * 
 * Copyright (c) 2024 DDS Config Tool Authors
 * SPDX-License-Identifier: MIT
 */

#ifndef DDS_CONFIG_VALIDATOR_H
#define DDS_CONFIG_VALIDATOR_H

#include "dds_config_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Validation result entry */
typedef struct {
    char path[512];
    char message[1024];
    int severity;  /* 0=error, 1=warning, 2=info */
} dds_validation_entry_t;

/* Validation result */
typedef struct {
    dds_validation_entry_t *entries;
    size_t count;
    size_t capacity;
    size_t error_count;
    size_t warning_count;
    size_t info_count;
} dds_validation_result_t;

/* Validation options */
typedef struct {
    bool strict_mode;          /* Treat warnings as errors */
    bool check_consistency;    /* Check cross-reference consistency */
    bool check_performance;    /* Warn about performance issues */
    bool check_security;       /* Warn about security configuration */
} dds_validation_options_t;

/* Validator functions */
dds_validation_result_t* dds_validation_result_create(void);
void dds_validation_result_destroy(dds_validation_result_t *result);
void dds_validation_result_clear(dds_validation_result_t *result);
bool dds_validation_result_is_valid(const dds_validation_result_t *result);
void dds_validation_result_print(const dds_validation_result_t *result);

/* Validation functions */
dds_validation_result_t* dds_config_validate(
    const dds_configuration_t *config,
    const dds_validation_options_t *options);

dds_validation_result_t* dds_config_validate_domain(
    const dds_domain_config_t *domain,
    const dds_validation_options_t *options);

dds_validation_result_t* dds_config_validate_topic(
    const dds_topic_config_t *topic,
    const dds_configuration_t *context,
    const dds_validation_options_t *options);

dds_validation_result_t* dds_config_validate_qos_profile(
    const dds_qos_profile_t *profile,
    const dds_validation_options_t *options);

dds_validation_result_t* dds_config_validate_participant(
    const dds_participant_config_t *participant,
    const dds_configuration_t *context,
    const dds_validation_options_t *options);

/* Schema validation */
dds_validation_result_t* dds_config_validate_against_schema(
    const dds_configuration_t *config,
    const char *schema_file);

/* Specific validation checks */
bool dds_config_is_valid_domain_id(uint32_t domain_id);
bool dds_config_is_valid_topic_name(const char *name);
bool dds_config_is_valid_type_name(const char *name);
bool dds_config_is_valid_qos_combination(
    const dds_qos_profile_t *profile,
    char *error_msg, size_t error_msg_size);

/* Consistency checks */
dds_validation_result_t* dds_config_check_references(
    const dds_configuration_t *config);

dds_validation_result_t* dds_config_check_qos_consistency(
    const dds_configuration_t *config);

dds_validation_result_t* dds_config_check_domain_references(
    const dds_configuration_t *config);

/* Default validation options */
extern const dds_validation_options_t DDS_VALIDATION_DEFAULT_OPTIONS;
extern const dds_validation_options_t DDS_VALIDATION_STRICT_OPTIONS;

#ifdef __cplusplus
}
#endif

#endif /* DDS_CONFIG_VALIDATOR_H */
