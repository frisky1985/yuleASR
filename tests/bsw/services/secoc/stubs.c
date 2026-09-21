/**
 * @file stubs.c
 * @brief Dependency stubs for the SecOC unit test.
 *
 * SecOC.c references these symbols; minimal implementations keep the
 * test executable linkable without pulling in PduR/Csm/Mcal libraries.
 */

#include "Std_Types.h"
#include "ComStack_Types.h"

void Mcal_DisableAllInterrupts(void) {}
void Mcal_EnableAllInterrupts(void) {}

Std_ReturnType PduR_SecOCTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr) {
    (void)TxPduId;
    (void)PduInfoPtr;
    return E_OK;
}

void PduR_SecOCRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr) {
    (void)RxPduId;
    (void)PduInfoPtr;
}

Std_ReturnType Csm_MacGenerate(
    uint32 jobId,
    uint8 mode,
    const uint8* dataPtr,
    uint32 dataLength,
    uint8* macPtr,
    uint32* macLengthPtr) {
    (void)jobId; (void)mode; (void)dataPtr; (void)dataLength;
    (void)macPtr; (void)macLengthPtr;
    return E_OK;
}

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
    if (verifyPtr != NULL_PTR) {
        *verifyPtr = TRUE;
    }
    return E_OK;
}
