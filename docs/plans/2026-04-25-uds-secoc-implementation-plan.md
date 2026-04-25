# UDS诊断栈与SecOC安全栈并行实施计划

> **使用方法**: 调用 superpowers:executing-plans 或 parallel-embedded-automotive-dev 逐任务执行

**目标**: 6周内完成UDS诊断协议栈(DCM/DEM/DoCAN/DoIP)与SecOC安全栈(SecOC/CSM/CryIf/KeyM)的双线并行开发

**架构**: 依据2026-04-25设计文档，线路A实现ISO 14229/15765/13400标准的诊断协议栈，线路B实现AUTOSAR 4.4安全通信栈，共享密钥管理与DDS-Security集成

**技术栈**: C11, CMake, FreeRTOS, Unity测试, HSM硬件抽象, UDS/ISO-TP/DoIP协议

---

## Agent分工一览

```
Agent A1: UDS Core
- Task 1-3: DCM架架
- Task 4-6: 核心UDS服务
- Task 7-8: DEM DTC管理

Agent A2: DoCAN/DoIP Transport  
- Task 9-11: IsoTp协议栈
- Task 12-14: DoIP协议栈
- Task 15: PduR路由

Agent B1: SecOC
- Task 16-18: SecOC核心
- Task 19-20: Freshness管理
- Task 21: DDS集成

Agent B2: CSM/CryIf/KeyM
- Task 22-24: CSM Job管理
- Task 25-26: CryIf硬件抽象
- Task 27: KeyM密钥管理

Agent Coord: 架构集成
- Task 28-30: 测试框架
- Task 31-32: 文档生成
```

---

## 线路A: UDS诊断栈

### Task 1: 创建诊断模块目录结构

**Files:**
- 创建目录: `src/diagnostics/`, `src/diagnostics/dcm/`, `src/diagnostics/dem/`, `src/diagnostics/docan/`, `src/diagnostics/doip/`
- 创建: `src/diagnostics/CMakeLists.txt`

**Step 1: 创建目录结构**

```bash
mkdir -p src/diagnostics/{dcm,dem,docan,doip,tests}
```

**Step 2: 创建CMakeLists.txt**

```cmake
# src/diagnostics/CMakeLists.txt
add_library(diagnostics STATIC
    dcm/dcm_core.c
    dcm/dcm_services.c
    dcm/dcm_session.c
    dem/dem_core.c
    dem/dem_dtc.c
    dem/dem_event.c
    docan/isotp.c
    docan/isotp_canif.c
    docan/docan_pdurouter.c
    doip/doip_core.c
    doip/doip_vd.c
    doip/doip_routing.c
    doip/doip_diagnostic.c
)

target_include_directories(diagnostics PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

target_link_libraries(diagnostics
    common
    platform
    transport
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
```

**Step 3: 提交**

```bash
git add src/diagnostics/
git commit -m "feat(diagnostics): create module directory structure"
```

---

### Task 2: DCM核心数据结构定义

**Files:**
- 创建: `src/diagnostics/dcm/dcm_types.h`
- 创建: `src/diagnostics/dcm/dcm_core.h`

**Step 1: 定义DCM核心类型**

```c
// src/diagnostics/dcm/dcm_types.h
#ifndef DCM_TYPES_H
#define DCM_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "autosar_types.h"

#define DCM_MAX_SID                     256
#define DCM_MAX_DID                     65536
#define DCM_MAX_RID                     65536
#define DCM_MAX_DSD_SIZE                4096
#define DCM_MAX_CONCURRENT_REQUESTS     8

typedef enum {
    DCM_SESSION_DEFAULT = 0x01,
    DCM_SESSION_PROGRAMMING = 0x02,
    DCM_SESSION_EXTENDED = 0x03,
    DCM_SESSION_SAFETY_SYSTEM = 0x04
} dcm_session_type_t;

typedef enum {
    DCM_SECURITY_LOCKED = 0x00,
    DCM_SECURITY_L1 = 0x01,
    DCM_SECURITY_L2 = 0x02,
    DCM_SECURITY_L3 = 0x03,
    DCM_SECURITY_L4 = 0x04,
    DCM_SECURITY_L5 = 0x05
} dcm_security_level_t;

typedef enum {
    DCM_POS_OK = 0x00,
    DCM_POS_RESPONSE = 0x01
} dcm_positive_response_t;

typedef enum {
    // ISO 14229-1 负响应码
    DCM_NEG_GENERAL_REJECT = 0x10,
    DCM_NEG_SNS = 0x11,
    DCM_NEG_SFNS = 0x12,
    DCM_NEG_IMLOIF = 0x13,
    DCM_NEG_RTL = 0x14,
    DCM_NEG_NRC = 0x15,
    DCM_NEG_FPF = 0x16,
    DCM_NEG_ROOR = 0x31,
    DCM_NEG_SAD = 0x32,
    DCM_NEG_AR = 0x33,
    DCM_NEG_IK = 0x35,
    DCM_NEG_ENOA = 0x36,
    DCM_NEG_RTDNE = 0x37,
    DCM_NEG_TDS = 0x38,
    DCM_NEG_UDNA = 0x3A,
    DCM_NEG_TFD = 0x3B,
    DCM_NEG_PA = 0x3C,
    DCM_NEG_DTC = 0x3D,
    DCM_NEG_ROE = 0x3E,
    DCM_NEG_SNSIAS = 0x3F,
    DCM_NEG_DNA = 0x40,
    DCM_NEG_TD = 0x41,
    DCM_NEG_CNC = 0x42,
    DCM_NEG_SAR = 0x43,
    DCM_NEG_BRR = 0x44,
    DCM_NEG_NR = 0x45,
    DCM_NEG_NC = 0x46,
    DCM_NEG_RR = 0x47,
    DCM_NEG_SDT = 0x48,
    DCM_NEG_SST = 0x49,
    DCM_NEG_VSTOOF = 0x4A,
    DCM_NEG_RAD = 0x4B,
    DCM_NEG_SDCSNC = 0x4C,
    DCM_NEG_SFTNF = 0x50,
    DCM_NEG_SADSA = 0x51,
    DCM_NEG_DNE = 0x70,
    DCM_NEG_TDSF = 0x71,
    DCM_NEG_GPF = 0x72,
    DCM_NEG_WBSC = 0x73,
    DCM_NEG_IBS = 0x74,
    DCM_NEG_DBSTL = 0x75,
    DCM_NEG_EDNE = 0x76,
    DCM_NEG_VMSAN = 0x77,
    DCM_NEG_WSC = 0x78,
    DCM_NEG_RSE = 0x7E,
    DCM_NEG_SNSIE = 0x7F,
    DCM_NEG_ES = 0x81,
    DCM_NEG_GPFES = 0x82,
    DCM_NEG_BRS = 0x83,
    DCM_NEG_BV = 0x84,
    DCM_NEG_ISOSAESA = 0x85,
    DCM_NEG_VTMS = 0x86,
    DCM_NEG_RMM = 0x87,
    DCM_NEG_VMSC = 0x88,
    DCM_NEG_TCOOM = 0x89
} dcm_negative_response_t;

// SID定义
typedef enum {
    DCM_SID_DIAGNOSTIC_SESSION_CONTROL = 0x10,
    DCM_SID_ECU_RESET = 0x11,
    DCM_SID_SECURITY_ACCESS = 0x27,
    DCM_SID_COMMUNICATION_CONTROL = 0x28,
    DCM_SID_TESTER_PRESENT = 0x3E,
    DCM_SID_CONTROL_DTC_SETTING = 0x85,
    DCM_SID_RESPONSE_ON_EVENT = 0x86,
    DCM_SID_READ_DATA_BY_IDENTIFIER = 0x22,
    DCM_SID_READ_MEMORY_BY_ADDRESS = 0x23,
    DCM_SID_READ_SCALING_DATA_BY_IDENTIFIER = 0x24,
    DCM_SID_READ_DATA_BY_PERIODIC_IDENTIFIER = 0x2A,
    DCM_SID_DYNAMICALLY_DEFINE_DATA_IDENTIFIER = 0x2C,
    DCM_SID_WRITE_DATA_BY_IDENTIFIER = 0x2E,
    DCM_SID_WRITE_MEMORY_BY_ADDRESS = 0x3D,
    DCM_SID_CLEAR_DIAGNOSTIC_INFORMATION = 0x14,
    DCM_SID_READ_DTC_INFORMATION = 0x19,
    DCM_SID_ROUTINE_CONTROL = 0x31,
    DCM_SID_REQUEST_DOWNLOAD = 0x34,
    DCM_SID_REQUEST_UPLOAD = 0x35,
    DCM_SID_TRANSFER_DATA = 0x36,
    DCM_SID_REQUEST_TRANSFER_EXIT = 0x37
} dcm_sid_t;

// 服务处理器回调
typedef dcm_negative_response_t (*dcm_service_handler_t)(
    const uint8_t *request,
    uint32_t request_len,
    uint8_t *response,
    uint32_t *response_len
);

typedef struct {
    uint8_t sid;
    dcm_service_handler_t handler;
    uint8_t min_request_len;
    dcm_session_type_t required_session;
    dcm_security_level_t required_security;
} dcm_service_config_t;

typedef struct {
    dcm_session_type_t current_session;
    dcm_security_level_t current_security;
    uint32_t session_timeout_ms;
    uint32_t s3_server_timeout_ms;
    uint32_t last_activity_ms;
    bool session_active;
} dcm_session_info_t;

#endif
```

**Step 2: 定义DCM核心接口**

```c
// src/diagnostics/dcm/dcm_core.h
#ifndef DCM_CORE_H
#define DCM_CORE_H

#include "dcm_types.h"

typedef struct dcm_context dcm_context_t;

dcm_context_t* dcm_init(const dcm_service_config_t *services, uint32_t service_count);
void dcm_deinit(dcm_context_t *ctx);

dcm_negative_response_t dcm_process_request(
    dcm_context_t *ctx,
    const uint8_t *request,
    uint32_t request_len,
    uint8_t *response,
    uint32_t *response_len,
    uint32_t max_response_len
);

dcm_negative_response_t dcm_send_positive_response(
    uint8_t sid,
    const uint8_t *data,
    uint32_t data_len,
    uint8_t *response,
    uint32_t *response_len
);

dcm_negative_response_t dcm_send_negative_response(
    uint8_t sid,
    dcm_negative_response_t nrc,
    uint8_t *response,
    uint32_t *response_len
);

#endif
```

**Step 3: 提交**

```bash
git add src/diagnostics/dcm/dcm_types.h src/diagnostics/dcm/dcm_core.h
git commit -m "feat(dcm): add core type definitions and interfaces"
```

---

### Task 3: DCM会话管理实现

**Files:**
- 创建: `src/diagnostics/dcm/dcm_session.c`
- 创建: `src/diagnostics/dcm/dcm_session.h`

**Step 1: 实现会话管理**

```c
// src/diagnostics/dcm/dcm_session.c
#include "dcm_session.h"
#include <string.h>

static dcm_session_info_t g_session_info = {
    .current_session = DCM_SESSION_DEFAULT,
    .current_security = DCM_SECURITY_LOCKED,
    .session_timeout_ms = 5000,
    .s3_server_timeout_ms = 5000,
    .last_activity_ms = 0,
    .session_active = false
};

dcm_session_info_t* dcm_session_get_info(void) {
    return &g_session_info;
}

void dcm_session_init(uint32_t session_timeout_ms, uint32_t s3_timeout_ms) {
    memset(&g_session_info, 0, sizeof(g_session_info));
    g_session_info.current_session = DCM_SESSION_DEFAULT;
    g_session_info.current_security = DCM_SECURITY_LOCKED;
    g_session_info.session_timeout_ms = session_timeout_ms;
    g_session_info.s3_server_timeout_ms = s3_timeout_ms;
    g_session_info.session_active = true;
    g_session_info.last_activity_ms = dcm_get_tick_ms();
}

dcm_negative_response_t dcm_session_control(
    const uint8_t *request,
    uint32_t request_len,
    uint8_t *response,
    uint32_t *response_len
) {
    if (request_len < 2) {
        return DCM_NEG_IMLOIF;
    }
    
    uint8_t requested_session = request[1];
    
    switch (requested_session) {
        case DCM_SESSION_DEFAULT:
        case DCM_SESSION_PROGRAMMING:
        case DCM_SESSION_EXTENDED:
        case DCM_SESSION_SAFETY_SYSTEM:
            g_session_info.current_session = requested_session;
            g_session_info.last_activity_ms = dcm_get_tick_ms();
            break;
        default:
            return DCM_NEG_SFNS;
    }
    
    // 构建正响应: [0x50, session_type, P2* (2 bytes), P2*_ext (2 bytes)]
    response[0] = 0x50;
    response[1] = requested_session;
    response[2] = 0x00;  // P2* high
    response[3] = 0x32;  // P2* low (50ms)
    response[4] = 0x01;  // P2*_ext high
    response[5] = 0xF4;  // P2*_ext low (5000ms)
    *response_len = 6;
    
    return DCM_POS_OK;
}

void dcm_session_update_activity(void) {
    g_session_info.last_activity_ms = dcm_get_tick_ms();
}

bool dcm_session_check_timeout(void) {
    if (!g_session_info.session_active) {
        return false;
    }
    
    uint32_t current_tick = dcm_get_tick_ms();
    uint32_t elapsed = current_tick - g_session_info.last_activity_ms;
    
    if (elapsed > g_session_info.session_timeout_ms) {
        // 回到默认会话
        g_session_info.current_session = DCM_SESSION_DEFAULT;
        g_session_info.current_security = DCM_SECURITY_LOCKED;
        g_session_info.session_active = false;
        return true;
    }
    return false;
}

bool dcm_session_is_allowed(dcm_session_type_t required_session) {
    return g_session_info.current_session >= required_session;
}

bool dcm_security_is_allowed(dcm_security_level_t required_security) {
    return g_session_info.current_security >= required_security;
}
```

**Step 2: 编译测试**

```c
// src/diagnostics/tests/test_dcm_session.c
#include "unity.h"
#include "dcm/dcm_session.h"

void setUp(void) {
    dcm_session_init(5000, 5000);
}

void test_session_control_default(void) {
    uint8_t request[] = {0x10, 0x01};
    uint8_t response[32];
    uint32_t response_len = 0;
    
    dcm_negative_response_t result = dcm_session_control(
        request, sizeof(request), response, &response_len
    );
    
    TEST_ASSERT_EQUAL(DCM_POS_OK, result);
    TEST_ASSERT_EQUAL(6, response_len);
    TEST_ASSERT_EQUAL(0x50, response[0]);
    TEST_ASSERT_EQUAL(0x01, response[1]);
}

void test_session_control_extended(void) {
    uint8_t request[] = {0x10, 0x03};
    uint8_t response[32];
    uint32_t response_len = 0;
    
    dcm_negative_response_t result = dcm_session_control(
        request, sizeof(request), response, &response_len
    );
    
    TEST_ASSERT_EQUAL(DCM_POS_OK, result);
    TEST_ASSERT_EQUAL(DCM_SESSION_EXTENDED, 
                      dcm_session_get_info()->current_session);
}

void test_session_control_invalid(void) {
    uint8_t request[] = {0x10, 0xFF};
    uint8_t response[32];
    uint32_t response_len = 0;
    
    dcm_negative_response_t result = dcm_session_control(
        request, sizeof(request), response, &response_len
    );
    
    TEST_ASSERT_EQUAL(DCM_NEG_SFNS, result);
}

void test_session_control_too_short(void) {
    uint8_t request[] = {0x10};
    uint8_t response[32];
    uint32_t response_len = 0;
    
    dcm_negative_response_t result = dcm_session_control(
        request, sizeof(request), response, &response_len
    );
    
    TEST_ASSERT_EQUAL(DCM_NEG_IMLOIF, result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_session_control_default);
    RUN_TEST(test_session_control_extended);
    RUN_TEST(test_session_control_invalid);
    RUN_TEST(test_session_control_too_short);
    return UNITY_END();
}
```

**Step 3: 提交**

```bash
git add src/diagnostics/dcm/dcm_session.c src/diagnostics/dcm/dcm_session.h
git add src/diagnostics/tests/test_dcm_session.c
git commit -m "feat(dcm): implement session management with tests"
```

---

### Task 4-32: (省略... 按照同样模式继续)

[为简洁计，以下任务按照相同结构，每个任务包含:
1. 头文件定义
2. 实源文件实现
3. 单元测试
4. 提交]

---

## 快速执行指南

### 并行启动所有Agents

```bash
# 使用多Agent并行执行
cd /home/admin/eth-dds-integration

# Agent A1
delegate_task --agent A1 --tasks tasks_A1.json &

# Agent A2  
delegate_task --agent A2 --tasks tasks_A2.json &

# Agent B1
delegate_task --agent B1 --tasks tasks_B1.json &

# Agent B2
delegate_task --agent B2 --tasks tasks_B2.json &

# 等待所有Agent完成
wait
```

### 常用命令

```bash
# 构建测试
mkdir build && cd build
cmake .. -DBUILD_TESTING=ON
make diagnostics_tests
cmake --build . --target test

# 运行特定测试
./src/diagnostics/tests/test_dcm_session

# 代码覆盖率
cmake .. -DENABLE_COVERAGE=ON
make coverage_report
```

---

## 测试覆盖要求

| 模块 | 最低覆盖率 | 关键测试场景 |
|--------|-----------|------------|
| DCM | 85% | SID 0x10/0x11/0x22/0x2E/0x31 |
| DEM | 80% | DTC确认/去确认/快照数据 |
| IsoTp | 90% | SF/FF/CF/FC所有组合 |
| DoIP | 85% | 路由激活/生命检查 |
| SecOC | 85% | MAC验证/新鲜度同步 |
| CSM | 80% | Job队列/异步回调 |
| CryIf | 75% | HSM适配器 |
| KeyM | 80% | 密钥派生/轮换 |

---

## 文档产出

- [ ] `docs/DIAGNOSTICS_DESIGN.md` - UDS诊断设计文档
- [ ] `docs/SECOC_DESIGN.md` - SecOC安全设计文档
- [ ] `docs/CRYPTO_STACK.md` - 加密栈架构文档
- [ ] API参考手册 (Doxygen生成)
- [ ] 配置工具更新 (DDS配置工具支持诊断配置)
