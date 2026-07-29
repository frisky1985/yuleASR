# S32K312 EVB Board Bring-Up Checklist

## 硬件信息

| 项 | 值 |
|------|-------|
| **MCU** | NXP S32K312 (Cortex-M7 @ 120 MHz) |
| **Flash** | 2 MB |
| **SRAM** | 512 KB |
| **EVB** | S32K312-EVB 或 S32K3X4EVB-Q257 |
| **调试器** | 板载 OpenSDA (S32K312-EVB) 或 JLink |
| **调试接口** | JTAG/SWD (10-pin Cortex Debug) |

## 硬件采购

### S32K312 EVB 购买链接

- **NXP 官方 S32K312-EVB**: https://www.nxp.com/part/S32K312-EVB
  - 官方评估板，含板载 OpenSDA 调试器
  - 供应商: DigiKey, Mouser, Arrow
  
- **NXP S32K3X4EVB-Q257**: https://www.nxp.com/part/S32K3X4EVB-Q257
  - S32K344 评估板，兼容 S32K312 (同封装)
  - 适合需要更多外设验证的场景

- **Mini Board / 核心板 (第三方)**:
  - 淘宝/闲鱼搜索 "S32K312 核心板" 或 "S32K314 开发板"
  - 确认板载调试器类型 (OpenSDA / JLink / DAPLink)

### 必需附件

| 附件 | 说明 | 推荐型号 |
|------|------|----------|
| USB 线 | Type-C 数据线 (用于供电 + 调试) | 任意高质量数据线 |
| JLink (可选) | 外部调试器，如果板载调试器不可靠 | SEGGER JLink EDU / JLink Plus |
| USB-UART 模块 | 串口调试 (如果板载无 USB-UART) | CP2102 / CH340G |
| 杜邦线 | 跳线连接 | 母对母 10cm |

## 验证流程

### 阶段 1: 上电检查

- [ ] **1.1** EVB 上电，检查电源指示灯 (PWR LED)
- [ ] **1.2** 测量 VDD 电压 (3.3V ±5%)
- [ ] **1.3** 测量 VCORE 电压 (1.25V ±5%)
- [ ] **1.4** 检查复位按键功能 (按下后 RESET LED 亮起)
- [ ] **1.5** 检查晶振起振 (用示波器观察 EXTAL 引脚 40 MHz)

### 阶段 2: 调试器连接

#### OpenSDA (板载, S32K312-EVB)

- [ ] **2.1** USB 连接至 PC，OpenSDA 枚举为 USB 存储设备
- [ ] **2.2** 安装或更新 OpenSDA 固件 (下载自 NXP OpenSDA 页面)
- [ ] **2.3** 切换调试模式至 JLink 或 CMSIS-DAP
- [ ] **2.4** 设备管理器/系统报告识别调试接口

#### JLink (外部)

- [ ] **2.5** JLink 连接 EVB SWD 接口 (3.3V, SWDIO, SWCLK, GND)
- [ ] **2.6** JLink 驱动安装完成 (SEGGER JLink 驱动)
- [ ] **2.7** JLinkExe 能识别 S32K312:

```bash
JLinkExe -device S32K312 -if SWD -speed 4000
# 预期输出: "Cortex-M7 identified"
```

#### OpenOCD

- [ ] **2.8** OpenOCD 能连接目标:

```bash
openocd -f scripts/openocd_s32k312.cfg
# 预期输出: "target state: halted"
```

### 阶段 3: 烧写测试

- [ ] **3.1** 编译最小 blinky 程序 (GPIO 翻转)
- [ ] **3.2** 生成二进制文件 (blinky.hex / blinky.bin)
- [ ] **3.3** OpenOCD 烧写:

```bash
openocd -f scripts/openocd_s32k312.cfg \
  -c "program build/blinky.hex 0x00400000 verify reset exit"
```

- [ ] **3.4** JLink 烧写:

```bash
JLinkExe -device S32K312 -if SWD -speed 4000 -autoconnect 1 \
  -CommanderScript scripts/flash.jlink
```

- [ ] **3.5** 验证 LED 按预期闪烁 (GPIO 输出)
- [ ] **3.6** 烧写后排针/RAM 无异常发热

### 阶段 4: 串口通信

- [ ] **4.1** 识别串口设备 (USB-UART 或板载虚拟串口)

```bash
# macOS
ls /dev/cu.usbserial-* 或 ls /dev/cu.usbmodem*
# Linux
dmesg | grep ttyUSB
# Windows
设备管理器 → 端口 (COM 和 LPT)
```

- [ ] **4.2** 配置串口终端:

| 参数 | 值 |
|------|------|
| Baud rate | 115200 |
| Data bits | 8 |
| Parity | None |
| Stop bits | 1 |
| Flow control | None |

- [ ] **4.3** 打开串口并发送回车 (检查回显/响应)
- [ ] **4.4** 运行串口回环测试 (TX-RX 短接)
- [ ] **4.5** 运行 UART 打印程序: 收到 "Hello from S32K312" 输出

### 阶段 5: 逐模块验证 Core MCAL

#### 5.1 MCU 模块

- [ ] MCU 初始化成功
- [ ] 时钟配置为 120 MHz
- [ ] 系统时钟切换正常
- [ ] 复位原因读取正确
- [ ] 低功耗模式可进入/退出 (SLEEP / DEEP SLEEP)

#### 5.2 PORT 模块

- [ ] 所有 Debug LED 引脚初始化为 GPIO 输出
- [ ] UART TX/RX 引脚配置为 ALT 功能
- [ ] 按键引脚配置为 GPIO 输入 (内部上拉)
- [ ] Pin 方向切换正常

#### 5.3 DIO 模块

- [ ] 写 LED 引脚: LED 亮/灭
- [ ] 读按键状态: 按下/释放正确识别
- [ ] 多个通道同时翻转正常

#### 5.4 GPT 模块 (可选)

- [ ] 定时器初始化成功
- [ ] 定时中断产生 (1ms 周期)
- [ ] 定时器停止/启动功能正常

#### 5.5 WDG 模块 (可选)

- [ ] WDG 初始化成功
- [ ] WDG 喂狗正常 (不触发复位)
- [ ] WDG 超时触发复位

### 阶段 6: BSW Services 验证

- [ ] **6.1** Det (Default Error Tracer): 错误上报正确
- [ ] **6.2** EcuM (ECU Manager): 启动/关闭序列正常
- [ ] **6.3** BswM (BSW Mode Manager): 模式切换正常
- [ ] **6.4** SchM (BSW Scheduler): 任务按周期执行
- [ ] **6.5** NvM (NVRAM Manager): 读写/校验正常

### 阶段 7: 通信协议

- [ ] **7.1** CAN: 回环测试 (Loopback mode)
- [ ] **7.2** CAN: 外部通信测试 (接 CAN 分析仪)
- [ ] **7.3** LIN (如有): 主/从模式通信
- [ ] **7.4** UDS (DCM): 诊断会话切换
- [ ] **7.5** UDS: 读 DID / 写 DID

### 阶段 8: 压力测试

- [ ] **8.1** 长时间运行 (48h): 无死机/复位
- [ ] **8.2** 频繁复位 (100 次): 启动正常
- [ ] **8.3** 极温测试 (如有条件): -40°C ~ 125°C 正常运行
- [ ] **8.4** 内存完整性: SRAM R/W 循环测试通过
- [ ] **8.5** Flash 擦写耐久: 写入/擦除 100 次无损坏

## 调试常用命令

### OpenOCD 连接

```bash
openocd -f scripts/openocd_s32k312.cfg
```

### JLink 连接

```bash
JLinkExe -device S32K312 -if SWD -speed 4000 -autoconnect 1
# 在 JLink 命令行中:
#   halt
#   loadfile build/yuleasr.hex
#   reset
#   go
```

### 串口监视器

```bash
# macOS / Linux
screen /dev/cu.usbserial-XXXX 115200
# 或
picocom -b 115200 /dev/cu.usbserial-XXXX

# Linux
minicom -D /dev/ttyUSB0 -b 115200

# Windows (PowerShell)
# 使用 PuTTY 或 Serial Monitor
```

## 故障排查

| 问题 | 可能原因 | 解决 |
|-------|-----------|--------|
| 调试器无法连接 | SWD 引脚连接错误 | 检查 3.3V, SWDIO, SWCLK, GND, RESET |
| 烧写后无反应 | Vector table 不在起始地址 | 检查链接脚本 |
| 串口无输出 | UART 引脚配置错误 | 检查 PORT 初始化 |
| WDG 循环复位 | 未正确喂狗 | 确认调度器中调用 Wdg_SetTriggerCondition |
| 高频运行不稳定 | 供电不足 / 时钟配置错误 | 检查 VCORE 和时钟分频 |

## QEMU 与真板工作流

```
开发阶段       →  QEMU 仿真 (无硬件依赖)
              ↓
模块验证       →  真板 + 逐模块 checklist
              ↓
集成测试       →  真板 + CAN/LIN 环境
              ↓
系统测试       →  真板 + 完整系统
              ↓
生产测试       →  真板 + 可靠性/压力测试
```

## 附录: 引脚映射 (S32K312-EVB)

| 功能 | 引脚 | EVB 连接 |
|------|------|----------|
| LED0 (蓝) | PTD15 | RGB LED 蓝色 |
| LED1 (绿) | PTD16 | RGB LED 绿色 |
| LED2 (红) | PTD0  | RGB LED 红色 |
| UART0_TX | PTC7 | 板载 OpenSDA 虚拟串口 |
| UART0_RX | PTC6 | 同上 |
| CAN0_TX | PTE5 | CAN 收发器 |
| CAN0_RX | PTE4 | CAN 收发器 |
| SWDIO | PTA3 | 调试接口 |
| SWCLK | PTA2 | 调试接口 |

> **注意**: 以上引脚映射基于 NXP S32K312-EVB 参考设计。请以具体 EVB 原理图为准。

---

**验证人签名**: \_\_\_\_\_\_\_\_\_\_\_\_ \**日期**: \_\_\_\_\_\_\_\_\_\_\_\_
