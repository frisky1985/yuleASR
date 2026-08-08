/*
 * DDS Configuration Tool - Hot Reload Implementation
 *
 * Copyright (c) 2024 DDS Config Tool Authors
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#include "config_hot_reload.h"
#include "dds_config_parser.h"
#include "dds_config_validator.h"

/* ============================================================================
 * Constants
 * ============================================================================ */
#define INOTIFY_EVENT_SIZE  (sizeof(struct inotify_event))
#define INOTIFY_BUF_LEN     (1024 * (INOTIFY_EVENT_SIZE + 16))
#define DEFAULT_BACKUP_DIR  "./config_backup"

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

#define LOCK(ctx) pthread_mutex_lock(&(ctx)->lock)
#define UNLOCK(ctx) pthread_mutex_unlock(&(ctx)->lock)

/* ============================================================================
 * Static Variables
 * ============================================================================ */
static volatile sig_atomic_t g_reload_signaled = 0;

/* ============================================================================
 * Error Strings
 * ============================================================================ */
const char* dds_hot_reload_error_string(dds_reload_error_t error)
{
    switch (error) {
        case DDS_RELOAD_OK: return "Success";
        case DDS_RELOAD_ERR_INVALID_ARGUMENT: return "Invalid argument";
        case DDS_RELOAD_ERR_NOT_INITIALIZED: return "Not initialized";
        case DDS_RELOAD_ERR_ALREADY_RUNNING: return "Already running";
        case DDS_RELOAD_ERR_WATCH_LIMIT: return "Watch limit reached";
        case DDS_RELOAD_ERR_CALLBACK_LIMIT: return "Callback limit reached";
        case DDS_RELOAD_ERR_FILE_NOT_FOUND: return "File not found";
        case DDS_RELOAD_ERR_PARSE_FAILED: return "Parse failed";
        case DDS_RELOAD_ERR_VALIDATION_FAILED: return "Validation failed";
        case DDS_RELOAD_ERR_APPLY_FAILED: return "Apply failed";
        case DDS_RELOAD_ERR_ROLLBACK_FAILED: return "Rollback failed";
        case DDS_RELOAD_ERR_MEMORY: return "Memory allocation failed";
        case DDS_RELOAD_ERR_SYSTEM: return "System error";
        default: return "Unknown error";
    }
}

const char* dds_reload_event_string(dds_reload_event_t event)
{
    switch (event) {
        case DDS_RELOAD_EVENT_CONFIG_CHANGED: return "CONFIG_CHANGED";
        case DDS_RELOAD_EVENT_CONFIG_RELOADED: return "CONFIG_RELOADED";
        case DDS_RELOAD_EVENT_CONFIG_ROLLBACK: return "CONFIG_ROLLBACK";
        case DDS_RELOAD_EVENT_VALIDATION_FAILED: return "VALIDATION_FAILED";
        case DDS_RELOAD_EVENT_APPLY_FAILED: return "APPLY_FAILED";
        case DDS_RELOAD_EVENT_WATCH_TRIGGERED: return "WATCH_TRIGGERED";
        default: return "UNKNOWN";
    }
}

const char* dds_change_type_string(dds_change_type_t change)
{
    switch (change) {
        case DDS_CHANGE_ADDED: return "ADDED";
        case DDS_CHANGE_MODIFIED: return "MODIFIED";
        case DDS_CHANGE_REMOVED: return "REMOVED";
        case DDS_CHANGE_REORDERED: return "REORDERED";
        default: return "UNKNOWN";
    }
}

const char* dds_config_section_string(dds_config_section_t section)
{
    switch (section) {
        case DDS_CONFIG_SECTION_SYSTEM: return "System";
        case DDS_CONFIG_SECTION_DOMAIN: return "Domain";
        case DDS_CONFIG_SECTION_TOPIC: return "Topic";
        case DDS_CONFIG_SECTION_QOS: return "QoS";
        case DDS_CONFIG_SECTION_PARTICIPANT: return "Participant";
        case DDS_CONFIG_SECTION_TYPE: return "Type";
        case DDS_CONFIG_SECTION_SECURITY: return "Security";
        case DDS_CONFIG_SECTION_MONITORING: return "Monitoring";
        case DDS_CONFIG_SECTION_LOGGING: return "Logging";
        default: return "Unknown";
    }
}

/* ============================================================================
 * Time and Version Utilities
 * ============================================================================ */

uint64_t dds_reload_get_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

dds_config_version_t dds_reload_generate_version(void)
{
    static uint32_t counter = 0;
    uint64_t timestamp = dds_reload_get_time_ms();
    return (dds_config_version_t)((timestamp << 16) | (counter++ & 0xFFFF));
}

void dds_reload_compute_hash(const char *data, size_t len, char *hash_out)
{
    /* Simple hash for demonstration - use SHA-256 in production */
    unsigned long hash = 5381;
    for (size_t i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + data[i];
    }
    snprintf(hash_out, DDS_RELOAD_HASH_SIZE, "%016lx", hash);
}

/* ============================================================================
 * Policy Functions
 * ============================================================================ */

dds_reload_policy_t dds_reload_policy_default(void)
{
    dds_reload_policy_t policy;
    dds_reload_policy_init(&policy);
    return policy;
}

dds_reload_policy_t dds_reload_policy_conservative(void)
{
    dds_reload_policy_t policy;
    policy.strategy = DDS_STRATEGY_MANUAL;
    policy.defer_delay_ms = 5000;
    policy.validate_before_apply = true;
    policy.create_backup = true;
    policy.allow_rollback = true;
    policy.rollback_timeout_ms = 30000;
    policy.notify_on_change = true;
    policy.log_changes = true;
    policy.max_retries = 3;
    return policy;
}

dds_reload_policy_t dds_reload_policy_aggressive(void)
{
    dds_reload_policy_t policy;
    policy.strategy = DDS_STRATEGY_IMMEDIATE;
    policy.defer_delay_ms = 100;
    policy.validate_before_apply = true;
    policy.create_backup = false;
    policy.allow_rollback = false;
    policy.rollback_timeout_ms = 0;
    policy.notify_on_change = true;
    policy.log_changes = true;
    policy.max_retries = 1;
    return policy;
}

void dds_reload_policy_init(dds_reload_policy_t *policy)
{
    if (!policy) return;
    
    policy->strategy = DDS_STRATEGY_DEFERRED;
    policy->defer_delay_ms = 1000;
    policy->validate_before_apply = true;
    policy->create_backup = true;
    policy->allow_rollback = true;
    policy->rollback_timeout_ms = 30000;
    policy->notify_on_change = true;
    policy->log_changes = true;
    policy->max_retries = 2;
}

/* ============================================================================
 * Lifecycle Functions
 * ============================================================================ */

dds_hot_reload_context_t* dds_hot_reload_create(const dds_reload_policy_t *policy)
{
    dds_hot_reload_context_t *ctx = calloc(1, sizeof(dds_hot_reload_context_t));
    if (!ctx) return NULL;
    
    /* Initialize policy */
    if (policy) {
        ctx->policy = *policy;
    } else {
        ctx->policy = dds_reload_policy_default();
    }
    
    /* Initialize version history */
    ctx->version_history.capacity = DDS_RELOAD_MAX_CONFIG_VERSIONS;
    ctx->version_history.versions = calloc(ctx->version_history.capacity,
                                            sizeof(dds_config_version_entry_t));
    if (!ctx->version_history.versions) {
        free(ctx);
        return NULL;
    }
    
    /* Initialize mutex */
    pthread_mutex_init(&ctx->lock, NULL);
    
    /* Set default paths */
    SAFE_STRCPY(ctx->backup_dir, DEFAULT_BACKUP_DIR, DDS_RELOAD_MAX_PATH_LEN);
    
    ctx->inotify_fd = -1;
    ctx->initialized = false;
    ctx->monitoring = false;
    ctx->reloading = false;
    ctx->auto_reload = false;
    ctx->thread_running = false;
    
    return ctx;
}

void dds_hot_reload_destroy(dds_hot_reload_context_t *ctx)
{
    if (!ctx) return;
    
    /* Stop monitoring */
    dds_hot_reload_stop_monitoring(ctx);
    
    /* Free version history */
    if (ctx->version_history.versions) {
        for (size_t i = 0; i < ctx->version_history.count; i++) {
            if (ctx->version_history.versions[i].config) {
                dds_config_destroy(ctx->version_history.versions[i].config);
            }
        }
        free(ctx->version_history.versions);
    }
    
    /* Free current and pending configs */
    if (ctx->current_config) {
        dds_config_destroy(ctx->current_config);
    }
    if (ctx->pending_config) {
        dds_config_destroy(ctx->pending_config);
    }
    
    /* Cleanup mutex */
    pthread_mutex_destroy(&ctx->lock);
    
    free(ctx);
}

dds_reload_error_t dds_hot_reload_init(dds_hot_reload_context_t *ctx,
                                        const char *config_file)
{
    if (!ctx || !config_file) {
        return DDS_RELOAD_ERR_INVALID_ARGUMENT;
    }
    
    LOCK(ctx);
    
    if (ctx->initialized) {
        UNLOCK(ctx);
        return DDS_RELOAD_ERR_ALREADY_RUNNING;
    }
    
    /* Store config file path */
    SAFE_STRCPY(ctx->config_file_path, config_file, DDS_RELOAD_MAX_PATH_LEN);
    
    /* Initialize inotify */
    ctx->inotify_fd = inotify_init1(IN_NONBLOCK);
    if (ctx->inotify_fd < 0) {
        UNLOCK(ctx);
        return DDS_RELOAD_ERR_SYSTEM;
    }
    
    /* Load initial configuration */
    dds_config_parser_t *parser = dds_config_parser_create(NULL, NULL);
    if (!parser) {
        close(ctx->inotify_fd);
        ctx->inotify_fd = -1;
        UNLOCK(ctx);
        return DDS_RELOAD_ERR_MEMORY;
    }
    
    dds_config_error_t err = dds_config_parser_load_file(parser, config_file,
                                                          DDS_CONFIG_FORMAT_AUTO,
                                                          &ctx->current_config);
    dds_config_parser_destroy(parser);
    
    if (err != DDS_CONFIG_OK) {
        close(ctx->inotify_fd);
        ctx->inotify_fd = -1;
        UNLOCK(ctx);
        return DDS_RELOAD_ERR_PARSE_FAILED;
    }
    
    /* Create initial version entry */
    dds_config_version_entry_t *version = &ctx->version_history.versions[0];
    version->version = dds_reload_generate_version();
    version->timestamp_ms = dds_reload_get_time_ms();
    version->config = dds_config_clone(ctx->current_config);
    version->active = true;
    version->rollback_point = true;
    SAFE_STRCPY(version->description, "Initial configuration", 256);
    
    /* Compute hash */
    /* TODO: Compute from serialized config */
    SAFE_STRCPY(version->config_hash, "initial", DDS_RELOAD_HASH_SIZE);
    
    ctx->version_history.count = 1;
    ctx->version_history.current_version = version->version;
    ctx->version_history.last_stable_version = version->version;
    
    ctx->initialized = true;
    
    UNLOCK(ctx);
    return DDS_RELOAD_OK;
}

dds_reload_error_t dds_hot_reload_shutdown(dds_hot_reload_context_t *ctx)
{
    if (!ctx) return DDS_RELOAD_ERR_INVALID_ARGUMENT;
    
    LOCK(ctx);
    
    if (!ctx->initialized) {
        UNLOCK(ctx);
        return DDS_RELOAD_ERR_NOT_INITIALIZED;
    }
    
    dds_hot_reload_stop_monitoring(ctx);
    
    if (ctx->inotify_fd >= 0) {
        close(ctx->inotify_fd);
        ctx->inotify_fd = -1;
    }
    
    ctx->initialized = false;
    
    UNLOCK(ctx);
    return DDS_RELOAD_OK;
}

/* ============================================================================
 * File Watching Functions
 * ============================================================================ */

dds_reload_error_t dds_hot_reload_add_watch(dds_hot_reload_context_t *ctx,
                                             const char *path,
                                             dds_watch_type_t type,
                                             uint32_t events,
                                             bool recursive)
{
    if (!ctx || !path) return DDS_RELOAD_ERR_INVALID_ARGUMENT;
    
    LOCK(ctx);
    
    if (!ctx->initialized) {
        UNLOCK(ctx);
        return DDS_RELOAD_ERR_NOT_INITIALIZED;
    }
    
    if (ctx->watch_count >= DDS_RELOAD_MAX_WATCH_PATHS) {
        UNLOCK(ctx);
        return DDS_RELOAD_ERR_WATCH_LIMIT;
    }
    
    /* Find empty slot */
    size_t idx = ctx->watch_count;
    
    /* Setup watch */
    uint32_t mask = 0;
    if (events & DDS_EVENT_CREATE) mask |= IN_CREATE;
    if (events & DDS_EVENT_MODIFY) mask |= IN_MODIFY;
    if (events & DDS_EVENT_DELETE) mask |= IN_DELETE;
    if (events & DDS_EVENT_RENAME) mask |= IN_MOVED_FROM | IN_MOVED_TO;
    if (events & DDS_EVENT_ATTRIB) mask |= IN_ATTRIB;
    
    int wd = inotify_add_watch(ctx->inotify_fd, path, mask);
    if (wd < 0) {
        UNLOCK(ctx);
        return DDS_RELOAD_ERR_SYSTEM;
    }
    
    dds_watch_entry_t *watch = &ctx->watches[idx];
    SAFE_STRCPY(watch->path, path, DDS_RELOAD_MAX_PATH_LEN);
    watch->type = type;
    watch->events = events;
    watch->recursive = recursive;
    watch->enabled = true;
    watch->watch_descriptor = wd;
    
    ctx->watch_count++;
    
    UNLOCK(ctx);
    return DDS_RELOAD_OK;
}

dds_reload_error_t dds_hot_reload_remove_watch(dds_hot_reload_context_t *ctx,
                                                const char *path)
{
    if (!ctx || !path) return DDS_RELOAD_ERR_INVALID_ARGUMENT;
    
    LOCK(ctx);
    
    for (size_t i = 0; i < ctx->watch_count; i++) {
        if (strcmp(ctx->watches[i].path, path) == 0) {
            inotify_rm_watch(ctx->inotify_fd, ctx->watches[i].watch_descriptor);
            
            /* Shift remaining watches */
            if (i < ctx->watch_count - 1) {
                memmove(&ctx->watches[i], &ctx->watches[i + 1],
                        (ctx->watch_count - i - 1) * sizeof(dds_watch_entry_t));
            }
            
            ctx->watch_count--;
            UNLOCK(ctx);
            return DDS_RELOAD_OK;
        }
    }
    
    UNLOCK(ctx);
    return DDS_RELOAD_ERR_FILE_NOT_FOUND;
}

dds_reload_error_t dds_hot_reload_enable_watch(dds_hot_reload_context_t *ctx,
                                                const char *path,
                                                bool enable)
{
    if (!ctx || !path) return DDS_RELOAD_ERR_INVALID_ARGUMENT;
    
    LOCK(ctx);
    
    for (size_t i = 0; i < ctx->watch_count; i++) {
        if (strcmp(ctx->watches[i].path, path) == 0) {
            ctx->watches[i].enabled = enable;
            UNLOCK(ctx);
            return DDS_RELOAD_OK;
        }
    }
    
    UNLOCK(ctx);
    return DDS_RELOAD_ERR_FILE_NOT_FOUND;
}

/* ============================================================================
 * Monitor Thread
 * ============================================================================ */

static void* dds_hot_reload_monitor_thread(void *arg)
{
    dds_hot_reload_context_t *ctx = (dds_hot_reload_context_t*)arg;
    char buffer[INOTIFY_BUF_LEN];
    
    while (ctx->thread_running) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(ctx->inotify_fd, &fds);
        
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;  /* 100ms timeout */
        
        int ret = select(ctx->inotify_fd + 1, &fds, NULL, NULL, &tv);
        
        if (ret > 0 && FD_ISSET(ctx->inotify_fd, &fds)) {
            ssize_t len = read(ctx->inotify_fd, buffer, INOTIFY_BUF_LEN);
            
            if (len > 0) {
                /* Process events */
                for (char *ptr = buffer; ptr < buffer + len; ) {
                    struct inotify_event *event = (struct inotify_event*)ptr;
                    
                    if (event->len > 0) {
                        /* Find watch entry */
                        for (size_t i = 0; i < ctx->watch_count; i++) {
                            if (ctx->watches[i].watch_descriptor == event->wd &&
                                ctx->watches[i].enabled) {
                                /* Notify callbacks */
                                for (size_t j = 0; j < ctx->callback_count; j++) {
                                    if (ctx->callbacks[j].active &&
                                        ctx->callbacks[j].callback) {
                                        dds_reload_event_data_t event_data;
                                        event_data.type = DDS_RELOAD_EVENT_WATCH_TRIGGERED;
                                        event_data.userdata = ctx->callbacks[j].userdata;
                                        ctx->callbacks[j].callback(&event_data);
                                    }
                                }
                                
                                /* Auto-reload if enabled */
                                if (ctx->auto_reload) {
                                    dds_hot_reload_trigger_async(ctx);
                                }
                                
                                break;
                            }
                        }
                    }
                    
                    ptr += INOTIFY_EVENT_SIZE + event->len;
                }
            }
        }
        
        /* Check for signal-based reload trigger */
        if (g_reload_signaled) {
            g_reload_signaled = 0;
            dds_hot_reload_trigger_async(ctx);
        }
    }
    
    return NULL;
}

dds_reload_error_t dds_hot_reload_start_monitoring(dds_hot_reload_context_t *ctx)
{
    if (!ctx) return DDS_RELOAD_ERR_INVALID_ARGUMENT;
    
    LOCK(ctx);
    
    if (!ctx->initialized) {
        UNLOCK(ctx);
        return DDS_RELOAD_ERR_NOT_INITIALIZED;
    }
    
    if (ctx->monitoring) {
        UNLOCK(ctx);
        return DDS_RELOAD_ERR_ALREADY_RUNNING;
    }
    
    /* Add default watch for config file if not already watched */
    if (ctx->config_file_path[0] != '\0') {
        bool has_watch = false;
        for (size_t i = 0; i < ctx->watch_count; i++) {
            if (strcmp(ctx->watches[i].path, ctx->config_file_path) == 0) {
                has_watch = true;
                break;
            }
        }
        
        if (!has_watch) {
            UNLOCK(ctx);
            dds_reload_error_t err = dds_hot_reload_add_watch(
                ctx, ctx->config_file_path, DDS_WATCH_FILE,
                DDS_EVENT_MODIFY | DDS_EVENT_CREATE, false);
            if (err != DDS_RELOAD_OK) {
                return err;
            }
            LOCK(ctx);
        }
    }
    
    ctx->thread_running = true;
    ctx->monitoring = true;
    
    if (pthread_create(&ctx->monitor_thread, NULL, 
                       dds_hot_reload_monitor_thread, ctx) != 0) {
        ctx->thread_running = false;
        ctx->monitoring = false;
        UNLOCK(ctx);
        return DDS_RELOAD_ERR_SYSTEM;
    }
    
    UNLOCK(ctx);
    return DDS_RELOAD_OK;
}

dds_reload_error_t dds_hot_reload_stop_monitoring(dds_hot_reload_context_t *ctx)
{
    if (!ctx) return DDS_RELOAD_ERR_INVALID_ARGUMENT;
    
    LOCK(ctx);
    
    if (!ctx->monitoring) {
        UNLOCK(ctx);
        return DDS_RELOAD_OK;
    }
    
    ctx->thread_running = false;
    ctx->monitoring = false;
    
    UNLOCK(ctx);
    
    pthread_join(ctx->monitor_thread, NULL);
    
    return DDS_RELOAD_OK;
}

/* ============================================================================
 * Callback Registration
 * ============================================================================ */

dds_reload_error_t dds_hot_reload_register_callback(dds_hot_reload_context_t *ctx,
                                                     dds_reload_callback_t callback,
                                                     uint32_t event_mask,
                                                     void *userdata)
{
    if (!ctx || !callback) return DDS_RELOAD_ERR_INVALID_ARGUMENT;
    
    LOCK(ctx);
    
    if (ctx->callback_count >= DDS_RELOAD_MAX_CALLBACKS) {
        UNLOCK(ctx);
        return DDS_RELOAD_ERR_CALLBACK_LIMIT;
    }
    
    ctx->callbacks[ctx->callback_count].callback = callback;
    ctx->callbacks[ctx->callback_count].event_mask = event_mask;
    ctx->callbacks[ctx->callback_count].userdata = userdata;
    ctx->callbacks[ctx->callback_count].active = true;
    ctx->callback_count++;
    
    UNLOCK(ctx);
    return DDS_RELOAD_OK;
}

dds_reload_error_t dds_hot_reload_unregister_callback(dds_hot_reload_context_t *ctx,
                                                       dds_reload_callback_t callback)
{
    if (!ctx || !callback) return DDS_RELOAD_ERR_INVALID_ARGUMENT;
    
    LOCK(ctx);
    
    for (size_t i = 0; i < ctx->callback_count; i++) {
        if (ctx->callbacks[i].callback == callback) {
            /* Shift remaining callbacks */
            if (i < ctx->callback_count - 1) {
                memmove(&ctx->callbacks[i], &ctx->callbacks[i + 1],
                        (ctx->callback_count - i - 1) * sizeof(dds_reload_callback_entry_t));
            }
            ctx->callback_count--;
            UNLOCK(ctx);
            return DDS_RELOAD_OK;
        }
    }
    
    UNLOCK(ctx);
    return DDS_RELOAD_ERR_NOT_INITIALIZED;
}

/* ============================================================================
 * Reload Functions
 * ============================================================================ */

dds_reload_error_t dds_hot_reload_trigger(dds_hot_reload_context_t *ctx)
{
    if (!ctx) return DDS_RELOAD_ERR_INVALID_ARGUMENT;
    
    LOCK(ctx);
    
    if (!ctx->initialized) {
        UNLOCK(ctx);
        return DDS_RELOAD_ERR_NOT_INITIALIZED;
    }
    
    if (ctx->reloading) {
        UNLOCK(ctx);
        return DDS_RELOAD_ERR_ALREADY_RUNNING;
    }
    
    ctx->reloading = true;
    
    UNLOCK(ctx);
    
    /* Load new configuration */
    dds_config_parser_t *parser = dds_config_parser_create(NULL, NULL);
    if (!parser) {
        ctx->reloading = false;
        return DDS_RELOAD_ERR_MEMORY;
    }
    
    dds_configuration_t *new_config = NULL;
    dds_config_error_t err = dds_config_parser_load_file(parser, ctx->config_file_path,
                                                          DDS_CONFIG_FORMAT_AUTO,
                                                          &new_config);
    dds_config_parser_destroy(parser);
    
    if (err != DDS_CONFIG_OK) {
        ctx->reloading = false;
        return DDS_RELOAD_ERR_PARSE_FAILED;
    }
    
    /* Validate new configuration */
    /* TODO: Use proper validator */
    
    /* Store as pending config */
    LOCK(ctx);
    if (ctx->pending_config) {
        dds_config_destroy(ctx->pending_config);
    }
    ctx->pending_config = new_config;
    UNLOCK(ctx);
    
    /* Apply based on strategy */
    dds_config_version_t new_version;
    dds_reload_error_t reload_err;
    
    if (ctx->policy.strategy == DDS_STRATEGY_IMMEDIATE) {
        reload_err = dds_hot_reload_apply_pending(ctx, &new_version);
    } else {
        /* For deferred/manual, just validate and notify */
        dds_config_diff_t *diff = NULL;
        reload_err = dds_hot_reload_validate_pending(ctx, &diff);
        if (reload_err == DDS_RELOAD_OK && diff) {
            dds_config_diff_destroy(diff);
        }
    }
    
    ctx->reloading = false;
    
    return reload_err;
}

dds_reload_error_t dds_hot_reload_trigger_async(dds_hot_reload_context_t *ctx)
{
    /* For simplicity, just call sync version */
    /* In production, this would queue the request */
    return dds_hot_reload_trigger(ctx);
}

dds_reload_error_t dds_hot_reload_validate_pending(dds_hot_reload_context_t *ctx,
                                                    dds_config_diff_t **diff)
{
    if (!ctx || !diff) return DDS_RELOAD_ERR_INVALID_ARGUMENT;
    
    LOCK(ctx);
    
    if (!ctx->pending_config) {
        UNLOCK(ctx);
        return DDS_RELOAD_ERR_NOT_INITIALIZED;
    }
    
    *diff = dds_config_diff_create();
    if (!*diff) {
        UNLOCK(ctx);
        return DDS_RELOAD_ERR_MEMORY;
    }
    
    dds_reload_error_t err = DDS_RELOAD_OK;
    
    if (ctx->current_config && ctx->pending_config) {
        dds_reload_error_t calc_err = dds_config_diff_calculate(
            ctx->current_config, ctx->pending_config, *diff);
        
        if (calc_err != DDS_RELOAD_OK) {
            dds_config_diff_destroy(*diff);
            *diff = NULL;
            err = DDS_RELOAD_ERR_VALIDATION_FAILED;
        }
    }
    
    UNLOCK(ctx);
    return err;
}

dds_reload_error_t dds_hot_reload_apply_pending(dds_hot_reload_context_t *ctx,
                                                 dds_config_version_t *new_version)
{
    if (!ctx || !new_version) return DDS_RELOAD_ERR_INVALID_ARGUMENT;
    
    LOCK(ctx);
    
    if (!ctx->pending_config) {
        UNLOCK(ctx);
        return DDS_RELOAD_ERR_NOT_INITIALIZED;
    }
    
    /* Create backup if requested */
    if (ctx->policy.create_backup && ctx->current_config) {
        /* TODO: Save current config to backup */
    }
    
    /* Add to version history */
    if (ctx->version_history.count >= ctx->version_history.capacity) {
        /* Remove oldest non-active version */
        for (size_t i = 0; i < ctx->version_history.count - 1; i++) {
            if (!ctx->version_history.versions[i].active &&
                !ctx->version_history.versions[i].rollback_point) {
                if (ctx->version_history.versions[i].config) {
                    dds_config_destroy(ctx->version_history.versions[i].config);
                }
                memmove(&ctx->version_history.versions[i],
                        &ctx->version_history.versions[i + 1],
                        (ctx->version_history.count - i - 1) * 
                        sizeof(dds_config_version_entry_t));
                ctx->version_history.count--;
                break;
            }
        }
    }
    
    /* Create new version entry */
    dds_config_version_entry_t *version = 
        &ctx->version_history.versions[ctx->version_history.count];
    version->version = dds_reload_generate_version();
    version->timestamp_ms = dds_reload_get_time_ms();
    version->config = dds_config_clone(ctx->pending_config);
    version->active = true;
    version->rollback_point = true;
    SAFE_STRCPY(version->description, "Hot reload", 256);
    
    /* Mark old version as inactive */
    for (size_t i = 0; i < ctx->version_history.count; i++) {
        ctx->version_history.versions[i].active = false;
    }
    
    ctx->version_history.count++;
    ctx->version_history.current_version = version->version;
    ctx->version_history.last_stable_version = version->version;
    
    /* Swap configs */
    dds_config_destroy(ctx->current_config);
    ctx->current_config = ctx->pending_config;
    ctx->pending_config = NULL;
    
    *new_version = version->version;
    
    UNLOCK(ctx);
    
    /* Notify callbacks */
    for (size_t i = 0; i < ctx->callback_count; i++) {
        if (ctx->callbacks[i].active &&
            (ctx->callbacks[i].event_mask & (1 << DDS_RELOAD_EVENT_CONFIG_RELOADED))) {
            dds_reload_event_data_t event_data;
            event_data.type = DDS_RELOAD_EVENT_CONFIG_RELOADED;
            event_data.version = *new_version;
            event_data.userdata = ctx->callbacks[i].userdata;
            ctx->callbacks[i].callback(&event_data);
        }
    }
    
    return DDS_RELOAD_OK;
}

dds_reload_error_t dds_hot_reload_discard_pending(dds_hot_reload_context_t *ctx)
{
    if (!ctx) return DDS_RELOAD_ERR_INVALID_ARGUMENT;
    
    LOCK(ctx);
    
    if (ctx->pending_config) {
        dds_config_destroy(ctx->pending_config);
        ctx->pending_config = NULL;
    }
    
    UNLOCK(ctx);
    return DDS_RELOAD_OK;
}

/* ============================================================================
 * Version Management
 * ============================================================================ */

dds_config_version_t dds_hot_reload_get_current_version(dds_hot_reload_context_t *ctx)
{
    if (!ctx) return 0;
    return ctx->version_history.current_version;
}

dds_config_version_t dds_hot_reload_get_latest_version(dds_hot_reload_context_t *ctx)
{
    if (!ctx || ctx->version_history.count == 0) return 0;
    return ctx->version_history.versions[ctx->version_history.count - 1].version;
}

dds_reload_error_t dds_hot_reload_rollback(dds_hot_reload_context_t *ctx,
                                            dds_config_version_t version)
{
    if (!ctx) return DDS_RELOAD_ERR_INVALID_ARGUMENT;
    
    LOCK(ctx);
    
    /* Find version */
    dds_config_version_entry_t *target = NULL;
    for (size_t i = 0; i < ctx->version_history.count; i++) {
        if (ctx->version_history.versions[i].version == version) {
            target = &ctx->version_history.versions[i];
            break;
        }
    }
    
    if (!target) {
        UNLOCK(ctx);
        return DDS_RELOAD_ERR_NOT_INITIALIZED;
    }
    
    /* Restore config */
    dds_configuration_t *restored = dds_config_clone(target->config);
    if (!restored) {
        UNLOCK(ctx);
        return DDS_RELOAD_ERR_MEMORY;
    }
    
    dds_config_destroy(ctx->current_config);
    ctx->current_config = restored;
    ctx->version_history.current_version = version;
    
    /* Mark as active */
    for (size_t i = 0; i < ctx->version_history.count; i++) {
        ctx->version_history.versions[i].active = 
            (ctx->version_history.versions[i].version == version);
    }
    
    UNLOCK(ctx);
    
    /* Notify callbacks */
    for (size_t i = 0; i < ctx->callback_count; i++) {
        if (ctx->callbacks[i].active &&
            (ctx->callbacks[i].event_mask & (1 << DDS_RELOAD_EVENT_CONFIG_ROLLBACK))) {
            dds_reload_event_data_t event_data;
            event_data.type = DDS_RELOAD_EVENT_CONFIG_ROLLBACK;
            event_data.version = version;
            event_data.userdata = ctx->callbacks[i].userdata;
            ctx->callbacks[i].callback(&event_data);
        }
    }
    
    return DDS_RELOAD_OK;
}

dds_reload_error_t dds_hot_reload_rollback_last(dds_hot_reload_context_t *ctx)
{
    if (!ctx) return DDS_RELOAD_ERR_INVALID_ARGUMENT;
    
    LOCK(ctx);
    
    if (ctx->version_history.count < 2) {
        UNLOCK(ctx);
        return DDS_RELOAD_ERR_NOT_INITIALIZED;
    }
    
    /* Find last stable version */
    dds_config_version_t stable = ctx->version_history.last_stable_version;
    
    UNLOCK(ctx);
    
    return dds_hot_reload_rollback(ctx, stable);
}

dds_config_version_entry_t* dds_hot_reload_get_version_info(
    dds_hot_reload_context_t *ctx,
    dds_config_version_t version)
{
    if (!ctx) return NULL;
    
    for (size_t i = 0; i < ctx->version_history.count; i++) {
        if (ctx->version_history.versions[i].version == version) {
            return &ctx->version_history.versions[i];
        }
    }
    
    return NULL;
}

/* ============================================================================
 * Diff Functions
 * ============================================================================ */

dds_config_diff_t* dds_config_diff_create(void)
{
    dds_config_diff_t *diff = calloc(1, sizeof(dds_config_diff_t));
    if (!diff) return NULL;
    
    diff->capacity = DDS_RELOAD_MAX_DIFF_ENTRIES;
    diff->entries = calloc(diff->capacity, sizeof(dds_config_diff_entry_t));
    if (!diff->entries) {
        free(diff);
        return NULL;
    }
    
    return diff;
}

void dds_config_diff_destroy(dds_config_diff_t *diff)
{
    if (!diff) return;
    free(diff->entries);
    free(diff);
}

dds_reload_error_t dds_config_diff_calculate(const dds_configuration_t *old_config,
                                              const dds_configuration_t *new_config,
                                              dds_config_diff_t *diff)
{
    if (!old_config || !new_config || !diff) {
        return DDS_RELOAD_ERR_INVALID_ARGUMENT;
    }
    
    /* Compare QoS profiles */
    for (size_t i = 0; i < new_config->qos_profile_count; i++) {
        bool found = false;
        for (size_t j = 0; j < old_config->qos_profile_count; j++) {
            if (strcmp(new_config->qos_profiles[i].name, 
                       old_config->qos_profiles[j].name) == 0) {
                found = true;
                /* Compare contents */
                if (memcmp(&new_config->qos_profiles[i], 
                          &old_config->qos_profiles[j],
                          sizeof(dds_qos_profile_t)) != 0) {
                    /* Modified */
                    if (diff->count < diff->capacity) {
                        dds_config_diff_entry_t *entry = &diff->entries[diff->count++];
                        entry->type = DDS_CHANGE_MODIFIED;
                        entry->section = DDS_CONFIG_SECTION_QOS;
                        snprintf(entry->path, sizeof(entry->path), 
                                "qos_profiles.%s", new_config->qos_profiles[i].name);
                        entry->can_hot_apply = true;
                    }
                }
                break;
            }
        }
        
        if (!found && diff->count < diff->capacity) {
            /* Added */
            dds_config_diff_entry_t *entry = &diff->entries[diff->count++];
            entry->type = DDS_CHANGE_ADDED;
            entry->section = DDS_CONFIG_SECTION_QOS;
            snprintf(entry->path, sizeof(entry->path), 
                    "qos_profiles.%s", new_config->qos_profiles[i].name);
            entry->can_hot_apply = true;
        }
    }
    
    /* Compare topics */
    for (size_t i = 0; i < new_config->topic_count; i++) {
        bool found = false;
        for (size_t j = 0; j < old_config->topic_count; j++) {
            if (strcmp(new_config->topics[i].name, old_config->topics[j].name) == 0) {
                found = true;
                break;
            }
        }
        
        if (!found && diff->count < diff->capacity) {
            dds_config_diff_entry_t *entry = &diff->entries[diff->count++];
            entry->type = DDS_CHANGE_ADDED;
            entry->section = DDS_CONFIG_SECTION_TOPIC;
            snprintf(entry->path, sizeof(entry->path), 
                    "topics.%s", new_config->topics[i].name);
            entry->can_hot_apply = true;
        }
    }
    
    /* Check for critical changes */
    for (size_t i = 0; i < diff->count; i++) {
        if (!diff->entries[i].can_hot_apply) {
            diff->has_critical_changes = true;
            diff->has_restart_required = true;
            break;
        }
    }
    
    return DDS_RELOAD_OK;
}

bool dds_config_diff_can_hot_reload(const dds_config_diff_t *diff)
{
    if (!diff) return false;
    return !diff->has_restart_required;
}

bool dds_config_diff_requires_restart(const dds_config_diff_t *diff)
{
    if (!diff) return false;
    return diff->has_restart_required;
}

void dds_config_diff_print(const dds_config_diff_t *diff, FILE *output)
{
    if (!diff || !output) return;
    
    fprintf(output, "Configuration Differences:\n");
    fprintf(output, "==========================\n");
    fprintf(output, "Total changes: %zu\n", diff->count);
    fprintf(output, "Restart required: %s\n", 
            diff->has_restart_required ? "yes" : "no");
    fprintf(output, "\n");
    
    for (size_t i = 0; i < diff->count; i++) {
        const dds_config_diff_entry_t *entry = &diff->entries[i];
        fprintf(output, "[%zu] %s - %s: %s\n",
                i + 1,
                dds_change_type_string(entry->type),
                dds_config_section_string(entry->section),
                entry->path);
    }
}

/* ============================================================================
 * Statistics
 * ============================================================================ */

dds_reload_error_t dds_hot_reload_get_stats(dds_hot_reload_context_t *ctx,
                                             dds_hot_reload_stats_t *stats)
{
    if (!ctx || !stats) return DDS_RELOAD_ERR_INVALID_ARGUMENT;
    
    LOCK(ctx);
    
    /* Calculate stats from version history */
    stats->reload_count = 0;
    stats->rollback_count = 0;
    
    for (size_t i = 0; i < ctx->version_history.count; i++) {
        if (i > 0) {
            stats->reload_count++;
        }
    }
    
    if (ctx->version_history.count > 0) {
        stats->last_reload_time_ms = 
            ctx->version_history.versions[ctx->version_history.count - 1].timestamp_ms;
    } else {
        stats->last_reload_time_ms = 0;
    }
    
    stats->is_monitoring = ctx->monitoring;
    stats->is_reloading = ctx->reloading;
    stats->current_version = ctx->version_history.current_version;
    stats->stable_version = ctx->version_history.last_stable_version;
    
    UNLOCK(ctx);
    
    return DDS_RELOAD_OK;
}

dds_reload_error_t dds_hot_reload_enable_auto_reload(dds_hot_reload_context_t *ctx,
                                                      bool enable)
{
    if (!ctx) return DDS_RELOAD_ERR_INVALID_ARGUMENT;
    
    LOCK(ctx);
    ctx->auto_reload = enable;
    UNLOCK(ctx);
    
    return DDS_RELOAD_OK;
}

dds_reload_error_t dds_hot_reload_set_backup_dir(dds_hot_reload_context_t *ctx,
                                                  const char *dir)
{
    if (!ctx || !dir) return DDS_RELOAD_ERR_INVALID_ARGUMENT;
    
    LOCK(ctx);
    SAFE_STRCPY(ctx->backup_dir, dir, DDS_RELOAD_MAX_PATH_LEN);
    UNLOCK(ctx);
    
    return DDS_RELOAD_OK;
}
