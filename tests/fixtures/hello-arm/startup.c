/**
 * @file startup.c
 * @brief Minimal startup code for ARM Cortex-M7
 *
 * Provides reset vector, interrupt vector table,
 * and basic initialization for SIL testing.
 */

#include <stdint.h>

/* Stack top address (defined in linker script) */
extern uint32_t _estack;

/* Entry points */
extern int main(void);

/* Default exception handlers (weak aliases) */
void Reset_Handler(void) __attribute__((naked, used));
void NMI_Handler(void) __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void) __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void) __attribute__((weak, alias("Default_Handler")));

/* Default handler: infinite loop */
void Default_Handler(void)
{
    while (1)
    {
        __asm volatile("wfi");
    }
}

/* Interrupt vector table */
__attribute__((section(".isr_vector"), used))
void (* const g_pfnVectors[])(void) = {
    (void (*)(void))&_estack, /* 0: Stack pointer */
    Reset_Handler,         /* 1: Reset */
    NMI_Handler,           /* 2: NMI */
    HardFault_Handler,     /* 3: Hard Fault */
    MemManage_Handler,     /* 4: MemManage */
    BusFault_Handler,      /* 5: Bus Fault */
    UsageFault_Handler,    /* 6: Usage Fault */
    0, 0, 0, 0,           /* 7-10: Reserved */
    SVC_Handler,           /* 11: SVCall */
    DebugMon_Handler,      /* 12: Debug Monitor */
    0,                     /* 13: Reserved */
    PendSV_Handler,        /* 14: PendSV */
    SysTick_Handler,       /* 15: SysTick */
};

/* Data sections from linker script */
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

/* Reset handler: initialize data/bss, call main */
void Reset_Handler(void)
{
    uint32_t *src, *dst;

    /* Copy .data from flash to RAM */
    src = &_sidata;
    dst = &_sdata;
    while (dst < &_edata)
    {
        *dst++ = *src++;
    }

    /* Zero-fill .bss */
    dst = &_sbss;
    while (dst < &_ebss)
    {
        *dst++ = 0U;
    }

    /* Call main */
    (void)main();

    /* Infinite loop after main returns */
    while (1)
    {
        __asm volatile("wfi");
    }
}
