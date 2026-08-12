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

## 国密 SM2/SM3 支持状态 (就绪框架, 2026-08-12)

### 结论

**SM2/SM3 当前无法完整实装**: 本仓库无任何 SM 后端可用, 已落地为
「SM2 就绪框架」(枚举/分发/文档), 全部调用显式 fail-closed。

### 依赖调研结论

| 候选后端 | SM2/SM3 支持 | 说明 |
|----------|-------------|------|
| mbedtls (third_party/mbedtls) | ❌ 无 | 上游 mbedtls 不包含 SM2/SM3 算法 |
| S32K312 HSM/HSE (src/bsw/mcal/crypto) | ❌ 无 | HSM 仅 AES/ECC P-256/P-384/SHA-256/RNG (见 Crypto_S32K312_Hsm.h SID 表), HSE 固件不支持国密 |
| Csm 栈 (crypto_stack) | ❌ 无 | csm_algorithm_t 原无 SM 条目; 后端仅 mbedtls SHA-256/HMAC/AES |
| GmSSL / 其他国密库 | ✅ 需引入 | 唯一可行路径: 引入 GmSSL (或 SM 版 HSM 固件) 后接入 CSM |

### 已落地 (就绪框架)

1. `Csm_Types.h`: 新增 `CSM_ALGOFAM_SM2` / `CSM_ALGOFAM_SM3` 算法族枚举。
2. `csm_core.h`: 新增 `CSM_ALGO_SM3_HASH` / `CSM_ALGO_SM2_SM3` 算法枚举,
   名称表同步; 实际调用由现有 fail-closed 分发返回
   `CSM_ERROR_ALGO_NOT_SUPPORTED` (test_csm 已覆盖)。
3. `bl_secure_boot.h/c`: `BL_SB_SIGN_SM2_SM3` / `BL_SB_HASH_SM3` 在分发表中
   显式返回 `BL_SB_ERROR_ALGO_NOT_SUPPORTED` (新增错误码 -19), 不得降级到
   SHA-256/ECDSA (防算法降级攻击); test_bootloader 已覆盖。

### 完整实装路径 (后续)

1. 引入 GmSSL 或具备 SM 能力的 HSM 固件。
2. Csm 后端接入: `csm_hash` 支持 SM3, `csm_mac_verify`/签名路径支持 SM2-SM3。
3. 移除 `bl_secure_boot` 中 SM2/SM3 的 NOT_SUPPORTED 分支, 改分发到 CSM。
4. 固件头/证书链 SM2 验签 + 端到端测试。
