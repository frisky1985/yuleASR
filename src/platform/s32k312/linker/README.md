# S32K312 Linker Scripts

## Overview

This directory contains linker scripts and configuration files for the NXP S32K312 platform (ARM Cortex-M7).

## Features

- **Standard Linker Script**: `s32k312.ld`
  - Complete memory map for 2MB Flash and 512KB SRAM
  - Interrupt vector table placement
  - Code, data, and BSS sections

- **AUTOSAR Memory Partitions**:
  - OS Partition (privileged)
  - Safety-Critical Partition (ECC protected)
  - QM (Quality Managed) Partition
  - Shared Resources Partition

- **Safety Features**:
  - ECC protection support
  - Lockstep monitor integration
  - MPU region definitions

## Memory Map

```
Flash Memory (2MB):
  0x0000_0000 - 0x0000_03FF : Reserved (1KB)
  0x0000_0400 - 0x0000_04FF : Vector Table (256B)
  0x0000_0500 - 0x001F_7FFF : Code and Constants (~2MB)
  0x001F_8000 - 0x001F_FFFF : Reserved for NVM (32KB)

SRAM (512KB):
  0x2000_0000 - 0x2000_FFFF : Safe RAM (64KB, ECC Protected)
  0x2001_0000 - 0x2006_FFFF : General RAM (384KB)
  0x2007_0000 - 0x2007_FFFF : Shared RAM (64KB)
```

## Stack and Heap

| Region | Size | Address |
|--------|------|---------|
| Stack  | 128KB | 0x2006_0000 - 0x2008_0000 |
| Heap   | 256KB | Dynamic in General RAM |

## Files

| File | Description |
|------|-------------|
| `s32k312.ld` | Main linker script |
| `Linker_Cfg.h` | C header with memory layout definitions |
| `Mpu_Cfg.ld` | MPU region configuration |
| `README.md` | This file |

## Usage

### GCC Toolchain

```bash
arm-none-eabi-gcc -T s32k312.ld -o output.elf <sources>
```

### Memory Section Placement

Use these attributes to place code/data in specific sections:

```c
// Safety-critical code
__attribute__((section(".safety_text")))
void Safety_Critical_Function(void) { ... }

// Safety-critical data
__attribute__((section(".safety_data")))
uint32_t safety_critical_var;

// OS-specific code
__attribute__((section(".os_text")))
void OS_Specific_Function(void) { ... }
```

### MPU Configuration

The MPU is configured by startup code using regions defined in `Mpu_Cfg.ld`:

| Region | Purpose | Access |
|--------|---------|--------|
| 0 | Flash Code | Execute-only (Privileged) |
| 1 | Flash Data | Read-only |
| 2 | Safe RAM | Full Access (ECC) |
| 3 | General RAM | Full Access |
| 4 | Stack | Read/Write (No Execute) |
| 5 | Shared RAM | Full Access |
| 6 | Peripherals | Device Memory |
| 7 | OS Reserved | Privileged Only |

## Linker Symbols

The linker script exports these symbols for use in C/C++ code:

```c
// Section boundaries
extern uint32_t __text_start, __text_end;
extern uint32_t __data_start, __data_end;
extern uint32_t __bss_start, __bss_end;

// Safe RAM boundaries
extern uint32_t __safe_data_start, __safe_data_end;
extern uint32_t __safe_bss_start, __safe_bss_end;

// Stack and heap
extern uint32_t __stack_top, __stack_bottom;
extern uint32_t __heap_start, __heap_end;

// Vector table
extern uint32_t __vector_table;
```

## Customization

To customize the linker script for your application:

1. **Adjust Stack/Heap Size**: Modify `STACK_SIZE` and `HEAP_SIZE` in `s32k312.ld`

2. **Add Custom Sections**: Add new sections in the SECTIONS block:
   ```ld
   .custom_section :
   {
       KEEP(*(.custom_data))
   } > ram_general
   ```

3. **Modify Memory Layout**: Adjust MEMORY blocks for different memory sizes

## Safety Considerations

- Always place safety-critical variables in `.safety_data` section (ECC protected)
- Use appropriate MPU regions for memory protection
- Ensure stack overflow protection is enabled
- Validate memory initialization in startup code

## References

- S32K3xx Reference Manual (RM)
- ARM Cortex-M7 Technical Reference Manual
- AUTOSAR Memory Mapping Specification
