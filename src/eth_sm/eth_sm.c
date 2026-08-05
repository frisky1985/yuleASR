/**
 * @file eth_sm.c
 * @brief EthSM (Ethernet State Manager) implementation
 * @version 1.0
 * @date 2026-04-26
 *
 * @note MISRA C:2012 compliant
 * @note AUTOSAR EthSM specification compliant
 * @note State machine: UNINIT -> INIT -> WAIT_REQ -> READY
 */

#include <string.h>
#include "eth_sm.h"

/* ============================================================================
 * Private data structures
 * ============================================================================ */

typedef struct {
    bool used;
    eth_sm_network_config_t config;
    eth_sm_network_info_t info;
    eth_sm_mode_indication_cb_t mode_cb;
    void *mode_cb_data;
    eth_sm_state_change_cb_t state_cb;
    void *state_cb_data;
    eth_sm_link_change_cb_t link_cb;
    void *link_cb_data;
    bool mode_requested;
    eth_sm_mode_t requested_mode;
    bool wakeup_pending;
    uint32_t startup_delay_remaining;
    uint32_t shutdown_delay_remaining;
} eth_sm_network_internal_t;

typedef struct {
    bool initialized;
    const eth_sm_config_t *config;
    eth_sm_network_internal_t networks[ETHSM_MAX_NETWORKS];
    eth_sm_bswm_request_cb_t bswm_request_cb;
    void *bswm_request_data;
} eth_sm_ctx_t;

/* ============================================================================
 * Static variables
 * ============================================================================ */

static eth_sm_ctx_t g_ethsm_ctx;

/* ============================================================================
 * Internal function declarations
 * ============================================================================ */

static int16_t eth_sm_find_free_network(void);
static eth_sm_error_t eth_sm_validate_network_idx(uint8_t network_idx);
static void eth_sm_set_state(uint8_t network_idx, eth_sm_state_t new_state);
static void eth_sm_set_mode(uint8_t network_idx, eth_sm_mode_t new_mode);
static void eth_sm_set_link_state(uint8_t network_idx, eth_sm_link_state_t link_state);

/* ============================================================================
 * EthSM lifecycle API implementation
 * ============================================================================ */

eth_sm_error_t eth_sm_init(const eth_sm_config_t *config)
{
    eth_sm_error_t result = ETHSM_OK;

    if (g_ethsm_ctx.initialized)
    {
        result = ETHSM_E_ALREADY_INITIALIZED;
    }
    else
    {
        (void)memset(&g_ethsm_ctx, 0, sizeof(g_ethsm_ctx));
        g_ethsm_ctx.config = config;
        g_ethsm_ctx.initialized = true;

        /* Initialize all networks to UNINIT state */
        uint8_t i;
        for (i = 0u; i < ETHSM_MAX_NETWORKS; i++)
        {
            g_ethsm_ctx.networks[i].info.network_idx = i;
            g_ethsm_ctx.networks[i].info.sm_state = ETHSM_STATE_UNINIT;
            g_ethsm_ctx.networks[i].info.current_mode = ETHSM_MODE_NO_COMM;
            g_ethsm_ctx.networks[i].info.link_state = ETHSM_LINK_OFF;
            g_ethsm_ctx.networks[i].info.comm_state = ETHSM_COMM_NONE;
        }
    }

    return result;
}

void eth_sm_deinit(void)
{
    uint8_t i;

    if (g_ethsm_ctx.initialized)
    {
        for (i = 0u; i < ETHSM_MAX_NETWORKS; i++)
        {
            if (g_ethsm_ctx.networks[i].used)
            {
                (void)eth_sm_delete_network(i);
            }
        }
        g_ethsm_ctx.initialized = false;
    }
}

void eth_sm_get_version(uint8_t *major, uint8_t *minor, uint8_t *patch)
{
    if ((major != NULL) && (minor != NULL) && (patch != NULL))
    {
        *major = ETHSM_MAJOR_VERSION;
        *minor = ETHSM_MINOR_VERSION;
        *patch = ETHSM_PATCH_VERSION;
    }
}

/* ============================================================================
 * Network management API implementation
 * ============================================================================ */

eth_sm_error_t eth_sm_create_network(const eth_sm_network_config_t *config,
                                      uint8_t *network_idx)
{
    eth_sm_error_t result;
    int16_t idx;

    result = eth_sm_validate_config(config);
    
    if ((result == ETHSM_OK) && (network_idx != NULL))
    {
        idx = eth_sm_find_free_network();
        
        if (idx < 0)
        {
            result = ETHSM_E_NO_RESOURCE;
        }
        else
        {
            (void)memcpy(&g_ethsm_ctx.networks[idx].config, config,
                        sizeof(eth_sm_network_config_t));
            g_ethsm_ctx.networks[idx].used = true;
            g_ethsm_ctx.networks[idx].info.network_idx = (uint8_t)idx;
            
            /* State transition: UNINIT -> INIT */
            eth_sm_set_state((uint8_t)idx, ETHSM_STATE_INIT);
            
            /* If start_all_channels is set, transition to WAIT_REQ */
            if (config->start_all_channels)
            {
                eth_sm_set_state((uint8_t)idx, ETHSM_STATE_WAIT_REQ);
            }
            
            *network_idx = (uint8_t)idx;
        }
    }
    else
    {
        result = ETHSM_E_PARAM_POINTER;
    }

    return result;
}

eth_sm_error_t eth_sm_delete_network(uint8_t network_idx)
{
    eth_sm_error_t result;

    result = eth_sm_validate_network_idx(network_idx);
    
    if (result == ETHSM_OK)
    {
        if (g_ethsm_ctx.networks[network_idx].info.sm_state == ETHSM_STATE_READY)
        {
            /* Release communication mode first */
            (void)eth_sm_release_com_mode(network_idx);
        }
        
        g_ethsm_ctx.networks[network_idx].used = false;
        g_ethsm_ctx.networks[network_idx].info.sm_state = ETHSM_STATE_UNINIT;
    }

    return result;
}

eth_sm_error_t eth_sm_get_network_config(uint8_t network_idx,
                                          eth_sm_network_config_t *config)
{
    eth_sm_error_t result;

    result = eth_sm_validate_network_idx(network_idx);
    
    if ((result == ETHSM_OK) && (config != NULL))
    {
        (void)memcpy(config, &g_ethsm_ctx.networks[network_idx].config,
                    sizeof(eth_sm_network_config_t));
    }
    else
    {
        result = ETHSM_E_PARAM_POINTER;
    }

    return result;
}

/* ============================================================================
 * State machine control API implementation
 * ============================================================================ */

eth_sm_error_t eth_sm_request_com_mode(uint8_t network_idx)
{
    eth_sm_error_t result;

    result = eth_sm_validate_network_idx(network_idx);
    
    if (result == ETHSM_OK)
    {
        if (g_ethsm_ctx.networks[network_idx].info.sm_state == ETHSM_STATE_WAIT_REQ)
        {
            /* Set mode request flag and prepare for transition to READY */
            g_ethsm_ctx.networks[network_idx].mode_requested = true;
            g_ethsm_ctx.networks[network_idx].requested_mode = ETHSM_MODE_FULL_COMM;
            g_ethsm_ctx.networks[network_idx].startup_delay_remaining =
                g_ethsm_ctx.networks[network_idx].config.startup_delay_ms;
        }
        else
        {
            result = ETHSM_E_INV_MODE;
        }
    }

    return result;
}

eth_sm_error_t eth_sm_release_com_mode(uint8_t network_idx)
{
    eth_sm_error_t result;

    result = eth_sm_validate_network_idx(network_idx);
    
    if (result == ETHSM_OK)
    {
        if (g_ethsm_ctx.networks[network_idx].info.sm_state == ETHSM_STATE_READY)
        {
            /* Prepare for transition to WAIT_REQ */
            g_ethsm_ctx.networks[network_idx].mode_requested = false;
            g_ethsm_ctx.networks[network_idx].requested_mode = ETHSM_MODE_NO_COMM;
            g_ethsm_ctx.networks[network_idx].shutdown_delay_remaining =
                g_ethsm_ctx.networks[network_idx].config.shutdown_delay_ms;
        }
        else
        {
            result = ETHSM_E_INV_MODE;
        }
    }

    return result;
}

eth_sm_error_t eth_sm_set_network_mode(uint8_t network_idx, eth_sm_mode_t mode)
{
    eth_sm_error_t result;

    result = eth_sm_validate_network_idx(network_idx);
    
    if (result == ETHSM_OK)
    {
        eth_sm_set_mode(network_idx, mode);
    }

    return result;
}

eth_sm_error_t eth_sm_get_network_mode(uint8_t network_idx,
                                        eth_sm_mode_t *mode)
{
    eth_sm_error_t result;

    result = eth_sm_validate_network_idx(network_idx);
    
    if ((result == ETHSM_OK) && (mode != NULL))
    {
        *mode = g_ethsm_ctx.networks[network_idx].info.current_mode;
    }
    else
    {
        result = ETHSM_E_PARAM_POINTER;
    }

    return result;
}

eth_sm_error_t eth_sm_get_state(uint8_t network_idx, eth_sm_state_t *state)
{
    eth_sm_error_t result;

    result = eth_sm_validate_network_idx(network_idx);
    
    if ((result == ETHSM_OK) && (state != NULL))
    {
        *state = g_ethsm_ctx.networks[network_idx].info.sm_state;
    }
    else
    {
        result = ETHSM_E_PARAM_POINTER;
    }

    return result;
}

eth_sm_error_t eth_sm_is_network_ready(uint8_t network_idx, bool *ready)
{
    eth_sm_error_t result;

    result = eth_sm_validate_network_idx(network_idx);
    
    if ((result == ETHSM_OK) && (ready != NULL))
    {
        *ready = (g_ethsm_ctx.networks[network_idx].info.sm_state == ETHSM_STATE_READY) &&
                 (g_ethsm_ctx.networks[network_idx].info.link_state == ETHSM_LINK_ON);
    }
    else
    {
        result = ETHSM_E_PARAM_POINTER;
    }

    return result;
}

/* ============================================================================
 * Link state API implementation
 * ============================================================================ */

eth_sm_error_t eth_sm_link_state_change(uint8_t network_idx,
                                         eth_sm_link_state_t link_state)
{
    eth_sm_error_t result;

    result = eth_sm_validate_network_idx(network_idx);
    
    if (result == ETHSM_OK)
    {
        eth_sm_set_link_state(network_idx, link_state);
    }

    return result;
}

eth_sm_error_t eth_sm_get_link_state(uint8_t network_idx,
                                      eth_sm_link_state_t *link_state)
{
    eth_sm_error_t result;

    result = eth_sm_validate_network_idx(network_idx);
    
    if ((result == ETHSM_OK) && (link_state != NULL))
    {
        *link_state = g_ethsm_ctx.networks[network_idx].info.link_state;
    }
    else
    {
        result = ETHSM_E_PARAM_POINTER;
    }

    return result;
}

eth_sm_error_t eth_sm_wait_for_link(uint8_t network_idx, uint32_t timeout_ms)
{
    eth_sm_error_t result;
    uint32_t elapsed = 0u;

    (void)timeout_ms;
    result = eth_sm_validate_network_idx(network_idx);
    
    if (result == ETHSM_OK)
    {
        /* Simplified - wait for link up */
        while ((elapsed < timeout_ms) && 
               (g_ethsm_ctx.networks[network_idx].info.link_state != ETHSM_LINK_ON))
        {
            elapsed += 10u;
        }
        
        if (g_ethsm_ctx.networks[network_idx].info.link_state != ETHSM_LINK_ON)
        {
            result = ETHSM_E_NOT_OK;
        }
    }

    return result;
}

/* ============================================================================
 * Main function API implementation
 * ============================================================================ */

void eth_sm_main_function(uint32_t elapsed_ms)
{
    uint8_t i;

    if (g_ethsm_ctx.initialized)
    {
        for (i = 0u; i < ETHSM_MAX_NETWORKS; i++)
        {
            if (g_ethsm_ctx.networks[i].used)
            {
                /* Process state machine transitions */
                switch (g_ethsm_ctx.networks[i].info.sm_state)
                {
                    case ETHSM_STATE_WAIT_REQ:
                        if (g_ethsm_ctx.networks[i].mode_requested)
                        {
                            if (g_ethsm_ctx.networks[i].startup_delay_remaining > elapsed_ms)
                            {
                                g_ethsm_ctx.networks[i].startup_delay_remaining -= elapsed_ms;
                            }
                            else
                            {
                                /* Transition to READY */
                                eth_sm_set_state(i, ETHSM_STATE_READY);
                                eth_sm_set_mode(i, ETHSM_MODE_FULL_COMM);
                                g_ethsm_ctx.networks[i].info.comm_state = ETHSM_COMM_REQUESTED;
                            }
                        }
                        break;

                    case ETHSM_STATE_READY:
                        if (!g_ethsm_ctx.networks[i].mode_requested)
                        {
                            if (g_ethsm_ctx.networks[i].shutdown_delay_remaining > elapsed_ms)
                            {
                                g_ethsm_ctx.networks[i].shutdown_delay_remaining -= elapsed_ms;
                            }
                            else
                            {
                                /* Transition to WAIT_REQ */
                                eth_sm_set_state(i, ETHSM_STATE_WAIT_REQ);
                                eth_sm_set_mode(i, ETHSM_MODE_NO_COMM);
                                g_ethsm_ctx.networks[i].info.comm_state = ETHSM_COMM_RELEASED;
                            }
                        }
                        break;

                    default:
                        /* No transitions in other states */
                        break;
                }
            }
        }
    }
}

void eth_sm_process_mode_request(uint8_t network_idx)
{
    (void)network_idx;
    /* Process pending mode requests - called from main function */
}

eth_sm_error_t eth_sm_check_transitions(uint8_t network_idx)
{
    eth_sm_error_t result;

    result = eth_sm_validate_network_idx(network_idx);
    
    /* Check if state transition is allowed */
    if (result == ETHSM_OK)
    {
        /* Simplified - actual check would validate preconditions */
        result = ETHSM_OK;
    }

    return result;
}

/* ============================================================================
 * Callback registration API implementation
 * ============================================================================ */

eth_sm_error_t eth_sm_register_mode_indication(uint8_t network_idx,
                                                eth_sm_mode_indication_cb_t callback,
                                                void *user_data)
{
    eth_sm_error_t result;

    result = eth_sm_validate_network_idx(network_idx);
    
    if (result == ETHSM_OK)
    {
        if (network_idx == 0xFFu)
        {
            /* Register for all networks */
            uint8_t i;
            for (i = 0u; i < ETHSM_MAX_NETWORKS; i++)
            {
                g_ethsm_ctx.networks[i].mode_cb = callback;
                g_ethsm_ctx.networks[i].mode_cb_data = user_data;
            }
        }
        else
        {
            g_ethsm_ctx.networks[network_idx].mode_cb = callback;
            g_ethsm_ctx.networks[network_idx].mode_cb_data = user_data;
        }
    }

    return result;
}

eth_sm_error_t eth_sm_unregister_mode_indication(uint8_t network_idx)
{
    eth_sm_error_t result;

    result = eth_sm_validate_network_idx(network_idx);
    
    if (result == ETHSM_OK)
    {
        g_ethsm_ctx.networks[network_idx].mode_cb = NULL;
        g_ethsm_ctx.networks[network_idx].mode_cb_data = NULL;
    }

    return result;
}

eth_sm_error_t eth_sm_register_state_change(uint8_t network_idx,
                                             eth_sm_state_change_cb_t callback,
                                             void *user_data)
{
    eth_sm_error_t result;

    result = eth_sm_validate_network_idx(network_idx);
    
    if (result == ETHSM_OK)
    {
        if (network_idx == 0xFFu)
        {
            /* Register for all networks */
            uint8_t i;
            for (i = 0u; i < ETHSM_MAX_NETWORKS; i++)
            {
                g_ethsm_ctx.networks[i].state_cb = callback;
                g_ethsm_ctx.networks[i].state_cb_data = user_data;
            }
        }
        else
        {
            g_ethsm_ctx.networks[network_idx].state_cb = callback;
            g_ethsm_ctx.networks[network_idx].state_cb_data = user_data;
        }
    }

    return result;
}

eth_sm_error_t eth_sm_unregister_state_change(uint8_t network_idx)
{
    eth_sm_error_t result;

    result = eth_sm_validate_network_idx(network_idx);
    
    if (result == ETHSM_OK)
    {
        g_ethsm_ctx.networks[network_idx].state_cb = NULL;
        g_ethsm_ctx.networks[network_idx].state_cb_data = NULL;
    }

    return result;
}

eth_sm_error_t eth_sm_register_link_change(uint8_t network_idx,
                                            eth_sm_link_change_cb_t callback,
                                            void *user_data)
{
    eth_sm_error_t result;

    result = eth_sm_validate_network_idx(network_idx);
    
    if (result == ETHSM_OK)
    {
        if (network_idx == 0xFFu)
        {
            /* Register for all networks */
            uint8_t i;
            for (i = 0u; i < ETHSM_MAX_NETWORKS; i++)
            {
                g_ethsm_ctx.networks[i].link_cb = callback;
                g_ethsm_ctx.networks[i].link_cb_data = user_data;
            }
        }
        else
        {
            g_ethsm_ctx.networks[network_idx].link_cb = callback;
            g_ethsm_ctx.networks[network_idx].link_cb_data = user_data;
        }
    }

    return result;
}

eth_sm_error_t eth_sm_unregister_link_change(uint8_t network_idx)
{
    eth_sm_error_t result;

    result = eth_sm_validate_network_idx(network_idx);
    
    if (result == ETHSM_OK)
    {
        g_ethsm_ctx.networks[network_idx].link_cb = NULL;
        g_ethsm_ctx.networks[network_idx].link_cb_data = NULL;
    }

    return result;
}

/* ============================================================================
 * BswM integration API implementation
 * ============================================================================ */

eth_sm_error_t eth_sm_bswm_request_mode(uint8_t network_idx, eth_sm_mode_t mode)
{
    eth_sm_error_t result;
    bool accepted = true;

    result = eth_sm_validate_network_idx(network_idx);
    
    if (result == ETHSM_OK)
    {
        /* Check with BswM callback if registered */
        if (g_ethsm_ctx.bswm_request_cb != NULL)
        {
            accepted = g_ethsm_ctx.bswm_request_cb(network_idx, mode);
        }
        
        if (accepted)
        {
            if (mode == ETHSM_MODE_FULL_COMM)
            {
                result = eth_sm_request_com_mode(network_idx);
            }
            else
            {
                result = eth_sm_release_com_mode(network_idx);
            }
        }
        else
        {
            result = ETHSM_E_NOT_OK;
        }
    }

    return result;
}

eth_sm_error_t eth_sm_bswm_get_current_mode(uint8_t network_idx,
                                             eth_sm_mode_t *mode)
{
    return eth_sm_get_network_mode(network_idx, mode);
}

eth_sm_error_t eth_sm_register_bswm_request(eth_sm_bswm_request_cb_t callback,
                                             void *user_data)
{
    eth_sm_error_t result = ETHSM_OK;

    if (!g_ethsm_ctx.initialized)
    {
        result = ETHSM_E_NOT_INITIALIZED;
    }
    else
    {
        g_ethsm_ctx.bswm_request_cb = callback;
        g_ethsm_ctx.bswm_request_data = user_data;
    }

    return result;
}

/* ============================================================================
 * EcuM integration API implementation
 * ============================================================================ */

void eth_sm_ecum_notify(uint8_t network_idx, bool has_comm)
{
    eth_sm_error_t result;

    result = eth_sm_validate_network_idx(network_idx);
    
    if (result == ETHSM_OK)
    {
        g_ethsm_ctx.networks[network_idx].info.comm_state =
            has_comm ? ETHSM_COMM_REQUESTED : ETHSM_COMM_RELEASED;
    }
}

eth_sm_error_t eth_sm_set_wakeup(uint8_t network_idx)
{
    eth_sm_error_t result;

    result = eth_sm_validate_network_idx(network_idx);
    
    if (result == ETHSM_OK)
    {
        g_ethsm_ctx.networks[network_idx].wakeup_pending = true;
    }

    return result;
}

eth_sm_error_t eth_sm_clear_wakeup(uint8_t network_idx)
{
    eth_sm_error_t result;

    result = eth_sm_validate_network_idx(network_idx);
    
    if (result == ETHSM_OK)
    {
        g_ethsm_ctx.networks[network_idx].wakeup_pending = false;
    }

    return result;
}

eth_sm_error_t eth_sm_has_wakeup(uint8_t network_idx, bool *has_wakeup)
{
    eth_sm_error_t result;

    result = eth_sm_validate_network_idx(network_idx);
    
    if ((result == ETHSM_OK) && (has_wakeup != NULL))
    {
        *has_wakeup = g_ethsm_ctx.networks[network_idx].wakeup_pending;
    }
    else
    {
        result = ETHSM_E_PARAM_POINTER;
    }

    return result;
}

/* ============================================================================
 * Diagnostic API implementation
 * ============================================================================ */

eth_sm_error_t eth_sm_get_network_info(uint8_t network_idx,
                                        eth_sm_network_info_t *info)
{
    eth_sm_error_t result;

    result = eth_sm_validate_network_idx(network_idx);
    
    if ((result == ETHSM_OK) && (info != NULL))
    {
        (void)memcpy(info, &g_ethsm_ctx.networks[network_idx].info,
                    sizeof(eth_sm_network_info_t));
    }
    else
    {
        result = ETHSM_E_PARAM_POINTER;
    }

    return result;
}

eth_sm_error_t eth_sm_get_last_transition(uint8_t network_idx,
                                           eth_sm_transition_info_t *trans)
{
    eth_sm_error_t result;

    (void)trans;
    result = eth_sm_validate_network_idx(network_idx);
    
    /* Simplified - would store last transition in real implementation */

    return result;
}

uint8_t eth_sm_get_network_count(void)
{
    return ETHSM_MAX_NETWORKS;
}

uint8_t eth_sm_get_active_network_count(void)
{
    uint8_t count = 0u;
    uint8_t i;

    for (i = 0u; i < ETHSM_MAX_NETWORKS; i++)
    {
        if (g_ethsm_ctx.networks[i].used)
        {
            count++;
        }
    }

    return count;
}

uint8_t eth_sm_get_ready_network_count(void)
{
    uint8_t count = 0u;
    uint8_t i;

    for (i = 0u; i < ETHSM_MAX_NETWORKS; i++)
    {
        if (g_ethsm_ctx.networks[i].used &&
            (g_ethsm_ctx.networks[i].info.sm_state == ETHSM_STATE_READY))
        {
            count++;
        }
    }

    return count;
}

/* ============================================================================
 * Debug and diagnostic API implementation
 * ============================================================================ */

void eth_sm_print_debug_info(uint8_t network_idx)
{
    (void)network_idx;
    /* Debug output implementation */
}

eth_sm_error_t eth_sm_validate_config(const eth_sm_config_t *config)
{
    eth_sm_error_t result = ETHSM_OK;

    if (config == NULL)
    {
        result = ETHSM_E_PARAM_POINTER;
    }
    else if (config->network_count > ETHSM_MAX_NETWORKS)
    {
        result = ETHSM_E_PARAM_CONFIG;
    }

    return result;
}

bool eth_sm_is_initialized(void)
{
    return g_ethsm_ctx.initialized;
}

eth_sm_error_t eth_sm_force_state(uint8_t network_idx, eth_sm_state_t state)
{
    eth_sm_error_t result;

    result = eth_sm_validate_network_idx(network_idx);
    
    if (result == ETHSM_OK)
    {
        eth_sm_set_state(network_idx, state);
    }

    return result;
}

/* ============================================================================
 * Internal helper functions implementation
 * ============================================================================ */

static int16_t eth_sm_find_free_network(void)
{
    int16_t idx = -1;
    uint8_t i;

    for (i = 0u; i < ETHSM_MAX_NETWORKS; i++)
    {
        if (!g_ethsm_ctx.networks[i].used)
        {
            idx = (int16_t)i;
            break;
        }
    }

    return idx;
}

static eth_sm_error_t eth_sm_validate_network_idx(uint8_t network_idx)
{
    eth_sm_error_t result;

    if (!g_ethsm_ctx.initialized)
    {
        result = ETHSM_E_NOT_INITIALIZED;
    }
    else if (network_idx >= ETHSM_MAX_NETWORKS)
    {
        result = ETHSM_E_INV_NETWORK_IDX;
    }
    else if (!g_ethsm_ctx.networks[network_idx].used)
    {
        result = ETHSM_E_INV_NETWORK_IDX;
    }
    else
    {
        result = ETHSM_OK;
    }

    return result;
}

static void eth_sm_set_state(uint8_t network_idx, eth_sm_state_t new_state)
{
    eth_sm_state_t old_state = g_ethsm_ctx.networks[network_idx].info.sm_state;
    
    if (old_state != new_state)
    {
        g_ethsm_ctx.networks[network_idx].info.sm_state = new_state;
        g_ethsm_ctx.networks[network_idx].info.trans_request_count++;
        
        /* Call state change callback */
        if (g_ethsm_ctx.networks[network_idx].state_cb != NULL)
        {
            g_ethsm_ctx.networks[network_idx].state_cb(network_idx, old_state,
                                                        new_state,
                                                        g_ethsm_ctx.networks[network_idx].state_cb_data);
        }
    }
}

static void eth_sm_set_mode(uint8_t network_idx, eth_sm_mode_t new_mode)
{
    eth_sm_mode_t old_mode = g_ethsm_ctx.networks[network_idx].info.current_mode;
    
    if (old_mode != new_mode)
    {
        g_ethsm_ctx.networks[network_idx].info.current_mode = new_mode;
        g_ethsm_ctx.networks[network_idx].info.mode_indication_count++;
        
        /* Call mode indication callback */
        if (g_ethsm_ctx.networks[network_idx].mode_cb != NULL)
        {
            g_ethsm_ctx.networks[network_idx].mode_cb(network_idx, new_mode,
                                                       g_ethsm_ctx.networks[network_idx].mode_cb_data);
        }
    }
}

static void eth_sm_set_link_state(uint8_t network_idx, eth_sm_link_state_t link_state)
{
    if (g_ethsm_ctx.networks[network_idx].info.link_state != link_state)
    {
        g_ethsm_ctx.networks[network_idx].info.link_state = link_state;
        g_ethsm_ctx.networks[network_idx].info.link_change_count++;
        
        /* Call link change callback */
        if (g_ethsm_ctx.networks[network_idx].link_cb != NULL)
        {
            g_ethsm_ctx.networks[network_idx].link_cb(network_idx, link_state,
                                                       g_ethsm_ctx.networks[network_idx].link_cb_data);
        }
    }
}
