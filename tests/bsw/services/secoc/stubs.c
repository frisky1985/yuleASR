/**
 * @file stubs.c
 * @brief Controllable dependency stubs for the SecOC unit test.
 */

#include "Std_Types.h"
#include "ComStack_Types.h"
#include <string.h>

void Mcal_DisableAllInterrupts(void) {}
void Mcal_EnableAllInterrupts(void) {}

/* --- PduR stubs --- */
Std_ReturnType PduR_SecOCTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr) {
    (void)TxPduId;
    (void)PduInfoPtr;
    return E_OK;
}

void PduR_SecOCRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr) {
    (void)RxPduId;
    (void)PduInfoPtr;
}

/* --- Csm stubs with controllable behavior --- */
Std_ReturnType stub_CsmMacGenerateReturn = E_OK;
uint32 Csm_MacGenerate_callCount = 0;

Std_ReturnType Csm_MacGenerate(
    uint32 jobId,
    uint8 mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* macPtr,
    uint32* macLengthPtr) {
    (void)jobId; (void)mode; (void)dataPtr; (void)dataLength;
    Csm_MacGenerate_callCount++;
    if (macPtr != NULL_PTR && macLengthPtr != NULL_PTR) {
        memset(macPtr, 0xAA, *macLengthPtr);
    }
    return stub_CsmMacGenerateReturn;
}

Std_ReturnType stub_CsmMacVerifyReturn = E_OK;
uint8 stub_CsmMacVerifyResult = 0U; /* CSM_E_VER_OK */
uint32 Csm_MacVerify_callCount = 0;

Std_ReturnType Csm_MacVerify(
    uint32 jobId,
    uint8 mode,
    const uint8* dataPtr,
    uint32 dataLength,
    const uint8* macPtr,
    uint32 macLength,
    boolean* verifyPtr) {
    (void)jobId; (void)mode; (void)dataPtr; (void)dataLength;
    (void)macPtr; (void)macLength;
    Csm_MacVerify_callCount++;
    if (verifyPtr != NULL_PTR) {
        *verifyPtr = stub_CsmMacVerifyResult;
    }
    return stub_CsmMacVerifyReturn;
}

/* --- NvM stubs with controllable behavior --- */
Std_ReturnType stub_NvMReadBlockReturn = E_NOT_OK;
uint8 stub_NvMReadBlockData[256];
uint16 stub_NvMReadBlockDataLen = 0;
uint32 NvM_ReadBlock_callCount = 0;

Std_ReturnType NvM_ReadBlock(uint16 BlockId, void* DstPtr) {
    (void)BlockId;
    NvM_ReadBlock_callCount++;
    if (stub_NvMReadBlockReturn == E_OK && DstPtr != NULL_PTR && stub_NvMReadBlockDataLen > 0) {
        memcpy(DstPtr, stub_NvMReadBlockData, stub_NvMReadBlockDataLen);
    }
    return stub_NvMReadBlockReturn;
}

uint32 NvM_WriteBlock_callCount = 0;
Std_ReturnType stub_NvMWriteBlockReturn = E_OK;

Std_ReturnType NvM_WriteBlock(uint16 BlockId, const void* SrcPtr) {
    (void)BlockId;
    (void)SrcPtr;
    NvM_WriteBlock_callCount++;
    return stub_NvMWriteBlockReturn;
}
