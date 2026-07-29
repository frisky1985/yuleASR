# CanTSyn - CAN Time Synchronization

## Overview

CanTSyn implements AUTOSAR CAN Time Synchronization for distributed time synchronization over CAN bus. It enables precise time synchronization between ECUs using SYNC/FUP message pairs.

## Standards

- AUTOSAR SWS CAN Time Synchronization
- IEEE 802.1AS (Time-Sensitive Networking)
- AUTOSAR Classic Platform 4.4.0

## Features

### Time Synchronization
- **Global Time Synchronization** - Master/Slave architecture
- **SYNC Message** - Synchronization trigger (type 0x10)
- **FUP Message** - Follow-up with precise timestamp
- **OFS Message** - Offset correction (type 0x20)

### Time Domains
- Supports up to 4 time domains (0-3)
- Independent synchronization per domain
- Time domain multiplexing on same CAN bus

### Master/Slave Modes
- **Time Master** - Transmits SYNC/FUP messages
- **Time Slave** - Receives and synchronizes to master
- **OCS Support** - Offset Correction Scale handling

## Message Format

### SYNC Message (16 bytes)
```
Byte 0:    Message Type (0x10)
Byte 1:    Time Domain ID
Byte 2-9:  Reserved
Byte 10-15: User Data
```

### FUP Message (16 bytes)
```
Byte 0:    Message Type (0x10)
Byte 1:    Time Domain ID
Byte 2-9:  SGW (Synchronization Gateway) + Reserved
Byte 10:   Time Sec (seconds)
Byte 11-14: Time NS (nanoseconds)
Byte 15:   User Byte
```

## APIs

| API | Function |
|-----|----------|
| `CanTSyn_Init()` | Initialize module |
| `CanTSyn_DeInit()` | Deinitialize module |
| `CanTSyn_GetVersionInfo()` | Get version info |
| `CanTSyn_MainFunction()` | Periodic processing |
| `CanTSyn_Transmit()` | Transmit time message |
| `CanTSyn_RxIndication()` | Reception callback |
| `CanTSyn_TxConfirmation()` | Transmission confirmation |

## Configuration

### Pre-compile
- `CANTSYN_VERSION_INFO_API` - Enable version info
- `CANTSYN_DEV_ERROR_DETECT` - Enable error detection
- `CANTSYN_TIME_DOMAIN_COUNT` - Number of time domains

### Link-time
- Time domain configurations
- Master/Slave assignments
- Message PDU mappings
- Cycle times

## Dependencies

- CanIf (CAN Interface)
- StbM (Synchronized Time Base Manager)
- Os (Operating System)
- DET (Development Error Tracer)

## Usage Example

```c
#include "CanTSyn.h"

void CanTSyn_Example(void)
{
    /* Initialize */
    CanTSyn_Init(&CanTSyn_Config);

    /* Main loop */
    while (1) {
        CanTSyn_MainFunction();
        
        /* Time sync happens automatically:
         * - Master sends SYNC/FUP periodically
         * - Slave receives and updates StbM
         */
    }
}
```

## Time Synchronization Flow

```
Time Master                    CAN Bus                    Time Slave
    |                             |                           |
    |-- SYNC (T0) --------------->|                           |
    |                             |-- SYNC ------------------->|
    |                             |                           |-- Capture T1
    |                             |                           |
    |-- FUP (T0 precise) -------->|                           |
    |                             |-- FUP ------------------->|
    |                             |                           |-- Update StbM
    |                             |                           |
```

## Source Code

- `/home/admin/yuleASR/src/bsw/services/cantsyn/`
  - `include/CanTSyn.h` - Public API
  - `include/CanTSyn_Cfg.h` - Configuration
  - `src/CanTSyn.c` - Implementation (516 lines)
  - `src/CanTSyn_Lcfg.c` - Link-time config

## Tests

- `/home/admin/yuleASR/tests/unit/autosar/services/CanTSyn/`

## References

- AUTOSAR_SWS_CANTimeSynchronization
- IEEE 802.1AS-2020
