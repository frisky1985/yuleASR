# yuleASR — AUTOSAR BSW 平台规范文档

> **文档版本**: v1.1.0  
> **适用范围**: yuleASR AUTOSAR Classic Platform BSW 实现  
> **合规标准**: AUTOSAR R21-11, MISRA C:2023, ASPICE SWE.5

---

## 1. 项目定位

yuleASR 是一个轻量级 AUTOSAR Classic Platform BSW (Basic Software) 参考实现，专注于嵌入式汽车电子控制单元 (ECU) 的基础软件栈。本项目覆盖 MCAL (Microcontroller Abstraction Layer)、ECUAL (ECU Abstraction Layer) 和服务层 (Services Layer) 的核心模块。

### 1.1 目标平台
- NXP S32K312 (ARM Cortex-M7)
- ARM Cortex-M4/M7 系列 MCU
- 符合 ISO 26262 ASIL-B 安全等级

---

## 2. 整体架构

```
┌─────────────────────────────────────────────┐
│               Application Layer              │
├─────────────────────────────────────────────┤
│              RTE (Runtime Environment)        │
├──────────────────┬──────────────────────────┤
│   Services Layer  │      ECU Abstraction     │
│   (OS, Com, Dem, │      (ECUAL)             │
│    Det, EcuM)    │  (Wdg, Dio, Port, etc.)  │
├──────────────────┴──────────────────────────┤
│        MCAL (Microcontroller Abstraction)    │
│   (ADC, CAN, LIN, SPI, I2C, PWM, GPT, ICU)  │
├─────────────────────────────────────────────┤
│               Microcontroller                │
└─────────────────────────────────────────────┘
```

### 2.1 层级职责

| 层级 | 职责 | SHALL/SHOULD/MAY |
|------|------|-----------------|
| MCAL | 直接操作MCU外设寄存器，提供标准化硬件抽象接口 | SHALL 隐藏硬件细节 |
| ECUAL | 封装MCAL，提供 ECU 级功能抽象 | SHALL 依赖 MCAL API |
| Services | 提供 OS、通信、诊断、ECU 管理等基础服务 | SHALL 使用标准 AUTOSAR 接口 |
| RTE | 运行时环境，连接应用与 BSW | MAY 仅提供 stub 实现 |

---

## 3. 功能需求 (SHALL/SHOULD/MAY)

### 3.1 MCAL 模块

| ID | 需求 | 优先级 |
|----|------|--------|
| MCAL-SHALL-001 | MCAL SHALL 提供标准 AUTOSAR API (如 Adc_Init, Can_Write) | 强制 |
| MCAL-SHALL-002 | 所有 MCAL 模块 SHALL 支持同步和中断两种操作模式 | 强制 |
| MCAL-SHALL-003 | MCAL SHALL 使用 MISRA C:2023 合规编码风格 | 强制 |
| MCAL-SHOULD-001 | MCAL SHOULD 提供 DMA 支持以降低 CPU 负载 | 建议 |
| MCAL-MAY-001 | MCAL MAY 支持硬件触发器级联 | 可选 |

### 3.2 ECUAL 模块

| ID | 需求 | 优先级 |
|----|------|--------|
| ECUAL-SHALL-001 | ECUAL SHALL 使用 MCAL API, 不直接操作硬件寄存器 | 强制 |
| ECUAL-SHALL-002 | 看门狗管理器 SHALL 在超时前刷新 | 强制 |
| ECUAL-SHOULD-001 | 数字 I/O 抽象 SHOULD 支持运行时引脚重配置 | 建议 |
| ECUAL-MAY-001 | ECUAL MAY 实现休眠模式唤醒检测逻辑 | 可选 |

### 3.3 Services 层

| ID | 需求 | 优先级 |
|----|------|--------|
| SVC-SHALL-001 | OS SHALL 提供符合 OSEK/AUTOSAR OS 的调度服务 | 强制 |
| SVC-SHALL-002 | 通信栈 (CAN/以太网) SHALL 实现 PDU 路由 | 强制 |
| SVC-SHALL-003 | 诊断事件管理器 (Dem) SHALL 记录并上报 DTC | 强制 |
| SVC-SHOULD-001 | ECU 管理器 SHOULD 支持多唤醒源 | 建议 |
| SVC-MAY-001 | 通信栈 MAY 支持 LIN 和 FlexRay | 可选 |

---

## 4. 非功能需求

| ID | 需求 | 指标 |
|----|------|------|
| NFR-SHALL-001 | 代码 MISRA C:2023 合规 | 零 Required 违规 |
| NFR-SHALL-002 | 单元测试行覆盖率 | ≥85% |
| NFR-SHALL-003 | 条件覆盖率 | ≥80% |
| NFR-SHALL-004 | 静态分析 (cppcheck) | 无报错 |
| NFR-SHOULD-001 | 模块间解耦 | 模块件头文件依赖 ≤5 |
| NFR-MAY-001 | 性能基准 | 中断延迟 ≤1μs |

---

## 5. 文件组织

```
yuleASR/
├── src/
│   ├── mcal/        # MCAL 模块
│   ├── ecual/       # ECU 抽象层
│   ├── services/    # 服务层 (OS, Com, Dem, EcuM, Det)
│   └── test/        # 测试框架
├── include/         # 公共头文件
├── docs/            # 文档
├── specs/           # 规格说明书
├── tests/           # 测试用例
├── config/          # 配置
└── tools/           # 工具链
```

---

## 6. 追溯矩阵

所有需求通过 `REQ-XXX` 标识符追溯至 AUTOSAR SRS 和测试用例。参见 `specs/module-requirements.md` 和 `docs/architecture.md`。

---

## 7. MISRA 合规策略

- SHALL 使用 MISRA C:2023 `safety` 配置
- SHOULD 对所有代码执行静态分析
- MAY 在偏差审批后放宽性能关键路径的规则
- 偏差记录: `specs/misra-acceptance-matrix.md`
