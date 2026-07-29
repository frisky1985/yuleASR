# Phase 1: Foundation - Design Document

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    Service Layer                            │
│  ┌──────────┐                                              │
│  │   Wdgm   │ ◄── Supervised Entities (SE)                 │
│  └────┬─────┘                                              │
├───────┼────────────────────────────────────────────────────┤
│       │                   ECUAL Layer                       │
│  ┌────┴─────┐                                              │
│  │  WdgIf   │ ◄── Watchdog Interface                       │
│  └────┬─────┘                                              │
├───────┼────────────────────────────────────────────────────┤
│       │                   MCAL Layer                        │
│  ┌────┴─────┐  ┌──────────┐                               │
│  │   Wdg    │  │   Fls    │ ◄── Flash Operations          │
│  └──────────┘  └──────────┘                               │
├─────────────────────────────────────────────────────────────┤
│              Libraries / Common                             │
│  ┌──────────┐                                              │
│  │   Det    │ ◄── Error Reporting                          │
│  └──────────┘                                              │
└─────────────────────────────────────────────────────────────┘
```

## Component Design

### 1. Det (Development Error Tracer)

#### Design Principles
- Centralized error reporting for all BSW modules
- Minimal runtime overhead
- Configurable error hook functions
- Runtime and transient fault support

#### API Design
```c
// Core functions
void Det_Init(const Det_ConfigType* ConfigPtr);
Std_ReturnType Det_ReportError(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 ErrorId
);
void Det_Start(void);
Std_ReturnType Det_ReportRuntimeError(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 ErrorId
);
Std_ReturnType Det_ReportTransientFault(
    uint16 ModuleId,
    uint8 InstanceId,
    uint8 ApiId,
    uint8 FaultId
);
```

#### Error Handling Flow
```
BSW Module → Det_ReportError() → Error Hook → Log/Handle
                                    ↓
                            Optional: Notify Dcm/Dem
```

### 2. Fls (Flash Driver)

#### Design Principles
- Asynchronous operation mode (default)
- Job-based processing
- Sector-based operations
- Error recovery mechanisms

#### State Machine
```
                    ┌──────────────┐
         Init()     │   FLS_IDLE   │
      ┌────────────►│              │◄─────────┐
      │             └──────────────┘          │
      │                    │                  │
      │     Erase/Write    │                  │
      │                    ▼                  │
      │             ┌──────────────┐  Job     │
      │             │  FLS_BUSY    │  Done    │
      │             │              ├──────────┘
      │             └──────────────┘
      │                    │
      │     Error          │
      │                    ▼
      │             ┌──────────────┐
      └─────────────┤  FLS_ERROR   │
                    │              │
                    └──────────────┘
```

#### API Design
```c
// Core functions
void Fls_Init(const Fls_ConfigType* ConfigPtr);
Std_ReturnType Fls_Erase(Fls_AddressType TargetAddress, Fls_LengthType Length);
Std_ReturnType Fls_Write(Fls_AddressType TargetAddress, const uint8* SourceAddress, Fls_LengthType Length);
void Fls_Read(Fls_AddressType SourceAddress, uint8* TargetAddress, Fls_LengthType Length);
Fls_StatusType Fls_GetStatus(void);
Mem_JobResultType Fls_GetJobResult(void);
void Fls_Cancel(void);
void Fls_MainFunction(void);
```

### 3. WdgIf + Wdgm

#### Design Principles
- Layered watchdog management
- Multiple supervised entities
- Configurable supervision strategies
- Integration with EcuM for mode management

#### Wdgm Supervision Types
1. **Alive Supervision** - Periodic checkpoint expected
2. **Deadline Supervision** - Time between checkpoints monitored
3. **Logical Supervision** - Checkpoint sequence verification

#### Architecture
```
┌─────────────────────────────────────┐
│  Application / SWCs                 │
│  (via RTE)                          │
└─────────────┬───────────────────────┘
              │ WdgM_CheckpointReached()
┌─────────────▼───────────────────────┐
│  Wdgm (Watchdog Manager)            │
│  - Supervised Entities (SE)         │
│  - Supervision cycles               │
└─────────────┬───────────────────────┘
              │ WdgIf_SetTriggerCondition()
┌─────────────▼───────────────────────┐
│  WdgIf (Watchdog Interface)         │
│  - Hardware abstraction             │
└─────────────┬───────────────────────┘
              │ Wdg_SetTriggerCondition()
┌─────────────▼───────────────────────┐
│  Wdg (MCAL)                         │
│  - Hardware watchdog                │
└─────────────────────────────────────┘
```

## Testing Strategy

### Unit Testing (TDD)
1. Red phase: Write failing test
2. Green phase: Implement minimum code to pass
3. Refactor: Improve code quality

### Integration Testing
- Det with Dcm/Dem integration
- Fls with Fee integration
- Wdgm with EcuM integration

### Scenario Testing (OpenSpec)
```gherkin
Scenario: Flash write operation
  Given the Fls module is initialized
  When a write job is started
  Then the job should complete successfully
  And the written data should match

Scenario: Watchdog deadline violation
  Given Wdgm is monitoring an SE
  When deadline is exceeded
  Then watchdog should trigger system reset
```

## Configuration Strategy

### Post-build Configuration
- Det_Cfg.h / Det_LCfg.c
- Fls_Cfg.h / Fls_LCfg.c
- WdgIf_Cfg.h
- Wdgm_Cfg.h / Wdgm_LCfg.c

### Configuration Parameters
- Error hook functions (Det)
- Flash sector layout (Fls)
- Supervision periods and thresholds (Wdgm)
