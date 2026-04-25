# OTA UDS Client Module

## Overview

OTA UDS Client模块实现基于ISO 14229-1:2020 UDS协议的在线更新客户端，支持UDS 0x34/0x36/0x37传输服务和0x31例程控制服务。

## Features

- **UDS 0x34 Request Download**: 请求下载固件，支持数据格式和目标地址配置
- **UDS 0x36 Transfer Data**: 分块传输固件数据，支持块序列号管理
- **UDS 0x37 Request Transfer Exit**: 结束数据传输
- **UDS 0x31 Routine Control**: 执行验证例程和固件激活
- **Progress Callback**: 实时传输进度回调
- **State Management**: 完整的OTA状态机
- **DEM Integration**: 更新失败DTC记录

## Architecture

```
OTA UDS Client
    |├-- ota_uds_client.c    # 主实现
    |├-- ota_uds_client.h    # 头文件和API
```

## Dependencies

- DCM (Diagnostic Communication Manager): UDS协议基础类型定义
- CSM (Crypto Services Manager): 签名验证
- KeyM (Key Manager): 密钥管理
- DEM (Diagnostic Event Manager): DTC记录

## Usage Example

```c
#include "ota_uds_client.h"

// 配置传输接口
ota_uds_transport_if_t transport = {
    .send_request = my_send_request,
    .enter_programming_session = my_enter_session,
    .unlock_security = my_unlock,
    .ecu_reset = my_reset
};

// 初始化
ota_uds_config_t config = {
    .target_ecu_id = 0x0101,
    .transfer_block_size = 4095,
    .transport = &transport
};

ota_uds_context_t ctx;
ota_uds_init(&ctx, &config);

// 启动OTA会话
eret = ota_uds_start_session(&ctx);

// 请求下载
ota_firmware_info_t fw_info = {
    .firmware_version = 0x01020000,
    .firmware_size = 65536,
    .download_address = 0x08010000
};
ota_uds_request_download(&ctx, &fw_info);

// 传输固件
eret = ota_uds_transfer_firmware(&ctx, firmware_data, firmware_size);

// 完成更新
eret = ota_uds_complete_update(&ctx);
```

## Compliance

- ISO 14229-1:2020 UDS
- UNECE R156 Software Update Management System
- ISO/SAE 21434 Cybersecurity
