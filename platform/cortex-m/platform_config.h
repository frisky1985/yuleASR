/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : Generic Cortex-M Support
*
* SW Version           : 1.0.0
* Build Date           : 2026-04-19
*
* (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
* All Rights Reserved.
==================================================================================================*/

#ifndef PLATFORM_CONFIG_H
#define PLATFORM_CONFIG_H

/*==================================================================================================
*                                     Cortex-M Core Selection
==================================================================================================*/

/* Supported Cortex-M cores */
#define CORTEX_M0           0x00
#define CORTEX_M0PLUS       0x01
#define CORTEX_M3           0x03
#define CORTEX_M4           0x04
#define CORTEX_M7           0x07
#define CORTEX_M23          0x17
#define CORTEX_M33          0x21
#define CORTEX_M55          0x37

/* Default to Cortex-M4 if not specified */
#ifndef CORTEX_M_CORE
    #define CORTEX_M_CORE   CORTEX_M4
#endif

/*==================================================================================================
*                                     Core Features
==================================================================================================*/

#if (CORTEX_M_CORE == CORTEX_M0) || (CORTEX_M_CORE == CORTEX_M0PLUS)
    #define CORTEX_M_HAS_FPU        STD_OFF
    #define CORTEX_M_HAS_MPU        STD_OFF
    #define CORTEX_M_HAS_DSP        STD_OFF
    #define CORTEX_M_CACHE_LINE_SIZE 0
#elif (CORTEX_M_CORE == CORTEX_M3)
    #define CORTEX_M_HAS_FPU        STD_OFF
    #define CORTEX_M_HAS_MPU        STD_ON
    #define CORTEX_M_HAS_DSP        STD_OFF
    #define CORTEX_M_CACHE_LINE_SIZE 0
#elif (CORTEX_M_CORE == CORTEX_M4)
    #define CORTEX_M_HAS_FPU        STD_ON
    #define CORTEX_M_HAS_MPU        STD_ON
    #define CORTEX_M_HAS_DSP        STD_ON
    #define CORTEX_M_CACHE_LINE_SIZE 0
#elif (CORTEX_M_CORE == CORTEX_M7)
    #define CORTEX_M_HAS_FPU        STD_ON
    #define CORTEX_M_HAS_MPU        STD_ON
    #define CORTEX_M_HAS_DSP        STD_ON
    #define CORTEX_M_CACHE_LINE_SIZE 32
#elif (CORTEX_M_CORE == CORTEX_M23)
    #define CORTEX_M_HAS_FPU        STD_OFF
    #define CORTEX_M_HAS_MPU        STD_ON
    #define CORTEX_M_HAS_DSP        STD_OFF
    #define CORTEX_M_CACHE_LINE_SIZE 0
#elif (CORTEX_M_CORE == CORTEX_M33) || (CORTEX_M_CORE == CORTEX_M55)
    #define CORTEX_M_HAS_FPU        STD_ON
    #define CORTEX_M_HAS_MPU        STD_ON
    #define CORTEX_M_HAS_DSP        STD_ON
    #define CORTEX_M_CACHE_LINE_SIZE 0
#endif

/*==================================================================================================
*                                     Memory Configuration
==================================================================================================*/

/* Flash memory */
#ifndef FLASH_BASE
    #define FLASH_BASE          0x08000000U
#endif

#ifndef FLASH_SIZE
    #define FLASH_SIZE          0x00100000U  /* 1MB default */
#endif

/* SRAM memory */
#ifndef SRAM_BASE
    #define SRAM_BASE           0x20000000U
#endif

#ifndef SRAM_SIZE
    #define SRAM_SIZE           0x00040000U  /* 256KB default */
#endif

/* Peripheral base */
#ifndef PERIPH_BASE
    #define PERIPH_BASE         0x40000000U
#endif

/*==================================================================================================
*                                     Clock Configuration
==================================================================================================*/

/* System clock frequency (Hz) */
#ifndef SYSTEM_CLOCK_FREQ
    #define SYSTEM_CLOCK_FREQ   168000000U  /* 168MHz default */
#endif

/* AHB clock frequency (Hz) */
#ifndef AHB_CLOCK_FREQ
    #define AHB_CLOCK_FREQ      SYSTEM_CLOCK_FREQ
#endif

/* APB1 clock frequency (Hz) */
#ifndef APB1_CLOCK_FREQ
    #define APB1_CLOCK_FREQ     (SYSTEM_CLOCK_FREQ / 4)
#endif

/* APB2 clock frequency (Hz) */
#ifndef APB2_CLOCK_FREQ
    #define APB2_CLOCK_FREQ     (SYSTEM_CLOCK_FREQ / 2)
#endif

/*==================================================================================================
*                                     NVIC Configuration
==================================================================================================*/

/* Number of priority bits implemented */
#if (CORTEX_M_CORE == CORTEX_M0) || (CORTEX_M_CORE == CORTEX_M0PLUS)
    #define NVIC_PRIO_BITS      2
#elif (CORTEX_M_CORE == CORTEX_M3) || (CORTEX_M_CORE == CORTEX_M4) || \
      (CORTEX_M_CORE == CORTEX_M7)
    #define NVIC_PRIO_BITS      4
#elif (CORTEX_M_CORE == CORTEX_M23) || (CORTEX_M_CORE == CORTEX_M33) || \
      (CORTEX_M_CORE == CORTEX_M55)
    #define NVIC_PRIO_BITS      4
#else
    #define NVIC_PRIO_BITS      4
#endif

/* Maximum number of interrupts */
#ifndef NVIC_MAX_IRQ
    #define NVIC_MAX_IRQ        240
#endif

/*==================================================================================================
*                                     SysTick Configuration
==================================================================================================*/

/* SysTick clock source */
#define SYSTICK_CLKSOURCE_HCLK_DIV8     0x00
#define SYSTICK_CLKSOURCE_HCLK          0x01

#ifndef SYSTICK_CLKSOURCE
    #define SYSTICK_CLKSOURCE   SYSTICK_CLKSOURCE_HCLK
#endif

/*==================================================================================================
*                                     Debug Configuration
==================================================================================================*/

/* Enable debug features */
#ifndef DEBUG_ENABLE
    #define DEBUG_ENABLE        STD_ON
#endif

/* Debug UART baudrate */
#ifndef DEBUG_UART_BAUDRATE
    #define DEBUG_UART_BAUDRATE 115200
#endif

/*==================================================================================================
*                                     Compiler Abstraction
==================================================================================================*/

/* ARM Compiler */
#if defined(__CC_ARM)
    #define CORTEX_M_INLINE     __inline
    #define CORTEX_M_STATIC_INLINE static __inline
    #define CORTEX_M_WEAK       __weak
    #define CORTEX_M_IRQ        __irq

/* GCC */
#elif defined(__GNUC__)
    #define CORTEX_M_INLINE     inline
    #define CORTEX_M_STATIC_INLINE static inline
    #define CORTEX_M_WEAK       __attribute__((weak))
    #define CORTEX_M_IRQ        __attribute__((interrupt))

/* IAR */
#elif defined(__ICCARM__)
    #define CORTEX_M_INLINE     inline
    #define CORTEX_M_STATIC_INLINE static inline
    #define CORTEX_M_WEAK       __weak
    #define CORTEX_M_IRQ        __irq

/* Default */
#else
    #define CORTEX_M_INLINE     inline
    #define CORTEX_M_STATIC_INLINE static inline
    #define CORTEX_M_WEAK
    #define CORTEX_M_IRQ
#endif

/*==================================================================================================
*                                     Assembly Instructions
==================================================================================================*/

/* Enable interrupts */
#define CORTEX_M_ENABLE_IRQ()   __asm volatile ("cpsie i")

/* Disable interrupts */
#define CORTEX_M_DISABLE_IRQ()  __asm volatile ("cpsid i")

/* Wait for interrupt */
#define CORTEX_M_WFI()          __asm volatile ("wfi")

/* Wait for event */
#define CORTEX_M_WFE()          __asm volatile ("wfe")

/* Send event */
#define CORTEX_M_SEV()          __asm volatile ("sev")

/* Data synchronization barrier */
#define CORTEX_M_DSB()          __asm volatile ("dsb")

/* Instruction synchronization barrier */
#define CORTEX_M_ISB()          __asm volatile ("isb")

/* Memory synchronization barrier */
#define CORTEX_M_DMB()          __asm volatile ("dmb")

/*==================================================================================================
*                                     NVIC Functions
==================================================================================================*/

/* NVIC registers */
#define NVIC_ISER_BASE          (0xE000E100U)
#define NVIC_ICER_BASE          (0xE000E180U)
#define NVIC_ISPR_BASE          (0xE000E200U)
#define NVIC_ICPR_BASE          (0xE000E280U)
#define NVIC_IABR_BASE          (0xE000E300U)
#define NVIC_IPR_BASE           (0xE000E400U)

/* Enable IRQ */
CORTEX_M_STATIC_INLINE void NVIC_EnableIRQ(uint32 IRQn)
{
    volatile uint32* iser = (volatile uint32*)NVIC_ISER_BASE;
    iser[IRQn >> 5] = (1U << (IRQn & 0x1F));
}

/* Disable IRQ */
CORTEX_M_STATIC_INLINE void NVIC_DisableIRQ(uint32 IRQn)
{
    volatile uint32* icer = (volatile uint32*)NVIC_ICER_BASE;
    icer[IRQn >> 5] = (1U << (IRQn & 0x1F));
}

/* Set priority */
CORTEX_M_STATIC_INLINE void NVIC_SetPriority(uint32 IRQn, uint32 priority)
{
    volatile uint8* ipr = (volatile uint8*)NVIC_IPR_BASE;
    ipr[IRQn] = (uint8)(priority << (8 - NVIC_PRIO_BITS));
}

/* Get priority */
CORTEX_M_STATIC_INLINE uint32 NVIC_GetPriority(uint32 IRQn)
{
    volatile uint8* ipr = (volatile uint8*)NVIC_IPR_BASE;
    return (uint32)(ipr[IRQn] >> (8 - NVIC_PRIO_BITS));
}

/*==================================================================================================
*                                     SysTick Functions
==================================================================================================*/

/* SysTick registers */
#define SYSTICK_CTRL            (*(volatile uint32*)0xE000E010)
#define SYSTICK_LOAD            (*(volatile uint32*)0xE000E014)
#define SYSTICK_VAL             (*(volatile uint32*)0xE000E018)
#define SYSTICK_CALIB           (*(volatile uint32*)0xE000E01C)

/* SysTick control bits */
#define SYSTICK_CTRL_ENABLE     0x01
#define SYSTICK_CTRL_TICKINT    0x02
#define SYSTICK_CTRL_CLKSOURCE  0x04
#define SYSTICK_CTRL_COUNTFLAG  0x10000

/* Initialize SysTick */
CORTEX_M_STATIC_INLINE void SysTick_Init(uint32 ticks)
{
    SYSTICK_LOAD = ticks - 1;
    SYSTICK_VAL = 0;
    SYSTICK_CTRL = SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_ENABLE;
}

/* Get SysTick count */
CORTEX_M_STATIC_INLINE uint32 SysTick_GetCount(void)
{
    return SYSTICK_VAL;
}

/* Check if SysTick has counted to 0 */
CORTEX_M_STATIC_INLINE uint32 SysTick_GetFlag(void)
{
    return (SYSTICK_CTRL & SYSTICK_CTRL_COUNTFLAG) ? 1 : 0;
}

#endif /* PLATFORM_CONFIG_H */
