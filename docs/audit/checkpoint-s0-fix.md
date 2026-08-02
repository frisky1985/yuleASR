# Checkpoint: S0 修复启动（2026-08-01）

## Session Info
- Branch: v1.3.0 (874f18e)
- Task: S0 致命问题修复（能链接、能启动）

## 全量检查结论（已完成）
- 5 项 S0 致命：ASW 链接失败 / FreeRTOS 内核缺失 / OS 任务入口 8 缺 / 调度链三重断裂 / RTE 数据通路死
- 已交叉验证 FreeRTOS 确实不在仓库（stub 头 17-67 行、1059 个 .a 无内核符号、git 历史为 CI 修复创建）

## 已确认事实
1. FreeRTOS-Kernel V11.1.0 已下载到 /tmp/FreeRTOS-Kernel（含 Posix port + ARM_CM33）
2. src/bsw/os/include/ 下有 stub 头（FreeRTOS.h 39 行、task.h 67 行等）需替换为官方头
3. Os.c 调用 20+ 个标准 FreeRTOS API（xTaskCreate/xEventGroupSetBits/xSemaphoreTake/xTimerCreate 等），与官方 API 兼容
4. Os_AlarmConfigs 配置表**有** Callback（OsAlarm_BswM_MainFunction_Callback 等），但 Os_InitAlarms 运行时覆盖为 NULL_PTR
5. Os_Cfg.c extern 声明 8 个 OsTask_*_Entry 无定义
6. Rte_ConnectPort/Rte_Start **有定义**（Rte.c:292/331）但零调用者
7. EcuM.c 中 4 处 Rte_Start() 被注释
8. Rte.h 声明 34 个 Rte_Read_SWC_* / Rte_Write_SWC_* 无实现
9. Rte_AswScheduler_Start 有定义无调用者
10. native 构建已有 libOs.a（stub 版 48.9K）；ARM GCC 已装 (/opt/homebrew/bin/arm-none-eabi-gcc)
11. SetRelAlarm 实现正常但不设置 alarm->Callback（调用方需确保）

## Sprint Contract
- 文件: docs/audit/sprint-contract-s0-fix.md
- 5 项 In Scope，8 条验收标准 C1-C8，Evaluator 可客观验证

## 下一步（Generator）
1. 复制 FreeRTOS 内核到 third_party/freertos/
2. 删除 src/bsw/os/include/ 下 stub 头，保留 Os.h/Os_Cfg.h 等自有头
3. 重写 FreeRTOSConfig.h（完整配置）
4. 修 Os_InitAlarms（Callback 从配置表复制）
5. 实现 8 个 OsTask_*_Entry
6. 实现 Rte_Read_SWC_* / Rte_Write_SWC_*（Rte.c）
7. 恢复 EcuM Rte_Start() + 调度链调用
8. native 构建验证 → ARM 交叉验证
