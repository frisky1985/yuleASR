# 系统级集成测试场景文档

## 文档信息
- **项目名称**: yuleASR Classic AUTOSAR BSW
- **版本**: R22-11
- **文档版本**: 1.0.0
- **日期**: 2026-04-29

---

## 1. 概述

本文档描述了yuleASR项目的系统级集成测试场景，覆盖模块间协同工作的验证。

### 测试范围
- 存储链路集成 (NvM → Fee → Fls → Fls_Hw)
- 看门狗监督链 (Wdgm → WdgIf → Wdg → Wdg_Hw)
- 安全通信链 (SecOC → Csm → Crypto)
- BSW模块交互 (EcuM, BswM, SchM)
- 错误处理链 (Det → Dem)

---

## 2. 存储链路集成测试

### 2.1 存储栈初始化
**测试ID**: STORAGE_INIT_001
**目标**: 验证存储栈从硬件到服务层的正确初始化

**测试步骤**:
1. 初始化Fls驱动 (MCAL层)
2. 初始化Fee模块 (ECUAL层)
3. 初始化NvM模块 (服务层)
4. 验证各模块状态为IDLE

**预期结果**:
- Fls状态: FLS_IDLE
- Fee状态: FEE_IDLE
- NvM状态: NVM_IDLE
- 无错误报告

### 2.2 写读循环测试
**测试ID**: STORAGE_RW_001
**目标**: 验证完整的写入-读取数据流

**测试步骤**:
1. 准备64字节测试数据
2. 调用NvM_WriteBlock写入数据
3. 等待写入完成回调
4. 调用NvM_ReadBlock读取数据
5. 等待读取完成回调
6. 验证读取数据与写入数据一致

**预期结果**:
- 写入操作返回E_OK
- 读取操作返回E_OK
- CRC校验通过
- 数据一致性验证通过

### 2.3 掉电恢复测试
**测试ID**: STORAGE_PWR_001
**目标**: 验证异常掉电后的数据恢复能力

**测试步骤**:
1. 写入关键数据到Block
2. 模拟写入过程中的掉电 (故障注入)
3. 重新初始化存储栈
4. 尝试读取Block数据
5. 验证冗余拷贝或默认数据

**预期结果**:
- 系统能够识别数据损坏
- 恢复机制激活
- 数据完整性可验证

### 2.4 写入重试测试
**测试ID**: STORAGE_RETRY_001
**目标**: 验证写入失败时的自动重试机制

**测试步骤**:
1. 配置最大重试次数为3
2. 注入前2次写入失败故障
3. 第3次写入成功
4. 验证写入最终成功

**预期结果**:
- 前2次写入返回E_NOT_OK
- 第3次写入返回E_OK
- 最终数据正确写入

### 2.5 垃圾回收测试
**测试ID**: STORAGE_GC_001
**目标**: 验证垃圾回收机制触发和运行

**测试步骤**:
1. 填充存储空间至阈值以下
2. 继续写入新数据
3. 观察GC触发
4. 验证GC后空间可用

**预期结果**:
- GC在低空间时自动触发
- GC过程中状态为BUSY
- GC后可用空间增加

---

## 3. 看门狗监督链测试

### 3.1 看门狗链初始化
**测试ID**: WDG_INIT_001
**目标**: 验证看门狗链从硬件到服务层的初始化

**测试步骤**:
1. 初始化Wdg驱动 (MCAL)
2. 初始化WdgIf (ECUAL)
3. 初始化Wdgm (服务层)
4. 验证各层状态

**预期结果**:
- Wdg初始化成功
- WdgIf状态为IDLE
- Wdgm全局状态为OK

### 3.2 正常喂狗测试
**测试ID**: WDG_FEED_001
**目标**: 验证正常运行时的喂狗流程

**测试步骤**:
1. 配置看门狗超时时间为100ms
2. 每50ms调用WdgIf_Trigger
3. 检查Wdgm检查点报告
4. 运行10个周期

**预期结果**:
- 所有触发返回E_OK
- Wdgm本地状态保持OK
- 无超时复位

### 3.3 超时检测测试
**测试ID**: WDG_TIMEOUT_001
**目标**: 验证看门狗超时检测机制

**测试步骤**:
1. 配置快速模式 (10ms超时)
2. 故意停止喂狗
3. 监控Wdgm状态变化
4. 验证超时检测

**预期结果**:
- Wdgm状态变为FAILED
- 触发错误报告
- 可选执行复位

### 3.4 复位恢复测试
**测试ID**: WDG_RESET_001
**目标**: 验证看门狗复位后的系统恢复

**测试步骤**:
1. 正常运行并触发看门狗复位
2. 系统重新启动
3. 重新初始化看门狗链
4. 验证恢复正常运行

**预期结果**:
- 复位后系统可启动
- 看门狗链重新初始化成功
- 状态恢复为OK

### 3.5 检查点监督测试
**测试ID**: WDG_CHECK_001
**目标**: 验证检查点方式的alive监督

**测试步骤**:
1. 配置监督实体和检查点
2. 按预期顺序报告检查点
3. 运行Wdgm_MainFunction
4. 验证监督状态

**预期结果**:
- 检查点正确记录
- Alive计数器更新
- 监督状态为OK

---

## 4. 安全通信链测试

### 4.1 安全栈初始化
**测试ID**: SEC_INIT_001
**目标**: 验证安全模块初始化链

**测试步骤**:
1. 初始化Csm模块
2. 配置密钥
3. 初始化SecOC
4. 验证状态

**预期结果**:
- Csm初始化成功
- 密钥有效
- SecOC状态为可操作

### 4.2 MAC生成验证测试
**测试ID**: SEC_MAC_001
**目标**: 验证MAC生成和验证流程

**测试步骤**:
1. 准备测试数据
2. 调用Csm_MacGenerate生成MAC
3. 发送数据和MAC
4. 接收方调用Csm_MacVerify验证

**预期结果**:
- MAC生成成功
- MAC验证返回OK
- 数据完整性确认

### 4.3 PDU认证测试
**测试ID**: SEC_PDU_001
**目标**: 验证带认证的PDU传输

**测试步骤**:
1. 构建PDU数据
2. 添加 freshness值
3. 计算认证MAC
4. 组装完整PDU
5. 接收方验证并提取数据

**预期结果**:
- PDU组装成功
- 认证验证通过
- 数据正确提取

### 4.4 MAC验证失败测试
**测试ID**: SEC_FAIL_001
**目标**: 验证篡改检测能力

**测试步骤**:
1. 生成原始MAC
2. 修改PDU数据
3. 使用原始MAC验证
4. 检测验证失败

**预期结果**:
- 篡改被检测
- 验证返回NOT_OK
- 错误报告生成

### 4.5 Freshness同步测试
**测试ID**: SEC_FRESH_001
**目标**: 验证Freshness值同步

**测试步骤**:
1. 初始化同步计数器
2. 发送方递增Freshness
3. 接收方验证Freshness窗口
4. 测试重同步场景

**预期结果**:
- Freshness正确递增
- 窗口验证通过
- 重同步成功

---

## 5. BSW模块交互测试

### 5.1 EcuM-Wdgm集成
**测试ID**: BSW_ECUM_001
**目标**: 验证EcuM对Wdgm的初始化和协调

**测试步骤**:
1. EcuM_Init启动初始化
2. EcuM_StartupTwo执行第二阶段
3. 验证Wdgm初始化完成
4. 检查全局状态

**预期结果**:
- EcuM进入RUN状态
- Wdgm初始化成功
- 监督链正常运行

### 5.2 BswM-Mem集成
**测试ID**: BSW_BSWM_001
**目标**: 验证BswM与内存管理的协调

**测试步骤**:
1. BswM初始化
2. 存储栈初始化
3. BswM请求影响内存的模式
4. 验证内存操作协调

**预期结果**:
- BswM状态正常
- 存储栈可操作
- 模式切换无冲突

### 5.3 SchM调度测试
**测试ID**: BSW_SCHM_001
**目标**: 验证SchM对各模块主函数的调度

**测试步骤**:
1. 配置调度周期
2. 启动调度器
3. 验证各模块MainFunction被调用
4. 检查执行时序

**预期结果**:
- 各MainFunction按周期调用
- 无调度冲突
- 系统响应正常

### 5.4 模式切换测试
**测试ID**: BSW_MODE_001
**目标**: 验证模式切换对看门狗的影响

**测试步骤**:
1. BswM初始化
2. Wdgm初始化在SLOW模式
3. BswM请求RUN模式
4. Wdgm切换到FAST模式
5. 验证模式切换成功

**预期结果**:
- 模式请求被接受
- Wdgm模式变更
- 新监督参数生效

### 5.5 关闭序列测试
**测试ID**: BSW_SHUTDOWN_001
**目标**: 验证协调的关机流程

**测试步骤**:
1. 系统运行在RUN模式
2. EcuM_SelectShutdownTarget设置目标
3. BswM_Deinit反初始化
4. EcuM_Shutdown执行关机

**预期结果**:
- 关机目标设置成功
- BswM正确反初始化
- 系统正常关机

---

## 6. 错误处理链测试

### 6.1 Det错误报告
**测试ID**: ERR_DET_001
**目标**: 验证错误报告到Det模块

**测试步骤**:
1. Det初始化
2. 模块触发错误条件
3. 验证Det_ReportError被调用
4. 检查错误记录

**预期结果**:
- 错误被正确报告
- 模块ID正确
- 错误ID准确

### 6.2 错误传播链
**测试ID**: ERR_PROP_001
**目标**: 验证错误从模块传播到Det/Dem

**测试步骤**:
1. 配置错误注入
2. 触发模块错误
3. 监控Det报告
4. 监控Dem记录 (如可用)

**预期结果**:
- 错误传播到Det
- 错误传播到Dem
- 错误信息完整

### 6.3 多错误源测试
**测试ID**: ERR_MULTI_001
**目标**: 验证多模块并发错误处理

**测试步骤**:
1. 同时触发多个模块错误
2. 监控错误队列
3. 验证错误记录顺序
4. 检查系统稳定性

**预期结果**:
- 所有错误被记录
- 无错误丢失
- 系统保持可运行

### 6.4 错误恢复测试
**测试ID**: ERR_RECOV_001
**目标**: 验证错误恢复机制

**测试步骤**:
1. 触发可恢复错误
2. 执行恢复流程
3. 重新初始化模块
4. 验证恢复正常

**预期结果**:
- 恢复流程执行
- 模块重新初始化成功
- 系统恢复正常

### 6.5 Dem错误记录
**测试ID**: ERR_DEM_001
**目标**: 验证Dem错误记录 (如可用)

**测试步骤**:
1. Dem初始化
2. 触发诊断事件
3. 调用Dem报告接口
4. 验证事件记录

**预期结果**:
- 事件正确记录
- DTC正确设置
- 冻结帧保存

---

## 7. 故障注入机制

### 7.1 支持的故障类型

| 故障代码 | 名称 | 描述 |
|---------|------|------|
| FAULT_NONE | 无故障 | 正常操作模式 |
| FAULT_FLASH_WRITE_FAIL | Flash写入失败 | 模拟Flash写入错误 |
| FAULT_FLASH_READ_FAIL | Flash读取失败 | 模拟Flash读取错误 |
| FAULT_FLASH_ERASE_FAIL | Flash擦除失败 | 模拟Flash擦除错误 |
| FAULT_WDG_TIMEOUT | 看门狗超时 | 模拟喂狗超时 |
| FAULT_CRYPTO_FAIL | 加密操作失败 | 模拟加密错误 |
| FAULT_NVM_CRC_FAIL | NVMCRC错误 | 模拟CRC校验失败 |
| FAULT_COMM_TIMEOUT | 通信超时 | 模拟通信超时 |

### 7.2 故障注入API

```c
/* 设置故障 */
void FaultInjection_Set(FaultInjectionType fault);

/* 清除故障 */
void FaultInjection_Clear(void);

/* 检查故障是否激活 */
boolean FaultInjection_IsActive(FaultInjectionType fault);
```

### 7.3 使用示例

```c
/* 注入Flash写入失败 */
FaultInjection_Set(FAULT_FLASH_WRITE_FAIL);

/* 执行测试 */
result = Fee_Write(blockId, data);

/* 清除故障 */
FaultInjection_Clear();
```

---

## 8. 测试执行环境

### 8.1 硬件要求
- x86_64 Linux主机 (用于仿真)
- 或目标硬件平台

### 8.2 软件依赖
- GCC编译器
- 测试框架 (自定义Unity-like框架)
- 模块mock/stub

### 8.3 编译命令
```bash
cd /home/admin/yuleASR/tests/integration
gcc -I../../include -I../../src/bsw/*/include \
    system_integration_test.c \
    ../../src/bsw/*/src/*.c \
    ../mocks/*.c \
    -o system_integration_test
```

### 8.4 运行命令
```bash
./system_integration_test
```

---

## 9. 测试结果分析

### 9.1 通过标准
- 所有测试用例通过
- 无内存泄漏
- 代码覆盖率 > 80%

### 9.2 结果格式
```
===============================================================
  Test Results:
    Total:   25
    Passed:  25
    Failed:  0
    Skipped: 0

  Test Statistics:
    Write Operations: 10
    Read Operations:  10
    Watchdog Triggers: 50
    Crypto Operations: 8
    Errors Reported:   5
===============================================================
  ALL TESTS PASSED!
```

### 9.3 覆盖率报告
- 函数覆盖率: > 90%
- 语句覆盖率: > 85%
- 分支覆盖率: > 80%

---

## 10. 附录

### 10.1 相关文档
- AutoSAR_SWS_NVRAMManager.pdf
- AutoSAR_SWS_WatchdogManager.pdf
- AutoSAR_SWS_SecureOnboardCommunication.pdf
- AutoSAR_SWS_CryptoServicesManager.pdf
- AutoSAR_SWS_ECUM.pdf
- AutoSAR_SWS_BswM.pdf

### 10.2 修订历史

| 版本 | 日期 | 修改内容 | 作者 |
|------|------|---------|------|
| 1.0.0 | 2026-04-29 | 初始版本 | System Test Team |

---

**版权所有 (c) 2024-2026 上海予乐电子科技有限公司**
