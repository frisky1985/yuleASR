# Lin Slave 通用设计代码

基于普通串口(UART)实现的 LIN 2.2A Slave 驱动模块。

## 特性

- ✓ LIN 2.1/2.2A 协议兼容
- ✓ 报文头检测 (Break + Sync + PID)
- ✓ PID 保护位计算和验证
- ✓ 经典和增强校验和支持
- ✓ 事件触发回调机制
- ✓ 错误检测和处理
- ✓ 状态机实现

## 架构

```
┌─────────────────────────────────────────────────────────────────┐
│  应用层 (App Layer)                        │
│  - 数据处理回调                          │
│  - 报文发送接口                          │
├─────────────────────────────────────────────────────────────────┤
│  中间层 (Lin Core)                        │
│  - 状态机管理                            │
│  - PID/Checksum 处理                    │
│  - 报文编码解码                        │
├─────────────────────────────────────────────────────────────────┤
│  底层 (HAL)                              │
│  - UART 初始化/发送/接收                │
│  - 中断管理                            │
│  - Break 检测                        │
└─────────────────────────────────────────────────────────────────┘
```

## 目录结构

```
src/mcal/lin/
├─── include/           # 头文件
│   ├─── Std_Types.h      # 标准类型定义
│   ├─── LinSlave_Types.h # LinSlave 类型定义
│   ├─── LinSlave_Cfg.h   # 配置参数
│   ├─── LinSlave.h       # 主头文件
│   ├─── LinSlave_Pid.h   # PID处理
│   ├─── LinSlave_Checksum.h  # 校验和处理
│   └─── LinSlave_Hal.h   # 硬件抽象层
├─── src/               # 源代码
│   ├─── LinSlave.c       # 核心实现
│   ├─── LinSlave_Pid.c   # PID算法
│   ├─── LinSlave_Checksum.c  # 校验和算法
│   └─── LinSlave_Hal.c   # HAL层实现(模拟)
├─── test/              # 测试代码
│   └─── test_linslave.c  # 单元测试
├─── example/           # 示例
│   └─── main.c           # 使用示例
└─── Makefile           # 构建脚本
```

## 快速开始

### 编译和测试

```bash
cd src/mcal/lin
make test
```

### 使用示例

```bash
make
./build/example/example
```

## API 使用示例

```c
#include "LinSlave.h"

/* 回调函数: 当收到匹配的报文时调用 */
void RxCallback(uint8 Pid, uint8* ResponseData, uint8* Length, uint8* CsumType)
{
    /* 设置响应数据 */
    ResponseData[0] = 0x11;
    ResponseData[1] = 0x22;
    *Length = 2;
    *CsumType = 1;  /* 增强校验和 */
}

int main(void)
{
    LinSlave_ConfigType config = {
        .NodeId = 5,           /* 从机节点ID */
        .BaudRate = 19200,     /* 波特率 */
        .ResponseLength = 8,   /* 响应数据长度 */
        .ChecksumType = 1      /* 增强校验和 */
    };
    
    /* 初始化 */
    LinSlave_Init(&config);
    
    /* 注册回调 */
    LinSlave_RegisterRxCallback(RxCallback);
    
    /* 中断函数注册到底层 */
    /* 由底层串口中断调用 LinSlave_RxInterruptHandler() */
    
    return 0;
}
```

## 配置参数

在 `LinSlave_Cfg.h` 中修改配置:

```c
#define LINSLAVE_NODE_ID          5       /* 从机节点ID (0-59) */
#define LINSLAVE_BAUDRATE         19200   /* 波特率 */
#define LINSLAVE_RESPONSE_LENGTH  8       /* 响应数据长度 */
#define LINSLAVE_CHECKSUM_TYPE    1       /* 0=经典, 1=增强 */
```

## 状态机

```
IDLE → RX_BREAK → RX_SYNC → RX_PID → RX_DATA → RX_CSUM → TX_RESPONSE → IDLE
```

## 移植指南

将此代码移植到具体MCU时，需要实现 `LinSlave_Hal.c` 中的函数:

1. `LinSlave_Hal_UartInit()` - 初始化UART
2. `LinSlave_Hal_UartSend()` - 发送单字节
3. `LinSlave_Hal_EnableRxInterrupt()` - 使能接收中断
4. `LinSlave_Hal_EnableBreakDetection()` - 使能Break检测
5. `LinSlave_Hal_GetTimestampMs()` - 获取时间戳

在底层UART中断服务中调用 `LinSlave_RxInterruptHandler()` 和 `LinSlave_BreakDetected()`。

## 版本信息

- 版本: 1.0.0
- 作者: Auto-generated
- 协议: LIN 2.2A

## 许可

MIT License
