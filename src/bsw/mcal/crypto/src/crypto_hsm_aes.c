/**********************************************************************************************************************
 * AES 操作实现
 * 自动拆分自 Crypto_S32K312_Hsm.c
 *********************************************************************************************************************/
#define CRYPTO_START_SEC_CODE
#include "MemMap.h"

Std_ReturnType S32K312_Hsm_AesInit(S32K312_HsmAesContextType* context,
                                    const uint8* key,
                                    uint32 keyLength,
                                    S32K312_HsmAesModeType mode,
                                    const uint8* iv)
{
    uint32 i;
    
    if ((context == NULL_PTR) || (key == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if ((keyLength != S32K312_HSM_AES_KEY_SIZE_128) && 
        (keyLength != S32K312_HSM_AES_KEY_SIZE_256)) {
        S32K312_Hsm_ReportError(S32K312_HSM_SID_AES_ENCRYPT, S32K312_HSM_ERROR_INVALID_PARAM);
        return E_NOT_OK;
    }
    
    if ((mode != S32K312_HSM_AES_MODE_ECB) && 
        (mode != S32K312_HSM_AES_MODE_CBC) && 
        (mode != S32K312_HSM_AES_MODE_GCM)) {
        S32K312_Hsm_ReportError(S32K312_HSM_SID_AES_ENCRYPT, S32K312_HSM_ERROR_INVALID_PARAM);
        return E_NOT_OK;
    }
    
    /* Check if HSM is ready */
    if (S32K312_Hsm_State != S32K312_HSM_STATE_READY) {
        return E_NOT_OK;
    }
    
    /* Clear context first */
    for (i = 0U; i < S32K312_HSM_AES_KEY_SIZE_256; i++) {
        context->key[i] = 0U;
    }
    for (i = 0U; i < S32K312_HSM_AES_IV_SIZE; i++) {
        context->iv[i] = 0U;
    }
    
    /* Copy key */
    for (i = 0U; i < keyLength; i++) {
        context->key[i] = key[i];
    }
    context->keyLength = keyLength;
    context->mode = mode;
    context->keyLoaded = TRUE;
    
    /* Copy IV if needed (not for ECB) */
    if ((mode != S32K312_HSM_AES_MODE_ECB) && (iv != NULL_PTR)) {
        for (i = 0U; i < S32K312_HSM_AES_IV_SIZE; i++) {
            context->iv[i] = iv[i];
        }
    }
    
    return E_OK;
}

/**********************************************************************************************************************
 * S32K312_Hsm_AesEcbEncrypt
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_AesEcbEncrypt(const S32K312_HsmAesContextType* context,
                                          const uint8* plaintext,
                                          uint8* ciphertext,
                                          uint32 length)
{
    Std_ReturnType result = E_NOT_OK;
    uint32 i;
    uint32 blocks;
    uint32 ctrlReg;
    
    if ((context == NULL_PTR) || (plaintext == NULL_PTR) || (ciphertext == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if (!context->keyLoaded) {
        return E_NOT_OK;
    }
    
    if ((length == 0U) || ((length % S32K312_HSM_AES_BLOCK_SIZE) != 0U)) {
        S32K312_Hsm_ReportError(S32K312_HSM_SID_AES_ENCRYPT, S32K312_HSM_ERROR_INVALID_PARAM);
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_State != S32K312_HSM_STATE_READY) {
        return E_NOT_OK;
    }
    
    /* Check AES availability */
    if ((S32K312_Hsm_ConfigPtr == NULL_PTR) || (!S32K312_Hsm_ConfigPtr->enableAes)) {
        return E_NOT_OK;
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_BUSY;
    
    blocks = length / S32K312_HSM_AES_BLOCK_SIZE;
    
    /* Setup control register */
    ctrlReg = S32K312_HSM_AES_CTRL_ENABLE | S32K312_HSM_AES_CTRL_MODE_ECB | 
              S32K312_HSM_AES_CTRL_ENCRYPT;
    
    if (context->keyLength == S32K312_HSM_AES_KEY_SIZE_256) {
        ctrlReg |= S32K312_HSM_AES_CTRL_KEYLEN_256;
    }
    
    /* Process each block */
    for (i = 0U; i < blocks; i++) {
        uint32 blockOffset = i * S32K312_HSM_AES_BLOCK_SIZE;
        uint32 j;
        
        /* Wait for AES module ready */
        result = S32K312_Hsm_WaitForAesReady(S32K312_HSM_TIMEOUT_AES);
        if (result != E_OK) {
            S32K312_Hsm_LastError = S32K312_HSM_ERROR_TIMEOUT;
            break;
        }
        
        /* Load key (only needed for first block or key change) */
        if (S32K312_Hsm_AesRegs != NULL_PTR) {
            uint32 wordCount = context->keyLength / 4U;
            for (j = 0U; j < wordCount; j++) {
                S32K312_Hsm_AesRegs->KEY[j] = 
                    ((uint32)context->key[j * 4U] << 24) |
                    ((uint32)context->key[j * 4U + 1U] << 16) |
                    ((uint32)context->key[j * 4U + 2U] << 8) |
                    (uint32)context->key[j * 4U + 3U];
            }
            
            /* Load plaintext data */
            for (j = 0U; j < 4U; j++) {
                S32K312_Hsm_AesRegs->DATA_IN[j] =
                    ((uint32)plaintext[blockOffset + j * 4U] << 24) |
                    ((uint32)plaintext[blockOffset + j * 4U + 1U] << 16) |
                    ((uint32)plaintext[blockOffset + j * 4U + 2U] << 8) |
                    (uint32)plaintext[blockOffset + j * 4U + 3U];
            }
            
            /* Start operation */
            S32K312_Hsm_AesRegs->CTRL = ctrlReg | S32K312_HSM_AES_CTRL_START;
            
            /* Wait for completion */
            result = S32K312_Hsm_WaitForAesReady(S32K312_HSM_TIMEOUT_AES);
            if (result != E_OK) {
                S32K312_Hsm_LastError = S32K312_HSM_ERROR_TIMEOUT;
                break;
            }
            
            /* Check for error */
            if ((S32K312_Hsm_AesRegs->STATUS & S32K312_HSM_AES_CTRL_ERROR) != 0U) {
                S32K312_Hsm_LastError = S32K312_HSM_ERROR_HARDWARE;
                result = E_NOT_OK;
                break;
            }
            
            /* Read ciphertext */
            for (j = 0U; j < 4U; j++) {
                uint32 data = S32K312_Hsm_AesRegs->DATA_OUT[j];
                ciphertext[blockOffset + j * 4U] = (uint8)(data >> 24);
                ciphertext[blockOffset + j * 4U + 1U] = (uint8)(data >> 16);
                ciphertext[blockOffset + j * 4U + 2U] = (uint8)(data >> 8);
                ciphertext[blockOffset + j * 4U + 3U] = (uint8)(data);
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
 * S32K312_Hsm_AesEcbDecrypt
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_AesEcbDecrypt(const S32K312_HsmAesContextType* context,
                                          const uint8* ciphertext,
                                          uint8* plaintext,
                                          uint32 length)
{
    Std_ReturnType result = E_NOT_OK;
    uint32 i;
    uint32 blocks;
    uint32 ctrlReg;
    
    if ((context == NULL_PTR) || (ciphertext == NULL_PTR) || (plaintext == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if (!context->keyLoaded) {
        return E_NOT_OK;
    }
    
    if ((length == 0U) || ((length % S32K312_HSM_AES_BLOCK_SIZE) != 0U)) {
        S32K312_Hsm_ReportError(S32K312_HSM_SID_AES_DECRYPT, S32K312_HSM_ERROR_INVALID_PARAM);
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_State != S32K312_HSM_STATE_READY) {
        return E_NOT_OK;
    }
    
    if ((S32K312_Hsm_ConfigPtr == NULL_PTR) || (!S32K312_Hsm_ConfigPtr->enableAes)) {
        return E_NOT_OK;
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_BUSY;
    
    blocks = length / S32K312_HSM_AES_BLOCK_SIZE;
    
    /* Setup control register for decryption */
    ctrlReg = S32K312_HSM_AES_CTRL_ENABLE | S32K312_HSM_AES_CTRL_MODE_ECB | 
              S32K312_HSM_AES_CTRL_DECRYPT;
    
    if (context->keyLength == S32K312_HSM_AES_KEY_SIZE_256) {
        ctrlReg |= S32K312_HSM_AES_CTRL_KEYLEN_256;
    }
    
    /* Process each block */
    for (i = 0U; i < blocks; i++) {
        uint32 blockOffset = i * S32K312_HSM_AES_BLOCK_SIZE;
        uint32 j;
        
        result = S32K312_Hsm_WaitForAesReady(S32K312_HSM_TIMEOUT_AES);
        if (result != E_OK) {
            S32K312_Hsm_LastError = S32K312_HSM_ERROR_TIMEOUT;
            break;
        }
        
        if (S32K312_Hsm_AesRegs != NULL_PTR) {
            /* Load key */
            uint32 wordCount = context->keyLength / 4U;
            for (j = 0U; j < wordCount; j++) {
                S32K312_Hsm_AesRegs->KEY[j] = 
                    ((uint32)context->key[j * 4U] << 24) |
                    ((uint32)context->key[j * 4U + 1U] << 16) |
                    ((uint32)context->key[j * 4U + 2U] << 8) |
                    (uint32)context->key[j * 4U + 3U];
            }
            
            /* Load ciphertext */
            for (j = 0U; j < 4U; j++) {
                S32K312_Hsm_AesRegs->DATA_IN[j] =
                    ((uint32)ciphertext[blockOffset + j * 4U] << 24) |
                    ((uint32)ciphertext[blockOffset + j * 4U + 1U] << 16) |
                    ((uint32)ciphertext[blockOffset + j * 4U + 2U] << 8) |
                    (uint32)ciphertext[blockOffset + j * 4U + 3U];
            }
            
            /* Start operation */
            S32K312_Hsm_AesRegs->CTRL = ctrlReg | S32K312_HSM_AES_CTRL_START;
            
            result = S32K312_Hsm_WaitForAesReady(S32K312_HSM_TIMEOUT_AES);
            if (result != E_OK) {
                S32K312_Hsm_LastError = S32K312_HSM_ERROR_TIMEOUT;
                break;
            }
            
            /* Check for error */
            if ((S32K312_Hsm_AesRegs->STATUS & S32K312_HSM_AES_CTRL_ERROR) != 0U) {
                S32K312_Hsm_LastError = S32K312_HSM_ERROR_HARDWARE;
                result = E_NOT_OK;
                break;
            }
            
            /* Read plaintext */
            for (j = 0U; j < 4U; j++) {
                uint32 data = S32K312_Hsm_AesRegs->DATA_OUT[j];
                plaintext[blockOffset + j * 4U] = (uint8)(data >> 24);
                plaintext[blockOffset + j * 4U + 1U] = (uint8)(data >> 16);
                plaintext[blockOffset + j * 4U + 2U] = (uint8)(data >> 8);
                plaintext[blockOffset + j * 4U + 3U] = (uint8)(data);
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
 * S32K312_Hsm_AesCbcEncrypt
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_AesCbcEncrypt(const S32K312_HsmAesContextType* context,
                                          const uint8* plaintext,
                                          uint8* ciphertext,
                                          uint32 length)
{
    /* For hardware implementation, CBC is similar to ECB with IV handling */
    /* In a real implementation, the hardware would handle the chaining */
    /* This is a simplified implementation */
    
    if ((context == NULL_PTR) || (plaintext == NULL_PTR) || (ciphertext == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if ((length == 0U) || ((length % S32K312_HSM_AES_BLOCK_SIZE) != 0U)) {
        return E_NOT_OK;
    }
    
    /* CBC mode requires IV */
    if (context->mode != S32K312_HSM_AES_MODE_CBC) {
        return E_NOT_OK;
    }
    
    /* Use hardware CBC mode if available, otherwise use ECB with software chaining */
    /* This stub uses ECB as the underlying primitive */
    return S32K312_Hsm_AesEcbEncrypt(context, plaintext, ciphertext, length);
}

/**********************************************************************************************************************
 * S32K312_Hsm_AesCbcDecrypt
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_AesCbcDecrypt(const S32K312_HsmAesContextType* context,
                                          const uint8* ciphertext,
                                          uint8* plaintext,
                                          uint32 length)
{
    if ((context == NULL_PTR) || (ciphertext == NULL_PTR) || (plaintext == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if ((length == 0U) || ((length % S32K312_HSM_AES_BLOCK_SIZE) != 0U)) {
        return E_NOT_OK;
    }
    
    if (context->mode != S32K312_HSM_AES_MODE_CBC) {
        return E_NOT_OK;
    }
    
    return S32K312_Hsm_AesEcbDecrypt(context, ciphertext, plaintext, length);
}

/**********************************************************************************************************************
 * S32K312_Hsm_AesGcmEncrypt
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_AesGcmEncrypt(const S32K312_HsmAesContextType* context,
                                          const uint8* plaintext,
                                          uint32 plaintextLength,
                                          const uint8* aad,
                                          uint32 aadLength,
                                          const uint8* iv,
                                          uint8* ciphertext,
                                          uint8* tag)
{
    Std_ReturnType result = E_NOT_OK;
    uint32 ctrlReg;
    uint32 i;
    
    if ((context == NULL_PTR) || (iv == NULL_PTR) || (tag == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if ((plaintext == NULL_PTR) && (plaintextLength > 0U)) {
        return E_NOT_OK;
    }
    
    if (!context->keyLoaded) {
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_State != S32K312_HSM_STATE_READY) {
        return E_NOT_OK;
    }
    
    if ((S32K312_Hsm_ConfigPtr == NULL_PTR) || (!S32K312_Hsm_ConfigPtr->enableAes)) {
        return E_NOT_OK;
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_BUSY;
    
    /* Setup control register for GCM encryption */
    ctrlReg = S32K312_HSM_AES_CTRL_ENABLE | S32K312_HSM_AES_CTRL_MODE_GCM | 
              S32K312_HSM_AES_CTRL_ENCRYPT;
    
    if (context->keyLength == S32K312_HSM_AES_KEY_SIZE_256) {
        ctrlReg |= S32K312_HSM_AES_CTRL_KEYLEN_256;
    }
    
    result = S32K312_Hsm_WaitForAesReady(S32K312_HSM_TIMEOUT_AES);
    if (result == E_OK) {
        if (S32K312_Hsm_AesRegs != NULL_PTR) {
            /* Load key */
            uint32 wordCount = context->keyLength / 4U;
            for (i = 0U; i < wordCount; i++) {
                S32K312_Hsm_AesRegs->KEY[i] = 
                    ((uint32)context->key[i * 4U] << 24) |
                    ((uint32)context->key[i * 4U + 1U] << 16) |
                    ((uint32)context->key[i * 4U + 2U] << 8) |
                    (uint32)context->key[i * 4U + 3U];
            }
            
            /* Load IV (96 bits for GCM) */
            for (i = 0U; i < 3U; i++) {
                S32K312_Hsm_AesRegs->IV[i] =
                    ((uint32)iv[i * 4U] << 24) |
                    ((uint32)iv[i * 4U + 1U] << 16) |
                    ((uint32)iv[i * 4U + 2U] << 8) |
                    (uint32)iv[i * 4U + 3U];
            }
            S32K312_Hsm_AesRegs->IV[3] = 0x00000001U; /* Counter initial value */
            
            /* Process AAD if present */
            if ((aad != NULL_PTR) && (aadLength > 0U)) {
                S32K312_Hsm_AesRegs->AAD_LEN = aadLength;
                /* AAD processing would happen here in full implementation */
            }
            
            /* Process plaintext if present */
            if ((plaintext != NULL_PTR) && (plaintextLength > 0U)) {
                uint32 blocks = plaintextLength / S32K312_HSM_AES_BLOCK_SIZE;
                uint32 j;
                
                for (j = 0U; j < blocks; j++) {
                    uint32 offset = j * S32K312_HSM_AES_BLOCK_SIZE;
                    uint32 k;
                    
                    for (k = 0U; k < 4U; k++) {
                        S32K312_Hsm_AesRegs->DATA_IN[k] =
                            ((uint32)plaintext[offset + k * 4U] << 24) |
                            ((uint32)plaintext[offset + k * 4U + 1U] << 16) |
                            ((uint32)plaintext[offset + k * 4U + 2U] << 8) |
                            (uint32)plaintext[offset + k * 4U + 3U];
                    }
                    
                    S32K312_Hsm_AesRegs->CTRL = ctrlReg | S32K312_HSM_AES_CTRL_START;
                    
                    result = S32K312_Hsm_WaitForAesReady(S32K312_HSM_TIMEOUT_AES);
                    if (result != E_OK) {
                        break;
                    }
                    
                    for (k = 0U; k < 4U; k++) {
                        uint32 data = S32K312_Hsm_AesRegs->DATA_OUT[k];
                        ciphertext[offset + k * 4U] = (uint8)(data >> 24);
                        ciphertext[offset + k * 4U + 1U] = (uint8)(data >> 16);
                        ciphertext[offset + k * 4U + 2U] = (uint8)(data >> 8);
                        ciphertext[offset + k * 4U + 3U] = (uint8)(data);
                    }
                }
            }
            
            /* Read authentication tag */
            if (result == E_OK) {
                for (i = 0U; i < 4U; i++) {
                    uint32 data = S32K312_Hsm_AesRegs->TAG_OUT[i];
                    tag[i * 4U] = (uint8)(data >> 24);
                    tag[i * 4U + 1U] = (uint8)(data >> 16);
                    tag[i * 4U + 2U] = (uint8)(data >> 8);
                    tag[i * 4U + 3U] = (uint8)(data);
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
 * S32K312_Hsm_AesGcmDecrypt
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_AesGcmDecrypt(const S32K312_HsmAesContextType* context,
                                          const uint8* ciphertext,
                                          uint32 ciphertextLength,
                                          const uint8* aad,
                                          uint32 aadLength,
                                          const uint8* iv,
                                          const uint8* tag,
                                          uint8* plaintext)
{
    /* GCM decrypt is similar to encrypt with tag verification */
    /* Full implementation would verify the tag after decryption */
    
    uint8 computedTag[S32K312_HSM_AES_GCM_TAG_SIZE];
    Std_ReturnType result;
    uint32 i;
    
    result = S32K312_Hsm_AesGcmEncrypt(context, ciphertext, ciphertextLength,
                                        aad, aadLength, iv, plaintext, computedTag);
    
    if (result == E_OK) {
        /* Verify tag */
        for (i = 0U; i < S32K312_HSM_AES_GCM_TAG_SIZE; i++) {
            if (computedTag[i] != tag[i]) {
                result = E_NOT_OK;
                break;
            }
        }
    }
    
    /* Clear computed tag */
    for (i = 0U; i < S32K312_HSM_AES_GCM_TAG_SIZE; i++) {
/*         computedTag[i] = 0U; */
    }
    
    return result;
}

/**********************************************************************************************************************
 * GLOBAL FUNCTIONS - ECC OPERATIONS
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * S32K312_Hsm_EccInit
 *********************************************************************************************************************/
#define CRYPTO_STOP_SEC_CODE
#include "MemMap.h"
