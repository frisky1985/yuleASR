/**
 * @file crypto_stack_integration.h
 * @brief Crypto Stack Integration Layer
 * @version 1.0
 * @date 2026-04-25
 *
 * Integration layer for CSM/CryIf/KeyM with SecOC and DDS Security
 * Provides unified interface for the crypto stack components
 */

#ifndef CRYPTO_STACK_INTEGRATION_H
#define CRYPTO_STACK_INTEGRATION_H

#include "csm/csm_core.h"
#include "cryif/cryif_core.h"
#include "keym/keym_core.h"

/* SecOC Integration */
#include "secoc/secoc_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Unified Crypto Stack Context
 * ============================================================================ */

typedef struct {
    csm_context_t *csm;
    cryif_context_t *cryif;
    keym_context_t *keym;
    secoc_context_t *secoc;
    void *dds_security;  /* DDS Security context */
    bool initialized;
} crypto_stack_context_t;

/* ============================================================================
 * Initialization
 * ============================================================================ */

/**
 * @brief Initialize the complete crypto stack
 * @return Unified context pointer, NULL on failure
 */
crypto_stack_context_t* crypto_stack_init(void);

/**
 * @brief Deinitialize the crypto stack
 * @param ctx Unified context
 */
void crypto_stack_deinit(crypto_stack_context_t *ctx);

/* ============================================================================
 * SecOC Integration
 * ============================================================================ */

/**
 * @brief Configure SecOC to use KeyM for key management
 * @param ctx Crypto stack context
 * @return Status code
 */
int crypto_stack_configure_secoc_keys(crypto_stack_context_t *ctx);

/**
 * @brief Get key slot for SecOC PDU
 * @param ctx Crypto stack context
 * @param pdu_id SecOC PDU ID
 * @return Key slot ID
 */
uint8_t crypto_stack_get_secoc_key_slot(crypto_stack_context_t *ctx, uint32_t pdu_id);

/* ============================================================================
 * DDS Security Integration
 * ============================================================================ */

/**
 * @brief Integrate DDS Security certificate repository with KeyM
 * @param ctx Crypto stack context
 * @param dds_sec DDS Security context
 * @return Status code
 */
int crypto_stack_integrate_dds_security(crypto_stack_context_t *ctx, void *dds_sec);

/**
 * @brief Import DDS certificate keys to KeyM
 * @param ctx Crypto stack context
 * @param cert_name Certificate name
 * @param key_slot Output key slot
 * @return Status code
 */
int crypto_stack_import_dds_cert_key(crypto_stack_context_t *ctx, 
                                      const char *cert_name,
                                      uint8_t *key_slot);

/**
 * @brief Rotate keys for DDS Security
 * @param ctx Crypto stack context
 * @return Number of keys rotated
 */
uint32_t crypto_stack_rotate_dds_keys(crypto_stack_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* CRYPTO_STACK_INTEGRATION_H */
