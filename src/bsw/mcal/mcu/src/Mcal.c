/*==================================================================================================
 * Mcal.c - MCAL interrupt control (real implementation)
 *
 * Implements the MCAL-level interrupt control functions per the S32K312
 * (ARM Cortex-M7) MCAL semantics:
 *   - Mcal_DisableAllInterrupts(): set PRIMASK (cpsid i)  -> all maskable
 *     interrupts disabled
 *   - Mcal_EnableAllInterrupts():  clear PRIMASK (cpsie i) -> interrupts
 *     enabled
 *   - Mcal_ResetSystem(): system reset via SCB->AIRCR (VECTKEY | SYSRESETREQ)
 *
 * On aarch64 host builds the same semantics are provided with the real
 * PSTATE.DAIF interrupt-mask instructions (msr daifset/daifclr #2).
 *================================================================================================*/
/* @req SWS_Mcu_00001 @req SWS_Mcu_00002 @req SWS_Mcu_00003 */

#include "Mcal.h"

/*==================================================================================================
 *                                      Mcal_DisableAllInterrupts
 *================================================================================================*/
void Mcal_DisableAllInterrupts(void)
{
#if defined(__aarch64__) && !defined(__APPLE__)
    /* Real interrupt masking on AArch64: set the IRQ mask bit (DAIF.I)
     * NOTE: excluded on Apple platforms — macOS user-space forbids DAIF
     * modification (msr daifset raises SIGILL/EXC_BAD_INSTRUCTION). */
    __asm volatile ("msr daifset, #2" ::: "memory");
#elif defined(__arm__) || defined(__thumb__)
    /* Cortex-M7: set PRIMASK (disables all maskable interrupts) */
    __asm volatile ("cpsid i" ::: "memory");
#else
    /* Non-ARM host or Apple aarch64: no interrupt control available in user space. */
#endif
}

/*==================================================================================================
 *                                      Mcal_EnableAllInterrupts
 *================================================================================================*/
void Mcal_EnableAllInterrupts(void)
{
#if defined(__aarch64__) && !defined(__APPLE__)
    /* Real interrupt unmasking on AArch64: clear the IRQ mask bit (DAIF.I)
     * NOTE: excluded on Apple platforms — macOS user-space forbids DAIF
     * modification (msr daifclr raises SIGILL/EXC_BAD_INSTRUCTION). */
    __asm volatile ("msr daifclr, #2" ::: "memory");
#elif defined(__arm__) || defined(__thumb__)
    /* Cortex-M7: clear PRIMASK (enables maskable interrupts) */
    __asm volatile ("cpsie i" ::: "memory");
#else
    /* Non-ARM host or Apple aarch64: no interrupt control available in user space. */
#endif
}

/*==================================================================================================
 *                                      Mcal_ResetSystem
 *================================================================================================*/
void Mcal_ResetSystem(void)
{
#if defined(__arm__) || defined(__thumb__)
    /* Cortex-M7 (S32K312): SCB->AIRCR = VECTKEY | SYSRESETREQ
     * SCB base 0xE000ED00, AIRCR offset 0x0C, VECTKEY 0x05FA0000,
     * SYSRESETREQ bit 2. */
    volatile uint32* scbAircr = (volatile uint32*)0xE000ED0CU;
    *scbAircr = (0x05FA0000UL | (1UL << 2));
    /* Wait for reset to take effect */
    for (;;)
    {
        /* spin */
    }
#elif defined(__aarch64__)
    /* No user-space system reset on AArch64 hosts; trap instead of silently
     * returning so the caller observes the failure. */
    __builtin_trap();
#else
    /* Non-ARM host: no reset available. */
#endif
}
