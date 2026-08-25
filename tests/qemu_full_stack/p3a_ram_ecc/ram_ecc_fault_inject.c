/*
 * ram_ecc_fault_inject.c - C9: Software hooks to simulate RAM ECC faults
 */
#include <string.h>
#include <stdint.h>

typedef unsigned char uint8;
typedef unsigned int uint32;
typedef uint8 Std_ReturnType;
#define E_OK 0U
#define E_NOT_OK 1U

#define RAMSAFETY_OK      0U
#define RAMSAFETY_FAILED  1U

static void (*s_sec_cb)(uint32_t addr) = NULL;
static void (*s_ded_cb)(uint32_t addr) = NULL;
static uint32_t s_error_count = 0U;
static uint32_t s_corrected_count = 0U;
static uint32_t s_safe_state_called = 0U;
static uint8_t  s_module_state = RAMSAFETY_OK;

void FaultInject_RegisterEccCallbacks(void (*sec)(uint32_t), void (*ded)(uint32_t))
{
    s_sec_cb = sec;
    s_ded_cb = ded;
}

void FaultInject_FlipBit1(uint8_t *addr, uint8_t bitPos)
{
    if (addr == NULL) return;
    *addr ^= (uint8_t)(1U << (bitPos & 7U));
    if (s_sec_cb != NULL) { s_sec_cb((uint32_t)(uintptr_t)addr); }
}

void FaultInject_FlipBit2(uint8_t *addr, uint8_t pos1, uint8_t pos2)
{
    if (addr == NULL) return;
    *addr ^= (uint8_t)((1U << (pos1 & 7U)) | (1U << (pos2 & 7U)));
    if (s_ded_cb != NULL) { s_ded_cb((uint32_t)(uintptr_t)addr); }
}

/* RamSafety test API (normally in RamSafety.c; stubbed here for self-contained test) */
void RamSafety_HandleSingleBitError(uint32_t addr)
{
    (void)addr;
    s_corrected_count++;
    s_error_count++;
}

void RamSafety_HandleDoubleBitError(uint32_t addr)
{
    (void)addr;
    s_error_count++;
    s_module_state = RAMSAFETY_FAILED;
    s_safe_state_called = 1U;
}

void RamSafety_EnterSafeState(void)
{
    s_safe_state_called = 1U;
}

uint32_t RamSafety_GetErrorCount(void)   { return s_error_count; }
uint32_t RamSafety_GetCorrectedCount(void){ return s_corrected_count; }
uint8_t  RamSafety_GetState(void)        { return s_module_state; }
uint32_t RamSafety_GetSafeStateCalls(void){ return s_safe_state_called; }
void     RamSafety_ResetStats(void)
{
    s_error_count = 0U; s_corrected_count = 0U; s_safe_state_called = 0U;
    s_module_state = RAMSAFETY_OK;
}

/* March C- on a small region */
Std_ReturnType RamSafety_VerifyRegion(const uint8_t *start, uint32_t len)
{
    if (start == NULL || len == 0U) return E_NOT_OK;
    /* Simple additive checksum */
    uint32_t sum = 0U;
    for (uint32_t i = 0; i < len; i++) sum += start[i];
    return (sum == 0U) ? E_NOT_OK : E_OK;
}
