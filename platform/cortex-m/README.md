# Generic Cortex-M Platform Support

## Overview

This directory contains generic support for ARM Cortex-M microcontrollers, allowing yuleASR to run on various Cortex-M based chips.

## Supported Cores

| Core | FPU | MPU | DSP | Cache |
|:-----|:---:|:---:|:---:|:-----:|
| Cortex-M0 | No | No | No | No |
| Cortex-M0+ | No | No | No | No |
| Cortex-M3 | No | Yes | No | No |
| Cortex-M4 | Yes | Yes | Yes | No |
| Cortex-M7 | Yes | Yes | Yes | Yes |
| Cortex-M23 | No | Yes | No | No |
| Cortex-M33 | Yes | Yes | Yes | No |
| Cortex-M55 | Yes | Yes | Yes | No |

## Configuration

### Select Core

Define the target Cortex-M core in your build configuration:

```c
#define CORTEX_M_CORE CORTEX_M4
```

Available options:
- `CORTEX_M0`
- `CORTEX_M0PLUS`
- `CORTEX_M3`
- `CORTEX_M4`
- `CORTEX_M7`
- `CORTEX_M23`
- `CORTEX_M33`
- `CORTEX_M55`

### Memory Configuration

Override default memory settings:

```c
#define FLASH_BASE      0x08000000U
#define FLASH_SIZE      0x00100000U  /* 1MB */
#define SRAM_BASE       0x20000000U
#define SRAM_SIZE       0x00040000U  /* 256KB */
```

### Clock Configuration

```c
#define SYSTEM_CLOCK_FREQ   168000000U  /* 168MHz */
```

## Files

| File | Description |
|:-----|:------------|
| `platform_config.h` | Core configuration and low-level functions |
| `startup_cortex_m.c` | Startup code with vector table |
| `README.md` | This documentation |

## Usage

### 1. Include Platform Header

```c
#include "platform_config.h"
```

### 2. Use NVIC Functions

```c
/* Enable interrupt */
NVIC_EnableIRQ(TIM2_IRQn);

/* Set priority */
NVIC_SetPriority(TIM2_IRQn, 5);
```

### 3. Use SysTick

```c
/* Initialize for 1ms tick at 168MHz */
SysTick_Init(168000);
```

### 4. Critical Sections

```c
/* Disable interrupts */
CORTEX_M_DISABLE_IRQ();

/* Critical code */

/* Enable interrupts */
CORTEX_M_ENABLE_IRQ();
```

## Integration with BSW

### Mcu Driver

The platform support integrates with the Mcu driver:

```c
Mcu_ConfigType mcu_config = {
    .ClockSettings = &clock_config,
    .RamSectorSettings = ram_config,
    .NumClockSettings = 1,
    .NumRamSectors = 1
};

Mcu_Init(&mcu_config);
```

### Port Driver

GPIO configuration uses platform-specific registers:

```c
/* Configure PA5 as output */
GPIOA->MODER &= ~(3U << (5 * 2));
GPIOA->MODER |= (1U << (5 * 2));
```

### CAN Driver

CAN peripheral configuration:

```c
/* Enable CAN clock */
RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;

/* Configure CAN */
CAN1->BTR = (prescaler << 0) | (bs1 << 16) | (bs2 << 20) | (sjw << 24);
```

## Build Configuration

### GCC

```bash
arm-none-eabi-gcc \
    -mcpu=cortex-m4 \
    -mthumb \
    -mfpu=fpv4-sp-d16 \
    -mfloat-abi=hard \
    -D CORTEX_M_CORE=CORTEX_M4 \
    -c platform_config.c
```

### IAR

```bash
iccarm \
    --cpu Cortex-M4 \
    --fpu VFPv4_sp \
    -D CORTEX_M_CORE=CORTEX_M4 \
    platform_config.c
```

### Keil/ARM Compiler

```bash
armcc \
    --cpu Cortex-M4.fp \
    -D CORTEX_M_CORE=CORTEX_M4 \
    -c platform_config.c
```

## Porting Guide

### 1. Create Chip-Specific Directory

```
platform/
├── cortex-m/           # Generic support
└── stm32f4/           # STM32F4 specific
    ├── include/
    └── src/
```

### 2. Implement Chip-Specific Functions

```c
/* stm32f4_clock.c */
void SystemClock_Config(void)
{
    /* Configure PLL for 168MHz */
    RCC->PLLCFGR = ...;
}
```

### 3. Link Startup Code

Modify linker script for your chip's memory layout.

## Examples

### STM32F407

```c
#define CORTEX_M_CORE       CORTEX_M4
#define SYSTEM_CLOCK_FREQ   168000000U
#define FLASH_SIZE          0x00100000U
#define SRAM_SIZE           0x00020000U
```

### STM32H743 (Cortex-M7)

```c
#define CORTEX_M_CORE       CORTEX_M7
#define SYSTEM_CLOCK_FREQ   480000000U
#define FLASH_SIZE          0x00200000U
#define SRAM_SIZE           0x00100000U
```

### NXP LPC1768 (Cortex-M3)

```c
#define CORTEX_M_CORE       CORTEX_M3
#define SYSTEM_CLOCK_FREQ   100000000U
#define FLASH_SIZE          0x00080000U
#define SRAM_SIZE           0x00010000U
```

## Testing

Run unit tests on target:

```bash
make test TARGET=cortex-m4
```

## License

Copyright (c) 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
All Rights Reserved.
