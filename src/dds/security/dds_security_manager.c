/*
 * dds_security_manager.c - DDS-Security Manager Implementation
 *
 * Copyright (c) 2024-2025
 *
 * This file implements the centralized security manager that coordinates
 * authentication, access control, and encryption for DDS-Security.
 * Designed for automotive use cases per ISO/SAE 21434 and ISO 26262.
 */

#include "dds_security_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

uint64_t dds_get_current_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static void guid_to_string(const rtps_guid_t *guid, char *str, size_t str_size)
{
    if (!guid || !str || str_size < 37) {
        return;
    }
    
    snprintf(str, str_size, 
             "%02X%02X%02X%02X-%02X%02X%02X%02X-%02X%02X%02X%02X",
             guid->prefix[0], guid->prefix[1], guid->prefix[2], guid->prefix[3],
             guid->prefix[4], guid->prefix[5], guid->prefix[6], guid->prefix[7],
             guid->prefix[8], guid->prefix[9], guid->prefix[10], guid->prefix[11]);
}

static bool guid_equal(const rtps_guid_t *a, const rtps_guid_t *b)
{
    if (!a || !b) {
        return false;
    }
    return memcmp(a->prefix, b->prefix, 12) == 0 && a->entity_id == b->entity_id;
}

/* ============================================================================
 * Initialization and Deinitialization
 * ============================================================================ */

dds_security_context_t* dds_security_manager_init(const dds_security_config_t *config)
{
    if (!config) {
        return NULL;
    }

    dds_security_context_t *ctx = (dds_security_context_t*)calloc(1, sizeof(dds_security_context_t));
    if (!ctx) {
        return NULL;
    }

    ctx->state = DDS_SECMGR_STATE_INITIALIZING;
    ctx->start_time = dds_get_current_time_ms();

    /* Copy configuration */
    memcpy(&ctx->config, config, sizeof(dds_security_config_t));

    /* Allocate participant management */
    ctx->max_participants = DDS_SECMGR_MAX_PARTICIPANTS;
    ctx->participants = (dds_sec_participant_t*)calloc(ctx->max_participants, sizeof(dds_sec_participant_t));
    if (!ctx->participants) {
        free(ctx);
        return NULL;
    }

    /* Initialize sub-modules */
    ctx->auth_ctx = dds_auth_init(config);
    if (!ctx->auth_ctx) {
        free(ctx->participants);
        free(ctx);
        return NULL;
    }

    ctx->access_ctx = dds_access_init(config);
    if (!ctx->access_ctx) {
        dds_auth_deinit(ctx->auth_ctx);
        free(ctx->participants);
        free(ctx);
        return NULL;
    }

    ctx->crypto_ctx = dds_crypto_init(config);
    if (!ctx->crypto_ctx) {
        dds_access_deinit(ctx->access_ctx);
        dds_auth_deinit(ctx->auth_ctx);
        free(ctx->participants);
        free(ctx);
        return NULL;
    }

    /* Initialize event manager */
    ctx->event_mgr.max_events = DDS_SECMGR_MAX_SECURITY_EVENTS;
    ctx->event_mgr.events = (dds_security_event_record_t*)calloc(
        ctx->event_mgr.max_events, sizeof(dds_security_event_record_t));
    if (!ctx->event_mgr.events) {
        dds_crypto_deinit(ctx->crypto_ctx);
        dds_access_deinit(ctx->access_ctx);
        dds_auth_deinit(ctx->auth_ctx);
        free(ctx->participants);
        free(ctx);
        return NULL;
    }

    /* Initialize audit log manager */
    ctx->audit_mgr.max_entries = config->max_audit_entries > 0 ? 
                                  config->max_audit_entries : 
                                  DDS_SECURITY_MAX_AUDIT_LOG_ENTRIES;
    ctx->audit_mgr.log_entries = (dds_security_audit_log_t*)calloc(
        ctx->audit_mgr.max_entries, sizeof(dds_security_audit_log_t));
    if (!ctx->audit_mgr.log_entries) {
        free(ctx->event_mgr.events);
        dds_crypto_deinit(ctx->crypto_ctx);
        dds_access_deinit(ctx->access_ctx);
        dds_auth_deinit(ctx->auth_ctx);
        free(ctx->participants);
        free(ctx);
        return NULL;
    }

    /* Initialize default policy */
    ctx->policy_set.default_flags = config->policy_flags;

    ctx->state = DDS_SECMGR_STATE_READY;

    /* Log initialization */
    dds_security_log_audit(ctx, DDS_SEC_EVT_SYSTEM_STARTUP, 
                           DDS_SEC_SEVERITY_INFO, NULL,
                           "DDS-Security manager initialized");

    return ctx;
}

void dds_security_manager_deinit(dds_security_context_t *ctx)
{
    if (!ctx) {
        return;
    }

    ctx->state = DDS_SECMGR_STATE_SHUTDOWN;

    /* Log shutdown */
    dds_security_log_audit(ctx, DDS_SEC_EVT_SYSTEM_SHUTDOWN,
                           DDS_SEC_SEVERITY_INFO, NULL,
                           "DDS-Security manager shutting down");

    /* Deinitialize sub-modules */
    if (ctx->crypto_ctx) {
        dds_crypto_deinit(ctx->crypto_ctx);
    }
    if (ctx->access_ctx) {
        dds_access_deinit(ctx->access_ctx);
    }
    if (ctx->auth_ctx) {
        dds_auth_deinit(ctx->auth_ctx);
    }

    /* Free participants */
    if (ctx->participants) {
        /* Clear sensitive data */
        for (uint32_t i = 0; i < ctx->max_participants; i++) {
            if (ctx->participants[i].state != DDS_SEC_PARTICIPANT_UNAUTHENTICATED) {
                memset(ctx->participants[i].replay_window.window, 0, 
                       sizeof(ctx->participants[i].replay_window.window));
            }
        }
        free(ctx->participants);
    }

    /* Free event manager */
    if (ctx->event_mgr.events) {
        free(ctx->event_mgr.events);
    }

    /* Free audit log */
    if (ctx->audit_mgr.log_entries) {
        free(ctx->audit_mgr.log_entries);
    }

    memset(ctx, 0, sizeof(dds_security_context_t));
    free(ctx);
}

bool dds_security_manager_is_initialized(dds_security_context_t *ctx)
{
    return ctx && ctx->state == DDS_SECMGR_STATE_READY;
}

dds_security_manager_state_t dds_security_manager_get_state(dds_security_context_t *ctx)
{
    return ctx ? ctx->state : DDS_SECMGR_STATE_UNINITIALIZED;
}

/* ============================================================================
 * Participant Security Management
 * ============================================================================ */

dds_security_status_t dds_security_register_participant(dds_security_context_t *ctx,
                                                         const rtps_guid_t *guid,
                                                         const char *subject_name)
{
    if (!ctx || !guid) {
        return DDS_SECURITY_ERROR_INVALID_PARAM;
    }

    if (ctx->state != DDS_SECMGR_STATE_READY) {
        return DDS_SECURITY_ERROR_NOT_INITIALIZED;
    }

    /* Check if participant already exists */
    dds_sec_participant_t *participant = dds_security_find_participant(ctx, guid);
    if (participant) {
        return DDS_SECURITY_ERROR_ALREADY_INITIALIZED;
    }

    /* Find available slot */
    for (uint32_t i = 0; i < ctx->max_participants; i++) {
        if (ctx->participants[i].state == DDS_SEC_PARTICIPANT_UNAUTHENTICATED) {
            participant = &ctx->participants[i];
            break;
        }
    }

    if (!participant) {
        return DDS_SECURITY_ERROR_NO_MEMORY;
    }

    memset(participant, 0, sizeof(dds_sec_participant_t));
    memcpy(&participant->guid, guid, sizeof(rtps_guid_t));
    participant->state = DDS_SEC_PARTICIPANT_UNAUTHENTICATED;
    participant->created_time = dds_get_current_time_ms();
    participant->security_context = ctx;

    if (subject_name) {
        strncpy(participant->subject_name, subject_name, sizeof(participant->subject_name) - 1);
    }

    ctx->participant_count++;

    dds_security_log_audit(ctx, DDS_SEC_EVT_AUTH_BEGIN, DDS_SEC_SEVERITY_INFO,
                           guid, "Participant registered");

    return DDS_SECURITY_OK;
}

dds_security_status_t dds_security_unregister_participant(dds_security_context_t *ctx,
                                                           const rtps_guid_t *guid)
{
    if (!ctx || !guid) {
        return DDS_SECURITY_ERROR_INVALID_PARAM;
    }

    dds_sec_participant_t *participant = dds_security_find_participant(ctx, guid);
    if (!participant) {
        return DDS_SECURITY_ERROR_INVALID_PARAM;
    }

    /* Close crypto session if exists */
    if (participant->crypto_session_id != 0) {
        dds_crypto_close_session(ctx->crypto_ctx, participant->crypto_session_id);
    }

    /* Clear handshake if exists */
    if (participant->handshake) {
        dds_auth_handshake_destroy(ctx->auth_ctx, participant->handshake);
    }

    memset(participant, 0, sizeof(dds_sec_participant_t));
    ctx->participant_count--;

    dds_security_log_audit(ctx, DDS_SEC_EVT_SECURE_CHANNEL_LOST, DDS_SEC_SEVERITY_INFO,
                           guid, "Participant unregistered");

    return DDS_SECURITY_OK;
}

dds_sec_participant_t* dds_security_find_participant(dds_security_context_t *ctx,
                                                      const rtps_guid_t *guid)
{
    if (!ctx || !guid) {
        return NULL;
    }

    for (uint32_t i = 0; i < ctx->max_participants; i++) {
        if (ctx->participants[i].state != DDS_SEC_PARTICIPANT_UNAUTHENTICATED &&
            guid_equal(&ctx->participants[i].guid, guid)) {
            return &ctx->participants[i];
        }
    }

    return NULL;
}

/* ============================================================================
 * Authentication
 * ============================================================================ */

dds_security_status_t dds_security_begin_authentication(dds_security_context_t *ctx,
                                                         const rtps_guid_t *local_guid,
                                                         const rtps_guid_t *remote_guid)
{
    if (!ctx || !local_guid || !remote_guid) {
        return DDS_SECURITY_ERROR_INVALID_PARAM;
    }

    dds_sec_participant_t *local = dds_security_find_participant(ctx, local_guid);
    if (!local) {
        return DDS_SECURITY_ERROR_INVALID_PARAM;
    }

    local->state = DDS_SEC_PARTICIPANT_AUTHENTICATING;

    /* Create handshake context */
    dds_security_handshake_t *handshake = dds_auth_handshake_create(ctx->auth_ctx, 
                                                                     local_guid, 
                                                                     remote_guid);
    if (!handshake) {
        return DDS_SECURITY_ERROR_NO_MEMORY;
    }

    local->handshake = handshake;
    ctx->total_auth_attempts++;

    dds_security_trigger_event(ctx, DDS_SEC_EVT_AUTH_BEGIN, DDS_SEC_SEVERITY_INFO,
                               local_guid, "Authentication started");

    return DDS_SECURITY_OK;
}

dds_security_status_t dds_security_complete_authentication(dds_security_context_t *ctx,
                                                            const rtps_guid_t *guid)
{
    if (!ctx || !guid) {
        return DDS_SECURITY_ERROR_INVALID_PARAM;
    }

    dds_sec_participant_t *participant = dds_security_find_participant(ctx, guid);
    if (!participant) {
        return DDS_SECURITY_ERROR_INVALID_PARAM;
    }

    if (!participant->identity_verified) {
        ctx->total_auth_failures++;
        participant->auth_failures++;
        participant->state = DDS_SEC_PARTICIPANT_UNAUTHENTICATED;
        
        dds_security_trigger_event(ctx, DDS_SEC_EVT_AUTH_FAILURE, DDS_SEC_SEVERITY_ERROR,
                                   guid, "Authentication failed");
        
        return DDS_SECURITY_ERROR_AUTHENTICATION_FAILED;
    }

    /* Create crypto session */
    uint8_t shared_secret[64];
    uint32_t secret_len = 0;
    
    dds_auth_get_shared_secret(participant->handshake, shared_secret, &secret_len);

    participant->crypto_session_id = dds_crypto_create_session(ctx->crypto_ctx,
                                                                guid, guid,
                                                                shared_secret, 
                                                                secret_len);
    
    /* Clear shared secret from memory */
    memset(shared_secret, 0, sizeof(shared_secret));

    participant->state = DDS_SEC_PARTICIPANT_SECURE_ESTABLISHED;
    participant->auth_completed_time = dds_get_current_time_ms();
    ctx->total_auth_success++;

    dds_security_trigger_event(ctx, DDS_SEC_EVT_AUTH_SUCCESS, DDS_SEC_SEVERITY_INFO,
                               guid, "Authentication completed successfully");

    dds_security_log_audit(ctx, DDS_SEC_EVT_SECURE_CHANNEL_ESTABLISHED,
                           DDS_SEC_SEVERITY_INFO, guid,
                           "Secure channel established");

    return DDS_SECURITY_OK;
}

bool dds_security_is_participant_authenticated(dds_security_context_t *ctx,
                                                const rtps_guid_t *guid)
{
    if (!ctx || !guid) {
        return false;
    }

    dds_sec_participant_t *participant = dds_security_find_participant(ctx, guid);
    if (!participant) {
        return false;
    }

    return participant->state == DDS_SEC_PARTICIPANT_SECURE_ESTABLISHED;
}

/* ============================================================================
 * Data Protection
 * ============================================================================ */

dds_security_status_t dds_security_protect_data(dds_security_context_t *ctx,
                                                const rtps_guid_t *participant_guid,
                                                const char *topic_name,
                                                const uint8_t *plaintext,
                                                uint32_t plaintext_len,
                                                uint8_t *protected_data,
                                                uint32_t *protected_len)
{
    if (!ctx || !participant_guid || !plaintext || !protected_data || !protected_len) {
        return DDS_SECURITY_ERROR_INVALID_PARAM;
    }

    dds_sec_participant_t *participant = dds_security_find_participant(ctx, participant_guid);
    if (!participant) {
        return DDS_SECURITY_ERROR_INVALID_PARAM;
    }

    if (participant->state != DDS_SEC_PARTICIPANT_SECURE_ESTABLISHED) {
        return DDS_SECURITY_ERROR_AUTHENTICATION_FAILED;
    }

    if (participant->crypto_session_id == 0) {
        return DDS_SECURITY_ERROR_KEY_EXCHANGE_FAILED;
    }

    /* Encrypt data */
    dds_crypto_status_t crypto_status = dds_crypto_encrypt_rtps_data(
        ctx->crypto_ctx,
        participant->crypto_session_id,
        plaintext, plaintext_len,
        protected_data, *protected_len,
        protected_len);

    if (crypto_status != DDS_CRYPTO_OK) {
        dds_security_trigger_event(ctx, DDS_SEC_EVT_ENCRYPTION_ERROR, 
                                   DDS_SEC_SEVERITY_ERROR,
                                   participant_guid, "Encryption failed");
        return DDS_SECURITY_ERROR_ENCRYPTION_FAILED;
    }

    participant->messages_encrypted++;
    participant->last_activity = dds_get_current_time_ms();

    return DDS_SECURITY_OK;
}

dds_security_status_t dds_security_unprotect_data(dds_security_context_t *ctx,
                                                  const rtps_guid_t *participant_guid,
                                                  const rtps_guid_t *from_guid,
                                                  const uint8_t *protected_data,
                                                  uint32_t protected_len,
                                                  uint8_t *plaintext,
                                                  uint32_t *plaintext_len)
{
    if (!ctx || !participant_guid || !protected_data || !plaintext || !plaintext_len) {
        return DDS_SECURITY_ERROR_INVALID_PARAM;
    }

    dds_sec_participant_t *participant = dds_security_find_participant(ctx, participant_guid);
    if (!participant) {
        return DDS_SECURITY_ERROR_INVALID_PARAM;
    }

    if (participant->crypto_session_id == 0) {
        return DDS_SECURITY_ERROR_KEY_EXCHANGE_FAILED;
    }

    /* Check for replay attacks */
    dds_security_status_t replay_status = dds_security_detect_replay(ctx, from_guid, 0);
    if (replay_status != DDS_SECURITY_OK) {
        ctx->total_replay_detected++;
        participant->replay_detected++;
        return replay_status;
    }

    /* Decrypt data */
    dds_crypto_status_t crypto_status = dds_crypto_decrypt_rtps_data(
        ctx->crypto_ctx,
        participant->crypto_session_id,
        protected_data, protected_len,
        plaintext, *plaintext_len,
        plaintext_len);

    if (crypto_status != DDS_CRYPTO_OK) {
        participant->decrypted_failed++;
        dds_security_trigger_event(ctx, DDS_SEC_EVT_DECRYPTION_ERROR,
                                   DDS_SEC_SEVERITY_ERROR,
                                   participant_guid, "Decryption failed");
        return DDS_SECURITY_ERROR_DECRYPTION_FAILED;
    }

    participant->messages_decrypted++;
    participant->last_activity = dds_get_current_time_ms();

    return DDS_SECURITY_OK;
}

/* ============================================================================
 * Access Control
 * ============================================================================ */

dds_security_status_t dds_security_check_access(dds_security_context_t *ctx,
                                                 const rtps_guid_t *participant_guid,
                                                 dds_domain_id_t domain_id,
                                                 const char *topic_name,
                                                 dds_permission_action_t action)
{
    if (!ctx || !participant_guid || !topic_name) {
        return DDS_SECURITY_ERROR_INVALID_PARAM;
    }

    /* Skip access control if not enabled in policy */
    if (!(ctx->config.policy_flags & DDS_SEC_POLICY_ACCESS_CONTROL)) {
        return DDS_SECURITY_OK;
    }

    dds_sec_participant_t *participant = dds_security_find_participant(ctx, participant_guid);
    
    dds_access_request_t request = {
        .subject_name = participant ? participant->subject_name : "",
        .domain_id = domain_id,
        .topic_name = topic_name,
        .partition_name = NULL,
        .requested_action = action,
        .asil_level = ctx->config.required_asil_level,
        .is_encrypted = true,
        .is_authenticated = participant && participant->identity_verified
    };

    dds_access_status_t access_status;
    
    switch (action) {
        case DDS_PERM_PUBLISH:
        case DDS_PERM_WRITE:
            access_status = dds_access_check_publish(ctx->access_ctx, &request);
            break;
            
        case DDS_PERM_SUBSCRIBE:
        case DDS_PERM_READ:
            access_status = dds_access_check_subscribe(ctx->access_ctx, &request);
            break;
            
        default:
            access_status = dds_access_check_permission(ctx->access_ctx, &request) == 
                           DDS_ACCESS_DECISION_ALLOW ? DDS_ACCESS_OK : DDS_ACCESS_ERROR_ACCESS_DENIED;
    }

    if (access_status != DDS_ACCESS_OK) {
        ctx->total_access_violations++;
        
        dds_security_trigger_event(ctx, DDS_SEC_EVT_ACCESS_DENIED, 
                                   DDS_SEC_SEVERITY_WARNING,
                                   participant_guid, 
                                   "Access denied for topic: %s", topic_name);
        
        if (ctx->config.on_access_violation) {
            ctx->config.on_access_violation(topic_name, action);
        }
        
        return DDS_SECURITY_ERROR_ACCESS_DENIED;
    }

    return DDS_SECURITY_OK;
}

/* ============================================================================
 * Event Management
 * ============================================================================ */

dds_security_status_t dds_security_register_event_callback(dds_security_context_t *ctx,
                                                            dds_security_event_callback_t callback,
                                                            void *user_data)
{
    if (!ctx || !callback) {
        return DDS_SECURITY_ERROR_INVALID_PARAM;
    }

    ctx->event_mgr.callback = callback;
    ctx->event_mgr.callback_user_data = user_data;

    return DDS_SECURITY_OK;
}

dds_security_status_t dds_security_unregister_event_callback(dds_security_context_t *ctx,
                                                              dds_security_event_callback_t callback)
{
    if (!ctx) {
        return DDS_SECURITY_ERROR_INVALID_PARAM;
    }

    if (ctx->event_mgr.callback == callback) {
        ctx->event_mgr.callback = NULL;
        ctx->event_mgr.callback_user_data = NULL;
    }

    return DDS_SECURITY_OK;
}

dds_security_status_t dds_security_trigger_event(dds_security_context_t *ctx,
                                                  dds_security_event_type_t event,
                                                  dds_security_severity_t severity,
                                                  const rtps_guid_t *participant_guid,
                                                  const char *format, ...)
{
    if (!ctx || !format) {
        return DDS_SECURITY_ERROR_INVALID_PARAM;
    }

    /* Format message */
    va_list args;
    va_start(args, format);
    char message[256];
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    /* Add to event history */
    dds_security_event_record_t *record = &ctx->event_mgr.events[ctx->event_mgr.write_index];
    record->timestamp = dds_get_current_time_ms();
    record->event_type = event;
    record->severity = severity;
    if (participant_guid) {
        memcpy(&record->participant_guid, participant_guid, sizeof(rtps_guid_t));
    }
    strncpy(record->description, message, sizeof(record->description) - 1);

    ctx->event_mgr.write_index = (ctx->event_mgr.write_index + 1) % ctx->event_mgr.max_events;
    if (ctx->event_mgr.event_count < ctx->event_mgr.max_events) {
        ctx->event_mgr.event_count++;
    }

    /* Call registered callback */
    if (ctx->event_mgr.callback && severity >= ctx->event_mgr.min_severity) {
        ctx->event_mgr.callback(event, severity, participant_guid, message, 
                               ctx->event_mgr.callback_user_data);
    }

    /* Also log to audit log for critical events */
    if (severity >= DDS_SEC_SEVERITY_ERROR) {
        dds_security_log_audit(ctx, event, severity, participant_guid, message);
    }

    return DDS_SECURITY_OK;
}

/* ============================================================================
 * Audit Logging
 * ============================================================================ */

dds_security_status_t dds_security_log_audit(dds_security_context_t *ctx,
                                              dds_security_event_type_t event_type,
                                              dds_security_severity_t severity,
                                              const rtps_guid_t *participant_guid,
                                              const char *message)
{
    if (!ctx || !message) {
        return DDS_SECURITY_ERROR_INVALID_PARAM;
    }

    if (!ctx->config.enable_audit_log) {
        return DDS_SECURITY_OK;
    }

    /* Add to audit log */
    dds_security_audit_log_t *log = &ctx->audit_mgr.log_entries[ctx->audit_mgr.write_index];
    log->timestamp = dds_get_current_time_ms();
    log->event_type = event_type;
    log->severity = severity;
    if (participant_guid) {
        memcpy(&log->participant_guid, participant_guid, sizeof(rtps_guid_t));
    }
    strncpy(log->message, message, sizeof(log->message) - 1);

    ctx->audit_mgr.write_index = (ctx->audit_mgr.write_index + 1) % ctx->audit_mgr.max_entries;
    if (ctx->audit_mgr.entry_count < ctx->audit_mgr.max_entries) {
        ctx->audit_mgr.entry_count++;
    } else {
        ctx->audit_mgr.total_dropped++;
    }

    ctx->audit_mgr.total_logged++;

    /* Write to file if enabled */
    if (ctx->audit_mgr.file_logging_enabled && ctx->audit_mgr.log_file_path[0]) {
        FILE *fp = fopen(ctx->audit_mgr.log_file_path, "a");
        if (fp) {
            char guid_str[64] = "N/A";
            if (participant_guid) {
                guid_to_string(participant_guid, guid_str, sizeof(guid_str));
            }
            
            fprintf(fp, "[%llu] [%d] [%s] %s\n",
                    (unsigned long long)log->timestamp,
                    severity,
                    guid_str,
                    message);
            fclose(fp);
        }
    }

    return DDS_SECURITY_OK;
}

dds_security_status_t dds_security_configure_audit(dds_security_context_t *ctx,
                                                    bool enable,
                                                    const char *file_path,
                                                    dds_security_severity_t min_severity)
{
    if (!ctx) {
        return DDS_SECURITY_ERROR_INVALID_PARAM;
    }

    ctx->config.enable_audit_log = enable;
    ctx->audit_mgr.file_logging_enabled = enable && file_path != NULL;
    ctx->audit_mgr.min_severity = min_severity;

    if (file_path) {
        strncpy(ctx->audit_mgr.log_file_path, file_path, 
                sizeof(ctx->audit_mgr.log_file_path) - 1);
    }

    return DDS_SECURITY_OK;
}

/* ============================================================================
 * Replay Protection
 * ============================================================================ */

dds_security_status_t dds_security_detect_replay(dds_security_context_t *ctx,
                                                  const rtps_guid_t *participant_guid,
                                                  uint64_t seq_number)
{
    if (!ctx || !participant_guid) {
        return DDS_SECURITY_ERROR_INVALID_PARAM;
    }

    /* Skip replay protection if not enabled */
    if (!(ctx->config.policy_flags & DDS_SEC_POLICY_REPLAY_PROTECTION)) {
        return DDS_SECURITY_OK;
    }

    dds_sec_participant_t *participant = dds_security_find_participant(ctx, participant_guid);
    if (!participant) {
        return DDS_SECURITY_OK; /* Allow unknown participants for discovery */
    }

    /* Check sequence number */
    if (seq_number > 0 && seq_number <= participant->last_seq_number) {
        /* Check window for exact replay */
        int64_t diff = participant->last_seq_number - seq_number;
        if (diff < DDS_SECURITY_REPLAY_WINDOW_SIZE) {
            /* Check if this exact sequence number was seen */
            uint32_t idx = (participant->replay_window.write_index - diff - 1 + DDS_SECURITY_REPLAY_WINDOW_SIZE) 
                          % DDS_SECURITY_REPLAY_WINDOW_SIZE;
            if (participant->replay_window.window[idx] == seq_number) {
                dds_security_trigger_event(ctx, DDS_SEC_EVT_REPLAY_DETECTED,
                                           DDS_SEC_SEVERITY_CRITICAL,
                                           participant_guid,
                                           "Replay attack detected: seq=%llu", 
                                           (unsigned long long)seq_number);
                return DDS_SECURITY_ERROR_REPLAY_DETECTED;
            }
        }
    }

    return DDS_SECURITY_OK;
}

void dds_security_update_seq_window(dds_security_context_t *ctx,
                                    const rtps_guid_t *participant_guid,
                                    uint64_t seq_number)
{
    if (!ctx || !participant_guid) {
        return;
    }

    dds_sec_participant_t *participant = dds_security_find_participant(ctx, participant_guid);
    if (!participant) {
        return;
    }

    /* Update window */
    participant->replay_window.window[participant->replay_window.write_index] = seq_number;
    participant->replay_window.write_index = (participant->replay_window.write_index + 1) % DDS_SECURITY_REPLAY_WINDOW_SIZE;
    
    if (participant->replay_window.valid_entries < DDS_SECURITY_REPLAY_WINDOW_SIZE) {
        participant->replay_window.valid_entries++;
    }

    if (seq_number > participant->last_seq_number) {
        participant->last_seq_number = seq_number;
    }
}

/* ============================================================================
 * Maintenance Operations
 * ============================================================================ */

dds_security_status_t dds_security_maintain(dds_security_context_t *ctx,
                                            uint64_t current_time)
{
    if (!ctx) {
        return DDS_SECURITY_ERROR_INVALID_PARAM;
    }

    if (ctx->state != DDS_SECMGR_STATE_READY) {
        return DDS_SECURITY_ERROR_NOT_INITIALIZED;
    }

    /* Check handshake timeouts */
    dds_auth_check_handshake_timeouts(ctx->auth_ctx, current_time);

    /* Check key expiry */
    dds_crypto_check_key_expiry(ctx->crypto_ctx, current_time);

    /* Check certificate expiry */
    dds_security_check_certificate_expiry(ctx, current_time);

    /* Check permissions expiry */
    dds_access_check_permissions_expiry(ctx->access_ctx, current_time);

    return DDS_SECURITY_OK;
}

dds_security_status_t dds_security_rotate_keys(dds_security_context_t *ctx,
                                                const rtps_guid_t *participant_guid)
{
    if (!ctx) {
        return DDS_SECURITY_ERROR_INVALID_PARAM;
    }

    if (participant_guid) {
        /* Rotate for specific participant */
        dds_sec_participant_t *participant = dds_security_find_participant(ctx, participant_guid);
        if (participant && participant->crypto_session_id != 0) {
            dds_crypto_update_session_key(ctx->crypto_ctx, participant->crypto_session_id);
        }
    } else {
        /* Rotate for all participants */
        for (uint32_t i = 0; i < ctx->max_participants; i++) {
            if (ctx->participants[i].state == DDS_SEC_PARTICIPANT_SECURE_ESTABLISHED &&
                ctx->participants[i].crypto_session_id != 0) {
                dds_crypto_update_session_key(ctx->crypto_ctx, 
                                              ctx->participants[i].crypto_session_id);
            }
        }
    }

    dds_security_trigger_event(ctx, DDS_SEC_EVT_KEY_ROTATION, DDS_SEC_SEVERITY_INFO,
                               participant_guid, "Keys rotated");

    return DDS_SECURITY_OK;
}

uint32_t dds_security_check_certificate_expiry(dds_security_context_t *ctx,
                                               uint64_t current_time)
{
    if (!ctx) {
        return 0;
    }

    uint32_t expiry_count = 0;
    
    /* Check participant certificates */
    for (uint32_t i = 0; i < ctx->max_participants; i++) {
        dds_sec_participant_t *p = &ctx->participants[i];
        if (p->state != DDS_SEC_PARTICIPANT_UNAUTHENTICATED) {
            dds_auth_status_t status = dds_auth_check_cert_validity(
                ctx->auth_ctx, &p->identity.cert_chain.certs[0], current_time);
            
            if (status == DDS_AUTH_ERROR_CERT_EXPIRED) {
                expiry_count++;
                dds_security_trigger_event(ctx, DDS_SEC_EVT_CERT_EXPIRED,
                                           DDS_SEC_SEVERITY_ERROR,
                                           &p->guid, "Certificate expired");
            }
        }
    }

    return expiry_count;
}

/* ============================================================================
 * Statistics
 * ============================================================================ */

void dds_security_get_statistics(dds_security_context_t *ctx,
                                 uint64_t *total_auth_success,
                                 uint64_t *total_auth_failures,
                                 uint64_t *total_access_violations,
                                 uint64_t *total_replay_detected)
{
    if (!ctx) {
        return;
    }

    if (total_auth_success) *total_auth_success = ctx->total_auth_success;
    if (total_auth_failures) *total_auth_failures = ctx->total_auth_failures;
    if (total_access_violations) *total_access_violations = ctx->total_access_violations;
    if (total_replay_detected) *total_replay_detected = ctx->total_replay_detected;
}

const char* dds_security_participant_state_string(dds_sec_participant_state_t state)
{
    switch (state) {
        case DDS_SEC_PARTICIPANT_UNAUTHENTICATED:   return "UNAUTHENTICATED";
        case DDS_SEC_PARTICIPANT_AUTHENTICATING:    return "AUTHENTICATING";
        case DDS_SEC_PARTICIPANT_AUTHENTICATED:     return "AUTHENTICATED";
        case DDS_SEC_PARTICIPANT_AUTHORIZED:        return "AUTHORIZED";
        case DDS_SEC_PARTICIPANT_SECURE_ESTABLISHED: return "SECURE_ESTABLISHED";
        case DDS_SEC_PARTICIPANT_REVOKED:           return "REVOKED";
        default: return "UNKNOWN";
    }
}

const char* dds_security_error_string(dds_security_status_t status)
{
    switch (status) {
        case DDS_SECURITY_OK:                       return "OK";
        case DDS_SECURITY_ERROR_GENERIC:            return "Generic error";
        case DDS_SECURITY_ERROR_INVALID_PARAM:      return "Invalid parameter";
        case DDS_SECURITY_ERROR_NO_MEMORY:          return "Out of memory";
        case DDS_SECURITY_ERROR_CERT_INVALID:       return "Invalid certificate";
        case DDS_SECURITY_ERROR_CERT_EXPIRED:       return "Certificate expired";
        case DDS_SECURITY_ERROR_CERT_REVOKED:       return "Certificate revoked";
        case DDS_SECURITY_ERROR_SIGNATURE_INVALID:  return "Invalid signature";
        case DDS_SECURITY_ERROR_AUTHENTICATION_FAILED: return "Authentication failed";
        case DDS_SECURITY_ERROR_ACCESS_DENIED:      return "Access denied";
        case DDS_SECURITY_ERROR_PERMISSIONS_INVALID: return "Invalid permissions";
        case DDS_SECURITY_ERROR_ENCRYPTION_FAILED:  return "Encryption failed";
        case DDS_SECURITY_ERROR_DECRYPTION_FAILED:  return "Decryption failed";
        case DDS_SECURITY_ERROR_REPLAY_DETECTED:    return "Replay attack detected";
        case DDS_SECURITY_ERROR_KEY_EXCHANGE_FAILED: return "Key exchange failed";
        case DDS_SECURITY_ERROR_NOT_INITIALIZED:    return "Not initialized";
        case DDS_SECURITY_ERROR_ALREADY_INITIALIZED: return "Already initialized";
        case DDS_SECURITY_ERROR_PLUGIN_NOT_FOUND:   return "Plugin not found";
        case DDS_SECURITY_ERROR_TIMEOUT:            return "Timeout";
        default: return "Unknown error";
    }
}
