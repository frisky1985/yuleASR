/**
 * @file NvM_WriteProtection.c
 * @brief AUTOSAR NvM Write Protection Implementation
 * @version 4.4.0
 * @date 2025
 *
 * AUTOSAR Classic Platform - NvM Write Protection (Module ID: 0x0E)
 *
 * Implements write protection mechanisms:
 * - Block-level write protection
 * - Time-based protection windows
 * - Incremental storage status management
 * - Write-once protection
 *
 * Features:
 * - Configurable write protection per block
 * - Protection window timer
 * - Automatic protection after write
 * - Write-once enforcement
 *
 * Copyright (c) 2025
 */

#include "NvM_Private.h"

/*============================================================================*
 * Local Variables
 *============================================================================*/
static uint32_t NvM_WriteProtectionStartTime = 0u;
static boolean NvM_GlobalWriteProtection = FALSE;

/*============================================================================*
 * Public API Implementation
 *============================================================================*/

/**
 * @brief Check if write protection is active for a block
 * @param BlockId Block identifier
 * @return TRUE if write protected, FALSE otherwise
 */
boolean NvM_WriteProtection_IsActive(NvM_BlockIdType BlockId)
{
    const NvM_BlockDescriptorType* config;
    uint32_t currentTime;
    uint32_t elapsedTime;
    
    if (NvM_IsBlockIdValid(BlockId) == FALSE) {
        return TRUE; /* Invalid blocks are protected */
    }
    
    config = NvM_Global.Blocks[BlockId].Config;
    
    if (config == NULL_PTR) {
        return TRUE; /* Unconfigured blocks are protected */
    }
    
    /* Check global write protection */
    if (NvM_GlobalWriteProtection == TRUE) {
        return TRUE;
    }
    
    /* Check block-level write protection flag */
    if (config->BlockWriteProt == TRUE) {
        return TRUE;
    }
    
    /* Check runtime write protection status */
    if (NvM_Global.Blocks[BlockId].Status.WriteProtected == TRUE) {
        /* Check if protection window has expired */
        currentTime = NvM_Global.CurrentTimeMs;
        elapsedTime = currentTime - NvM_Global.Blocks[BlockId].Status.LastWriteTime;
        
        if (elapsedTime < NvM_Config.WriteProtectionWindow) {
            return TRUE;
        } else {
            /* Protection window expired - clear protection */
            NvM_Global.Blocks[BlockId].Status.WriteProtected = FALSE;
        }
    }
    
    /* Check write-once blocks */
    if (config->WriteBlockOnce == TRUE) {
        /* If block has been written before, it's protected */
        if (NvM_Global.Blocks[BlockId].Status.LastWriteTime > 0u) {
            return TRUE;
        }
    }
    
    return FALSE;
}

/**
 * @brief Enable write protection for a block
 * @param BlockId Block identifier
 */
void NvM_WriteProtection_Enable(NvM_BlockIdType BlockId)
{
    if (NvM_IsBlockIdValid(BlockId) == FALSE) {
        return;
    }
    
    NvM_Global.Blocks[BlockId].Status.WriteProtected = TRUE;
    NvM_Global.Blocks[BlockId].Status.LastWriteTime = NvM_Global.CurrentTimeMs;
}

/**
 * @brief Disable write protection for a block
 * @param BlockId Block identifier
 */
void NvM_WriteProtection_Disable(NvM_BlockIdType BlockId)
{
    if (NvM_IsBlockIdValid(BlockId) == FALSE) {
        return;
    }
    
    /* Cannot disable protection for write-once blocks */
    if ((NvM_Global.Blocks[BlockId].Config != NULL_PTR) &&
        (NvM_Global.Blocks[BlockId].Config->WriteBlockOnce == TRUE)) {
        return;
    }
    
    NvM_Global.Blocks[BlockId].Status.WriteProtected = FALSE;
}

/**
 * @brief Enable global write protection
 */
void NvM_WriteProtection_EnableGlobal(void)
{
    NvM_GlobalWriteProtection = TRUE;
    NvM_WriteProtectionStartTime = NvM_Global.CurrentTimeMs;
}

/**
 * @brief Disable global write protection
 */
void NvM_WriteProtection_DisableGlobal(void)
{
    NvM_GlobalWriteProtection = FALSE;
}

/**
 * @brief Check if global write protection is active
 * @return TRUE if global protection active, FALSE otherwise
 */
boolean NvM_WriteProtection_IsGlobalActive(void)
{
    if (NvM_GlobalWriteProtection == TRUE) {
        uint32_t elapsedTime = NvM_Global.CurrentTimeMs - NvM_WriteProtectionStartTime;
        
        /* Check if global protection window has expired */
        if (elapsedTime >= NvM_Config.WriteProtectionWindow) {
            NvM_GlobalWriteProtection = FALSE;
            return FALSE;
        }
        
        return TRUE;
    }
    
    return FALSE;
}

/**
 * @brief Update write protection after successful write
 * @param BlockId Block identifier
 */
void NvM_WriteProtection_UpdateAfterWrite(NvM_BlockIdType BlockId)
{
    const NvM_BlockDescriptorType* config;
    
    if (NvM_IsBlockIdValid(BlockId) == FALSE) {
        return;
    }
    
    config = NvM_Global.Blocks[BlockId].Config;
    
    if (config == NULL_PTR) {
        return;
    }
    
    /* Update last write time */
    NvM_Global.Blocks[BlockId].Status.LastWriteTime = NvM_Global.CurrentTimeMs;
    
    /* Enable automatic write protection if configured */
    #if (NVM_AUTO_WRITE_PROTECT == STD_ON)
    if (config->BlockWriteProt == TRUE) {
        NvM_Global.Blocks[BlockId].Status.WriteProtected = TRUE;
    }
    #endif
    
    /* Write-once blocks are automatically protected after first write */
    if (config->WriteBlockOnce == TRUE) {
        NvM_Global.Blocks[BlockId].Status.WriteProtected = TRUE;
    }
}

/**
 * @brief Check if block has write-once protection
 * @param BlockId Block identifier
 * @return TRUE if write-once, FALSE otherwise
 */
boolean NvM_WriteProtection_IsWriteOnce(NvM_BlockIdType BlockId)
{
    if (NvM_IsBlockIdValid(BlockId) == FALSE) {
        return FALSE;
    }
    
    if (NvM_Global.Blocks[BlockId].Config == NULL_PTR) {
        return FALSE;
    }
    
    return (NvM_Global.Blocks[BlockId].Config->WriteBlockOnce == TRUE) ? TRUE : FALSE;
}
