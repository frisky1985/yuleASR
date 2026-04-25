/**
 * @file cryif_core.c
 * @brief CryIf (Crypto Interface) Core Implementation
 * @version 1.0
 * @date 2026-04-25
 *
 * Implementation of AUTOSAR CryIf 4.4 specification
 * HSM/TPM/TEE hardware abstraction layer
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cryif_core.h"

#define CRYIF_VERSION "4.4.0-AUTOSAR"

/* ============================================================================
 * Internal Function Declarations
 * ============================================================================ */

static cryif_driver_t* cryif_select_driver(cryif_context_t *ctx,
                                           cryif_operation_t op,
                                           uint32_t algorithm);
static cryif_key_type_t cryif_map_algorithm_to_key_type(uint32_t algorithm);

/* ============================================================================
 * Initialization/Deinitialization
 * ============================================================================ */

cryif_context_t* cryif_init(void)
{
    cryif_context_t *ctx;
    
    ctx = (cryif_context_t*)calloc(1, sizeof(cryif_context_t));
    if (ctx == NULL) {
        return NULL;
    }
    
    /* Initialize driver table */
    for (int i = 0; i < CRYIF_MAX_DRIVERS; i++) {
        ctx->drivers[i] = NULL;
    }
    ctx->num_drivers = 0;
    
    /* Initialize channels */
    for (int i = 0; i < CRYIF_MAX_CHANNELS; i++) {
        ctx->channels[i].channel_id = CRYIF_CHANNEL_ID_INVALID;
        ctx->channels[i].driver = NULL;
    }
    ctx->num_channels = 0;
    
    /* Initialize key slots */
    for (int i = 0; i < CRYIF_MAX_KEY_SLOTS; i++) {
        ctx->key_slots[i].slot_id = i;
        ctx->key_slots[i].driver = NULL;
        ctx->key_slots[i].driver_slot_id = CRYIF_KEY_SLOT_INVALID;
        ctx->key_slots[i].state = CRYIF_KEY_STATE_EMPTY;
    }
    
    /* Initialize statistics */
    ctx->stats.total_operations = 0;
    ctx->stats.hw_operations = 0;
    ctx->stats.sw_fallback_operations = 0;
    ctx->stats.failed_operations = 0;
    ctx->stats.key_operations = 0;
    
    /* Set default driver selection function */
    ctx->select_driver = cryif_select_driver;
    ctx->default_driver = NULL;
    
    ctx->initialized = true;
    
    return ctx;
}

void cryif_deinit(cryif_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return;
    }
    
    /* Cleanup drivers */
    for (uint32_t i = 0; i < ctx->num_drivers; i++) {
        if (ctx->drivers[i] != NULL && ctx->drivers[i]->deinit != NULL) {
            ctx->drivers[i]->deinit(ctx->drivers[i]);
        }
    }
    
    ctx->initialized = false;
    free(ctx);
}

/* ============================================================================
 * Driver Management
 * ============================================================================ */

cryif_status_t cryif_register_driver(cryif_context_t *ctx, cryif_driver_t *driver)
{
    if (ctx == NULL || !ctx->initialized || driver == NULL) {
        return CRYIF_ERROR_INVALID_PARAM;
    }
    
    if (ctx->num_drivers >= CRYIF_MAX_DRIVERS) {
        return CRYIF_ERROR_NO_MEMORY;
    }
    
    /* Initialize driver */
    if (driver->init != NULL) {
        cryif_status_t status = driver->init(driver);
        if (status != CRYIF_OK) {
            return status;
        }
    }
    
    ctx->drivers[ctx->num_drivers++] = driver;
    
    /* Set as default if first driver */
    if (ctx->num_drivers == 1) {
        ctx->default_driver = driver;
    }
    
    return CRYIF_OK;
}

cryif_status_t cryif_unregister_driver(cryif_context_t *ctx, cryif_driver_t *driver)
{
    if (ctx == NULL || !ctx->initialized || driver == NULL) {
        return CRYIF_ERROR_INVALID_PARAM;
    }
    
    /* Find and remove driver */
    for (uint32_t i = 0; i < ctx->num_drivers; i++) {
        if (ctx->drivers[i] == driver) {
            /* Deinit driver */
            if (driver->deinit != NULL) {
                driver->deinit(driver);
            }
            
            /* Shift remaining drivers */
            for (uint32_t j = i; j < ctx->num_drivers - 1; j++) {
                ctx->drivers[j] = ctx->drivers[j + 1];
            }
            ctx->num_drivers--;
            
            /* Update default driver if needed */
            if (ctx->default_driver == driver) {
                ctx->default_driver = (ctx->num_drivers > 0) ? ctx->drivers[0] : NULL;
            }
            
            return CRYIF_OK;
        }
    }
    
    return CRYIF_ERROR_INVALID_PARAM;
}

cryif_driver_t* cryif_find_driver_for_algo(cryif_context_t *ctx, uint32_t algorithm)
{
    if (ctx == NULL || !ctx->initialized) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < ctx->num_drivers; i++) {
        if (ctx->drivers[i] != NULL && ctx->drivers[i]->supports_algorithm != NULL) {
            if (ctx->drivers[i]->supports_algorithm(ctx->drivers[i], algorithm)) {
                return ctx->drivers[i];
            }
        }
    }
    
    return NULL;
}

cryif_status_t cryif_set_default_driver(cryif_context_t *ctx, cryif_driver_t *driver)
{
    if (ctx == NULL || !ctx->initialized || driver == NULL) {
        return CRYIF_ERROR_INVALID_PARAM;
    }
    
    ctx->default_driver = driver;
    return CRYIF_OK;
}

/* ============================================================================
 * Channel Management
 * ============================================================================ */

cryif_status_t cryif_channel_configure(cryif_context_t *ctx, uint8_t channel_id,
                                       cryif_driver_t *driver, uint32_t priority)
{
    if (ctx == NULL || !ctx->initialized || channel_id >= CRYIF_MAX_CHANNELS) {
        return CRYIF_ERROR_INVALID_PARAM;
    }
    
    ctx->channels[channel_id].channel_id = channel_id;
    ctx->channels[channel_id].driver = driver;
    ctx->channels[channel_id].priority = priority;
    
    return CRYIF_OK;
}

cryif_status_t cryif_channel_get_info(cryif_context_t *ctx, uint8_t channel_id,
                                      cryif_channel_t *channel)
{
    if (ctx == NULL || !ctx->initialized || channel == NULL ||
        channel_id >= CRYIF_MAX_CHANNELS) {
        return CRYIF_ERROR_INVALID_PARAM;
    }
    
    memcpy(channel, &ctx->channels[channel_id], sizeof(cryif_channel_t));
    return CRYIF_OK;
}

/* ============================================================================
 * Key Slot Management
 * ============================================================================ */

cryif_status_t cryif_key_slot_allocate(cryif_context_t *ctx, uint8_t *slot_id,
                                       cryif_driver_t *driver)
{
    if (ctx == NULL || !ctx->initialized || slot_id == NULL) {
        return CRYIF_ERROR_INVALID_PARAM;
    }
    
    /* If specific slot requested, check availability */
    if (*slot_id != CRYIF_KEY_SLOT_INVALID) {
        if (*slot_id >= CRYIF_MAX_KEY_SLOTS) {
            return CRYIF_ERROR_INVALID_PARAM;
        }
        if (ctx->key_slots[*slot_id].state != CRYIF_KEY_STATE_EMPTY) {
            return CRYIF_ERROR_KEY_SLOT_OCCUPIED;
        }
    } else {
        /* Find free slot */
        for (int i = 0; i < CRYIF_MAX_KEY_SLOTS; i++) {
            if (ctx->key_slots[i].state == CRYIF_KEY_STATE_EMPTY) {
                *slot_id = i;
                break;
            }
        }
        if (*slot_id == CRYIF_KEY_SLOT_INVALID) {
            return CRYIF_ERROR_NO_MEMORY;
        }
    }
    
    ctx->key_slots[*slot_id].state = CRYIF_KEY_STATE_VALID;
    ctx->key_slots[*slot_id].driver = (driver != NULL) ? driver : ctx->default_driver;
    ctx->key_slots[*slot_id].driver_slot_id = *slot_id;
    
    ctx->stats.key_operations++;
    
    return CRYIF_OK;
}

cryif_status_t cryif_key_slot_free(cryif_context_t *ctx, uint8_t slot_id)
{
    if (ctx == NULL || !ctx->initialized || slot_id >= CRYIF_MAX_KEY_SLOTS) {
        return CRYIF_ERROR_INVALID_PARAM;
    }
    
    if (ctx->key_slots[slot_id].state == CRYIF_KEY_STATE_EMPTY) {
        return CRYIF_ERROR_KEY_SLOT_NOT_FOUND;
    }
    
    /* Erase key in hardware first */
    if (ctx->key_slots[slot_id].driver != NULL &&
        ctx->key_slots[slot_id].driver->key_erase != NULL) {
        ctx->key_slots[slot_id].driver->key_erase(
            ctx->key_slots[slot_id].driver, 
            ctx->key_slots[slot_id].driver_slot_id);
    }
    
    ctx->key_slots[slot_id].state = CRYIF_KEY_STATE_EMPTY;
    ctx->key_slots[slot_id].driver = NULL;
    ctx->key_slots[slot_id].driver_slot_id = CRYIF_KEY_SLOT_INVALID;
    
    return CRYIF_OK;
}

cryif_status_t cryif_key_import(cryif_context_t *ctx, uint8_t slot_id,
                                cryif_key_type_t key_type,
                                const uint8_t *key, uint32_t key_len)
{
    cryif_driver_t *driver;
    
    if (ctx == NULL || !ctx->initialized || slot_id >= CRYIF_MAX_KEY_SLOTS ||
        key == NULL || key_len == 0) {
        return CRYIF_ERROR_INVALID_PARAM;
    }
    
    if (ctx->key_slots[slot_id].state == CRYIF_KEY_STATE_EMPTY) {
        return CRYIF_ERROR_KEY_SLOT_NOT_FOUND;
    }
    
    driver = ctx->key_slots[slot_id].driver;
    if (driver == NULL || driver->key_import == NULL) {
        return CRYIF_ERROR_OPERATION_NOT_SUPPORTED;
    }
    
    ctx->stats.key_operations++;
    return driver->key_import(driver, ctx->key_slots[slot_id].driver_slot_id,
                              key_type, key, key_len);
}

cryif_status_t cryif_key_export(cryif_context_t *ctx, uint8_t slot_id,
                                uint8_t *key, uint32_t *key_len)
{
    cryif_driver_t *driver;
    
    if (ctx == NULL || !ctx->initialized || slot_id >= CRYIF_MAX_KEY_SLOTS ||
        key == NULL || key_len == NULL) {
        return CRYIF_ERROR_INVALID_PARAM;
    }
    
    if (ctx->key_slots[slot_id].state == CRYIF_KEY_STATE_EMPTY) {
        return CRYIF_ERROR_KEY_SLOT_NOT_FOUND;
    }
    
    driver = ctx->key_slots[slot_id].driver;
    if (driver == NULL || driver->key_export == NULL) {
        return CRYIF_ERROR_OPERATION_NOT_SUPPORTED;
    }
    
    ctx->stats.key_operations++;
    return driver->key_export(driver, ctx->key_slots[slot_id].driver_slot_id,
                              key, key_len);
}

cryif_status_t cryif_key_generate(cryif_context_t *ctx, uint8_t slot_id,
                                  cryif_key_type_t key_type, uint32_t key_len)
{
    cryif_driver_t *driver;
    
    if (ctx == NULL || !ctx->initialized || slot_id >= CRYIF_MAX_KEY_SLOTS) {
        return CRYIF_ERROR_INVALID_PARAM;
    }
    
    if (ctx->key_slots[slot_id].state == CRYIF_KEY_STATE_EMPTY) {
        return CRYIF_ERROR_KEY_SLOT_NOT_FOUND;
    }
    
    driver = ctx->key_slots[slot_id].driver;
    if (driver == NULL || driver->key_generate == NULL) {
        return CRYIF_ERROR_OPERATION_NOT_SUPPORTED;
    }
    
    ctx->stats.key_operations++;
    return driver->key_generate(driver, ctx->key_slots[slot_id].driver_slot_id,
                                key_type, key_len);
}

cryif_status_t cryif_key_erase(cryif_context_t *ctx, uint8_t slot_id)
{
    return cryif_key_slot_free(ctx, slot_id);
}

cryif_status_t cryif_key_get_info(cryif_context_t *ctx, uint8_t slot_id,
                                  cryif_key_slot_info_t *info)
{
    cryif_driver_t *driver;
    
    if (ctx == NULL || !ctx->initialized || slot_id >= CRYIF_MAX_KEY_SLOTS ||
        info == NULL) {
        return CRYIF_ERROR_INVALID_PARAM;
    }
    
    if (ctx->key_slots[slot_id].state == CRYIF_KEY_STATE_EMPTY) {
        return CRYIF_ERROR_KEY_SLOT_NOT_FOUND;
    }
    
    driver = ctx->key_slots[slot_id].driver;
    if (driver != NULL && driver->key_get_info != NULL) {
        return driver->key_get_info(driver, ctx->key_slots[slot_id].driver_slot_id, info);
    }
    
    /* Return basic info */
    info->slot_id = slot_id;
    info->state = ctx->key_slots[slot_id].state;
    
    return CRYIF_OK;
}

cryif_status_t cryif_key_copy(cryif_context_t *ctx, uint8_t source_slot,
                              uint8_t target_slot)
{
    cryif_driver_t *driver;
    
    if (ctx == NULL || !ctx->initialized ||
        source_slot >= CRYIF_MAX_KEY_SLOTS || target_slot >= CRYIF_MAX_KEY_SLOTS) {
        return CRYIF_ERROR_INVALID_PARAM;
    }
    
    if (ctx->key_slots[source_slot].state == CRYIF_KEY_STATE_EMPTY ||
        ctx->key_slots[target_slot].state == CRYIF_KEY_STATE_EMPTY) {
        return CRYIF_ERROR_KEY_SLOT_NOT_FOUND;
    }
    
    driver = ctx->key_slots[source_slot].driver;
    if (driver == NULL || driver->key_copy == NULL) {
        return CRYIF_ERROR_OPERATION_NOT_SUPPORTED;
    }
    
    ctx->stats.key_operations++;
    return driver->key_copy(driver, 
                            ctx->key_slots[source_slot].driver_slot_id,
                            ctx->key_slots[target_slot].driver_slot_id);
}

/* ============================================================================
 * Synchronous Crypto Operations
 * ============================================================================ */

cryif_status_t cryif_encrypt(cryif_context_t *ctx, uint8_t key_slot,
                             uint32_t algorithm,
                             const uint8_t *plaintext, uint32_t pt_len,
                             const uint8_t *iv, uint32_t iv_len,
                             const uint8_t *aad, uint32_t aad_len,
                             uint8_t *ciphertext, uint32_t *ct_len,
                             uint8_t *tag, uint32_t *tag_len)
{
    cryif_driver_t *driver;
    
    if (ctx == NULL || !ctx->initialized || key_slot >= CRYIF_MAX_KEY_SLOTS) {
        return CRYIF_ERROR_INVALID_PARAM;
    }
    
    if (ctx->key_slots[key_slot].state == CRYIF_KEY_STATE_EMPTY) {
        return CRYIF_ERROR_KEY_SLOT_NOT_FOUND;
    }
    
    driver = ctx->key_slots[key_slot].driver;
    if (driver == NULL || driver->encrypt == NULL) {
        return CRYIF_ERROR_OPERATION_NOT_SUPPORTED;
    }
    
    ctx->stats.total_operations++;
    ctx->stats.hw_operations++;
    
    return driver->encrypt(driver, ctx->key_slots[key_slot].driver_slot_id,
                           plaintext, pt_len, iv, iv_len, aad, aad_len,
                           ciphertext, ct_len, tag, tag_len);
}

cryif_status_t cryif_decrypt(cryif_context_t *ctx, uint8_t key_slot,
                             uint32_t algorithm,
                             const uint8_t *ciphertext, uint32_t ct_len,
                             const uint8_t *iv, uint32_t iv_len,
                             const uint8_t *aad, uint32_t aad_len,
                             const uint8_t *tag, uint32_t tag_len,
                             uint8_t *plaintext, uint32_t *pt_len)
{
    cryif_driver_t *driver;
    
    if (ctx == NULL || !ctx->initialized || key_slot >= CRYIF_MAX_KEY_SLOTS) {
        return CRYIF_ERROR_INVALID_PARAM;
    }
    
    if (ctx->key_slots[key_slot].state == CRYIF_KEY_STATE_EMPTY) {
        return CRYIF_ERROR_KEY_SLOT_NOT_FOUND;
    }
    
    driver = ctx->key_slots[key_slot].driver;
    if (driver == NULL || driver->decrypt == NULL) {
        return CRYIF_ERROR_OPERATION_NOT_SUPPORTED;
    }
    
    ctx->stats.total_operations++;
    ctx->stats.hw_operations++;
    
    return driver->decrypt(driver, ctx->key_slots[key_slot].driver_slot_id,
                           ciphertext, ct_len, iv, iv_len, aad, aad_len,
                           tag, tag_len, plaintext, pt_len);
}

cryif_status_t cryif_mac_generate(cryif_context_t *ctx, uint8_t key_slot,
                                  uint32_t algorithm,
                                  const uint8_t *data, uint32_t data_len,
                                  uint8_t *mac, uint32_t *mac_len)
{
    cryif_driver_t *driver;
    
    if (ctx == NULL || !ctx->initialized || key_slot >= CRYIF_MAX_KEY_SLOTS) {
        return CRYIF_ERROR_INVALID_PARAM;
    }
    
    if (ctx->key_slots[key_slot].state == CRYIF_KEY_STATE_EMPTY) {
        return CRYIF_ERROR_KEY_SLOT_NOT_FOUND;
    }
    
    driver = ctx->key_slots[key_slot].driver;
    if (driver == NULL || driver->mac_generate == NULL) {
        return CRYIF_ERROR_OPERATION_NOT_SUPPORTED;
    }
    
    ctx->stats.total_operations++;
    ctx->stats.hw_operations++;
    
    return driver->mac_generate(driver, ctx->key_slots[key_slot].driver_slot_id,
                                data, data_len, NULL, 0, mac, mac_len);
}

cryif_status_t cryif_mac_verify(cryif_context_t *ctx, uint8_t key_slot,
                                uint32_t algorithm,
                                const uint8_t *data, uint32_t data_len,
                                const uint8_t *mac, uint32_t mac_len,
                                bool *verify_result)
{
    cryif_driver_t *driver;
    
    if (ctx == NULL || !ctx->initialized || key_slot >= CRYIF_MAX_KEY_SLOTS ||
        verify_result == NULL) {
        return CRYIF_ERROR_INVALID_PARAM;
    }
    
    if (ctx->key_slots[key_slot].state == CRYIF_KEY_STATE_EMPTY) {
        return CRYIF_ERROR_KEY_SLOT_NOT_FOUND;
    }
    
    driver = ctx->key_slots[key_slot].driver;
    if (driver == NULL || driver->mac_verify == NULL) {
        return CRYIF_ERROR_OPERATION_NOT_SUPPORTED;
    }
    
    ctx->stats.total_operations++;
    ctx->stats.hw_operations++;
    
    return driver->mac_verify(driver, ctx->key_slots[key_slot].driver_slot_id,
                              data, data_len, NULL, 0, mac, mac_len,
                              verify_result);
}

cryif_status_t cryif_sign(cryif_context_t *ctx, uint8_t key_slot,
                          uint32_t algorithm,
                          const uint8_t *data, uint32_t data_len,
                          uint8_t *signature, uint32_t *sig_len)
{
    cryif_driver_t *driver;
    
    if (ctx == NULL || !ctx->initialized || key_slot >= CRYIF_MAX_KEY_SLOTS) {
        return CRYIF_ERROR_INVALID_PARAM;
    }
    
    if (ctx->key_slots[key_slot].state == CRYIF_KEY_STATE_EMPTY) {
        return CRYIF_ERROR_KEY_SLOT_NOT_FOUND;
    }
    
    driver = ctx->key_slots[key_slot].driver;
    if (driver == NULL || driver->sign == NULL) {
        return CRYIF_ERROR_OPERATION_NOT_SUPPORTED;
    }
    
    ctx->stats.total_operations++;
    ctx->stats.hw_operations++;
    
    return driver->sign(driver, ctx->key_slots[key_slot].driver_slot_id,
                        data, data_len, signature, sig_len);
}

cryif_status_t cryif_verify(cryif_context_t *ctx, uint8_t key_slot,
                            uint32_t algorithm,
                            const uint8_t *data, uint32_t data_len,
                            const uint8_t *signature, uint32_t sig_len,
                            bool *verify_result)
{
    cryif_driver_t *driver;
    
    if (ctx == NULL || !ctx->initialized || key_slot >= CRYIF_MAX_KEY_SLOTS ||
        verify_result == NULL) {
        return CRYIF_ERROR_INVALID_PARAM;
    }
    
    if (ctx->key_slots[key_slot].state == CRYIF_KEY_STATE_EMPTY) {
        return CRYIF_ERROR_KEY_SLOT_NOT_FOUND;
    }
    
    driver = ctx->key_slots[key_slot].driver;
    if (driver == NULL || driver->verify == NULL) {
        return CRYIF_ERROR_OPERATION_NOT_SUPPORTED;
    }
    
    ctx->stats.total_operations++;
    ctx->stats.hw_operations++;
    
    return driver->verify(driver, ctx->key_slots[key_slot].driver_slot_id,
                          data, data_len, signature, sig_len, verify_result);
}

cryif_status_t cryif_hash(cryif_context_t *ctx, uint32_t algorithm,
                          const uint8_t *data, uint32_t data_len,
                          uint8_t *hash, uint32_t *hash_len)
{
    cryif_driver_t *driver;
    
    if (ctx == NULL || !ctx->initialized) {
        return CRYIF_ERROR_INVALID_PARAM;
    }
    
    /* Try to find driver that supports this hash algorithm */
    driver = ctx->default_driver;
    for (uint32_t i = 0; i < ctx->num_drivers; i++) {
        if (ctx->drivers[i] != NULL && ctx->drivers[i]->hash != NULL) {
            if (ctx->drivers[i]->supports_algorithm == NULL ||
                ctx->drivers[i]->supports_algorithm(ctx->drivers[i], algorithm)) {
                driver = ctx->drivers[i];
                break;
            }
        }
    }
    
    if (driver == NULL || driver->hash == NULL) {
        return CRYIF_ERROR_OPERATION_NOT_SUPPORTED;
    }
    
    ctx->stats.total_operations++;
    ctx->stats.hw_operations++;
    
    return driver->hash(driver, algorithm, data, data_len, hash, hash_len);
}

cryif_status_t cryif_random_generate(cryif_context_t *ctx,
                                     uint8_t *random_data, uint32_t random_len)
{
    cryif_driver_t *driver;
    
    if (ctx == NULL || !ctx->initialized || random_data == NULL || random_len == 0) {
        return CRYIF_ERROR_INVALID_PARAM;
    }
    
    driver = ctx->default_driver;
    if (driver == NULL || driver->random_generate == NULL) {
        return CRYIF_ERROR_OPERATION_NOT_SUPPORTED;
    }
    
    ctx->stats.total_operations++;
    ctx->stats.hw_operations++;
    
    return driver->random_generate(driver, random_data, random_len);
}

/* ============================================================================
 * Key Derivation
 * ============================================================================ */

cryif_status_t cryif_key_derive(cryif_context_t *ctx, uint8_t parent_slot,
                                uint8_t target_slot, uint32_t kdf_type,
                                const uint8_t *context, uint32_t context_len,
                                const uint8_t *label, uint32_t label_len)
{
    cryif_driver_t *driver;
    
    if (ctx == NULL || !ctx->initialized ||
        parent_slot >= CRYIF_MAX_KEY_SLOTS || target_slot >= CRYIF_MAX_KEY_SLOTS) {
        return CRYIF_ERROR_INVALID_PARAM;
    }
    
    if (ctx->key_slots[parent_slot].state == CRYIF_KEY_STATE_EMPTY ||
        ctx->key_slots[target_slot].state == CRYIF_KEY_STATE_EMPTY) {
        return CRYIF_ERROR_KEY_SLOT_NOT_FOUND;
    }
    
    driver = ctx->key_slots[parent_slot].driver;
    if (driver == NULL || driver->key_derive == NULL) {
        return CRYIF_ERROR_OPERATION_NOT_SUPPORTED;
    }
    
    ctx->stats.key_operations++;
    
    return driver->key_derive(driver,
                              ctx->key_slots[parent_slot].driver_slot_id,
                              ctx->key_slots[target_slot].driver_slot_id,
                              context, context_len, label, label_len);
}

/* ============================================================================
 * Software Fallback
 * ============================================================================ */

cryif_status_t cryif_enable_software_fallback(cryif_context_t *ctx, bool enable)
{
    (void)ctx;
    (void)enable;
    /* Software fallback implementation would go here */
    return CRYIF_OK;
}

bool cryif_is_hw_available(cryif_context_t *ctx)
{
    if (ctx == NULL || !ctx->initialized) {
        return false;
    }
    
    return (ctx->num_drivers > 0 && ctx->default_driver != NULL);
}

/* ============================================================================
 * Debug and Diagnostics
 * ============================================================================ */

const char* cryif_get_hw_type_name(cryif_hw_type_t hw_type)
{
    switch (hw_type) {
        case CRYIF_HW_SOFTWARE:     return "SOFTWARE";
        case CRYIF_HW_HSM:          return "HSM";
        case CRYIF_HW_TPM:          return "TPM";
        case CRYIF_HW_TEE:          return "TEE";
        case CRYIF_HW_SE:           return "SE";
        case CRYIF_HW_CRYPTO_ENGINE: return "CRYPTO_ENGINE";
        case CRYIF_HW_CUSTOM:       return "CUSTOM";
        default:                    return "UNKNOWN";
    }
}

cryif_status_t cryif_get_stats(cryif_context_t *ctx, uint64_t *hw_ops,
                               uint64_t *sw_ops, uint64_t *failed_ops)
{
    if (ctx == NULL || !ctx->initialized) {
        return CRYIF_ERROR_INVALID_PARAM;
    }
    
    if (hw_ops != NULL) *hw_ops = ctx->stats.hw_operations;
    if (sw_ops != NULL) *sw_ops = ctx->stats.sw_fallback_operations;
    if (failed_ops != NULL) *failed_ops = ctx->stats.failed_operations;
    
    return CRYIF_OK;
}

const char* cryif_get_version(void)
{
    return CRYIF_VERSION;
}

/* ============================================================================
 * Internal Functions
 * ============================================================================ */

static cryif_driver_t* cryif_select_driver(cryif_context_t *ctx,
                                           cryif_operation_t op,
                                           uint32_t algorithm)
{
    (void)op;
    
    /* First try to find driver that supports the algorithm */
    cryif_driver_t *driver = cryif_find_driver_for_algo(ctx, algorithm);
    if (driver != NULL) {
        return driver;
    }
    
    /* Fall back to default driver */
    return ctx->default_driver;
}

static cryif_key_type_t cryif_map_algorithm_to_key_type(uint32_t algorithm)
{
    /* Map CSM algorithm to CryIf key type */
    if (algorithm >= 0x60 && algorithm < 0x80) {
        /* ECC algorithms */
        switch (algorithm) {
            case 0x60: return CRYIF_KEY_TYPE_ECC_P192;
            case 0x61: return CRYIF_KEY_TYPE_ECC_P224;
            case 0x62: return CRYIF_KEY_TYPE_ECC_P256;
            case 0x63: return CRYIF_KEY_TYPE_ECC_P384;
            case 0x64: return CRYIF_KEY_TYPE_ECC_P521;
            default: return CRYIF_KEY_TYPE_AES_128;
        }
    } else if (algorithm >= 0x40 && algorithm < 0x60) {
        /* RSA algorithms */
        switch (algorithm) {
            case 0x41: return CRYIF_KEY_TYPE_RSA_1024;
            case 0x42:
            case 0x45:
            case 0x46: return CRYIF_KEY_TYPE_RSA_2048;
            case 0x43:
            case 0x47: return CRYIF_KEY_TYPE_RSA_4096;
            default: return CRYIF_KEY_TYPE_RSA_2048;
        }
    }
    
    return CRYIF_KEY_TYPE_AES_128;
}
