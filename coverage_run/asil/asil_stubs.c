/* asil_stubs.c — Native host stubs for ASIL coverage test binaries
 *
 * Test doubles for BSW dependencies not linked into host unit-test
 * binaries.  Behavior is controllable via the exported variables so the
 * drivers can exercise both success and error paths of the module under
 * test (white-box fault injection at the boundary).
 */
#include "asil_stubs.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Os.h"
#include "Os_TimingProtection_Cfg.h"

MemIf_StatusType   asil_memif_status       = MEMIF_IDLE;
MemIf_JobResultType asil_memif_job_result  = MEMIF_JOB_OK;
Std_ReturnType     asil_memif_read_result  = E_OK;
Std_ReturnType     asil_memif_write_result = E_OK;

uint32 asil_rtos_tick = 0U;

/* ---- MemIf device layer stubs ---- */
void MemIf_Init(const MemIf_ConfigType* ConfigPtr)
{
    (void)ConfigPtr;
    asil_memif_status = MEMIF_IDLE;
}

void MemIf_Cancel(uint8 DeviceIndex)
{
    (void)DeviceIndex;
}

Std_ReturnType MemIf_Read(uint8 DeviceIndex, uint16 BlockNumber,
                          uint16 BlockOffset, uint8* DataPtr, uint16 Length)
{
    (void)DeviceIndex; (void)BlockNumber; (void)BlockOffset;
    (void)DataPtr; (void)Length;
    return asil_memif_read_result;
}

Std_ReturnType MemIf_Write(uint8 DeviceIndex, uint16 BlockNumber,
                           const uint8* DataPtr)
{
    (void)DeviceIndex; (void)BlockNumber; (void)DataPtr;
    return asil_memif_write_result;
}

Std_ReturnType MemIf_InvalidateBlock(uint8 DeviceIndex, uint16 BlockNumber)
{
    (void)DeviceIndex; (void)BlockNumber;
    return E_OK;
}

Std_ReturnType MemIf_EraseImmediateBlock(uint8 DeviceIndex, uint16 BlockNumber)
{
    (void)DeviceIndex; (void)BlockNumber;
    return E_OK;
}

MemIf_StatusType MemIf_GetStatus(uint8 DeviceIndex)
{
    (void)DeviceIndex;
    return asil_memif_status;
}

MemIf_JobResultType MemIf_GetJobResult(uint8 DeviceIndex)
{
    (void)DeviceIndex;
    return asil_memif_job_result;
}

void MemIf_SetMode(uint8 DeviceIndex, MemIf_ModeType Mode)
{
    (void)DeviceIndex; (void)Mode;
}

/* ---- FreeRTOS tick source stub ---- */
TickType_t xTaskGetTickCount(void)
{
    return (TickType_t)asil_rtos_tick;
}

/* ---- OS hooks referenced by Os_TimingProtection.c (weak symbols so the
 *      real Os.c implementation, when linked, takes precedence) ---- */
void Os_ErrorHook(uint32 ErrorCode)
{
    (void)ErrorCode;
}

void Os_ProtectionHook(uint32 ProtectionError)
{
    (void)ProtectionError;
}

void Os_TerminateTask(void)
{
    /* Host stub — real Os.c (FreeRTOS-wrapped) not linked in this binary */
}

/* ---- OS timing budget configuration (declared extern in
 *      Os_TimingProtection_Cfg.h; production Os_Cfg.c provides it but is
 *      not linked into this host binary) ---- */
#if (OS_TASK_COUNT > 0)
/* Per-task budgets: task 0 keeps ERROR_HOOK (baseline), the others use
 * the remaining fault actions so every Os_CheckTimingFault branch is
 * driven by the host test (test double only — production Os_Cfg.c
 * untouched). */
const Os_TaskTimingBudgetType Os_TaskTimingBudgets[OS_TASK_COUNT] = {
    [0] = { 0, 100000U, 10000U, OS_TIMING_ACTION_ERROR_HOOK },
    [1] = { 0, 100000U, 10000U, OS_TIMING_ACTION_TASK_KILL },
    [2] = { 0, 100000U, 10000U, OS_TIMING_ACTION_TASK_RESTART },
    [3] = { 0, 100000U, 10000U, OS_TIMING_ACTION_PROTECTION_HOOK },
    [4] = { 0, 100000U, 10000U, OS_TIMING_ACTION_NONE },
#if (OS_TASK_COUNT > 5)
    [5 ... (OS_TASK_COUNT - 1)] = { 0, 100000U, 10000U, OS_TIMING_ACTION_ERROR_HOOK }
#endif
};
#endif

#if (OS_RESOURCE_COUNT > 0)
const Os_ResourceTimingBudgetType Os_ResourceTimingBudgets[OS_RESOURCE_COUNT] = {
    [0 ... (OS_RESOURCE_COUNT - 1)] = { 0, 50000U, OS_TIMING_ACTION_ERROR_HOOK }
};
#endif

const Os_InterruptTimingBudgetType Os_InterruptTimingBudget = {
    10000U, 50000U, OS_TIMING_ACTION_ERROR_HOOK
};

/* ---- NvM default configuration (declared extern in NvM.h; the
 *      production NvM_test.c defines it but is not part of the host
 *      test binary — empty config exercises null-handling paths) ---- */
const NvM_ConfigType NvM_Config = { 0 };

/* ---- MCAL interrupt barrier (used by NvM ECC handler) ---- */
void Mcal_DisableAllInterrupts(void) {}
void Mcal_EnableAllInterrupts(void) {}

/* ---- E2E module lifecycle (E2E_Init/E2E_DeInit are declared in E2E.h
 *      but not yet implemented in src/bsw/services/e2e/src/E2E.c —
 *      host stubs keep the API contract testable; finding logged) ---- */
Std_ReturnType E2E_Init(const void* ConfigPtr)
{
    (void)ConfigPtr;
    return E_OK;
}

Std_ReturnType E2E_DeInit(void)
{
    return E_OK;
}

/* ---- Optional CRC64 (used by E2E P05/P06) ---- */
uint64 Crc_CalculateCRC64(const uint8* DataPtr, uint32 Length, uint64 CrcValue, boolean FirstCall)
{
    /* CRC-64/ECMA-182 reference implementation (host test double).
     * Crc module only mandates CRC8/16/32; CRC64 is an optional
     * extension used by E2E profiles 5/6. */
    uint64 crc = FirstCall ? 0ULL : CrcValue;
    uint32 i;
    for (i = 0U; i < Length; i++)
    {
        crc ^= ((uint64)DataPtr[i] << 56U);
        for (int b = 0; b < 8; b++)
        {
            crc = (crc & 0x8000000000000000ULL)
                ? ((crc << 1U) ^ 0x42F0E1EBA9EA3693ULL)
                : (crc << 1U);
        }
    }
    return crc;
}
