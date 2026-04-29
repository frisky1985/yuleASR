# UDS 完整服务实现任务清单

**项目**: eth-dds-integration  
**版本**: 1.3.0  
**OpenSpec Change**: uds-complete-optimization  
**目标**: 完整实现 ISO 14229-1 UDS 服务 + DoCAN/DoIP 传输层优化

---

## 里程碑 M1: 核心 UDS 服务

### T001: 实现 0x22 Read Data By Identifier
**状态**: 待执行  
**优先级**: 高  
**估计时长**: 1天

**需求**:
- 实现 ISO 14229-1 第10.3节
- 支持标准数据标识符 (0xF1xx-0xF6xx, 0x0100-0xEFFF)
- 支持多 DID 读取请求
- 支持动态长度数据响应
- 实现 UDS_NRC 处理:
  - requestSequenceError (0x24)
  - requestOutOfRange (0x31)
  - securityAccessDenied (0x33)

**输出物**:
- `dcm_did.h/c` - DID 管理模块
- 更新 `dcm.c` - 添加 0x22 路由

---

### T002: 实现 0x2E Write Data By Identifier
**状态**: 待执行  
**优先级**: 高  
**估计时长**: 1天

**需求**:
- 实现 ISO 14229-1 第10.6节
- 支持写入安全校验
- 支持写入回调机制
- 实现 UDS_NRC 处理:
  - conditionsNotCorrect (0x22)
  - requestOutOfRange (0x31)
  - securityAccessDenied (0x33)
  - generalProgrammingFailure (0x72)

**输出物**:
- 扩展 `dcm_did.c/h` - 添加写入支持
- 更新 `dcm.c` - 添加 0x2E 路由

---

### T003: 实现 0x3E Tester Present
**状态**: 待执行  
**优先级**: 高  
**估计时长**: 0.5天

**需求**:
- 实现 ISO 14229-1 第10.5节
- 支持 sub-function 0x00 (zeroSubFunction) 和 0x80 (zeroSubFunction + SPRMIB)
- 与 Session 超时机制集成
- 保持会话活动状态

**输出物**:
- 更新 `dcm_session.c` - 添加 tester present 处理
- 更新 `dcm.c` - 添加 0x3E 路由

---

### T004: 实现 0x34/0x35/0x36/0x37 程序传输服务
**状态**: 待执行  
**优先级**: 中  
**估计时长**: 2天

**需求**:
- 0x34 Request Download (下载请求)
- 0x35 Request Upload (上传请求)
- 0x36 Transfer Data (数据传输)
- 0x37 Request Transfer Exit (退出传输)
- 实现 blockSequenceCounter 管理
- 支持多种压缩方法 (0x00=未压缩)
- 支持多种加密方法 (0x00=未加密)
- 实现 transferRequestParameterRecord 处理

**输出物**:
- `dcm_transfer.h/c` - 程序传输管理模块
- 更新 `dcm.c` - 添加 0x34-0x37 路由

---

### T005: 实现 0x2F Input Output Control By Identifier
**状态**: 待执行  
**优先级**: 中  
**估计时长**: 1天

**需求**:
- 实现 ISO 14229-1 第10.7节
- 支持 controlOptionRecord:
  - returnControlToECU (0x00)
  - resetToDefault (0x01)
  - freezeCurrentState (0x02)
  - shortTermAdjustment (0x03)
- 实现 controlEnableMaskRecord 处理
- 支持 controlStatusRecord 响应

**输出物**:
- `dcm_io_control.h/c` - I/O 控制模块
- 更新 `dcm.c` - 添加 0x2F 路由

---

## 里程碑 M2: 附加功能

### T006: 实现 0x85 Control DTC Setting
**状态**: 待执行  
**优先级**: 中  
**估计时长**: 0.5天

**需求**:
- 实现 ISO 14229-1 第11.2节
- 支持 DTCSettingType:
  - on (0x01) - 启用 DTC 设置
  - off (0x02) - 禁用 DTC 设置
- 与 DEM 集成 - 控制 DTC 更新

**输出物**:
- 更新 `dcm_dem_integration.c/h` - 添加 DTC 控制
- 更新 `dcm.c` - 添加 0x85 路由

---

### T007: 实现 0x23/0x24/0x2A/0x86 低优先级服务
**状态**: 待执行  
**优先级**: 低  
**估计时长**: 2天

**服务列表**:
- 0x23 Read Memory By Address (扩展存储读取)
- 0x24 Read Scaling Data By Identifier (比例数据读取)
- 0x2A Read Data By Periodic Identifier (周期数据读取)
- 0x86 Response On Event (事件响应)

**输出物**:
- `dcm_memory.c` 扩展 - 添加 0x23
- `dcm_periodic.h/c` - 周期数据服务
- `dcm_response_on_event.h/c` - 事件响应服务

---

## 里程碑 M3: DoCAN 实现

### T008: 实现 DoCAN 核心 (ISO 15765-2)
**状态**: 待执行  
**优先级**: 高  
**估计时长**: 2天

**需求**:
- 实现 ISO 15765-2:2016 CAN 上诊断
- 支持单帧 (SF) 消息
- 支持多帧 (FF/CF/FC) 消息传输
- 实现流量控制 (BlockSize, STmin)
- 支持标准 CAN (11-bit) 和扩展 CAN (29-bit)
- 支持 CAN FD

**输出物**:
- `docan_types.h` - DoCAN 类型定义
- `docan_core.h/c` - 核心实现

---

### T009: 实现 DoCAN-DCM 集成
**状态**: 待执行  
**优先级**: 高  
**估计时长**: 1天

**需求**:
- 实现 DoCAN 到 DCM 的消息路由
- 实现 DCM 到 DoCAN 的响应发送
- 支持物理请求地址映射
- 实现诊断会话管理

**输出物**:
- `docan_dcm_integration.h/c` - DoCAN-DCM 集成层

---

## 里程碑 M4: 传输层优化

### T010: 优化 DoIP-DCM 集成层
**状态**: 待执行  
**优先级**: 中  
**估计时长**: 1天

**需求**:
- 完善 DoIP 到 DCM 的消息路由
- 优化大数据包处理 (>4096 bytes)
- 实现并发连接支持
- 添加 DoIP 诊断消息缓冲

**输出物**:
- 更新 `doip_core.c` - 优化诊断消息处理
- 新增 `doip_dcm_bridge.h/c` - DoIP-DCM 桥接层

---

### T011: 实现统一传输层抽象
**状态**: 待执行  
**优先级**: 低  
**估计时长**: 1天

**需求**:
- 实现统一的传输层接口
- 支持 DoIP/DoCAN/IsoTp 动态切换
- 提供传输层状态查询

**输出物**:
- `dcm_transport.h/c` - 传输层抽象模块

---

## 里程碑 M5: 验证与测试

### T012: UDS 服务单元测试
**状态**: 待执行  
**优先级**: 高  
**估计时长**: 2天

**需求**:
- 为每个新增 UDS 服务创建单元测试
- 测试正常流程
- 测试错误处理 (NRC)
- 测试边界条件

**输出物**:
- `tests/diagnostics/test_dcm_did.c` - DID 服务测试
- `tests/diagnostics/test_dcm_transfer.c` - 传输服务测试
- `tests/diagnostics/test_dcm_io_control.c` - I/O 控制测试

---

### T013: DoCAN 测试
**状态**: 待执行  
**优先级**: 高  
**估计时长**: 1天

**需求**:
- 测试单帧传输
- 测试多帧传输 (SF/FF/CF/FC)
- 测试流量控制
- 测试超时处理

**输出物**:
- `tests/transport/test_docan_core.c` - DoCAN 核心测试

---

### T014: 系统集成测试
**状态**: 待执行  
**优先级**: 中  
**估计时长**: 1天

**需求**:
- 测试完整诊断流程 (DoIP/DoCAN)
- 测试多协议同时连接
- 测试负载场景

**输出物**:
- `tests/system/test_diagnostic_stack.c` - 诊断栈系统测试

---

### T015: 文档和合规验证
**状态**: 待执行  
**优先级**: 中  
**估计时长**: 1天

**需求**:
- 更新 DCM 设计文档
- 验证 AUTOSAR 合规性
- 验证 ISO 标准合规性
- 生成测试覆盖率报告

**输出物**:
- 更新 `docs/DCM_DESIGN.md`
- `COMPLIANCE_REPORT.md` - 合规验证报告
- `覆盖率报告`

---

## 任务汇总

| 阶段 | 任务数 | 预估时长 | 状态 |
|------|--------|----------|------|
| M1 核心服务 | 5 | 5.5天 | 待开始 |
| M2 附加功能 | 2 | 2.5天 | 待开始 |
| M3 DoCAN | 2 | 3天 | 待开始 |
| M4 传输层优化 | 2 | 2天 | 待开始 |
| M5 验证测试 | 4 | 5天 | 待开始 |
| **总计** | **15** | **18天** | - |

---

## 依赖关系

```
T001/T002/T003
    ↓
T004/T005/T006/T007
    ↓
T008/T009 (DoCAN)
    ↓
T010/T011 (传输层优化)
    ↓
T012/T013/T014/T015 (测试验证)
```

---

*最后更新: 2026-04-29*  
*OpenSpec Change: uds-complete-optimization*
