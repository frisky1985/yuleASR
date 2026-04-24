/*
 * DDS Configuration Parser - Header
 * 
 * Copyright (c) 2024 DDS Config Tool Authors
 * SPDX-License-Identifier: MIT
 */

#ifndef DDS_CONFIG_PARSER_H
#define DDS_CONFIG_PARSER_H

#include "dds_config_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Parser context */
typedef struct dds_config_parser dds_config_parser_t;

/* Supported file formats */
typedef enum {
    DDS_CONFIG_FORMAT_AUTO = 0,
    DDS_CONFIG_FORMAT_YAML,
    DDS_CONFIG_FORMAT_JSON,
    DDS_CONFIG_FORMAT_XML
} dds_config_format_t;

/* Parser callbacks */
typedef struct {
    void (*on_error)(const char *file, int line, int column, 
                     const char *message, void *userdata);
    void (*on_warning)(const char *file, int line, int column,
                       const char *message, void *userdata);
} dds_config_parser_callbacks_t;

/* Parser functions */
dds_config_parser_t* dds_config_parser_create(
    const dds_config_parser_callbacks_t *callbacks, 
    void *userdata);

void dds_config_parser_destroy(dds_config_parser_t *parser);

dds_config_error_t dds_config_parser_load_file(
    dds_config_parser_t *parser,
    const char *filename,
    dds_config_format_t format,
    dds_configuration_t **config);

dds_config_error_t dds_config_parser_load_string(
    dds_config_parser_t *parser,
    const char *data,
    size_t length,
    dds_config_format_t format,
    dds_configuration_t **config);

/* Configuration memory management */
dds_configuration_t* dds_config_create(void);
void dds_config_destroy(dds_configuration_t *config);
dds_configuration_t* dds_config_clone(const dds_configuration_t *config);

/* Utility functions */
dds_config_format_t dds_config_detect_format(const char *filename);
const char* dds_config_format_extension(dds_config_format_t format);

#ifdef __cplusplus
}
#endif

#endif /* DDS_CONFIG_PARSER_H */
