/**
 * @file startup_S32K312.S
 * @brief Cortex-M7 startup file for NXP S32K312
 * @version 1.0.0
 * @date 2026-07-12
 *
 * Startup sequence:
 *  1. Initialize stack pointer
 *  2. Call SystemInit (WDT disable, FPU enable, clock config)
 *  3. Copy .data section (LMA to VMA)
 *  4. Clear .bss section
 *  5. Call main
 */

.syntax unified
.arch armv7e-m
.fpu fpv5-d16
.thumb

/*============================================================================
 * External symbols
 *============================================================================*/
.global Reset_Handler
.global Default_Handler
.global HardFault_Handler
.global MemManage_Handler
.global BusFault_Handler
.global UsageFault_Handler
.global SVC_Handler
.global DebugMon_Handler
.global PendSV_Handler
.global SysTick_Handler

/* External references */
.extern SystemInit
.extern main
.extern __stack_top
.extern __data_start
.extern __data_end
.extern __data_load
.extern __bss_start
.extern __bss_end

/*============================================================================
 * Interrupt Vector Table
 *============================================================================*/
.section .isr_vector, "a"
.type g_pfnVectors, %object
.size g_pfnVectors, . - g_pfnVectors

g_pfnVectors:
    .word __stack_top                   /* 0: Stack top */
    .word Reset_Handler                 /* 1: Reset */
    .word NMI_Handler                   /* 2: NMI */
    .word HardFault_Handler             /* 3: Hard Fault */
    .word MemManage_Handler             /* 4: MemManage */
    .word BusFault_Handler              /* 5: Bus Fault */
    .word UsageFault_Handler            /* 6: Usage Fault */
    .word 0                             /* 7: Reserved */
    .word 0                             /* 8: Reserved */
    .word 0                             /* 9: Reserved */
    .word 0                             /* 10: Reserved */
    .word SVC_Handler                   /* 11: SVCall */
    .word DebugMon_Handler              /* 12: Debug Monitor */
    .word 0                             /* 13: Reserved */
    .word PendSV_Handler                /* 14: PendSV */
    .word SysTick_Handler               /* 15: SysTick */

    /* External Interrupts (IRQ 0-31) */
    .word DMA0_IRQHandler               /* 16: DMA0 */
    .word DMA1_IRQHandler               /* 17: DMA1 */
    .word DMA2_IRQHandler               /* 18: DMA2 */
    .word DMA3_IRQHandler               /* 19: DMA3 */
    .word FLASH_IRQHandler              /* 20: Flash */
    .word ECC_IRQHandler                /* 21: ECC */
    .word CAN0_IRQHandler               /* 22: CAN0 */
    .word CAN1_IRQHandler               /* 23: CAN1 */
    .word CAN2_IRQHandler               /* 24: CAN2 */
    .word LIN_IRQHandler                /* 25: LIN */
    .word LPI2C_IRQHandler              /* 26: LPI2C */
    .word LPSPI_IRQHandler              /* 27: LPSPI */
    .word FLEXIO_IRQHandler             /* 28: FlexIO */
    .word EWM_IRQHandler                /* 29: EWM */
    .word WDOG_IRQHandler               /* 30: WDOG */
    .word LPTMR0_IRQHandler             /* 31: LPTMR0 */
    .word LPTMR1_IRQHandler             /* 32: LPTMR1 */
    .word PIT0_IRQHandler               /* 33: PIT0 */
    .word PIT1_IRQHandler               /* 34: PIT1 */
    .word PIT2_IRQHandler               /* 35: PIT2 */
    .word PIT3_IRQHandler               /* 36: PIT3 */
    .word ADC0_IRQHandler               /* 37: ADC0 */
    .word ADC1_IRQHandler               /* 38: ADC1 */
    .word CMP0_IRQHandler               /* 39: CMP0 */
    .word CMP1_IRQHandler               /* 40: CMP1 */
    .word FTM0_IRQHandler               /* 41: FTM0 */
    .word FTM1_IRQHandler               /* 42: FTM1 */
    .word FTM2_IRQHandler               /* 43: FTM2 */
    .word FTM3_IRQHandler               /* 44: FTM3 */
    .word ENET_IRQHandler               /* 45: ENET */
    .word SIUL2_IRQHandler              /* 46: SIUL2 */
    .word FCCU_IRQHandler               /* 47: FCCU */

/*============================================================================
 * Default Handler (weak alias - infinite loop)
 *============================================================================*/
.section .text.Default_Handler, "ax"
.type Default_Handler, %function
.thumb_func
Default_Handler:
    b Default_Handler
    .size Default_Handler, . - Default_Handler

/* Weak aliases for all handlers */
.macro weak_alias name
    .weak \name
    .thumb_set \name, Default_Handler
.endm

weak_alias NMI_Handler
weak_alias HardFault_Handler
weak_alias MemManage_Handler
weak_alias BusFault_Handler
weak_alias UsageFault_Handler
weak_alias SVC_Handler
weak_alias DebugMon_Handler
weak_alias PendSV_Handler
weak_alias SysTick_Handler
weak_alias DMA0_IRQHandler
weak_alias DMA1_IRQHandler
weak_alias DMA2_IRQHandler
weak_alias DMA3_IRQHandler
weak_alias FLASH_IRQHandler
weak_alias ECC_IRQHandler
weak_alias CAN0_IRQHandler
weak_alias CAN1_IRQHandler
weak_alias CAN2_IRQHandler
weak_alias LIN_IRQHandler
weak_alias LPI2C_IRQHandler
weak_alias LPSPI_IRQHandler
weak_alias FLEXIO_IRQHandler
weak_alias EWM_IRQHandler
weak_alias WDOG_IRQHandler
weak_alias LPTMR0_IRQHandler
weak_alias LPTMR1_IRQHandler
weak_alias PIT0_IRQHandler
weak_alias PIT1_IRQHandler
weak_alias PIT2_IRQHandler
weak_alias PIT3_IRQHandler
weak_alias ADC0_IRQHandler
weak_alias ADC1_IRQHandler
weak_alias CMP0_IRQHandler
weak_alias CMP1_IRQHandler
weak_alias FTM0_IRQHandler
weak_alias FTM1_IRQHandler
weak_alias FTM2_IRQHandler
weak_alias FTM3_IRQHandler
weak_alias ENET_IRQHandler
weak_alias SIUL2_IRQHandler
weak_alias FCCU_IRQHandler

/*============================================================================
 * Reset Handler
 *============================================================================*/
.section .text.Reset_Handler, "ax"
.type Reset_Handler, %function
.thumb_func
Reset_Handler:
    /* Set stack pointer */
    ldr r0, =__stack_top
    msr msp, r0

    /* Call SystemInit (WDT disable, FPU enable, clock config) */
    bl SystemInit

    /* Copy .data section from LMA to VMA */
    ldr r1, =__data_load
    ldr r2, =__data_start
    ldr r3, =__data_end
    subs r3, r2, r3        /* r3 = -size */
    cmp r3, #0
    beq .L_zero_bss

.L_copy_data:
    subs r3, r3, #4
    ldr r0, [r1, r3]
    str r0, [r2, r3]
    bne .L_copy_data

    /* Clear .bss section */
.L_zero_bss:
    ldr r1, =__bss_start
    ldr r2, =__bss_end
    movs r0, #0

.L_clear_bss:
    cmp r1, r2
    it cc
    strcc r0, [r1], #4
    bcc .L_clear_bss

    /* Call main */
    bl main

    /* Infinite loop if main returns */
.L_loop:
    b .L_loop
    .size Reset_Handler, . - Reset_Handler

/*============================================================================
 * SystemInit — Minimal hardware initialization
 *============================================================================*/
.section .text.SystemInit, "ax"
.type SystemInit, %function
.thumb_func
SystemInit:
    /* Disable WDOG (watchdog) */
    ldr r0, =0x40052000       /* WDOG_BASE on S32K312 */
    ldr r1, =0xD928C520       /* Unlock sequence */
    str r1, [r0, #0x04]       /* WDOG_CNT */
    ldr r1, =0xC520D928
    str r1, [r0, #0x04]
    movs r1, #0
    str r1, [r0, #0x06]       /* WDOG_TOVAL - disable */

    /* Enable FPU (CP10/CP11) */
    movw r0, #0xED88
    movt r0, #0xE000
    ldr r1, [r0, #0]
    orr r1, r1, #(0xF << 20)  /* CP10, CP11 FULL_ACCESS */
    str r1, [r0, #0]
    dsb
    isb

    /* Enable FPU in FPCCR */
    movw r0, #0xF000
    movt r0, #0xE000
    ldr r1, [r0, #0]
    orr r1, r1, #(1 << 30)   /* ASPEN = 1 */
    orr r1, r1, #(1 << 31)   /* LSPEN = 1 */
    str r1, [r0, #0]

    bx lr
    .size SystemInit, . - SystemInit

.end
