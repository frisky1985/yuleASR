# yuleASR QEMU 仿真环境搭建指南

## 概述

本文档说明如何为 yuleASR AUTOSAR BSW (S32K312 Cortex-M7) 搭建 QEMU 仿真回落环境。

### 为什么需要 QEMU 仿真？

- **无板开发**: 在没有 S32K312 EVB 真板的情况下进行开发和自测
- **CI/CD 集成**: 自动化流水线无需硬件依赖即可编译验证
- **快速迭代**: 编译-运行-调试周期只需数秒，无需烧写
- **早期问题发现**: 在硬件到位前发现架构和逻辑问题

### 局限性

QEMU 仿真**无法**覆盖：
- 真实外设行为（CAN/LIN/SPI 等时序相关）
- S32K312 特有硬件模块（HSE/HSM/SAK/SEC 等）
- 实时性/中断延迟测量
- 电源管理/低功耗行为

## 技术方案

| 项目 | 说明 |
|------|------|
| **QEMU 板级** | `mps2-an500` (ARM MPS2 with AN500 FPGA for Cortex-M7) |
| **CPU** | Cortex-M7 (与 S32K312 同架构) |
| **Flash** | 0x00000000 - 0x003FFFFF (4 MB) |
| **SRAM** | 0x20000000 - 0x203FFFFF (4 MB) |
| **UART** | CMSDK APB UART at 0x40004000 |
| **交叉编译器** | arm-none-eabi-gcc |
| **启动方式** | 直接加载 ELF (-kernel) |

## 前置条件

### 1. 安装 QEMU

```bash
# macOS (Homebrew)
brew install qemu

# Ubuntu/Debian
sudo apt install qemu-system-arm

# Fedora
sudo dnf install qemu-system-arm

# 验证
qemu-system-arm --version
# 预期输出: QEMU emulator version 11.x.x
```

### 2. 安装 ARM 交叉编译器

```bash
# macOS (Homebrew)
brew install arm-none-eabi-gcc

# Ubuntu/Debian
sudo apt install gcc-arm-none-eabi

# Fedora
sudo dnf install arm-none-eabi-gcc-cs

# 验证
arm-none-eabi-gcc --version
# 预期输出: arm-none-eabi-gcc (GCC) 13.x.x
```

### 3. 验证工具链

```bash
# 检查工具
which arm-none-eabi-gcc
which qemu-system-arm
which cmake
which make
```

## 快速开始

### 使用 Makefile (推荐，最快路径)

```bash
# 进入 QEMU 目录
cd yuleASR/qemu/

# 编译
make

# 运行
make run

# 预期输出:
# ========================================
#  yuleASR BSW -- QEMU Simulation (MPS2 AN500)
#  Target: ARM Cortex-M7 (simulating S32K312)
# ========================================
# --- MCAL Module Checks ---
# [MCU] Mcu_Init: OK (QEMU stub)
# [MCU] Mcu_InitClock: OK (simulated)
# ...
# [PASS] All QEMU tests PASSED
```

### 使用 CMake (集成到主构建)

```bash
# 从 yuleASR 根目录
cmake -B build-qemu \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake \
  -DQEMU_BUILD=ON \
  -S .

cmake --build build-qemu

# 运行
./build-qemu/yuleasr_qemu
# 或在 QEMU 中运行
cmake --build build-qemu --target qemu-run
```

### 直接使用命令行

```bash
qemu-system-arm \
  -M mps2-an500 \
  -cpu cortex-m7 \
  -nographic \
  -kernel yuleASR/qemu/build/yuleasr_qemu.elf
```

## 目录结构

```
yuleASR/qemu/
├── Makefile              # QEMU 构建系统（Make）
├── CMakeLists.txt        # QEMU 构建系统（CMake 集成）
├── link.ld               # QEMU 链接脚本（Flash/SRAM 映射）
├── startup.S             # QEMU 启动代码（Reset_Handler）
├── main_qemu.c           # QEMU 测试应用程序 + MCAL 存根
└── build/                # 构建输出目录
    ├── yuleasr_qemu.elf  # ELF 可执行文件
    ├── yuleasr_qemu.bin  # 裸二进制文件
    ├── yuleasr_qemu.map  # 链接映射
    ├── *.o               # 目标文件
```

## Makefile 目标

| 目标 | 说明 |
|------|------|
| `make` / `make all` | 编译生成 ELF + BIN |
| `make run` | 编译 + 在 QEMU 中运行 |
| `make debug` | 编译 + QEMU GDB 服务器 (端口 1234) |
| `make size` | 显示各段大小 |
| `make disasm` | 反汇编前 100 条指令 |
| `make clean` | 清理构建产物 |

## GDB 调试

### 方法一: QEMU + GDB (本地)

终端 1:
```bash
cd yuleASR/qemu
make debug
# 输出: QEMU with GDB server on port 1234
```

终端 2:
```bash
arm-none-eabi-gdb build/yuleasr_qemu.elf \
  -ex "target remote :1234" \
  -ex "load" \
  -ex "break main" \
  -ex "continue"
```

### 方法二: VS Code launch.json

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "QEMU Debug (Cortex-M7)",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/qemu/build/yuleasr_qemu.elf",
            "cwd": "${workspaceFolder}/qemu",
            "MIMode": "gdb",
            "miDebuggerPath": "arm-none-eabi-gdb",
            "miDebuggerServerAddress": "localhost:1234",
            "setupCommands": [
                { "text": "target extended-remote :1234" },
                { "text": "load" },
                { "text": "monitor reset" }
            ]
        }
    ]
}
```

## CI/CD 集成 (GitHub Actions)

```yaml
# .github/workflows/qemu-test.yml
name: QEMU Simulation Test

on: [push, pull_request]

jobs:
  qemu-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install dependencies
        run: |
          sudo apt update
          sudo apt install -y gcc-arm-none-eabi qemu-system-arm
      - name: Build for QEMU
        run: |
          cd qemu
          make
      - name: Run in QEMU
        run: |
          cd qemu
          timeout 10 qemu-system-arm \
            -M mps2-an500 -cpu cortex-m7 -nographic \
            -kernel build/yuleasr_qemu.elf 2>&1 | \
            grep -q "All QEMU tests PASSED"
```

## 常见问题

### QEMU 无输出

确认 UART 地址正确。MPS2 AN500 使用 **CMSDK APB UART**（非 PL011），地址为 `0x40004000`。

### 链接失败: undefined reference to `_sbrk`

确保 `link.ld` 中有 `PROVIDE(__HeapBase = __heap_start__)` 和 `PROVIDE(__HeapLimit = __heap_end__)`。

### QEMU 显示 "Lockup: can't escalate 3 to HardFault"

原因通常是中断向量表损坏或 SP 未正确初始化。确认：
1. 链接脚本 `.vector_table` 节在 FLASH 起始位置
2. `_estack` 符号正确指向 SRAM 顶部
3. 启动代码中 MSP 初始化正确

### CMSDK APB UART "invalid baudrate" 警告

此为 QEMU 内部警告，不影响功能。可安全忽略。如需消除，设置 BAUDDIV 为期望波特率值即可。

## 扩展: 添加更多 MCAL 存根

当需要测试更多 BSW 模块时，在 `main_qemu.c` 中添加对应存根：

```c
/* SPI 存根示例 */
Std_ReturnType Spi_Init(const Spi_ConfigType *ConfigPtr) {
    (void)ConfigPtr;
    uart_puts("[SPI] Spi_Init: OK (QEMU stub)\r\n");
    return E_OK;
}

/* 添加到 main() 中 */
Spi_Init(&SpiConfig);
```

---

> **下一步**: 真板就绪后，参考 `docs/board-bringup-checklist.md` 验证硬件功能。
