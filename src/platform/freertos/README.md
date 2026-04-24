# FreeRTOS Kernel Integration

This directory contains the FreeRTOS kernel source code integrated into the eth-dds-integration project.

## Version Information

- **FreeRTOS Kernel Version**: V10.6.2 LTS
- **Release Date**: November 2023
- **Source**: https://github.com/FreeRTOS/FreeRTOS-Kernel/releases/tag/V10.6.2
- **License**: MIT (see kernel/LICENSE.md)

## Directory Structure

```
src/platform/freertos/
├── README.md              # This file
└── kernel/                # FreeRTOS kernel source code
    ├── include/           # FreeRTOS header files
    │   └── FreeRTOS.h     # Main FreeRTOS header
    ├── portable/          # Platform-specific port files
    │   ├── ARMClang/      # ARM Compiler 6 port
    │   ├── ARMv8M/        # ARMv8-M architecture port
    │   ├── Common/        # Common port utilities
    │   ├── GCC/           # GCC compiler ports (ARM, RISC-V, etc.)
    │   └── MemMang/       # Memory management implementations
    ├── croutine.c         # Co-routine support
    ├── event_groups.c     # Event groups implementation
    ├── list.c             # List data structure
    ├── queue.c            # Queues and semaphores
    ├── stream_buffer.c    # Stream buffers
    ├── tasks.c            # Task management
    ├── timers.c           # Software timers
    ├── LICENSE.md         # FreeRTOS license
    ├── History.txt        # Version history
    └── README.md          # FreeRTOS README
```

## Integration Notes

### Port Selection

The following portable layers are retained for embedded automotive use cases:

1. **GCC/** - GCC compiler ports for various ARM architectures (Cortex-M, Cortex-R, Cortex-A)
2. **ARMClang/** - ARM Compiler 6 ports
3. **ARMv8M/** - ARMv8-M secure/non-secure context management
4. **Common/** - Common port utilities
5. **MemMang/** - Memory management implementations (heap_1.c through heap_5.c)

### Required Application Files

To use FreeRTOS, the application must provide:

1. **FreeRTOSConfig.h** - Configuration header file (place in application include path)
2. **Port-specific files** - Select from `portable/<compiler>/<architecture>/`
3. **Memory management** - Select one from `portable/MemMang/heap_x.c`

### Typical CMake Integration

```cmake
# Add FreeRTOS kernel sources
set(FREERTOS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/tasks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/list.c
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/timers.c
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/event_groups.c
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/stream_buffer.c
    # Select memory manager (heap_4.c recommended for most applications)
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/portable/MemMang/heap_4.c
)

# Add port-specific sources based on target
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    list(APPEND FREERTOS_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/kernel/portable/GCC/ARM_CM4F/port.c
        ${CMAKE_CURRENT_SOURCE_DIR}/kernel/portable/GCC/ARM_CM4F/portmacro.h
    )
endif()

# Include directories
target_include_directories(your_target PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/include
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel/portable/GCC/ARM_CM4F
)
```

### Configuration

Key configuration options in `FreeRTOSConfig.h`:

| Macro | Description | Typical Value |
|-------|-------------|---------------|
| configUSE_PREEMPTION | Enable preemptive scheduling | 1 |
| configUSE_IDLE_HOOK | Enable idle hook | 0 |
| configUSE_TICK_HOOK | Enable tick hook | 0 |
| configCPU_CLOCK_HZ | CPU clock frequency in Hz | SystemCoreClock |
| configTICK_RATE_HZ | Tick rate (Hz) | 1000 |
| configMAX_PRIORITIES | Number of task priorities | 5-16 |
| configMINIMAL_STACK_SIZE | Minimal stack size (words) | 128 |
| configTOTAL_HEAP_SIZE | Total heap size (bytes) | 32768 |
| configMAX_TASK_NAME_LEN | Max task name length | 16 |
| configUSE_TRACE_FACILITY | Enable trace facility | 1 (debug) |
| configUSE_16_BIT_TICKS | Use 16-bit tick type | 0 |
| configIDLE_SHOULD_YIELD | Idle task yields to equal priority | 1 |

## Memory Management Options

Located in `kernel/portable/MemMang/`:

- **heap_1.c** - Simple, no free() support, deterministic
- **heap_2.c** - Best fit, no coalescence, fragmentation possible
- **heap_3.c** - Wrapper around standard library malloc/free
- **heap_4.c** - First fit with coalescence, **recommended for most applications**
- **heap_5.c** - Heap_4 with multiple non-contiguous heap regions

## Supported Architectures

### GCC Ports
- ARM Cortex-M0/M0+/M1
- ARM Cortex-M3
- ARM Cortex-M4/M4F
- ARM Cortex-M7
- ARM Cortex-M23/M33/M55/M85
- ARM Cortex-R4/R5/R7/R8
- ARM Cortex-A9/A53/A72
- RISC-V
- x86 (for simulation)

### ARMClang Ports
- ARMv6-M, ARMv7-M, ARMv8-M
- ARMv7-R, ARMv8-R
- ARMv7-A, ARMv8-A

## Safety Considerations

This FreeRTOS version (V10.6.2 LTS) is a Long Term Support release suitable for safety-critical applications when combined with appropriate safety measures:

- Use static allocation where possible (configSUPPORT_STATIC_ALLOCATION)
- Enable stack overflow checking (configCHECK_FOR_STACK_OVERFLOW)
- Use assertion hooks (configASSERT)
- Implement MPU protection for critical tasks (if hardware supports)
- Follow MISRA C guidelines when applicable

## Updates

To update to a newer FreeRTOS version:

1. Download new release from https://github.com/FreeRTOS/FreeRTOS-Kernel/releases
2. Replace contents of `kernel/` directory
3. Remove unnecessary portable directories (keep only GCC, ARMClang, ARMv8M, Common, MemMang)
4. Update version information in this README
5. Test application thoroughly

## References

- FreeRTOS Documentation: https://www.freertos.org/Documentation/RTOS_book.html
- FreeRTOS API Reference: https://www.freertos.org/a00106.html
- GitHub Repository: https://github.com/FreeRTOS/FreeRTOS-Kernel
- License: MIT License (see kernel/LICENSE.md)
