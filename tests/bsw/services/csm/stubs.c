/**
 * @file stubs.c
 * @brief Dependency stubs for the Csm unit test.
 *
 * The production Csm.c references these symbols:
 *  - CryIf_*      : hardware crypto backend (other unit under test)
 *  - Csm_Cfg_*    : config/hw-service layer (Csm_Cfg.c, needs mbedTLS +
 *                   the mcal_crypto pool which cannot build with
 *                   YULE_ENABLE_MCAL=OFF -- pre-existing src/ defect)
 * Minimal implementations keep the test executable linkable while the
 * SUT under test remains the unmodified production Csm.c.
 *
 * Each stub has a controllable return value and optional behavior callback
 * so tests can exercise both success and failure branches.
 */

#include "Std_Types.h"
#include "CryIf.h"
#include "Csm_Cfg.h"
#include <string.h>

/* ---- Controllable stub state ---- */

/* CryIf_KeyGenerate */
Std_ReturnType stub_CryIf_KeyGenerate_ret = E_NOT_OK;
Std_ReturnType CryIf_KeyGenerate(CryIf_KeyIdType cryIfKeyId) {
    (void)cryIfKeyId;
    return stub_CryIf_KeyGenerate_ret;
}

/* CryIf_KeyDerive */
Std_ReturnType stub_CryIf_KeyDerive_ret = E_NOT_OK;
Std_ReturnType CryIf_KeyDerive(
    CryIf_KeyIdType cryIfKeyId,
    CryIf_KeyIdType targetCryIfKeyId) {
    (void)cryIfKeyId;
    (void)targetCryIfKeyId;
    return stub_CryIf_KeyDerive_ret;
}

/* CryIf_KeyExchangeCalcPubValue */
Std_ReturnType stub_CryIf_KeyExchangeCalcPubValue_ret = E_NOT_OK;
uint8 stub_CryIf_KeyExchangeCalcPubValue_data[64];
uint32 stub_CryIf_KeyExchangeCalcPubValue_len = 0;
Std_ReturnType CryIf_KeyExchangeCalcPubValue(
    CryIf_KeyIdType cryIfKeyId,
    uint8* publicValuePtr,
    uint32* publicValueLengthPtr) {
    (void)cryIfKeyId;
    if (stub_CryIf_KeyExchangeCalcPubValue_ret == E_OK &&
        publicValuePtr != NULL && publicValueLengthPtr != NULL) {
        uint32 copyLen = stub_CryIf_KeyExchangeCalcPubValue_len;
        if (copyLen > *publicValueLengthPtr) copyLen = *publicValueLengthPtr;
        memcpy(publicValuePtr, stub_CryIf_KeyExchangeCalcPubValue_data, copyLen);
        *publicValueLengthPtr = copyLen;
    }
    return stub_CryIf_KeyExchangeCalcPubValue_ret;
}

/* CryIf_KeyExchangeCalcSecret */
Std_ReturnType stub_CryIf_KeyExchangeCalcSecret_ret = E_NOT_OK;
Std_ReturnType CryIf_KeyExchangeCalcSecret(
    CryIf_KeyIdType cryIfKeyId,
    const uint8* partnerPublicValuePtr,
    uint32 partnerPublicValueLength) {
    (void)cryIfKeyId;
    (void)partnerPublicValuePtr;
    (void)partnerPublicValueLength;
    return stub_CryIf_KeyExchangeCalcSecret_ret;
}

/* ---- Csm_Cfg backend stubs ---- */

/* Csm_Cfg_KeyWrite */
Std_ReturnType stub_Csm_Cfg_KeyWrite_ret = E_NOT_OK;
Std_ReturnType Csm_Cfg_KeyWrite(
    uint32 keyId,
    uint32 elementId,
    const uint8* data,
    uint32 length) {
    (void)keyId; (void)elementId; (void)data; (void)length;
    return stub_Csm_Cfg_KeyWrite_ret;
}

/* Csm_Cfg_KeyRead */
Std_ReturnType stub_Csm_Cfg_KeyRead_ret = E_NOT_OK;
Std_ReturnType Csm_Cfg_KeyRead(
    uint32 keyId,
    uint32 elementId,
    uint8* data,
    uint32* length) {
    (void)keyId; (void)elementId; (void)data; (void)length;
    return stub_Csm_Cfg_KeyRead_ret;
}

/* Csm_Cfg_HwService - the main crypto service dispatch stub.
 * Configurable return value, output data, and output length.
 * Also records the last call parameters for verification. */
Std_ReturnType stub_Csm_Cfg_HwService_ret = E_NOT_OK;
uint8 stub_Csm_Cfg_HwService_output[256];
uint32 stub_Csm_Cfg_HwService_outputLen = 0;
Csm_ServiceType stub_Csm_Cfg_HwService_lastService = CSM_SERVICE_NONE;
uint32 stub_Csm_Cfg_HwService_lastJobId = 0;
uint32 stub_Csm_Cfg_HwService_callCount = 0;
boolean stub_Csm_Cfg_HwService_verifyResult = FALSE;

Std_ReturnType Csm_Cfg_HwService(
    uint32 jobId,
    Csm_ServiceType serviceType,
    const uint8* input,
    uint32 inputLength,
    uint8* output,
    uint32* outputLength) {
    (void)input; (void)inputLength;
    stub_Csm_Cfg_HwService_lastJobId = jobId;
    stub_Csm_Cfg_HwService_lastService = serviceType;
    stub_Csm_Cfg_HwService_callCount++;

    if (stub_Csm_Cfg_HwService_ret == E_OK && output != NULL && outputLength != NULL) {
        uint32 len = stub_Csm_Cfg_HwService_outputLen;
        uint32 copyLen = (len < *outputLength) ? len : *outputLength;
        memcpy(output, stub_Csm_Cfg_HwService_output, copyLen);
        *outputLength = len;  /* Report actual required length, not capped */
    }
    return stub_Csm_Cfg_HwService_ret;
}

/* Csm_Cfg_RandomGenerate */
Std_ReturnType stub_Csm_Cfg_RandomGenerate_ret = E_NOT_OK;
uint8 stub_Csm_Cfg_RandomGenerate_data[256];
Std_ReturnType Csm_Cfg_RandomGenerate(
    uint8* data,
    uint32 length) {
    if (stub_Csm_Cfg_RandomGenerate_ret == E_OK && data != NULL) {
        /* Fill with deterministic pattern for testability */
        uint32 i;
        for (i = 0; i < length; i++) {
            data[i] = stub_Csm_Cfg_RandomGenerate_data[i % 256];
        }
    }
    return stub_Csm_Cfg_RandomGenerate_ret;
}

uint32 Csm_Cfg_GetTimestamp(void) {
    return 0U;
}

/* Helper to reset all stubs to defaults */
void stubs_reset(void) {
    stub_CryIf_KeyGenerate_ret = E_NOT_OK;
    stub_CryIf_KeyDerive_ret = E_NOT_OK;
    stub_CryIf_KeyExchangeCalcPubValue_ret = E_NOT_OK;
    stub_CryIf_KeyExchangeCalcPubValue_len = 0;
    stub_CryIf_KeyExchangeCalcSecret_ret = E_NOT_OK;
    stub_Csm_Cfg_KeyWrite_ret = E_NOT_OK;
    stub_Csm_Cfg_KeyRead_ret = E_NOT_OK;
    stub_Csm_Cfg_HwService_ret = E_NOT_OK;
    stub_Csm_Cfg_HwService_outputLen = 0;
    stub_Csm_Cfg_HwService_lastService = CSM_SERVICE_NONE;
    stub_Csm_Cfg_HwService_lastJobId = 0;
    stub_Csm_Cfg_HwService_callCount = 0;
    stub_Csm_Cfg_HwService_verifyResult = FALSE;
    stub_Csm_Cfg_RandomGenerate_ret = E_NOT_OK;
    memset(stub_Csm_Cfg_HwService_output, 0, sizeof(stub_Csm_Cfg_HwService_output));
    memset(stub_Csm_Cfg_RandomGenerate_data, 0xAB, sizeof(stub_Csm_Cfg_RandomGenerate_data));
}
