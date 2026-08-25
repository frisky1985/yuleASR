# 实现任务清单

## 里程碑 M1: Eth 实现 (1周)

### T1.1 Eth 头文件完善 (4小时)
- [x] 补全 Eth.h API声明
- [x] 定义所有必需的数据类型
- [x] 添加安全相关的检查宏
- [x] 创建 Eth_Lcfg.h

### T1.2 Eth 核心实现 (20小时)
- [x] Eth.c 框架搭建
- [x] 初始化逻辑 (Eth_Init, Eth_ControllerInit)
- [x] 模式管理 (Set/GetControllerMode)
- [x] PHY接口 (WriteMII, ReadMII)
- [x] 发送功能 (ProvideTxBuffer, Transmit)
- [x] 接收功能 (Receive)
- [x] 中断处理 (Eth_Irq.c)
- [x] Eth_MainFunction 周期调度函数

### T1.3 Eth 单元测试 (8小时)
- [x] 初始化测试
- [x] 发送流程测试
- [x] 接收流程测试
- [x] 错误处理测试
- [x] 覆盖率达到 80%

**M1 交付物:** 可工作的 Eth 驱动 + 测试报告 ✅

---

## 里程碑 M2: Icu 实现 (5天)

### T2.1 Icu 头文件创建 (4小时)
- [x] 创建 Icu.h
- [x] 创建 Icu_Cfg.h
- [x] 创建 Icu_Lcfg.h
- [x] 创建 Icu_Private.h

### T2.2 Icu 核心实现 (20小时)
- [x] 初始化/反初始化
- [x] 边沿检测功能
- [x] 信号测量功能
- [x] 时间戳功能
- [x] 边沿计数功能
- [x] 中断处理

### T2.3 Icu 单元测试 (6小时)
- [x] 各测量模式测试
- [x] 通知回调测试
- [x] 错误处理测试

**M2 交付物:** 可工作的 Icu 驱动 + 测试报告 ✅

---

## 里程碑 M3: FrTp 实现 (1周)

### T3.1 FrTp 头文件创建 (4小时)
- [x] 创建 FrTp.h, FrTp_Cfg.h
- [x] 定义 PDU类型和状态机
- [x] 创建 FrTp_Private.h

### T3.2 FrTp 核心实现 (24小时)
- [x] 初始化逻辑
- [x] 发送状态机 (FrTp_TxSm.c)
- [x] 发送功能 (FrTp_Tx.c)
- [x] 接收功能 (FrTp_Rx.c)
- [x] 流量控制逻辑
- [x] 主函数 (FrTp_MainFunction)

### T3.3 FrTp 单元测试 (8小时)
- [x] 单帧传输测试
- [x] 多帧传输测试
- [x] 流量控制测试
- [x] 超时处理测试

**M3 交付物:** 可工作的 FrTp 模块 + 测试报告 ✅

---

## 里程碑 M4: Ocu 实现 (3天, 可选)

### T4.1 Ocu 头文件创建 (3小时)
- [x] 创建 Ocu.h, Ocu_Cfg.h, Ocu_Lcfg.h

### T4.2 Ocu 核心实现 (12小时)
- [x] 初始化/反初始化
- [x] 比较功能
- [x] PWM功能
- [x] 中断处理

### T4.3 Ocu 单元测试 (4小时)
- [x] 功能测试
- [x] 波形测试

**M4 交付物:** Ocu 驱动 + 测试报告 ✅

---

## 总体进度

| 里程碑 | 估计工时 | 状态 |
|-------|---------|------|
| M1: Eth | 32小时 | ✅ 完成 |
| M2: Icu | 30小时 | ✅ 完成 |
| M3: FrTp | 36小时 | ✅ 完成 |
| M4: Ocu | 19小时 | ✅ 完成 |
| **合计** | **117小时** | **全部完成** |

---

更新日期: 2026-08-25
