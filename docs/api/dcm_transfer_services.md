# DCM Program Transfer Services Implementation

## Overview

This document describes the implementation of UDS Program Transfer Services (0x34-0x37) according to ISO 14229-1:2020.

## Services Implemented

| Service ID | Name | Description | ISO Section |
|:----------:|:-----|:------------|:------------|
| 0x34 | Request Download | Initiates data download to ECU | 10.8 |
| 0x35 | Request Upload | Initiates data upload from ECU | 10.9 |
| 0x36 | Transfer Data | Transfers data blocks | 10.10 |
| 0x37 | Request Transfer Exit | Completes transfer session | 10.11 |

## Module Structure

```
src/bsw/services/dcm/
├── include/
│   ├── Dcm.h              (existing - updated with service IDs)
│   ├── Dcm_Cfg.h          (existing - updated with transfer config)
│   └── dcm_transfer.h     (new - transfer service interface)
└── src/
    ├── Dcm.c              (existing - updated with routing)
    └── dcm_transfer.c     (new - transfer service implementation)
```

## Key Features

### 1. Block Sequence Counter Management

- Counter starts at 1 per ISO 14229-1 requirement
- Wraps from 0xFF to 0x00
- Validated on each TransferData request
- Returns NRC 0x73 (wrongBlockSequenceCounter) on mismatch

### 2. Compression Methods

| Value | Method | Description |
|:-----:|:-------|:------------|
| 0x00 | None | No compression (default) |
| 0x01 | ZLIB | ZLIB compression |
| 0x02 | LZ4 | LZ4 compression |
| 0x03 | LZMA | LZMA compression |

### 3. Encryption Methods

| Value | Method | Description |
|:-----:|:-------|:------------|
| 0x00 | None | No encryption (default) |
| 0x01 | AES128 | AES-128 encryption |
| 0x02 | AES256 | AES-256 encryption |
| 0x03 | RSA | RSA encryption |

### 4. Address Format

The `AddressAndLengthFormatIdentifier` byte encodes:
- Bits 7-4: Memory size length in bytes
- Bits 3-0: Memory address length in bytes

Supported formats:
- 0x11: 1 byte address, 1 byte size
- 0x22: 2 byte address, 2 byte size
- 0x44: 4 byte address, 4 byte size (default)
- 0x88: 8 byte address, 8 byte size

### 5. Memory Address Mapping

The module supports logical to physical address mapping through the `Dcm_MemoryMappingType` structure:

```c
typedef struct {
    uint32 LogicalStartAddr;    /* Logical start address */
    uint32 LogicalEndAddr;      /* Logical end address */
    uint32 PhysicalStartAddr;   /* Physical start address */
    uint8  MemoryType;          /* Flash, RAM, etc. */
    uint8  AccessRights;        /* Read/Write/Execute */
} Dcm_MemoryMappingType;
```

Default mappings:
- Flash: 0x08000000 - 0x081FFFFF (2MB)
- RAM: 0x20000000 - 0x2007FFFF (512KB)

## API Reference

### Initialization

```c
void Dcm_TransferInit(void);
```

Initializes the transfer module state.

### Service Handlers

```c
Std_ReturnType Dcm_TransferProcessRequestDownload(uint8 ProtocolId, const uint8* Data, uint16 Length);
Std_ReturnType Dcm_TransferProcessRequestUpload(uint8 ProtocolId, const uint8* Data, uint16 Length);
Std_ReturnType Dcm_TransferProcessTransferData(uint8 ProtocolId, const uint8* Data, uint16 Length);
Std_ReturnType Dcm_TransferProcessRequestTransferExit(uint8 ProtocolId, const uint8* Data, uint16 Length);
```

### Utility Functions

```c
const Dcm_TransferStatusType* Dcm_GetTransferStatus(void);
void Dcm_TransferReset(void);
void Dcm_IncrementBlockSequenceCounter(void);
Std_ReturnType Dcm_ValidateMemoryRange(uint32 MemoryAddress, uint32 MemorySize, Dcm_TransferDirectionType Direction);
Std_ReturnType Dcm_MapLogicalToPhysicalAddress(uint32 LogicalAddr, uint32* PhysicalAddr);
```

## Callback Interface

The module provides callback functions for application-specific handling:

```c
typedef struct {
    Dcm_ValidateMemoryAddressFncType ValidateMemoryAddress;
    Dcm_WriteMemoryFncType WriteMemory;
    Dcm_ReadMemoryFncType ReadMemory;
    Dcm_TransferExitFncType TransferExit;
    Dcm_DecompressDataFncType DecompressData;
    Dcm_DecryptDataFncType DecryptData;
} Dcm_TransferCallbackType;
```

## Negative Response Codes

| NRC | Name | Description |
|:----|:-----|:------------|
| 0x13 | incorrectMessageLengthOrInvalidFormat | Invalid request length |
| 0x22 | conditionsNotCorrect | Transfer state invalid |
| 0x24 | requestSequenceError | Wrong service sequence |
| 0x31 | requestOutOfRange | Invalid address/size |
| 0x70 | uploadDownloadNotAccepted | Transfer not allowed |
| 0x71 | transferDataSuspended | Transfer suspended |
| 0x72 | generalProgrammingFailure | Write/verify error |
| 0x73 | wrongBlockSequenceCounter | Invalid block counter |

## Usage Example

### Download Sequence

```c
/* 1. Request Download (0x34) */
uint8 downloadReq[] = {
    0x44,                       /* AddressAndLengthFormatIdentifier (4 byte addr/size) */
    0x08, 0x00, 0x00, 0x00,     /* Memory Address: 0x08000000 */
    0x00, 0x00, 0x10, 0x00,     /* Memory Size: 4096 bytes */
    0x00,                       /* Compression: None */
    0x00                        /* Encryption: None */
};
Dcm_TransferProcessRequestDownload(0, downloadReq, sizeof(downloadReq));

/* 2. Transfer Data (0x36) - Multiple blocks */
for (block = 1; block <= numBlocks; block++) {
    uint8 dataBlock[] = {block, /* data */};
    Dcm_TransferProcessTransferData(0, dataBlock, sizeof(dataBlock));
}

/* 3. Request Transfer Exit (0x37) */
Dcm_TransferProcessRequestTransferExit(0, NULL, 0);
```

### Upload Sequence

```c
/* 1. Request Upload (0x35) */
uint8 uploadReq[] = {
    0x44,                       /* AddressAndLengthFormatIdentifier */
    0x08, 0x00, 0x00, 0x00,     /* Memory Address */
    0x00, 0x00, 0x10, 0x00,     /* Memory Size */
    0x00,                       /* Compression: None */
    0x00                        /* Encryption: None */
};
Dcm_TransferProcessRequestUpload(0, uploadReq, sizeof(uploadReq));

/* 2. Transfer Data (0x36) - Request blocks */
for (block = 1; block <= numBlocks; block++) {
    uint8 blockReq[] = {block};
    Dcm_TransferProcessTransferData(0, blockReq, sizeof(blockReq));
    /* Data returned in positive response */
}

/* 3. Request Transfer Exit (0x37) */
Dcm_TransferProcessRequestTransferExit(0, NULL, 0);
```

## Configuration

### Dcm_Cfg.h Additions

```c
/* Transfer configuration */
#define DCM_DATA_TRANSFER_SUPPORT       (STD_ON)
#define DCM_TRANSFER_BLOCK_SIZE         (1024U)
#define DCM_TRANSFER_MAX_BLOCK_SIZE     (4096U)
```

### dcm_transfer.h Configuration

The module provides default configurations that can be overridden:

```c
const Dcm_TransferConfigType Dcm_TransferConfig;
const Dcm_TransferCallbackType Dcm_TransferCallbacks;
const Dcm_MemoryMappingType Dcm_MemoryMappings[];
```

## Testing

Unit tests are provided in `tests/unit/dcm/test_dcm_transfer.c`:

- Transfer initialization
- Block sequence counter wrap-around
- Memory range validation
- Address mapping
- Service handler validation
- Configuration checks

## Integration

The transfer services are integrated into the main DCM module through:

1. Include `dcm_transfer.h` in `Dcm.c`
2. Call `Dcm_TransferInit()` during `Dcm_Init()`
3. Route service IDs 0x34-0x37 to transfer handlers in `Dcm_ProcessRequest()`

## Compliance

This implementation conforms to:
- ISO 14229-1:2020 UDS Protocol
- AutoSAR Classic Platform 4.x DCM specification

## Revision History

| Version | Date | Description |
|:--------|:-----|:------------|
| 1.0.0 | 2026-04-29 | Initial implementation of 0x34-0x37 services |
