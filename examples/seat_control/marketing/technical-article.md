# yuleASR 实战：S32K312 座椅模块从零到编译通过

## 背景

在汽车电子开发中，AutoSAR 平台的门槛一直很高——复杂的配置工具链、高昂的商业授权、陡峭的学习曲线。yuleASR 作为开源 AutoSAR BSW 平台，旨在降低这个门槛。本文以 NXP S32K312 六向电动座椅控制为例，展示如何从零开始构建一个完整的 AutoSAR 应用。

## 架构设计（AUTOSAR 分层）

项目严格遵循 AutoSAR 四层架构：

```
┌──────────────────────────────────────────┐
│   SWC (SeatControl, SeatPosition, ...)   │  ← 应用层
├──────────────────────────────────────────┤
│   RTE (运行时环境)                        │  ← 运行时抽象
├──────────────────────────────────────────┤
│   BSW (Dio, Pwm, Adc, Can, Lin, Fls)    │  ← 基础软件
├──────────────────────────────────────────┤
│   MCAL (S32K312 寄存器层)                │  ← 微控制器抽象
└──────────────────────────────────────────┘
```

**应用层 (ASW/SWC)**：SeatControl 主状态机、SeatPosition PID 闭环控制、SeatHeating PWM 加热管理、SeatCommunication LIN/CAN 通信、SeatMemory 位置记忆。

**运行时环境 (RTE)**：通过 MainFunction 机制实现 10ms 周期调度，各模块间通过 API 调用解耦。

**基础软件 (BSW)**：DIO 负责按键扫描与 LED 指示，ADC 读取位置传感器，PWM 控制电机与加热。Demo 阶段使用 BSW Stub 替代真实 MCAL。

**微控制器抽象 (MCAL)**：面向 S32K312 (Cortex-M7 @ 80MHz)，配置锁步模式满足 ASIL-B 安全需求。

## 关键特性

- **6 向电动调节**：水平 ±230mm、靠背 0-60°、升降 50mm、倾角 15°，四轴独立 PID 闭环控制
- **座椅加热**：3 档 PWM 控制（关/40%/80%），10 分钟超时自动关闭
- **位置记忆**：2 组 Flash NVM 存储，Magic+Checksum 校验保护
- **通信协议**：LIN 接收开关命令 (ID=0x01)，CAN 广播座椅状态 (ID=0x501)
- **故障诊断**：限位保护、堵转检测、过流保护、跛行模式

## 构建方法

```cmake
# CMakeLists.txt 核心片段
cmake_minimum_required(VERSION 3.16)
project(SeatControl_Demo VERSION 1.0.0 LANGUAGES C ASM)

add_executable(seat_control
    src/main.c src/SeatControl.c src/SeatPosition.c
    src/SeatHeating.c src/SeatCommunication.c src/SeatMemory.c
    bsw_stubs/bsw_stubs.c startup/startup_S32K312.S
)

target_include_directories(seat_control PRIVATE
    include config bsw_stubs)
target_compile_options(seat_control PRIVATE
    -mcpu=cortex-m7 -mthumb -std=c99 -Wall -Werror)
```

初始化序列：

```c
int main(void) {
    // BSW 9 阶段初始化
    Mcu_Init(&Mcu_Config);   Mcu_DistributePllClock();  // 时钟/PLL
    Port_Init(&Port_Config);                              // 引脚复用
    Gpt_Init(&Gpt_Config);                                // 定时器
    Dio_Init(&Dio_Config);                                // GPIO
    Pwm_Init(&Pwm_Config);                                // PWM
    Adc_Init(&Adc_Config);                                // ADC
    Can_Init(&Can_Config);    Lin_Init(&Lin_Config);      // 通信
    Fls_Init(&Fls_Config);                                // Flash
    SeatControl_Init();                                   // 应用

    while (1) { SeatControl_MainFunction(); Delay(~10ms); }
}
```

## 总结

yuleASR 通过清晰的分层架构和 BSW Stub 机制，让开发者可以在无硬件的情况下快速验证架构设计。本 Demo 从创建工程到编译通过仅需数分钟，完整展示了 AutoSAR 应用从状态机设计到多模块集成的完整路径。配合 x86 单元测试框架，可在开发早期发现逻辑缺陷，大幅缩短嵌入式软件开发周期。

---

*上海毓特电子科技有限公司 — yuleASR 开源 AutoSAR 平台*
