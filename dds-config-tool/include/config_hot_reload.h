/*
 * DDS Configuration Tool - Hot Reload Support
 *
 * Copyright (c) 2024 DDS Config Tool Authors
 * SPDX-License-Identifier: MIT
 * 
 * Real-time configuration monitoring and hot reload capabilities
 */

#ifndef CONFIG_HOT_RELOAD_H
#define CONFIG_HOT_RELOAD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "dds_config_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Constants
 * ============================================================================ */
#define DDS_RELOAD_MAX_WATCH_PATHS      32
#define DDS_RELOAD_MAX_CALLBACKS        16
#define DDS_RELOAD_MAX_CONFIG_VERSIONS  10
#define DDS_RELOAD_MAX_DIFF_ENTRIES     256
#define DDS_RELOAD_MAX_PATH_LEN         1024
#define DDS_RELOAD_HASH_SIZE            64

/* ============================================================================
 * Error Codes
 * ============================================================================ */
typedef enum {
    DDS_RELOAD_OK = 0,
    DDS_RELOAD_ERR_INVALID_ARGUMENT = -1,
    DDS_RELOAD_ERR_NOT_INITIALIZED = -2,
    DDS_RELOAD_ERR_ALREADY_RUNNING = -3,
    DDS_RELOAD_ERR_WATCH_LIMIT = -4,
    DDS_RELOAD_ERR_CALLBACK_LIMIT = -5,
    DDS_RELOAD_ERR_FILE_NOT_FOUND = -6,
    DDS_RELOAD_ERR_PARSE_FAILED = -7,
    DDS_RELOAD_ERR_VALIDATION_FAILED = -8,
    DDS_RELOAD_ERR_APPLY_FAILED = -9,
    DDS_RELOAD_ERR_ROLLBACK_FAILED = -10,
    DDS_RELOAD_ERR_MEMORY = -11,
    DDS_RELOAD_ERR_SYSTEM = -12
} dds_reload_error_t;

/* ============================================================================
 * Configuration Version Management
 * ============================================================================ */
typedef uint32_t dds_config_version_t;

typedef struct {
    dds_config_version_t version;
    uint64_t timestamp_ms;
    char config_hash[DDS_RELOAD_HASH_SIZE];
    char description[256];
    dds_configuration_t *config;
    bool active;
    bool rollback_point;
} dds_config_version_entry_t;

typedef struct {
    dds_config_version_entry_t *versions;
    size_t count;
    size_t capacity;
    dds_config_version_t current_version;
    dds_config_version_t last_stable_version;
} dds_config_version_history_t;

/* ============================================================================
 * Change Types and Diff
 * ============================================================================ */
typedef enum {
    DDS_CHANGE_ADDED = 0,
    DDS_CHANGE_MODIFIED,
    DDS_CHANGE_REMOVED,
    DDS_CHANGE_REORDERED
} dds_change_type_t;

typedef enum {
    DDS_CONFIG_SECTION_SYSTEM = 0,
    DDS_CONFIG_SECTION_DOMAIN,
    DDS_CONFIG_SECTION_TOPIC,
    DDS_CONFIG_SECTION_QOS,
    DDS_CONFIG_SECTION_PARTICIPANT,
    DDS_CONFIG_SECTION_TYPE,
    DDS_CONFIG_SECTION_SECURITY,
    DDS_CONFIG_SECTION_MONITORING,
    DDS_CONFIG_SECTION_LOGGING
} dds_config_section_t;

typedef struct {
    dds_change_type_t type;
    dds_config_section_t section;
    char path[DDS_CONFIG_MAX_NAME_LEN * 2];
    char old_value[512];
    char new_value[512];
    bool requires_restart;
    bool can_hot_apply;
} dds_config_diff_entry_t;

typedef struct {
    dds_config_diff_entry_t *entries;
    size_t count;
    size_t capacity;
    bool has_critical_changes;
    bool has_restart_required;
    dds_config_version_t from_version;
    dds_config_version_t to_version;
} dds_config_diff_t;

/* ============================================================================
 * Watch and Monitor Types
 * ============================================================================ */
typedef enum {
    DDS_WATCH_FILE = 0,
    DDS_WATCH_DIRECTORY
} dds_watch_type_t;

typedef enum {
    DDS_EVENT_CREATE = 0x01,
    DDS_EVENT_MODIFY = 0x02,
    DDS_EVENT_DELETE = 0x04,
    DDS_EVENT_RENAME = 0x08,
    DDS_EVENT_ATTRIB = 0x10,
    DDS_EVENT_ALL = 0xFF
} dds_watch_event_t;

typedef struct {
    char path[DDS_RELOAD_MAX_PATH_LEN];
    dds_watch_type_t type;
    uint32_t events;  /* Bitmask of dds_watch_event_t */
    bool recursive;
    bool enabled;
    int watch_descriptor;
} dds_watch_entry_t;

/* ============================================================================
 * Callback Types
 * ============================================================================ */
typedef enum {
    DDS_RELOAD_EVENT_CONFIG_CHANGED = 0,
    DDS_RELOAD_EVENT_CONFIG_RELOADED,
    DDS_RELOAD_EVENT_CONFIG_ROLLBACK,
    DDS_RELOAD_EVENT_VALIDATION_FAILED,
    DDS_RELOAD_EVENT_APPLY_FAILED,
    DDS_RELOAD_EVENT_WATCH_TRIGGERED
} dds_reload_event_t;

typedef struct {
    dds_reload_event_t type;
    dds_config_version_t version;
    dds_config_version_t previous_version;
    dds_reload_error_t error;
    const char *message;
    const dds_config_diff_t *diff;
    void *userdata;
} dds_reload_event_data_t;

typedef int (*dds_reload_callback_t)(const dds_reload_event_data_t *event);

typedef struct {
    dds_reload_callback_t callback;
    uint32_t event_mask;  /* Events to subscribe to */
    void *userdata;
    bool active;
} dds_reload_callback_entry_t;

/* ============================================================================
 * Reload Strategy Types
 * ============================================================================ */
typedef enum {
    DDS_STRATEGY_IMMEDIATE = 0,     /* Apply immediately */
    DDS_STRATEGY_DEFERRED,          /* Apply at next safe point */
    DDS_STRATEGY_MANUAL,            /* Wait for manual confirmation */
    DDS_STRATEGY_SCHEDULED          /* Apply at scheduled time */
} dds_reload_strategy_t;

typedef struct {
    dds_reload_strategy_t strategy;
    uint32_t defer_delay_ms;
    bool validate_before_apply;
    bool create_backup;
    bool allow_rollback;
    uint32_t rollback_timeout_ms;
    bool notify_on_change;
    bool log_changes;
    uint32_t max_retries;
} dds_reload_policy_t;

/* ============================================================================
 * Hot Reload Context
 * ============================================================================ */
typedef struct {
    /* Configuration */
    dds_reload_policy_t policy;
    char config_file_path[DDS_RELOAD_MAX_PATH_LEN];
    char backup_dir[DDS_RELOAD_MAX_PATH_LEN];
    
    /* Version management */
    dds_config_version_history_t version_history;
    
    /* File watching */
    dds_watch_entry_t watches[DDS_RELOAD_MAX_WATCH_PATHS];
    size_t watch_count;
    int inotify_fd;  /* Linux inotify file descriptor */
    
    /* Callbacks */
    dds_reload_callback_entry_t callbacks[DDS_RELOAD_MAX_CALLBACKS];
    size_t callback_count;
    
    /* State */
    bool initialized;
    bool monitoring;
    bool reloading;
    bool auto_reload;
    
    /* Threading */
    pthread_t monitor_thread;
    pthread_mutex_t lock;
    bool thread_running;
    
    /* Current configuration */
    dds_configuration_t *current_config;
    dds_configuration_t *pending_config;
} dds_hot_reload_context_t;

/* ============================================================================
 * API Functions - Lifecycle
 * ============================================================================ */

dds_hot_reload_context_t* dds_hot_reload_create(const dds_reload_policy_t *policy);
void dds_hot_reload_destroy(dds_hot_reload_context_t *ctx);
dds_reload_error_t dds_hot_reload_init(dds_hot_reload_context_t *ctx,
                                        const char *config_file);
dds_reload_error_t dds_hot_reload_shutdown(dds_hot_reload_context_t *ctx);

/* ============================================================================
 * API Functions - File Watching
 * ============================================================================ */

dds_reload_error_t dds_hot_reload_add_watch(dds_hot_reload_context_t *ctx,
                                             const char *path,
                                             dds_watch_type_t type,
                                             uint32_t events,
                                             bool recursive);
dds_reload_error_t dds_hot_reload_remove_watch(dds_hot_reload_context_t *ctx,
                                                const char *path);
dds_reload_error_t dds_hot_reload_enable_watch(dds_hot_reload_context_t *ctx,
                                                const char *path,
                                                bool enable);
dds_reload_error_t dds_hot_reload_start_monitoring(dds_hot_reload_context_t *ctx);
dds_reload_error_t dds_hot_reload_stop_monitoring(dds_hot_reload_context_t *ctx);

/* ============================================================================
 * API Functions - Callback Registration
 * ============================================================================ */

dds_reload_error_t dds_hot_reload_register_callback(dds_hot_reload_context_t *ctx,
                                                     dds_reload_callback_t callback,
                                                     uint32_t event_mask,
                                                     void *userdata);
dds_reload_error_t dds_hot_reload_unregister_callback(dds_hot_reload_context_t *ctx,
                                                       dds_reload_callback_t callback);

/* ============================================================================
 * API Functions - Configuration Reload
 * ============================================================================ */

dds_reload_error_t dds_hot_reload_trigger(dds_hot_reload_context_t *ctx);
dds_reload_error_t dds_hot_reload_trigger_async(dds_hot_reload_context_t *ctx);
dds_reload_error_t dds_hot_reload_validate_pending(dds_hot_reload_context_t *ctx,
                                                    dds_config_diff_t **diff);
dds_reload_error_t dds_hot_reload_apply_pending(dds_hot_reload_context_t *ctx,
                                                 dds_config_version_t *new_version);
dds_reload_error_t dds_hot_reload_discard_pending(dds_hot_reload_context_t *ctx);
dds_reload_error_t dds_hot_reload_schedule(dds_hot_reload_context_t *ctx,
                                            uint64_t trigger_time_ms);

/* ============================================================================
 * API Functions - Version Management
 * ============================================================================ */

dds_config_version_t dds_hot_reload_get_current_version(dds_hot_reload_context_t *ctx);
dds_config_version_t dds_hot_reload_get_latest_version(dds_hot_reload_context_t *ctx);
dds_reload_error_t dds_hot_reload_rollback(dds_hot_reload_context_t *ctx,
                                            dds_config_version_t version);
dds_reload_error_t dds_hot_reload_rollback_last(dds_hot_reload_context_t *ctx);
dds_config_version_entry_t* dds_hot_reload_get_version_info(
    dds_hot_reload_context_t *ctx,
    dds_config_version_t version);
dds_reload_error_t dds_hot_reload_list_versions(dds_hot_reload_context_t *ctx,
                                                 dds_config_version_entry_t **versions,
                                                 size_t *count);
dds_reload_error_t dds_hot_reload_cleanup_old_versions(dds_hot_reload_context_t *ctx,
                                                        size_t keep_count);

/* ============================================================================
 * API Functions - Diff and Change Analysis
 * ============================================================================ */

dds_config_diff_t* dds_config_diff_create(void);
void dds_config_diff_destroy(dds_config_diff_t *diff);
dds_reload_error_t dds_config_diff_calculate(const dds_configuration_t *old_config,
                                              const dds_configuration_t *new_config,
                                              dds_config_diff_t *diff);
dds_reload_error_t dds_config_diff_analyze(dds_config_diff_t *diff);
bool dds_config_diff_can_hot_reload(const dds_config_diff_t *diff);
bool dds_config_diff_requires_restart(const dds_config_diff_t *diff);
void dds_config_diff_print(const dds_config_diff_t *diff, FILE *output);

/* ============================================================================
 * API Functions - Policy and Settings
 * ============================================================================ */

dds_reload_error_t dds_hot_reload_set_policy(dds_hot_reload_context_t *ctx,
                                              const dds_reload_policy_t *policy);
dds_reload_error_t dds_hot_reload_get_policy(dds_hot_reload_context_t *ctx,
                                              dds_reload_policy_t *policy);
dds_reload_error_t dds_hot_reload_enable_auto_reload(dds_hot_reload_context_t *ctx,
                                                      bool enable);
dds_reload_error_t dds_hot_reload_set_backup_dir(dds_hot_reload_context_t *ctx,
                                                  const char *dir);

/* ============================================================================
 * API Functions - Status and Statistics
 * ============================================================================ */

typedef struct {
    uint32_t reload_count;
    uint32_t rollback_count;
    uint32_t failure_count;
    uint64_t last_reload_time_ms;
    uint64_t total_watch_events;
    bool is_monitoring;
    bool is_reloading;
    dds_config_version_t current_version;
    dds_config_version_t stable_version;
} dds_hot_reload_stats_t;

dds_reload_error_t dds_hot_reload_get_stats(dds_hot_reload_context_t *ctx,
                                             dds_hot_reload_stats_t *stats);
const char* dds_hot_reload_error_string(dds_reload_error_t error);
const char* dds_reload_event_string(dds_reload_event_t event);
const char* dds_change_type_string(dds_change_type_t change);
const char* dds_config_section_string(dds_config_section_t section);

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

dds_reload_policy_t dds_reload_policy_default(void);
dds_reload_policy_t dds_reload_policy_conservative(void);
dds_reload_policy_t dds_reload_policy_aggressive(void);
void dds_reload_policy_init(dds_reload_policy_t *policy);

uint64_t dds_reload_get_time_ms(void);
dds_config_version_t dds_reload_generate_version(void);
void dds_reload_compute_hash(const char *data, size_t len, char *hash_out);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_HOT_RELOAD_H */
