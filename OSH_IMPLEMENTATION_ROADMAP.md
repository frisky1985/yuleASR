# 🚀 OSH Orchestrator 实施计划
## Classic AUTOSAR BSW 工程增量开发

> **创建时间**: 2026-04-28  
> **预估周期**: 8-10 周  
> **总代码量**: ~8,700 行  
> **模块数量**: 13 个

---

## 🎯 执行摘要

### 当前状态
- 已实现模块: **32个** (MCAL/ECUAL/Service/OS/RTE)
- 整体完成度: **~68.9%**
- 代码质量: 良好，但缺少测试覆盖

### 目标状态
- 完成后模块数: **45个**
- 目标完成度: **~95%**
- 测试覆盖率: **> 85%**

---

## 📅 阶段计划总览

```
时间线 (周)
┌────────────────────────────────────────────────────────────────────────────────┐
│  W1  │  W2  │  W3  │  W4  │  W5  │  W6  │  W7  │  W8  │  W9  │ W10  │
├────────────────────────────────────────────────────────────────────────────────┤
│ 🔴 P1: 基础设施                │ 🔴 P2: 安全功能                │ 🟡 P3: 网络扩展    │ 🟢 P4: 可选 │
│ Det                      │ Mem                      │ SoAd           │ Icu         │
│ Fls                      │ Csm                      │ SomeIpXf       │ FrTp        │
│ WdgIf/Wdgm               │ SecOC                    │ SomeIpTp       │ Eth完善     │
│                          │                          │ StbM           │             │
└────────────────────────────────────────────────────────────────────────────────┘
```

---

## 🔴 阶段 1: 基础设施与调试支持 (2-3周)

### 目标
建立工程基础设施，提供调试能力和系统安全监控。

### 模块详情

#### 1.1 Det - Development Error Tracer
```yaml
层级: Libraries
代码量: ~300 行
依赖: 无
周期: 3天
```

**API 接口:**
| 函数 | 说明 |
|:-----|:------|
| `Det_Init()` | 初始化Det模块 |
| `Det_ReportError()` | 报告开发错误 |
| `Det_Start()` | 启动错误追踪 |
| `Det_ReportRuntimeError()` | 报告运行时错误 |
| `Det_ReportTransientFault()` | 报告暂态故障 |

**交付物:**
- [ ] `Det.h` - 公共头文件
- [ ] `Det.c` - 核心实现
- [ ] `Det_Cfg.h` - 配置头文件
- [ ] `Det_LCfg.c` - 链接时配置
- [ ] `tests/Det_Test.c` - 单元测试

**验收标准:**
- [ ] 所有API函数实现完成
- [ ] 单元测试覆盖率 > 90%
- [ ] 可通过Dcm/Dem报告错误
- [ ] 支持回调注册机制

---

#### 1.2 Fls - Flash Driver
```yaml
层级: MCAL
代码量: ~800 行
依赖: 无
周期: 1周
```

**API 接口:**
| 函数 | 说明 |
|:-----|:------|
| `Fls_Init()` | 初始化Flash驱动 |
| `Fls_Erase()` | 擦除Flash页面 |
| `Fls_Write()` | 写入Flash数据 |
| `Fls_Read()` | 读取Flash数据 |
| `Fls_Compare()` | 比较Flash数据 |
| `Fls_SetMode()` | 设置操作模式 |
| `Fls_GetStatus()` | 获取驱动状态 |
| `Fls_GetJobResult()` | 获取作业结果 |
| `Fls_Cancel()` | 取消当前作业 |

**交付物:**
- [ ] `Fls.h`, `Fls.c`
- [ ] `Fls_Cfg.h`, `Fls_LCfg.c`
- [ ] `Fls_Internal.h`
- [ ] `tests/Fls_Test.c`

**验收标准:**
- [ ] 支持页面操作(erase/write/read)
- [ ] 支持异步操作模式
- [ ] 支持中断/轮询机制
- [ ] 与Fee/NvM集成通过

---

#### 1.3 WdgIf + Wdgm - 看门狗管理套件
```yaml
层级: ECUAL + Service
代码量: ~600 行
依赖: Wdg (MCAL) - 已存在
周期: 1周
```

**API 接口:**
| 函数 | 说明 |
|:-----|:------|
| `Wdgm_Init()` | 初始化看门狗管理器 |
| `Wdgm_SetMode()` | 设置模式 |
| `Wdgm_CheckpointReached()` | 报告检查点到达 |
| `Wdgm_GetLocalStatus()` | 获取局部状态 |
| `Wdgm_GetGlobalStatus()` | 获取全局状态 |
| `Wdgm_PerformReset()` | 执行复位 |
| `WdgIf_SetTriggerCondition()` | 设置触发条件 |
| `WdgIf_SetMode()` | 设置看门狗模式 |

**交付物:**
- [ ] `WdgIf.h`, `WdgIf.c`, `WdgIf_Cfg.h`
- [ ] `Wdgm.h`, `Wdgm.c`, `Wdgm_Cfg.h`, `Wdgm_LCfg.c`
- [ ] `tests/Wdg_Test.c`

**验收标准:**
- [ ] 支持多个监控实体(SE)
- [ ] 支持死线检测(deadline monitoring)
- [ ] 支持活跃监控(alive monitoring)
- [ ] 支持逻辑监控(logical monitoring)
- [ ] 与EcuM集成

### 里程碑 M1.3 退出准则
- [ ] Det 模块通过单元测试
- [ ] Fls 实现并与Fee集成验证
- [ ] WdgIf/Wdgm 死线监控测试通过

---

## 🔴 阶段 2: CyberSecurity 安全功能 (3-4周)

### 目标
实现现代汽车网络安全功能(CyberSecurity)，满足ISO/SAE 21434要求。

### 模块详情

#### 2.1 Mem - Memory Services
```yaml
层级: Service
代码量: ~400 行
依赖: 无
周期: 4天
```

**API 函数:**
- `Mem_Init()`, `Mem_Alloc()`, `Mem_Free()`, `Mem_Realloc()`, `Mem_GetStats()`

**验收标准:**
- [ ] 支持堆内存分配
- [ ] 防止内存泄漏
- [ ] 统计信息可查
- [ ] 线程安全

---

#### 2.2 Csm - Crypto Services Manager
```yaml
层级: Service
代码量: ~1200 行
依赖: Mem
周期: 1.5周
风险: 高 (加密算法复杂)
```

**API 接口:**
| 函数 | 说明 |
|:-----|:------|
| `Csm_Init()` | 初始化加密服务 |
| `Csm_Encrypt()` | AES加密 |
| `Csm_Decrypt()` | AES解密 |
| `Csm_MacGenerate()` | 生成MAC |
| `Csm_MacVerify()` | 验证MAC |
| `Csm_SignatureGenerate()` | 生成数字签名 |
| `Csm_SignatureVerify()` | 验证数字签名 |
| `Csm_Hash()` | 哈希计算 |
| `Csm_RandomGenerate()` | 随机数生成 |
| `Csm_KeyManagement()` | 密钥管理 |

**算法支持:**
- AES-128/192/256 (ECB, CBC, CTR)
- HMAC-SHA256, CMAC-AES
- SHA-256, SHA-384
- ECDSA签名
- 真随机数生成

**风险缓解:**
> 建议集成 mbedTLS 或自己的轻量级加密库，避免从头实现加密算法。

---

#### 2.3 SecOC - Secure Onboard Communication
```yaml
层级: Service
代码量: ~800 行
依赖: Csm, PduR, Com
周期: 1.5周
```

**API 接口:**
| 函数 | 说明 |
|:-----|:------|
| `SecOC_Init()` | 初始化安全通信 |
| `SecOC_Transmit()` | 发送安全消息 |
| `SecOC_IfTransmit()` | IF层发送 |
| `SecOC_RxIndication()` | 接收通知 |
| `SecOC_MainFunctionRx()` | RX主函数 |
| `SecOC_MainFunctionTx()` | TX主函数 |
| `SecOC_VerifyStatusOverride()` | 覆盖验证状态 |

**安全功能:**
- 消息认证码(MAC)生成与验证
- 重放攻击防护(Freshness Value管理)
- 同步/异步验证模式
- 可配置安全等级

### 里程碑 M2.3 退出准则
- [ ] Mem 内存管理测试通过
- [ ] Csm AES/HMAC功能验证
- [ ] SecOC 与Com/PduR集成测试通过
- [ ] 安全通信整合测试

---

## 🟡 阶段 3: 网络通信扩展 (2-3周)

### 目标
扩展以太网和SOME/IP通信能力支持服务架构。

### 模块详情

#### 3.1 SoAd - Socket Adapter
```yaml
层级: Service
代码量: ~1000 行
依赖: EthIf, TcpIp
周期: 1周
```

**API 函数:**
- TCP/UDP套接字管理
- PduR集成接口
- DoIP支持
- 连接管理

---

#### 3.2 SomeIpXf - SOME/IP Transformer
```yaml
层级: Service
代码量: ~600 行
依赖: Com, PduR
周期: 5天
```

**API 函数:**
- `SomeIpXf_Transform()` - 序列化
- `SomeIpXf_InvTransform()` - 反序列化
- 支持所有AUTOSAR数据类型

---

#### 3.3 SomeIpTp - SOME/IP Transport Protocol
```yaml
层级: Service
代码量: ~500 行
依赖: SomeIpXf, PduR
周期: 4天
```

**API 函数:**
- 分段传输
- 重组装功能
- 超时处理

---

#### 3.4 StbM - Synchronized Time Base Manager
```yaml
层级: Service
代码量: ~700 行
依赖: Gpt
周期: 6天
```

**API 函数:**
- `StbM_GetCurrentTime()` - 获取当前时间
- `StbM_SetGlobalTime()` - 设置全局时间
- `StbM_UpdateGlobalTime()` - 更新时间
- gPTP同步支持

### 里程碑 M3.4 退出准则
- [ ] SoAd TCP/UDP通信测试通过
- [ ] SomeIpXf 序列化测试通过
- [ ] StbM 时间同步功能验证

---

## 🟢 阶段 4: 可选驱动与优化 (1-2周)

### 模块详情

#### 4.1 Icu - Input Capture Unit
```yaml
层级: MCAL
代码量: ~400 行
依赖: Gpt
周期: 4天
优先级: 可选
```

**功能:**
- 边沿捕获
- 信号测量(占空比/周期)
- 时间戳功能
- 唤醒支持

---

#### 4.2 FrTp - FlexRay Transport Protocol
```yaml
层级: ECUAL
代码量: ~600 行
依赖: FrIf, PduR
周期: 5天
优先级: 可选
```

**功能:**
- ISO 10681-2 依从
- 多连接支持
- 流量控制

---

#### 4.3 Eth 驱动完善
```yaml
层级: MCAL
代码量: ~800 行 (补充)
依赖: 无
周期: 5天
优元级: 高 (现有为骨架)
```

**需补充的API:**
- MAC控制器初始化
- 数据收发
- MDIO读写
- 中断处理

### 里程碑 M4.3 退出准则
- [ ] Icu功能测试通过
- [ ] Eth驱动核心功能完成

---

## 🔗 模块依赖关系图

```
┌──────────────────────────────────────────────────────────────────────────────────┐
│                        阶段 1: 基础设施                              │
├────────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌────────┐        ┌────────┐        ┌────────┐                    │
│   │   Det   ──────┾│   Fls   ──────┾│ WdgIf  ──┾───────────────────────┾────────┐       │
│   └────────┘        └────────┘        └────────┘                    │ ┌────────┐  │
│                                                 │                        │─┾│  Wdgm  │  │
│                                                 │                        │  └────────┘  │
│                                                 │                        │                        │
├────────────────────────────────────────────────────────────────────────────────┤
│                        阶段 2: 安全功能                              │
├────────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌────────┐        ┌────────┐        ┌────────┐                    │
│   │   Mem   ──────┾│   Csm   ──────┾│  SecOC  ──┾─────────────────────┾─────────────┾───────────────┐  │
│   └────────┘        └────────┘        └────────┘                    │─┾────────────┾─────┾│  PduR  │  │
│                                                              │           │ ┌────────┐│  │
│                                                              └──────────┾──────┾│  Com   │  │
│                                                                          └────────┘  │
├────────────────────────────────────────────────────────────────────────────────┤
│                        阶段 3: 网络扩展                              │
├────────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌────────┐        ┌────────────────┐       ┌────────────┐       ┌────────┐  │
│   │   StbM  ──────┾│  SomeIpXf  ─────┾──────┾│ SomeIpTp  ──┾─────┾──────┾│  SoAd  │  │
│   └────────┘        └────────────────┘       └────────────┘       └────────┘  │
│                                                                             │
├────────────────────────────────────────────────────────────────────────────────┤
│                        阶段 4: 可选/优化                              │
└────────────────────────────────────────────────────────────────────────────────┘
│   ┌────────┐        ┌────────┐        ┌────────┐                    │
│   │   Icu   ──────┾│  FrTp   ──────┾│  Eth+  ────────────────────────────────────────────────────│
│   └────────┘        └────────┘        └────────┘                    │
└────────────────────────────────────────────────────────────────────────────────┘
```

---

## 📋 质量门禁 (Quality Gates)

### Gate 1: 代码审查
- [ ] AUTOSAR C 编码规范遵守
- [ ] 圈复杂度 < 10
- [ ] MISRA C:2012 合规检查通过
- [ ] 代码覆盖率 > 80%

### Gate 2: 集成测试
- [ ] 与上下游模块集成测试通过
- [ ] OpenSpec 场景测试通过
- [ ] 无内存泄漏 (静态分析)
- [ ] 无竞态条件 (ThreadSanitizer)

### Gate 3: 文档完整
- [ ] SWS 规范文档完整
- [ ] API 参考文档生成
- [ ] 配置参数说明
- [ ] 集成指南更新

---

## ⚠️ 风险与缓解策略

| 风险 | 影响 | 缓解策略 |
|:-----|:------|:---------|
| Csm 加密算法实现复杂 | 高 | 集成 mbedTLS 或自己的轻量级加密库 |
| SecOC 与现有Com/PduR集成 | 中 | 先实现原型验证接口兼容性 |
| Fls Flash 操作风险 | 中 | 充分测试异步操作和错误恢复 |
| 时间预估偏差 | 中 | 每阶段设置缓冲时间，允许迭代调整 |

---

## 📁 相关文件

- `OSH_IMPLEMENTATION_PLAN.json` - 机器可读的计划文件
- `CLASSIC_AUTOSAR_GAP_ANALYSIS.md` - 差距分析报告
- `openspec/changes/` - OpenSpec 规范定义
- `.harness/state.json` - Harness 项目状态

---

## 📞 执行命令

使用 OSH Orchestrator 执行开发:

```bash
# 初始化当前阶段
/osh execute --milestone=M1.1

# 查看状态
/osh status --verbose

# 验证完成
/osh validate --gate=1
```

---

## 📝 版本历史

| 版本 | 日期 | 说明 |
|:-----|:------|:------|
| 1.0.0 | 2026-04-28 | 初始版本 |

---

*本文档由 OSH Orchestrator 自动生成*
