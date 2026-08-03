/**
 * @file keym_core.c
 * @brief KeyM (Key Manager) Core Implementation
 * @version 1.0
 * @date 2026-04-25
 *
 * Implementation of AUTOSAR KeyM 4.4 specification
 * Key slot management, key derivation, and key rotation
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "keym_core.h"

#define KEYM_VERSION "4.4.0-AUTOSAR"

/* ============================================================================
 * Internal Function Declarations
 * ============================================================================ */

static keym_status_t keym_validate_slot(keym_context_t *ctx, uint8_t slot_id);
static void keym_update_version_history(keym_context_t *ctx, uint8_t slot_id);
static const char* keym_get_key_type_name_internal(keym_key_type_t type);
static const char* keym_get_key_state_name_internal(keym_key_state_t state);
static uint32_t keym_get_key_type_size(keym_key_type_t type);
static bool keym_is_key_usage_allowed(keym_slot_info_t *slot, keym_key_usage_t usage);

/* ============================================================================
 * Initialization/Deinitialization
 * ============================================================================ */

keym_context_t* keym_init(void *cryif, void *csm)
{
    keym_context_t *ctx;
    
    ctx = (keym_context_t*)calloc(1, sizeof(keym_context_t));
    if (ctx == NULL) {
        return NULL;
    }
    
    /* Initialize key slots */
    for (int i = 0; i < KEYM_MAX_KEY_SLOTS; i++) {
        ctx->slots[i].slot_id = i;
        ctx->slots[i].state = KEYM_STATE_EMPTY;
        ctx->slots[i].key_version = 0;
        ctx->slots[i].key_generation = 0;
        ctx->slots[i].parent_slot_id = KEYM_SLOT_ID_INVALID;
        ctx->slots[i].cryif_slot_id = CRYIF_KEY_SLOT_INVALID;
    }
    
    /* Initialize certificates */
    for (int i = 0; i < KEYM_MAX_CERTIFICATES; i++) {
        ctx->certificates[i].cert_id = i;
        ctx->certificates[i].is_revoked = false;
    }
    
    /* Set default rotation policy */
    ctx->rotation_policy.rotation_interval_ms = KEYM_ROTATION_INTERVAL_MS;
    ctx->rotation_policy.max_key_age_ms = KEYM_MAX_KEY_AGE_MS;
    ctx->rotation_policy.overlap_period_ms = 3600000; /* 1 hour */
    ctx->rotation_policy.auto_rotation = false;
    ctx->rotation_policy.keep_old_versions = true;
    ctx->rotation_policy.max_old_versions = 3;
    
    /* Set interfaces */
    ctx->cryif = cryif;
    ctx->csm = csm;
    ctx->dds_sec = NULL;
    
    /* Initialize statistics */
    ctx->total_key_operations = 0;
    ctx->total_derivations = 0;
    ctx->total_rotations = 0;
    
    ctx->initialized = true;
    
    return ctx;
}

void keym_deinit(keym_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return;
    }
    
    /* Clear all key slots */
    for (int i = 0; i < KEYM_MAX_KEY_SLOTS; i++) {
        if (ctx->slots[i].state != KEYM_STATE_EMPTY) {
            keym_slot_free(ctx, i);
        }
    }
    
    /* Free certificate data */
    for (int i = 0; i < KEYM_MAX_CERTIFICATES; i++) {
        if (ctx->certificates[i].cert_data != NULL) {
            free(ctx->certificates[i].cert_data);
        }
    }
    
    ctx->initialized = false;
    free(ctx);
}

/* ============================================================================
 * Key Slot Management
 * ============================================================================ */

keym_status_t keym_slot_allocate(keym_context_t *ctx, uint8_t *slot_id,
                                 const char *name, keym_key_type_t key_type)
{
    if (ctx == NULL || !ctx->initialized || slot_id == NULL) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    /* If specific slot requested */
    if (*slot_id != KEYM_SLOT_ID_INVALID) {
        if (*slot_id >= KEYM_MAX_KEY_SLOTS) {
            return KEYM_ERROR_INVALID_PARAM;
        }
        if (ctx->slots[*slot_id].state != KEYM_STATE_EMPTY) {
            return KEYM_ERROR_SLOT_OCCUPIED;
        }
    } else {
        /* Find free slot */
        for (int i = 0; i < KEYM_MAX_KEY_SLOTS; i++) {
            if (ctx->slots[i].state == KEYM_STATE_EMPTY) {
                *slot_id = i;
                break;
            }
        }
        if (*slot_id == KEYM_SLOT_ID_INVALID) {
            return KEYM_ERROR_NO_MEMORY;
        }
    }
    
    /* Initialize slot */
    keym_slot_info_t *slot = &ctx->slots[*slot_id];
    slot->state = KEYM_STATE_INACTIVE;
    slot->key_type = key_type;
    slot->key_len = keym_get_key_type_size(key_type);
    
    if (name != NULL) {
        strncpy(slot->name, name, KEYM_MAX_KEY_NAME_LEN - 1);
        slot->name[KEYM_MAX_KEY_NAME_LEN - 1] = '\0';
    }
    
    /* Allocate CryIf key slot if available */
    if (ctx->cryif != NULL) {
        /* This would call cryif_key_slot_allocate */
        slot->cryif_slot_id = *slot_id;  /* Simplified mapping */
    }
    
    return KEYM_OK;
}

keym_status_t keym_slot_free(keym_context_t *ctx, uint8_t slot_id)
{
    keym_status_t status;
    
    status = keym_validate_slot(ctx, slot_id);
    if (status != KEYM_OK) {
        return status;
    }
    
    /* Erase key material */
    memset(&ctx->materials[slot_id], 0, sizeof(keym_key_material_t));
    
    /* Free CryIf slot if allocated */
    if (ctx->slots[slot_id].cryif_slot_id != CRYIF_KEY_SLOT_INVALID &&
        ctx->cryif != NULL) {
        /* cryif_key_slot_free(ctx->cryif, ctx->slots[slot_id].cryif_slot_id); */
    }
    
    /* Clear slot info */
    memset(&ctx->slots[slot_id], 0, sizeof(keym_slot_info_t));
    ctx->slots[slot_id].slot_id = slot_id;
    ctx->slots[slot_id].state = KEYM_STATE_EMPTY;
    ctx->slots[slot_id].parent_slot_id = KEYM_SLOT_ID_INVALID;
    ctx->slots[slot_id].cryif_slot_id = CRYIF_KEY_SLOT_INVALID;
    
    return KEYM_OK;
}

keym_status_t keym_slot_get_info(keym_context_t *ctx, uint8_t slot_id,
                                 keym_slot_info_t *info)
{
    keym_status_t status;
    
    status = keym_validate_slot(ctx, slot_id);
    if (status != KEYM_OK) {
        return status;
    }
    
    if (info == NULL) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    memcpy(info, &ctx->slots[slot_id], sizeof(keym_slot_info_t));
    return KEYM_OK;
}

uint8_t keym_slot_find_by_name(keym_context_t *ctx, const char *name)
{
    if (ctx == NULL || !ctx->initialized || name == NULL) {
        return KEYM_SLOT_ID_INVALID;
    }
    
    for (int i = 0; i < KEYM_MAX_KEY_SLOTS; i++) {
        if (ctx->slots[i].state != KEYM_STATE_EMPTY &&
            strcmp(ctx->slots[i].name, name) == 0) {
            return i;
        }
    }
    
    return KEYM_SLOT_ID_INVALID;
}

keym_status_t keym_slot_set_attributes(keym_context_t *ctx, uint8_t slot_id,
                                       keym_key_usage_t usage_flags,
                                       bool persistent, bool exportable)
{
    keym_status_t status;
    
    status = keym_validate_slot(ctx, slot_id);
    if (status != KEYM_OK) {
        return status;
    }
    
    ctx->slots[slot_id].usage_flags = usage_flags;
    ctx->slots[slot_id].is_persistent = persistent;
    ctx->slots[slot_id].is_exportable = exportable;
    
    return KEYM_OK;
}

/* ============================================================================
 * Key Import/Export
 * ============================================================================ */

keym_status_t keym_key_import(keym_context_t *ctx, uint8_t slot_id,
                              const uint8_t *key_data, uint32_t key_len,
                              uint32_t key_version)
{
    keym_status_t status;
    keym_key_material_t *material;
    
    status = keym_validate_slot(ctx, slot_id);
    if (status != KEYM_OK) {
        return status;
    }
    
    if (key_data == NULL || key_len == 0) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    /* Check key length */
    if (key_len > KEYM_MAX_KEY_MATERIAL_SIZE) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    /* Update version history */
    if (ctx->slots[slot_id].key_version > 0) {
        keym_update_version_history(ctx, slot_id);
    }
    
    /* Store key material */
    material = &ctx->materials[slot_id];
    if (material->key_data == NULL) {
        material->key_data = malloc(KEYM_MAX_KEY_MATERIAL_SIZE);
        if (material->key_data == NULL) {
            return KEYM_ERROR_NO_MEMORY;
        }
    }
    
    memcpy(material->key_data, key_data, key_len);
    material->key_data_len = key_len;
    material->crc32 = 0;  /* TODO: Calculate CRC */
    
    /* Update slot info */
    ctx->slots[slot_id].key_version = key_version;
    ctx->slots[slot_id].key_len = key_len;
    ctx->slots[slot_id].is_imported = true;
    ctx->slots[slot_id].created_time = 0;  /* TODO: Get current time */
    
    ctx->total_key_operations++;
    
    return KEYM_OK;
}

keym_status_t keym_key_export(keym_context_t *ctx, uint8_t slot_id,
                              uint8_t *key_data, uint32_t *key_len)
{
    keym_status_t status;
    keym_key_material_t *material;
    
    status = keym_validate_slot(ctx, slot_id);
    if (status != KEYM_OK) {
        return status;
    }
    
    if (key_data == NULL || key_len == NULL) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    /* Check if key is exportable */
    if (!ctx->slots[slot_id].is_exportable) {
        return KEYM_ERROR_PERMISSION_DENIED;
    }
    
    material = &ctx->materials[slot_id];
    if (material->key_data == NULL || material->key_data_len == 0) {
        return KEYM_ERROR_KEY_NOT_FOUND;
    }
    
    /* Check buffer size */
    if (*key_len < material->key_data_len) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    memcpy(key_data, material->key_data, material->key_data_len);
    *key_len = material->key_data_len;
    
    return KEYM_OK;
}

keym_status_t keym_key_generate(keym_context_t *ctx, uint8_t slot_id,
                                keym_key_type_t key_type)
{
    keym_status_t status;
    
    status = keym_validate_slot(ctx, slot_id);
    if (status != KEYM_OK) {
        return status;
    }
    
    /* Update slot type if different */
    if (ctx->slots[slot_id].key_type != key_type) {
        ctx->slots[slot_id].key_type = key_type;
        ctx->slots[slot_id].key_len = keym_get_key_type_size(key_type);
    }
    
    /* Allocate key material buffer */
    if (ctx->materials[slot_id].key_data == NULL) {
        ctx->materials[slot_id].key_data = malloc(KEYM_MAX_KEY_MATERIAL_SIZE);
        if (ctx->materials[slot_id].key_data == NULL) {
            return KEYM_ERROR_NO_MEMORY;
        }
    }
    
    /* Generate random key */
    if (ctx->cryif != NULL) {
        /* Use CryIf for hardware random generation */
        /* cryif_random_generate(ctx->cryif, ...); */
    }
    
    /* For now, fill with pseudo-random pattern */
    for (uint32_t i = 0; i < ctx->slots[slot_id].key_len; i++) {
        ctx->materials[slot_id].key_data[i] = (uint8_t)(i * 7 + slot_id * 13);
    }
    ctx->materials[slot_id].key_data_len = ctx->slots[slot_id].key_len;
    
    /* Update slot info */
    ctx->slots[slot_id].key_version = 1;
    ctx->slots[slot_id].key_generation++;
    ctx->slots[slot_id].created_time = 0;  /* TODO: Get current time */
    ctx->slots[slot_id].next_rotation_time = ctx->slots[slot_id].created_time +
                                              ctx->rotation_policy.rotation_interval_ms;
    
    ctx->total_key_operations++;
    
    return KEYM_OK;
}

keym_status_t keym_key_activate(keym_context_t *ctx, uint8_t slot_id)
{
    keym_status_t status;
    keym_key_state_t old_state;
    
    status = keym_validate_slot(ctx, slot_id);
    if (status != KEYM_OK) {
        return status;
    }
    
    if (ctx->slots[slot_id].state != KEYM_STATE_INACTIVE) {
        return KEYM_ERROR_KEY_INVALID;
    }
    
    old_state = ctx->slots[slot_id].state;
    ctx->slots[slot_id].state = KEYM_STATE_ACTIVE;
    ctx->slots[slot_id].activated_time = 0;  /* TODO: Get current time */
    
    /* Trigger state change callback */
    if (ctx->on_state_change != NULL) {
        ctx->on_state_change(slot_id, old_state, KEYM_STATE_ACTIVE, ctx->callback_user_data);
    }
    
    return KEYM_OK;
}

keym_status_t keym_key_revoke(keym_context_t *ctx, uint8_t slot_id)
{
    keym_status_t status;
    keym_key_state_t old_state;
    
    status = keym_validate_slot(ctx, slot_id);
    if (status != KEYM_OK) {
        return status;
    }
    
    old_state = ctx->slots[slot_id].state;
    ctx->slots[slot_id].state = KEYM_STATE_REVOKED;
    
    /* Trigger state change callback */
    if (ctx->on_state_change != NULL) {
        ctx->on_state_change(slot_id, old_state, KEYM_STATE_REVOKED, ctx->callback_user_data);
    }
    
    return KEYM_OK;
}

/* ============================================================================
 * Key Derivation (KDF)
 * ============================================================================ */

keym_status_t keym_key_derive(keym_context_t *ctx,
                              const keym_derivation_params_t *params,
                              uint8_t *derived_slot_id)
{
    keym_status_t status;
    uint8_t target_slot;
    
    if (ctx == NULL || !ctx->initialized || params == NULL || derived_slot_id == NULL) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    /* Validate parent slot */
    status = keym_validate_slot(ctx, params->parent_slot_id);
    if (status != KEYM_OK) {
        return status;
    }
    
    /* Allocate target slot if needed */
    if (params->target_slot_id == KEYM_SLOT_ID_INVALID) {
        target_slot = KEYM_SLOT_ID_INVALID;
        status = keym_slot_allocate(ctx, &target_slot, "derived_key",
                                    params->derived_key_type);
        if (status != KEYM_OK) {
            return status;
        }
        *derived_slot_id = target_slot;
    } else {
        target_slot = params->target_slot_id;
        status = keym_validate_slot(ctx, target_slot);
        if (status != KEYM_OK) {
            return status;
        }
        *derived_slot_id = target_slot;
    }
    
    /* Perform key derivation based on KDF type */
    switch (params->kdf_type) {
        case KEYM_KDF_HKDF_SHA256:
            status = keym_hkdf_derive(ctx, params->parent_slot_id, target_slot,
                                      NULL, 0,
                                      params->context, params->context_len,
                                      params->derived_key_len);
            break;
            
        case KEYM_KDF_NIST_SP800_108:
            /* NIST SP 800-108 KDF implementation */
            /* TODO: Implement */
            status = KEYM_OK;
            break;
            
        default:
            status = KEYM_ERROR_INVALID_PARAM;
            break;
    }
    
    if (status == KEYM_OK) {
        /* Update derived key info */
        ctx->slots[target_slot].parent_slot_id = params->parent_slot_id;
        ctx->slots[target_slot].kdf_type = params->kdf_type;
        ctx->total_derivations++;
    }
    
    return status;
}

keym_status_t keym_hkdf_derive(keym_context_t *ctx, uint8_t parent_slot,
                               uint8_t target_slot,
                               const uint8_t *salt, uint32_t salt_len,
                               const uint8_t *info, uint32_t info_len,
                               uint32_t key_len)
{
    /* Simplified HKDF implementation */
    /* In production, this would use proper HKDF-SHA256 from crypto library */
    
    (void)ctx;
    (void)parent_slot;
    (void)salt;
    (void)salt_len;
    (void)info;
    (void)info_len;
    
    if (key_len > KEYM_MAX_KEY_MATERIAL_SIZE) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    /* Allocate key material for derived key */
    if (ctx->materials[target_slot].key_data == NULL) {
        ctx->materials[target_slot].key_data = malloc(KEYM_MAX_KEY_MATERIAL_SIZE);
        if (ctx->materials[target_slot].key_data == NULL) {
            return KEYM_ERROR_NO_MEMORY;
        }
    }
    
    /* Fill with derived pattern (simplified - should use real HKDF) */
    for (uint32_t i = 0; i < key_len; i++) {
        ctx->materials[target_slot].key_data[i] = (uint8_t)(i * 3 + target_slot * 11 + 0x5A);
    }
    ctx->materials[target_slot].key_data_len = key_len;
    
    return KEYM_OK;
}

/* ============================================================================
 * Key Rotation
 * ============================================================================ */

keym_status_t keym_set_rotation_policy(keym_context_t *ctx,
                                       const keym_rotation_policy_t *policy)
{
    if (ctx == NULL || !ctx->initialized || policy == NULL) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    memcpy(&ctx->rotation_policy, policy, sizeof(keym_rotation_policy_t));
    return KEYM_OK;
}

keym_status_t keym_rotate_key(keym_context_t *ctx, uint8_t slot_id,
                              uint8_t *new_slot_id)
{
    keym_status_t status;
    uint8_t new_slot;
    
    status = keym_validate_slot(ctx, slot_id);
    if (status != KEYM_OK) {
        return status;
    }
    
    /* Mark current key for rotation */
    ctx->slots[slot_id].state = KEYM_STATE_PENDING_ROTATION;
    
    /* Allocate new slot if needed */
    if (new_slot_id != NULL && *new_slot_id == KEYM_SLOT_ID_INVALID) {
        new_slot = KEYM_SLOT_ID_INVALID;
        status = keym_slot_allocate(ctx, &new_slot,
                                    ctx->slots[slot_id].name,
                                    ctx->slots[slot_id].key_type);
        if (status != KEYM_OK) {
            ctx->slots[slot_id].state = KEYM_STATE_ACTIVE;
            return status;
        }
        *new_slot_id = new_slot;
    } else if (new_slot_id != NULL) {
        new_slot = *new_slot_id;
    } else {
        new_slot = slot_id;  /* In-place rotation */
    }
    
    /* Generate new key */
    status = keym_key_generate(ctx, new_slot, ctx->slots[slot_id].key_type);
    if (status != KEYM_OK) {
        if (new_slot_id != NULL && *new_slot_id != slot_id) {
            keym_slot_free(ctx, new_slot);
        }
        ctx->slots[slot_id].state = KEYM_STATE_ACTIVE;
        return status;
    }
    
    /* Activate new key */
    status = keym_key_activate(ctx, new_slot);
    if (status != KEYM_OK) {
        if (new_slot_id != NULL && *new_slot_id != slot_id) {
            keym_slot_free(ctx, new_slot);
        }
        ctx->slots[slot_id].state = KEYM_STATE_ACTIVE;
        return status;
    }
    
    /* Mark old key as rotated */
    ctx->slots[slot_id].state = KEYM_STATE_ROTATED;
    ctx->slots[slot_id].expired_time = 0;  /* TODO: Get current time */
    
    /* Update version history */
    keym_update_version_history(ctx, slot_id);
    
    ctx->total_rotations++;
    
    /* Trigger rotation callback */
    if (ctx->on_rotation != NULL) {
        ctx->on_rotation(new_slot, ctx->slots[new_slot].key_version, ctx->callback_user_data);
    }
    
    return KEYM_OK;
}

uint32_t keym_check_and_rotate(keym_context_t *ctx, uint64_t current_time)
{
    uint32_t rotations = 0;
    
    if (ctx == NULL || !ctx->initialized) {
        return 0;
    }
    
    if (!ctx->rotation_policy.auto_rotation) {
        return 0;
    }
    
    for (int i = 0; i < KEYM_MAX_KEY_SLOTS; i++) {
        if (ctx->slots[i].state == KEYM_STATE_ACTIVE) {
            /* Check if rotation is needed */
            if (ctx->slots[i].next_rotation_time <= current_time ||
                (current_time - ctx->slots[i].activated_time) >= ctx->rotation_policy.max_key_age_ms) {
                
                uint8_t new_slot = KEYM_SLOT_ID_INVALID;
                if (keym_rotate_key(ctx, i, &new_slot) == KEYM_OK) {
                    rotations++;
                }
            }
        }
    }
    
    return rotations;
}

keym_status_t keym_get_version_history(keym_context_t *ctx, uint8_t slot_id,
                                       keym_version_history_t *history,
                                       uint32_t max_entries,
                                       uint32_t *num_entries)
{
    keym_status_t status;
    
    status = keym_validate_slot(ctx, slot_id);
    if (status != KEYM_OK) {
        return status;
    }
    
    if (history == NULL || num_entries == NULL) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    uint32_t count = 0;
    for (int i = 0; i < KEYM_MAX_KEY_VERSIONS && count < max_entries; i++) {
        if (ctx->version_history[slot_id][i].version > 0) {
            memcpy(&history[count], &ctx->version_history[slot_id][i],
                   sizeof(keym_version_history_t));
            count++;
        }
    }
    
    *num_entries = count;
    return KEYM_OK;
}

/* ============================================================================
 * DDS Security Integration
 * ============================================================================ */

keym_status_t keym_integrate_dds_security(keym_context_t *ctx, void *dds_sec)
{
    if (ctx == NULL || !ctx->initialized) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    ctx->dds_sec = dds_sec;
    return KEYM_OK;
}

keym_status_t keym_import_from_dds_cert(keym_context_t *ctx, const char *cert_name,
                                        uint8_t *key_slot)
{
    (void)ctx;
    (void)cert_name;
    (void)key_slot;
    /* TODO: Implement DDS certificate key import */
    return KEYM_OK;
}

keym_status_t keym_export_to_dds_cert(keym_context_t *ctx, uint8_t key_slot,
                                      const char *cert_name)
{
    (void)ctx;
    (void)key_slot;
    (void)cert_name;
    /* TODO: Implement key export to DDS certificate */
    return KEYM_OK;
}

keym_status_t keym_register_certificate(keym_context_t *ctx, uint8_t cert_id,
                                        const char *name,
                                        const uint8_t *cert_data, uint32_t cert_len)
{
    if (ctx == NULL || !ctx->initialized || cert_data == NULL || cert_len == 0) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    if (cert_id >= KEYM_MAX_CERTIFICATES) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    /* Free existing certificate data */
    if (ctx->certificates[cert_id].cert_data != NULL) {
        free(ctx->certificates[cert_id].cert_data);
    }
    
    /* Allocate and copy new certificate */
    ctx->certificates[cert_id].cert_data = malloc(cert_len);
    if (ctx->certificates[cert_id].cert_data == NULL) {
        return KEYM_ERROR_NO_MEMORY;
    }
    
    memcpy(ctx->certificates[cert_id].cert_data, cert_data, cert_len);
    ctx->certificates[cert_id].cert_data_len = cert_len;
    ctx->certificates[cert_id].is_revoked = false;
    
    if (name != NULL) {
        strncpy(ctx->certificates[cert_id].name, name, KEYM_MAX_KEY_NAME_LEN - 1);
        ctx->certificates[cert_id].name[KEYM_MAX_KEY_NAME_LEN - 1] = '\0';
    }
    
    return KEYM_OK;
}

keym_status_t keym_update_certificate(keym_context_t *ctx, uint8_t cert_id,
                                      const uint8_t *cert_data, uint32_t cert_len)
{
    /* Update is same as register */
    return keym_register_certificate(ctx, cert_id, NULL, cert_data, cert_len);
}

keym_status_t keym_revoke_certificate(keym_context_t *ctx, uint8_t cert_id)
{
    if (ctx == NULL || !ctx->initialized || cert_id >= KEYM_MAX_CERTIFICATES) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    ctx->certificates[cert_id].is_revoked = true;
    return KEYM_OK;
}

/* ============================================================================
 * SecOC Integration
 * ============================================================================ */

keym_status_t keym_configure_secoc_key(keym_context_t *ctx, uint32_t secoc_pdu_id,
                                       uint8_t *key_slot)
{
    char slot_name[KEYM_MAX_KEY_NAME_LEN];
    keym_status_t status;
    uint8_t slot;
    
    if (ctx == NULL || !ctx->initialized || key_slot == NULL) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    /* Create slot name based on PDU ID */
    snprintf(slot_name, sizeof(slot_name), "SecOC_PDU_%lu", (unsigned long)secoc_pdu_id);
    
    /* Check if already exists */
    slot = keym_slot_find_by_name(ctx, slot_name);
    if (slot != KEYM_SLOT_ID_INVALID) {
        *key_slot = slot;
        return KEYM_OK;
    }
    
    /* Allocate new slot */
    slot = KEYM_SLOT_ID_INVALID;
    status = keym_slot_allocate(ctx, &slot, slot_name, KEYM_TYPE_AES_128);
    if (status != KEYM_OK) {
        return status;
    }
    
    /* Set SecOC usage */
    keym_slot_set_attributes(ctx, slot, KEYM_USAGE_SECOC, true, false);
    
    *key_slot = slot;
    return KEYM_OK;
}

uint8_t keym_get_secoc_key_slot(keym_context_t *ctx, uint32_t pdu_id)
{
    char slot_name[KEYM_MAX_KEY_NAME_LEN];
    
    if (ctx == NULL || !ctx->initialized) {
        return KEYM_SLOT_ID_INVALID;
    }
    
    snprintf(slot_name, sizeof(slot_name), "SecOC_PDU_%lu", (unsigned long)pdu_id);
    return keym_slot_find_by_name(ctx, slot_name);
}

/* ============================================================================
 * Callback Management
 * ============================================================================ */

keym_status_t keym_register_rotation_callback(keym_context_t *ctx,
                                               keym_rotation_callback_t callback,
                                               void *user_data)
{
    if (ctx == NULL || !ctx->initialized) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    ctx->on_rotation = callback;
    ctx->callback_user_data = user_data;
    
    return KEYM_OK;
}

keym_status_t keym_register_state_callback(keym_context_t *ctx,
                                            keym_state_callback_t callback,
                                            void *user_data)
{
    if (ctx == NULL || !ctx->initialized) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    ctx->on_state_change = callback;
    ctx->callback_user_data = user_data;
    
    return KEYM_OK;
}

/* ============================================================================
 * Secure Storage
 * ============================================================================ */

keym_status_t keym_load_persistent_keys(keym_context_t *ctx)
{
    (void)ctx;
    /* TODO: Implement persistent key loading from secure storage */
    return KEYM_OK;
}

keym_status_t keym_save_persistent_keys(keym_context_t *ctx)
{
    (void)ctx;
    /* TODO: Implement persistent key saving to secure storage */
    return KEYM_OK;
}

/* ============================================================================
 * Debug and Diagnostics
 * ============================================================================ */

const char* keym_get_key_type_name(keym_key_type_t type)
{
    return keym_get_key_type_name_internal(type);
}

const char* keym_get_key_state_name(keym_key_state_t state)
{
    return keym_get_key_state_name_internal(state);
}

const char* keym_get_version(void)
{
    return KEYM_VERSION;
}

void keym_debug_print_slot(keym_context_t *ctx, uint8_t slot_id)
{
    keym_slot_info_t *slot;
    
    if (ctx == NULL || slot_id >= KEYM_MAX_KEY_SLOTS) {
        return;
    }
    
    slot = &ctx->slots[slot_id];
    
    printf("Key Slot %d:\n", slot_id);
    printf("  Name: %s\n", slot->name);
    printf("  Type: %s\n", keym_get_key_type_name_internal(slot->key_type));
    printf("  State: %s\n", keym_get_key_state_name_internal(slot->state));
    printf("  Version: %u\n", slot->key_version);
    printf("  Key Length: %u\n", slot->key_len);
    printf("  Persistent: %s\n", slot->is_persistent ? "Yes" : "No");
    printf("  Exportable: %s\n", slot->is_exportable ? "Yes" : "No");
    printf("  Usage: 0x%02X\n", slot->usage_flags);
}

/* ============================================================================
 * Internal Functions
 * ============================================================================ */

static keym_status_t keym_validate_slot(keym_context_t *ctx, uint8_t slot_id)
{
    if (ctx == NULL || !ctx->initialized) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    if (slot_id >= KEYM_MAX_KEY_SLOTS) {
        return KEYM_ERROR_INVALID_PARAM;
    }
    
    if (ctx->slots[slot_id].state == KEYM_STATE_EMPTY) {
        return KEYM_ERROR_SLOT_NOT_FOUND;
    }
    
    return KEYM_OK;
}

static void keym_update_version_history(keym_context_t *ctx, uint8_t slot_id)
{
    /* Shift history entries */
    for (int i = KEYM_MAX_KEY_VERSIONS - 1; i > 0; i--) {
        memcpy(&ctx->version_history[slot_id][i],
               &ctx->version_history[slot_id][i - 1],
               sizeof(keym_version_history_t));
    }
    
    /* Add current version to history */
    ctx->version_history[slot_id][0].version = ctx->slots[slot_id].key_version;
    ctx->version_history[slot_id][0].created_time = ctx->slots[slot_id].created_time;
    ctx->version_history[slot_id][0].expired_time = ctx->slots[slot_id].expired_time;
    ctx->version_history[slot_id][0].state = ctx->slots[slot_id].state;
}

static const char* keym_get_key_type_name_internal(keym_key_type_t type)
{
    switch (type) {
        case KEYM_TYPE_AES_128:         return "AES-128";
        case KEYM_TYPE_AES_192:         return "AES-192";
        case KEYM_TYPE_AES_256:         return "AES-256";
        case KEYM_TYPE_HMAC_SHA1:       return "HMAC-SHA1";
        case KEYM_TYPE_HMAC_SHA256:     return "HMAC-SHA256";
        case KEYM_TYPE_HMAC_SHA384:     return "HMAC-SHA384";
        case KEYM_TYPE_HMAC_SHA512:     return "HMAC-SHA512";
        case KEYM_TYPE_RSA_1024:        return "RSA-1024";
        case KEYM_TYPE_RSA_2048:        return "RSA-2048";
        case KEYM_TYPE_RSA_4096:        return "RSA-4096";
        case KEYM_TYPE_ECC_P192:        return "ECC-P192";
        case KEYM_TYPE_ECC_P224:        return "ECC-P224";
        case KEYM_TYPE_ECC_P256:        return "ECC-P256";
        case KEYM_TYPE_ECC_P384:        return "ECC-P384";
        case KEYM_TYPE_ECC_P521:        return "ECC-P521";
        case KEYM_TYPE_DERIVED:         return "DERIVED";
        case KEYM_TYPE_KEY_MATERIAL:    return "KEY_MATERIAL";
        default:                        return "UNKNOWN";
    }
}

static const char* keym_get_key_state_name_internal(keym_key_state_t state)
{
    switch (state) {
        case KEYM_STATE_EMPTY:          return "EMPTY";
        case KEYM_STATE_ACTIVE:         return "ACTIVE";
        case KEYM_STATE_INACTIVE:       return "INACTIVE";
        case KEYM_STATE_EXPIRED:        return "EXPIRED";
        case KEYM_STATE_REVOKED:        return "REVOKED";
        case KEYM_STATE_PENDING_ROTATION: return "PENDING_ROTATION";
        case KEYM_STATE_ROTATED:        return "ROTATED";
        default:                        return "UNKNOWN";
    }
}

static uint32_t keym_get_key_type_size(keym_key_type_t type)
{
    switch (type) {
        case KEYM_TYPE_AES_128:         return 16;
        case KEYM_TYPE_AES_192:         return 24;
        case KEYM_TYPE_AES_256:         return 32;
        case KEYM_TYPE_HMAC_SHA1:       return 20;
        case KEYM_TYPE_HMAC_SHA256:     return 32;
        case KEYM_TYPE_HMAC_SHA384:     return 48;
        case KEYM_TYPE_HMAC_SHA512:     return 64;
        case KEYM_TYPE_RSA_1024:        return 128;
        case KEYM_TYPE_RSA_2048:        return 256;
        case KEYM_TYPE_RSA_4096:        return 512;
        case KEYM_TYPE_ECC_P192:        return 24;
        case KEYM_TYPE_ECC_P224:        return 28;
        case KEYM_TYPE_ECC_P256:        return 32;
        case KEYM_TYPE_ECC_P384:        return 48;
        case KEYM_TYPE_ECC_P521:        return 66;
        default:                        return 32;
    }
}

static bool keym_is_key_usage_allowed(keym_slot_info_t *slot, keym_key_usage_t usage)
{
    if (slot == NULL) return false;
    return (slot->usage_flags & usage) != 0;
}
