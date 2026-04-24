# NvM Enhancement - Task Tracking

> **Change:** dev-nvm-enhancement  
> **Module:** NvM (NVRAM Manager)  
> **Date:** 2026-04-24  
> **Author:** Shanghai Yule Electronics Technology Co., Ltd.

---

## Completed Tasks

### OpenSpec Specification
- [x] Create `openspec/changes/dev-nvm-enhancement/specs/` directory
- [x] Write `NvM_spec.md` with:
  - [x] Module overview and key responsibilities
  - [x] Block type descriptions (Native, Redundant, Dataset)
  - [x] Complete API list (Lifecycle, Single Block, System-Level, Status Control, Protection Control, Termination Control)
  - [x] Data type definitions (`NvM_RequestResultType`, `NvM_BlockManagementType`, `NvM_BlockCrcType`, `NvM_BlockDescriptorType`, `NvM_ConfigType`)
  - [x] DET error code table with API usage mapping
  - [x] Configuration parameters (Pre-compile, Block Descriptor)
  - [x] 6 usage scenarios:
    1. ECU startup ReadAll
    2. Application write to Native block
    3. Redundant block recovery
    4. Dataset block A/B switching
    5. WriteOnce block protection
    6. CRC failure and ROM default recovery
  - [x] Dependencies and version history

### Unit Tests
- [x] Create `src/bsw/services/nvm/src/NvM_test.c` with:
  - [x] Mock implementations for `MemIf_Read`, `MemIf_Write`, `MemIf_GetStatus`, `MemIf_InvalidateBlock`, `MemIf_EraseImmediateBlock`, `Det_ReportError`
  - [x] Test configuration (`NvM_Config`) with 3 block descriptors
  - [x] 11 enabled test cases:
    1. `nvm_init_valid_config` - initialization success
    2. `nvm_init_null_config` - NULL config DET error
    3. `nvm_readblock_success` - ReadBlock queues successfully
    4. `nvm_readblock_uninit` - uninitialized DET error
    5. `nvm_writeblock_success` - WriteBlock queues successfully
    6. `nvm_restoreblockdefaults` - ROM data copied to RAM
    7. `nvm_geterrorstatus` - error status retrieval
    8. `nvm_setramblockstatus` - dirty flag set
    9. `nvm_mainfunction_processes_jobs` - MainFunction job processing
    10. `nvm_eraseblock_success` - EraseNvBlock placeholder
    11. `nvm_invalidateblock_success` - InvalidateNvBlock placeholder
  - [x] 6 placeholder tests wrapped in `#if 0` with `TODO: enable after implementation`:
    - `nvm_deinit_success` (NvM_DeInit not implemented)
    - `nvm_writeblock_writeprot` (write protection check not implemented)
    - `nvm_writeblock_once_protection` (WriteOnce check not implemented)
    - `nvm_readall_queues_blocks` (NvM_ReadAll not implemented)
    - `nvm_writeall_queues_dirty_blocks` (NvM_WriteAll not implemented)
    - `nvm_canceljobs_removes_pending` (NvM_CancelJobs not implemented)

---

## Pending / Blocked

- [ ] Full implementation of unimplemented APIs:
  - `NvM_DeInit`
  - `NvM_ReadAll`
  - `NvM_WriteAll`
  - `NvM_ReadPRAMBlock`
  - `NvM_WritePRAMBlock`
  - `NvM_WriteBlockOnce`
  - `NvM_SetDataIndex`
  - `NvM_CancelJobs`
  - `NvM_SetBlockLockStatus`
  - `NvM_SetBlockProtection`
  - `NvM_SetWriteOnceStatus`
  - `NvM_RepairDamagedBlocks`
  - `NvM_KillWriteAll`
  - `NvM_KillReadAll`
- [ ] Enable placeholder tests after implementation
- [ ] Full WriteBlock write-protection and WriteOnce checks
- [ ] Full EraseNvBlock and InvalidateNvBlock implementation (currently placeholders)

---

## Notes

- Test file follows PduR_test.c style and test_framework.h conventions.
- NvM_spec.md follows PduR_spec.md OpenSpec format.
- No modifications made to `NvM.c` or `NvM.h` (implementation owned by another agent).
- `NvM_BlockDescriptorType` in header is missing `DeviceId` field used by `NvM.c`; this is a pre-existing issue to be resolved by the implementation agent.
