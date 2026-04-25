# OTA Core Module

这个目录包含OTA (Over-The-Air)核心模块的实现，包括OTA Manager、Downloader和Package Parser。

## 模块概览

### 1. ota_types.h
定义了OTA模块使用的通用类型、错误码和枚举，包括：
- OTA状态机状态 (IDLE/DOWNLOADING/VERIFYING/INSTALLING等)
- 下载状态
- 压缩类型 (None/Zstd/LZ4/Gzip)
- 加密类型 (None/AES-128-GCM/AES-256-GCM)
- ECU更新信息
- Campaign信息

### 2. ota_manager.h/c
OTA管理器，负责：
- OTA状态机管理
- Campaign活动管理
- 前置条件检查
- 下载缓存管理 (支持断点续传)
- 回滚机制
- 与Bootloader分区管理器集成

#### 状态转换:
```
IDLE -> CAMPAIGN_RECEIVED -> DOWNLOADING -> VERIFYING -> READY_TO_INSTALL -> INSTALLING -> ACTIVATING -> SUCCESS
                                              |
                                              v
                                         ROLLING_BACK/FAILED
```

### 3. ota_downloader.h/c
HTTP下载器，支持：
- HTTP/HTTPS下载
- Range请求支持 (断点续传)
- 下载进度回调
- 下载统计信息 (速度、剩余时间)
- 哈希验证 (SHA-256)
- 多重试机制

#### 断点续传支持:
```c
// 检查服务器是否支持Range
ota_downloader_check_resume_support(ctx, url, &supports_range, &file_size);

// 从指定偏移量开始下载
request.offset = previous_offset;
request.mode = OTA_DL_MODE_RESUME;
```

### 4. ota_package.h/c
包解析器，支持：
- Vehicle Package (.vpkg) 格式
- ECU Package (.epkg) 格式
- Manifest JSON解析
- 签名验证 (Raw ECDSA/CMS/COSE)
- 哈希验证
- 压缩解压 (Zstd/LZ4)

#### 包格式:
**Vehicle Package (.vpkg)**:
```
+--------+----------+---------+----------+-----------+
| Header | Manifest | Payload | Signature|
+--------+----------+---------+----------+
```

**ECU Package (.epkg)**:
```
+--------+----------+--------+-----+--------+
| Header | Firmware | Delta  | Hash| Layout |
+--------+----------+--------+-----+--------+
```

## 依赖

- Bootloader分区管理 (bl_partition.h/c)
- CSM密码服务管理 (csm_core.h) - 用于签名验证和哈希计算
- KeyM密钥管理 (keym_core.h) - 用于验证密钥管理
- OTA UDS Client (ota_uds_client.h/c) - 用于UDS传输

## API使用示例

### 基本OTA流程:
```c
// 1. 初始化OTA Manager
ota_manager_init(&mgr_ctx, &config, &partition_mgr);
ota_manager_init_download_cache(&mgr_ctx, cache_buffer, cache_size);

// 2. 接收Campaign
ota_campaign_info_t campaign = {
    .campaign_id = "CAMP-001",
    .name = "Q2 Firmware Update",
    .num_ecu_updates = 1,
    ...
};
ota_manager_receive_campaign(&mgr_ctx, &campaign);

// 3. 开始下载
ota_manager_start_download(&mgr_ctx, "CAMP-001");

// 4. 验证包
ota_manager_start_verify(&mgr_ctx);

// 5. 安装
ota_manager_start_install(&mgr_ctx);

// 6. 激活
ota_manager_activate(&mgr_ctx);
```

### 下载器使用:
```c
// 初始化
ota_downloader_init(&dl_ctx, &dl_config, &transport);

// 配置下载请求
ota_download_request_t request = {
    .url = "https://ota.example.com/firmware.bin",
    .package_id = "PKG-001",
    .offset = 0,
    .expected_size = 100000,
    .mode = OTA_DL_MODE_FULL
};

// 开始下载
ota_downloader_start(&dl_ctx, &request);

// 处理下载 (在主循环中)
while (ota_downloader_process(&dl_ctx)) {
    // 处理其他任务
}
```

## 单元测试

测试位于 `tests/unit/ota/`:
- `test_ota_manager.c` - OTA Manager测试
- `test_ota_downloader.c` - OTA Downloader测试
- `test_ota_package.c` - OTA Package Parser测试

运行测试:
```bash
cd /home/admin/eth-dds-integration
gcc -I src -I tests/unity tests/unit/ota/test_ota_manager.c src/ota/ota_manager.c src/ota/ota_types.h -o test_ota_manager
./test_ota_manager
```

## 安全特性

- 固件签名验证 (使用CSM)
- 固件哈希验证 (SHA-256)
- 版本回滚保护
- 加密传输支持 (AES-GCM)
- 分区交换保护

## 规范合规

- UNECE R156 SUMS
- ISO/SAE 21434 网络安全
- ISO 14229-1 UDS
- ISO 24089 软件更新工程

## 版本

- 版本: 1.0.0
- 日期: 2026-04-26
- ASIL等级: D
