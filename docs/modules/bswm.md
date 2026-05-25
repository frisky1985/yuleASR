# BSWM - BSW Mode Manager

## Overview

BSWM (Basic Software Mode Manager) is a key AUTOSAR Services layer module responsible for coordinating mode transitions across the ECU software stack. It provides a centralized mechanism for managing operational modes and executing associated rules and actions based on mode requests from various sources.

## Standards

- AUTOSAR SWS BSW Mode Manager
- AUTOSAR Classic Platform 4.4.0
- MISRA C:2012

## Features

### Mode Management
- **Mode Request Ports** - Receive mode requests from different sources
- **Mode Rules** - Define conditions for mode-based actions
- **Action Lists** - Execute sequences of actions based on rule evaluation
- **Mode Arbitration** - Coordinate conflicting mode requests

### Request Sources
- **EcuM** - ECU State Manager (startup, run, shutdown, sleep)
- **ComM** - Communication Manager (communication modes)
- **DCM** - Diagnostic Communication Manager (diagnostic sessions)
- **Generic** - Application-specific mode requests
- **NvM** - Non-Volatile Memory (block write protection)

### Rule Processing
- **Rule Evaluation** - Evaluate rules based on current mode states
- **Action Execution** - Execute actions when rules are satisfied
- **Configurable Rules** - Statically configured rules at build time
- **Main Function Processing** - Periodic rule evaluation in MainFunction

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                  Application Software (ASW)                     │
│                  (Mode Request through RTE)                    │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       v
┌─────────────────────────────────────────────────────────────┐
│              BSWM - BSW Mode Manager                            │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Mode Request Ports                                  │   │
│  │  - EcuM State Port                                   │   │
│  │  - ComM Mode Port                                    │   │
│  │  - DCM Mode Port                                     │   │
│  │  - Generic Request Ports                             │   │
│  └──────────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Rule Engine                                         │   │
│  │  - Evaluate Rules                                    │   │
│  │  - Match Mode Conditions                             │   │
│  │  - Trigger Action Lists                              │   │
│  └──────────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Action Execution                                    │   │
│  │  - Mode Switches                                     │   │
│  │  - Module Scheduling                                 │   │
│  │  - User-defined Actions                              │   │
│  └──────────────────────────────────────────────────────┘   │
└──────┬──────────────┬──────────────┬────────────────────────┘
       │              │              │
       v              v              v
┌──────────┐  ┌──────────┐  ┌──────────┐
│   EcuM   │  │   ComM   │  │   DCM    │
└──────────┘  └──────────┘  └──────────┘
```

## Mode Request Sources

| Source | Description | Typical Modes |
|--------|-------------|---------------|
| EcuM | ECU State | STARTUP, RUN, SHUTDOWN, SLEEP |
| ComM | Communication | FULL_COM, SILENT_COM, NO_COM |
| DCM | Diagnostic | DEFAULT, PROGRAMMING, EXTENDED |
| Generic | Application-defined | User-defined modes |

## Mode Definitions

| Mode | Value | Description |
|------|-------|-------------|
| BSWM_MODE_STARTUP | 0x00 | System startup mode |
| BSWM_MODE_RUN | 0x01 | Normal operation mode |
| BSWM_MODE_SHUTDOWN | 0x02 | System shutdown mode |
| BSWM_MODE_SLEEP | 0x03 | Low power sleep mode |
| BSWM_MODE_WAKEUP | 0x04 | Wakeup transition mode |

## ECU States

| State | Value | Description |
|-------|-------|-------------|
| BSWM_ECUM_STATE_STARTUP | 0x10 | ECU startup phase |
| BSWM_ECUM_STATE_RUN | 0x20 | ECU run phase |
| BSWM_ECUM_STATE_SHUTDOWN | 0x30 | ECU shutdown phase |
| BSWM_ECUM_STATE_SLEEP | 0x40 | ECU sleep phase |

## Action Types

| Action Type | Description |
|-------------|-------------|
| BSWM_ACTION_SCHEDULE | Schedule a BSW module |
| BSWM_ACTION_SWITCH_MODE | Perform a mode switch |
| BSWM_ACTION_EXECUTE_ACTION_LIST | Execute another action list |
| BSWM_ACTION_USER_CALL | Call user-defined function |

## APIs

### Core APIs

| API | Function |
|-----|----------|
| `BswM_Init()` | Initialize BSWM module |
| `BswM_DeInit()` | Deinitialize BSWM module |
| `BswM_GetVersionInfo()` | Get version information |
| `BswM_MainFunction()` | Main processing function (periodic) |

### Mode Request APIs

| API | Function |
|-----|----------|
| `BswM_RequestMode()` | Request a mode change (generic port) |
| `BswM_EcuM_CurrentState()` | Report ECU state change |
| `BswM_ComM_CurrentMode()` | Report ComM mode change |
| `BswM_Dcm_RequestCommunicationMode()` | Report DCM communication mode |

### Helper Functions (Internal)

| API | Function |
|-----|----------|
| `BswM_IsInitialized()` | Check if BSWM is initialized |
| `BswM_GetCurrentMode()` | Get current mode for a port |

## Configuration

### Pre-compile Configuration

| Parameter | Description |
|-----------|-------------|
| `BSWM_DEV_ERROR_DETECT` | Enable development error detection |
| `BSWM_VERSION_INFO_API` | Enable version info API |
| `BSWM_MAX_MODE_REQUEST_PORTS` | Maximum number of mode request ports |
| `BSWM_MAX_RULES` | Maximum number of rules |
| `BSWM_MAX_ACTIONS` | Maximum number of actions |
| `BSWM_MAX_ACTION_LISTS` | Maximum number of action lists |

### Mode Request Port Configuration

```c
const BswM_ModeRequestPortType modeRequestPorts[] = {
    {
        .PortId = 0,
        .SourceType = BSWM_ECUM_REQUEST,
        .CurrentMode = BSWM_MODE_STARTUP,
        .IsValid = TRUE
    },
    {
        .PortId = 1,
        .SourceType = BSWM_COMM_REQUEST,
        .CurrentMode = BSWM_MODE_RUN,
        .IsValid = TRUE
    }
};
```

### Rule Configuration

```c
const BswM_RuleType rules[] = {
    {
        .RuleId = 0,
        .ModeRequestPortId = 0,
        .ExpectedMode = BSWM_MODE_RUN,
        .ActionListId = 0,
        .IsActive = TRUE
    }
};
```

### Module Configuration

```c
const BswM_ConfigType bswmConfig = {
    .NumModeRequestPorts = 2,
    .NumRules = 1,
    .NumActionLists = 1,
    .ModeRequestPorts = modeRequestPorts,
    .Rules = rules
};
```

## Usage Examples

### Basic Initialization

```c
#include "BswM.h"

void InitializeBswM(void)
{
    BswM_Init(&bswmConfig);
}
```

### Requesting a Mode Change

```c
void EnterRunMode(void)
{
    /* Request RUN mode on generic port 0 */
    BswM_RequestMode(0, BSWM_MODE_RUN);
}
```

### Processing Mode Changes

```c
void BswM_CyclicTask(void)
{
    /* Called periodically (e.g., every 10ms) */
    BswM_MainFunction();
}
```

### Handling ECU State Changes

```c
void EcuM_StateChangeCallback(EcuM_StateType newState)
{
    /* Forward ECU state to BSWM */
    BswM_EcuM_CurrentState(newState);
}
```

### Complete Mode Transition Example

```c
typedef enum {
    APP_MODE_INIT,
    APP_MODE_NORMAL,
    APP_MODE_DIAGNOSTIC,
    APP_MODE_SLEEP
} AppModeType;

void PerformModeTransition(AppModeType targetMode)
{
    switch (targetMode) {
        case APP_MODE_NORMAL:
            /* Request communication mode */
            BswM_RequestMode(PORT_COMM_MODE, BSWM_MODE_RUN);
            break;
            
        case APP_MODE_DIAGNOSTIC:
            /* Request diagnostic mode */
            BswM_RequestMode(PORT_DIAG_MODE, BSWM_MODE_RUN);
            break;
            
        case APP_MODE_SLEEP:
            /* Request sleep preparation */
            BswM_RequestMode(PORT_SYS_MODE, BSWM_MODE_SHUTDOWN);
            break;
            
        default:
            break;
    }
    
    /* Process the mode request */
    BswM_MainFunction();
}
```

### Rule-Based Action Configuration

```c
/* Example: Enable DCM processing when in diagnostic mode */
const BswM_RuleType diagnosticRule = {
    .RuleId = RULE_ENABLE_DCM,
    .ModeRequestPortId = PORT_DIAG_MODE,
    .ExpectedMode = BSWM_MODE_RUN,
    .ActionListId = ACTIONLIST_ENABLE_DCM,
    .IsActive = TRUE
};
```

## Error Handling

### Error Codes

| Error Code | Value | Description |
|------------|-------|-------------|
| BSWM_E_NO_ERROR | 0x00 | No error |
| BSWM_E_PARAM_POINTER | 0x01 | NULL pointer error |
| BSWM_E_UNINIT | 0x02 | Module not initialized |
| BSWM_E_PARAM_INVALID | 0x03 | Invalid parameter |

### Error Detection

When `BSWM_DEV_ERROR_DETECT` is enabled:
- `BswM_Init()` with NULL config reports `BSWM_E_PARAM_POINTER`
- `BswM_RequestMode()` before init reports `BSWM_E_UNINIT`
- `BswM_RequestMode()` with invalid port reports `BSWM_E_PARAM_INVALID`
- `BswM_GetVersionInfo()` with NULL pointer reports `BSWM_E_PARAM_POINTER`

## Dependencies

- **Det** - Diagnostic Error Tracer (for error reporting)
- **SchM** - Schedule Manager (for exclusive areas)
- **EcuM** - ECU State Manager (mode request source)
- **ComM** - Communication Manager (mode request source)
- **DCM** - Diagnostic Communication Manager (mode request source)

## Timing Requirements

| Requirement | Value |
|-------------|-------|
| MainFunction Period | Typically 10ms |
| Mode Request Latency | < 2 MainFunction cycles |
| Rule Evaluation | Every MainFunction call |

## Testing

### Unit Test Coverage

The BSWM module includes comprehensive unit tests covering:

- **Initialization**: Valid/NULL config, re-initialization
- **Deinitialization**: Normal deinit, uninitialized deinit
- **Version Info**: Valid pointer, NULL pointer
- **Mode Requests**: Valid port, invalid port, uninitialized access
- **Main Function**: Initialized, uninitialized, multiple calls
- **Callbacks**: EcuM, ComM, DCM callbacks
- **State Management**: Init state, after deinit
- **Configuration**: Rule counts, active status
- **Complex Scenarios**: Complete lifecycle, multiple users

### Test File

- `/home/admin/yuleASR/tests/unit/autosar/services/test_bswm.c`

### Running Tests

```bash
cd /home/admin/yuleASR/tests/unit/autosar/services
gcc -o test_bswm test_bswm.c -lcmocka
./test_bswm
```

## Source Code

- `/home/admin/yuleASR/src/bsw/services/bswm/`
  - `include/BswM.h` - Public API
  - `include/BswM_Cfg.h` - Configuration
  - `src/BswM.c` - Core implementation
  - `src/BswM_Lcfg.c` - Link-time configuration

## References

- AUTOSAR_SWS_BSWModeManager
- AUTOSAR Classic Platform 4.4.0
- AUTOSAR Methodology
