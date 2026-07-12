# yuleASR — S32K312 座椅控制 Demo (Seat Control Demo)

## 项目概述

基于 yuleASR 开源 AutoSAR BSW 平台的 **6向电动座椅控制** 演示工程。  
目标芯片: NXP S32K312 (ARM Cortex-M7, 80MHz)

### 功能特性

| 功能 | 说明 |
|------|------|
| **6向调节** | 水平/靠背/升降/倾斜 — 4路电机闭环控制 |
| **座椅加热** | 3档 (关/低/高) — PWM 占空比控制 + 10分钟自动超时关闭 |
| **位置记忆** | 2组记忆位 — Flash NVM 存储 + 校验和验证 |
| **LIN 通信** | 主节点 @ 19200bps — 接收开关面板命令 |
| **CAN 通信** | 250kbps — 周期性广播座椅状态 (ID=0x501) |
| **故障诊断** | 过流 / 堵转 / 限位开关 / ADC 失效检测 |
| **安全机制** | 限位保护 + 电机软启动 + 跛行模式 |

### 硬件连接图

```
                    ┌──────────────────────┐
                    │     LIN 开关面板       │
                    │   (前/后/仰/升/降/加   │
                    │    热/记忆1/记忆2)     │
                    └──────┬───────────────┘
                           │ LIN (19200bps)
                    ┌──────┴───────────────┐
                    │                     │
    ┌───────────────┤   S32K312 MCU       ├───────────────┐
    │  CAN Bus      │   (Cortex-M7)       │   CAN Bus     │
    │  (250kbps)    │   80MHz             │   (250kbps)    │
    │               │   Lockstep ON       │                │
    └───────────────┤                     ├───────────────┘
                    │                     │
                    └──────┬───────────────┘
                           │
          ┌────────────────┼────────────────┐
          │                │                │
    ┌─────┴─────┐   ┌─────┴─────┐   ┌─────┴─────┐
    │ 电机驱动板  │   │ 加热元件   │   │ 传感器    │
    │ H-Bridge ×4 │   │ (PTC)     │   │ 限位开关×8 │
    │ + PWM      │   │           │   │ ADC×4     │
    └─────────────┘   └───────────┘   └───────────┘
```

## 目录结构

```
examples/seat_control/
├── CMakeLists.txt          # CMake 构建配置
├── README.md               # 本文件
├── startup_S32K312.S       # Cortex-M7 启动文件
├── config/
│   ├── Dio_Cfg.h           # DIO 通道定义 (开关/LED/继电器)
│   ├── Pwm_Cfg.h           # PWM 通道配置 (电机/加热)
│   ├── Adc_Cfg.h           # ADC 通道配置 (位置传感器)
│   ├── Gpt_Cfg.h           # GPT 定时器配置 (1ms/10ms/100ms)
│   ├── Port_Cfg.h          # PORT 引脚映射 (PTA-PTF)
│   ├── Mcu_Cfg.h           # MCU 时钟/锁步配置
│   ├── Can_Cfg.h           # CAN 通信配置 (250kbps)
│   ├── Lin_Cfg.h           # LIN 通信配置 (19200bps)
│   ├── Fls_Cfg.h           # Flash 存储器配置 (记忆存储)
│   └── Seat_Cfg.h          # 座椅参数配置 (行程/阈值/错误码)
├── include/
│   ├── SeatControl.h       # 主状态机接口
│   ├── SeatPosition.h      # 位置闭环控制接口
│   ├── SeatHeating.h       # 加热控制接口
│   ├── SeatCommunication.h # LIN/CAN 通信接口
│   └── SeatMemory.h        # 位置记忆接口
└── src/
    ├── main.c              # 入口 + BSW 9阶段初始化
    ├── SeatControl.c       # 主状态机实现
    ├── SeatPosition.c      # PID 闭环 + 电机控制
    ├── SeatHeating.c       # 加热 PWM + 超时控制
    ├── SeatCommunication.c # LIN/CAN 报文处理
    └── SeatMemory.c        # Flash 读写 + 校验
```

## 构建方法

### 前提条件

- **arm-none-eabi-gcc** >= 10.3 (已安装 @ v16.1.0)
- **CMake** >= 3.16
- **make** / **ninja**

### 构建命令

```bash
# 进入示例目录
cd examples/seat_control

# 创建构建目录
mkdir -p build && cd build

# 配置 CMake
cmake -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-arm-none-eabi.cmake \
      -DARM_GCC_BIN_PATH=/opt/homebrew/bin \
      -DTARGET_TRIPLE=arm-none-eabi \
      -DBUILD_EXAMPLES=ON ..

# 编译
make -j$(nproc)
```

### 预期产出

```
build/seat_control.elf        # ELF 可执行文件
build/seat_control.bin        # 原始二进制文件 (烧录用)
build/seat_control.hex        # Intel HEX 格式
build/seat_control.map        # 链接映射文件
build/seat_control.lst        # 反汇编列表
```

### 内存占用

| 区域     | 大小    | 说明                |
|----------|---------|---------------------|
| Flash    | ~64KB   | 代码 + 常量数据      |
| SRAM     | ~16KB   | 栈 + 堆 + 全局变量  |
| NVM      | 64KB    | 位置记忆存储 (可选)  |

## yuleASR 集成图示

```
yuleASR/
├── cmake/
│   └── toolchain-arm-none-eabi.cmake     # ARM GCC 工具链
├── src/
│   ├── bsw/
│   │   ├── mcal/                          # MCAL 驱动层
│   │   │   ├── dio/       → mcal_dio      # DIO
│   │   │   ├── pwm/       → mcal_pwm      # PWM
│   │   │   ├── adc/       → mcal_adc      # ADC
│   │   │   ├── gpt/       → mcal_gpt      # GPT
│   │   │   ├── port/      → mcal_port     # PORT
│   │   │   ├── mcu/       → mcal_mcu      # MCU
│   │   │   ├── can/       → mcal_can      # CAN
│   │   │   ├── lin/       → mcal_lin      # LIN
│   │   │   ├── fls/       → mcal_fls      # Flash
│   │   │   ├── spi/       → mcal_spi      # SPI
│   │   │   └── icu/       → mcal_icu      # ICU
│   │   ├── ecual/                          # ECU 抽象层
│   │   ├── services/                       # 服务层
│   │   └── boot/                           # Bootloader
│   ├── rte/                                # 运行时环境
│   ├── platform/
│   │   └── s32k312/
│   │       ├── include/                    # 平台头文件
│   │       ├── linker/
│   │       │   └── s32k312.ld              # 链接脚本
│   │       └── src/
│   └── asw/                                # 应用层软件
└── examples/
    └── seat_control/     ◄── 本 Demo     # 座椅控制示例
```

## BSW 初始化顺序

```
main()
├── Stage 1: Mcu_Init()        — 时钟/PLL/锁步
├── Stage 2: Port_Init()       — 引脚复用/DIO方向
├── Stage 3: Gpt_Init()        — 定时器通道
├── Stage 4: Dio_Init()        — 数字I/O电平
├── Stage 5: Pwm_Init()        — PWM输出通道
├── Stage 6: Adc_Init()        — ADC采样通道
├── Stage 7: Can_Init()        — CAN控制器
├── Stage 8: Lin_Init()        — LIN控制器
├── Stage 9: Fls_Init()        — Flash驱动
├── SeatControl_Init()         — 应用层初始化
└── while(1) ~10ms 循环
     ├── SeatControl_MainFunction()
     │   ├── ReadSwitchInputs()      — DIO 读取开关状态
     │   ├── StateMachine()          — 状态机跳转
     │   ├── SeatPosition_Process()  — PID 闭环控制
     │   ├── SeatHeating_Main()      — 加热 PWM + 超时
     │   ├── SeatComm_Main()         — LIN/CAN 通信
     │   └── FaultCheck()            — 故障诊断
     └── Delay(~10ms)
```

## 软件架构

### 状态机

```
         ┌──────────────────────────────────────┐
         │                                      │
    ┌────▼─────┐   MOVING    ┌──────────┐       │
    │  IDLE    ├─────────────►  MOVING  │       │
    │          │◄─────────────┤          │       │
    └────┬─────┘   completed └──────────┘       │
         │                                       │
    ┌────▼─────┐   HEATING   ┌──────────┐       │
    │  IDLE    ├─────────────► HEATING  │       │
    │          │◄─────────────┤  +timer  │       │
    └────┬─────┘   timeout   └──────────┘       │
         │                                       │
    ┌────▼─────┐  MEM_RECALL ┌──────────┐       │
    │  IDLE    ├─────────────►  MEMORY  │       │
    │          │◄─────────────┤  RECALL │       │
    └────┬─────┘   completed └──────────┘       │
         │                                       │
    ┌────▼─────┐  fault      ┌──────────┐       │
    │  ANY     ├─────────────►  ERROR   │       │
    │          │             │ blink LED│       │
    └──────────┘             └──────────┘       │
                                                  │
    ┌──────────┐  persist    ┌──────────┐       │
    │  ERROR   ├─────────────►  LIMP    │       │
    │          │             │  HOME    │       │
    └──────────┘             └──────────┘       │
                                                  │
    └──────────────────────────────────────┘
```

### 通信协议

#### LIN 命令帧 (ID=0x01, 8字节)

| Byte | 内容         | 说明                   |
|------|-------------|------------------------|
| 0    | Command     | 0x01=Move, 0x03=Heat   |
| 1    | Axis/Mode   | 0x00-0x03 (axis)       |
| 2-3  | Parameter   | Delta / level (LE)     |
| 4-7  | Reserved    | —                      |

#### CAN 状态帧 (ID=0x501, DLC=8)

| Byte | 内容         | 说明                   |
|------|-------------|------------------------|
| 0    | State       | 座椅状态枚举            |
| 1-2  | ErrorCode   | 故障码 (LE)            |
| 3    | Heater      | 加热档位                |
| 4-5  | HorzPos     | 水平位置 (BE)           |
| 6    | Reserved    | —                      |
| 7    | Checksum    | XOR of bytes 0-6       |

## 许可

本示例工程基于 yuleASR 项目，遵循 MIT 许可协议。

---

## Getting Started

```bash
# 克隆 yuleASR
git clone https://github.com/frisky1985/yuleASR.git
cd yuleASR/examples/seat_control

# 编译（ARM 交叉编译）
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-arm-none-eabi.cmake \
      -DARM_GCC_BIN_PATH=/opt/homebrew/bin \
      -DTARGET_TRIPLE=arm-none-eabi \
      -DCMAKE_EXE_LINKER_FLAGS="-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard -Wl,--gc-sections -nostartfiles -nodefaultlibs -lgcc" ..
make -j8

# 运行测试（x86_64 Mac 原生编译）
mkdir build_tests && cd build_tests
cmake -DBUILD_TESTS=ON .. && make && ctest
```

## Prerequisites

- **arm-none-eabi-gcc** >= 13.x
- **CMake** >= 3.16
- **make** / **ninja**

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![Tests](https://img.shields.io/badge/tests-31%20passed-brightgreen)]()
[![Quality](https://img.shields.io/badge/quality-6.56%2F10-yellow)]()
[![License](https://img.shields.io/badge/license-MIT-blue)]()

---

*上海毓特电子科技有限公司 — yuleASR 开源 AutoSAR 平台*
