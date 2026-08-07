/*==================================================================================================
 * S32K312 HSM 密钥管理及辅助函数
 * 自动拆分自 Crypto_S32K312_Hsm.c
 *================================================================================================*/
#define CRYPTO_START_SEC_CODE
#include "MemMap.h"
Std_ReturnType S32K312_Hsm_KeyImport(uint8 slotId,
                                      uint8 keyType,
                                      const uint8* keyData,
                                      uint16 keyLength)
{
    Std_ReturnType result = E_NOT_OK;
    uint32 i;
    uint32 wordCount;
    
    if (keyData == NULL_PTR) {
        return E_NOT_OK;
    }
    
    if (slotId >= S32K312_HSM_MAX_KEY_SLOTS) {
        S32K312_Hsm_ReportError(S32K312_HSM_SID_AES_ENCRYPT, S32K312_HSM_ERROR_INVALID_PARAM);
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_State != S32K312_HSM_STATE_READY) {
        return E_NOT_OK;
    }
    
    if ((S32K312_Hsm_ConfigPtr == NULL_PTR) || (!S32K312_Hsm_ConfigPtr->enableKeyStore)) {
        return E_NOT_OK;
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_BUSY;
    
    if (S32K312_Hsm_KeyStoreRegs != NULL_PTR) {
        /* Select key slot */
        S32K312_Hsm_KeyStoreRegs->SLOT_SEL = (uint32)slotId;
        
        /* Load key data */
        wordCount = ((uint32)keyLength + 3U) / 4U;
        for (i = 0U; i < wordCount; i++) {
            S32K312_Hsm_KeyStoreRegs->KEY_DATA[i] =
                ((uint32)keyData[i * 4U] << 24) |
                ((uint32)keyData[(i * 4U) + 1U] << 16) |
                ((uint32)keyData[(i * 4U) + 2U] << 8) |
                (uint32)keyData[(i * 4U) + 3U];
        }
        
        /* Issue load command */
        S32K312_Hsm_KeyStoreRegs->CTRL = S32K312_HSM_KEYSTORE_CMD_LOAD | 
                                          ((uint32)keyType << 8) | 
                                          ((uint32)keyLength << 16);
        
        /* Wait for completion */
        result = S32K312_Hsm_WaitReady(S32K312_HSM_TIMEOUT_DEFAULT);
        if (result != E_OK) {
            S32K312_Hsm_LastError = S32K312_HSM_ERROR_TIMEOUT;
        }
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_READY;
    return result;
}

/**********************************************************************************************************************
 * S32K312_Hsm_KeyExport
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_KeyExport(uint8 slotId,
                                      uint8* keyData,
                                      uint16* keyLength)
{
    /* Key export may be restricted based on slot configuration */
    /* For security, many slots may not allow export */
    
    (void)slotId;
    (void)keyData;
    (void)keyLength;
    
    return E_NOT_OK;  /* Not implemented - key export restricted */
}

/**********************************************************************************************************************
 * S32K312_Hsm_KeyErase
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_KeyErase(uint8 slotId)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (slotId >= S32K312_HSM_MAX_KEY_SLOTS) {
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_State != S32K312_HSM_STATE_READY) {
        return E_NOT_OK;
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_BUSY;
    
    if (S32K312_Hsm_KeyStoreRegs != NULL_PTR) {
        /* Select key slot */
        S32K312_Hsm_KeyStoreRegs->SLOT_SEL = (uint32)slotId;
        
        /* Issue erase command */
        S32K312_Hsm_KeyStoreRegs->CTRL = S32K312_HSM_KEYSTORE_CMD_ERASE;
        
        /* Wait for completion */
        result = S32K312_Hsm_WaitReady(S32K312_HSM_TIMEOUT_DEFAULT);
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_READY;
    return result;
}

/**********************************************************************************************************************
 * S32K312_Hsm_KeyGetSlotInfo
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_KeyGetSlotInfo(uint8 slotId,
                                           S32K312_HsmKeySlotType* slotInfo)
{
    if ((slotId >= S32K312_HSM_MAX_KEY_SLOTS) || (slotInfo == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_KeyStoreRegs != NULL_PTR) {
        /* Read slot status from hardware */
        uint32 lockStatus = S32K312_Hsm_KeyStoreRegs->LOCK[slotId / 8U];
        
        slotInfo->slotId = slotId;
        slotInfo->locked = ((lockStatus >> (slotId % 8U)) & 0x1U) != 0U;
        slotInfo->occupied = FALSE; /* Would be determined from hardware status */
        slotInfo->keyType = 0U;
        slotInfo->keyLength = 0U;
    } else {
        return E_NOT_OK;
    }
    
    return E_OK;
}

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS - UTILITY
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * S32K312_Hsm_WaitReady
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_WaitReady(uint32 timeoutUs)
{
    /* Simple timeout loop - in production, use proper timer */
    volatile uint32 i;
    uint32 maxIterations = timeoutUs * 10U; /* Approximate */
    
    for (i = 0U; i < maxIterations; i++) {
        if ((S32K312_Hsm_GlobalRegs != NULL_PTR) &&
            ((S32K312_Hsm_GlobalRegs->STATUS & S32K312_HSM_STATUS_BUSY) == 0U)) {
            return E_OK;
        }
    }
    
    return E_NOT_OK;  /* Timeout */
}

/**********************************************************************************************************************
 * S32K312_Hsm_ClearError
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_ClearError(void)
{
    if (S32K312_Hsm_GlobalRegs != NULL_PTR) {
        /* Clear error status in hardware */
        S32K312_Hsm_GlobalRegs->ERROR_STATUS = 0xFFFFFFFFU;
    }
    
    S32K312_Hsm_LastError = S32K312_HSM_SUCCESS;
    
    if (S32K312_Hsm_State == S32K312_HSM_STATE_ERROR) {
        S32K312_Hsm_State = S32K312_HSM_STATE_READY;
    }
    
    return E_OK;
}

/**********************************************************************************************************************
 * S32K312_Hsm_GetFirmwareVersion
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_GetFirmwareVersion(uint8* version)
{
    if (version == NULL_PTR) {
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_GlobalRegs != NULL_PTR) {
        uint32 fwVersion = S32K312_Hsm_GlobalRegs->VERSION;
        version[0] = (uint8)((fwVersion >> 24) & 0xFFU);
        version[1] = (uint8)((fwVersion >> 16) & 0xFFU);
        version[2] = (uint8)((fwVersion >> 8) & 0xFFU);
        version[3] = (uint8)(fwVersion & 0xFFU);
    } else {
        version[0] = S32K312_HSM_SW_MAJOR_VERSION;
        version[1] = S32K312_HSM_SW_MINOR_VERSION;
        version[2] = S32K312_HSM_SW_PATCH_VERSION;
        version[3] = 0U;
    }
    
    return E_OK;
}

/**********************************************************************************************************************
 * LOCAL FUNCTIONS
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * S32K312_Hsm_ValidateConfig
 *********************************************************************************************************************/
STATIC Std_ReturnType S32K312_Hsm_ValidateConfig(const S32K312_HsmConfigType* config)
{
    if (config == NULL_PTR) {
        return E_NOT_OK;
    }
    
    /* At least one module should be enabled */
    if ((!config->enableAes) && (!config->enableEcc) && 
        (!config->enableSha) && (!config->enableTrng) && (!config->enableKeyStore)) {
        return E_NOT_OK;
    }
    
    /* Validate timeout */
    if (config->timeoutUs == 0U) {
        /* Use default timeout */
    }
    
    return E_OK;
}

/**********************************************************************************************************************
 * S32K312_Hsm_InitRegisters
 *********************************************************************************************************************/
STATIC Std_ReturnType S32K312_Hsm_InitRegisters(void)
{
    /* Initialize register pointers to HSM base addresses */
    S32K312_Hsm_GlobalRegs = (S32K312_HsmGlobalRegsType*)S32K312_HSM_BASE_ADDR;
    S32K312_Hsm_AesRegs = (S32K312_HsmAesRegsType*)S32K312_HSM_AES_BASE;
    S32K312_Hsm_EccRegs = (S32K312_HsmEccRegsType*)S32K312_HSM_ECC_BASE;
    S32K312_Hsm_ShaRegs = (S32K312_HsmShaRegsType*)S32K312_HSM_SHA_BASE;
    S32K312_Hsm_KeyStoreRegs = (S32K312_HsmKeyStoreRegsType*)S32K312_HSM_KEYSTORE_BASE;
    
    /* Verify hardware presence by checking version register */
    if (S32K312_Hsm_GlobalRegs != NULL_PTR) {
        uint32 version = S32K312_Hsm_GlobalRegs->VERSION;
        if (version == 0U) {
            /* No hardware detected */
            return E_NOT_OK;
        }
    }
    
    return E_OK;
}

/**********************************************************************************************************************
 * S32K312_Hsm_WaitForAesReady
 *********************************************************************************************************************/
STATIC Std_ReturnType S32K312_Hsm_WaitForAesReady(uint32 timeoutUs)
{
    volatile uint32 i;
    uint32 maxIterations = timeoutUs * 10U;
    
    for (i = 0U; i < maxIterations; i++) {
        if (S32K312_Hsm_AesRegs != NULL_PTR) {
            uint32 status = S32K312_Hsm_AesRegs->STATUS;
            if (((status & S32K312_HSM_AES_CTRL_BUSY) == 0U) ||
                ((status & S32K312_HSM_AES_CTRL_DONE) != 0U)) {
                return E_OK;
            }
        }
    }
    
    return E_NOT_OK;
}

/**********************************************************************************************************************
 * S32K312_Hsm_WaitForEccReady
 *********************************************************************************************************************/
STATIC Std_ReturnType S32K312_Hsm_WaitForEccReady(uint32 timeoutUs)
{
    volatile uint32 i;
    uint32 maxIterations = timeoutUs * 10U;
    
    for (i = 0U; i < maxIterations; i++) {
        if (S32K312_Hsm_EccRegs != NULL_PTR) {
            uint32 status = S32K312_Hsm_EccRegs->STATUS;
            if (((status & S32K312_HSM_ECC_CTRL_BUSY) == 0U) ||
                ((status & S32K312_HSM_ECC_CTRL_DONE) != 0U)) {
                return E_OK;
            }
        }
    }
    
    return E_NOT_OK;
}

/**********************************************************************************************************************
 * S32K312_Hsm_WaitForShaReady
 *********************************************************************************************************************/
STATIC Std_ReturnType S32K312_Hsm_WaitForShaReady(uint32 timeoutUs)
{
    volatile uint32 i;
    uint32 maxIterations = timeoutUs * 10U;
    
    for (i = 0U; i < maxIterations; i++) {
        if (S32K312_Hsm_ShaRegs != NULL_PTR) {
            uint32 status = S32K312_Hsm_ShaRegs->STATUS;
            if (((status & S32K312_HSM_SHA_CTRL_BUSY) == 0U) ||
                ((status & S32K312_HSM_SHA_CTRL_DONE) != 0U)) {
                return E_OK;
            }
        }
    }
    
    return E_NOT_OK;
}

/**********************************************************************************************************************
 * S32K312_Hsm_ClearAesContext
 *********************************************************************************************************************/
STATIC void S32K312_Hsm_ClearAesContext(void)
{
    uint32 i;
    
    if (S32K312_Hsm_AesRegs != NULL_PTR) {
        /* Clear key registers */
        for (i = 0U; i < 8U; i++) {
            S32K312_Hsm_AesRegs->KEY[i] = 0U;
        }
        /* Clear IV registers */
        for (i = 0U; i < 4U; i++) {
            S32K312_Hsm_AesRegs->IV[i] = 0U;
        }
        /* Clear data registers */
        for (i = 0U; i < 4U; i++) {
            S32K312_Hsm_AesRegs->DATA_IN[i] = 0U;
            S32K312_Hsm_AesRegs->DATA_OUT[i] = 0U;
        }
    }
}

/**********************************************************************************************************************
 * S32K312_Hsm_ClearEccContext
 *********************************************************************************************************************/
STATIC void S32K312_Hsm_ClearEccContext(void)
{
    uint32 i;
    
    if (S32K312_Hsm_EccRegs != NULL_PTR) {
        /* Clear scalar registers */
        for (i = 0U; i < 12U; i++) {
            S32K312_Hsm_EccRegs->SCALAR[i] = 0U;
        }
        /* Clear point registers */
        for (i = 0U; i < 24U; i++) {
            S32K312_Hsm_EccRegs->POINT_IN[i] = 0U;
            S32K312_Hsm_EccRegs->POINT_OUT[i] = 0U;
        }
    }
}

/**********************************************************************************************************************
 * S32K312_Hsm_ReportError
 *********************************************************************************************************************/
STATIC void S32K312_Hsm_ReportError(uint8 serviceId, uint32 errorCode)
{
    (void)serviceId;
    S32K312_Hsm_LastError = errorCode;
    
    #if (CRYPTO_CFG_DEV_ERROR_DETECT == STD_ON)
    /* Report to DET if configured */
    /* Det_ReportError(CRYPTO_MODULE_ID, 0, serviceId, (uint8)errorCode); */
    #endif
}


#define CRYPTO_STOP_SEC_CODE
#include "MemMap.h"
