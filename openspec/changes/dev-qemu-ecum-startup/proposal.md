# 变更提案：QEMU EcuM 启动序列验证

> **变更 ID**: dev-qemu-ecum-startup  
> **状态**: Proposed  
> **优先级**: P1  
> **负责人**: Track-A  
> **创建日期**: 2026-08-22  
> **依赖变更**: dev-qemu-assert-infra, dev-qemu-os-schedule  
> **目标版本**: v1.6.0-alpha.1  
> **估计工时**: 32h (4d)

## 背景

当前 QEMU 验证直接调用 `Mcu_Init()` / `Port_Init()` 等单独的 MCAL 桩函数，没有走 EcuM 统一入口。在真实 ECU 中，BSW 初始化由 `EcuM_Init()` → `EcuM_StartupTwo()` → `EcuM_StartupThree()` 三阶段驱动，BswM 在 StartupThree 后接收 RUN 请求并切换到 RUN 状态。需要验证此启动序列在 QEMU 上正确执行。

## 目标

1. 在 QEMU 上用真实 `EcuM.c` + `BswM.c` 驱动三阶段启动
2. 验证 MCAL 初始化先于 ECUAL 初始化（AUTOSAR 分层约束）
3. 验证 BswM 在 StartupThree 后进入 `BSWM_STATE_RUN`
4. 验证 `EcuM_RequestShutdown()` 触发逆序 deinit

## 范围

### 包含内容
- `tests/qemu_full_stack/p1b_ecum_startup/main_ecum_startup.c` — 验证入口
- `tests/qemu_full_stack/p1b_ecum_startup/ecum_test_stubs.c` — MCAL/ECUAL 桩（带 UART 打点）
- `tests/qemu_full_stack/p1b_ecum_startup/build.sh` — 构建脚本

### 不包含内容
- MCAL/ECUAL 真实代码（使用带 UART 打点的桩）
- CAN/Com/NvM 等服务层真实代码（C4-C5 负责）

## 验收标准

- [ ] S3.1: UART 顺序输出 `PHASE1_DONE PHASE2_DONE PHASE3_DONE`
- [ ] S3.2: `BswM_GetState()` == `BSWM_STATE_RUN`
- [ ] S3.3: `MCAL_INIT_DONE` 先于 `CANIF_INIT_DONE`
- [ ] S3.4: `SERVICE_DEINIT ECUAL_DEINIT MCAL_DEINIT` 逆序正确
- [ ] CI `run_qemu_test.sh` exit code == 0

## 风险评估

| 风险 | 概率 | 影响 | 缓解措施 |
|:-----|:-----|:-----|:---------|
| EcuM.c 依赖太多 BSW 模块导致链接失败 | 高 | 高 | 使用带 UART 打点的桩替代未验证模块 |
| BswM 状态机需要 ComM 等上游模块配合 | 中 | 中 | 提供 ComM stub 返回 RUN 请求 |
