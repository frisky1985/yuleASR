/**********************************************************************************************************************
 * SHA-256 与密钥存储操作实现
 * 自动拆分自 Crypto_S32K312_Hsm.c
 *********************************************************************************************************************/
#define CRYPTO_START_SEC_CODE
#include "MemMap.h"

Std_ReturnType S32K312_Hsm_Sha256Init(S32K312_HsmSha256ContextType* context)
{
    uint32 i;
    
    if (context == NULL_PTR) {
        return E_NOT_OK;
    }
    
    context->totalLength = 0U;
    context->bufferLength = 0U;
    context->initialized = FALSE;
    
    for (i = 0U; i < S32K312_HSM_SHA256_BLOCK_SIZE; i++) {
        context->buffer[i] = 0U;
    }
    
    return E_OK;
}

/**********************************************************************************************************************
 * S32K312_Hsm_Sha256Update
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_Sha256Update(S32K312_HsmSha256ContextType* context,
                                         const uint8* data,
                                         uint32 length)
{
    uint32 i;
    
    if ((context == NULL_PTR) || (data == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_State != S32K312_HSM_STATE_READY) {
        return E_NOT_OK;
    }
    
    /* Accumulate data in buffer */
    for (i = 0U; i < length; i++) {
        context->buffer[context->bufferLength] = data[i];
        context->bufferLength++;
        context->totalLength++;
        
        /* Process when buffer is full */
        if (context->bufferLength >= S32K312_HSM_SHA256_BLOCK_SIZE) {
            /* In streaming mode, would process block here */
            context->bufferLength = 0U;
        }
    }
    
    context->initialized = TRUE;
    return E_OK;
}

/**********************************************************************************************************************
 * S32K312_Hsm_Sha256Finish
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_Sha256Finish(S32K312_HsmSha256ContextType* context,
                                         uint8* digest)
{
    Std_ReturnType result;
    
    if ((context == NULL_PTR) || (digest == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    /* Process any remaining data */
    result = S32K312_Hsm_Sha256(context->buffer, context->bufferLength, digest);
    
    /* Clear sensitive context */
    S32K312_Hsm_Sha256Init(context);
    
    return result;
}

/**********************************************************************************************************************
 * S32K312_Hsm_Sha256
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_Sha256(const uint8* data,
                                   uint32 length,
                                   uint8* digest)
{
    Std_ReturnType result = E_NOT_OK;
    uint32 i;
    
    if ((data == NULL_PTR) || (digest == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_State != S32K312_HSM_STATE_READY) {
        return E_NOT_OK;
    }
    
    if ((S32K312_Hsm_ConfigPtr == NULL_PTR) || (!S32K312_Hsm_ConfigPtr->enableSha)) {
        return E_NOT_OK;
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_BUSY;
    
    result = S32K312_Hsm_WaitForShaReady(S32K312_HSM_TIMEOUT_SHA);
    if (result == E_OK) {
        if (S32K312_Hsm_ShaRegs != NULL_PTR) {
            /* Setup SHA-256 mode */
            S32K312_Hsm_ShaRegs->CTRL = S32K312_HSM_SHA_CTRL_ENABLE | 
                                         S32K312_HSM_SHA_CTRL_MODE_SHA256;
            S32K312_Hsm_ShaRegs->DATA_LEN = length;
            
            /* Process data in blocks */
            /* Simplified implementation - full version would handle all blocks */
            if (length <= 64U) {
                uint32 wordCount = (length + 3U) / 4U;
                for (i = 0U; i < wordCount; i++) {
                    S32K312_Hsm_ShaRegs->DATA_IN[i] =
                        ((uint32)data[i * 4U] << 24) |
                        ((uint32)data[i * 4U + 1U] << 16) |
                        ((uint32)data[i * 4U + 2U] << 8) |
                        (uint32)data[i * 4U + 3U];
                }
            }
            
            /* Start hash computation */
            S32K312_Hsm_ShaRegs->CTRL |= S32K312_HSM_SHA_CTRL_START;
            
            result = S32K312_Hsm_WaitForShaReady(S32K312_HSM_TIMEOUT_SHA);
            if (result == E_OK) {
                /* Check for error */
                if ((S32K312_Hsm_ShaRegs->STATUS & S32K312_HSM_SHA_CTRL_ERROR) != 0U) {
                    S32K312_Hsm_LastError = S32K312_HSM_ERROR_HARDWARE;
                    result = E_NOT_OK;
                } else {
                    /* Read digest */
                    for (i = 0U; i < 8U; i++) {
                        uint32 data = S32K312_Hsm_ShaRegs->DIGEST[i];
                        digest[i * 4U] = (uint8)(data >> 24);
                        digest[i * 4U + 1U] = (uint8)(data >> 16);
                        digest[i * 4U + 2U] = (uint8)(data >> 8);
                        digest[i * 4U + 3U] = (uint8)(data);
                    }
                }
            }
        }
    }
    
    if (result == E_OK) {
        S32K312_Hsm_OperationCount++;
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_READY;
    return result;
}

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS - KEY STORAGE
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * S32K312_Hsm_KeyImport
 *********************************************************************************************************************/
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
                ((uint32)keyData[i * 4U + 1U] << 16) |
                ((uint32)keyData[i * 4U + 2U] << 8) |
                (uint32)keyData[i * 4U + 3U];
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
#define CRYPTO_STOP_SEC_CODE
#include "MemMap.h"
