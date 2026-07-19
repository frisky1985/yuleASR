# yuleASR — 统一架构定位文档

> **版本**: v1.1.0  
> **最后更新**: 2026-07-19  
> **类别**: 架构定位 (SWE.5 V-Model 左侧)

---

## 1. 项目定位

yuleASR（yule AUTOSAR Stack Reference）是一个**教育级/参考级 AUTOSAR Classic Platform BSW 实现**，其核心定位为：

- **学习平台**: 帮助嵌入式工程师理解 AUTOSAR BSW 的分层架构和模块接口
- **参考实现**: 展示 MCAL/ECUAL/Services 各层模块的标准化 API 设计
- **原型基础**: 可在 NXP S32K312 上进行硬件验证和功能原型开发

> ⚠️ **免责声明**: yuleASR 不是生产级 AUTOSAR 栈，不适用于量产汽车 ECU。  
> 如需生产级实现，请使用 Vector MICROSAR、EB tresos 等商业方案。

---

## 2. 与 AUTOSAR 标准的关系

### 2.1 覆盖范围

| AUTOSAR 层级 | yuleASR 覆盖 | 说明 |
|-------------|-------------|------|
| 应用层 (SWC) | ⚠️ 部分 | RTE stub 提供 |
| RTE | ⚠️ 部分 | 仅提供 header stub |
| Services Layer | ✅ 主要 | OS, Com, Dem, Det, EcuM |
| ECUAL | ✅ 主要 | WdgM, Dio, Port, PWM |
| MCAL | ✅ 主要 | Adc, Can, Lin, Spi, I2c, Gpt, Icu, Pwm, Fls |
| 微控制器 | ✅ 完整 | NXP S32K312 BSP |

### 2.2 合规等级

- **AUTOSAR R21-11 API**: 接口签名和语义兼容
- **配置方式**: 手动结构体配置 (非 XML/arxml 自动生成)
- **MISRA C:2023**: Required 规则强制合规
- **ASPICE**: 遵循 V-Model 文档要求，非正式认证

---

## 3. 技术栈

| 组件 | 选型 | 说明 |
|------|------|------|
| MCU | NXP S32K312 | ARM Cortex-M7, 120 MHz |
| 编译器 | ARM GCC (arm-none-eabi-gcc) | v12+ |
| 静态分析 | cppcheck | MISRA C:2023 规则 |
| 单元测试 | Unity/CMock, pytest | C + Python |
| 构建系统 | CMake | v3.25+ |
| CI/CD | yuleOSH | Pipeline 自动验证 |

---

## 4. 架构决策记录 (ADR)

### ADR-001: 分层而不分进程
- **决策**: BSW 各层运行在同一地址空间，无 MMU 隔离
- **理由**: AUTOSAR Classic Platform 定义为单核/单地址空间运行
- **后果**: 各层通过函数调用直接交互，需通过模块间 API 契约保证隔离

### ADR-002: 静态配置优先
- **决策**: 所有模块使用编译期静态配置结构体
- **理由**: 符合 AUTOSAR 的配置阶段概念，避免运行时动态配置风险
- **后果**: 缺少运行时配置灵活性，但满足 MISRA 对动态内存的限制

### ADR-003: MISRA 规则集使用 safety profile
- **决策**: 所有生产代码使用 MISRA C:2023 `safety` profile
- **理由**: ASIL-B 目标等级需要最严格的规则集
- **后果**: 部分性能优化代码需提交偏差审批

---

## 5. 质量目标

| 指标 | 目标值 | 当前值 (2026-07-19) | 状态 |
|------|--------|-------------------|------|
| MISRA Required 违规 | 0 | - | 📊 |
| 单元测试通过率 | ≥95% | 95.7% | ✅ |
| 行覆盖率 | ≥85% | 82.5% | ⚠️ |
| 集成测试通过率 | ≥90% | 93.7% | ✅ |
| 合格性测试 | 100% | 100% | ✅ |

---

## 6. 依赖关系

```
┌──────────────────────────────────────┐
│           Application                 │
└──────────────┬───────────────────────┘
               │ AUTOSAR API
┌──────────────▼───────────────────────┐
│          RTE (Stub)                   │
└──────────────┬───────────────────────┘
               │
     ┌─────────┴──────────┐
     │                    │
┌────▼─────┐     ┌───────▼───────┐
│ Services │     │    ECUAL      │
│ (OS,Com, │     │ (Wdg,Dio,    │
│  Dem,    │     │  Port,PWM)   │
│  Det)    │     │              │
└────┬─────┘     └───────┬───────┘
     │                   │
     └──────┬────────────┘
            │
┌───────────▼────────────┐
│        MCAL            │
│ (Adc,Can,Lin,Spi,I2c,  │
│  Gpt,Icu,Pwm,Fls)      │
└───────────┬────────────┘
            │ Hardware Abstraction
┌───────────▼────────────┐
│      HAL (S32K312)      │
└────────────────────────┘
```
