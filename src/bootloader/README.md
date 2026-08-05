# Bootloader Modules

## 概述

Bootloader模块提供OTA更新所需的安全启动和回滚机制，包括A/B分区管理、安全验证和自动回滚。

## 模块

### 1. 分区管理器 (bl_partition)

A/B分区管理，支持无感知OTA更新。

**Features:**
- 分区表管理 (CRC32验证)
- A/B分区切换
- 分区状态跟踪 (非活动/活动/可启动/更新中/回滚)
- Flash操作 (读取/撰除/编程/验证)
- 启动计数跟踪

### 2. 安全启动 (bl_secure_boot)

固件验证和版本防回滚。

**Features:**
- 固件头解析和验证
- SHA-256/384/512哈希验证 (CSM集成)
- ECDSA P-256签名验证 (CSM集成)
- 版本防回滚保护
- 证书链验证
- 启动尝试跟踪

### 3. 回滚机制 (bl_rollback)

更新失败时自动回滚到之前版本。

**Features:**
- 版本历史管理 (4个历史版本)
- 启动失败检测
- 自动回滚触发
- 分区切换回滚
- 回滚确认

## 架构

```
Bootloader
    |
    |├-- bl_partition.h/c     # 分区管理
    |├-- bl_secure_boot.h/c   # 安全启动
    |└-- bl_rollback.h/c      # 回滚机制
```

## 依赖

- CSM (Crypto Services Manager): 加密/哈希/签名服务
- KeyM (Key Manager): 密钥管理
- DEM (Diagnostic Event Manager): DTC记录

## 使用示例

### 分区管理

```c
#include "bl_partition.h"

// 初始化分区管理器
bl_partition_manager_t mgr;
bl_partition_init(&mgr, &flash_driver, partition_table_addr);

// 获取活动分区
bl_partition_info_t active;
bl_partition_get_active_app(&mgr, &active);

// 获取OTA目标分区 (非活动)
bl_partition_info_t target;
bl_partition_get_ota_target(&mgr, &target);

// A/B分区切换
bl_partition_switch_active(&mgr, "app_b");
bl_partition_commit_switch(&mgr);
```

### 安全启动

```c
#include "bl_secure_boot.h"

// 初始化安全启动
bl_secure_boot_context_t ctx;
bl_secure_boot_config_t config = {
    .verify_signature = true,
    .verify_hash = true,
    .verify_version = true,
    .min_firmware_version = 0x01000000
};
bl_secure_boot_init(&ctx, &config, csm_ctx, keym_ctx);

// 验证固件
bl_secure_boot_error_t ret = bl_secure_boot_verify(&ctx, firmware, size);
if (ret == BL_SB_OK) {
    // 验证通过，可以启动
}

// 检查版本回滚
ret = bl_secure_boot_check_rollback(&ctx, new_version, current_version);
```

### 回滚机制

```c
#include "bl_rollback.h"

// 初始化回滚管理器
bl_rollback_manager_t mgr;
bl_rollback_config_t config = {
    .max_boot_attempts = 3,
    .auto_rollback_enabled = true
};
bl_rollback_init(&mgr, &config, partition_mgr);

// 记录新版本安装
bl_rollback_record_install(&mgr, new_version, partition_id, hash);

// 启动时记录尝试
bl_rollback_record_boot_attempt(&mgr);

// 启动成功后记录
bl_rollback_record_boot_result(&mgr, BL_BOOT_RESULT_SUCCESS);

// 检查是否需要回滚
bool need_rollback;
bl_rollback_check_needed(&mgr, &need_rollback, &target_version);

// 执行回滚
if (need_rollback) {
    bl_rollback_execute(&mgr, BL_ROLLBACK_REASON_BOOT_FAILURE);
}

// 回滚成功后确认
bl_rollback_confirm(&mgr);
```

## 合规

- UNECE R156 Software Update Management System
- ISO/SAE 21434 Cybersecurity Engineering
- ASIL-D Safety Level
