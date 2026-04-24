# NVM (Non-Volatile Memory) Module

## Overview

AUTOSAR-compliant Non-Volatile Memory (NvM) module for automotive safety-critical applications (ASIL-D).

This module provides reliable persistent storage management with:
- Block-based data organization
- Dual redundancy for critical data
- CRC32 data integrity checking
- Write verification and retry mechanisms
- Wear leveling for flash memory
- Power loss protection
- Asynchronous operation support

## Features

### Core Management (nvm_core)
- Block management and configuration
- Write protection mechanisms
- Write retry mechanism (configurable)
- Write verification (read-after-write)
- Slow read verification for marginal cell detection
- Cold boot validation
- Safety monitor with ASIL-D compliance

### Redundancy Management (nvm_redundancy)
- Dual block redundancy
- Checksum-based data validation
- Automatic data recovery
- Configuration data partitioning (A/B scheme)
- Partition swap capability

### Block Manager (nvm_block_manager)
- Block configuration table
- Block state management
- CRC32 calculation and validation
- Magic number validation
- Version management
- Automatic recovery on corruption

### Job Queue (nvm_job_queue)
- Asynchronous operation support
- Priority-based job queue
- Batch operations (atomic)
- Job cancellation
- Rollback support
- Completion callbacks

### Storage Interface (nvm_storage_if)
- Flash/EEPROM abstraction
- Page management
- Wear leveling
- Device registration
- Multi-device support

## Directory Structure

```
src/nvm/
├── nvm.h                   # Main header (include this)
├── nvm.c                   # Main implementation
├── nvm_types.h             # Type definitions
├── nvm_core.h/c            # Core management
├── nvm_block_manager.h/c   # Block management
├── nvm_redundancy.h/c      # Redundancy management
├── nvm_job_queue.h/c       # Job queue
├── nvm_storage_if.h/c      # Storage interface
├── example_nvm_usage.c     # Usage example
├── CMakeLists.txt          # Build configuration
└── README.md               # This file
```

## Quick Start

### Basic Usage

```c
#include "nvm.h"

// Initialize NVM module
Nvm_Init();

// Register storage device
Nvm_RegisterStorage(0, NVM_DEV_FLASH_INTERNAL, 0x10000, 0x10000);

// Configure a block
Nvm_ConfigureBlock(0, "CONFIG", NVM_BLOCK_TYPE_CONFIG, 0x10000, 256, true);

// Write data
uint8_t data[256] = { ... };
Nvm_WriteBlock(0, data, sizeof(data));

// Read data
uint8_t readBuffer[256];
Nvm_ReadBlock(0, readBuffer, sizeof(readBuffer));

// Deinitialize
Nvm_Deinit();
```

### Asynchronous Operations

```c
void MyCallback(uint16_t blockId, Nvm_JobType_t jobType, 
                Nvm_ErrorCode_t result, void* userData)
{
    if (result == NVM_OK) {
        // Operation successful
    }
}

// Submit async write
Nvm_WriteBlockAsync(0, data, length, MyCallback, NULL);

// Process jobs in cyclic task
void CyclicTask(void)
{
    Nvm_MainFunction();
}
```

## Configuration

### Block Types
- `NVM_BLOCK_TYPE_DATA` - Regular data storage
- `NVM_BLOCK_TYPE_CONFIG` - Configuration data
- `NVM_BLOCK_TYPE_CALIBRATION` - Calibration parameters
- `NVM_BLOCK_TYPE_DIAGNOSTIC` - Diagnostic data
- `NVM_BLOCK_TYPE_SECURITY` - Security keys/certificates
- `NVM_BLOCK_TYPE_FACTORY` - Factory settings (read-only)

### Protection Levels
- `NVM_PROT_NONE` - No protection
- `NVM_PROT_WRITE` - Write protected
- `NVM_PROT_ERASE` - Erase protected
- `NVM_PROT_ALL` - Full protection

### Job Priorities
- `NVM_PRIO_LOW` - Background tasks
- `NVM_PRIO_NORMAL` - Normal operations
- `NVM_PRIO_HIGH` - Critical operations
- `NVM_PRIO_CRITICAL` - Safety-critical (ASIL-D)

## ASIL-D Safety Features

1. **Data Integrity**
   - CRC32 checksum for all data blocks
   - Magic number validation
   - Version checking

2. **Redundancy**
   - Dual block storage for critical data
   - Automatic recovery from backup
   - A/B partition scheme

3. **Error Detection**
   - Write verification
   - Read-after-write compare
   - Slow read for marginal cells

4. **Fault Tolerance**
   - Write retry mechanism
   - Power loss detection
   - Cold boot validation
   - Block recovery procedures

5. **Safety Monitoring**
   - Operation counters
   - Error tracking
   - Recovery statistics
   - Wear level monitoring

## Build Instructions

### Using CMake

```bash
cd src/nvm
mkdir build && cd build
cmake ..
make
```

### Run Example

```bash
./nvm_example
```

### Integration

Add to your project's CMakeLists.txt:

```cmake
add_subdirectory(src/nvm)
target_link_libraries(your_target PRIVATE nvm)
```

## API Reference

See header files for detailed API documentation:
- `nvm.h` - High-level API
- `nvm_core.h` - Core management
- `nvm_block_manager.h` - Block operations
- `nvm_redundancy.h` - Redundancy features
- `nvm_job_queue.h` - Asynchronous operations
- `nvm_storage_if.h` - Storage abstraction

## Memory Layout

Each NVM block consists of:
```
+------------------+
| Block Header     |  40 bytes
| - Magic (4)      |
| - Version (2)    |
| - Block ID (2)   |
| - Data Len (4)   |
| - Sequence (4)   |
| - CRC32 (4)      |
| - Timestamp (4)  |
| - Reserved (16)  |
+------------------+
| User Data        |  Variable
+------------------+
```

## Limitations

- Maximum 64 blocks (`NVM_MAX_BLOCK_COUNT`)
- Maximum 32 pending jobs (`NVM_MAX_JOB_QUEUE`)
- Maximum 256 bytes per page
- Maximum 16 sectors per device
- Block name limited to 32 characters

## License

This module is part of the eth-dds-integration project.
See project root for license information.

## Version

- Module Version: 1.0.0
- AUTOSAR Version: 4.4.0
