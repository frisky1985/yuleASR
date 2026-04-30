# DoIP (Diagnostic over IP) 使用手册

## 概述

DoIP是基于ISO 13400标准的诊断协议，允许通过以太网进行车辆诊断通信。支持高速诊断数据传输和远程诊断功能。

## 功能特点

| 特性 | 说明 |
|------|------|
| 车辆发现 | 通过Vehicle Identification发现网络中的车辆 |
| 路由激活 | 建立到车辆的诊断会话 |
| 诊断传输 | 支持标准UDS诊断消息 |
| 心跳检测 | 监控连接状态 |

## 配置方法

### 1. 基本配置 (DoIP_Cfg.h)

```c
/* 协议版本 */
#define DOIP_PROTOCOL_VERSION       0x02
#define DOIP_PROTOCOL_VERSION_INV   0xFD

/* 请求字节限制 */
#define DOIP_MAX_REQUEST_LENGTH     4096

/* 活跃检查 */
#define DOIP_ALIVE_CHECK_TIMEOUT    500  /* ms */
#define DOIP_ALIVE_CHECK_RESPONSE   500  /* ms */

/* 重试次数 */
#define DOIP_MAX_RETRIES            3
```

### 2. 连接配置

```c
const DoIP_ConnectionConfigType DoIP_ConnectionConfig[] = {
    {
        .ConnectionId = 0,
        .LocalAddr = {192, 168, 1, 100},    /* 本地IP */
        .LocalPort = 13400,                  /* DoIP标准端口 */
        .Protocol = DOIP_TCP,
        .ActivationType = DOIP_ACTIVATION_DEFAULT
    },
    {
        .ConnectionId = 1,
        .LocalPort = 13400,
        .Protocol = DOIP_UDP,
        .ActivationType = DOIP_ACTIVATION_NONE
    }
};
```

## API参考

### 核心函数

| 函数 | 功能 | 参数 |
|------|------|------|
| `DoIP_Init` | 初始化 | ConfigPtr |
| `DoIP_TcpTransmit` | TCP发送 | PayloadType, Data, Length |
| `DoIP_UdpTransmit` | UDP发送 | PayloadType, Data, Length |
| `DoIP_RoutingActivationRequest` | 路由激活请求 | SourceAddr, ActivationType |
| `DoIP_DiagnosticMessage_Transmit` | 诊断消息发送 | SrcAddr, DstAddr, Data, Length |
| `DoIP_MainFunction` | 主函数 | void |

### 时间参数

```c
typedef struct {
    uint16 InitialVehicleAnnouncementTime;  /* 500ms */
    uint16 VehicleAnnouncementInterval;      /* 500ms */
    uint8  VehicleAnnouncementCount;         /* 3 */
    uint16 AliveCheckResponseTimeout;        /* 500ms */
    uint16 DiagnosticMessageTimeout;         /* 2000ms */
} DoIP_TimingType;
```

## 使用示例

### 车辆发现

```c
#include "DoIP.h"

void DiscoverVehicles(void) {
    /* 发送Vehicle Identification Request */
    DoIP_TcpTransmit(0x0001, NULL, 0);
    
    /* 等待Vehicle Announcement */
    /* 响应通知通过DoIP_RxIndication */
}

void DoIP_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr) {
    /* 解析Generic Header */
    DoIP_GenericHeaderType header;
    DoIP_ParseGenericHeader(PduInfoPtr->SduDataPtr, &header);
    
    switch (header.PayloadType) {
        case 0x0004:  /* Vehicle Identification Response */
            ProcessVehicleInfo(PduInfoPtr->SduDataPtr + 8);
            break;
        case 0x0006:  /* Routing Activation Response */
            ProcessRoutingActivation(PduInfoPtr->SduDataPtr + 8);
            break;
        case 0x8001:  /* Diagnostic Message */
            ProcessDiagnosticMessage(PduInfoPtr->SduDataPtr + 8);
            break;
    }
}
```

### 路由激活

```c
void ActivateRouting(void) {
    uint16 sourceAddr = 0x0E00;  /* 诊断仪地址 */
    uint8 activationType = 0x00;  /* 默认激活 */
    
    Std_ReturnType result = DoIP_RoutingActivationRequest(sourceAddr, activationType);
    
    if (result == E_OK) {
        /* 等待激活响应 */
    }
}
```

### 诊断会话

```c
void StartDiagnosticSession(void) {
    uint16 testerAddr = 0x0E00;
    uint16 ecuAddr = 0xE000;
    
    /* 发送会话控制请求 (0x10 0x01) */
    uint8 sessionRequest[] = {0x10, 0x01};
    DoIP_DiagnosticMessage_Transmit(testerAddr, ecuAddr, 
                                     sessionRequest, sizeof(sessionRequest));
}
```

## 问题排查

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| 连接失败 | 网络配置错误 | 检查IP地址和端口 |
| 路由激活拒绝 | 资源不足或不支持的激活类型 | 检查Activation Type |
| 超时 | 网络延迟或车辆未响应 | 调整超时参数 |
| 无法发现车辆 | 广播被阻止 | 检查网络防火墙设置 |

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0.0 | 2026-04-30 | 支持ISO 13400-2:2019 |
