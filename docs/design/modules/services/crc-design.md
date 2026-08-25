# CRC Design Document

> **Module ID**: 0xC9 (201)  
> **AUTOSAR Layer**: Services  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR SWS CRC Routines  
> **Source Path**: `src/bsw/services/crc/`  
> **Doc Version**: 1.0  
> **Status**: Approved

---

## 1. 模块概述

CRC 模块提供多种 CRC（循环冗余校验）算法实现，用于数据完整性验证。被 NvM（数据块校验）、Com（信号保护）、E2E（端到端保护）等模块广泛依赖。支持 CRC8、CRC16、CRC32 等标准算法，提供查表法和计算法两种实现。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS CRC Routines | 4.4.0 | CRC 算法规范 |
| SAE J1850 | — | CRC8 标准 |
| ISO 11574 | — | CRC16 标准 |
| IEEE 802.3 | — | CRC32 标准 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | NvM, Com, E2E, SecOC | 调用 CRC 计算/校验接口 |
| 公共 | Det | 参数错误报告 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│    NvM / Com / E2E / SecOC          │
├─────────────────────────────────────┤
│          CRC (Services)             │
├─────────────────────────────────────┤
│        (无下层依赖)                  │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **CRC8 引擎**: SAE J1850 多项式 0x1D，可选查表/计算
- **CRC16 引擎**: CCITT 多项式 0x1021
- **CRC32 引擎**: IEEE 多项式 0x04C11DB7
- **Fast 变体**: 预计算查表，ROM 换速度

### 3.3 文件结构

```
src/bsw/services/crc/
├── include/
│   ├── Crc.h         # 公共 API
│   └── Crc_Cfg.h     # 配置（算法选择）
└── src/
    ├── Crc.c          # 核心实现
    └── Crc_Lcfg.c     # CRC 查找表
```

---

## 4. 状态机

CRC 模块无状态，所有 API 为纯函数（无内部状态）。

---

## 5. 数据结构

```c
/* CRC 查找表类型 */
extern const uint8  Crc_Table8[256];
extern const uint16 Crc_Table16[256];
extern const uint32 Crc_Table32[256];
```

---

## 6. API 规范

| API | 说明 | SWS 需求 |
|-----|------|----------|
| `uint8 Crc_CalculateCRC8(const uint8* DataPtr, uint32 Length, uint8 StartValue8)` | 计算 CRC8 | SWS_Crc_00003 |
| `uint8 Crc_CalculateCRC8H2F(const uint8* DataPtr, uint32 Length, uint8 StartValue8)` | CRC8 H2F 变体 | SWS_Crc_00003 |
| `uint16 Crc_CalculateCRC16(const uint8* DataPtr, uint32 Length, uint16 StartValue16)` | 计算 CRC16 | SWS_Crc_00004 |
| `uint32 Crc_CalculateCRC32(const uint8* DataPtr, uint32 Length, uint32 StartValue32)` | 计算 CRC32 | SWS_Crc_00005 |
| `uint32 Crc_CalculateCRC32P4(const uint8* DataPtr, uint32 Length, uint32 StartValue32)` | CRC32 P4 变体 | SWS_Crc_00005 |

---

## 7. 处理流程

### 7.1 CRC32 计算流程（查表法）

1. 初始化 `crc = StartValue32 ^ 0xFFFFFFFF`
2. 遍历每个字节: `crc = Table32[(crc ^ byte) & 0xFF] ^ (crc >> 8)`
3. 返回 `crc ^ 0xFFFFFFFF`

---

## 8. 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `CRC_CRC8_SUPPORT` | STD_ON | 启用 CRC8 |
| `CRC_CRC16_SUPPORT` | STD_ON | 启用 CRC16 |
| `CRC_CRC32_SUPPORT` | STD_ON | 启用 CRC32 |
| `CRC_TABLE_LOOKUP` | STD_ON | 使用查表法（FALSE=纯计算） |

---

## 9. 错误处理

- NULL DataPtr → 返回 0（或 StartValue）
- Length = 0 → 返回 StartValue
- 无 DET 报告（纯函数，无状态错误）

---

## 10. 内存与性能

- **RAM**: 0（纯 ROM 查找表）
- **ROM**: CRC8 表 256B + CRC16 表 512B + CRC32 表 1024B ≈ 1.8 KB
- **性能**: 查表法 O(n)，每字节 1 次表查找 + 1 次 XOR + 1 次移位

---

## 11. 集成指南

- NvM 在写入/读取数据块时调用 CRC32 校验
- Com 在信号打包时调用 CRC8 保护
- E2E Profile 使用 CRC16 或 CRC32

---

## 12. 测试策略

- 已知输入/输出向量测试（标准测试向量）
- 空数据 / 单字节 / 大数据块边界测试
- StartValue 非零测试
- NULL 指针容错测试

---

## 13. 实现说明

- 查找表声明为 `const` 放置于 ROM（通过 MemMap 段控制）
- 支持编译时裁剪不需要的 CRC 算法
- 所有函数为可重入纯函数

---

## 14. 参考文献

- AUTOSAR_SWS_CRCLibrary.pdf (R4.4.0)
- yuleASR CRC 模块源码: `src/bsw/services/crc/`
