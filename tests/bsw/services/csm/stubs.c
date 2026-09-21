/**
 * @file stubs.c
 * @brief Dependency stubs for the Csm unit test.
 *
 * The production Csm.c references these symbols:
 *  - CryIf_*      : hardware crypto backend (other unit under test)
 *  - Csm_Cfg_*    : config/hw-service layer (Csm_Cfg.c, needs mbedTLS +
 *                   the mcal_crypto pool which cannot build with
 *                   YULE_ENABLE_MCAL=OFF — pre-existing src/ defect)
 * Minimal implementations keep the test executable linkable while the
 * SUT under test remains the unmodified production Csm.c.
 */

#include "Std_Types.h"
#include "CryIf.h"
#include "Csm_Cfg.h"

/* ---- CryIf hardware backend stubs ---- */

Std_ReturnType CryIf_KeyGenerate(CryIf_KeyIdType cryIfKeyId) {
    (void)cryIfKeyId;
    return E_NOT_OK;
}

Std_ReturnType CryIf_KeyDerive(
    CryIf_KeyIdType cryIfKeyId,
    CryIf_KeyIdType targetCryIfKeyId) {
    (void)cryIfKeyId;
    (void)targetCryIfKeyId;
    return E_NOT_OK;
}

Std_ReturnType CryIf_KeyExchangeCalcPubValue(
    CryIf_KeyIdType cryIfKeyId,
    uint8* publicValuePtr,
    uint32* publicValueLengthPtr) {
    (void)cryIfKeyId;
    (void)publicValuePtr;
    (void)publicValueLengthPtr;
    return E_NOT_OK;
}

Std_ReturnType CryIf_KeyExchangeCalcSecret(
    CryIf_KeyIdType cryIfKeyId,
    const uint8* partnerPublicValuePtr,
    uint32 partnerPublicValueLength) {
    (void)cryIfKeyId;
    (void)partnerPublicValuePtr;
    (void)partnerPublicValueLength;
    return E_NOT_OK;
}

/* ---- Csm_Cfg backend stubs ---- */

Std_ReturnType Csm_Cfg_KeyWrite(
    uint32 keyId,
    uint32 elementId,
    const uint8* data,
    uint32 length) {
    (void)keyId; (void)elementId; (void)data; (void)length;
    return E_NOT_OK;
}

Std_ReturnType Csm_Cfg_KeyRead(
    uint32 keyId,
    uint32 elementId,
    uint8* data,
    uint32* length) {
    (void)keyId; (void)elementId; (void)data; (void)length;
    return E_NOT_OK;
}

Std_ReturnType Csm_Cfg_HwService(
    uint32 jobId,
    Csm_ServiceType serviceType,
    const uint8* input,
    uint32 inputLength,
    uint8* output,
    uint32* outputLength) {
    (void)jobId; (void)serviceType; (void)input; (void)inputLength;
    (void)output; (void)outputLength;
    return E_NOT_OK;
}

Std_ReturnType Csm_Cfg_RandomGenerate(
    uint8* data,
    uint32 length) {
    (void)data; (void)length;
    return E_NOT_OK;
}

uint32 Csm_Cfg_GetTimestamp(void) {
    return 0U;
}
