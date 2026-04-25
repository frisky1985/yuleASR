# RAM ECC Protection Module

## Overview

This module provides comprehensive RAM ECC (Error Correction Code) protection for automotive applications, compliant with **ISO 26262 ASIL-D** safety level and **AUTOSAR R22-11** standards.

### Key Features

- **SECDED Algorithm**: Single Error Correction, Double Error Detection
- **ASIL-D Compliance**: Meets automotive safety integrity level requirements
- **Hardware Independent**: Software-based ECC for portability
- **Memory Protection**: Guard words, boundary checks, double-free detection
- **Background Monitoring**: Continuous RAM integrity scanning
- **Safe Memory Operations**: Verified memcpy, memset, memcmp with ECC

---

## Architecture

### Layered Architecture

```
┌─────────────────────────────────────────────┐
│           Application Layer                 │
└─────────────────────────────────────────────┘
                      │
┌─────────────────────────────────────────────┐
│  RamSafety (ram_safety.h/c)                 │
│  ├─ Unified API for all RAM protection      │
│  ├─ Safe memory operations                  │
│  ├─ Memory mapping management               │
│  └─ Safety assertions (ASIL-D)              │
└─────────────────────────────────────────────┘
                      │
┌─────────────────────┬───────────────────────┐
│  RamMonitor         │  EccAllocator         │
│  (ram_monitor.h/c)  │  (ecc_allocator.h/c)  │
│  ├─ Background scan │  ├─ Protected malloc  │
│  ├─ Region mgmt     │  ├─ Guard words       │
│  └─ Error reporting │  └─ ECC per block     │
└─────────────────────┴───────────────────────┘
                      │
┌─────────────────────┬───────────────────────┐
│  EccChecker         │  EccEncoder           │
│  (ecc_checker.h/c)  │  (ecc_encoder.h/c)    │
│  ├─ Error detect    │  ├─ SECDED encoding   │
│  ├─ Auto-correct    │  ├─ 32/64-bit modes   │
│  └─ Statistics      │  └─ Batch encoding    │
└─────────────────────┴───────────────────────┘
```

### Module Responsibilities

| Module | Primary Function | Safety Mechanism |
|--------|------------------|------------------|
| `ecc_encoder` | Generate ECC codes | Hamming SECDED algorithm |
| `ecc_checker` | Detect/correct errors | Syndrome calculation |
| `ecc_allocator` | Safe memory allocation | Guard words + ECC metadata |
| `ram_monitor` | Background scanning | Periodic integrity checks |
| `ram_safety` | Integration layer | Unified API + assertions |

---

## SECDED Implementation

### Algorithm Details

The module implements **Single Error Correction, Double Error Detection** using extended Hamming codes:

**For 32-bit data (ECC7):**
- 6 Hamming parity bits for single-error location
- 1 overall parity bit for double-error detection
- 21.9% memory overhead (7 bits per 32-bit word)

**For 64-bit data (ECC8):**
- 7 Hamming parity bits for single-error location
- 1 overall parity bit for double-error detection
- 12.5% memory overhead (8 bits per 64-bit word)

### Error Detection Capability

| Error Type | Detection | Correction | Syndrome |
|------------|-----------|------------|----------|
| No error | Yes | N/A | 0 |
| Single-bit data | Yes | Yes | Non-zero, valid position |
| Single-bit ECC | Yes | Yes | Non-zero, valid position |
| Double-bit | Yes | No | Non-zero, parity bit 0 |
| Multi-bit (>2) | Maybe | No | Unpredictable |

### Syndrome Interpretation

```
Syndrome = Computed_ECC XOR Stored_ECC

Bit 6 (P64 for 32-bit) / Bit 7 (P128 for 64-bit):
  = 1: Odd number of errors (single-bit, correctable)
  = 0: Even number of errors (double-bit, uncorrectable)
```

---

## API Usage Guide

### 1. Initialization

#### Basic Initialization
```c
#include "ram_safety.h"

void init_ram_safety(void) {
    RamSafetyConfigType config;
    memset(&config, 0, sizeof(config));
    
    /* Configure safety level */
    config.check_mode = RAM_SAFETY_CHECK_FULL;
    config.asil_level = ASIL_D;
    
    /* Enable submodules */
    config.enable_monitor = TRUE;
    config.enable_allocator = TRUE;
    config.enable_timer_checks = TRUE;
    config.timer_interval_ms = 100;
    
    /* Encoder configuration */
    config.encoder_config.mode = ECC_MODE_32BIT;
    config.encoder_config.asil_level = ASIL_D;
    
    /* Checker configuration */
    config.checker_config.auto_correct = TRUE;
    config.checker_config.log_errors = TRUE;
    
    /* Monitor configuration */
    config.monitor_config.max_regions = 16;
    config.monitor_config.auto_start = TRUE;
    
    /* Initialize */
    if (RamSafety_Init(&config) != E_OK) {
        /* Handle initialization failure */
        Error_Handler();
    }
}
```

#### Deinitialization
```c
void shutdown_ram_safety(void) {
    RamSafety_DeInit();
}
```

### 2. ECC Encoding/Decoding

#### Direct ECC Operations
```c
/* Encode 32-bit data */
uint32_t data = 0x12345678;
uint8_t ecc_code;

if (EccEncoder_Encode32(data, &ecc_code) == E_OK) {
    /* Store data and ecc_code together */
}

/* Check and correct data */
EccCheckResultType result;
if (EccChecker_CheckAndCorrect32(corrupted_data, ecc_code, &result) == E_OK) {
    if (result.error_type == ECC_ERROR_SINGLE_BIT) {
        /* Use result.corrected_data */
        printf("Corrected bit %d\n", result.error_position);
    }
}
```

#### Batch Encoding
```c
uint32_t data_array[100];
uint8_t ecc_array[100];

/* Fill data_array... */

/* Encode all at once */
EccEncoder_BatchEncode32(data_array, ecc_array, 100);
```

### 3. Safe Memory Allocation

#### Basic Allocation
```c
/* Allocate ECC-protected memory */
void *ptr = RamSafety_Malloc(256);
if (ptr == NULL) {
    /* Handle allocation failure */
}

/* Use memory */
memset(ptr, 0, 256);

/* Free memory */
RamSafety_Free(ptr);
```

#### Advanced Allocation with Flags
```c
#include "ecc_allocator.h"

/* Allocate with specific flags */
void *ptr = EccAllocator_Malloc(
    1024,
    ECC_ALLOC_FLAG_ECC |      /* Enable ECC */
    ECC_ALLOC_FLAG_GUARD |    /* Enable guard words */
    ECC_ALLOC_FLAG_ZERO       /* Zero memory */
);

/* Aligned allocation */
void *aligned = EccAllocator_AlignedAlloc(
    1024,
    32,                       /* 32-byte alignment */
    ECC_ALLOC_FLAG_ECC
);
```

#### Reallocation
```c
void *new_ptr = EccAllocator_Realloc(
    old_ptr,
    new_size,
    ECC_ALLOC_FLAG_ECC
);
```

### 4. Safe Memory Operations

#### Safe Memory Copy
```c
/* Basic safe copy */
RamSafety_MemCpy(dst, src, size, RAM_OP_FLAG_NONE);

/* Copy with source verification */
RamSafety_MemCpy(dst, src, size, RAM_OP_FLAG_VERIFY_SRC);

/* Copy with destination verification */
RamSafety_MemCpy(dst, src, size, RAM_OP_FLAG_VERIFY_DST);

/* Full verification */
RamSafety_MemCpy(dst, src, size, 
    RAM_OP_FLAG_VERIFY_SRC | 
    RAM_OP_FLAG_VERIFY_DST |
    RAM_OP_FLAG_BARRIER
);
```

#### Safe Memory Set
```c
/* Set with verification */
RamSafety_MemSet(ptr, 0xAA, 256, RAM_OP_FLAG_VERIFY_DST);
```

#### Safe Memory Compare
```c
/* Compare with ECC verification */
int32_t result = RamSafety_MemCmp(ptr1, ptr2, size, 
    RAM_OP_FLAG_VERIFY_SRC);

if (result == 0) {
    /* Memory regions are equal */
} else if (result > 0) {
    /* ptr1 > ptr2 */
} else {
    /* ptr1 < ptr2 */
}
```

### 5. Memory Region Monitoring

#### Register a Region
```c
RamRegionType region;
memset(&region, 0, sizeof(region));

region.region_id = 1;
region.start_address = 0x20000000;  /* Example RAM address */
region.size_bytes = 65536;           /* 64 KB */
region.word_size = 4;                /* 32-bit words */
region.attributes = RAM_REGION_ATTR_CRITICAL | RAM_REGION_ATTR_SAFEGUARD;
region.scan_interval_ms = 100;       /* 100ms interval */
region.priority = 10;                /* High priority */
region.enabled = TRUE;

/* Provide ECC storage or set to NULL for compute-on-fly */
static uint8_t ecc_storage[65536 / 4];
region.ecc_storage = ecc_storage;

if (RamMonitor_RegisterRegion(&region) != E_OK) {
    /* Handle registration failure */
}
```

#### Manual Region Scan
```c
/* Scan specific region */
EccCheckResultType result;
if (RamMonitor_ScanRegion(1, &result) != E_OK) {
    /* Errors detected */
    printf("Errors in region 1: %s\n", 
           EccChecker_GetErrorString(result.error_type));
}

/* Scan all regions */
uint32_t error_count = RamMonitor_ScanAll();
if (error_count > 0) {
    printf("Found errors in %d regions\n", error_count);
}

/* Scan critical regions only */
error_count = RamMonitor_ScanCritical();
```

### 6. Memory Mapping

```c
/* Initialize memory map */
RamSafety_InitMemoryMap();

/* Add mapping */
RamSafety_MapMemory(
    0x10000000,   /* Virtual address */
    0x20000000,   /* Physical address */
    4096,         /* Size */
    0             /* Attributes */
);

/* Translate addresses */
uint32_t phys_addr;
if (RamSafety_VirtToPhys(virt_addr, &phys_addr) == E_OK) {
    /* Use physical address */
}
```

---

## Error Handling Procedures

### Error Types

```c
typedef enum {
    ECC_ERROR_NONE = 0,         /* No error */
    ECC_ERROR_SINGLE_BIT,       /* Single-bit error (correctable) */
    ECC_ERROR_DOUBLE_BIT,       /* Double-bit error (uncorrectable) */
    ECC_ERROR_MULTI_BIT,        /* Multi-bit error (uncorrectable) */
    ECC_ERROR_ECC_CORRUPT       /* ECC code corrupted */
} EccErrorType;
```

### Error Callback Registration

```c
void SafetyErrorHandler(
    uint32_t error_code,
    const char *func,
    const char *file,
    uint32_t line
) {
    /* Log to NV memory */
    LogError(error_code, func, file, line);
    
    /* Notify safety manager */
    SafetyManager_Notify(error_code);
    
    /* Enter safe state for critical errors */
    if (error_code == E_SAFETY_ECC_DOUBLE_BIT) {
        EnterSafeState();
    }
}

/* Register callback during initialization */
RamSafety_SetErrorCallback(SafetyErrorHandler);
```

### Statistics Monitoring

```c
/* Get ECC checker statistics */
const EccErrorStatsType *stats = EccChecker_GetStats();

printf("Total checks: %lu\n", stats->total_checks);
printf("Single-bit errors: %lu\n", stats->single_bit_errors);
printf("Double-bit errors: %lu\n", stats->double_bit_errors);
printf("Error rate: %lu per 1000\n", stats->error_rate);

/* Get allocator statistics */
const EccAllocStatsType *alloc_stats = EccAllocator_GetStats();

printf("Allocations: %lu\n", alloc_stats->total_allocations);
printf("Current used: %lu bytes\n", alloc_stats->current_used);
printf("ECC errors: %lu\n", alloc_stats->ecc_errors);
```

### Safety Assertions (ASIL-D)

```c
/* Enable assertions */
#define RAM_SAFETY_ENABLE_ASSERTS

/* Use assertions */
RAM_SAFETY_ASSERT(ptr != NULL);
RAM_SAFETY_ASSERT_ALIGN(ptr, 8);
RAM_SAFETY_ASSERT_RANGE(size, 1, MAX_SIZE);
```

### Recommended Error Handling Flow

```
Error Detected
      │
      ▼
┌─────────────┐
│ Log Error   │ ──► Write to NVM
└─────────────┘
      │
      ▼
┌─────────────┐
│ Classify    │ ──► Single-bit / Double-bit / Other
└─────────────┘
      │
      ├─────────────┬─────────────┐
      ▼             ▼             ▼
┌──────────┐  ┌──────────┐  ┌──────────┐
│ Single   │  │ Double   │  │ Other    │
│ Correct  │  │ Report   │  │ Analyze  │
└──────────┘  └──────────┘  └──────────┘
      │             │             │
      ▼             ▼             ▼
Continue     Enter Safe    Depends on
Operation    State         Severity
```

---

## Performance Characteristics

### Timing (Typical)

| Operation | Cycles | Time @100MHz | Notes |
|-----------|--------|--------------|-------|
| ECC Encode 32-bit | 15-20 | 150-200 ns | Hamming matrix |
| ECC Encode 64-bit | 25-35 | 250-350 ns | Extended Hamming |
| ECC Check 32-bit | 20-30 | 200-300 ns | Syndrome calc |
| Single-bit correct | 30-40 | 300-400 ns | + bit flip |
| Malloc (small) | 200-500 | 2-5 us | With guards |
| Free | 150-300 | 1.5-3 us | Coalescing check |
| Region scan (1KB) | 1500-2000 | 15-20 us | Full ECC verify |

### Memory Overhead

| Component | Overhead | Calculation |
|-----------|----------|-------------|
| ECC storage (32-bit) | 21.9% | 7 bits / 32 bits |
| ECC storage (64-bit) | 12.5% | 8 bits / 64 bits |
| Allocator per block | ~48 bytes | Header + metadata |
| Guard words | 4-8 bytes | Front + rear |
| Monitor regions | 64 bytes each | Region table entry |

### Code Size

| Module | Size (KB) |
|--------|-----------|
| ecc_encoder | ~1 |
| ecc_checker | ~2 |
| ecc_allocator | ~4 |
| ram_monitor | ~3 |
| ram_safety | ~2 |
| **Total** | **~12** |

### RAM Usage

| Component | Size | Notes |
|-----------|------|-------|
| Static state | ~100 bytes | All modules |
| Heap (configurable) | 256 KB default | User data + ECC |
| Region table | ~1 KB | 16 regions max |
| Statistics | ~100 bytes | Counters |

---

## Integration Examples

### Example 1: Basic DDS Application

```c
#include "ram_safety.h"
#include "dds_application.h"

int main(void) {
    /* Initialize RAM safety first */
    RamSafetyConfigType safety_config = {
        .check_mode = RAM_SAFETY_CHECK_FULL,
        .enable_allocator = TRUE,
        .asil_level = ASIL_D
    };
    RamSafety_Init(&safety_config);
    
    /* Allocate DDS buffers with ECC */
    DdsBufferType *tx_buffer = RamSafety_Malloc(sizeof(DdsBufferType));
    DdsBufferType *rx_buffer = RamSafety_Malloc(sizeof(DdsBufferType));
    
    /* Main loop */
    while (1) {
        /* Periodic safety check */
        if (RamSafety_QuickCheck() != E_OK) {
            HandleSafetyError();
        }
        
        /* Process DDS messages */
        ProcessDdsMessages(tx_buffer, rx_buffer);
        
        /* Full check periodically */
        static uint32_t last_full_check = 0;
        if (GetTick() - last_full_check > 1000) {
            RamSafety_FullCheck();
            last_full_check = GetTick();
        }
    }
}
```

### Example 2: RTOS Integration

```c
#include "FreeRTOS.h"
#include "task.h"
#include "ram_safety.h"

void RamMonitor_Task(void *pvParameters) {
    (void)pvParameters;
    
    for (;;) {
        /* Scan all regions */
        uint32_t errors = RamMonitor_ScanAll();
        
        if (errors > 0) {
            /* Notify system task */
            xTaskNotify(system_task_handle, errors, eSetValueWithOverwrite);
        }
        
        /* Delay before next scan */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void Safety_Init(void) {
    /* Initialize RAM safety */
    RamSafety_Init(&safety_config);
    
    /* Create monitor task */
    xTaskCreate(
        RamMonitor_Task,
        "RamMon",
        configMINIMAL_STACK_SIZE * 2,
        NULL,
        tskIDLE_PRIORITY + 2,
        NULL
    );
}
```

### Example 3: Critical Data Protection

```c
typedef struct {
    uint32_t timestamp;
    uint32_t sensor_value;
    uint32_t checksum;
} CriticalSensorData;

static CriticalSensorData *sensor_data;
static uint8_t sensor_ecc;

void InitCriticalData(void) {
    /* Allocate in protected memory */
    sensor_data = RamSafety_Malloc(sizeof(CriticalSensorData));
    
    /* Register as critical region */
    RamRegionType region = {
        .region_id = REGION_SENSOR_DATA,
        .start_address = (uint32_t)sensor_data,
        .size_bytes = sizeof(CriticalSensorData),
        .attributes = RAM_REGION_ATTR_CRITICAL | RAM_REGION_ATTR_SAFEGUARD,
        .scan_interval_ms = 50,  /* Fast scan for critical data */
        .priority = 100
    };
    RamMonitor_RegisterRegion(&region);
}

void UpdateSensorData(uint32_t value) {
    /* Safe write with verification */
    sensor_data->timestamp = GetTimestamp();
    sensor_data->sensor_value = value;
    sensor_data->checksum = CalculateChecksum(sensor_data);
    
    /* Regenerate ECC */
    RamSafety_Encode32(*(uint32_t*)sensor_data, &sensor_ecc);
}
```

---

## Configuration Options

### CMake Options

```cmake
# Enable RAM safety module (default: ON)
set(ENABLE_SAFETY_RAM ON)

# Set heap size (default: 256KB)
set(ECC_HEAP_SIZE 262144)

# Enable assertions (default: ON for debug)
set(RAM_SAFETY_ENABLE_ASSERTS ON)
```

### Compile-Time Configuration

```c
/* In ecc_allocator.h */
#define ECC_ALLOC_HEAP_SIZE         (256U * 1024U)  /* 256KB */
#define ECC_ALLOC_ALIGN_DEFAULT     8U
#define ECC_ALLOC_MAX_ALLOCATIONS   1024U

/* In ram_monitor.h */
#define RAM_MONITOR_MAX_REGIONS     16U
#define RAM_MONITOR_INTERVAL_FAST   10U
#define RAM_MONITOR_INTERVAL_NORMAL 100U
```

---

## Testing

### Build and Run Tests

```bash
# Build test executable
gcc -I./src -I./src/common/types -I./src/common/utils \
    -DENABLE_SAFETY_RAM \
    src/safety/ram/test_ram_ecc.c \
    src/safety/ram/ecc_encoder.c \
    src/safety/ram/ecc_checker.c \
    src/safety/ram/ecc_allocator.c \
    src/safety/ram/ram_monitor.c \
    src/safety/ram/ram_safety.c \
    -o test_ram_ecc

# Run tests
./test_ram_ecc

# Expected output:
# ========================================
# RAM ECC Protection Module Test Suite
# ========================================
# 
# === Testing ECC Encoder ===
# [PASS] ECC Encoder initialization
# [PASS] ECC Encode32 basic
# ...
# Test Summary: XX passed, 0 failed
```

### Test Coverage

| Test Category | Coverage | Status |
|---------------|----------|--------|
| ECC encoding | Full | PASS |
| Single-bit correction | Full | PASS |
| Allocator basic | Full | PASS |
| Double-free detection | Full | PASS |
| Double-bit detection | Partial | IMPROVE |
| Fault injection | Basic | IMPROVE |

---

## Troubleshooting

### Common Issues

#### Issue: Initialization Failure
```
Symptom: RamSafety_Init() returns E_NOT_OK
Causes:
  - NULL configuration pointer
  - Invalid ASIL level
  - Insufficient memory
Solution:
  - Check all config parameters
  - Verify heap size is sufficient
  - Check return codes of submodules
```

#### Issue: ECC Errors on Valid Data
```
Symptom: False ECC error detection
Causes:
  - ECC code not stored/retrieved correctly
  - Memory corruption
  - Mismatched data/ECC pairs
Solution:
  - Verify ECC storage alignment
  - Check for buffer overruns
  - Validate data/ECC pairing
```

#### Issue: Allocator Returns NULL
```
Symptom: RamSafety_Malloc() returns NULL
Causes:
  - Heap exhausted
  - Fragmentation
  - Block size too large
Solution:
  - Check heap statistics
  - Implement defragmentation
  - Verify allocation size
```

---

## File Structure

```
src/safety/ram/
├── ecc_encoder.h         # SECDED encoding API
├── ecc_encoder.c         # Encoder implementation
├── ecc_checker.h         # Error detection/correction API
├── ecc_checker.c         # Checker implementation
├── ecc_allocator.h       # Protected allocator API
├── ecc_allocator.c       # Allocator implementation
├── ram_monitor.h         # Background monitoring API
├── ram_monitor.c         # Monitor implementation
├── ram_safety.h          # Unified safety interface
├── ram_safety.c          # Integration implementation
├── test_ram_ecc.c        # Unit tests
├── README.md             # This file
└── docs/
    └── RAM_ECC_DESIGN_REVIEW.md  # Design review document
```

---

## References

1. ISO 26262-1:2018 - Road vehicles - Functional safety
2. ISO 26262-5:2018 - Product development at the hardware level
3. ISO 26262-6:2018 - Product development at the software level
4. AUTOSAR R22-11 Specification
5. Hamming, R.W. - Error Detecting and Error Correcting Codes (1950)

---

## License

See project root for license information.

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2024 | Initial release |
