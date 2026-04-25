# Project Optimization Update

**Date**: 2025-04-25  
**Change**: Retain FreeRTOS Kernel in Repository

---

## Revised Decision: Keep FreeRTOS Kernel

Based on requirement that OS directly calls FreeRTOS Kernel APIs, the kernel **must remain** in the repository.

### Current FreeRTOS Integration

```
src/platform/freertos/
├── kernel/                      # FreeRTOS V10.6.2 LTS
│   ├── tasks.c                   # Core scheduler
│   ├── queue.c                   # IPC mechanisms
│   ├── list.c                    # Kernel lists
│   ├── timers.c                  # Software timers
│   ├── event_groups.c            # Event groups
│   ├── stream_buffer.c           # Stream buffers
│   ├── croutine.c                # Co-routines
│   ├── include/                  # Public headers
│   ├── portable/                 # Architecture ports
│   │   ├── GCC/
│   │   │   ├── ARM_CM4F/            # Cortex-M4F with FPU
│   │   │   ├── ARM_CM7/r0p1/      # Cortex-M7
│   │   │   ├── ARM_CM33/          # Cortex-M33 TrustZone
│   │   │   ├── ARM_CM33_NTZ/      # Cortex-M33 no TZ
│   │   │   └── ...
│   │   └── MemMang/
│   │       ├── heap_1.c         # No free
│   │       ├── heap_2.c         # Best fit
│   │       ├── heap_3.c         # Use libc
│   │       ├── heap_4.c         # First fit (used)
│   │       └── heap_5.c         # Multiple regions
│   └── LICENSE.md               # MIT License
├── config/                      # FreeRTOSConfig.h variants
│   ├── FreeRTOSConfig.h           # Default config
│   ├── FreeRTOSConfig_CM7.h       # CM7 optimized
│   └── FreeRTOSConfig_CM33.h      # CM33 with TrustZone
├── port/                        # Platform abstraction
│   ├── os_port.c                  # OS abstraction layer
│   └── port.h                     # Unified port header
├── CMakeLists.txt              # Multi-arch build config
└── README.md                   # Integration guide
```

**Size**: 9.0 MB | **Files**: 318

---

## Alternative Optimizations (Instead of Removing Kernel)

### 1. Trim Unused Ports

**Current**: All GCC ports included (~50 port variations)  
**Needed**: Only 5 ports

**Optimization**: Remove unused architecture ports

```bash
# Keep only required ports
KEEP_PORTS=(
    "ARM_CM4F"
    "ARM_CM7"
    "ARM_CM33"
    "ARM_CM33_NTZ"
    "ThirdParty/GCC/Posix"
)

# Remove other ports (optional, saves ~2MB)
# cd src/platform/freertos/kernel/portable/GCC
# rm -rf ARM_CM0 ARM_CM3 ARM_CM23 ... (unused)
```

**Impact**: -2MB, -50 files (optional)

### 2. Optimize CMake Configuration

**Current**: Selects sources based on FREERTOS_PORT  
**Already Optimal**: Only compiles selected port sources

**Verified in CMakeLists.txt**:
```cmake
# Only includes selected port sources
if(FREERTOS_PORT STREQUAL "ARM_CM7")
    list(APPEND FREERTOS_SOURCES
        ${FREERTOS_KERNEL_DIR}/portable/GCC/ARM_CM7/r0p1/port.c
    )
elseif(FREERTOS_PORT STREQUAL "ARM_CM4F")
    # Only CM4F port
endif()
```

✅ **Already optimized** - only selected port is compiled

### 3. Documentation Instead of Removal

**Instead of**: Removing kernel  
**Do**: Document integration architecture

```markdown
# FreeRTOS Integration Architecture

## Design Decision
FreeRTOS Kernel V10.6.2 LTS is vendored into the repository to ensure:
1. **Build reproducibility** - Fixed version across all builds
2. **Offline builds** - No external dependencies
3. **OS API compatibility** - Direct kernel API access
4. **Safety certification** - Controlled, auditable code base

## Version
- FreeRTOS V10.6.2 LTS (Long Term Support)
- Released: 2023-10
- Support until: 2026-10

## Supported Architectures
| Port | Target MCU | Use Case |
|------|------------|----------|
| ARM_CM4F | STM32F4, S32K1 | General purpose |
| ARM_CM7 | STM32H7, S32G3 | High performance |
| ARM_CM33 | STM32U5, AURIX TC3xx | Security/TrustZone |
| ARM_CM33_NTZ | Same as above | No TrustZone |
| POSIX | Linux/x86 | Simulation/testing |

## Build Integration
```
# Multi-architecture build
mkdir build && cd build
cmake .. -DFREERTOS_PORT=ARM_CM7
make -j$(nproc)
```

## OS Abstraction Layer
The `port/os_port.c` provides unified interface:
- Task creation/deletion
- Semaphore/mutex operations  
- Queue operations
- Timer services
- Interrupt management
```

### 4. .gitattributes for Diff Performance

```bash
# .gitattributes
# Mark FreeRTOS kernel as generated (skip diff in PRs)
src/platform/freertos/kernel/** linguist-generated=true

# But keep our config and port visible
src/platform/freertos/config/** linguist-generated=false
src/platform/freertos/port/** linguist-generated=false
```

**Benefit**: GitHub PRs won't show full FreeRTOS diffs

### 5. Shallow Clone Instructions

For faster clones, document shallow clone:

```bash
# For developers who don't modify kernel
git clone --depth 1 https://github.com/frisky1985/yuleASR.git

# For full history (kernel modifications)
git clone https://github.com/frisky1985/yuleASR.git
```

---

## Revised Critical Issues (Updated)

| ID | Issue | Priority | Action |
|----|-------|----------|--------|
| **C1** | RAM ECC missing types | P0 | Fix this week |
| **C2** | RAM ECC missing tests | P0 | Fix this week |
| **C3** | ~~Remove FreeRTOS~~ | ~~P1~~ | **CANCELLED** |
| **C4** | Documentation consolidation | P2 | Proceed |

---

## Updated Optimization Roadmap

### Phase 1 (Week 1-2): Critical Fixes Only
- [ ] Fix C1: RAM ECC type definitions
- [ ] Fix C2: RAM ECC double-bit tests
- [ ] ~~Remove FreeRTOS~~ → **Skip**
- [ ] Document FreeRTOS integration

### Phase 2-5: Unchanged
- Build system modularization
- CI/CD setup
- Testing expansion
- Documentation

---

## Recommended FreeRTOS Maintenance

### Version Management
```bash
# When updating FreeRTOS:
cd src/platform/freertos/kernel

# 1. Check current version
git log --oneline -1

# 2. Fetch new LTS
curl -L https://github.com/FreeRTOS/FreeRTOS-Kernel/archive/refs/tags/V10.6.3.tar.gz | tar xz

# 3. Update version notes
echo "10.6.3" > VERSION
git add -A
git commit -m "Update FreeRTOS to V10.6.3"
```

### Configuration Management
```c
// config/FreeRTOSConfig.h
// Common settings for all architectures

#define configUSE_PREEMPTION                    1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCPU_CLOCK_HZ                      (SystemCoreClock)
#define configTICK_RATE_HZ                      (1000)
#define configMAX_PRIORITIES                    (16)
#define configMINIMAL_STACK_SIZE                (128)
#define configMAX_TASK_NAME_LEN                 (16)
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS            1
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_QUEUE_SETS                    0
#define configUSE_TIME_SLICING                  1
#define configUSE_NEWLIB_REENTRANT              0

// Memory allocation
#define configSUPPORT_STATIC_ALLOCATION         1
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configTOTAL_HEAP_SIZE                   (64 * 1024)
#define configAPPLICATION_ALLOCATED_HEAP        0

// Hook functions
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

// Run time stats
#define configGENERATE_RUN_TIME_STATS           1
#define configUSE_TRACE_FACILITY                1

// Co-routines
#define configUSE_CO_ROUTINES                   0

// Software timers
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            configMINIMAL_STACK_SIZE

// Interrupt nesting
#define configKERNEL_INTERRUPT_PRIORITY         (7 << 5)
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    (5 << 5)
```

---

## Summary

| Aspect | Original Plan | Revised Plan |
|--------|---------------|--------------|
| FreeRTOS | Remove, use submodule | **Keep vendored** |
| Repo size | -9MB | Keep +9MB |
| Build complexity | Add submodule step | **No change** |
| Offline builds | Requires network | **Fully offline** |
| Safety audit | Track submodule | **Audit local code** |

**Decision**: Keeping FreeRTOS kernel vendored is the **correct choice** for:
- Embedded automotive systems
- Safety-critical applications (ASIL-D)
- Reproducible builds
- Offline development

The alternative optimizations (trim unused ports, .gitattributes, documentation) provide sufficient repository management without sacrificing functionality.

---

**Action**: Proceed with remaining optimizations (C1, C2, C4) while retaining FreeRTOS kernel.
