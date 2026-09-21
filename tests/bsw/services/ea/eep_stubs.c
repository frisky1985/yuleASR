/**
 * @file eep_stubs.c
 * @brief EEPROM driver stubs shared by the Ea unit tests (ea_test, ea_svc_test).
 *
 * Records every call the SUT (Ea.c) makes into the EEPROM driver API and lets
 * each test control the reported job result so the Ea_MainFunction() job
 * lifecycle can be driven deterministically.
 */

#include "Std_Types.h"
#include "MemIf.h"

/* ---- call recorders ---- */
uint32 stub_Eep_Read_calls = 0u;
uint32 stub_Eep_Write_calls = 0u;
uint32 stub_Eep_Erase_calls = 0u;
uint32 stub_Eep_Cancel_calls = 0u;
uint32 stub_Eep_GetJobResult_calls = 0u;

uint32 stub_Eep_last_Address = 0u;
uint16 stub_Eep_last_Length = 0u;
uint8* stub_Eep_last_ReadPtr = (uint8*)0;
const uint8* stub_Eep_last_WritePtr = (const uint8*)0;

/* ---- controllable return values ---- */
Std_ReturnType stub_Eep_Read_ret = E_OK;
Std_ReturnType stub_Eep_Write_ret = E_OK;
Std_ReturnType stub_Eep_Erase_ret = E_OK;
MemIf_JobResultType stub_Eep_JobResult = MEMIF_JOB_OK;

Std_ReturnType Eep_Read(uint32 EepromAddress, uint8* DataBufferPtr, uint16 Length)
{
    stub_Eep_Read_calls++;
    stub_Eep_last_Address = EepromAddress;
    stub_Eep_last_ReadPtr = DataBufferPtr;
    stub_Eep_last_Length = Length;
    return stub_Eep_Read_ret;
}

Std_ReturnType Eep_Write(uint32 EepromAddress, const uint8* DataBufferPtr, uint16 Length)
{
    stub_Eep_Write_calls++;
    stub_Eep_last_Address = EepromAddress;
    stub_Eep_last_WritePtr = DataBufferPtr;
    stub_Eep_last_Length = Length;
    return stub_Eep_Write_ret;
}

Std_ReturnType Eep_Erase(uint32 EepromAddress, uint16 Length)
{
    stub_Eep_Erase_calls++;
    stub_Eep_last_Address = EepromAddress;
    stub_Eep_last_Length = Length;
    return stub_Eep_Erase_ret;
}

MemIf_StatusType Eep_GetStatus(void)
{
    return MEMIF_IDLE;
}

/* ABI note: Ea.c compiles against the ECUAL MemIf.h
 * (src/bsw/ecual/memif/include/MemIf.h) whose MemIf_JobResultType numeric
 * values are OK=0, FAILED=1, PENDING=2, CANCELED=3 — a different order from
 * the services MemIf.h used here (OK=0, PENDING=1, CANCELED=2, FAILED=3).
 * The tests drive stub_Eep_JobResult with services-view macros, so translate
 * to the SUT's view before returning. */
MemIf_JobResultType Eep_GetJobResult(void)
{
    static const uint8 svcToSut[4] = { 0u /*OK->OK*/, 2u /*PENDING->PENDING*/,
                                       3u /*CANCELED->CANCELED*/, 1u /*FAILED->FAILED*/ };
    stub_Eep_GetJobResult_calls++;
    return (MemIf_JobResultType)svcToSut[(uint32)stub_Eep_JobResult & 3u];
}

void Eep_Cancel(void)
{
    stub_Eep_Cancel_calls++;
}
