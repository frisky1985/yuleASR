/*
 * DDS Configuration Tool - Code Generator Implementation
 *
 * Copyright (c) 2024 DDS Config Tool Authors
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

#include "code_generator.h"

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

#define BUFFER_GROW_SIZE 4096

/* ============================================================================
 * Generator Lifecycle
 * ============================================================================ */

void dds_gen_options_init(dds_gen_options_t *options)
{
    if (!options) return;
    
    memset(options, 0, sizeof(dds_gen_options_t));
    dds_gen_options_set_defaults(options);
}

void dds_gen_options_set_defaults(dds_gen_options_t *options)
{
    if (!options) return;
    
    SAFE_STRCPY(options->output_dir, "./generated", DDS_GEN_MAX_OUTPUT_PATH_LEN);
    options->format = DDS_GEN_OUTPUT_C;
    options->style = DDS_GEN_STYLE_READABLE;
    
    SAFE_STRCPY(options->namespace_name, "dds_gen", DDS_GEN_MAX_NAMESPACE_LEN);
    SAFE_STRCPY(options->prefix, "dds_", DDS_GEN_MAX_PREFIX_LEN);
    options->use_prefix = true;
    
    options->indent_size = 4;
    options->use_tabs = false;
    options->max_line_length = 100;
    
    options->generate_comments = true;
    options->generate_examples = false;
    options->generate_tests = false;
    options->generate_documentation = false;
    options->generate_build_files = true;
    options->generate_pkg_config = true;
    
    options->generate_c_compat = true;
    options->use_c99_types = true;
    options->generate_static_lib = true;
    options->generate_shared_lib = false;
    
    options->generate_safety_wrappers = false;
    options->generate_sec_module = false;
    
    options->generate_autosar_wrapper = false;
    options->generate_e2e_protection = false;
}

dds_gen_context_t* dds_gen_create_context(const dds_gen_options_t *options)
{
    dds_gen_context_t *ctx = calloc(1, sizeof(dds_gen_context_t));
    if (!ctx) return NULL;
    
    ctx->options = options;
    ctx->indent_level = 0;
    
    /* Allocate initial buffer */
    ctx->buffer_size = BUFFER_GROW_SIZE;
    ctx->buffer = malloc(ctx->buffer_size);
    if (!ctx->buffer) {
        free(ctx);
        return NULL;
    }
    ctx->buffer[0] = '\0';
    ctx->buffer_used = 0;
    
    return ctx;
}

void dds_gen_destroy_context(dds_gen_context_t *ctx)
{
    if (!ctx) return;
    
    if (ctx->file && ctx->file != stdout) {
        fclose(ctx->file);
    }
    
    free(ctx->buffer);
    free(ctx);
}

dds_gen_result_t* dds_gen_create_result(void)
{
    dds_gen_result_t *result = calloc(1, sizeof(dds_gen_result_t));
    return result;
}

void dds_gen_destroy_result(dds_gen_result_t *result)
{
    free(result);
}

/* ============================================================================
 * Context Helper Functions
 * ============================================================================ */

void dds_gen_indent(dds_gen_context_t *ctx)
{
    if (!ctx || !ctx->file) return;
    
    char indent_char = ctx->options->use_tabs ? '\t' : ' ';
    int indent_amount = ctx->options->use_tabs ? 1 : ctx->options->indent_size;
    
    for (uint32_t i = 0; i < ctx->indent_level; i++) {
        for (int j = 0; j < indent_amount; j++) {
            fputc(indent_char, ctx->file);
        }
    }
}

void dds_gen_indent_increase(dds_gen_context_t *ctx)
{
    if (ctx) ctx->indent_level++;
}

void dds_gen_indent_decrease(dds_gen_context_t *ctx)
{
    if (ctx && ctx->indent_level > 0) ctx->indent_level--;
}

int dds_gen_printf(dds_gen_context_t *ctx, const char *fmt, ...)
{
    if (!ctx || !ctx->file) return -1;
    
    va_list args;
    va_start(args, fmt);
    
    dds_gen_indent(ctx);
    int result = vfprintf(ctx->file, fmt, args);
    
    va_end(args);
    return result;
}

int dds_gen_comment(dds_gen_context_t *ctx, const char *fmt, ...)
{
    if (!ctx || !ctx->file || !ctx->options->generate_comments) return 0;
    
    va_list args;
    va_start(args, fmt);
    
    dds_gen_indent(ctx);
    fprintf(ctx->file, "/* ");
    vfprintf(ctx->file, fmt, args);
    fprintf(ctx->file, " */\n");
    
    va_end(args);
    return 0;
}

int dds_gen_blank_line(dds_gen_context_t *ctx)
{
    if (!ctx || !ctx->file) return -1;
    return fprintf(ctx->file, "\n");
}

/* ============================================================================
 * Type Mapping
 * ============================================================================ */

const char* dds_gen_map_idl_to_c(dds_base_type_t base_type)
{
    switch (base_type) {
        case DDS_BASE_TYPE_SHORT:      return "int16_t";
        case DDS_BASE_TYPE_LONG:       return "int32_t";
        case DDS_BASE_TYPE_USHORT:     return "uint16_t";
        case DDS_BASE_TYPE_ULONG:      return "uint32_t";
        case DDS_BASE_TYPE_LONGLONG:   return "int64_t";
        case DDS_BASE_TYPE_ULONGLONG:  return "uint64_t";
        case DDS_BASE_TYPE_FLOAT:      return "float";
        case DDS_BASE_TYPE_DOUBLE:     return "double";
        case DDS_BASE_TYPE_LONG_DOUBLE: return "long double";
        case DDS_BASE_TYPE_BOOLEAN:    return "bool";
        case DDS_BASE_TYPE_CHAR:       return "char";
        case DDS_BASE_TYPE_WCHAR:      return "wchar_t";
        case DDS_BASE_TYPE_OCTET:      return "uint8_t";
        case DDS_BASE_TYPE_STRING:     return "char*";
        case DDS_BASE_TYPE_WSTRING:    return "wchar_t*";
        case DDS_BASE_TYPE_USER_DEFINED: return "struct";
        default: return "void*";
    }
}

const char* dds_gen_map_idl_to_cpp(dds_base_type_t base_type)
{
    switch (base_type) {
        case DDS_BASE_TYPE_SHORT:      return "int16_t";
        case DDS_BASE_TYPE_LONG:       return "int32_t";
        case DDS_BASE_TYPE_USHORT:     return "uint16_t";
        case DDS_BASE_TYPE_ULONG:      return "uint32_t";
        case DDS_BASE_TYPE_LONGLONG:   return "int64_t";
        case DDS_BASE_TYPE_ULONGLONG:  return "uint64_t";
        case DDS_BASE_TYPE_FLOAT:      return "float";
        case DDS_BASE_TYPE_DOUBLE:     return "double";
        case DDS_BASE_TYPE_LONG_DOUBLE: return "long double";
        case DDS_BASE_TYPE_BOOLEAN:    return "bool";
        case DDS_BASE_TYPE_CHAR:       return "char";
        case DDS_BASE_TYPE_WCHAR:      return "wchar_t";
        case DDS_BASE_TYPE_OCTET:      return "uint8_t";
        case DDS_BASE_TYPE_STRING:     return "std::string";
        case DDS_BASE_TYPE_WSTRING:    return "std::wstring";
        case DDS_BASE_TYPE_USER_DEFINED: return "";
        default: return "void*";
    }
}

const char* dds_gen_map_idl_to_python(dds_base_type_t base_type)
{
    switch (base_type) {
        case DDS_BASE_TYPE_SHORT:
        case DDS_BASE_TYPE_LONG:
        case DDS_BASE_TYPE_LONGLONG:   return "int";
        case DDS_BASE_TYPE_USHORT:
        case DDS_BASE_TYPE_ULONG:
        case DDS_BASE_TYPE_ULONGLONG:  return "int";
        case DDS_BASE_TYPE_FLOAT:
        case DDS_BASE_TYPE_DOUBLE:
        case DDS_BASE_TYPE_LONG_DOUBLE: return "float";
        case DDS_BASE_TYPE_BOOLEAN:    return "bool";
        case DDS_BASE_TYPE_CHAR:       return "str";
        case DDS_BASE_TYPE_WCHAR:      return "str";
        case DDS_BASE_TYPE_OCTET:      return "int";
        case DDS_BASE_TYPE_STRING:     return "str";
        case DDS_BASE_TYPE_WSTRING:    return "str";
        default: return "object";
    }
}

const char* dds_gen_map_idl_to_java(dds_base_type_t base_type)
{
    switch (base_type) {
        case DDS_BASE_TYPE_SHORT:      return "short";
        case DDS_BASE_TYPE_LONG:       return "int";
        case DDS_BASE_TYPE_USHORT:     return "int";
        case DDS_BASE_TYPE_ULONG:      return "long";
        case DDS_BASE_TYPE_LONGLONG:   return "long";
        case DDS_BASE_TYPE_ULONGLONG:  return "long";
        case DDS_BASE_TYPE_FLOAT:      return "float";
        case DDS_BASE_TYPE_DOUBLE:     return "double";
        case DDS_BASE_TYPE_LONG_DOUBLE: return "double";
        case DDS_BASE_TYPE_BOOLEAN:    return "boolean";
        case DDS_BASE_TYPE_CHAR:       return "char";
        case DDS_BASE_TYPE_WCHAR:      return "char";
        case DDS_BASE_TYPE_OCTET:      return "byte";
        case DDS_BASE_TYPE_STRING:     return "String";
        case DDS_BASE_TYPE_WSTRING:    return "String";
        default: return "Object";
    }
}

const char* dds_gen_map_idl_to_arxml(dds_base_type_t base_type)
{
    switch (base_type) {
        case DDS_BASE_TYPE_SHORT:      return "sint16";
        case DDS_BASE_TYPE_LONG:       return "sint32";
        case DDS_BASE_TYPE_USHORT:     return "uint16";
        case DDS_BASE_TYPE_ULONG:      return "uint32";
        case DDS_BASE_TYPE_LONGLONG:   return "sint64";
        case DDS_BASE_TYPE_ULONGLONG:  return "uint64";
        case DDS_BASE_TYPE_FLOAT:      return "float32";
        case DDS_BASE_TYPE_DOUBLE:     return "float64";
        case DDS_BASE_TYPE_BOOLEAN:    return "boolean";
        case DDS_BASE_TYPE_CHAR:       return "char";
        case DDS_BASE_TYPE_OCTET:      return "uint8";
        default: return "undefined";
    }
}

/* ============================================================================
 * Name Sanitization
 * ============================================================================ */

void dds_gen_sanitize_name(char *dest, const char *src, size_t max_len)
{
    if (!dest || !src || max_len == 0) return;
    
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j < max_len - 1; i++) {
        char c = src[i];
        
        /* Replace invalid characters with underscore */
        if (isalnum(c) || c == '_') {
            dest[j++] = c;
        } else if (c == ' ' || c == '-' || c == '.') {
            dest[j++] = '_';
        }
        /* Skip other special characters */
    }
    
    dest[j] = '\0';
    
    /* Ensure it doesn't start with a digit */
    if (j > 0 && isdigit(dest[0])) {
        memmove(dest + 1, dest, j);
        dest[0] = '_';
        dest[j + 1] = '\0';
    }
}

void dds_gen_to_upper_snake_case(char *dest, const char *src, size_t max_len)
{
    if (!dest || !src || max_len == 0) return;
    
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j < max_len - 1; i++) {
        char c = src[i];
        
        if (isalnum(c)) {
            /* Add underscore before uppercase letters (except first) */
            if (isupper(c) && j > 0 && i > 0 && isalnum(src[i-1]) && !isupper(src[i-1])) {
                if (j < max_len - 1) dest[j++] = '_';
            }
            dest[j++] = toupper(c);
        } else if (c == ' ' || c == '-' || c == '_') {
            if (j > 0 && dest[j-1] != '_') {
                dest[j++] = '_';
            }
        }
    }
    
    dest[j] = '\0';
}

void dds_gen_to_camel_case(char *dest, const char *src, size_t max_len)
{
    if (!dest || !src || max_len == 0) return;
    
    size_t j = 0;
    bool capitalize_next = false;
    
    for (size_t i = 0; src[i] != '\0' && j < max_len - 1; i++) {
        char c = src[i];
        
        if (isalnum(c)) {
            if (capitalize_next) {
                dest[j++] = toupper(c);
                capitalize_next = false;
            } else {
                dest[j++] = tolower(c);
            }
        } else if (c == ' ' || c == '_' || c == '-') {
            capitalize_next = true;
        }
    }
    
    dest[j] = '\0';
}

void dds_gen_to_pascal_case(char *dest, const char *src, size_t max_len)
{
    if (!dest || !src || max_len == 0) return;
    
    dds_gen_to_camel_case(dest, src, max_len);
    if (dest[0] != '\0') {
        dest[0] = toupper(dest[0]);
    }
}

/* ============================================================================
 * IDL Generation
 * ============================================================================ */

dds_config_error_t dds_gen_idl(const dds_configuration_t *config,
                                const dds_gen_options_t *options,
                                dds_gen_result_t *result)
{
    if (!config || !options || !result) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    /* Create output directory */
    mkdir(options->output_dir, 0755);
    
    /* Generate IDL file */
    char filename[DDS_GEN_MAX_OUTPUT_PATH_LEN];
    snprintf(filename, sizeof(filename), "%s/%s_types.idl",
             options->output_dir, options->namespace_name);
    
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        snprintf(result->last_error, sizeof(result->last_error),
                 "Failed to create file: %s", filename);
        result->success = false;
        return DDS_CONFIG_ERR_FILE_IO;
    }
    
    /* Generate header */
    fprintf(fp, "/* Generated DDS IDL file */\n");
    fprintf(fp, "/* Generator: DDS Config Tool */\n");
    fprintf(fp, "/* Timestamp: %ld */\n", (long)time(NULL));
    fprintf(fp, "/* Namespace: %s */\n\n", options->namespace_name);
    
    fprintf(fp, "module %s {\n\n", options->namespace_name);
    
    /* Generate types */
    for (size_t i = 0; i < config->type_count; i++) {
        const dds_type_def_t *type = &config->types[i];
        
        switch (type->kind) {
            case DDS_TYPE_STRUCT:
                fprintf(fp, "    struct %s {\n", type->name);
                for (size_t j = 0; j < type->field_count; j++) {
                    const dds_field_t *field = &type->fields[j];
                    fprintf(fp, "        %s %s", 
                            dds_gen_map_idl_to_c(field->base_type), 
                            field->name);
                    
                    /* Handle arrays */
                    if (field->array_dimension > 0) {
                        for (size_t k = 0; k < field->array_dimension; k++) {
                            fprintf(fp, "[%d]", field->array_bounds[k]);
                        }
                    }
                    
                    if (field->is_key) {
                        fprintf(fp, "; //@key");
                    }
                    fprintf(fp, ";\n");
                }
                fprintf(fp, "    };\n\n");
                break;
                
            case DDS_TYPE_ENUM:
                fprintf(fp, "    enum %s {\n", type->name);
                for (size_t j = 0; j < type->field_count; j++) {
                    fprintf(fp, "        %s%s\n", type->fields[j].name,
                            (j < type->field_count - 1) ? "," : "");
                }
                fprintf(fp, "    };\n\n");
                break;
                
            case DDS_TYPE_UNION:
                fprintf(fp, "    union %s switch (%s) {\n", 
                        type->name, 
                        type->field_count > 0 ? type->fields[0].type_name : "long");
                for (size_t j = 1; j < type->field_count; j++) {
                    fprintf(fp, "        case %d: %s %s;\n",
                            (int)j, 
                            type->fields[j].type_name,
                            type->fields[j].name);
                }
                fprintf(fp, "    };\n\n");
                break;
                
            default:
                break;
        }
    }
    
    /* Close module */
    fprintf(fp, "};\n");
    
    fclose(fp);
    
    result->files_generated++;
    result->success = true;
    
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_gen_idl_types(const dds_type_def_t *types,
                                      size_t type_count,
                                      const dds_gen_options_t *options,
                                      dds_gen_result_t *result)
{
    /* Simplified implementation */
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_gen_idl_topics(const dds_topic_config_t *topics,
                                       size_t topic_count,
                                       const dds_gen_options_t *options,
                                       dds_gen_result_t *result)
{
    /* Simplified implementation */
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_gen_idl_qos(const dds_qos_profile_t *profiles,
                                    size_t profile_count,
                                    const dds_gen_options_t *options,
                                    dds_gen_result_t *result)
{
    /* Simplified implementation */
    return DDS_CONFIG_OK;
}

/* ============================================================================
 * C Code Generation
 * ============================================================================ */

dds_config_error_t dds_gen_c_code(const dds_configuration_t *config,
                                   const dds_gen_options_t *options,
                                   dds_gen_result_t *result)
{
    if (!config || !options || !result) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    /* Create output directory */
    mkdir(options->output_dir, 0755);
    
    char header_file[DDS_GEN_MAX_OUTPUT_PATH_LEN];
    char source_file[DDS_GEN_MAX_OUTPUT_PATH_LEN];
    
    snprintf(header_file, sizeof(header_file), "%s/%s_config.h",
             options->output_dir, options->namespace_name);
    snprintf(source_file, sizeof(source_file), "%s/%s_config.c",
             options->output_dir, options->namespace_name);
    
    /* Generate header */
    dds_config_error_t err = dds_gen_c_header(config, options, header_file, result);
    if (err != DDS_CONFIG_OK) return err;
    
    /* Generate source */
    err = dds_gen_c_source(config, options, source_file, result);
    if (err != DDS_CONFIG_OK) return err;
    
    /* Generate build files if requested */
    if (options->generate_build_files) {
        dds_gen_c_cmakelists(config, options, result);
        dds_gen_c_makefile(config, options, result);
    }
    
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_gen_c_header(const dds_configuration_t *config,
                                     const dds_gen_options_t *options,
                                     const char *filename,
                                     dds_gen_result_t *result)
{
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        snprintf(result->last_error, sizeof(result->last_error),
                 "Failed to create file: %s", filename);
        return DDS_CONFIG_ERR_FILE_IO;
    }
    
    char guard_name[DDS_GEN_MAX_NAMESPACE_LEN];
    dds_gen_to_upper_snake_case(guard_name, options->namespace_name, sizeof(guard_name));
    
    /* File header */
    fprintf(fp, "/*\n");
    fprintf(fp, " * Generated DDS Configuration Header\n");
    fprintf(fp, " * Generator: DDS Config Tool\n");
    fprintf(fp, " * Timestamp: %ld\n", (long)time(NULL));
    fprintf(fp, " * Namespace: %s\n", options->namespace_name);
    fprintf(fp, " */\n\n");
    
    /* Include guard */
    fprintf(fp, "#ifndef %s_CONFIG_H\n", guard_name);
    fprintf(fp, "#define %s_CONFIG_H\n\n", guard_name);
    
    /* Standard includes */
    fprintf(fp, "#include <stdint.h>\n");
    fprintf(fp, "#include <stdbool.h>\n");
    fprintf(fp, "#include <stddef.h>\n\n");
    
    /* DDS includes */
    fprintf(fp, "#include <dds/dds.h>\n\n");
    
    /* Version info */
    fprintf(fp, "#define %s_CONFIG_VERSION \"%s\"\n", 
            options->namespace_name, config->version);
    fprintf(fp, "#define %s_CONFIG_MAJOR %d\n", 
            options->namespace_name, DDS_CONFIG_VERSION_MAJOR);
    fprintf(fp, "#define %s_CONFIG_MINOR %d\n", 
            options->namespace_name, DDS_CONFIG_VERSION_MINOR);
    fprintf(fp, "#define %s_CONFIG_PATCH %d\n\n", 
            options->namespace_name, DDS_CONFIG_VERSION_PATCH);
    
    /* Generate type definitions */
    fprintf(fp, "/* ============================================================================\n");
    fprintf(fp, " * Type Definitions\n");
    fprintf(fp, " * ============================================================================ */\n\n");
    
    for (size_t i = 0; i < config->type_count; i++) {
        const dds_type_def_t *type = &config->types[i];
        
        if (type->kind == DDS_TYPE_STRUCT) {
            fprintf(fp, "typedef struct {\n");
            for (size_t j = 0; j < type->field_count; j++) {
                const dds_field_t *field = &type->fields[j];
                fprintf(fp, "    %s %s",
                        dds_gen_map_idl_to_c(field->base_type),
                        field->name);
                
                if (field->array_dimension > 0) {
                    for (size_t k = 0; k < field->array_dimension; k++) {
                        fprintf(fp, "[%d]", field->array_bounds[k]);
                    }
                }
                fprintf(fp, ";\n");
            }
            fprintf(fp, "} %s_t;\n\n", type->name);
        }
    }
    
    /* Generate QoS profile extern declarations */
    fprintf(fp, "/* ============================================================================\n");
    fprintf(fp, " * QoS Profiles\n");
    fprintf(fp, " * ============================================================================ */\n\n");
    
    for (size_t i = 0; i < config->qos_profile_count; i++) {
        fprintf(fp, "extern dds_qos_t* %s_qos_profile_%s;\n",
                options->prefix, config->qos_profiles[i].name);
    }
    fprintf(fp, "\n");
    
    /* Generate function declarations */
    fprintf(fp, "/* ============================================================================\n");
    fprintf(fp, " * API Functions\n");
    fprintf(fp, " * ============================================================================ */\n\n");
    
    fprintf(fp, "dds_entity_t %screate_participant(dds_domainid_t domain_id);\n", 
            options->prefix);
    fprintf(fp, "dds_entity_t %screate_topic(dds_entity_t participant, const char* name, dds_entity_t type);\n",
            options->prefix);
    fprintf(fp, "dds_qos_t* %sget_qos_profile(const char* name);\n",
            options->prefix);
    fprintf(fp, "int %sinit(void);\n", options->prefix);
    fprintf(fp, "void %scleanup(void);\n", options->prefix);
    fprintf(fp, "\n");
    
    /* Close include guard */
    fprintf(fp, "#endif /* %s_CONFIG_H */\n", guard_name);
    
    fclose(fp);
    result->files_generated++;
    
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_gen_c_source(const dds_configuration_t *config,
                                     const dds_gen_options_t *options,
                                     const char *filename,
                                     dds_gen_result_t *result)
{
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        snprintf(result->last_error, sizeof(result->last_error),
                 "Failed to create file: %s", filename);
        return DDS_CONFIG_ERR_FILE_IO;
    }
    
    /* File header */
    fprintf(fp, "/*\n");
    fprintf(fp, " * Generated DDS Configuration Implementation\n");
    fprintf(fp, " * Generator: DDS Config Tool\n");
    fprintf(fp, " * Timestamp: %ld\n", (long)time(NULL));
    fprintf(fp, " */\n\n");
    
    /* Includes */
    fprintf(fp, "#include \"%s_config.h\"\n\n", options->namespace_name);
    fprintf(fp, "#include <string.h>\n\n");
    
    /* Static QoS profiles */
    fprintf(fp, "/* ============================================================================\n");
    fprintf(fp, " * QoS Profiles\n");
    fprintf(fp, " * ============================================================================ */\n\n");
    
    for (size_t i = 0; i < config->qos_profile_count; i++) {
        const dds_qos_profile_t *profile = &config->qos_profiles[i];
        
        fprintf(fp, "static dds_qos_t %s_qos_%s = {\n", 
                options->prefix, profile->name);
        
        if (profile->has_reliability) {
            fprintf(fp, "    .reliability = {\n");
            fprintf(fp, "        .kind = %s,\n",
                    profile->reliability.kind == DDS_RELIABILITY_RELIABLE 
                    ? "DDS_RELIABILITY_RELIABLE" : "DDS_RELIABILITY_BEST_EFFORT");
            fprintf(fp, "    },\n");
        }
        
        if (profile->has_durability) {
            fprintf(fp, "    .durability = {\n");
            fprintf(fp, "        .kind = DDS_DURABILITY_%s,\n",
                    dds_qos_durability_to_string(profile->durability.kind));
            fprintf(fp, "    },\n");
        }
        
        fprintf(fp, "};\n\n");
        
        fprintf(fp, "dds_qos_t* %s_qos_profile_%s = &%s_qos_%s;\n\n",
                options->prefix, profile->name, options->prefix, profile->name);
    }
    
    /* Implementation functions */
    fprintf(fp, "/* ============================================================================\n");
    fprintf(fp, " * API Implementation\n");
    fprintf(fp, " * ============================================================================ */\n\n");
    
    fprintf(fp, "static dds_entity_t g_participant = 0;\n");
    fprintf(fp, "static bool g_initialized = false;\n\n");
    
    fprintf(fp, "int %sinit(void)\n{\n", options->prefix);
    fprintf(fp, "    if (g_initialized) return 0;\n\n");
    fprintf(fp, "    /* Initialize DDS runtime */\n");
    fprintf(fp, "    dds_return_t rc = dds_init(0, NULL);\n");
    fprintf(fp, "    if (rc != DDS_RETCODE_OK) return -1;\n\n");
    fprintf(fp, "    g_initialized = true;\n");
    fprintf(fp, "    return 0;\n");
    fprintf(fp, "}\n\n");
    
    fprintf(fp, "void %scleanup(void)\n{\n", options->prefix);
    fprintf(fp, "    if (!g_initialized) return;\n\n");
    fprintf(fp, "    if (g_participant) {\n");
    fprintf(fp, "        dds_delete(g_participant);\n");
    fprintf(fp, "        g_participant = 0;\n");
    fprintf(fp, "    }\n\n");
    fprintf(fp, "    dds_fini();\n");
    fprintf(fp, "    g_initialized = false;\n");
    fprintf(fp, "}\n\n");
    
    fprintf(fp, "dds_entity_t %screate_participant(dds_domainid_t domain_id)\n{\n",
            options->prefix);
    fprintf(fp, "    if (!g_initialized) %sinit();\n\n", options->prefix);
    fprintf(fp, "    g_participant = dds_create_participant(domain_id, NULL, NULL);\n");
    fprintf(fp, "    return g_participant;\n");
    fprintf(fp, "}\n\n");
    
    fprintf(fp, "dds_qos_t* %sget_qos_profile(const char* name)\n{\n", options->prefix);
    fprintf(fp, "    (void)name;\n");
    fprintf(fp, "    /* TODO: Lookup QoS profile by name */\n");
    fprintf(fp, "    return NULL;\n");
    fprintf(fp, "}\n");
    
    fclose(fp);
    result->files_generated++;
    
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_gen_c_cmakelists(const dds_configuration_t *config,
                                         const dds_gen_options_t *options,
                                         dds_gen_result_t *result)
{
    char filename[DDS_GEN_MAX_OUTPUT_PATH_LEN];
    snprintf(filename, sizeof(filename), "%s/CMakeLists.txt", options->output_dir);
    
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        return DDS_CONFIG_ERR_FILE_IO;
    }
    
    fprintf(fp, "# Generated CMakeLists.txt\n");
    fprintf(fp, "cmake_minimum_required(VERSION 3.10)\n\n");
    fprintf(fp, "project(%s_config VERSION %s LANGUAGES C)\n\n",
            options->namespace_name, config->version);
    
    fprintf(fp, "# Find DDS package\n");
    fprintf(fp, "find_package(CycloneDDS REQUIRED)\n\n");
    
    fprintf(fp, "# Source files\n");
    fprintf(fp, "set(SOURCES\n");
    fprintf(fp, "    %s_config.c\n", options->namespace_name);
    fprintf(fp, ")\n\n");
    
    fprintf(fp, "# Header files\n");
    fprintf(fp, "set(HEADERS\n");
    fprintf(fp, "    %s_config.h\n", options->namespace_name);
    fprintf(fp, ")\n\n");
    
    if (options->generate_static_lib) {
        fprintf(fp, "# Static library\n");
        fprintf(fp, "add_library(%s_config_static STATIC ${SOURCES})\n",
                options->namespace_name);
        fprintf(fp, "target_link_libraries(%s_config_static CycloneDDS::ddsc)\n",
                options->namespace_name);
        fprintf(fp, "set_target_properties(%s_config_static PROPERTIES\n",
                options->namespace_name);
        fprintf(fp, "    OUTPUT_NAME %s_config\n", options->namespace_name);
        fprintf(fp, ")\n\n");
    }
    
    if (options->generate_shared_lib) {
        fprintf(fp, "# Shared library\n");
        fprintf(fp, "add_library(%s_config_shared SHARED ${SOURCES})\n",
                options->namespace_name);
        fprintf(fp, "target_link_libraries(%s_config_shared CycloneDDS::ddsc)\n",
                options->namespace_name);
        fprintf(fp, "set_target_properties(%s_config_shared PROPERTIES\n",
                options->namespace_name);
        fprintf(fp, "    OUTPUT_NAME %s_config\n", options->namespace_name);
        fprintf(fp, "    VERSION ${PROJECT_VERSION}\n");
        fprintf(fp, "    SOVERSION ${PROJECT_VERSION_MAJOR}\n");
        fprintf(fp, ")\n\n");
    }
    
    fprintf(fp, "# Install targets\n");
    fprintf(fp, "install(FILES ${HEADERS} DESTINATION include)\n");
    fprintf(fp, "install(TARGETS %s_config_static\n", options->namespace_name);
    fprintf(fp, "        DESTINATION lib)\n");
    
    fclose(fp);
    result->files_generated++;
    
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_gen_c_makefile(const dds_configuration_t *config,
                                       const dds_gen_options_t *options,
                                       dds_gen_result_t *result)
{
    char filename[DDS_GEN_MAX_OUTPUT_PATH_LEN];
    snprintf(filename, sizeof(filename), "%s/Makefile", options->output_dir);
    
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        return DDS_CONFIG_ERR_FILE_IO;
    }
    
    fprintf(fp, "# Generated Makefile\n\n");
    fprintf(fp, "CC = gcc\n");
    fprintf(fp, "CFLAGS = -Wall -Wextra -O2 -fPIC\n");
    fprintf(fp, "INCLUDES = -I. $(shell pkg-config --cflags cyclonedds)\n");
    fprintf(fp, "LIBS = $(shell pkg-config --libs cyclonedds)\n\n");
    
    fprintf(fp, "LIBNAME = lib%s_config\n", options->namespace_name);
    fprintf(fp, "SOURCES = %s_config.c\n", options->namespace_name);
    fprintf(fp, "OBJECTS = $(SOURCES:.c=.o)\n\n");
    
    fprintf(fp, "all: $(LIBNAME).a $(LIBNAME).so\n\n");
    
    fprintf(fp, "$(LIBNAME).a: $(OBJECTS)\n");
    fprintf(fp, "\tar rcs $@ $^\n\n");
    
    fprintf(fp, "$(LIBNAME).so: $(OBJECTS)\n");
    fprintf(fp, "\t$(CC) -shared -o $@ $^ $(LIBS)\n\n");
    
    fprintf(fp, "%%.o: %%.c\n");
    fprintf(fp, "\t$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<\n\n");
    
    fprintf(fp, "clean:\n");
    fprintf(fp, "\trm -f $(OBJECTS) $(LIBNAME).a $(LIBNAME).so\n\n");
    
    fprintf(fp, "install:\n");
    fprintf(fp, "\tcp $(LIBNAME).a $(LIBNAME).so /usr/local/lib/\n");
    fprintf(fp, "\tcp %s_config.h /usr/local/include/\n", options->namespace_name);
    
    fclose(fp);
    result->files_generated++;
    
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_gen_c_types(const dds_type_def_t *types,
                                    const dds_gen_options_t *options,
                                    dds_gen_context_t *ctx,
                                    dds_gen_result_t *result)
{
    /* Placeholder implementation */
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_gen_c_qos(const dds_qos_profile_t *profiles,
                                  const dds_gen_options_t *options,
                                  dds_gen_context_t *ctx,
                                  dds_gen_result_t *result)
{
    /* Placeholder implementation */
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_gen_c_participant(const dds_participant_config_t *participants,
                                          size_t participant_count,
                                          const dds_gen_options_t *options,
                                          dds_gen_context_t *ctx,
                                          dds_gen_result_t *result)
{
    /* Placeholder implementation */
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_gen_c_topic(const dds_topic_config_t *topics,
                                    size_t topic_count,
                                    const dds_gen_options_t *options,
                                    dds_gen_context_t *ctx,
                                    dds_gen_result_t *result)
{
    /* Placeholder implementation */
    return DDS_CONFIG_OK;
}

/* ============================================================================
 * C++ Code Generation (Placeholders)
 * ============================================================================ */

dds_config_error_t dds_gen_cpp_code(const dds_configuration_t *config,
                                     const dds_gen_options_t *options,
                                     dds_gen_result_t *result)
{
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_gen_cpp_header(const dds_configuration_t *config,
                                       const dds_gen_options_t *options,
                                       const char *filename,
                                       dds_gen_result_t *result)
{
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_gen_cpp_source(const dds_configuration_t *config,
                                       const dds_gen_options_t *options,
                                       const char *filename,
                                       dds_gen_result_t *result)
{
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_gen_cpp_types(const dds_type_def_t *types,
                                      const dds_gen_options_t *options,
                                      dds_gen_context_t *ctx,
                                      dds_gen_result_t *result)
{
    return DDS_CONFIG_OK;
}

/* ============================================================================
 * AUTOSAR ARXML Generation
 * ============================================================================ */

dds_config_error_t dds_gen_arxml(const dds_configuration_t *config,
                                  const dds_automotive_config_t *auto_config,
                                  const dds_gen_options_t *options,
                                  dds_gen_result_t *result)
{
    if (!config || !auto_config || !options || !result) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    /* Create output directory */
    mkdir(options->output_dir, 0755);
    
    char filename[DDS_GEN_MAX_OUTPUT_PATH_LEN];
    snprintf(filename, sizeof(filename), "%s/%s.arxml",
             options->output_dir, options->namespace_name);
    
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        snprintf(result->last_error, sizeof(result->last_error),
                 "Failed to create file: %s", filename);
        return DDS_CONFIG_ERR_FILE_IO;
    }
    
    /* ARXML header */
    fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(fp, "<AUTOSAR xmlns=\"http://autosar.org/schema/r4.0\"\n");
    fprintf(fp, "         xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n");
    fprintf(fp, "         xsi:schemaLocation=\"http://autosar.org/schema/r4.0 ");
    fprintf(fp, "AUTOSAR_00048.xsd\">\n");
    
    dds_gen_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.file = fp;
    ctx.options = options;
    ctx.indent_level = 1;
    
    /* AR-packages */
    dds_gen_printf(&ctx, "<AR-PACKAGES>\n");
    dds_gen_indent_increase(&ctx);
    
    dds_gen_printf(&ctx, "<AR-PACKAGE>\n");
    dds_gen_indent_increase(&ctx);
    dds_gen_printf(&ctx, "<SHORT-NAME>%s</SHORT-NAME>\n", options->namespace_name);
    dds_gen_printf(&ctx, "<ELEMENTS>\n");
    dds_gen_indent_increase(&ctx);
    
    /* Generate data types */
    for (size_t i = 0; i < config->type_count; i++) {
        dds_gen_arxml_datatype(&config->types[i], options, &ctx, result);
    }
    
    /* Generate AUTOSAR Adaptive services if enabled */
    if (auto_config->autosar_adaptive.enabled) {
        dds_gen_arxml_adaptive(&auto_config->autosar_adaptive, options, &ctx, result);
    }
    
    dds_gen_indent_decrease(&ctx);
    dds_gen_printf(&ctx, "</ELEMENTS>\n");
    dds_gen_indent_decrease(&ctx);
    dds_gen_printf(&ctx, "</AR-PACKAGE>\n");
    dds_gen_indent_decrease(&ctx);
    dds_gen_printf(&ctx, "</AR-PACKAGES>\n");
    
    fprintf(fp, "</AUTOSAR>\n");
    
    fclose(fp);
    result->files_generated++;
    result->success = true;
    
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_gen_arxml_adaptive(const dds_autosar_adaptive_config_t *config,
                                           const dds_gen_options_t *options,
                                           dds_gen_context_t *ctx,
                                           dds_gen_result_t *result)
{
    if (!config || !ctx) return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    
    dds_gen_printf(ctx, "<ADAPTIVE-APPLICATION-SW-COMPONENT-TYPE>\n");
    dds_gen_indent_increase(ctx);
    
    dds_gen_printf(ctx, "<SHORT-NAME>DDSApplication</SHORT-NAME>\n");
    dds_gen_printf(ctx, "<PROVIDED-PORTS>\n");
    dds_gen_indent_increase(ctx);
    
    for (size_t i = 0; i < config->service_count; i++) {
        dds_gen_arxml_service(&config->services[i], options, ctx, result);
    }
    
    dds_gen_indent_decrease(ctx);
    dds_gen_printf(ctx, "</PROVIDED-PORTS>\n");
    dds_gen_indent_decrease(ctx);
    dds_gen_printf(ctx, "</ADAPTIVE-APPLICATION-SW-COMPONENT-TYPE>\n");
    
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_gen_arxml_classic(const dds_autosar_classic_config_t *config,
                                          const dds_gen_options_t *options,
                                          dds_gen_context_t *ctx,
                                          dds_gen_result_t *result)
{
    /* Placeholder */
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_gen_arxml_service(const dds_autosar_service_t *service,
                                          const dds_gen_options_t *options,
                                          dds_gen_context_t *ctx,
                                          dds_gen_result_t *result)
{
    if (!service || !ctx) return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    
    dds_gen_printf(ctx, "<SERVICE-INTERFACE>\n");
    dds_gen_indent_increase(ctx);
    
    dds_gen_printf(ctx, "<SHORT-NAME>%s</SHORT-NAME>\n", service->name);
    dds_gen_printf(ctx, "<SERVICE-INTERFACE-ID>%u</SERVICE-INTERFACE-ID>\n", 
                   service->service_id);
    dds_gen_printf(ctx, "<EVENTS>\n");
    dds_gen_indent_increase(ctx);
    
    if (service->type == DDS_AUTOSAR_EVENT) {
        dds_gen_printf(ctx, "<EVENT>\n");
        dds_gen_indent_increase(ctx);
        dds_gen_printf(ctx, "<SHORT-NAME>Event_%s</SHORT-NAME>\n", service->name);
        dds_gen_printf(ctx, "<EVENT-ID>%u</EVENT-ID>\n", service->eventgroup_id);
        dds_gen_indent_decrease(ctx);
        dds_gen_printf(ctx, "</EVENT>\n");
    }
    
    dds_gen_indent_decrease(ctx);
    dds_gen_printf(ctx, "</EVENTS>\n");
    dds_gen_indent_decrease(ctx);
    dds_gen_printf(ctx, "</SERVICE-INTERFACE>\n");
    
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_gen_arxml_datatype(const dds_type_def_t *type,
                                           const dds_gen_options_t *options,
                                           dds_gen_context_t *ctx,
                                           dds_gen_result_t *result)
{
    if (!type || !ctx) return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    
    dds_gen_printf(ctx, "<IMPLEMENTATION-DATA-TYPE>\n");
    dds_gen_indent_increase(ctx);
    
    dds_gen_printf(ctx, "<SHORT-NAME>%s</SHORT-NAME>\n", type->name);
    dds_gen_printf(ctx, "<CATEGORY>STRUCTURE</CATEGORY>\n");
    dds_gen_printf(ctx, "<SUB-ELEMENTS>\n");
    dds_gen_indent_increase(ctx);
    
    for (size_t i = 0; i < type->field_count; i++) {
        const dds_field_t *field = &type->fields[i];
        
        dds_gen_printf(ctx, "<IMPLEMENTATION-DATA-TYPE-ELEMENT>\n");
        dds_gen_indent_increase(ctx);
        dds_gen_printf(ctx, "<SHORT-NAME>%s</SHORT-NAME>\n", field->name);
        dds_gen_printf(ctx, "<SW-DATA-DEF-PROPS>\n");
        dds_gen_indent_increase(ctx);
        dds_gen_printf(ctx, "<IMPLEMENTATION-DATA-TYPE-REF DEST=\"IMPLEMENTATION-DATA-TYPE\">");
        fprintf(ctx->file, "%s</IMPLEMENTATION-DATA-TYPE-REF>\n",
                dds_gen_map_idl_to_arxml(field->base_type));
        dds_gen_indent_decrease(ctx);
        dds_gen_printf(ctx, "</SW-DATA-DEF-PROPS>\n");
        dds_gen_indent_decrease(ctx);
        dds_gen_printf(ctx, "</IMPLEMENTATION-DATA-TYPE-ELEMENT>\n");
    }
    
    dds_gen_indent_decrease(ctx);
    dds_gen_printf(ctx, "</SUB-ELEMENTS>\n");
    dds_gen_indent_decrease(ctx);
    dds_gen_printf(ctx, "</IMPLEMENTATION-DATA-TYPE>\n");
    
    return DDS_CONFIG_OK;
}

/* ============================================================================
 * Security Artifact Generation
 * ============================================================================ */

dds_config_error_t dds_gen_csr(const dds_sec_identity_info_t *identity,
                                const dds_gen_options_t *options,
                                dds_gen_result_t *result)
{
    /* Placeholder - would use OpenSSL in real implementation */
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_gen_certificate_request(const dds_sec_identity_info_t *identity,
                                                const char *key_file,
                                                const dds_gen_options_t *options,
                                                dds_gen_result_t *result)
{
    /* Placeholder */
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_gen_governance_xml(const dds_sec_governance_t *governance,
                                           const dds_gen_options_t *options,
                                           dds_gen_result_t *result)
{
    if (!governance || !options || !result) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    mkdir(options->output_dir, 0755);
    
    char filename[DDS_GEN_MAX_OUTPUT_PATH_LEN];
    snprintf(filename, sizeof(filename), "%s/governance.xml", options->output_dir);
    
    return dds_sec_save_governance(governance, filename);
}

dds_config_error_t dds_gen_permissions_xml(const dds_sec_permissions_doc_t *permissions,
                                            const dds_gen_options_t *options,
                                            dds_gen_result_t *result)
{
    if (!permissions || !options || !result) {
        return DDS_CONFIG_ERR_INVALID_ARGUMENT;
    }
    
    mkdir(options->output_dir, 0755);
    
    char filename[DDS_GEN_MAX_OUTPUT_PATH_LEN];
    snprintf(filename, sizeof(filename), "%s/permissions.xml", options->output_dir);
    
    return dds_sec_save_permissions(permissions, filename);
}

dds_config_error_t dds_gen_security_config_h(const dds_sec_full_config_t *sec_config,
                                              const dds_gen_options_t *options,
                                              dds_gen_result_t *result)
{
    /* Placeholder */
    return DDS_CONFIG_OK;
}

/* ============================================================================
 * Configuration Serialization
 * ============================================================================ */

dds_config_error_t dds_gen_json(const dds_configuration_t *config,
                                 const dds_gen_options_t *options,
                                 dds_gen_result_t *result)
{
    /* Placeholder - would use JSON library in real implementation */
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_gen_yaml(const dds_configuration_t *config,
                                 const dds_gen_options_t *options,
                                 dds_gen_result_t *result)
{
    /* Placeholder - would use YAML library in real implementation */
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_gen_xml(const dds_configuration_t *config,
                                const dds_gen_options_t *options,
                                dds_gen_result_t *result)
{
    /* Placeholder */
    return DDS_CONFIG_OK;
}

/* ============================================================================
 * Build System Generation
 * ============================================================================ */

dds_config_error_t dds_gen_cmake(const dds_configuration_t *config,
                                  const dds_gen_options_t *options,
                                  dds_gen_result_t *result)
{
    return dds_gen_c_cmakelists(config, options, result);
}

dds_config_error_t dds_gen_meson(const dds_configuration_t *config,
                                  const dds_gen_options_t *options,
                                  dds_gen_result_t *result)
{
    /* Placeholder */
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_gen_bazel(const dds_configuration_t *config,
                                  const dds_gen_options_t *options,
                                  dds_gen_result_t *result)
{
    /* Placeholder */
    return DDS_CONFIG_OK;
}

dds_config_error_t dds_gen_pkg_config(const dds_configuration_t *config,
                                       const dds_gen_options_t *options,
                                       dds_gen_result_t *result)
{
    char filename[DDS_GEN_MAX_OUTPUT_PATH_LEN];
    snprintf(filename, sizeof(filename), "%s/%s_config.pc", 
             options->output_dir, options->namespace_name);
    
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        return DDS_CONFIG_ERR_FILE_IO;
    }
    
    fprintf(fp, "# Generated pkg-config file\n");
    fprintf(fp, "prefix=/usr/local\n");
    fprintf(fp, "exec_prefix=${prefix}\n");
    fprintf(fp, "libdir=${prefix}/lib\n");
    fprintf(fp, "includedir=${prefix}/include\n\n");
    
    fprintf(fp, "Name: %s_config\n", options->namespace_name);
    fprintf(fp, "Description: Generated DDS Configuration Library\n");
    fprintf(fp, "Version: %s\n", config->version);
    fprintf(fp, "Libs: -L${libdir} -l%s_config\n", options->namespace_name);
    fprintf(fp, "Cflags: -I${includedir}\n");
    
    fclose(fp);
    result->files_generated++;
    
    return DDS_CONFIG_OK;
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

const char* dds_gen_format_extension(dds_gen_output_format_t format)
{
    switch (format) {
        case DDS_GEN_OUTPUT_IDL:         return ".idl";
        case DDS_GEN_OUTPUT_C:           return ".c";
        case DDS_GEN_OUTPUT_CPP:         return ".cpp";
        case DDS_GEN_OUTPUT_PYTHON:      return ".py";
        case DDS_GEN_OUTPUT_JAVA:        return ".java";
        case DDS_GEN_OUTPUT_ARXML:       return ".arxml";
        case DDS_GEN_OUTPUT_XML:         return ".xml";
        case DDS_GEN_OUTPUT_JSON:        return ".json";
        case DDS_GEN_OUTPUT_YAML:        return ".yaml";
        case DDS_GEN_OUTPUT_CSR:         return ".csr";
        case DDS_GEN_OUTPUT_CERT:        return ".pem";
        case DDS_GEN_OUTPUT_GOVERNANCE:  return ".xml";
        case DDS_GEN_OUTPUT_PERMISSIONS: return ".xml";
        default: return ".txt";
    }
}

const char* dds_gen_format_name(dds_gen_output_format_t format)
{
    switch (format) {
        case DDS_GEN_OUTPUT_IDL:         return "IDL";
        case DDS_GEN_OUTPUT_C:           return "C";
        case DDS_GEN_OUTPUT_CPP:         return "C++";
        case DDS_GEN_OUTPUT_PYTHON:      return "Python";
        case DDS_GEN_OUTPUT_JAVA:        return "Java";
        case DDS_GEN_OUTPUT_ARXML:       return "ARXML";
        case DDS_GEN_OUTPUT_XML:         return "XML";
        case DDS_GEN_OUTPUT_JSON:        return "JSON";
        case DDS_GEN_OUTPUT_YAML:        return "YAML";
        case DDS_GEN_OUTPUT_CSR:         return "CSR";
        case DDS_GEN_OUTPUT_CERT:        return "Certificate";
        case DDS_GEN_OUTPUT_GOVERNANCE:  return "Governance";
        case DDS_GEN_OUTPUT_PERMISSIONS: return "Permissions";
        default: return "Unknown";
    }
}
