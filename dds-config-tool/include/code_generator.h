/*
 * DDS Configuration Tool - Code Generator
 *
 * Copyright (c) 2024 DDS Config Tool Authors
 * SPDX-License-Identifier: MIT
 * 
 * Code generation for DDS configurations including IDL, C code, ARXML, and certificates
 */

#ifndef CODE_GENERATOR_H
#define CODE_GENERATOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "dds_config_types.h"
#include "security_config.h"
#include "automotive_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Generator Options and Configuration
 * ============================================================================ */
#define DDS_GEN_MAX_OUTPUT_PATH_LEN     1024
#define DDS_GEN_MAX_NAMESPACE_LEN       256
#define DDS_GEN_MAX_PREFIX_LEN          64
#define DDS_GEN_MAX_INDENT_SIZE         8
#define DDS_GEN_MAX_HEADER_LEN          4096

typedef enum {
    DDS_GEN_OUTPUT_IDL = 0,         /* DDS IDL */
    DDS_GEN_OUTPUT_C,               /* C source code */
    DDS_GEN_OUTPUT_CPP,             /* C++ source code */
    DDS_GEN_OUTPUT_PYTHON,          /* Python bindings */
    DDS_GEN_OUTPUT_JAVA,            /* Java bindings */
    DDS_GEN_OUTPUT_ARXML,           /* AUTOSAR ARXML */
    DDS_GEN_OUTPUT_XML,             /* Generic XML */
    DDS_GEN_OUTPUT_JSON,            /* JSON */
    DDS_GEN_OUTPUT_YAML,            /* YAML */
    DDS_GEN_OUTPUT_CSR,             /* Certificate Signing Request */
    DDS_GEN_OUTPUT_CERT,            /* X.509 Certificate */
    DDS_GEN_OUTPUT_GOVERNANCE,      /* DDS Security Governance */
    DDS_GEN_OUTPUT_PERMISSIONS      /* DDS Security Permissions */
} dds_gen_output_format_t;

typedef enum {
    DDS_GEN_STYLE_COMPACT = 0,      /* Minimal whitespace */
    DDS_GEN_STYLE_READABLE,         /* Human-readable with comments */
    DDS_GEN_STYLE_DOCUMENTED        /* Fully documented with examples */
} dds_gen_style_t;

typedef struct {
    /* Output configuration */
    char output_dir[DDS_GEN_MAX_OUTPUT_PATH_LEN];
    dds_gen_output_format_t format;
    dds_gen_style_t style;
    
    /* Naming conventions */
    char namespace_name[DDS_GEN_MAX_NAMESPACE_LEN];
    char prefix[DDS_GEN_MAX_PREFIX_LEN];
    bool use_prefix;
    
    /* Code formatting */
    uint32_t indent_size;
    bool use_tabs;
    int max_line_length;
    
    /* Feature toggles */
    bool generate_comments;
    bool generate_examples;
    bool generate_tests;
    bool generate_documentation;
    bool generate_build_files;      /* CMakeLists.txt, Makefile, etc. */
    bool generate_pkg_config;       /* pkg-config .pc files */
    
    /* C/C++ specific */
    bool generate_c_compat;         /* C compatibility for C++ */
    bool use_c99_types;             /* stdint.h types */
    bool generate_static_lib;       /* Static library */
    bool generate_shared_lib;       /* Shared library */
    
    /* Safety/Security */
    bool generate_safety_wrappers;  /* ASIL-compliant wrappers */
    bool generate_sec_module;       /* Security module */
    
    /* Automotive specific */
    bool generate_autosar_wrapper;  /* AUTOSAR wrapper code */
    bool generate_e2e_protection;   /* E2E protection code */
} dds_gen_options_t;

typedef struct {
    uint32_t files_generated;
    uint32_t lines_generated;
    uint32_t types_generated;
    uint32_t functions_generated;
    char last_error[512];
    bool success;
} dds_gen_result_t;

typedef struct {
    FILE *file;
    uint32_t indent_level;
    const dds_gen_options_t *options;
    char *buffer;
    size_t buffer_size;
    size_t buffer_used;
} dds_gen_context_t;

/* ============================================================================
 * Generator Lifecycle
 * ============================================================================ */
dds_gen_context_t* dds_gen_create_context(const dds_gen_options_t *options);
void dds_gen_destroy_context(dds_gen_context_t *ctx);
dds_gen_result_t* dds_gen_create_result(void);
void dds_gen_destroy_result(dds_gen_result_t *result);

/* ============================================================================
 * IDL Generation
 * ============================================================================ */
dds_config_error_t dds_gen_idl(const dds_configuration_t *config,
                                const dds_gen_options_t *options,
                                dds_gen_result_t *result);
dds_config_error_t dds_gen_idl_types(const dds_type_def_t *types,
                                      size_t type_count,
                                      const dds_gen_options_t *options,
                                      dds_gen_result_t *result);
dds_config_error_t dds_gen_idl_topics(const dds_topic_config_t *topics,
                                       size_t topic_count,
                                       const dds_gen_options_t *options,
                                       dds_gen_result_t *result);
dds_config_error_t dds_gen_idl_qos(const dds_qos_profile_t *profiles,
                                    size_t profile_count,
                                    const dds_gen_options_t *options,
                                    dds_gen_result_t *result);

/* ============================================================================
 * C Code Generation
 * ============================================================================ */
dds_config_error_t dds_gen_c_code(const dds_configuration_t *config,
                                   const dds_gen_options_t *options,
                                   dds_gen_result_t *result);
dds_config_error_t dds_gen_c_header(const dds_configuration_t *config,
                                     const dds_gen_options_t *options,
                                     const char *filename,
                                     dds_gen_result_t *result);
dds_config_error_t dds_gen_c_source(const dds_configuration_t *config,
                                     const dds_gen_options_t *options,
                                     const char *filename,
                                     dds_gen_result_t *result);
dds_config_error_t dds_gen_c_types(const dds_type_def_t *types,
                                    const dds_gen_options_t *options,
                                    dds_gen_context_t *ctx,
                                    dds_gen_result_t *result);
dds_config_error_t dds_gen_c_qos(const dds_qos_profile_t *profiles,
                                  const dds_gen_options_t *options,
                                  dds_gen_context_t *ctx,
                                  dds_gen_result_t *result);
dds_config_error_t dds_gen_c_participant(const dds_participant_config_t *participants,
                                          size_t participant_count,
                                          const dds_gen_options_t *options,
                                          dds_gen_context_t *ctx,
                                          dds_gen_result_t *result);
dds_config_error_t dds_gen_c_topic(const dds_topic_config_t *topics,
                                    size_t topic_count,
                                    const dds_gen_options_t *options,
                                    dds_gen_context_t *ctx,
                                    dds_gen_result_t *result);
dds_config_error_t dds_gen_c_cmakelists(const dds_configuration_t *config,
                                         const dds_gen_options_t *options,
                                         dds_gen_result_t *result);
dds_config_error_t dds_gen_c_makefile(const dds_configuration_t *config,
                                       const dds_gen_options_t *options,
                                       dds_gen_result_t *result);

/* ============================================================================
 * C++ Code Generation
 * ============================================================================ */
dds_config_error_t dds_gen_cpp_code(const dds_configuration_t *config,
                                     const dds_gen_options_t *options,
                                     dds_gen_result_t *result);
dds_config_error_t dds_gen_cpp_header(const dds_configuration_t *config,
                                       const dds_gen_options_t *options,
                                       const char *filename,
                                       dds_gen_result_t *result);
dds_config_error_t dds_gen_cpp_source(const dds_configuration_t *config,
                                       const dds_gen_options_t *options,
                                       const char *filename,
                                       dds_gen_result_t *result);
dds_config_error_t dds_gen_cpp_types(const dds_type_def_t *types,
                                      const dds_gen_options_t *options,
                                      dds_gen_context_t *ctx,
                                      dds_gen_result_t *result);

/* ============================================================================
 * AUTOSAR ARXML Generation
 * ============================================================================ */
dds_config_error_t dds_gen_arxml(const dds_configuration_t *config,
                                  const dds_automotive_config_t *auto_config,
                                  const dds_gen_options_t *options,
                                  dds_gen_result_t *result);
dds_config_error_t dds_gen_arxml_adaptive(const dds_autosar_adaptive_config_t *config,
                                           const dds_gen_options_t *options,
                                           dds_gen_context_t *ctx,
                                           dds_gen_result_t *result);
dds_config_error_t dds_gen_arxml_classic(const dds_autosar_classic_config_t *config,
                                          const dds_gen_options_t *options,
                                          dds_gen_context_t *ctx,
                                          dds_gen_result_t *result);
dds_config_error_t dds_gen_arxml_service(const dds_autosar_service_t *service,
                                          const dds_gen_options_t *options,
                                          dds_gen_context_t *ctx,
                                          dds_gen_result_t *result);
dds_config_error_t dds_gen_arxml_datatype(const dds_type_def_t *type,
                                           const dds_gen_options_t *options,
                                           dds_gen_context_t *ctx,
                                           dds_gen_result_t *result);

/* ============================================================================
 * Security Artifact Generation
 * ============================================================================ */
dds_config_error_t dds_gen_csr(const dds_sec_identity_info_t *identity,
                                const dds_gen_options_t *options,
                                dds_gen_result_t *result);
dds_config_error_t dds_gen_certificate_request(const dds_sec_identity_info_t *identity,
                                                const char *key_file,
                                                const dds_gen_options_t *options,
                                                dds_gen_result_t *result);
dds_config_error_t dds_gen_governance_xml(const dds_sec_governance_t *governance,
                                           const dds_gen_options_t *options,
                                           dds_gen_result_t *result);
dds_config_error_t dds_gen_permissions_xml(const dds_sec_permissions_doc_t *permissions,
                                            const dds_gen_options_t *options,
                                            dds_gen_result_t *result);
dds_config_error_t dds_gen_security_config_h(const dds_sec_full_config_t *sec_config,
                                              const dds_gen_options_t *options,
                                              dds_gen_result_t *result);

/* ============================================================================
 * Configuration Serialization
 * ============================================================================ */
dds_config_error_t dds_gen_json(const dds_configuration_t *config,
                                 const dds_gen_options_t *options,
                                 dds_gen_result_t *result);
dds_config_error_t dds_gen_yaml(const dds_configuration_t *config,
                                 const dds_gen_options_t *options,
                                 dds_gen_result_t *result);
dds_config_error_t dds_gen_xml(const dds_configuration_t *config,
                                const dds_gen_options_t *options,
                                dds_gen_result_t *result);

/* ============================================================================
 * Build System Generation
 * ============================================================================ */
dds_config_error_t dds_gen_cmake(const dds_configuration_t *config,
                                  const dds_gen_options_t *options,
                                  dds_gen_result_t *result);
dds_config_error_t dds_gen_meson(const dds_configuration_t *config,
                                  const dds_gen_options_t *options,
                                  dds_gen_result_t *result);
dds_config_error_t dds_gen_bazel(const dds_configuration_t *config,
                                  const dds_gen_options_t *options,
                                  dds_gen_result_t *result);
dds_config_error_t dds_gen_pkg_config(const dds_configuration_t *config,
                                       const dds_gen_options_t *options,
                                       dds_gen_result_t *result);

/* ============================================================================
 * Utility Functions
 * ============================================================================ */
void dds_gen_options_init(dds_gen_options_t *options);
void dds_gen_options_set_defaults(dds_gen_options_t *options);
const char* dds_gen_format_extension(dds_gen_output_format_t format);
const char* dds_gen_format_name(dds_gen_output_format_t format);

/* Context helper functions */
void dds_gen_indent(dds_gen_context_t *ctx);
void dds_gen_indent_increase(dds_gen_context_t *ctx);
void dds_gen_indent_decrease(dds_gen_context_t *ctx);
int dds_gen_printf(dds_gen_context_t *ctx, const char *fmt, ...);
int dds_gen_comment(dds_gen_context_t *ctx, const char *fmt, ...);
int dds_gen_blank_line(dds_gen_context_t *ctx);

/* Type mapping */
const char* dds_gen_map_idl_to_c(dds_base_type_t base_type);
const char* dds_gen_map_idl_to_cpp(dds_base_type_t base_type);
const char* dds_gen_map_idl_to_python(dds_base_type_t base_type);
const char* dds_gen_map_idl_to_java(dds_base_type_t base_type);
const char* dds_gen_map_idl_to_arxml(dds_base_type_t base_type);

/* Name sanitization */
void dds_gen_sanitize_name(char *dest, const char *src, size_t max_len);
void dds_gen_to_upper_snake_case(char *dest, const char *src, size_t max_len);
void dds_gen_to_camel_case(char *dest, const char *src, size_t max_len);
void dds_gen_to_pascal_case(char *dest, const char *src, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* CODE_GENERATOR_H */
