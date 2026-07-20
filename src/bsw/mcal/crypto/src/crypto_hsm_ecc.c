/**********************************************************************************************************************
 * ECC 操作实现
 * 自动拆分自 Crypto_S32K312_Hsm.c
 *********************************************************************************************************************/
#define CRYPTO_START_SEC_CODE
#include "MemMap.h"

Std_ReturnType S32K312_Hsm_EccInit(S32K312_HsmEccContextType* context,
                                    S32K312_HsmEccCurveType curve)
{
    uint32 i;
    
    if (context == NULL_PTR) {
        return E_NOT_OK;
    }
    
    if ((curve != S32K312_HSM_ECC_CURVE_SECP256R1) && 
        (curve != S32K312_HSM_ECC_CURVE_SECP384R1)) {
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_State != S32K312_HSM_STATE_READY) {
        return E_NOT_OK;
    }
    
    /* Clear context */
    for (i = 0U; i < S32K312_HSM_ECC_P384_KEY_SIZE; i++) {
        context->privateKey[i] = 0U;
    }
    for (i = 0U; i < S32K312_HSM_ECC_P384_POINT_SIZE; i++) {
        context->publicKey[i] = 0U;
    }
    
    context->curve = curve;
    context->keyLength = (curve == S32K312_HSM_ECC_CURVE_SECP256R1) ? 
                         S32K312_HSM_ECC_P256_KEY_SIZE : S32K312_HSM_ECC_P384_KEY_SIZE;
    context->keyLoaded = FALSE;
    
    return E_OK;
}

/**********************************************************************************************************************
 * S32K312_Hsm_EccLoadPrivateKey
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_EccLoadPrivateKey(S32K312_HsmEccContextType* context,
                                              const uint8* privateKey,
                                              uint32 keyLength)
{
    uint32 i;
    
    if ((context == NULL_PTR) || (privateKey == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if (keyLength != context->keyLength) {
        return E_NOT_OK;
    }
    
    /* Load private key */
    for (i = 0U; i < keyLength; i++) {
        context->privateKey[i] = privateKey[i];
    }
    
    context->keyLoaded = TRUE;
    return E_OK;
}

/**********************************************************************************************************************
 * S32K312_Hsm_EccLoadPublicKey
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_EccLoadPublicKey(S32K312_HsmEccContextType* context,
                                             const uint8* publicKey,
                                             uint32 keyLength)
{
    uint32 i;
    uint32 expectedLength;
    
    if ((context == NULL_PTR) || (publicKey == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    expectedLength = (context->curve == S32K312_HSM_ECC_CURVE_SECP256R1) ? 
                     S32K312_HSM_ECC_P256_POINT_SIZE : S32K312_HSM_ECC_P384_POINT_SIZE;
    
    if (keyLength != expectedLength) {
        return E_NOT_OK;
    }
    
    /* Load public key */
    for (i = 0U; i < keyLength; i++) {
        context->publicKey[i] = publicKey[i];
    }
    
    context->keyLoaded = TRUE;
    return E_OK;
}

/**********************************************************************************************************************
 * S32K312_Hsm_EccPointMultiply
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_EccPointMultiply(const S32K312_HsmEccContextType* context,
                                             uint8* resultPoint)
{
    Std_ReturnType result = E_NOT_OK;
    uint32 ctrlReg;
    uint32 i;
    uint32 wordCount;
    
    if ((context == NULL_PTR) || (resultPoint == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if (!context->keyLoaded) {
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_State != S32K312_HSM_STATE_READY) {
        return E_NOT_OK;
    }
    
    if ((S32K312_Hsm_ConfigPtr == NULL_PTR) || (!S32K312_Hsm_ConfigPtr->enableEcc)) {
        return E_NOT_OK;
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_BUSY;
    
    /* Setup control register */
    ctrlReg = S32K312_HSM_ECC_CTRL_ENABLE | S32K312_HSM_ECC_CTRL_OP_PMULT;
    if (context->curve == S32K312_HSM_ECC_CURVE_SECP384R1) {
        ctrlReg |= S32K312_HSM_ECC_CTRL_CURVE_P384;
    }
    
    result = S32K312_Hsm_WaitForEccReady(S32K312_HSM_TIMEOUT_ECC);
    if (result == E_OK) {
        if (S32K312_Hsm_EccRegs != NULL_PTR) {
            wordCount = context->keyLength / 4U;
            
            /* Load scalar (private key) */
            for (i = 0U; i < wordCount; i++) {
                S32K312_Hsm_EccRegs->SCALAR[i] =
                    ((uint32)context->privateKey[i * 4U] << 24) |
                    ((uint32)context->privateKey[i * 4U + 1U] << 16) |
                    ((uint32)context->privateKey[i * 4U + 2U] << 8) |
                    (uint32)context->privateKey[i * 4U + 3U];
            }
            
            /* Start point multiplication (uses generator point) */
            S32K312_Hsm_EccRegs->CTRL = ctrlReg | S32K312_HSM_ECC_CTRL_START;
            
            result = S32K312_Hsm_WaitForEccReady(S32K312_HSM_TIMEOUT_ECC);
            if (result == E_OK) {
                /* Check for error */
                if ((S32K312_Hsm_EccRegs->STATUS & S32K312_HSM_ECC_CTRL_ERROR) != 0U) {
                    S32K312_Hsm_LastError = S32K312_HSM_ERROR_HARDWARE;
                    result = E_NOT_OK;
                } else {
                    /* Read result point */
                    uint32 pointWords = wordCount * 2U; /* X and Y coordinates */
                    for (i = 0U; i < pointWords; i++) {
                        uint32 data = S32K312_Hsm_EccRegs->POINT_OUT[i];
                        resultPoint[i * 4U] = (uint8)(data >> 24);
                        resultPoint[i * 4U + 1U] = (uint8)(data >> 16);
                        resultPoint[i * 4U + 2U] = (uint8)(data >> 8);
                        resultPoint[i * 4U + 3U] = (uint8)(data);
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
 * S32K312_Hsm_EccSign
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_EccSign(const S32K312_HsmEccContextType* context,
                                    const uint8* digest,
                                    uint32 digestLength,
                                    uint8* signature,
                                    uint32* signatureLength)
{
    Std_ReturnType result = E_NOT_OK;
    uint32 ctrlReg;
    uint32 i;
    uint32 wordCount;
    
    if ((context == NULL_PTR) || (digest == NULL_PTR) || 
        (signature == NULL_PTR) || (signatureLength == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if (!context->keyLoaded) {
        return E_NOT_OK;
    }
    
    if (digestLength != S32K312_HSM_SHA256_DIGEST_SIZE) {
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_State != S32K312_HSM_STATE_READY) {
        return E_NOT_OK;
    }
    
    if ((S32K312_Hsm_ConfigPtr == NULL_PTR) || (!S32K312_Hsm_ConfigPtr->enableEcc)) {
        return E_NOT_OK;
    }
    
    S32K312_Hsm_State = S32K312_HSM_STATE_BUSY;
    
    /* Setup control register */
    ctrlReg = S32K312_HSM_ECC_CTRL_ENABLE | S32K312_HSM_ECC_CTRL_OP_SIGN;
    if (context->curve == S32K312_HSM_ECC_CURVE_SECP384R1) {
        ctrlReg |= S32K312_HSM_ECC_CTRL_CURVE_P384;
    }
    
    result = S32K312_Hsm_WaitForEccReady(S32K312_HSM_TIMEOUT_ECC);
    if (result == E_OK) {
        if (S32K312_Hsm_EccRegs != NULL_PTR) {
            wordCount = context->keyLength / 4U;
            
            /* Load private key */
            for (i = 0U; i < wordCount; i++) {
                S32K312_Hsm_EccRegs->SCALAR[i] =
                    ((uint32)context->privateKey[i * 4U] << 24) |
                    ((uint32)context->privateKey[i * 4U + 1U] << 16) |
                    ((uint32)context->privateKey[i * 4U + 2U] << 8) |
                    (uint32)context->privateKey[i * 4U + 3U];
            }
            
            /* Load hash */
            for (i = 0U; i < 8U; i++) {
                S32K312_Hsm_EccRegs->HASH[i] =
                    ((uint32)digest[i * 4U] << 24) |
                    ((uint32)digest[i * 4U + 1U] << 16) |
                    ((uint32)digest[i * 4U + 2U] << 8) |
                    (uint32)digest[i * 4U + 3U];
            }
            
            /* Start signing */
            S32K312_Hsm_EccRegs->CTRL = ctrlReg | S32K312_HSM_ECC_CTRL_START;
            
            result = S32K312_Hsm_WaitForEccReady(S32K312_HSM_TIMEOUT_ECC);
            if (result == E_OK) {
                if ((S32K312_Hsm_EccRegs->STATUS & S32K312_HSM_ECC_CTRL_ERROR) != 0U) {
                    S32K312_Hsm_LastError = S32K312_HSM_ERROR_HARDWARE;
                    result = E_NOT_OK;
                } else {
                    /* Read signature (R || S) */
                    for (i = 0U; i < wordCount; i++) {
                        uint32 dataR = S32K312_Hsm_EccRegs->SIG_R[i];
                        signature[i * 4U] = (uint8)(dataR >> 24);
                        signature[i * 4U + 1U] = (uint8)(dataR >> 16);
                        signature[i * 4U + 2U] = (uint8)(dataR >> 8);
                        signature[i * 4U + 3U] = (uint8)(dataR);
                        
                        uint32 dataS = S32K312_Hsm_EccRegs->SIG_S[i];
                        signature[context->keyLength + i * 4U] = (uint8)(dataS >> 24);
                        signature[context->keyLength + i * 4U + 1U] = (uint8)(dataS >> 16);
                        signature[context->keyLength + i * 4U + 2U] = (uint8)(dataS >> 8);
                        signature[context->keyLength + i * 4U + 3U] = (uint8)(dataS);
                    }
                    *signatureLength = context->keyLength * 2U;
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
 * S32K312_Hsm_EccVerify
 *********************************************************************************************************************/
Std_ReturnType S32K312_Hsm_EccVerify(const S32K312_HsmEccContextType* context,
                                      const uint8* digest,
                                      uint32 digestLength,
                                      const uint8* signature,
                                      uint32 signatureLength,
                                      Crypto_VerifyResultType* verifyResult)
{
    Std_ReturnType result = E_NOT_OK;
    uint32 ctrlReg;
    uint32 i;
    uint32 wordCount;
    
    if ((context == NULL_PTR) || (digest == NULL_PTR) || 
        (signature == NULL_PTR) || (verifyResult == NULL_PTR)) {
        return E_NOT_OK;
    }
    
    if (!context->keyLoaded) {
        return E_NOT_OK;
    }
    
    if ((digestLength != S32K312_HSM_SHA256_DIGEST_SIZE) ||
        (signatureLength != (context->keyLength * 2U))) {
        return E_NOT_OK;
    }
    
    if (S32K312_Hsm_State != S32K312_HSM_STATE_READY) {
        return E_NOT_OK;
    }
    
    if ((S32K312_Hsm_ConfigPtr == NULL_PTR) || (!S32K312_Hsm_ConfigPtr->enableEcc)) {
        return E_NOT_OK;
    }
    
    *verifyResult = CRYPTO_VERIFICATION_FAILED;
    S32K312_Hsm_State = S32K312_HSM_STATE_BUSY;
    
    /* Setup control register */
    ctrlReg = S32K312_HSM_ECC_CTRL_ENABLE | S32K312_HSM_ECC_CTRL_OP_VERIFY;
    if (context->curve == S32K312_HSM_ECC_CURVE_SECP384R1) {
        ctrlReg |= S32K312_HSM_ECC_CTRL_CURVE_P384;
    }
    
    result = S32K312_Hsm_WaitForEccReady(S32K312_HSM_TIMEOUT_ECC);
    if (result == E_OK) {
        if (S32K312_Hsm_EccRegs != NULL_PTR) {
            wordCount = context->keyLength / 4U;
            
            /* Load public key */
            for (i = 0U; i < (wordCount * 2U); i++) {
                S32K312_Hsm_EccRegs->POINT_IN[i] =
                    ((uint32)context->publicKey[i * 4U] << 24) |
                    ((uint32)context->publicKey[i * 4U + 1U] << 16) |
                    ((uint32)context->publicKey[i * 4U + 2U] << 8) |
                    (uint32)context->publicKey[i * 4U + 3U];
            }
            
            /* Load hash */
            for (i = 0U; i < 8U; i++) {
                S32K312_Hsm_EccRegs->HASH[i] =
                    ((uint32)digest[i * 4U] << 24) |
                    ((uint32)digest[i * 4U + 1U] << 16) |
                    ((uint32)digest[i * 4U + 2U] << 8) |
                    (uint32)digest[i * 4U + 3U];
            }
            
            /* Load signature */
            for (i = 0U; i < wordCount; i++) {
                S32K312_Hsm_EccRegs->SIG_R[i] =
                    ((uint32)signature[i * 4U] << 24) |
                    ((uint32)signature[i * 4U + 1U] << 16) |
                    ((uint32)signature[i * 4U + 2U] << 8) |
                    (uint32)signature[i * 4U + 3U];
                
                S32K312_Hsm_EccRegs->SIG_S[i] =
                    ((uint32)signature[context->keyLength + i * 4U] << 24) |
                    ((uint32)signature[context->keyLength + i * 4U + 1U] << 16) |
                    ((uint32)signature[context->keyLength + i * 4U + 2U] << 8) |
                    (uint32)signature[context->keyLength + i * 4U + 3U];
            }
            
            /* Start verification */
            S32K312_Hsm_EccRegs->CTRL = ctrlReg | S32K312_HSM_ECC_CTRL_START;
            
            result = S32K312_Hsm_WaitForEccReady(S32K312_HSM_TIMEOUT_ECC);
            if (result == E_OK) {
                if ((S32K312_Hsm_EccRegs->STATUS & S32K312_HSM_ECC_CTRL_ERROR) != 0U) {
                    *verifyResult = CRYPTO_VERIFICATION_FAILED;
                } else {
                    *verifyResult = CRYPTO_VERIFICATION_PASSED;
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
 * GLOBAL FUNCTIONS - SHA-256 OPERATIONS
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * S32K312_Hsm_Sha256Init
 *********************************************************************************************************************/
#define CRYPTO_STOP_SEC_CODE
#include "MemMap.h"
