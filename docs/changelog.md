# 版本历史

## 概述

本文档记录 YuleTech AutoSAR BSW Platform 的版本变更历史。

---

## [1.0.0] - 2026-04-15

### 概述

首个正式发布版本，提供完整的 AutoSAR BSW 基础软件栈实现。

### 新增功能

#### MCAL 层 (9 个模块)

- **Mcu** - 微控制器驱动
  - 初始化、时钟配置、复位控制
  - 低功耗模式管理
  
- **Port** - 端口驱动
  - 引脚配置和方向控制
  
- **Dio** - 数字 I/O 驱动
  - 数字输入/输出控制
  
- **Can** - CAN 驱动
  - CAN 控制器初始化
  - 消息发送/接收
  - 中断处理
  
- **Spi** - SPI 驱动
  - SPI 通信接口
  
- **Gpt** - 通用定时器驱动
  - 定时器功能
  
- **Pwm** - PWM 驱动
  - PWM 信号生成
  
- **Adc** - ADC 驱动
  - 模数转换功能
  
- **Wdg** - 看门狗驱动
  - 看门狗监控

#### ECUAL 层 (9 个模块)

- **CanIf** - CAN 接口
  - CAN 硬件抽象
  - PDU 路由
  - 总线关闭处理
  
- **IoHwAb** - I/O 硬件抽象
  - 信号抽象和处理
  
- **CanTp** - CAN 传输协议
  - ISO-TP 协议支持
  
- **EthIf** - 以太网接口
  - 以太网通信
  
- **MemIf** - 存储器接口
  - 统一存储器访问
  
- **Fee** - Flash EEPROM 仿真
  - Flash 模拟 EEPROM
  
- **Ea** - EEPROM 抽象
  - EEPROM 访问抽象
  
- **FrIf** - FlexRay 接口
  - FlexRay 通信
  
- **LinIf** - LIN 接口
  - LIN 通信

#### Service 层 (5 个模块)

- **Com** - 通信服务
  - 基于信号的通信
  - 信号组管理
  - IPDU 传输控制
  
- **PduR** - PDU 路由器
  - 路由表管理
  - 向上/向下路由
  - FIFO 队列管理
  
- **NvM** - NVRAM 管理器
  - 块管理
  - CRC 校验
  - 作业队列
  
- **Dcm** - 诊断通信管理器
  - UDS 服务支持
  - 会话管理
  - 安全访问控制
  
- **Dem** - 诊断事件管理器
  - 事件状态管理
  - DTC 管理
  - 老化机制

#### RTE 层 (1 个模块)

- **Rte** - 运行时环境
  - 组件通信接口
  - 数据类型定义
  - 任务调度器
  - COM/NVM 接口

### 文档

- 项目 README
- 架构设计文档
- API 参考文档
- 开发指南
- 模块清单

### 验证

- PduR 验证报告
- NvM 验证报告
- RTE 验证报告

### 开发工具

- OpenSpec 规范框架
- Superpowers 工作流
- Harness Engineering 约束

### 已知问题

无

### 兼容性

- AutoSAR Classic Platform 4.4
- NXP i.MX8M Mini
- GCC ARM Embedded 10.3.1+

---

## 版本编号规则

本项目遵循 [语义化版本](https://semver.org/lang/zh-CN/) 规范：

```
版本格式：主版本号.次版本号.修订号

主版本号：不兼容的 API 修改
次版本号：向下兼容的功能新增
修订号：向下兼容的问题修正
```

### 示例

- `1.0.0` - 初始发布
- `1.1.0` - 新增功能
- `1.1.1` - 问题修复
- `2.0.0` - 不兼容变更

---

## 发布计划

### 短期计划 (v1.1.0)

- [ ] 添加更多 MCAL 驱动
- [ ] 完善 RTE 代码生成
- [ ] 增加单元测试覆盖率
- [ ] 优化性能

### 中期计划 (v1.2.0)

- [ ] 支持更多硬件平台
- [ ] 添加 SOME/IP 支持
- [ ] 完善诊断功能
- [ ] 增加安全功能

### 长期计划 (v2.0.0)

- [ ] 支持 AutoSAR Adaptive
- [ ] 云端配置工具
- [ ] AI 辅助开发
- [ ] 完整工具链

---

## 贡献者

感谢以下贡献者：

- YuleTech AutoSAR Team
- 开源社区贡献者

---

**文档版本**: 1.0.0
**最后更新**: 2026-04-15
**作者**: YuleTech AutoSAR Team
