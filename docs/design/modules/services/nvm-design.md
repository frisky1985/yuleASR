> **Module ID**: 0x14  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR_SWS_NVRAMManager  
> **Source Path**: `src/bsw/services/nvm/`  
> **Reference Document**: `docs/modules/NVM.md`  
> **Doc Version**: 1.0  
> **Status**: Approved

# NvM (NVRAM Manager) Design Document

## 1. Module Overview

The NVRAM Manager (NvM) is an AUTOSAR Service Layer module that provides non-volatile data storage services to application software and other BSW modules. It manages the persistence of RAM blocks by reading from and writing to underlying memory abstraction modules (`MemIf` → `Fee` / `Ea`). NvM supports native, redundant, and dataset block management types, optional CRC protection, write protection, write-once semantics, and bulk startup/shutdown operations (`ReadAll` / `WriteAll`).

### 1.1 Position in the Stack

```
┌─────────────────────────────────────────────────────────┐
│  Application / RTE / Dcm / Dem / EcuM / BswM            │
├─────────────────────────────────────────────────────────┤
│  NvM (Services)                                         │
├─────────────────────────────────────────────────────────┤
│  MemIf (Services)                                       │
├─────────────────────────────────────────────────────────┤
│  Fee (ECUAL) ──► Flash Driver (MCAL)                    │
│  Ea  (ECUAL) ──► EEPROM Driver (MCAL)                   │
└─────────────────────────────────────────────────────────┘
```

---

## 2. Standards & Dependencies

### 2.1 Standards

| Standard | Version | Description |
|----------|---------|-------------|
| AUTOSAR SWS NVRAM Manager | 4.4.0 | NvM specification |
| AUTOSAR SWS Memory Stack | 4.4.0 | MemIf / Fee / Ea specifications |

### 2.2 Dependencies

| Module | Direction | Purpose |
|--------|-----------|---------|
| MemIf | Lower | Hardware-independent memory interface |
| Fee / Ea | Lower | Flash/EEPROM abstraction |
| Det | Common | Development error detection |
| Dem / EcuM / Dcm | Upper/Peer | Consumers of persistent data |

---

## 3. Architecture Design

### 3.1 Internal Components

| Component | Responsibility |
|-----------|----------------|
| Job Queue Manager | Maintains standard and immediate job queues; schedules one job per `NvM_MainFunction` cycle |
| Block Manager | Looks up block descriptors and validates block IDs |
| Read/Write Engine | Calls `MemIf_Read` / `MemIf_Write`, appends/validates CRC, handles redundant copies |
| Restore Manager | Copies ROM default data to RAM when NV read fails or is invalid |
| Erase/Invalidate Manager | Issues `MemIf_EraseImmediateBlock` / `MemIf_InvalidateBlock` |
| Multi-block Manager | Implements `ReadAll` / `WriteAll` and `KillReadAll` / `KillWriteAll` |
| CRC Engine | Computes/verifies CRC-8/16/32 over block payloads |
| Block State Manager | Tracks per-block job pending, data changed, write-once, lock, and last result states |

### 3.2 File Structure

```
src/bsw/services/nvm/
├── include/
│   ├── NvM.h              # Public API and types
│   ├── NvM_Cfg.h          # Pre-compile configuration
│   ├── NvM_Private.h      # Legacy private types (partially superseded by NvM.c internals)
│   ├── NvM_EccHandler.h   # ECC handler interface
│   └── NvM_MemMap.h       # Memory mapping header
└── src/
    ├── NvM.c              # Core implementation
    ├── NvM_Redundant.c    # Redundant block handling
    ├── NvM_EccHandler.c   # ECC handler
    ├── NvM_EccHandler_Cfg.c
    └── NvM_test.c         # Unit tests
```

---

## 4. State Machines

### 4.1 Module State

```
UNINIT ──NvM_Init──► IDLE ──job start──► BUSY ──job complete──► IDLE
```

- `NVM_STATE_UNINIT`: module not initialized.
- `NVM_STATE_IDLE`: ready to accept and process queued jobs.
- `NVM_STATE_BUSY`: a standard job has been submitted to `MemIf` and is awaiting completion.

### 4.2 Job State

```
PENDING ──pop from queue──► PROCESSING ──MemIf completes──► IDLE
```

- `NVM_JOB_STATE_PENDING`: queued but not yet started.
- `NVM_JOB_STATE_PROCESSING`: submitted to `MemIf`.
- `NVM_JOB_STATE_IDLE`: job finished (success or failure).

### 4.3 Block Runtime State

Per-block state tracks:

- `LastResult` — result of the most recent request.
- `JobPending` — whether a job is currently queued or running for this block.
- `DataValid` / `DataChanged` — validity and dirty flags.
- `DataIndex` — selected dataset index for dataset blocks.
- `BlockLocked` — runtime lock status.
- `WriteOnceDone` — write-once protection flag.

---

## 5. Core Data Structures

### 5.1 Job Queue Entry

```c
typedef struct
{
    NvM_BlockIdType BlockId;
    uint8 JobType;
    uint8 JobState;
    void* DataPtr;
    NvM_RequestResultType Result;
    uint8 RetryCount;
    uint8 CopyIndex;
} NvM_JobQueueEntryType;
```

### 5.2 Block Runtime State

```c
typedef struct
{
    NvM_RequestResultType LastResult;
    uint8 JobPending;
    uint8 WriteCounter;
    boolean DataValid;
    boolean DataChanged;
    uint8 DataIndex;
    boolean BlockLocked;
    boolean WriteOnceDone;
} NvM_BlockStateType;
```

### 5.3 Module Internal State

```c
typedef struct
{
    uint8 State;
    const NvM_ConfigType* ConfigPtr;
    NvM_JobQueueEntryType StandardQueue[NVM_SIZE_STANDARD_JOB_QUEUE];
    uint8 StandardQueueHead;
    uint8 StandardQueueTail;
    uint8 StandardQueueCount;
    NvM_JobQueueEntryType ImmediateQueue[NVM_SIZE_IMMEDIATE_JOB_QUEUE];
    uint8 ImmediateQueueHead;
    uint8 ImmediateQueueTail;
    uint8 ImmediateQueueCount;
    NvM_BlockStateType BlockStates[NVM_NUM_OF_NVRAM_BLOCKS];
    NvM_JobQueueEntryType ActiveJob;
    NvM_JobQueueEntryType* CurrentJob;
    boolean ReadAllInProgress;
    boolean WriteAllInProgress;
    boolean KillReadAllRequested;
    boolean KillWriteAllRequested;
    uint16 ReadAllPendingCount;
    uint16 WriteAllPendingCount;
} NvM_InternalStateType;
```

### 5.4 Block Descriptor

```c
typedef struct {
    NvM_BlockIdType BlockId;
    uint8 DeviceId;
    uint16 BlockBaseNumber;
    NvM_BlockManagementType ManagementType;
    uint8 NumberOfNvBlocks;
    uint8 NumberOfDataSets;
    uint16 NvBlockLength;
    uint16 NvBlockNum;
    uint16 RomBlockNum;
    void (*InitCallback)(void);
    void (*JobEndCallback)(void);
    NvM_BlockCrcType CrcType;
    boolean BlockUseCrc;
    boolean BlockUseSetRamBlockStatus;
    boolean BlockWriteProt;
    boolean BlockWriteOnce;
    boolean BlockAutoValidation;
    boolean BlockUseMirror;
    boolean BlockUseCompression;
    const void* RomBlockData;
    void* RamBlockData;
    void* MirrorBlockData;
} NvM_BlockDescriptorType;
```

### 5.5 Root Configuration

```c
typedef struct {
    const NvM_BlockDescriptorType* BlockDescriptors;
    uint16 NumBlockDescriptors;
    uint16 NumOfNvBlocks;
    uint16 NumOfDataSets;
    uint16 NumOfRomBlocks;
    uint16 MaxNumberOfWriteRetries;
    uint16 MaxNumberOfReadRetries;
    boolean DevErrorDetect;
    boolean VersionInfoApi;
    boolean SetRamBlockStatusApi;
    boolean GetErrorStatusApi;
    boolean SetBlockProtectionApi;
    boolean GetBlockProtectionApi;
    boolean SetDataIndexApi;
    boolean GetDataIndexApi;
    boolean CancelJobApi;
    boolean KillWriteAllApi;
    boolean KillReadAllApi;
    boolean RepairDamagedBlocksApi;
    boolean CalcRamBlockCrc;
    boolean UseCrcCompMechanism;
    uint16 MainFunctionPeriod;
} NvM_ConfigType;
```

---

## 6. API Design

### 6.1 Public Interfaces

| API | Signature | Function | SID | SWS 需求 |
|-----|-----------|----------|-----|----------|
| `NvM_Init` | `void NvM_Init(const NvM_ConfigType* ConfigPtr)` | Initialize NvM | 0x00 | SWS_NvM_00001 |
| `NvM_SetDataIndex` | `Std_ReturnType NvM_SetDataIndex(NvM_BlockIdType BlockId, uint8 DataIndex)` | Select dataset | 0x01 | SWS_NvM_00005 |
| `NvM_GetErrorStatus` | `Std_ReturnType NvM_GetErrorStatus(NvM_BlockIdType BlockId, NvM_RequestResultType* RequestResultPtr)` | Read last result | 0x04 | SWS_NvM_00011 |
| `NvM_SetRamBlockStatus` | `Std_ReturnType NvM_SetRamBlockStatus(NvM_BlockIdType BlockId, boolean BlockChanged)` | Mark dirty | 0x05 | SWS_NvM_00012 |
| `NvM_ReadBlock` | `Std_ReturnType NvM_ReadBlock(NvM_BlockIdType BlockId, void* NvM_DstPtr)` | Read block | 0x06 | SWS_NvM_00002 |
| `NvM_WriteBlock` | `Std_ReturnType NvM_WriteBlock(NvM_BlockIdType BlockId, const void* NvM_SrcPtr)` | Write block | 0x07 | SWS_NvM_00003 |
| `NvM_RestoreBlockDefaults` | `Std_ReturnType NvM_RestoreBlockDefaults(NvM_BlockIdType BlockId, void* NvM_DestPtr)` | Restore ROM defaults | 0x08 | SWS_NvM_00004 |
| `NvM_EraseNvBlock` | `Std_ReturnType NvM_EraseNvBlock(NvM_BlockIdType BlockId)` | Erase block | 0x09 | SWS_NvM_00013 |
| `NvM_InvalidateNvBlock` | `Std_ReturnType NvM_InvalidateNvBlock(NvM_BlockIdType BlockId)` | Invalidate block | 0x0B | SWS_NvM_00014 |
| `NvM_ReadAll` | `Std_ReturnType NvM_ReadAll(void)` | Read all permanent RAM blocks | 0x0C | SWS_NvM_00016 |
| `NvM_WriteAll` | `Std_ReturnType NvM_WriteAll(void)` | Write all dirty RAM blocks | 0x0D | SWS_NvM_00017 |
| `NvM_ReadPRAMBlock` | `Std_ReturnType NvM_ReadPRAMBlock(NvM_BlockIdType BlockId)` | Read permanent RAM block | 0x16 | SWS_NvM_00107 |
| `NvM_WritePRAMBlock` | `Std_ReturnType NvM_WritePRAMBlock(NvM_BlockIdType BlockId)` | Write permanent RAM block | 0x17 | SWS_NvM_00108 |
| `NvM_WriteBlockOnce` | `Std_ReturnType NvM_WriteBlockOnce(NvM_BlockIdType BlockId, const void* NvM_SrcPtr)` | Write-once block | 0x0F | SWS_NvM_00006 |
| `NvM_CancelJobs` | `Std_ReturnType NvM_CancelJobs(NvM_BlockIdType BlockId)` | Cancel queued jobs | 0x10 | SWS_NvM_00106 |
| `NvM_SetBlockLockStatus` | `Std_ReturnType NvM_SetBlockLockStatus(NvM_BlockIdType BlockId, boolean BlockLocked)` | Lock/unlock block | 0x13 | SWS_NvM_00007 |
| `NvM_SetBlockProtection` | `Std_ReturnType NvM_SetBlockProtection(NvM_BlockIdType BlockId, boolean ProtectionEnabled)` | Runtime protection | 0x14 | SWS_NvM_00008 |
| `NvM_KillWriteAll` | `void NvM_KillWriteAll(void)` | Abort WriteAll | 0x19 | SWS_NvM_00018 |
| `NvM_KillReadAll` | `void NvM_KillReadAll(void)` | Abort ReadAll | 0x1A | SWS_NvM_00019 |
| `NvM_GetVersionInfo` | `void NvM_GetVersionInfo(Std_VersionInfoType* versioninfo)` | Version info | 0x0F | SWS_NvM_00010 |
| `NvM_MainFunction` | `void NvM_MainFunction(void)` | Periodic processing | 0x1C | SWS_NvM_00015 |

### 6.2 Service IDs and DET Error Codes

Key DET error codes from `NvM.h`:

| Code | Name | Trigger |
|------|------|---------|
| 0x0A | `NVM_E_PARAM_BLOCK_ID` | Invalid block ID |
| 0x0B | `NVM_E_PARAM_BLOCK_DATA_IDX` | Invalid data index |
| 0x0C | `NVM_E_PARAM_BLOCK_TYPE` | Operation not valid for block type |
| 0x0D | `NVM_E_PARAM_DATA_IDX` | Invalid dataset index |
| 0x0E | `NVM_E_PARAM_POINTER` | NULL pointer argument |
| 0x12 | `NVM_E_WRITE_PROTECTED` | Write protected or write-once |
| 0x13 | `NVM_E_BLOCK_LOCKED` | Block is locked |
| 0x14 | `NVM_E_NOT_INITIALIZED` | Module not initialized |
| 0x15 | `NVM_E_BLOCK_PENDING` | Job already pending for block |
| 0x16 | `NVM_E_BLOCK_CONFIG` | Invalid block configuration |

---

## 7. Processing Flows

### 7.1 Single Block Read

1. Validate state, block ID, and destination pointer.
2. Check no job is already pending for the block.
3. Push a `NVM_JOB_TYPE_READ` entry into the standard queue.
4. In `NvM_MainFunction`, pop the job and call `NvM_ProcessReadJob`.
5. `NvM_ProcessReadJob` calls `MemIf_Read(DeviceId, BlockNumber, Offset, DataPtr, Length + CrcSize)`.
6. On `MemIf` completion, verify CRC if enabled.
7. If CRC mismatch: try redundant copy, then fall back to ROM defaults (`NVM_REQ_INTEGRITY_FAILED` / `NVM_REQ_RESTORED_FROM_ROM`).
8. Invoke `JobEndCallback` and release the block.

### 7.2 Single Block Write

1. Validate block ID, source pointer, and write protection (write-protected, write-once, or locked blocks are rejected).
2. Queue a `NVM_JOB_TYPE_WRITE` entry.
3. In `NvM_MainFunction`, pop and call `NvM_ProcessWriteJob`.
4. Compute CRC and append it to the data buffer if configured.
5. Call `MemIf_Write(DeviceId, BlockNumber, DataPtr)`.
6. For redundant blocks, repeat for the second copy on success.
7. On completion, clear `DataChanged`; for write-once blocks set `WriteOnceDone`.

### 7.3 Restore Block Defaults

1. Queue a high-priority `NVM_JOB_TYPE_RESTORE` entry in the immediate queue.
2. Copy `RomBlockData` to the destination RAM buffer.
3. Return `NVM_REQ_OK`.

### 7.4 ReadAll / WriteAll

- `NvM_ReadAll`: iterates all configured blocks with `RamBlockData != NULL`, calls `NvM_ReadBlock`, and counts pending jobs. Sets `ReadAllInProgress`.
- `NvM_WriteAll`: iterates all dirty blocks (`DataChanged == TRUE`) and calls `NvM_WriteBlock`. Sets `WriteAllInProgress`.
- Batch flags are cleared when the last pending job completes.

### 7.5 Kill Operations

`NvM_KillReadAll` / `NvM_KillWriteAll` set request flags. In the next `NvM_MainFunction`, all matching READ/WRITE jobs are removed from the standard queue, block states are set to `NVM_REQ_CANCELED`, and batch flags are cleared.

### 7.6 MainFunction Scheduling

```
1. Handle KillReadAll / KillWriteAll requests.
2. If IDLE:
   a. Pop immediate queue (restore jobs) and process synchronously.
   b. Else pop standard queue and submit to MemIf → state = BUSY.
3. If BUSY:
   a. Poll MemIf status.
   b. On MEMIF_IDLE, evaluate job result.
   c. Retry on failure up to MaxNumberOfReadRetries / MaxNumberOfWriteRetries.
   d. Finalize job, update block state, invoke callback, return to IDLE.
```

---

## 8. Configuration Design

### 8.1 Pre-compile Configuration (`NvM_Cfg.h`)

| Macro | Value | Description |
|-------|-------|-------------|
| `NVM_DEV_ERROR_DETECT` | `STD_ON` | DET enabled |
| `NVM_VERSION_INFO_API` | `STD_ON` | Version info API |
| `NVM_NUM_OF_NVRAM_BLOCKS` | 32 | Total NVRAM blocks |
| `NVM_NUM_OF_DATASETS` | 8 | Dataset instances |
| `NVM_NUM_OF_ROM_BLOCKS` | 16 | ROM default blocks |
| `NVM_REDUNDANT_STORAGE_ENABLED` | `STD_ON` | Redundant storage enabled |
| `NVM_NUM_REDUNDANT_BLOCKS` | 8 | Redundant block copies |
| `NVM_MAX_BLOCK_SIZE` | 1024 | Maximum block size |
| `NVM_MAIN_FUNCTION_PERIOD_MS` | 10 | MainFunction period |
| `NVM_SIZE_STANDARD_JOB_QUEUE` | 16 | Standard job queue depth |
| `NVM_SIZE_IMMEDIATE_JOB_QUEUE` | 4 | Immediate job queue depth |
| `NVM_MAX_NUMBER_OF_WRITE_RETRIES` | 3 | Write retry limit |
| `NVM_MAX_NUMBER_OF_READ_RETRIES` | 3 | Read retry limit |
| `NVM_CALC_RAM_BLOCK_CRC` | `STD_ON` | CRC calculation enabled |
| `NVM_USE_CRC_COMP_MECHANISM` | `STD_ON` | CRC comparison enabled |
| `NVM_SET_RAM_BLOCK_STATUS_API` | `STD_ON` | Dirty-mark API |
| `NVM_GET_ERROR_STATUS_API` | `STD_ON` | Error-status API |
| `NVM_SET_BLOCK_PROTECTION_API` | `STD_ON` | Protection API |
| `NVM_CANCEL_JOB_API` | `STD_ON` | Cancel API |

### 8.2 Block Management Types

| Type | Description |
|------|-------------|
| `NVM_BLOCK_NATIVE` | One NV block maps to one RAM block |
| `NVM_BLOCK_REDUNDANT` | Two NV copies; second used on CRC/read failure |
| `NVM_BLOCK_DATASET` | Multiple NV blocks selectable by data index |

### 8.3 CRC Types

| Type | Size | Polynomial |
|------|------|------------|
| `NVM_CRC_8` | 1 byte | 0x1D |
| `NVM_CRC_16` | 2 bytes | 0x1021 |
| `NVM_CRC_32` | 4 bytes | 0x04C11DB7 |
| `NVM_CRC_NONE` | 0 | No CRC |

---

## 9. Error Handling & Safety

### 9.1 Request Result Codes

| Code | Meaning |
|------|---------|
| `NVM_REQ_OK` | Request completed successfully |
| `NVM_REQ_NOT_OK` | Request failed |
| `NVM_REQ_PENDING` | Request queued or in progress |
| `NVM_REQ_INTEGRITY_FAILED` | NV data CRC mismatch |
| `NVM_REQ_BLOCK_SKIPPED` | Block skipped during multi-block op |
| `NVM_REQ_NV_INVALIDATED` | Block marked invalid |
| `NVM_REQ_CANCELED` | Job canceled |
| `NVM_REQ_REDUNDANCY_FAILED` | Both redundant copies failed |
| `NVM_REQ_RESTORED_FROM_ROM` | Defaults restored after read failure |
| `NVM_REQ_RESTORED_DEFAULTS` | Defaults restored explicitly |

### 9.2 Safety Mechanisms

- Per-block `JobPending` flag prevents concurrent jobs on the same block.
- Write protection, block locking, and write-once flags prevent unintended writes.
- CRC verification on read detects data corruption.
- Redundant storage provides a fallback copy.
- ROM default fallback ensures a valid RAM image even when NV is corrupt or empty.
- Retry counters bound the time spent retrying failed `MemIf` operations.

---

## 10. Memory & Performance

### 10.1 MemMap Sections

| Section | Usage |
|---------|-------|
| `NVM_START_SEC_VAR_CLEARED_UNSPECIFIED` | `NvM_InternalState` |
| `NVM_START_SEC_CONFIG_DATA_UNSPECIFIED` | Link-time block/configuration tables |
| `NVM_START_SEC_CODE` | Module code |

### 10.2 Resource Estimation

| Resource | Estimate | Note |
|----------|----------|------|
| RAM | `NVM_SIZE_STANDARD_JOB_QUEUE*entry + NVM_SIZE_IMMEDIATE_JOB_QUEUE*entry + NVM_NUM_OF_NVRAM_BLOCKS*state + block buffers` | Job queues and block states |
| ROM | Block descriptor table + default ROM data + code | Scales with block count |
| Stack | Low | No recursion; local buffers reused |
| CPU | One job step per `NvM_MainFunction` cycle | Polling-based, every 10 ms |

---

## 11. Integration Guide

### 11.1 Lower Layer (MemIf / Fee / Ea)

- NvM calls `MemIf_Read`, `MemIf_Write`, `MemIf_EraseImmediateBlock`, `MemIf_InvalidateBlock`, `MemIf_GetStatus`, and `MemIf_GetJobResult`.
- `NvM_JobEndNotification` and `NvM_JobErrorNotification` are provided as no-op callbacks; the current implementation uses polling in `NvM_MainFunction`.

### 11.2 Upper Layers

- Application software calls `NvM_ReadBlock` / `NvM_WriteBlock` for explicit persistence.
- `NvM_ReadAll` is typically called by `EcuM` during startup.
- `NvM_WriteAll` is typically called by `EcuM` / `BswM` during shutdown.

### 11.3 Initialization Order

1. Initialize underlying Flash/EEPROM drivers and `MemIf`.
2. Call `NvM_Init(&NvM_Config)`.
3. Call `NvM_ReadAll()` to restore permanent RAM blocks.
4. Run `NvM_MainFunction()` cyclically until all startup jobs complete.

---

## 12. Test Strategy

### 12.1 Unit Tests

| Scenario | Coverage |
|----------|----------|
| Init/DeInit | NULL config, double init, state transitions |
| Read/Write | Native, redundant, and dataset blocks |
| CRC | CRC-8/16/32 verification and mismatch handling |
| Restore | ROM fallback after read failure |
| Write protection | Locked, write-protected, and write-once blocks |
| ReadAll/WriteAll | Multi-block scheduling and Kill operations |
| Error status | `NvM_GetErrorStatus` after each operation |

### 12.2 Integration Tests

| Scenario | Purpose |
|----------|---------|
| Startup recovery | `NvM_ReadAll` restores all configured blocks |
| Shutdown flush | `NvM_WriteAll` persists dirty blocks |
| Power-loss during write | Verify redundant/RAM fallback behavior |
| Flash wear simulation | Retry and error propagation to `MemIf` |

---

## 13. Implementation Notes / TODO

- **Job notification callbacks**: `NvM_JobEndNotification` and `NvM_JobErrorNotification` are currently no-ops; the module relies on polling. A future interrupt-driven `MemIf` integration should update the active job state from these callbacks.
- **Compression**: `BlockUseCompression` is defined but no compression/decompression is implemented.
- **Mirror blocks**: `MirrorBlockData` is present in descriptors but the main read/write path uses the caller-supplied or `RamBlockData` pointer directly.
- **CancelJobs**: `NvM_CancelJobs` is declared but not fully implemented in the current source.
- **RepairDamagedBlocks**: API is declared but disabled (`NVM_REPAIR_DAMAGED_BLOCKS_API` = `STD_OFF`).
- **Private header mismatch**: `NvM_Private.h` defines an older `NvM_InternalStateType`; the active implementation in `NvM.c` uses the larger runtime structure shown above. Code that includes both must not rely on the private header's state definition.
- **Block numbering**: `BlockBaseNumber` plus dataset/redundant index is passed to `MemIf`; the link-time configuration must align these numbers with the Fee/Ea block layout.

---

## 14. References

1. AUTOSAR_SWS_NVRAMManager.pdf — AUTOSAR Classic Platform 4.4.0
2. `docs/modules/NVM.md`
3. `src/bsw/services/nvm/include/NvM.h`
4. `src/bsw/services/nvm/include/NvM_Cfg.h`
5. `src/bsw/services/nvm/include/NvM_Private.h`
6. `src/bsw/services/nvm/src/NvM.c`
7. `src/bsw/services/nvm/src/NvM_Redundant.c`
