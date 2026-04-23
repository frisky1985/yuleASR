# PduR Module Development Tasks

## Overview
Implementation of the PduR (Protocol Data Unit Router) module for the AutoSAR Service Layer.

## Completed Tasks

### Specification
- [x] Create OpenSpec change directory `openspec/changes/dev-pdu-router/`
- [x] Write `PduR_spec.md` with:
  - [x] Module function overview
  - [x] API list (PduR_Init, PduR_DeInit, PduR_GetVersionInfo, PduR_ComTransmit, PduR_CanIfTxConfirmation, PduR_CanIfRxIndication, etc.)
  - [x] Data type definitions (PduR_StateType, PduR_RoutingPathType, etc.)
  - [x] Error handling with DET error codes
  - [x] Configuration parameter descriptions
  - [x] 4 usage scenarios (COM->CanIf TX, CanIf->COM RX, TxConfirmation, Uninit error)

### Source Code
- [x] Update `PduR.h`:
  - [x] Add callback Service IDs (RXINDICATION, TXCONFIRMATION, TRIGGERTRANSMIT)
  - [x] Add module-specific API mappings (PduR_ComTransmit, PduR_CanIfRxIndication, etc.)
- [x] Update `PduR.c`:
  - [x] Add missing includes (string.h, Com.h, Dcm.h, CanIf.h)
  - [x] Fix hardcoded Service IDs to use PDUR_SID_* macros
- [x] Create `PduR_Lcfg.c`:
  - [x] Define destination PDU configurations for 7 routing paths
  - [x] Define routing path configurations (COM->CanIf, CanIf->COM, DCM->CanIf, CanIf->DCM)
  - [x] Define routing path group configurations
  - [x] Export global `PduR_Config` configuration structure
- [x] Update `MemMap.h`:
  - [x] Add PDUR memory section macros for all compiler targets

### Testing
- [x] Create `PduR_test.c`:
  - [x] Mock functions for CanIf, Com, Dcm, Det
  - [x] Test: PduR_Init initialization success
  - [x] Test: PduR_Init NULL config DET error
  - [x] Test: PduR_ComTransmit routes to CanIf_Transmit
  - [x] Test: PduR_ComTransmit returns error when uninitialized
  - [x] Test: PduR_CanIfRxIndication routes to Com_RxIndication
  - [x] Test: PduR_CanIfRxIndication NULL pointer DET error
  - [x] Test: PduR_CanIfTxConfirmation routes to Com_TxConfirmation
  - [x] Test: PduR_CanIfTxConfirmation error when uninitialized
  - [x] Test: PduR_GetVersionInfo returns correct version
  - [x] Test: PduR_ComTransmit with unknown PDU ID returns E_NOT_OK

### Integration
- [x] Verify code follows AutoSAR 4.x naming conventions
- [x] Verify MemMap.h memory partitioning is used
- [x] Verify DET error detection is enabled for development
- [x] Verify consistent code style with existing MCAL/ECUAL modules

## Verification Status
- [x] OpenSpec specification complete
- [x] Source code implementation complete
- [x] Unit tests created
- [ ] Unit tests compiled and executed
- [ ] Integration tests passed

## Notes
- PduR.h and PduR.c existed prior to this change; they were enhanced with missing includes, API mappings, and corrected Service IDs.
- PduR_Cfg.h already existed and was not modified.
- The module-specific APIs (PduR_ComTransmit, PduR_CanIfRxIndication, etc.) are implemented as macros mapping to the generic internal functions.
