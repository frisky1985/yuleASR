# yuleASR — S32K312 座椅控制 Demo 架构概览

## AutoSAR 四层分层架构

yuleASR 座椅控制 Demo 严格遵循 AutoSAR 经典平台的四层金字塔结构，每一层提供明确的抽象边界：

```
                     ┌──────────────────────────────────────┐
                     │         应用层 (Application)          │
                     │                                      │
                     │  SeatControl  ─── 主状态机            │
                     │    ├── SeatPosition  ── PID 闭环      │
                     │    ├── SeatHeating   ── 加热控制       │
                     │    ├── SeatMemory    ── 位置记忆       │
                     │    └── SeatComm     ── LIN/CAN 通信    │
                     ├──────────────────────────────────────┤
                     │      运行时环境 (RTE)                  │
                     │                                      │
                     │  10ms MainFunction 周期调度             │
                     │  模块间 API 调用 + 数据一致性保护       │
                     ├──────────────────────────────────────┤
                     │     基础软件 (BSW)                    │
                     │                                      │
                     │  ┌─────┬─────┬─────┬─────┬─────┐    │
                     │  │ DIO │ PWM │ ADC │ GPT │ MCU │    │
                     │  ├─────┼─────┼─────┼─────┼─────┤    │
                     │  │ CAN │ LIN │ FL  │ PORT│ SPI │    │
                     │  └─────┴─────┴─────┴─────┴─────┘    │
                     ├──────────────────────────────────────┤
                     │  微控制器抽象 (MCAL)                  │
                     │                                      │
                     │  S32K312 (Cortex-M7 @ 80MHz)         │
                     │  Lockstep ON  │  ASIL-B safety        │
                     └──────────────────────────────────────┘
```

## 组件关系图

下面展示应用层组件之间的依赖关系和数据流：

```
┌──────────────────────────────────────────────────────────────────┐
│                        SeatControl (主状态机)                     │
│  ┌──────────────────────────────────────────────────────────────┐│
│  │  switch inputs ──► SeatControl_ReadSwitches()               ││
│  │                     ├─ SeatPosition_Jog*()                  ││
│  │                     ├─ SeatHeating_SetLevel()                ││
│  │                     └─ SeatMemory_Save()                     ││
│  │                                                            ││
│  │  state machine ──► SeatControl_StateMachine()               ││
│  │  fault check  ──► SeatControl_FaultCheck()                  ││
│  │                     └─ SeatPosition_IsLimitReached()         ││
│  └──────────────────────────────────────────────────────────────┘│
└──────────────────────────────────────────────────────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
        ▼                     ▼                     ▼
┌───────────────┐   ┌───────────────┐   ┌───────────────────┐
│ SeatPosition  │   │ SeatHeating   │   │ SeatCommunication  │
│ (PID 闭环)    │   │ (PWM 加热)    │   │ (LIN/CAN 通信)    │
│               │   │               │   │                   │
│ UpdateAdc()   │   │ SetLevel()    │   │ SendStatus()      │
│ PidUpdate()   │   │ MainFunction  │   │ ReceiveCommand()  │
│ SetMotor()    │   │ ─ timeout     │   │ ProcessCommand()  │
│ CheckLimits() │   │ ─ auto-off    │   │                   │
└───────┬───────┘   └───────┬───────┘   └────────┬──────────┘
        │                   │                     │
        ▼                   ▼                     ▼
   ┌────────┐         ┌────────┐           ┌──────────┐
   │ ADC    │         │ PWM    │           │ LIN/CAN  │
   │ GPT    │         │ DIO    │           │ MCU/GPT  │
   │ DIO    │         │ GPT    │           │          │
   └────────┘         └────────┘           └──────────┘
```

## ASCII 组件关系图

```
                    ┌─────────────────┐
                    │   SeatControl   │   ▲ 状态、错误码
                    │    (状态机)      │   │
                    └──┬──┬──┬──┬────┘   │
                       │  │  │  │        │
          ┌────────────┘  │  │  └──────────────┐
          ▼               ▼  ▼                 ▼
   ┌──────────┐   ┌──────────┐   ┌──────────────────┐
   │SeatPositn│   │SeatHeat  │   │SeatComm          │
   │ PID loop │   │PWM+Timer │   │LIN Rx / CAN Tx   │
   └─────┬────┘   └────┬─────┘   └────────┬─────────┘
         │              │                  │
    ┌────┴────┐    ┌────┴────┐       ┌────┴────┐
    │ADC+ PWM │    │PWM+DIO  │       │CAN+ LIN │
    │+DIO+MCU │    │+GPT     │       │+GPT     │
    └─────────┘    └─────────┘       └─────────┘
```

## BSW 模块依赖树

以下展示完整的 BSW 模块依赖关系，箭头表示"依赖于"关系：

```
Mcu (时钟/PLL/锁步)
├── Port (引脚复用/电气特性)
│   ├── Dio (数字 I/O)
│   │   ├── SeatControl_ReadSwitches (按键扫描)
│   │   ├── SeatPosition_SetMotorSpeed (方向控制)
│   │   ├── SeatHeating_SetLevel (LED 指示)
│   │   └── SeatControl_FaultCheck (限位检测)
│   ├── Pwm (PWM 输出)
│   │   ├── SeatPosition_SetMotorSpeed (电机速度)
│   │   └── SeatHeating_SetLevel (加热功率)
│   └── Adc (模拟采样)
│       └── SeatPosition_UpdateAdcReadings (位置反馈)
├── Gpt (定时器)
│   ├── 1ms   — 系统滴答
│   ├── 10ms  — 座椅控制周期
│   └── 100ms — 状态广播
├── Can (控制器局域网)
│   └── SeatComm_SendStatus (状态广播)
├── Lin (本地互联网络)
│   └── SeatComm_ReceiveCommand (开关命令接收)
└── Fls (Flash 存储器)
    └── SeatMemory_Save/Recall (位置记忆)
```

## 状态机说明

六向座椅的主状态机包含 6 个状态：

| 状态          | 说明               | 入口条件             | 出口条件                     |
|--------------|--------------------|---------------------|------------------------------|
| IDLE         | 空闲，等待命令       | 初始化完成 / 运动结束  | 任意命令                     |
| MOVING       | 电机运动            | 位置命令触发          | 到达目标 / 超时 / 故障       |
| HEATING      | 加热中              | 加热命令              | 超时 / 手动关闭              |
| MEMORY_RECALL| 记忆位调用          | 记忆键按下            | 调用完成 / 超时              |
| ERROR        | 故障，LED 闪烁       | 限位/堵转/过流        | ClearError 命令              |
| LIMP_HOME    | 跛行模式，仅加热可用  | 持续性故障            | 硬件复位                     |

## 电机闭环控制拓扑

```
                  +──── PID Controller ────+
Target ──►Error──►  P: Kp × error          │
                  │  I: Ki × ∫error dt     │──► Motor Speed (0-100%)
                  │  D: Kd × Δerror/dt     │    │
                  +────────────────────────+    │
                       ▲                       ▼
                       │                  ┌─────────┐
                       └── Position ◄─────┤ ADC     │
                           Feedback       │ Sensor  │
                                           └─────────┘
```

PID 参数（Q10 定点数）：

| 参数 | 值   | 物理意义       |
|------|------|----------------|
| Kp   | 512  | 比例增益 0.5    |
| Ki   | 32   | 积分增益 0.031  |
| Kd   | 128  | 微分增益 0.125  |
| Deadband | ±3mm | 停止死区    |

---

*上海毓特电子科技有限公司 — yuleASR 开源 AutoSAR 平台*
