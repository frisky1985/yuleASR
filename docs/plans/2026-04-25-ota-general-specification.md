# 通用OTA规范 (Over-The-Air Update Specification)

**版本**: 1.0  
**日期**: 2026-04-25  
**状态**: 设计阶段  
**依赖**: UDS诊断栈 (Line A), SecOC安全栈 (Line B)

---

## 1. 范围与目标

### 1.1 目标
定义车载ECU固件远程更新的通用规范，支持：
- 安全、可靠的固件下载
- A/B分区无感知切换
- 断点续传
- 回滚保护
- 符合ISO/SAE 21434网络安全要求

### 1.2 参考规范
- ISO 14229-1:2020 UDS - 0x34/0x35/0x36/0x37传输服务
- ISO 24089:2023 道路车辆软件更新工程
- ISO/SAE 21434:2021 汽车网络安全工程
- UNECE R156 软件更新管理系统
- AUTOSAR FOTA标准

---

## 2. 架构设计

### 2.1 总体架构

```
┌─────────────────────────────────────────────────────────────┐
│                   OTA Cloud Backend                            │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ • Firmware Repository (版本管理)                     │  │
│  │ • Campaign Management (更新活动管理)                   │  │
│  │ • Delta Generator (增量包生成)                        │  │
│  │ • Security Signing (固件签名)                        │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              │ HTTPS/MQTT/DDS
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   OTA Master (Vehicle Gateway)                 │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ OTA Core                                              │  │
│  │ • Package Manager (更新包管理)                       │  │
│  │ • Download Manager (下载管理/断点续传)               │  │
│  │ • Security Validator (签名验证)                    │  │
│  │ • Campaign Executor (更新执行)                       │  │
│  ├───────────────────────────────────────────────────────┤  │
│  │ OTA Agent API                                         │  │
│  │ • ECU Update Control                                  │  │
│  │ • Progress Reporting                                  │  │
│  │ • Rollback Trigger                                    │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              │
         ┌───────────────┬───────────────┬───────────────┐
         │        CAN        │      Ethernet      │      DoIP       │
         │        Bus        │      Switch        │     Gateway     │
         └───────────────┴───────────────┴───────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                   Target ECUs                                │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ Bootloader + UDS Stack (0x34/0x35/0x36/0x37)         │  │
│  │ • Partition A (Active) / Partition B (Update Target)    │  │
│  │ • Secure Boot Verification (SecOC/CSM/KeyM)             │  │
│  │ • Rollback Detection (Version Check)                    │  │
│  │ • DEM Integration (Update DTC Logging)                  │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 OTA流程状态机

```
                    +-----------+
                    |   IDLE    |
                    +-----+-----+
                          | Receive Campaign
                          v
              +---------------------------+
              |   CAMPAIGN_RECEIVED       |
              |   Validate manifest       |
              +-------------+-------------+
                            | Validation OK
                            v
              +---------------------------+
              |   DOWNLOADING             |
              |   HTTP range requests     |
              |   Resume capability       |
              +-------------+-------------+
                            | Download OK
                            v
              +---------------------------+
              |   VERIFYING               |
              |   Signature check         |
              |   Hash verification       |
              +-------------+-------------+
                            | Verify OK
                            v
              +---------------------------+
              |   READY_TO_INSTALL        |
              |   Wait for conditions     |
              +-------------+-------------+
                            | Conditions met
                            v
              +---------------------------+
              |   INSTALLING              |
              |   UDS 0x34/0x36/0x37      |
              |   Flash programming       |
              +-------------+-------------+
                            | Install OK
                            v
              +---------------------------+
              |   ACTIVATING              |
              |   Reboot request          |
              |   Partition switch        |
              +-------------+-------------+
                            | Reboot OK
                            v
              +---------------------------+
              |   VERIFYING_BOOT          |
              |   Secure boot check       |
              |   Version confirmation    |
              +-------------+-------------+
                            | Boot OK
                            v
                    +-------------+
                    |   SUCCESS   |
                    +-------------+

任意状态均可转移到:
    +---------------------------+
    |   ROLLING_BACK            |
    |   Restore previous        |
    +---------------------------+
              |
              v
    +---------------------------+
    |   FAILED                  |
    +---------------------------+
```

---

## 3. 更新包格式

### 3.1 物理格式 (Vehicle Package)

```
┌───────────────────────────────────────────────────┐
│           Vehicle Package (.vpkg)                        │
├───────────────────────────────────────────────────┤
│                                                          │
│  ┌─────────────────────────────────────────┐       │
│  │ Header                                             │       │
│  │ - Magic: "VPCK" (4 bytes)                          │       │
│  │ - Version: 1 (uint8)                               │       │
│  │ - Package ID: UUID (16 bytes)                      │       │
│  │ - Timestamp: Unix epoch (uint64)                   │       │
│  │ - Target Vehicle: VIN (17 bytes)                   │       │
│  │ - Target HW Version                                │       │
│  │ - Signature Offset                                 │       │
│  │ - Manifest Offset                                  │       │
│  └─────────────────────────────────────────┘       │
│                                                          │
│  ┌─────────────────────────────────────────┐       │
│  │ Manifest (JSON/Protobuf)                           │       │
│  │ - Campaign metadata                                │       │
│  │ - ECU update list                                  │       │
│  │ - Dependencies                                     │       │
│  │ - Pre-conditions                                   │       │
│  └─────────────────────────────────────────┘       │
│                                                          │
│  ┌─────────────────────────────────────────┐       │
│  │ Payload: ECU Packages (zip/tar)                    │       │
│  │ - ecu_1.epkg                                       │       │
│  │ - ecu_2.epkg                                       │       │
│  │ - ...                                              │       │
│  └─────────────────────────────────────────┘       │
│                                                          │
│  ┌─────────────────────────────────────────┐       │
│  │ Signature (CMS/PKCS#7 or Raw ECDSA)                │       │
│  │ - Signer certificate chain                         │       │
│  │ - Signature over header+manifest+payload           │       │
│  └─────────────────────────────────────────┘       │
│                                                          │
└───────────────────────────────────────────────────┘
```

### 3.2 ECU更新包格式 (ECU Package)

```
┌───────────────────────────────────────────────────┐
│           ECU Package (.epkg)                              │
├───────────────────────────────────────────────────┤
│                                                          │
│  ┌─────────────────────────────────────────┐       │
│  │ Header                                             │       │
│  │ - ECU ID (uint16)                                  │       │
│  │ - Target HW Version                                │       │
│  │ - Current FW Version (from)                        │       │
│  │ - Target FW Version (to)                           │       │
│  └─────────────────────────────────────────┘       │
│                                                          │
│  ┌─────────────────────────────────────────┐       │
│  │ Firmware Image                                     │       │
│  │ - Format: BIN/HEX/SREC/ELF                         │       │
│  │ - Compression: None/Zstd/LZ4                       │       │
│  │ - Encryption: Optional (AES-256-GCM)               │       │
│  └─────────────────────────────────────────┘       │
│                                                          │
│  ┌─────────────────────────────────────────┐       │
│  │ Delta Patch (Optional)                             │       │
│  │ - Base Version                                     │       │
│  │ - BSDIFF/HDiff patch data                          │       │
│  └─────────────────────────────────────────┘       │
│                                                          │
│  ┌─────────────────────────────────────────┐       │
│  │ Memory Layout Description                          │       │
│  │ - Flash regions                                    │       │
│  │ - Partition addresses                              │       │
│  └─────────────────────────────────────────┘       │
│                                                          │
│  ┌─────────────────────────────────────────┐       │
│  │ Checksum/Hash (SHA-256)                            │       │
│  └─────────────────────────────────────────┘       │
│                                                          │
└───────────────────────────────────────────────────┘
```

### 3.3 Manifest示例 (JSON)

```json
{
  "manifest_version": "1.0.0",
  "package_info": {
    "id": "550e8400-e29b-41d4-a716-446655440000",
    "created_at": "2026-04-25T10:00:00Z",
    "created_by": "ota_build_system",
    "target_vehicle": {
      "vin": "WBA12345678901234",
      "hw_platform": "EV_PLATFORM_V2",
      "compatibility": ["HW_V1", "HW_V2"]
    }
  },
  "campaign": {
    "id": "CAMP-2026-04-001",
    "name": "Q2 Firmware Update",
    "description": "Update infotainment and powertrain ECUs",
    "priority": "high",
    "scheduled_window": {
      "start": "2026-04-26T02:00:00Z",
      "end": "2026-04-30T23:59:59Z"
    }
  },
  "ecu_updates": [
    {
      "ecu_id": 0x0101,
      "name": "Infotainment_HU",
      "hardware_version": "2.1.0",
      "firmware_version": {
        "from": "3.2.1",
        "to": "3.3.0"
      },
      "package_file": "ecu_0101.epkg",
      "size": 52428800,
      "hash": "sha256:abc123...",
      "signature_required": true,
      "dependencies": [],
      "pre_conditions": {
        "battery_voltage_min_v": 12.0,
        "vehicle_speed_max_kmh": 0,
        "engine_state": "off",
        "safe_to_update": true
      },
      "installation": {
        "estimated_time_seconds": 300,
        "reboot_required": true,
        "user_confirmation": false
      }
    },
    {
      "ecu_id": 0x0201,
      "name": "Powertrain_ECU",
      "hardware_version": "1.5.0",
      "firmware_version": {
        "from": "2.0.0",
        "to": "2.1.0"
      },
      "package_file": "ecu_0201.epkg",
      "size": 104857600,
      "hash": "sha256:def456...",
      "signature_required": true,
      "dependencies": [
        {"ecu_id": 0x0101, "min_version": "3.3.0"}
      ],
      "pre_conditions": {
        "battery_voltage_min_v": 13.5,
        "vehicle_speed_max_kmh": 0,
        "parking_brake": "engaged",
        "gear": "park"
      },
      "installation": {
        "estimated_time_seconds": 600,
        "reboot_required": true,
        "user_confirmation": true,
        "confirmation_prompt": "Powertrain update requires 10 minutes. Do not drive during update."
      }
    }
  ],
  "rollout": {
    "strategy": "wave",
    "batch_size_percent": 10,
    "success_threshold_percent": 95
  }
}
```

---

## 4. 安全要求

### 4.1 签名验证链

```
┌─────────────────────────────────────────────────────────────┐
│           签名验证链                                    │
│                                                          │
│  Tier 1: Root CA (主机厂根证书)                           │
│     │                                                   │
│     │ 签发                                              │
│     ▼                                                   │
│  Tier 2: OEM CA (整车厂证书)                            │
│     │                                                   │
│     │ 签发                                              │
│     ▼                                                   │
│  Tier 3: OTA Signing Key (更新签名私钥)                    │
│     │                                                   │
│     │ 签名                                              │
│     ▼                                                   │
│  Package Signature (包签名)                              │
│                                                          │
│  验证过程:                                               │
│  1. 验证OEM CA证书由Root CA签发                        │
│  2. 验证更新签名由OEM CA授权                            │
│  3. 验证包内容签名正确                                    │
│  4. 验证固件指纹与manifest匹配                          │
│                                                          │
└─────────────────────────────────────────────────────────────┘
```

### 4.2 安全启动 (Secure Boot)

```c
// Secure Boot验证流程
typedef enum {
    SB_OK = 0,
    SB_INVALID_SIGNATURE,
    SB_INVALID_HASH,
    SB_VERSION_ROLLBACK,
    SB_INVALID_CERTIFICATE
} secure_boot_status_t;

secure_boot_status_t secure_boot_verify(
    const partition_info_t *partition,
    const certificate_chain_t *cert_chain,
    const keym_context_t *keym_ctx
) {
    // 1. 验证签名
    if (!verify_signature(partition->signature, cert_chain)) {
        return SB_INVALID_SIGNATURE;
    }
    
    // 2. 验证固件哈希
    if (!verify_hash(partition->firmware, partition->expected_hash)) {
        return SB_INVALID_HASH;
    }
    
    // 3. 防回滚检查
    if (partition->version <= get_current_version()) {
        return SB_VERSION_ROLLBACK;
    }
    
    // 4. 证书有效期检查
    if (!verify_certificate_validity(cert_chain)) {
        return SB_INVALID_CERTIFICATE;
    }
    
    return SB_OK;
}
```

### 4.3 与现有安全栈集成

```
┌─────────────────────────────────────────────────────────────┐
│              OTA安全与现有安全栈集成                        │
├─────────────────────────────────────────────────────────────┤
│                                                          │
│  OTA Agent → KeyM                                        │
│  • 从KeyM获取验证公钥                                    │
│  • 更新密钥轮换通知KeyM                                    │
│                                                          │
│  OTA Agent → CSM                                        │
│  • 使用CSM进行包签名验证                                  │
│  • 使用CSM计算固件哈希                                    │
│                                                          │
│  OTA Agent → SecOC                                      │
│  • 验证更新命令的SecOC保护                              │
│  • 更新SecOC密钥的OTA数据管理                           │
│                                                          │
│  Bootloader → SecOC/CSM/KeyM                            │
│  • 安全启动验证固件签名                                    │
│  • 使用KeyM管理的密钥进行验证                         │
│                                                          │
│  OTA Agent → DEM                                        │
│  • 记录更新失败DTC                                         │
│  • 更新状态通过DDS Topic发布                            │
│                                                          │
└─────────────────────────────────────────────────────────────┘
```

---

## 5. UDS集成

### 5.1 UDS服务映射

| UDS SID | 服务名称 | OTA用途 |
|---------|----------|---------|
| 0x10 | Diagnostic Session Control | 进入Programming会话 |
| 0x11 | ECU Reset | 更新完成后复位 |
| 0x22 | Read Data By Identifier | 读取当前版本/状态 |
| 0x27 | Security Access | 解锁编程权限 |
| 0x31 | Routine Control | 执行预编程例程 |
| 0x34 | Request Download | 请求下载固件 |
| 0x36 | Transfer Data | 传输固件数据 |
| 0x37 | Request Transfer Exit | 结束数据传输 |

### 5.2 UDS传输协议

```c
// OTA固件更新使用UDS传输协议
typedef struct {
    uint32_t memory_address;
    uint32_t memory_size;
    uint8_t data_format_identifier;  // 压缩/加密标识
    uint8_t address_length_format;    // 地址和长度格式
} download_request_t;

// UDS 0x34 Request Download流程
void ota_uds_download_flow(ecu_id_t ecu, const firmware_image_t *image) {
    // 1. 进入Extended会话
    uds_send_0x10(DCM_SESSION_EXTENDED);
    
    // 2. 解锁编程权限
    uds_send_0x27(0x01);  // Request Seed
    // Calculate Key...
    uds_send_0x27(0x02, key);  // Send Key
    
    // 3. 请求下载
    download_request_t req = {
        .memory_address = image->flash_address,
        .memory_size = image->size,
        .data_format_identifier = 0x00,  // 无压缩
        .address_length_format = 0x44    // 4字节地址, 4字节长度
    };
    uds_send_0x34(&req);
    
    // 4. 传输数据 (UDS 0x36)
    uint16_t block_sequence = 1;
    for (chunk in image->chunks) {
        uds_send_0x36(block_sequence++, chunk);
    }
    
    // 5. 结束传输 (UDS 0x37)
    uds_send_0x37();
    
    // 6. 执行验证例程 (UDS 0x31)
    uds_send_0x31(START_ROUTINE, VERIFY_CHECKSUM);
    
    // 7. 软复位 (UDS 0x11)
    uds_send_0x11(SOFT_RESET);
}
```

---

## 6. A/B分区方案

### 6.1 分区布局

```
Flash Memory Layout:
┌─────────────────────────────────────────────────────────────┐
│ Bootloader (Single Copy)                               │  64KB
│ - 固定位置，不可更新                                      │
│ - Secure Boot验证逻辑                                  │
├─────────────────────────────────────────────────────────────┤
│ Active Partition Table                                 │   4KB
│ - 当前活动分区标识 (A/B)                                 │
│ - 版本信息                                           │
│ - 尝试计数器                                         │
│ - CRC校验和                                          │
├─────────────────────────────────────────────────────────────┤
│ Partition A                                            │  2MB
│ - Application Firmware                                 │
│ - Calibration Data                                     │
├─────────────────────────────────────────────────────────────┤
│ Partition B                                            │  2MB
│ - Application Firmware (Backup)                        │
│ - Calibration Data (Backup)                            │
├─────────────────────────────────────────────────────────────┤
│ Download Buffer                                        │  512KB
│ - 临时存储下载的更新包                                     │
│ - 更新完成后清除                                       │
└─────────────────────────────────────────────────────────────┘
```

### 6.2 分区切换逻辑

```c
typedef enum {
    PARTITION_A = 0,
    PARTITION_B = 1
} partition_id_t;

typedef struct {
    partition_id_t active_partition;
    partition_id_t boot_partition;   // 下次启动分区
    uint32_t version_a;
    uint32_t version_b;
    uint8_t retry_count;
    bool rollback_triggered;
    uint32_t crc;
} partition_table_t;

// 设置下次启动分区
void set_boot_partition(partition_id_t partition) {
    partition_table_t *pt = get_partition_table();
    pt->boot_partition = partition;
    pt->retry_count = 0;
    pt->crc = calculate_crc(pt, sizeof(partition_table_t) - 4);
    flash_write(PARTITION_TABLE_ADDR, pt, sizeof(partition_table_t));
}

// Bootloader分区选择逻辑
partition_id_t bootloader_select_partition(void) {
    partition_table_t *pt = get_partition_table();
    
    // 检查CRC
    if (!verify_partition_table(pt)) {
        // CRC错误，使用默认分区A
        return PARTITION_A;
    }
    
    // 检查重试次数
    if (pt->retry_count >= 3) {
        // 切换到另一分区
        partition_id_t other = (pt->boot_partition == PARTITION_A) ? 
                               PARTITION_B : PARTITION_A;
        return other;
    }
    
    // 检查回滚标志
    if (pt->rollback_triggered) {
        return pt->active_partition;  // 保持当前分区
    }
    
    return pt->boot_partition;
}
```

---

## 7. 回滚机制

### 7.1 自动回滚触发条件

| 类型 | 触发条件 | 处理逻辑 |
|------|----------|---------|
| 启动失败 | 连续3次启动失败 | 自动切换到备份分区 |
| 验证失败 | Secure Boot验证失败 | 拒绝启动，切换分区 |
| 版本回滚 | 版本号小于当前 | 拒绝更新 |
| 用户取消 | 用户主动回退 | 等待下次更新窗口 |
| 云端回滚 | 批次更新失败率超过阈值 | 自动取消后续更新 |

### 7.2 回滚流程

```
Update Failed /
User Rollback    
       │
       ▼
+---------------+
│ Mark Active   │  标记当前分区为活动分区
│ Partition     │  (回滚到原分区)
+-------+-------+
        │
        ▼
+---------------+
│ Clear Update  │  清除更新标志
│ Flags         │
+-------+-------+
        │
        ▼
+---------------+
│ Log DTC       │  记录DTC: UpdateFailed (0xF0A0)
│ Event         │
+-------+-------+
        │
        ▼
+---------------+
│ Report Status │  上报回滚状态至OTA Cloud
│ to Cloud      │
+---------------+
```

---

## 8. DDS集成

### 8.1 OTA状态Topic

```yaml
# OTA状态通过DDS发布
OTACampaignStatus:
  topic: "Vehicle/OTA/Campaign/{campaign_id}/Status"
  qos: reliable_transient_local
  fields:
    - campaign_id: string
    - status: enum [IDLE, DOWNLOADING, VERIFYING, INSTALLING, SUCCESS, FAILED, ROLLING_BACK]
    - ecu_updates: sequence<ecu_update_status_t>
    - progress_percent: uint8
    - estimated_time_seconds: uint32
    - error_code: uint32
    - timestamp: uint64

ECUUpdateStatus:
  topic: "Vehicle/OTA/ECU/{ecu_id}/Status"
  qos: reliable_volatile
  fields:
    - ecu_id: uint16
    - current_version: string
    - target_version: string
    - status: enum [IDLE, DOWNLOADING, INSTALLING, VERIFYING, REBOOTING, SUCCESS, FAILED]
    - progress_bytes: uint64
    - total_bytes: uint64
    - last_error: string
    - timestamp: uint64

OTAFirmwareVersion:
  topic: "Vehicle/OTA/FirmwareVersion"
  qos: reliable_transient_local
  fields:
    - ecu_versions: sequence<ecu_version_t>
      - ecu_id: uint16
      - ecu_name: string
      - firmware_version: string
      - hardware_version: string
      - last_update_time: uint64
    - timestamp: uint64
```

---

## 9. 实施计划 (简要)

### 9.1 模块划分

```
src/ota/
├── ota_core/           # OTA核心逻辑
│   ├── ota_manager.c/h
│   ├── ota_downloader.c/h
│   └── ota_installer.c/h
├── ota_security/       # 安全验证
│   ├── ota_signature.c/h
│   └── ota_verifier.c/h
├── ota_uds/            # UDS集成
│   ├── ota_udsa_client.c/h
│   └── ota_transfer.c/h
├── bootloader/         # Bootloader扩展
│   ├── bl_partition.c/h
│   ├── bl_secure_boot.c/h
│   └── bl_rollback.c/h
└── tests/              # 测试
    ├── test_ota_core.c
    └── test_ota_security.c
```

### 9.2 里程碑

| 周期 | 任务 | 输出 |
|------|------|-------|
| Week 1 | OTA核心框架 | ota_manager, ota_downloader |
| Week 2 | 安全验证 | 签名验证, 安全启动 |
| Week 3 | UDS集成 | UDS传输客户端 |
| Week 4 | Bootloader扩展 | 分区切换, 回滚 |
| Week 5 | DDS集成 | 状态Topic发布 |
| Week 6 | 测试验收 | 端到端更新测试 |

---

## 10. 安全合规检查表

| 要求 | 实现方式 | 验证方法 |
|------|---------|----------|
| UNECE R156 SUMS | OTA Master + DEM集成 | TBD |
| ISO/SAE 21434 | SecOC + CSM + KeyM | 安全审计 |
| 固件签名 | ECDSA/SHA-256 | 签名验证测试 |
| 防回滚 | 版本号检查 | 版本对比测试 |
| 安全启动 | Bootloader扩展 | 启动验证 |
| 断点续传 | HTTP Range | 网络中断测试 |

---

**备注**: 本文档为OTA通用规范初稿，具体实施需等待UDS和SecOC完成后进行详细设计。
