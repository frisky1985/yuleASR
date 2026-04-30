# CanTSyn (CAN Time Synchronization) 使用手册

## 概述

CanTSyn实现AUTOSAR基于CAN的时间同步机制，支持全局时间基准(Global Time Base)同步。

## 功能特点

- SYNC报文发送和接收
- Follow-up报文支持
- 时间戳记录
- 时间偏移补偿

## API参考

| 函数 | 功能 |
|------|------|
| `CanTSyn_TransmitSync` | 发送SYNC报文 |
| `CanTSyn_TransmitFollowUp` | 发送Follow-up报文 |
| `CanTSyn_GetCurrentTime` | 获取当前时间 |

## 配置示例

```c
const CanTSyn_TimeDomainConfigType CanTSyn_Config[] = {
    {
        .TimeDomainId = 0,
        .SyncPeriod = 10,       /* 10ms */
        .TxPduId = 0
    }
};
```
