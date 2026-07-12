/*
 * boot_platform.h — Host-side (macOS/Linux) platform stub 
 * Replaces AUTOSAR Std_Types.h, CMSIS headers, and hardware registers
 * for running yuleASR Secure Boot tests natively.
 */

#ifndef BOOT_PLATFORM_H
#define BOOT_PLATFORM_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* === AUTOSAR Standard Types === */
typedef unsigned char  boolean;
typedef uint8_t        uint8;
typedef uint16_t       uint16;
typedef uint32_t       uint32;
typedef int8_t         sint8;
typedef int16_t        sint16;
typedef int32_t        sint32;

#define TRUE           1U
#define FALSE          0U
#define E_OK           0U
#define E_NOT_OK       1U
#define NULL_PTR       ((void *)0)

/* === MCU Register Stubs (for Boot_Loader.c compilation) === */
typedef struct {
    uint32_t CTRL;
    uint32_t ICSR;
} SysTick_Type;
#define SysTick         ((SysTick_Type *)0xE000E010UL)

typedef struct {
    uint32_t ICSR;
    uint32_t VTOR;
} SCB_Type;
#define SCB             ((SCB_Type *)0xE000ED00UL)

#define SysTick_CTRL    SysTick->CTRL
#define SCB_ICSR        SCB->ICSR

/* Stack prototype (for Boot_Loader_Jump) */
extern uint32_t __stack_top;

/* Inline assembly stubs — no-ops on host */
#define __asm(...)
#define __attribute__(x)

/* Host memory for flash emulation */
extern uint8_t g_boot_flash_ram[];

#endif /* BOOT_PLATFORM_H */
