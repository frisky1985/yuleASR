# EA Design Document

> **Module ID**: 0x31 (49)  
> **AUTOSAR Layer**: ECUAL  
> **AUTOSAR Version**: Classic Platform 4.4.0  
> **SWS Reference**: AUTOSAR SWS EEPROM Abstraction  
> **Source Path**: `src/bsw/ecual/ea/`  
> **Doc Version**: 1.0  
> **Status**: Approved

---

## 1. 模块概述

EA (EEPROM Abstraction) 提供 EEPROM/Flash 模拟非易失性存储的统一抽象接口。EA 将逻辑 Block 映射到物理 EEPROM 扇区，管理磨损均衡、数据搬移和坏块处理。EA 被 Mem 和 NvM 用作底层存储后端。

---

## 2. 标准与依赖

### 2.1 遵循标准

| 标准 | 版本 | 说明 |
|------|------|------|
| AUTOSAR SWS EEPROM Abstraction | 4.4.0 | EA 规范 |

### 2.2 模块依赖

| 依赖方向 | 模块 | 说明 |
|----------|------|------|
| 上层 | Mem, MemIf, NvM | 数据读写请求 |
| 下层 | Fls/Eep (MCAL) | 底层 Flash/EEPROM 驱动 |
| 下层 | Det | 错误报告 |

---

## 3. 架构设计

### 3.1 分层位置

```
┌─────────────────────────────────────┐
│      Mem / MemIf / NvM              │
├─────────────────────────────────────┤
│         EA (ECUAL)                  │
├─────────────────────────────────────┤
│      Fls / Eep (MCAL)               │
└─────────────────────────────────────┘
```

### 3.2 内部组件

- **Block Manager**: 逻辑块到物理扇区的映射
- **Wear Leveling**: 磨损均衡算法
- **Job Executor**: 异步读写/擦除作业执行
- **Data Integrity**: 数据完整性校验（CRC）

### 3.3 文件结构

```
src/bsw/ecual/ea/
├── include/
│   ├── Ea.h       # 公共 API
│   └── Ea_Cfg.h   # 扇区/块配置
└── src/
    ├── Ea.c        # 核心实现
    └── Ea_Lcfg.c   # 链接时配置
```

---

## 4. 状态机

```
          Ea_Init()
  UNINIT ──────────────► IDLE
                           │
              Ea_Read/Write/Erase()
                           │
                           ▼
                        BUSY
                           │
              Job Complete (Fls callback)
                           │
                           ▼
                         IDLE
```

---

## 5. 数据结构

```c
typedef enum {
    EA_JOB_NONE = 0,
    EA_JOB_READ,
    EA_JOB_WRITE,
    EA_JOB_ERASE
} Ea_JobType;

typedef struct {
    uint16 BlockNumber;
    uint32 PhysicalAddress;
    uint16 BlockSize;
    uint8  EraseCount;   /* 擦除计数（磨损均衡） */
} Ea_BlockDescriptorType;
```

---

## 6. API 规范

| API | 说明 | SWS 需求 |
|-----|------|----------|
| `void Ea_Init(const Ea_ConfigType* Config)` | 初始化 | SWS_Ea_00001 |
| `void Ea_DeInit(void)` | 反初始化 |  |
| `Std_ReturnType Ea_Read(uint16 BlockNumber, uint16 Offset, uint8* DataPtr, uint16 Length)` | 读取 | SWS_Ea_00005 |
| `Std_ReturnType Ea_Write(uint16 BlockNumber, uint16 Offset, const uint8* DataPtr, uint16 Length)` | 写入 | SWS_Ea_00006 |
| `Std_ReturnType Ea_Erase(uint16 BlockNumber)` | 擦除块 |  |
| `void Ea_MainFunction(void)` | 周期主函数 | SWS_Ea_00003 |
| `void Ea_GetVersionInfo(Std_VersionInfoType* VersionInfo)` | 版本信息 | SWS_Ea_00002 |

---

## 7. 处理流程

### 7.1 写入流程

1. 上层调用 `Ea_Write(BlockNumber, Offset, Data, Length)`
2. EA 查找 BlockNumber 对应的物理地址
3. 若目标扇区已满 → 触发数据搬移（Garbage Collection）
4. 调用 Fls_Write 写入物理 Flash
5. 写入 CRC 校验值
6. 完成后回调上层

---

## 8. 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `EA_NUM_BLOCKS` | 16U | 逻辑块数量 |
| `EA_BLOCK_SIZE` | 256U | 每块大小 (字节) |
| `EA_SECTOR_SIZE` | 4096U | 物理扇区大小 |
| `EA_WEAR_LEVELING` | STD_ON | 启用磨损均衡 |
| `EA_CRC_CHECK` | STD_ON | 启用 CRC 校验 |

---

## 9. 错误处理

| 错误码 | 触发条件 |
|--------|----------|
| `EA_E_UNINIT` | 初始化前调用 |
| `EA_E_INV_BLOCK` | 无效块号 |
| `EA_E_INVALID_OFFSET` | 偏移越界 |
| `EA_E_FLASH_ERROR` | 底层 Flash 操作失败 |
| `EA_E_CRC_MISMATCH` | CRC 校验不匹配 |

---

## 10. 内存与性能

- **RAM**: 块描述表 ~16 × 12B = 192 字节 + 作业状态
- **ROM**: ~4 KB 代码
- **性能**: Read ~10 µs, Write ~1 ms (Flash 编程时间)

---

## 11. 集成指南

- Mem 通过 EA 实现 EEPROM 存储后端
- EA 依赖 Fls MCAL 驱动进行实际 Flash 操作
- 磨损均衡参数需根据 Flash 寿命计算

---

## 12. 测试策略

- 读写往返正确性测试
- 擦除/重写循环测试
- 磨损均衡行为测试
- CRC 损坏检测测试
- 边界偏移测试

---

## 13. 实现说明

- 逻辑块使用索引表映射到物理地址
- 写入采用追加模式（不原地覆盖），擦除后重建索引
- CRC 使用 CRC32 算法

---

## 14. 参考文献

- AUTOSAR_SWS_EEPROMAbstraction.pdf (R4.4.0)
- yuleASR EA 源码: `src/bsw/ecual/ea/`
