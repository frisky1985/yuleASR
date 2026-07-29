# YuleTech BSW Phase 1 实施计划

> **For Claude:** REQUIRED SUB-SKILL: Use executing-plans to implement this plan task-by-task.

**Goal:** 完善测试体系 + 核心BSW模块框架，达到可编译状态

**Architecture:** 采用TDD方式，先写测试再实现框架代码，确保代码质量

**Tech Stack:** C99, CMake, Python, Unity测试框架

**Timeline:** 4周 (Week 1-4)

---

## Week 1: 完善文档和测试基础

### Task 1: 创建项目基础文档

**Files:**
- Create: `LICENSE`
- Create: `CHANGELOG.md`
- Modify: `README.md` (enhance)

**Step 1: 创建 Apache 2.0 LICENSE**

```bash
# 使用标准Apache 2.0许可证
cat > LICENSE << 'EOF'
                                 Apache License
                           Version 2.0, January 2004
                        http://www.apache.org/licenses/

   Copyright 2026 YuleTech

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
EOF
```

**Step 2: 创建 CHANGELOG.md**

```markdown
# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- 完整MCAL层驱动 (ADC, CAN, DIO, GPT, MCU, PORT, PWM, SPI, WDG)
- ECUAL层框架 (CanIf, CanTp, Ea, Fee)
- Services层框架 (COM, DCM, DEM, NVM)
- 130+单元测试用例
- 5层Mock系统
- Python构建系统
- Docusaurus文档站
- GitHub Actions CI/CD

## [1.0.0] - 2026-04-23

### Added
- Initial release
- Basic BSW structure
- Core MCAL drivers
- Test framework foundation
```

**Step 3: 提交**

```bash
git add LICENSE CHANGELOG.md
git commit -m "docs: add LICENSE and CHANGELOG"
```

---

### Task 2: 创建示例项目结构

**Files:**
- Create: `examples/led_blink/README.md`
- Create: `examples/led_blink/main.c`
- Create: `examples/led_blink/CMakeLists.txt`
- Create: `examples/can_loopback/README.md`
- Create: `examples/can_loopback/main.c`

**Step 1: 创建LED闪烁示例**

```bash
mkdir -p examples/led_blink examples/can_loopback
```

**Step 2: 编写LED示例代码**

```c
// examples/led_blink/main.c
/**
 * @file main.c
 * @brief LED Blink Example
 * @version 1.0.0
 * 
 * This example demonstrates basic usage of DIO and GPT drivers
 * to create a simple LED blinking application.
 */

#include "Dio.h"
#include "Gpt.h"
#include "Mcu.h"

#define LED_PIN     DioConf_DioChannel_LED
#define DELAY_MS    500

int main(void)
{
    /* Initialize MCU */
    Mcu_Init(&Mcu_Config);
    
    /* Initialize GPT */
    Gpt_Init(&Gpt_Config);
    
    /* Initialize DIO */
    Dio_Init(&Dio_Config);
    
    /* Main loop */
    while (1)
    {
        /* Turn LED on */
        Dio_WriteChannel(LED_PIN, STD_HIGH);
        Gpt_StartTimer(DELAY_MS);
        
        /* Turn LED off */
        Dio_WriteChannel(LED_PIN, STD_LOW);
        Gpt_StartTimer(DELAY_MS);
    }
    
    return 0;
}
```

**Step 3: 编写示例README**

```markdown
# LED Blink Example

## Overview
This example demonstrates basic DIO and GPT usage.

## Hardware Requirements
- STM32F4xx Nucleo board (or compatible)
- User LED on PA5

## Building
```bash
cd examples/led_blink
mkdir build && cd build
cmake ..
make
```

## Flashing
```bash
st-flash write led_blink.bin 0x08000000
```

## Expected Behavior
The user LED (LD2) should blink at 1Hz (500ms on, 500ms off).
```

**Step 4: 提交**

```bash
git add examples/
git commit -m "feat(examples): add LED blink and CAN loopback examples"
```

---

## Week 2: 实现核心BSW模块框架

### Task 3: 实现 EcuM (ECU State Manager)

**Files:**
- Create: `src/bsw/services/ecum/include/EcuM.h`
- Create: `src/bsw/services/ecum/include/EcuM_Cfg.h`
- Create: `src/bsw/services/ecum/src/EcuM.c`

**Step 1: 创建EcuM头文件**

```c
/**
 * @file EcuM.h
 * @brief ECU State Manager
 * @version 1.0.0
 */

#ifndef ECUM_H
#define ECUM_H

#include "Std_Types.h"

/* AUTOSAR Version */
#define ECUM_AR_RELEASE_MAJOR_VERSION       4
#define ECUM_AR_RELEASE_MINOR_VERSION       0
#define ECUM_AR_RELEASE_REVISION_VERSION    3

/* Module Version */
#define ECUM_SW_MAJOR_VERSION               1
#define ECUM_SW_MINOR_VERSION               0
#define ECUM_SW_PATCH_VERSION               0

/* Service IDs */
#define ECUM_INIT_SID                       0x00
#define ECUM_SHUTDOWN_SID                   0x01
#define ECUM_REQUESTRUN_SID                 0x02
#define ECUM_RELEASERUN_SID                 0x03
#define ECUM_SELECTSHUTDOWNTARGET_SID       0x04
#define ECUM_GETSTATE_SID                   0x05
#define ECUM_COMMODEREQUEST_SID             0x06
#define ECUM_SETWAKEUPEVENT_SID             0x07
#define ECUM_VALIDATEMCUWAKEUPEVENT_SID     0x08

/* Error Codes */
#define ECUM_E_NOT_INITIALIZED              0x10
#define ECUM_E_INVALID_PAR                  0x11
#define ECUM_E_NULL_POINTER                 0x12
#define ECUM_E_STATE_CHANGE_FAILED          0x13

/* Types */
typedef uint8 EcuM_StateType;
#define ECUM_STATE_STARTUP                  0x00
#define ECUM_STATE_RUN                      0x10
#define ECUM_STATE_POST_RUN                 0x20
#define ECUM_STATE_SLEEP                    0x30
#define ECUM_STATE_SHUTDOWN                 0x40
#define ECUM_STATE_OFF                      0x50

typedef uint8 EcuM_WakeupSourceType;
typedef uint8 EcuM_WakeupStatusType;
#define ECUM_WKSTATUS_NONE                  0x00
#define ECUM_WKSTATUS_PENDING               0x01
#define ECUM_WKSTATUS_VALIDATED             0x02
#define ECUM_WKSTATUS_EXPIRED               0x03

typedef uint8 EcuM_ShutdownTargetType;
#define ECUM_STATE_OFF                      0x00
#define ECUM_STATE_RESET                    0x01
#define ECUM_STATE_SLEEP                    0x02

typedef uint8 EcuM_UserType;

/* Function Prototypes */
extern void EcuM_Init(void);
extern void EcuM_StartupTwo(void);
extern void EcuM_Shutdown(void);
extern Std_ReturnType EcuM_RequestRUN(EcuM_UserType user);
extern Std_ReturnType EcuM_ReleaseRUN(EcuM_UserType user);
extern Std_ReturnType EcuM_SelectShutdownTarget(EcuM_ShutdownTargetType target, uint8 mode);
extern Std_ReturnType EcuM_GetState(EcuM_StateType* state);
extern void EcuM_SetWakeupEvent(EcuM_WakeupSourceType sources);
extern EcuM_WakeupStatusType EcuM_GetStatusOfWakeupSource(EcuM_WakeupSourceType sources);

#endif /* ECUM_H */
```

**Step 2: 创建EcuM配置头文件**

```c
/**
 * @file EcuM_Cfg.h
 * @brief ECU State Manager Configuration
 */

#ifndef ECUM_CFG_H
#define ECUM_CFG_H

/* Configuration */
#define ECUM_DEV_ERROR_DETECT               STD_ON
#define ECUM_VERSION_INFO_API               STD_ON
#define ECUM_MAIN_FUNCTION_PERIOD           10  /* ms */

/* Number of users */
#define ECUM_MAX_USERS                      32

/* Wakeup sources */
#define ECUM_WKSOURCE_POWER                 0x00000001
#define ECUM_WKSOURCE_RESET                 0x00000002
#define ECUM_WKSOURCE_INTERNAL_RESET        0x00000004
#define ECUM_WKSOURCE_INTERNAL_WDG          0x00000008
#define ECUM_WKSOURCE_EXTERNAL_WDG          0x00000010

#endif /* ECUM_CFG_H */
```

**Step 3: 创建EcuM实现**

```c
/**
 * @file EcuM.c
 * @brief ECU State Manager Implementation
 */

#include "EcuM.h"
#include "EcuM_Cfg.h"
#include "Det.h"

/* Internal State */
static EcuM_StateType EcuM_CurrentState = ECUM_STATE_OFF;
static EcuM_ShutdownTargetType EcuM_ShutdownTarget = ECUM_STATE_OFF;
static boolean EcuM_IsInitialized = FALSE;
static uint32 EcuM_RunRequests = 0;

#define ECUM_MODULE_ID                      0x0A
#define ECUM_INSTANCE_ID                    0x00

void EcuM_Init(void)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (EcuM_IsInitialized)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_INIT_SID, ECUM_E_NOT_INITIALIZED);
        return;
    }
#endif
    
    /* Initialize state */
    EcuM_CurrentState = ECUM_STATE_STARTUP;
    EcuM_RunRequests = 0;
    
    /* TODO: Initialize BSW modules */
    /* TODO: Initialize Scheduler */
    /* TODO: Initialize OS */
    
    EcuM_IsInitialized = TRUE;
    
    /* Transition to RUN state */
    EcuM_CurrentState = ECUM_STATE_RUN;
}

void EcuM_StartupTwo(void)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (!EcuM_IsInitialized)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_INIT_SID, ECUM_E_NOT_INITIALIZED);
        return;
    }
#endif
    
    /* TODO: Initialize SWCs */
    /* TODO: Start RTE */
}

Std_ReturnType EcuM_RequestRUN(EcuM_UserType user)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (!EcuM_IsInitialized)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_REQUESTRUN_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (user >= ECUM_MAX_USERS)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_REQUESTRUN_SID, ECUM_E_INVALID_PAR);
        return E_NOT_OK;
    }
#endif
    
    EcuM_RunRequests |= (1u << user);
    return E_OK;
}

Std_ReturnType EcuM_ReleaseRUN(EcuM_UserType user)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (!EcuM_IsInitialized)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_RELEASERUN_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (user >= ECUM_MAX_USERS)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_RELEASERUN_SID, ECUM_E_INVALID_PAR);
        return E_NOT_OK;
    }
#endif
    
    EcuM_RunRequests &= ~(1u << user);
    
    /* Check if all RUN requests released */
    if (EcuM_RunRequests == 0)
    {
        EcuM_CurrentState = ECUM_STATE_POST_RUN;
    }
    
    return E_OK;
}

Std_ReturnType EcuM_SelectShutdownTarget(EcuM_ShutdownTargetType target, uint8 mode)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (!EcuM_IsInitialized)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SELECTSHUTDOWNTARGET_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
#endif
    
    EcuM_ShutdownTarget = target;
    return E_OK;
}

Std_ReturnType EcuM_GetState(EcuM_StateType* state)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (!EcuM_IsInitialized)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_GETSTATE_SID, ECUM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (state == NULL_PTR)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_GETSTATE_SID, ECUM_E_NULL_POINTER);
        return E_NOT_OK;
    }
#endif
    
    *state = EcuM_CurrentState;
    return E_OK;
}

void EcuM_Shutdown(void)
{
#if (ECUM_DEV_ERROR_DETECT == STD_ON)
    if (!EcuM_IsInitialized)
    {
        Det_ReportError(ECUM_MODULE_ID, ECUM_INSTANCE_ID, ECUM_SHUTDOWN_SID, ECUM_E_NOT_INITIALIZED);
        return;
    }
#endif
    
    EcuM_CurrentState = ECUM_STATE_SHUTDOWN;
    
    /* TODO: De-initialize modules */
    /* TODO: Shutdown OS */
}
```

**Step 4: 创建EcuM测试**

```c
// tests/unit/services/test_ecum.c
#include "unity.h"
#include "EcuM.h"
#include "mock_det.h"

void setUp(void)
{
    /* Reset before each test */
}

void tearDown(void)
{
    /* Cleanup after each test */
}

void test_EcuM_Init_ShouldSetStartupState(void)
{
    EcuM_StateType state;
    
    EcuM_Init();
    
    TEST_ASSERT_EQUAL(E_OK, EcuM_GetState(&state));
    TEST_ASSERT_EQUAL(ECUM_STATE_RUN, state);
}

void test_EcuM_RequestRUN_ShouldSetRequest(void)
{
    EcuM_Init();
    
    TEST_ASSERT_EQUAL(E_OK, EcuM_RequestRUN(0));
    TEST_ASSERT_EQUAL(E_OK, EcuM_RequestRUN(1));
}

void test_EcuM_ReleaseRUN_ShouldClearRequest(void)
{
    EcuM_Init();
    
    EcuM_RequestRUN(0);
    TEST_ASSERT_EQUAL(E_OK, EcuM_ReleaseRUN(0));
}

void test_EcuM_SelectShutdownTarget_ShouldSetTarget(void)
{
    EcuM_Init();
    
    TEST_ASSERT_EQUAL(E_OK, EcuM_SelectShutdownTarget(ECUM_STATE_OFF, 0));
}
```

**Step 5: 提交**

```bash
git add src/bsw/services/ecum/ tests/unit/services/
git commit -m "feat(ecum): implement ECU State Manager framework"
```

---

### Task 4: 实现 BswM (BSW Mode Manager)

**Files:**
- Create: `src/bsw/services/bswm/include/BswM.h`
- Create: `src/bsw/services/bswm/include/BswM_Cfg.h`
- Create: `src/bsw/services/bswm/src/BswM.c`

**类似结构，包含:**
- Mode request interface
- Rule evaluation
- Action execution
- Configurable rules

**提交:**
```bash
git add src/bsw/services/bswm/
git commit -m "feat(bswm): implement BSW Mode Manager framework"
```

---

### Task 5: 实现 SchM (Scheduler Manager)

**Files:**
- Create: `src/bsw/services/schm/include/SchM.h`
- Create: `src/bsw/services/schm/include/SchM_Cfg.h`
- Create: `src/bsw/services/schm/src/SchM.c`

**关键功能:**
- Task scheduling
- Schedule table management
- Counter management
- Alarm handling

**提交:**
```bash
git add src/bsw/services/schm/
git commit -m "feat(schm): implement Scheduler Manager framework"
```

---

## Week 3: 扩展ECUAL测试

### Task 6: 为CanIf添加完整测试

**Files:**
- Create: `tests/unit/ecual/test_canif.c`

**测试用例:**
- Initialization
- Transmit/Receive
- Controller mode changes
- PDU routing
- Error handling

**提交:**
```bash
git add tests/unit/ecual/
git commit -m "test(canif): add comprehensive unit tests"
```

---

### Task 7: 为CanTp添加完整测试

**Files:**
- Create: `tests/unit/ecual/test_cantp.c`

**测试用例:**
- Single frame transmission
- Multi-frame transmission
- Flow control
- Timeout handling
- Error scenarios

**提交:**
```bash
git commit -m "test(cantp): add comprehensive unit tests"
```

---

## Week 4: 集成测试和验证

### Task 8: 创建集成测试

**Files:**
- Create: `tests/integration/bsw/test_ecum_bswm_integration.c`
- Create: `tests/integration/ecual/test_can_stack.c`

**测试场景:**
- EcuM startup sequence
- Mode switching with BswM
- CAN message flow (CanIf + CanTp + PduR)

**提交:**
```bash
git add tests/integration/
git commit -m "test(integration): add BSW integration tests"
```

---

### Task 9: 更新CMake配置

**Files:**
- Modify: `CMakeLists.txt` (add new modules)
- Modify: `tests/CMakeLists.txt` (add new tests)

**Step 1: 添加新模块到CMake**

```cmake
# Add ECUAL modules
add_library(Ecual STATIC
    src/bsw/ecual/canif/src/CanIf.c
    src/bsw/ecual/cantp/src/CanTp.c
    # ... other ecual modules
)

# Add Service modules
add_library(Services STATIC
    src/bsw/services/com/src/Com.c
    src/bsw/services/dcm/src/Dcm.c
    src/bsw/services/dem/src/Dem.c
    src/bsw/services/nvm/src/Nvm.c
    src/bsw/services/ecum/src/EcuM.c
    src/bsw/services/bswm/src/BswM.c
    src/bsw/services/schm/src/SchM.c
)
```

**Step 2: 提交**

```bash
git commit -m "build: update CMake for new modules"
```

---

### Task 10: 运行完整测试套件

**Commands:**
```bash
python3 tools/build/build.py configure --tests
python3 tools/build/build.py build
python3 tools/build/build.py test
```

**验证目标:**
- [ ] All unit tests pass
- [ ] All integration tests pass
- [ ] Code coverage > 60%
- [ ] Static analysis clean

**提交:**
```bash
git commit -m "chore: verify all tests pass"
```

---

## 里程碑检查点

### Week 1 结束
- [ ] LICENSE and CHANGELOG created
- [ ] 2+ examples created
- [ ] Examples build successfully

### Week 2 结束
- [ ] EcuM framework implemented
- [ ] BswM framework implemented
- [ ] SchM framework implemented
- [ ] All compile without errors

### Week 3 结束
- [ ] CanIf tests complete
- [ ] CanTp tests complete
- [ ] Code coverage > 50%

### Week 4 结束
- [ ] All tests pass
- [ ] Code coverage > 60%
- [ ] Static analysis clean
- [ ] Documentation updated

---

## 验证命令

```bash
# Build everything
python3 tools/build/build.py configure --tests
python3 tools/build/build.py build

# Run all tests
python3 tools/build/build.py test

# Run static analysis
python3 tools/build/build.py lint

# Check coverage
gcovr -r . --html --html-details -o coverage.html
```

---

## 执行方式

启动执行时，使用:
```bash
# In the yuleASR directory
cd ~/yuleASR

# Start implementing task by task
# Each task is self-contained and can be executed independently
```
