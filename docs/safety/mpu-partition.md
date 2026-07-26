# yuleASR MPU 分区方案

> **文档**: MPU 分区方案 (Memory Protection Unit Partition Plan)
> **版本**: 1.0 | **日期**: 2026-07-26
> **作者**: 小马 🐴 (质量架构师)
> **状态**: 方案设计
> **平台**: S32K312 (ARM Cortex-M4F) — MPU v7 架构
> **目标版本**: v2.0

---

## 1 概述

### 1.1 目的

本文档定义 yuleASR 在 S32K312 Cortex-M4F 上启用 MPU 的分区方案。MPU 提供硬件内存隔离，以解决 DFA 分析中识别的 D-FFI-001 和 D-FFI-002 (全局数据段/BSS 段无硬件隔离) 偏差，实现 ASIL B(B) 与 QM(B) 之间的 freedom from interference。

### 1.2 MPU 能力概览 (Cortex-M4F)

| 能力 | 说明 |
|------|------|
| MPU 区域数 | 8 个区域 |
| 区域最小大小 | 32 字节 (16 字节对齐要求) |
| 区域重叠 | 允许 (高编号区域覆盖低编号区域) |
| 访问权限 | 无访问 / 仅特权 / 完全访问 + 可执行控制 |
| Subregion | 每个区域可分 8 个 subregion, 独立使能/禁能 |
| 背景区域 | 默认 (未匹配区域) = 无访问权限 |
| 异常处理 | MPU 违规 → MemManage Fault (可配置的硬/可入栈异常) |
| L1 Cache | S32K312 无 L1 Cache (无需考虑 cache MPU 问题) |

### 1.3 设计原则

1. **最小特权**: 每个区域仅授予所需的最小访问权限
2. **ASIL B 优先**: ASIL B 模块内存区域受严格保护, QM 模块不可意外写入
3. **OS 特权隔离**: 内核态 (OS) 与用户态 (BSW 模块) 通过 MPU 隔离
4. **兼容 AUTOSAR OS**: 与 AUTOSAR SC3/SC4 保护方案对齐

---

## 2 内存映射与分区

### 2.1 物理内存布局 (S32K312)

```
起始地址      结束地址      大小       描述
0x0000_0000  0x0007_FFFF  512 KB     Flash (程序代码 + 配置常量)
0x1000_0000  0x1000_3FFF  16 KB      数据 Flash (仿真 EEPROM)
0x1FFF_8000  0x1FFF_FFFF  32 KB      SRAM (系统 RAM)
0x2000_0000  0x2000_7FFF  32 KB      SRAM_H (高地址 SRAM, 总计 64 KB)
0x4000_0000  0x400F_FFFF  1 MB−     外设映射区
...
```

> S32K312 总 SRAM = 64 KB (SRAM_L 32 KB + SRAM_H 32 KB)。实际项目可扩展到 128 KB 版本。

### 2.2 MPU 区域分配 (8 区域)

| 区域 # | 起始 | 大小 | 内容 | 权限 | 说明 |
|:------:|------|:----:|------|:----:|------|
| 0 | 0x0000_0000 | 512K | Flash: 代码 (全部) | 特权: RWX<br>用户: RX | 所有模块代码为只读执行 |
| 1 | 0x2000_0000 | 48K | RAM: 全局变量 + BSS (全部) | 特权: RW<br>用户: RW | ⚠️ 过渡期: v2.0 逐步细分 |
| 2 | 0x2000_C000 | 11K | OS 内核数据 (任务控制块, 调度表) | 特权: RW<br>用户: 无 | OS 内核区域, 用户代码不可访问 |
| 3 | 0x2000_C000+11K | 5K | OS 栈区域 | 特权: RW<br>用户: 当前任务栈 RW | 任务切换时动态更新 |
| 4 | 0x4000_0000 | 512K | 外设: 定时器, WDG, CAN, SPI | 特权: RW<br>用户: 无 | 外设区域默认仅 OS 访问 |
| 5 | — | — | **未分配** | — | 保留 |
| 6 | — | — | **未分配** | — | 保留 |
| 7 | — | — | **未分配** | — | 保留 |

### 2.3 目标分区方案 (v2.0 最终)

| 区域 # | 起始 | 大小 | 内容 | ASIL B 保护 |
|:------:|------|:----:|------|:----------:|
| 0 | 0x0000_0000 | 512K | Flash 代码 (全局只读) | ✅ 所有代码不可篡改 |
| 1 | 0x2000_0000 | 16K | **ASIL B 数据区** (E2E/WdgM/NvM/CryIf/SecOC/RamSafety) | ✅ QM 代码不可写 |
| 2 | 0x2000_4000 | 32K | QM 数据区 (Com/Dcm/PduR/Can/MCAL/...) | ⚠️ ASIL B 代码可读 (读兼容) |
| 3 | 0x2000_C000 | 12K | OS 内核 + 调度数据 | ✅ 用户态不可访问 |
| 4 | 0x2000_F000 | 4K | OS 栈 + 中断栈 | ✅ 仅特权态可写 |
| 5 | 0x4000_0000 | 512K | 关键外设 (WDG/GPT/NMI/HSM) | ✅ 仅 ASIL B 特权 |
| 6 | 0x4008_0000 | 512K | 非关键外设 (CAN/LIN/SPI/UART) | ✅ QM 代码特权访问 |
| 7 | — | — | 保留 | — |

---

## 3 ASIL B 数据区防护

### 3.1 被保护对象

区域 1 (0x2000_0000, 16K) 存放以下 ASIL B 模块的关键数据:

| 模块 | 关键变量 | 大小估算 | 说明 |
|------|---------|:--------:|------|
| **E2E** | E2E_PduConfigType, E2E_State | 2 KB | 配置表 + 运行状态 |
| **WdgM** | WdgM_Config, WdgM_SupervisedEntity | 2 KB | 监督实体配置 + 代数 |
| **NvM (校验层)** | NvM_BlockDescriptor, CRC 缓存 | 3 KB | 校验状态 + 块描述符 |
| **CryIf** | Crypto_KeyHandle, 回调队列 | 2 KB | 密钥句柄 + 操作状态 |
| **SecOC** | SecOC_AuthState, FreshnessValue | 2 KB | 认证状态 + 新鲜度值 |
| **RamSafety** | 测试结果映射, 故障记录 | 1 KB | 测试状态 |
| **OS Timing** | 时序监控计数器 | 1 KB | 执行/锁定/间隔时间 |
| **预留** | — | 3 KB | 扩展空间 |

### 3.2 防护策略

```
QM 代码尝试写入 ASIL B 数据区:
  ┌──────────────────────────────┐
  │ QM 模块指针越界写 0x2000_01xx │
  │      → MPU 违规触发          │
  │      → MemManage Fault       │
  │      → OS Fault Handler      │
  │      → 记录错误 (Det/Dem)    │
  │      → WdgM 复位              │
  └──────────────────────────────┘
```

**MPU 配置**: 区域 1 配置为「用户态: 只读 / 特权态: 读写」
- ASIL B 模块运行在特权态 → 可读写
- QM 模块运行在用户态 → 只读, 写操作触发 MemManage Fault

---

## 4 任务模式与 MPU 切换

### 4.1 任务分类

| 任务类型 | 运行模式 | MPU 配置 | 备注 |
|---------|---------|---------|------|
| ASIL B 任务 (E2E/WdgM/NvM 校验) | 特权态 (Privileged) | 区域 0-7 全部可访问 | 管理态, 可配置 MPU |
| ASIL B 中断 (NMI/HSM) | 处理模式 (Handler) | 区域 0-7 全部可访问 | 使用 MSP |
| QM 任务 (Com/Dcm/Can/Lin/...) | 用户态 (Unprivileged) | 区域 0-7 受限 (ASIL B 区只读) | 用户态, 不可改 MPU |
| QM 中断 (CAN/LIN/SPI) | 处理模式 | 区域 0-7 全部可访问 | 中断处理短暂恢复特权 |

### 4.2 MPU 动态更新

S32K312 MPU 配置在系统启动时一次完成 → **静态 MPU 方案** (不动态切换):

| 阶段 | MPU 状态 | 说明 |
|------|---------|------|
| 启动之初 | 禁用 | 启动代码位于特权态, 执行硬件/OS 初始化 |
| 完成启动→进入调度 | 启用 | OS 启动完成后加载 MPU 配置 |
| 运行时 | 静态 | 不切换 MPU 区域, 任务通过特权级/用户级隔离 |
| 异常处理 | 特权态 | 中断/异常时自动进入处理模式, 恢复全部权限 |

**为何静态:**
- 单核 64 KB RAM, 8 个 MPU 区域足够固定分配
- 动态 MPU 切换增加复杂性和 DFA 新路径
- AUTOSAR SC3 的静态分区符合要求

---

## 5 启动与配置

### 5.1 MPU 初始化序列

```c
/* 伪代码: yuleASR MPU 初始化 */
void YuleMpu_Init(void)
{
    /* 0. 启动时 MPU 禁用 */
    MPU->CTRL = 0;
    /* 1. 清除所有区域 */
    for (int i = 0; i < 8; i++) {
        MPU->RBAR[i] = 0;
        MPU->RASR[i] = 0;
    }
    /* 2. 配置区域 0: Flash (全地址, 全权限, 允许可执行) */
    MPU->RBAR[0] = 0x00000000 | MPU_REGION_VALID | (0 << 0);
    MPU->RASR[0] = MPU_AP_PRIV_RW_USER_RO   /* 特权读写, 用户只读 */
                 | MPU_EXEC_ALLOW
                 | MPU_REGION_SIZE(19)        /* 512K: 2^19 */
                 | MPU_REGION_ENABLE;
    /* 3. 配置区域 1: ASIL B 数据区 (16K) */
    MPU->RBAR[1] = 0x20000000 | MPU_REGION_VALID | (1 << 0);
    MPU->RASR[1] = MPU_AP_PRIV_RW_USER_RO   /* 特权读写, 用户只读 */
                 | MPU_EXEC_NEVER             /* 数据区不可执行 */
                 | MPU_REGION_SIZE(14)        /* 16K: 2^14 */
                 | MPU_REGION_ENABLE;
    /* 4. 配置区域 2: QM 数据区 (32K) */
    MPU->RBAR[2] = 0x20004000 | MPU_REGION_VALID | (2 << 0);
    MPU->RASR[2] = MPU_AP_PRIV_RW_USER_RW   /* 全访问 */
                 | MPU_EXEC_NEVER
                 | MPU_REGION_SIZE(15)        /* 32K: 2^15 */
                 | MPU_REGION_ENABLE;
    /* ... 区域 3-6 配置省略 ... */
    /* 5. 使能 MPU */
    MPU->CTRL = MPU_CTRL_ENABLE | MPU_CTRL_PRIVDEFENA;
    /* 6. DSB + ISB 同步 */
    __DSB();
    __ISB();
}
```

### 5.2 OS 集成

AUTOSAR OS 需支持 `OS_MPU_PROTECTION` 特性:

| OS 配置 | 值 | 说明 |
|---------|:--:|------|
| OS_MPU_PROTECTION | TRUE | 启用 MPU 保护 |
| OS_PRIVILEGED_TASKS | E2E, WdgM, NvM, CryIf, SecOC | ASIL B 任务 |
| OS_UNPRIVILEGED_TASKS | 所有 QM 任务 | Com, Dcm, PduR, Can, Lin, ... |
| OS_MPU_STATIC | TRUE | 静态 MPU 配置 |

---

## 6 安全相关考虑

### 6.1 MPU 对 FFI 的贡献

| DFA 偏差 | MPU 作用 | 可关闭? |
|----------|---------|:--------:|
| D-FFI-001 (全局数据段) | MPU 区域 1 保护 ASIL B 数据区, QM 用户态不可写 | ✅ 可关闭 |
| D-FFI-002 (BSS 段) | ASIL B BSS 段在区域 1, QM BSS 在区域 2, 硬件隔离 | ✅ 可关闭 |
| MEM-10/11 内存耦合 | 完全解决 | ✅ |
| 栈溢出保护 | OS 栈区域限制(区域 3+4) | ✅ 辅助 |
| 外设保护 | 关键外设仅特权可写(区域 5) | ✅ |

### 6.2 MPU 自身安全

| 故障模式 | 影响 | 检测 |
|----------|------|------|
| MPU 配置寄存器被篡改 | 保护失效 | OS 完整性校验 (启动时) |
| MPU 区域重叠配置错误 | 权限泄漏 | MPU 配置校验 + 启动自检 |
| MemManage Fault 未处理 | 系统行为未知 | OS Fault Handler → WdgM 复位 |
| MPU 禁用后未重启用 | 保护消失 | OS 心跳 + WdgM 监控 |

### 6.3 测试建议

| 测试项 | 方法 | 目标 |
|--------|------|------|
| MPU 区域边界测试 | QM 任务尝试写 ASIL B 区域 | 触发 MemManage Fault |
| MPU 写保护验证 | ASIL B 任务写自身区域 | 正常写入 |
| MPU 配置完整性 | 异常后检查 MPU 寄存器 | 配置未篡改 |
| 外设访问保护 | QM 任务访问 WDG 寄存器 | 触发 MemManage Fault |

---

## 7 实施路线图

| 版本 | 里程碑 | 内容 |
|:----:|--------|------|
| v1.4.0 | 链接脚本分区 | `yuleasr.ld` 内存区域划分, 静态分区 (无 MPU) |
| v1.5.0 | MPU 基础支持 | MPU 初始化, 区域 0-4 配置, 启动自检 |
| v1.6.0 | OS 集成 | AUTOSAR OS 特权/用户态任务分离 |
| v2.0.0 | 完整 MPU | 8 区域全配置, 外设区保护, D-FFI-001/002 偏差关闭 |

---

## 8 参考

| 文档 | 路径 |
|------|------|
| DFA 分析 | docs/safety/dfa-analysis.md |
| 安全架构 | docs/safety/safety-architecture.md |
| S32K312 RM | NXP S32K312 Reference Manual (Chapter 37: MPU) |
| ARMv7-M ARM | ARMv7-M Architecture Reference Manual (B3: MPU) |
| AUTOSAR SWS_OS | R21-11, §7.3 Protection |
