/* asil_stubs.h — Native host stubs for ASIL coverage test binaries
 *
 * Provides host-side implementations of BSW layer dependencies that are
 * not linked into unit-test binaries (MemIf device layer for NvM, and
 * FreeRTOS tick source for Os timing protection).
 *
 * These are test doubles only — production code is compiled unchanged.
 */
#ifndef ASIL_STUBS_H
#define ASIL_STUBS_H

#include "Std_Types.h"
#include "MemIf.h"

/* ---- MemIf device layer (used by NvM.c) ---- */
extern MemIf_StatusType  asil_memif_status;
extern MemIf_JobResultType asil_memif_job_result;
extern Std_ReturnType    asil_memif_read_result;
extern Std_ReturnType    asil_memif_write_result;

/* ---- Optional CRC64 (used by E2E P05/P06) ---- */
uint64 Crc_CalculateCRC64(const uint8* DataPtr, uint32 Length, uint64 CrcValue, boolean FirstCall);

/* ---- NvM default configuration (declared extern in NvM.h) ---- */
#include "NvM.h"
extern const NvM_ConfigType NvM_Config;

/* ---- MCAL interrupt barrier (used by NvM ECC handler) ---- */
void Mcal_DisableAllInterrupts(void);
void Mcal_EnableAllInterrupts(void);

/* ---- E2E module lifecycle (declared in E2E.h; not yet in src) ---- */
Std_ReturnType E2E_Init(const void* ConfigPtr);
Std_ReturnType E2E_DeInit(void);

/* ---- FreeRTOS tick source (used by Os_TimingProtection.c) ---- */
extern uint32 asil_rtos_tick;

#endif /* ASIL_STUBS_H */
