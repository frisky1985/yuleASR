# NvM (NVRAM Manager) Module Specification

> **Module:** NvM (NVRAM Manager)  
> **Layer:** Service Layer  
> **Standard:** AutoSAR Classic Platform 4.x  
> **Platform:** NXP i.MX8M Mini  
> **Author:** Shanghai Yule Electronics Technology Co., Ltd.

---

## 1. Module Overview

NvM is a core Service Layer module in the AutoSAR BSW stack. It is responsible for the management of non-volatile (NV) memory data, providing asynchronous read/write services, block management, data consistency protection, and robust error recovery mechanisms.

NvM abstracts the underlying memory hardware (Flash/EEPROM) through the Memory Abstraction Interface (MemIf) and offers a uniform API to upper-layer modules (e.g., Dcm, Dem) and the RTE for persistent data storage.

### Key Responsibilities
- Asynchronous read and write of NV data blocks
- Block management with support for Native, Redundant, and Dataset block types
- Data consistency protection via configurable CRC (8-bit, 16-bit, 32-bit)
- ROM default data restoration on read failure or CRC mismatch
- Write protection and Write-Once protection for critical data
- Job queuing with standard and immediate priority queues
- Multi-block operations (ReadAll / WriteAll) for ECU startup and shutdown

---

## 2. Block Types

### 2.1 Native Block

A single NV block mapped to one RAM mirror and one NV storage area. This is the simplest and most common block type.

- **Use case:** General configuration data, runtime persistent variables
- **Characteristics:** Single point of storage, efficient, minimal overhead

### 2.2 Redundant Block

Two NV blocks (primary and secondary) storing identical data. If the primary copy is corrupted, the secondary copy is used for recovery.

- **Use case:** Safety-critical data requiring high reliability
- **Characteristics:** Automatic failover, increased storage overhead, transparent to the application

### 2.3 Dataset Block

Multiple NV datasets (up to `NVM_NUM_OF_DATASETS`) associated with a single RAM block. The active dataset is selected via `NvM_SetDataIndex`.

- **Use case:** Calibration parameter sets (A/B faces), multiple vehicle configurations
- **Characteristics:** Index-based selection, shared RAM, multiple NV storage areas

---

## 3. API List

### 3.1 Lifecycle APIs

| API | Description |
|-----|-------------|
| `NvM_Init(const NvM_ConfigType* ConfigPtr)` | Initializes the NvM module with the given configuration |
| `NvM_DeInit(void)` | Deinitializes the NvM module |
| `NvM_GetVersionInfo(Std_VersionInfoType* versioninfo)` | Retrieves version information of the module |
| `NvM_MainFunction(void)` | Periodic main function for asynchronous job processing |

### 3.2 Single Block Operation APIs

| API | Description |
|-----|-------------|
| `NvM_ReadBlock(NvM_BlockIdType BlockId, void* NvM_DstPtr)` | Reads a block from NV memory into RAM |
| `NvM_WriteBlock(NvM_BlockIdType BlockId, const void* NvM_SrcPtr)` | Writes a block from RAM to NV memory |
| `NvM_RestoreBlockDefaults(NvM_BlockIdType BlockId, void* NvM_DestPtr)` | Restores block defaults from ROM data |
| `NvM_EraseNvBlock(NvM_BlockIdType BlockId)` | Erases a block in NV memory |
| `NvM_InvalidateNvBlock(NvM_BlockIdType BlockId)` | Invalidates a block in NV memory |

### 3.3 System-Level APIs

| API | Description |
|-----|-------------|
| `NvM_ReadAll(void)` | Reads all configured blocks during ECU startup |
| `NvM_WriteAll(void)` | Writes all dirty blocks during ECU shutdown |
| `NvM_ReadPRAMBlock(NvM_BlockIdType BlockId)` | Reads a permanent RAM block (configured RAM address) |
| `NvM_WritePRAMBlock(NvM_BlockIdType BlockId)` | Writes a permanent RAM block (configured RAM address) |

### 3.4 Status Control APIs

| API | Description |
|-----|-------------|
| `NvM_GetErrorStatus(NvM_BlockIdType BlockId, NvM_RequestResultType* RequestResultPtr)` | Gets the error status of a block |
| `NvM_SetRamBlockStatus(NvM_BlockIdType BlockId, boolean BlockChanged)` | Sets the RAM block dirty flag |
| `NvM_SetDataIndex(NvM_BlockIdType BlockId, uint8 DataIndex)` | Sets the active dataset index for a Dataset block |
| `NvM_CancelJobs(NvM_BlockIdType BlockId)` | Cancels all pending jobs for a block |

### 3.5 Protection Control APIs

| API | Description |
|-----|-------------|
| `NvM_SetBlockLockStatus(NvM_BlockIdType BlockId, boolean BlockLocked)` | Locks or unlocks a block against modification |
| `NvM_SetBlockProtection(NvM_BlockIdType BlockId, boolean ProtectionEnabled)` | Enables or disables write protection |
| `NvM_SetWriteOnceStatus(NvM_BlockIdType BlockId, boolean WriteOnce)` | Sets Write-Once protection status |

### 3.6 Termination Control APIs

| API | Description |
|-----|-------------|
| `NvM_KillWriteAll(void)` | Aborts an ongoing WriteAll operation |
| `NvM_KillReadAll(void)` | Aborts an ongoing ReadAll operation |

---

## 4. Data Types

### 4.1 NvM_RequestResultType

```c
typedef enum {
    NVM_REQ_OK = 0,
    NVM_REQ_NOT_OK,
    NVM_REQ_PENDING,
    NVM_REQ_INTEGRITY_FAILED,
    NVM_REQ_BLOCK_SKIPPED,
    NVM_REQ_NV_INVALIDATED,
    NVM_REQ_CANCELED,
    NVM_REQ_REDUNDANCY_FAILED,
    NVM_REQ_RESTORED_FROM_ROM,
    NVM_REQ_RESTORED_DEFAULTS
} NvM_RequestResultType;
```

### 4.2 NvM_BlockManagementType

```c
typedef enum {
    NVM_BLOCK_NATIVE = 0,
    NVM_BLOCK_REDUNDANT,
    NVM_BLOCK_DATASET
} NvM_BlockManagementType;
```

### 4.3 NvM_BlockCrcType

```c
typedef enum {
    NVM_CRC_NONE = 0,
    NVM_CRC_8,
    NVM_CRC_16,
    NVM_CRC_32
} NvM_BlockCrcType;
```

### 4.4 NvM_BlockDescriptorType

```c
typedef struct {
    NvM_BlockIdType BlockId;
    uint16 BlockBaseNumber;
    NvM_BlockManagementType ManagementType;
    uint8 NumberOfNvBlocks;
    uint8 NumberOfDataSets;
    uint16 NvBlockLength;
    uint16 NvBlockNum;
    uint16 RomBlockNum;
    uint16 InitCallback;
    uint16 JobEndCallback;
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

### 4.5 NvM_ConfigType

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

## 5. Error Handling (DET)

When `NVM_DEV_ERROR_DETECT == STD_ON`, the following development errors are reported via `Det_ReportError`:

| Error Code | Value | Description |
|------------|-------|-------------|
| `NVM_E_NOT_INITIALIZED` | 0x14U | Module not initialized |
| `NVM_E_BLOCK_PENDING` | 0x15U | Block already has a pending job |
| `NVM_E_BLOCK_CONFIG` | 0x16U | Invalid block configuration |
| `NVM_E_PARAM_BLOCK_ID` | 0x0AU | Invalid block ID |
| `NVM_E_PARAM_BLOCK_DATA_IDX` | 0x0BU | Invalid block data index |
| `NVM_E_PARAM_BLOCK_TYPE` | 0x0CU | Invalid block type |
| `NVM_E_PARAM_DATA_IDX` | 0x0DU | Invalid data index |
| `NVM_E_PARAM_POINTER` | 0x0EU | Null pointer passed |
| `NVM_E_BLOCK_LOCKED` | 0x13U | Block is locked |
| `NVM_E_WRITE_PROTECTED` | 0x12U | Block is write protected |

### DET Usage in APIs
- **NvM_Init**: Reports `NVM_E_PARAM_POINTER` if `ConfigPtr == NULL`
- **NvM_ReadBlock**: Reports `NVM_E_NOT_INITIALIZED` if not initialized, `NVM_E_PARAM_POINTER` if `NvM_DstPtr == NULL`, `NVM_E_PARAM_BLOCK_ID` if invalid block ID, `NVM_E_BLOCK_PENDING` if block already has pending job
- **NvM_WriteBlock**: Reports `NVM_E_NOT_INITIALIZED` if not initialized, `NVM_E_PARAM_POINTER` if `NvM_SrcPtr == NULL`, `NVM_E_PARAM_BLOCK_ID` if invalid block ID, `NVM_E_BLOCK_PENDING` if block already has pending job, `NVM_E_WRITE_PROTECTED` if block is write protected
- **NvM_RestoreBlockDefaults**: Reports `NVM_E_NOT_INITIALIZED` if not initialized, `NVM_E_PARAM_POINTER` if `NvM_DestPtr == NULL`, `NVM_E_PARAM_BLOCK_ID` if invalid block ID
- **NvM_GetErrorStatus**: Reports `NVM_E_NOT_INITIALIZED` if not initialized, `NVM_E_PARAM_POINTER` if `RequestResultPtr == NULL`
- **NvM_SetRamBlockStatus**: Reports `NVM_E_NOT_INITIALIZED` if not initialized, `NVM_E_PARAM_BLOCK_ID` if invalid block ID

---

## 6. Configuration Parameters

### Pre-Compile Configuration (NvM_Cfg.h)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `NVM_DEV_ERROR_DETECT` | STD_ON | Enable/disable development error detection |
| `NVM_VERSION_INFO_API` | STD_ON | Enable/disable version info API |
| `NVM_SET_RAM_BLOCK_STATUS_API` | STD_ON | Enable/disable SetRamBlockStatus API |
| `NVM_GET_ERROR_STATUS_API` | STD_ON | Enable/disable GetErrorStatus API |
| `NVM_SET_BLOCK_PROTECTION_API` | STD_ON | Enable/disable SetBlockProtection API |
| `NVM_NUM_OF_NVRAM_BLOCKS` | 32U | Maximum number of NVRAM blocks |
| `NVM_NUM_OF_DATASETS` | 8U | Maximum number of datasets per block |
| `NVM_NUM_OF_ROM_BLOCKS` | 16U | Maximum number of ROM blocks |
| `NVM_MAX_NUMBER_OF_WRITE_RETRIES` | 3U | Maximum write retry attempts |
| `NVM_MAX_NUMBER_OF_READ_RETRIES` | 3U | Maximum read retry attempts |
| `NVM_CALC_RAM_BLOCK_CRC` | STD_ON | Enable RAM block CRC calculation |
| `NVM_USE_CRC_COMP_MECHANISM` | STD_ON | Enable CRC comparison mechanism |
| `NVM_MAIN_FUNCTION_PERIOD_MS` | 10U | Main function period in milliseconds |
| `NVM_SIZE_STANDARD_JOB_QUEUE` | 16U | Standard job queue size |
| `NVM_SIZE_IMMEDIATE_JOB_QUEUE` | 4U | Immediate (high priority) job queue size |

### Block Descriptor Configuration

Each block descriptor defines:
- **BlockId**: Unique identifier for the block
- **BlockBaseNumber**: Base number for NV memory addressing
- **ManagementType**: NATIVE, REDUNDANT, or DATASET
- **NvBlockLength**: Length of the NV block in bytes
- **CrcType**: CRC_NONE, CRC_8, CRC_16, or CRC_32
- **BlockWriteProt**: Write protection flag
- **BlockWriteOnce**: Write-Once protection flag
- **RomBlockData**: Pointer to ROM default data
- **RamBlockData**: Pointer to permanent RAM block data

---

## 7. Scenarios

### Scenario 1: ECU Startup ReadAll

**Description:** During ECU startup, NvM_ReadAll is called to restore all configured NV blocks from non-volatile memory into RAM.

**Flow:**
1. BSW Mode Manager triggers NvM_ReadAll during startup phase
2. NvM iterates over all configured block descriptors
3. For each block, a read job is queued in the standard job queue
4. NvM_MainFunction processes each read job asynchronously
5. MemIf_Read is called to fetch data from the underlying memory device
6. On successful completion, block data is available in RAM
7. If read fails, ROM default data is restored via NvM_CopyRomDataToRam

**Expected Result:** All NV blocks are restored to RAM. Failed blocks are recovered from ROM defaults.

### Scenario 2: Application Write to Native Block

**Description:** Application layer writes updated configuration data to a Native NV block.

**Flow:**
1. Application calls `NvM_WriteBlock(NVM_BLOCK_ID_CONFIG, &configData)`
2. NvM validates the block ID and checks no pending job exists
3. A write job is queued in the standard job queue
4. Application continues execution (asynchronous)
5. NvM_MainFunction pops the job and calls `MemIf_Write`
6. On MemIf completion (MEMIF_IDLE), write counter is incremented
7. `NvM_GetErrorStatus` returns `NVM_REQ_OK`

**Expected Result:** Configuration data is persisted to NV memory asynchronously.

### Scenario 3: Redundant Block Recovery

**Description:** A Redundant block's primary copy is corrupted. NvM detects the failure and recovers from the secondary copy.

**Flow:**
1. NvM_ReadBlock is called for a redundant block
2. MemIf_Read returns integrity failure for the primary copy
3. NvM detects `NVM_REQ_INTEGRITY_FAILED`
4. NvM automatically attempts to read the secondary (redundant) copy
5. Secondary copy read succeeds
6. Data from secondary copy is copied to RAM
7. Request result is `NVM_REQ_OK` (recovered transparently)

**Expected Result:** Data is transparently recovered from the redundant copy without application intervention.

### Scenario 4: Dataset Block A/B Switching

**Description:** Switch between calibration parameter sets (e.g., calibration A and calibration B) using Dataset blocks.

**Flow:**
1. Current active dataset index is 0 (Calibration A)
2. Application calls `NvM_SetDataIndex(NVM_BLOCK_ID_CALIBRATION, 1U)`
3. NvM validates the data index against `NumberOfDataSets`
4. Active dataset index is updated to 1
5. Application calls `NvM_ReadBlock(NVM_BLOCK_ID_CALIBRATION, &calData)`
6. NvM uses dataset index 1 to calculate the NV memory offset
7. Calibration B data is read into RAM

**Expected Result:** Application transparently accesses different datasets via index switching.

### Scenario 5: WriteOnce Block Protection

**Description:** A software version block is configured with WriteOnce protection to prevent rollback attacks.

**Flow:**
1. Block descriptor has `BlockWriteOnce = TRUE`
2. First write: `NvM_WriteBlock(NVM_BLOCK_ID_VIN, &vinData)` succeeds
3. NvM sets internal WriteOnce flag as written
4. Second write attempt: `NvM_WriteBlock(NVM_BLOCK_ID_VIN, &newVinData)`
5. NvM detects WriteOnce already written
6. API returns `E_NOT_OK` and reports `NVM_E_WRITE_PROTECTED` to DET

**Expected Result:** WriteOnce blocks can only be written once, preventing tampering.

### Scenario 6: CRC Failure and ROM Default Recovery

**Description:** A block with CRC protection is read, but the stored CRC does not match the calculated CRC. NvM restores ROM default data.

**Flow:**
1. Block descriptor has `CrcType = NVM_CRC_16` and `BlockUseCrc = TRUE`
2. `NvM_ReadBlock` is called and MemIf_Read returns data
3. NvM_MainFunction detects MEMIF_IDLE (read complete)
4. CRC is calculated over the read data
5. Calculated CRC does not match stored CRC
6. NvM sets result to `NVM_REQ_INTEGRITY_FAILED`
7. NvM_CopyRomDataToRam copies ROM default data to RAM
8. Final result is updated to `NVM_REQ_RESTORED_FROM_ROM`

**Expected Result:** Corrupted NV data is detected and safe ROM defaults are restored.

---

## 8. Dependencies

### Upper Layer Modules
- **RTE**: Run Time Environment for application access
- **Dcm**: Diagnostic Communication Manager for UDS services
- **Dem**: Diagnostic Event Manager for fault memory

### Lower Layer Modules
- **MemIf**: Memory Interface abstraction
- **Fee**: Flash EEPROM Emulation (underlying driver)
- **Ea**: EEPROM Abstraction (underlying driver)

### Common Modules
- **Det**: Development Error Tracer
- **MemMap**: Memory mapping for section placement

---

## 9. Version History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0.0 | 2026-04-24 | YuleTech | Initial NvM specification with 6 scenarios |
