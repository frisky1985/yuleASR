# yuleASR QEMU Full-Stack Verification

在 QEMU mps2-an521 (Cortex-M33) 上对 yuleASR BSW 完整栈进行端到端验证。

## 目录结构

| 目录 | 验证内容 | 优先级 |
|------|---------|--------|
| `common/` | 共用断言基础设施（semihosting + Unity UART 移植） | P0 |
| `p0_assert_infra/` | 断言基础设施验证 | P0 |
| `p1a_os_schedule/` | OS 调度启动（FreeRTOS SysTick + 任务切换） | P1 |
| `p1b_ecum_startup/` | EcuM 三段式启动序列 | P1 |
| `p2a_can_loopback/` | CAN 软件回环 → CanIf → Com → RTE → ASW | P2 |
| `p2b_nvm_persist/` | NvM 掉电恢复（flash.bin 持久化 → reset → 读回） | P2 |
| `p2c_uds_inject/` | UDS 请求注入 → Dcm 响应断言 | P2 |
| `p3a_ram_ecc/` | RAM ECC 错误注入与恢复 | P3 |
| `p3b_wdg_timeout/` | WdgM checkpoint miss → 复位 | P3 |
| `p3c_secoc_loopback/` | SecOC → Csm → mbedTLS MAC 回环 | P3 |
| `p3d_irq_driven/` | 中断驱动数据流验证 | P3 |
| `ci/` | CI 驱动脚本 | — |

## 快速开始

```bash
# 运行单个测试
cd p0_assert_infra && ./build.sh run

# 批量运行（供 CI）
cd ci && ./run_all_qemu_tests.sh
```

## QEMU 命令

```
qemu-system-arm -machine mps2-an521 -cpu cortex-m33 \
  -kernel <elf> -nographic -serial stdio \
  --semihosting-config enable=on,target=native
```

## 复用资产

- `tests/qemu_m33/src/Uart_Cfg.c/h` — CMSDK UART 驱动
- `tests/qemu_m33/src/startup_m33.s` — Cortex-M33 启动代码
- `tests/qemu_m33/qemu_m33.ld` — 链接脚本
- `tests/qemu_m33/FreeRTOSConfig.h` — FreeRTOS 配置
- `tests/unit/framework/unity.c/h` — Unity 测试框架
