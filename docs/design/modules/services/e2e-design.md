# E2E Design Document

> **Module ID**: 0xF0 (240)  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR SWS End-to-End Protection  
> **Source Path**: `src/bsw/services/e2e/`  
> **Doc Version**: 1.0  
> **Status**: Approved

---

## 1. 模块概述

E2E (End-to-End) 模块提供通信链路的端到端数据完整性保护，确保信号从发送方 SWC 到接收方 SWC 的全链路安全。通过 CRC 校验、计数器、数据 ID 等机制检测数据丢失、篡改、重放、乱序等故障。E2E 是 ASIL-D 功能安全通信的核心保护机制，支持多种 Profile（P01/P04/P06/P07 等）适配不同场景。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS E2E Protection | 4.4.0 | E2E 保护规范 |
| ISO 26262 | Part 6 | 通信完整性要求 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | Com, PduR | 调用 E2E Protect/Check |
| 同层 | CRC | CRC 计算 |
| 下层 | Det | 错误报告 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│         Com / PduR                  │
├─────────────────────────────────────┤
│     E2E (Profile P01/P04/P06/P07)  │
├─────────────────────────────────────┤
│         CRC (Services)              │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **E2E 调度器**: 根据数据端口选择对应 Profile
- **Profile P01**: 简单 CRC + 计数器（8-bit Data ID）
- **Profile P04**: CRC + 计数器 + 长 Data ID（32-bit）
- **Profile P06**: CRC32 + 计数器 + 偏移
- **Profile P07**: CRC16 + 计数器 +  nibble

### 3.3 文件结构

```
src/bsw/services/e2e/
├── include/
│   ├── E2E.h        # 公共 API + Profile 调度
│   ├── E2E_Cfg.h    # Profile 选择配置
│   ├── E2E_P04.h    # Profile 4 接口
│   ├── E2E_P06.h    # Profile 6 接口
│   └── E2E_P07.h    # Profile 7 接口
└── src/
    ├── E2E.c         # Profile 调度
    ├── E2E_P04.c     # Profile 4 实现
    ├── E2E_P06.c     # Profile 6 实现
    └── E2E_P07.c     # Profile 7 实现
```

---

## 4. 状态机

E2E 为无状态纯函数调用。每个 Profile 的 Protect/Check 独立执行，无跨调用状态。

---

## 5. 数据结构

```c
typedef struct {
    uint8*  DataPtr;       /* 待保护/校验的数据指针 */
    uint16  DataLength;    /* 数据长度 */
    uint16  DataID;        /* 数据标识符 */
    uint8   Counter;       /* 序列计数器 (0-15 循环) */
} E2E_ProtectDataType;

typedef enum {
    E2E_STATUS_OK = 0,
    E2E_STATUS_ERROR,
    E2E_STATUS_NONEWDATA,
    E2E_STATUS_WRONGCRC,
    E2E_STATUS_WRONGCOUNTER,
    E2E_STATUS_WRONGDATAID
} E2E_CheckReturnType;
```

---

## 6. API 规范

| API | 说明 | SWS 需求 |
|-----|------|----------|
| `E2E_ReturnType E2E_Protect(E2E_ProfileType Profile, E2E_ProtectDataType* Data)` | 对数据施加 E2E 保护 |  |
| `E2E_CheckReturnType E2E_Check(E2E_ProfileType Profile, E2E_ProtectDataType* Data)` | 校验数据的 E2E 完整性 |  |
| `void E2E_P04Protect(E2E_P04ProtectArrayType* Data)` | Profile 4 保护 | SWS_E2E_00009 |
| `E2E_P04CheckReturnType E2E_P04Check(const E2E_P04CheckArrayType* Data)` | Profile 4 校验 | SWS_E2E_00010 |

---

## 7. 处理流程

### 7.1 Protect 流程 (以 P04 为例)

1. 提取 DataID、Counter、DataPtr
2. 调用 CRC_CalculateCRC16 计算数据区 CRC
3. 将 CRC、Counter、DataID 写入 E2E 头区域
4. Counter 递增 (mod 16)

### 7.2 Check 流程 (以 P04 为例)

1. 从 E2E 头提取接收端的 CRC、Counter、DataID
2. 验证 DataID 匹配 → 不匹配返回 WRONGDATAID
3. 重新计算 CRC → 不匹配返回 WRONGCRC
4. 验证 Counter 递增 → 异常返回 WRONGCOUNTER
5. 全部通过 → 返回 OK

---

## 8. 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `E2E_P04_SUPPORT` | STD_ON | Profile 4 支持 |
| `E2E_P06_SUPPORT` | STD_ON | Profile 6 支持 |
| `E2E_P07_SUPPORT` | STD_OFF | Profile 7 支持 |
| `E2E_MAX_DATA_LENGTH` | 4095U | 最大保护数据长度 |

---

## 9. 错误处理

- NULL DataPtr → 返回 E2E_STATUS_ERROR
- 数据长度不匹配 → 返回 E2E_STATUS_ERROR
- Counter 跳变 > 阈值 → 返回 E2E_STATUS_WRONGCOUNTER

---

## 10. 内存与性能

- **RAM**: 每个保护数据端口需 1 字节 Counter 状态
- **ROM**: 每个 Profile ~2 KB 代码 + CRC 查找表
- **性能**: Protect/Check 各 O(n) 数据长度，主要开销在 CRC 计算

---

## 11. 集成指南

- Com 在信号打包/解包时调用 E2E_Protect / E2E_Check
- 每个需保护的数据端口配置 Profile 类型和 DataID
- Counter 由 E2E 模块自动管理

---

## 12. 测试策略

- 正常 Protect → Check 往返测试
- CRC 篡改检测测试
- Counter 跳变/回绕测试
- DataID 不匹配测试
- 空数据/最大长度边界测试

---

## 13. 实现说明

- 每个 Profile 独立编译，通过 E2E_Cfg.h 裁剪
- Counter 使用 4-bit 循环 (0-15)，容忍 ±1 跳变
- CRC 计算委托给 CRC 模块

---

## 14. 参考文献

- AUTOSAR_SWS_E2ELibrary.pdf (R4.4.0)
- yuleASR E2E 模块源码: `src/bsw/services/e2e/`
