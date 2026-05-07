# yuleASR Classic AUTOSAR BSW 实施路线图

> 生成时间: 2026-04-29
> 覆盖率: 55.8% (48/86 模块)

---

## 执行摘要

当前项目已实现 **48 个 Classic AUTOSAR BSW 模块**，缺失 **38 个模块**，整体覆盖率为 **55.8%**。

### 已实现模块分布

| 层级 | 已实现 | 缺失 | 覆盖率 |
|-------|--------|-------|--------|
| MCAL | 14 | 8 | 64% |
| ECUAL | 11 | 16 | 41% |
| SERVICES | 20 | 14 | 59% |
| GENERAL | 1 | 0 | 100% |
| OS/RTE | 2 | 0 | 100% |

---

## 第一阶段: 立即完善现有模块 (2-4周)

### 1.1 PduR 强化 (必须首先)

**当前状态**: src/bsw/services/pdur/ 存在但代码较简单

**完善目标**:
- [ ] 实现多层路由策略 (Direct/Fan-Out/Gateway)
- [ ] 添加 TP 段落路由支持
- [ ] 实现动态路由更新
- [ ] 添加完整的单元测试

**参考实现**: 8-10个源文件, 2000-3000行代码

### 1.2 MemIf 完善

**完善目标**:
- [ ] 添加完整的模块配置结构
- [ ] 实现多设备支持
- [ ] 添加NvM集成接口

### 1.3 OS 实现强化

**当前状态**: 仅头文件存在

**完善目标**:
- [ ] 实现基于FreeRTOS的OS抽象层
- [ ] 添加任务调度服务
- [ ] 实现信号量/互斥锁
- [ ] 添加资源访问控制

---

## 第二阶段: 添加高优先级模块 (4-8周)

### 2.1 通信网络管理 (CanNm + CanSm)

**模块列表**:
1. **CanNm** - CAN Network Management (AUTOSAR 标准)
   - 实现OSEK直接管理
   - 添加节点监测
   - 集成ComM

2. **CanSm** - CAN State Manager
   - 状态机管理
   - 集成EcuM启动顺序
   - 链路诊断

**估计工作量**: 2个模块, ~3000行代码

### 2.2 功能管理 (FiM + IpduM)

1. **FiM** - Function Inhibition Manager
   - 障碍识别管理
   - 功能禁用/使能
   - Dem集成

2. **IpduM** - I-PDU Multiplexer
   - 多路复用
   - 动态段落切换
   - PduR集成

### 2.3 安全模块 (CryIf + Crypto + KeyM)

1. **CryIf** - Crypto Interface
   - 统一加密服务接口
   - 支持硬件/软件加密

2. **Crypto** - Crypto Services
   - 密码算法实现
   - 随机数生成
   - 密钥计算

3. **KeyM** - Key Manager
   - 密钥生命周期管理
   - 安全存储
   - 更新机制

---

## 第三阶段: 诊断与调试增强 (2-4周)

### 3.1 Dlt - 诊断日志跟踪

**功能**:
- 运行时日志输出
- 通过以太网输出
- 与Com集成

**应用**: 车辆生产阶段诊断

### 3.2 Xcp - 标定协议

**功能**:
- CCP/XCP 标定支持
- 测量数据获取
- 标定数据下载

**应用**: ECU 标定/刷写

---

## 第四阶段: 扩展功能 (4-8周)

### 4.1 收发器驱动 (CanTrcv + EthTrcv)

- **CanTrcv**: CAN收发器驱动 (TJA1043/TJA1051等)
- **EthTrcv**: 以太网收发器驱动

### 4.2 状态管理 (EthSM)

- 以太网状态转换
- 链路诊断
- ComM集成

### 4.3 FlexRay 支持 (Fr + FrNm + FrArTp)

**必要性**: 低 (除非项目需要FlexRay)

**模块**:
- Fr: FlexRay驱动
- FrNm: FlexRay网络管理
- FrArTp: 自动切换协议

---

## 实施计划模板

### 使用 OSH Orchestrator 启动新模块开发

```bash
# 示例: 启动 PduR 完善项目
/osh init --name=pdur-enhancement --type=embedded --level=standard
/osh discover
/osh spec --create=pdur-phase2
/osh execute --milestone=M1
```

### 模块开发检查清单

每个模块应包含:

```
src/bsw/<layer>/<module>/
├── include/
│   ├── <Module>.h          # 主头文件
│   ├── <Module>_Cfg.h      # 配置头文件
│   └── <Module>_MemMap.h   # 内存映射
├── src/
│   └── <Module>.c          # 源文件
└── test/                   # 单元测试
    ├── <Module>_Test.c
    └── Makefile
```

---

## 开发时间估算

| 阶段 | 模块数 | 预估工时 | 优先级 |
|-------|--------|----------|--------|
| 阶段1 | 3 | 2-4周 | 🔴 必须 |
| 阶段2 | 7 | 4-8周 | 🔴 高 |
| 阶段3 | 2 | 2-4周 | 🟡 中 |
| 阶段4 | 5+ | 4-8周 | 🟢 可选 |

**总体估算**: 12-24周完成所有高中优先级模块

---

## 模块依赖关系图

```
                        ┌─────────────────────────────────────────┐
                        │              APPLICATION (ASW)              │
                        └─────────────────────────────────────────┘
                                         │
                                    RTE  │  (已有)
                                         │
                        ┌─────────────────────────────────────────┐
                        │  SERVICES (已有基础, 需完善)            │
                        │  Com✓  Dcm✓  Dem✓  NvM✓  Csm✓  SecOC✓      │
                        │  PduR⚠ (需强化)  CanNm✗ (缺失)             │
                        │  CanSm✗ (缺失)  FiM✗ (缺失)               │
                        │  CryIf✗  Crypto✗  KeyM✗ (安全需要)        │
                        └─────────────────────────────────────────┘
                                         │
                        ┌─────────────────────────────────────────┐
                        │  ECU Abstraction (已有基础)               │
                        │  CanIf✓  CanTp✓  Fee✓  MemIf⚠            │
                        │  EthIf✓  SoAd✓                           │
                        │  CanTrcv✗  EthTrcv✗ (需添加)            │
                        └─────────────────────────────────────────┘
                                         │
                        ┌─────────────────────────────────────────┐
                        │  Microcontroller Drivers (已有)           │
                        │  Can✓  Eth✓  Fls✓  Spi✓  Mcu✓         │
                        │  Adc✓  Dio✓  Gpt✓  Pwm✓  Icu✓ Ocu✓   │
                        │  Wdg✓  Lin✓  Port✓                      │
                        └─────────────────────────────────────────┘
                                         │
                        ┌─────────────────────────────────────────┐
                        │           Microcontroller (硬件)            │
                        └─────────────────────────────────────────┘

图例: ✓ 完善  ⚠ 需改进  ✗ 缺失
```

---

## 推荐的下一步行动

### 立即执行 (Next Steps)

1. **启动 Phase 1**: 使用 `/osh` 命令启动 PduR 完善项目
2. **并行开发**: 使用多代理同时开发 CanNm 和 CanSm
3. **持续集成**: 每完成一个模块立即进行集成测试

### 使用 OSH 启动

```
用户: /osh init --name=pdur-enhancement --type=embedded --level=standard

AI: ✅ 已初始化 PduR 完善项目
   工作流级别: Standard
   下一步: /osh discover 查找相关技能

用户: /osh discover

AI: ✅ 发现相关技能:
   - autosecure/automotive-safety-design-review
   - embedded-bsw-development
   
用户: /osh spec --create=pdur-phase2

AI: ✅ 已创建 OpenSpec Change: pdur-phase2
   下一步: /osh execute --milestone=M1
```

---

## 附录

### 完整模块清单

详见: `CLASSIC_AUTOSAR_MODULE_GAP_ANALYSIS.json`

### 已实现模块详情

详见: `CLASSIC_AUTOSAR_COVERAGE_ANALYSIS.json`

---

*本文档由 OSH Orchestrator 自动生成*
