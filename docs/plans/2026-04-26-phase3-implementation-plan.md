# Phase 3 实施计划

**日期**: 2026-04-26  
**目标**: 完善配置工具、补全DCM服务、搭建HIL测试框架、MISRA合规

---

## 任务分配

### Agent 1: DDS配置工具完善 (Config Tool Enhancement)

**负责人**: Agent-Config  
**估计工期**: 5天  
**依赖**: 无

#### 任务列表
1. **诊断服务配置模块**
   - DCM会话参数配置 (P2/P2* timeout, S3 timeout)
   - 服务支持列表编辑器 (0x10, 0x11, 0x22, 0x2E, 0x27, 0x28, 0x2C, 0x34, 0x36, 0x37)
   - DID数据库管理 (0xF190 VIN, 0xF194 软件版本, 自定义DID)
   - 安全策略映射 (会话级别到服务访问权限)

2. **E2E配置模块**
   - Profile选择器 (P01/P02/P04/P05/P06/P07/P11/P22)
   - 数据ID映射管理
   - CRC多项式配置 (自定义多项式支持)
   - 状态机参数 (Window Size, Max Delta Counter)

3. **OTA配置模块**
   - 活动配置编辑器 (Campaign/Package/ECU关联)
   - 分区布局可视化 (A/B分区大小/偏移设置)
   - 下载参数 (HTTP服务器URL, 超时, 重试次数)
   - 安全验证配置 (签名算法、公钥指纹)

4. **配置验证与导出**
   - JSON Schema验证
   - 自动生成C头文件 (#define 配置宏)
   - 配置版本管理 (升级差异对比)

#### 输出物
- `tools/config_tool/diagnostic_config.py` - 诊断配置模块
- `tools/config_tool/e2e_config.py` - E2E配置模块
- `tools/config_tool/ota_config.py` - OTA配置模块
- `tools/config_tool/code_generator.py` - 代码生成器
- `tools/config_tool/gui/main_window_enhanced.py` - 增强版GUI

---

### Agent 2: DCM服务补全 (Missing UDS Services)

**负责人**: Agent-DCM  
**估计工期**: 4天  
**依赖**: 无

#### 任务列表
1. **0x27 SecurityAccess (安全访问)**
   - Seed-Key机制 (Request Seed / Send Key)
   - 多级别安全 (Level 1-5)
   - 安全超时锁定 (X1/X2错误计数)
   - 密钥衍生支持 (CSM集成)

2. **0x28 CommunicationControl (通信控制)**
   - 通信类型 (Normal/NetworkManagement/NormalAndNM)
   - 通信子网控制 (启用/禁用)
   - 应用层协议支持 (DDS Topic使能/禁用)

3. **0x2C DynamicallyDefineDataIdentifier (动态定义DID)**
   - 按地址定义 (Address + Size + Memory Type)
   - 按DID定义 (Source DID + Position + Size)
   - 动态DID清除

4. **0x3D WriteMemoryByAddress (按地址写入内存)**
   - 内存写入 (Address + Size + Data)
   - 写入前校验 (Memory Boundary/Protection)
   - 操作安全认证 (Pre-required Security Level)

5. **0x31 RoutineControl (例行程序控制)**
   - 启动例行程序 (Start Routine)
   - 停止例行程序 (Stop Routine)
   - 请求例行程序结果 (Request Results)
   - 预定义例行程序 (EraseMemory, CheckProgrammingDependencies)

#### 输出物
- `src/diagnostics/dcm/dcm_security.c/h` - 安全访问服务
- `src/diagnostics/dcm/dcm_communication.c/h` - 通信控制服务
- `src/diagnostics/dcm/dcm_dynamic_did.c/h` - 动态DID服务
- `src/diagnostics/dcm/dcm_memory.c/h` - 内存写入服务
- `src/diagnostics/dcm/dcm_routine.c/h` - 例行程序控制服务
- `tests/test_dcm_extended.c` - 扩展测试用例

---

### Agent 3: HIL测试框架 (Hardware-in-Loop)

**负责人**: Agent-HIL  
**估计工期**: 6天  
**依赖**: Agent 2 (部分测试需要完整DCM)

#### 任务列表
1. **HIL硬件抽象层**
   - CAN接口抽象 (PCAN/Vector/Kvaser)
   - 以太网接口抽象 (Raw Socket/pcap)
   - 诊断仪模拟器 (Test Equipment Simulator)

2. **测试用例管理**
   - UDS测试序列 (活动会话/服务调用/错误处理)
   - E2E保护测试 (Profile验证/错误检测/状态转换)
   - OTA流程测试 (下载/验证/安装/回滚)
   - SecOC安全测试 (MAC验证/Freshness同步/重放攻击)

3. **测试报告生成**
   - 测试结果记录 (Pass/Fail/Skip统计)
   - 覆盖率报告 (代码行覆盖/分支覆盖)
   - 性能指标 (Latency/Throughput/Timing)
   - HTML/PDF报告导出

4. **CI集成**
   - GitHub Actions工作流
   - 自动化测试触发
   - 测试结果上传

#### 输出物
- `tests/hil/hil_interface.h/c` - HIL硬件抽象层
- `tests/hil/test_sequences/uds_sequences.py` - UDS测试序列
- `tests/hil/test_sequences/e2e_sequences.py` - E2E测试序列
- `tests/hil/test_sequences/ota_sequences.py` - OTA测试序列
- `tests/hil/report_generator.py` - 报告生成器
- `.github/workflows/hil-tests.yml` - CI工作流

---

### Agent 4: MISRA C:2012合规 (Static Analysis)

**负责人**: Agent-MISRA  
**估计工工期**: 3天  
**依赖**: 无

#### 任务列表
1. **静态分析配置**
   - PC-lint Plus / Cppcheck 配置文件
   - MISRA C:2012规则集 (Required/Advisory)
   - 账驾级别定制 (ASIL-D扩展规则)
   - 抑制任务 (需要偏离申请的规则)

2. **代码修复 (Category 1 - Required)**
   - Rule 15.5: 单一出口点
   - Rule 17.7: 返回值使用
   - Rule 20.7: 宏参数括号
   - Rule 21.3: 标准库内存管理

3. **偏离申请 (Deviation Permits)**
   - 计算效率优先的场景
   - 第三方代码接口
   - 硬件相关代码

4. **自动化检查**
   - 预提交钩子
   - CI集成
   - 每日扫描报告

#### 输出物
- `tools/misra/pclint_config.lnt` - PC-lint配置
- `tools/misra/cppcheck_suppressions.xml` - Cppcheck抑制列表
- `tools/misra/deviation_permits.md` - 偏离申请文档
- `scripts/run_misra_check.sh` - 检查脚本
- `docs/compliance/MISRA_AUDIT.md` - 审计报告

---

## 开发时间线

```
Day 1-3:  
  [并行] Agent 1: 配置工具核心模块
  [并行] Agent 2: 0x27安全访问 + 0x28通信控制
  [并行] Agent 3: HIL硬件抽象层
  [并行] Agent 4: MISRA配置 + Required规则修复

Day 4-5:
  [并行] Agent 1: OTA配置 + 代码生成器
  [并行] Agent 2: 0x2C动态DID + 0x3D内存写入 + 0x31例行程序
  [并行] Agent 3: 测试序列开发
  [并行] Agent 4: 偏离申请 + 自动化集成

Day 6:
  [并行] Agent 1: 配置验证 + GUI集成
  [并行] Agent 3: 报告生成 + CI集成
  [整合] 所有Agents: 代码审查和合并

Day 7:
  [验证] 全员: 集成测试
  [文档] 更新设计文档和API参考
```

---

## 验收标准

| 检查项 | 标准 | 验证方法 |
|--------|------|----------|
| 配置工具完整性 | 支持诊断/E2E/OTA配置 | GUI演示 |
| DCM服务完整性 | 实现0x27/0x28/0x2C/0x3D/0x31 | 单元测试通过 |
| HIL测试覆盖 | UDS/E2E/OTA流程 | 测试报告 |
| MISRA合规 | Required规列100%通过 | 静态分析报告 |

---

## 风险与应对

| 风险 | 影响 | 应对措施 |
|------|------|----------|
| 诊断服务复杂度高 | Agent 2延期 | 优先实现必需服务(0x27/0x28) |
| HIL硬件兼容性 | Agent 3阻塞 | 提供软件模拟后退方案 |
| MISRA修复量大 | Agent 4延期 | 分批处理，先修复新代码 |

---

*计划版本: v1.0*  
*最后更新: 2026-04-26*
