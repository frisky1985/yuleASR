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
├── CMakeLists.txt         # CMake configuration with multi-arch support
├── README.md              # This file
├── config/                # Configuration headers
│   ├── FreeRTOSConfig.h           # Default configuration (ARM_CM4F/Posix)
│   ├── FreeRTOSConfig_CM7.h       # Cortex-M7 configuration
│   ├── FreeRTOSConfig_CM33.h      # Cortex-M33 with TrustZone
│   └── FreeRTOSConfig_CM33_NTZ.h  # Cortex-M33 without TrustZone
├── kernel/                # FreeRTOS kernel source code
│   ├── include/           # FreeRTOS header files
│   │   └── FreeRTOS.h     # Main FreeRTOS header
│   ├── portable/          # Platform-specific port files
│   │   ├── ARMClang/      # ARM Compiler 6 port
│   │   ├── ARMv8M/        # ARMv8-M architecture port
│   │   ├── Common/        # Common port utilities
│   │   ├── GCC/           # GCC compiler ports (ARM, RISC-V, etc.)
│   │   │   ├── ARM_CM4F/          # Cortex-M4 with FPU
│   │   │   ├── ARM_CM7/r0p1/      # Cortex-M7 (r0p1 revision)
│   │   │   ├── ARM_CM33/non_secure/ # Cortex-M33 TrustZone (non-secure)
│   │   │   └── ARM_CM33_NTZ/      # Cortex-M33 No TrustZone
│   │   └── MemMang/       # Memory management implementations
│   ├── croutine.c         # Co-routine support
│   ├── event_groups.c     # Event groups implementation
│   ├── list.c             # List data structure
│   ├── queue.c            # Queues and semaphores
│   ├── stream_buffer.c    # Stream buffers
│   ├── tasks.c            # Task management
│   ├── timers.c           # Software timers
│   ├── LICENSE.md         # FreeRTOS license
│   ├── History.txt        # Version history
│   └── README.md          # FreeRTOS README
└── port/                  # Platform abstraction layer
    └── os_port.c          # OS port implementation
```

## Supported Architectures

This CMake configuration supports the following FreeRTOS ports:

| Port | Description | Configuration File | TrustZone |
|------|-------------|-------------------|-----------|
| **ARM_CM4F** | ARM Cortex-M4 with FPU | `FreeRTOSConfig.h` | No |
| **ARM_CM7** | ARM Cortex-M7 (double-precision FPU) | `FreeRTOSConfig_CM7.h` | Optional |
| **ARM_CM33** | ARM Cortex-M33 with TrustZone | `FreeRTOSConfig_CM33.h` | Yes (Non-secure) |
| **ARM_CM33_NTZ** | ARM Cortex-M33 without TrustZone | `FreeRTOSConfig_CM33_NTZ.h` | No |
| **POSIX** | POSIX/Linux simulation | `FreeRTOSConfig.h` | N/A |

### Architecture Features

#### ARM_CM4F (Cortex-M4F)
- Single-precision FPU
- 4-bit interrupt priority
- Optimized task selection

#### ARM_CM7 (Cortex-M7)
- Double-precision FPU
- D-Cache and I-Cache support
- r0p1 port revision (fixes STKALIGN issues)
- Higher clock frequencies (up to 480MHz typical)

#### ARM_CM33 (Cortex-M33 with TrustZone)
- ARMv8-M Mainline architecture
- TrustZone security extension (non-secure side)
- MPU support enabled
- Single-precision FPU
- Secure context management for calling secure functions

#### ARM_CM33_NTZ (Cortex-M33 No TrustZone)
- ARMv8-M Mainline architecture
- No TrustZone (all code runs privileged)
- Simplified configuration
- Single-precision FPU

#### POSIX (Linux Simulation)
- For development and testing on Linux hosts
- Uses pthread for task scheduling
- Useful for algorithm development without hardware

## CMake Build Configuration

### Basic Usage

Add FreeRTOS to your CMake project:

```cmake
add_subdirectory(src/platform/freertos)

target_link_libraries(your_target PRIVATE freertos_kernel)
```

### Selecting the Port

Use the `FREERTOS_PORT` CMake option to select the target architecture:

```bash
# ARM Cortex-M4F (default)
cmake -DFREERTOS_PORT=ARM_CM4F ..

# ARM Cortex-M7
cmake -DFREERTOS_PORT=ARM_CM7 ..

# ARM Cortex-M33 with TrustZone
cmake -DFREERTOS_PORT=ARM_CM33 ..

# ARM Cortex-M33 without TrustZone
cmake -DFREERTOS_PORT=ARM_CM33_NTZ ..

# POSIX/Linux simulation
cmake -DFREERTOS_PORT=POSIX ..
```

### Complete Build Examples

#### Cross-compile for ARM Cortex-M7
```bash
mkdir build && cd build
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-gcc.cmake \
    -DFREERTOS_PORT=ARM_CM7 \
    -DCMAKE_BUILD_TYPE=Release
make
```

#### Cross-compile for ARM Cortex-M33 with TrustZone
```bash
mkdir build && cd build
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-gcc.cmake \
    -DFREERTOS_PORT=ARM_CM33 \
    -DCMAKE_BUILD_TYPE=Release
make
```

#### Build for POSIX simulation (Linux)
```bash
mkdir build && cd build
cmake .. \
    -DFREERTOS_PORT=POSIX \
    -DCMAKE_BUILD_TYPE=Debug
make
```

### Configuration File Selection

When using CMake, the appropriate `FreeRTOSConfig.h` is automatically used based on the port selection. To use architecture-specific configurations, copy the appropriate config file:

```bash
# For ARM_CM7
cp config/FreeRTOSConfig_CM7.h config/FreeRTOSConfig.h

# For ARM_CM33
cp config/FreeRTOSConfig_CM33.h config/FreeRTOSConfig.h

# For ARM_CM33_NTZ
cp config/FreeRTOSConfig_CM33_NTZ.h config/FreeRTOSConfig.h
```

Or modify your application include path to point to the specific configuration file.

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
2. **Port-specific files** - Selected automatically by CMake based on FREERTOS_PORT
3. **Memory management** - Uses heap_4.c by default (recommended for most applications)

### CMake Variables

After including FreeRTOS, the following variables are available:

| Variable | Description |
|----------|-------------|
| `FREERTOS_INCLUDE_DIRS` | Include directories for FreeRTOS headers |
| `FREERTOS_LIBRARY` | The freertos_kernel library target |
| `FREERTOS_KERNEL_DIR` | Path to kernel source directory |

## Configuration

Key configuration options in `FreeRTOSConfig.h`:

| Macro | Description | Typical Value |
|-------|-------------|---------------|
| configUSE_PREEMPTION | Enable preemptive scheduling | 1 |
| configUSE_IDLE_HOOK | Enable idle hook | 0 |
| configUSE_TICK_HOOK | Enable tick hook | 0 |
| configCPU_CLOCK_HZ | CPU clock frequency in Hz | SystemCoreClock |
| configTICK_RATE_HZ | Tick rate (Hz) | 1000 |
| configMAX_PRIORITIES | Number of task priorities | 5-16 |
| configMINIMAL_STACK_SIZE | Minimal stack size (words) | 128-512 |
| configTOTAL_HEAP_SIZE | Total heap size (bytes) | 32768-262144 |
| configMAX_TASK_NAME_LEN | Max task name length | 16 |
| configUSE_TRACE_FACILITY | Enable trace facility | 1 (debug) |
| configUSE_16_BIT_TICKS | Use 16-bit tick type | 0 |
| configIDLE_SHOULD_YIELD | Idle task yields to equal priority | 1 |

### Architecture-Specific Configuration

#### Cortex-M7 Specific
```c
#define configENABLE_FPU        1    // Enable double-precision FPU
#define configENABLE_MPU        0    // Optional MPU support
```

#### Cortex-M33 with TrustZone
```c
#define configENABLE_TRUSTZONE      1    // Enable TrustZone
#define configENABLE_SECURE_CONTEXT 1    // Enable secure context management
#define configENABLE_MPU            1    // Enable MPU
#define configENABLE_FPU            1    // Enable single-precision FPU
```

#### Cortex-M33 without TrustZone
```c
#define configENABLE_TRUSTZONE      0    // Disable TrustZone
#define configENABLE_SECURE_CONTEXT 0    // No secure context
#define configENABLE_MPU            0    // Optional MPU
#define configENABLE_FPU            1    // Enable single-precision FPU
```

## Memory Management Options

Located in `kernel/portable/MemMang/`:

- **heap_1.c** - Simple, no free() support, deterministic
- **heap_2.c** - Best fit, no coalescence, fragmentation possible
- **heap_3.c** - Wrapper around standard library malloc/free
- **heap_4.c** - First fit with coalescence, **recommended for most applications**
- **heap_5.c** - Heap_4 with multiple non-contiguous heap regions

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
- ARM CMSIS: https://arm-software.github.io/CMSIS_5/
