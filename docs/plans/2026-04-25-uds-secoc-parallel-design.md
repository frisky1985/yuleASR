# UDS诊断栈与SecOC安全栈并行开发设计

**日期**: 2026-04-25  
**版本**: 1.0  
**状态**: 已批准，待实施

---

## 1. 设计目标

实现车载诊断协议栈(UDS/DoCAN/DoIP)与安全通信栈(SecOC/CSM/KeyM)的双线并行开发，满足车规级量产要求。

---

## 2. UDS诊断协议栈 (线路A)

### 2.1 架构设计

```
┌─────────────────────────────────────────────────────────────┐
│                    UDS Application Layer                      │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │ DCM (诊断   │  │ DEM (诊断   │  │ FIM (功能抑制)      │  │
│  │ 通信管理)   │  │ 事件管理)   │  │                     │  │
│  └──────┬──────┘  └──────┬──────┘  └─────────────────────┘  │
└─────────┼────────────────┼───────────────────────────────────┘
          │                │
┌─────────┼────────────────┼───────────────────────────────────┐
│         │    UDS Transport Layer (DoCAN/DoIP)                 │
│  ┌──────▼──────┐  ┌──────▼──────┐  ┌─────────────────────┐  │
│  │ DoCAN       │  │ DoIP        │  │ PDU Router          │  │
│  │ (ISO 15765) │  │ (ISO 13400) │  │ (PduR)              │  │
│  │             │  │             │  │                     │  │
│  │ - IsoTp     │  │ - TCP/UDP   │  │ - 路由决策          │  │
│  │ - N_PCI     │  │ - Vehicle   │  │ - 协议切换          │  │
│  │ - FC/CF/SF  │  │   Discovery │  │                     │  │
│  └──────┬──────┘  └──────┬──────┘  └─────────────────────┘  │
└─────────┼────────────────┼───────────────────────────────────┘
          │                │
┌─────────┼────────────────┼───────────────────────────────────┐
│         │    Interface Layer                                          │
│  ┌──────▼──────┐  ┌──────▼──────┐  ┌─────────────────────┐  │
│  │ CanIf       │  │ EthIf       │  │ Socket Adapter      │  │
│  │             │  │             │  │                     │  │
│  │ - CAN FD    │  │ - 100BASE-T1│  │ - BSD Socket        │  │
│  │ - 2.0/FD    │  │ - VLAN      │  │ - Zero-Copy         │  │
│  └──────┬──────┘  └──────┬──────┘  └─────────────────────┘  │
└─────────┼────────────────┼───────────────────────────────────┘
          │                │
          ▼                ▼
    ┌─────────────────────────────────────────────────────┐
    │          Hardware Abstraction (现有)                 │
    │   Aurix GETH / S32G ENET / STM32 MAC                │
    └─────────────────────────────────────────────────────┘
```

### 2.2 模块规格

#### DCM (Diagnostic Communication Manager)

| 服务ID | 服务名称 | 优先级 | 说明 |
|--------|----------|--------|------|
| 0x10 | Diagnostic Session Control | P0 | 默认/扩展/编程会话 |
| 0x11 | ECU Reset | P0 | 硬复位/软复位/快速关闭 |
| 0x22 | Read Data By Identifier | P0 | 读取DID数据 |
| 0x2E | Write Data By Identifier | P1 | 写入DID数据 |
| 0x31 | Routine Control | P1 | 启动/停止/请求例程 |
| 0x34 | Request Download | P1 | 请求下载(OTA) |
| 0x35 | Request Upload | P2 | 请求上传 |
| 0x36 | Transfer Data | P1 | 传输数据块 |
| 0x37 | Request Transfer Exit | P1 | 退出传输 |
| 0x3E | Tester Present | P0 | 保持会话 |
| 0x85 | Control DTC Setting | P2 | DTC控制 |
| 0x86 | Response On Event | P2 | 事件响应 |

#### DEM (Diagnostic Event Manager)

```c
typedef struct {
    uint32_t dtc;                    // SAE J2012格式: 0xPPCCCC
    uint8_t status;                  // ISO 14229-1 DTCStatusMask
    uint8_t severity;                // DTC严重程度
    uint8_t fault_type;              // 故障类型
    uint32_t occurrence_counter;     // 发生计数器
    uint32_t aging_counter;          // 老化计数器
    uint64_t confirmed_timestamp;    // 确认时间戳
    uint64_t cleared_timestamp;      // 清除时间戳
    freeze_frame_t *freeze_frame;    // 快照数据
    uint8_t *extended_data;          // 扩展数据
} dem_dtc_record_t;
```

#### DoIP (ISO 13400-2)

```c
// DoIP报文结构
typedef struct {
    uint8_t protocol_version;        // 0x02 (DoIP ISO 13400-2:2019)
    uint8_t protocol_version_inv;    // 0xFD
    uint16_t payload_type;           // 见下表
    uint32_t payload_length;
    uint8_t payload[];
} doip_msg_t;

// Payload Type定义
#define DOIP_VID_REQUEST              0x0001  // Vehicle Identification Request
#define DOIP_VID_REQUEST_EID          0x0002  // Vehicle ID Request with EID
#define DOIP_VID_REQUEST_VIN          0x0003  // Vehicle ID Request with VIN
#define DOIP_VEHICLE_ANNOUNCEMENT     0x0004  // Vehicle Announcement
#define DOIP_ROUTING_ACTIVATION_REQ   0x0005  // Routing Activation Request
#define DOIP_ROUTING_ACTIVATION_RES   0x0006  // Routing Activation Response
#define DOIP_ALIVE_CHECK_REQ          0x0007  // Alive Check Request
#define DOIP_ALIVE_CHECK_RES          0x0008  // Alive Check Response
#define DOIP_DIAGNOSTIC_MSG           0x8001  // Diagnostic Message (UDS)
#define DOIP_DIAGNOSTIC_ACK           0x8002  // Diagnostic Message Ack
#define DOIP_DIAGNOSTIC_NACK          0x8003  // Diagnostic Message Nack
```

#### IsoTp (ISO 15765-2)

```c
// 寻址模式
typedef enum {
    ISOTP_NORMAL_ADDRESSING = 0,     // 标准寻址 (CAN ID区分ECU)
    ISOTP_EXTENDED_ADDRESSING,       // 扩展寻址 (N_TA包含在首字节)
    ISOTP_NORMAL_FIXED_ADDRESSING,   // 固定正常寻址 (29-bit CAN ID)
    ISOTP_MIXED_ADDRESSING           // 混合寻址 (29-bit + N_TA)
} isotp_addressing_mode_t;

// 协议控制信息
typedef struct {
    uint8_t pci_type;                // 0=SF, 1=FF, 2=CF, 3=FC
    uint8_t sn;                      // Sequence Number (CF)
    uint8_t fs;                      // Flow Status (FC)
    uint8_t bs;                      // Block Size (FC)
    uint8_t stmin;                   // Separation Time Min (FC)
    uint16_t data_len;               // 数据长度
} isotp_pci_t;
```

### 2.3 DDS集成设计

```yaml
# 诊断数据DDS Topic定义
diagnostic_topics:
  DiagnosticSession:
    topic_name: "Vehicle/Diagnostics/Session"
    qos_profile: "reliable_transient_local"
    message_type: "diagnostic_session_t"
    fields:
      - ecu_id: uint16
      - session_type: uint8  # 0x01=default, 0x02=programming, 0x03=extended
      - security_level: uint8
      - tester_address: uint16
      - timestamp: uint64

  DTCStatus:
    topic_name: "Vehicle/Diagnostics/DTC/Status"
    qos_profile: "keep_last_100"
    message_type: "dtc_status_t"
    fields:
      - ecu_id: uint16
      - dtc_count: uint16
      - dtc_list: sequence<dts_record_t>
      - timestamp: uint64

  DiagnosticData:
    topic_name: "Vehicle/Diagnostics/Data/{did}"
    qos_profile: "on_demand"
    message_type: "diagnostic_data_t"
    fields:
      - did: uint16
      - data: sequence<uint8>
      - timestamp: uint64

  DoIPRouting:
    topic_name: "Vehicle/Diagnostics/DoIP/Routing"
    qos_profile: "reliable_volatile"
    message_type: "doip_routing_event_t"
    fields:
      - event_type: uint8  # ACTIVATED/DEACTIVATED/TIMEOUT
      - logical_address: uint16
      - source_address: uint16
      - timestamp: uint64
```

---

## 3. SecOC安全栈 (线路B)

### 3.1 架构设计

```
┌──────────────────────────────────────────────────────────────┐
│              AUTOSAR Crypto Stack (Line B)                   │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌────────────────────────────────────────────────────────┐  │
│  │  SecOC (Secure Onboard Communication)                   │  │
│  │  ─────────────────────────────────────                   │  │
│  │  • Authenticated PDU with Freshness Value               │  │
│  │  • CMAC/AES-128/SHA-256 MAC 计算                        │  │
│  │  • 同步计数器/时间戳 Freshness                            │  │
│  │  • 与PduR集成，透明加解密                                 │  │
│  └────────────────────┬───────────────────────────────────┘  │
│                       │                                       │
│  ┌────────────────────▼───────────────────────────────────┐  │
│  │  CSM (Crypto Services Manager)                          │  │
│  │  ─────────────────────────────────────                   │  │
│  │  • Job-based 异步加密服务接口                             │  │
│  │  • 密钥管理接口 (通过KeyM)                                │  │
│  │  • 算法抽象 (AES/SHA/RSA/ECC/DRBG)                       │  │
│  └────────────────────┬───────────────────────────────────┘  │
│                       │                                       │
│  ┌────────────────────▼───────────────────────────────────┐  │
│  │  CryIf (Crypto Interface)                               │  │
│  │  ─────────────────────────────────────                   │  │
│  │  • HSM/TPM/TEE 硬件抽象层                                │  │
│  │  • 加密驱动路由                                          │  │
│  │  • 支持同步/异步操作                                      │  │
│  └────────────────────┬───────────────────────────────────┘  │
│                       │                                       │
│  ┌────────────────────▼───────────────────────────────────┐  │
│  │  KeyM (Key Manager)                                     │  │
│  │  ─────────────────────────────────────                   │  │
│  │  • 密钥槽管理 (Key Slots)                                 │  │
│  │  • 密钥更新/轮换策略                                       │  │
│  │  • 与DDS Security证书管理集成                              │  │
│  └────────────────────────────────────────────────────────┘  │
│                                                              │
├──────────────────────────────────────────────────────────────┤
│  复用现有模块                                                │
│  • DDS-Security Crypto (AES-GCM) 作为软件加密后端            │
│  • DDS-Security Auth (PKI) 用于证书链验证                    │
│  • RAM ECC / SafeRAM 保护密钥材料                            │
└──────────────────────────────────────────────────────────────┘
```

### 3.2 模块规格

#### SecOC (AUTOSAR SecOC 4.4)

```c
// SecOC配置
typedef struct {
    uint32_t secoc_pdu_id;
    secoc_freshness_type_t freshness_type;  // COUNTER / TIMESTAMP / TRIP
    secoc_auth_algorithm_t auth_algo;       // AES_CMAC / HMAC_SHA256 / GMAC
    uint8_t auth_key_id;
    uint8_t freshness_value_len;            // FV长度 (bits)
    uint8_t mac_len;                        // MAC截断长度 (bits)
    bool use_tx_confirmation;
    uint32_t freshness_counter_max;
    uint32_t freshness_counter_sync_gap;
} secoc_config_t;

// 认证PDU结构 (发送)
// [Original PDU Data | Freshness Value | MAC]

// Freshness Value类型
typedef enum {
    SECOC_FRESHNESS_COUNTER = 0,    // 单调递增计数器
    SECOC_FRESHNESS_TIMESTAMP,       // 基于时间戳
    SECOC_FRESHNESS_TRIP_COUNTER     // 行程计数器
} secoc_freshness_type_t;

// 验证结果
typedef enum {
    SECOC_VERIFICATION_SUCCESS = 0,
    SECOC_VERIFICATION_FAILURE,      // MAC验证失败
    SECOC_FRESHNESS_FAILURE,         // 新鲜度验证失败
    SECOC_REPLAY_DETECTED,           // 重放攻击检测
    SECOC_TIMEOUT                    // 验证超时
} secoc_verify_result_t;
```

#### CSM (Crypto Services Manager)

```c
// CSM Job类型
typedef enum {
    CSM_JOB_ENCRYPT = 0,
    CSM_JOB_DECRYPT,
    CSM_JOB_MAC_GENERATE,
    CSM_JOB_MAC_VERIFY,
    CSM_JOB_SIGNATURE_GENERATE,
    CSM_JOB_SIGNATURE_VERIFY,
    CSM_JOB_HASH,
    CSM_JOB_RANDOM_GENERATE
} csm_job_type_t;

// CSM Job结构
typedef struct {
    csm_job_type_t job_type;
    uint32_t job_id;
    uint8_t key_id;
    csm_algorithm_t algorithm;
    uint8_t *input;
    uint32_t input_len;
    uint8_t *output;
    uint32_t output_max_len;
    uint32_t *output_len;
    csm_callback_t callback;         // 异步回调
    void *user_data;
    csm_job_state_t state;
} csm_job_t;

// 支持的算法
typedef enum {
    CSM_ALGO_AES_128_CBC = 0,
    CSM_ALGO_AES_128_GCM,
    CSM_ALGO_AES_256_CBC,
    CSM_ALGO_AES_256_GCM,
    CSM_ALGO_SHA_256,
    CSM_ALGO_HMAC_SHA_256,
    CSM_ALGO_AES_CMAC,
    CSM_ALGO_RSA_PKCS1_SHA256,
    CSM_ALGO_ECDSA_P256_SHA256
} csm_algorithm_t;
```

#### CryIf (Crypto Interface)

```c
// 加密硬件类型
typedef enum {
    CRYIF_HW_SOFTWARE = 0,           // 软件实现
    CRYIF_HW_HSM,                    // 硬件安全模块 (Aurix HSM, etc.)
    CRYIF_HW_TPM,                    // TPM 2.0
    CRYIF_HW_TEE,                    // ARM TrustZone TEE
    CRYIF_HW_SE                       // 安全元件
} cryif_hw_type_t;

// 驱动接口
typedef struct {
    cryif_hw_type_t hw_type;
    const char *driver_name;
    
    // 初始化
    cryif_status_t (*init)(void);
    cryif_status_t (*deinit)(void);
    
    // 同步操作
    cryif_status_t (*encrypt_sync)(const uint8_t *key, uint32_t key_len,
                                    const uint8_t *plaintext, uint32_t pt_len,
                                    uint8_t *ciphertext, uint32_t *ct_len);
    cryif_status_t (*decrypt_sync)(const uint8_t *key, uint32_t key_len,
                                    const uint8_t *ciphertext, uint32_t ct_len,
                                    uint8_t *plaintext, uint32_t *pt_len);
    cryif_status_t (*mac_sync)(const uint8_t *key, uint32_t key_len,
                                const uint8_t *data, uint32_t data_len,
                                uint8_t *mac, uint32_t *mac_len);
    
    // 异步操作
    cryif_status_t (*submit_job)(csm_job_t *job);
    cryif_status_t (*cancel_job)(uint32_t job_id);
    cryif_status_t (*query_job)(uint32_t job_id, csm_job_state_t *state);
    
    // 密钥管理
    cryif_status_t (*key_import)(uint8_t slot_id, const uint8_t *key, uint32_t key_len);
    cryif_status_t (*key_export)(uint8_t slot_id, uint8_t *key, uint32_t *key_len);
    cryif_status_t (*key_generate)(uint8_t slot_id, cryif_key_type_t type);
    cryif_status_t (*key_erase)(uint8_t slot_id);
} cryif_driver_t;
```

#### KeyM (Key Manager)

```c
// 密钥槽定义
typedef struct {
    uint8_t slot_id;
    keym_key_type_t key_type;
    uint32_t key_len;
    uint32_t key_version;
    bool is_persistent;
    bool is_exportable;
    uint64_t valid_from;
    uint64_t valid_until;
    keym_key_usage_t allowed_usage;
} keym_slot_info_t;

// 密钥类型
typedef enum {
    KEYM_TYPE_AES_128 = 0,
    KEYM_TYPE_AES_256,
    KEYM_TYPE_HMAC_SHA256,
    KEYM_TYPE_RSA_2048,
    KEYM_TYPE_ECC_P256,
    KEYM_TYPE_DERIVED              // 派生密钥
} keym_key_type_t;

// 密钥使用场景
typedef enum {
    KEYM_USAGE_SECOC = 0x01,        // SecOC认证
    KEYM_USAGE_DTLS = 0x02,         // DTLS/TLS
    KEYM_USAGE_DDS_SECURITY = 0x04, // DDS-Security
    KEYM_USAGE_STORAGE = 0x08,      // 安全存储
    KEYM_USAGE_DEBUG = 0x10         // 调试认证
} keym_key_usage_t;

// 密钥派生
typedef struct {
    uint8_t parent_slot_id;
    uint8_t target_slot_id;
    keym_kdf_type_t kdf_type;       // HKDF / NIST KDF
    uint8_t *context;
    uint32_t context_len;
    uint8_t *label;
    uint32_t label_len;
} keym_derivation_params_t;
```

### 3.3 DDS-Security集成

```c
// 共享证书/密钥仓库
typedef struct {
    // DDS-Security证书
    dds_security_cert_t dds_cert;
    
    // SecOC密钥引用
    uint8_t secoc_key_slot[SECOC_MAX_KEYS];
    
    // KeyM密钥槽映射
    keym_slot_info_t key_slots[KEYM_MAX_SLOTS];
    
    // 统一密钥更新接口
    dds_status_t (*rotate_keys)(void);
    dds_status_t (*revoke_cert)(const char *cert_id);
} unified_security_context_t;
```

---

## 4. 并行开发计划

### 4.1 Agent分工

```
Agent A1: UDS Core
├── DCM服务实现 (0x10, 0x11, 0x22, 0x2E, 0x31)
├── DEM DTC管理
└── 依赖: Agent A2 (传输层)

Agent A2: DoCAN/DoIP Transport
├── IsoTp协议栈
├── DoIP协议栈
└── PduR路由

Agent B1: SecOC
├── SecOC PDU处理
├── Freshness管理
└── 与DDS Security集成

Agent B2: CSM/CryIf/KeyM
├── CSM Job管理
├── HSM抽象接口
└── 密钥生命周期

Agent Coord: 架构集成
├── 统一接口设计
├── 测试框架
└── 文档生成
```

### 4.2 6周里程碑

| 周 | Agent A (UDS) | Agent B (SecOC) | 集成点 |
|---|---|---|---|
| 1 | DCM框架 + 0x10/0x11 | CSM核心 + CryIf接口 | 接口定义冻结 |
| 2 | DEM DTC管理 | SecOC PDU处理 | 密钥管理共享 |
| 3 | DoCAN/IsoTp | Freshness同步 | HSM驱动适配 |
| 4 | DoIP实现 | DDS-SecOC集成 | 测试覆盖 |
| 5 | PduR集成 | KeyM密钥轮换 | 端到端验证 |
| 6 | 测试 + 优化 | 测试 + 优化 | 文档完善 |

---

## 5. OTA规范规划

### 5.1 OTA架构 (待详细设计)

```
┌─────────────────────────────────────────────────────────────┐
│                   OTA Master (云端/车机)                     │
├─────────────────────────────────────────────────────────────┤
│  • UDS 0x34/0x36/0x37 传输协议                              │
│  • Delta更新/压缩/断点续传                                   │
│  • A/B分区切换策略                                          │
│  • 回滚机制                                                 │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────▼───────────────────────────────┐
│                 OTA Agent (车内网关)                         │
│  • 更新包下载/缓存/验证                                      │
│  • 分区管理 (Bootloader + App + Cal)                        │
│  • UDS诊断转发至目标ECU                                      │
└─────────────────────────────────────────────────────────────┘
```

### 5.2 依赖关系

- **UDS 0x34/35/36/37**: 基础诊断传输服务
- **SecOC**: 安全启动验证 (Secure Boot)
- **DEM**: 更新失败DTC记录
- **KeyM**: 固件签名验证密钥管理

---

## 6. 文件结构

```
src/
├── diagnostics/                  # 线路A: UDS诊断
│   ├── CMakeLists.txt
│   ├── dcm/
│   │   ├── dcm_core.h/c
│   │   ├── dcm_services.h/c
│   │   └── dcm_session.h/c
│   ├── dem/
│   │   ├── dem_core.h/c
│   │   ├── dem_dtc.h/c
│   │   └── dem_event.h/c
│   ├── docan/
│   │   ├── isotp.h/c
│   │   ├── isotp_canif.h/c
│   │   └── docan_pdurouter.h/c
│   └── doip/
│       ├── doip_core.h/c
│       ├── doip_vd.h/c           # Vehicle Discovery
│       ├── doip_routing.h/c
│       └── doip_diagnostic.h/c
│
├── crypto_stack/                 # 线路B: 安全栈
│   ├── CMakeLists.txt
│   ├── secoc/
│   │   ├── secoc_core.h/c
│   │   ├── secoc_freshness.h/c
│   │   ├── secoc_verify.h/c
│   │   └── secoc_pdurouter.h/c
│   ├── csm/
│   │   ├── csm_core.h/c
│   │   ├── csm_jobs.h/c
│   │   └── csm_callback.h/c
│   ├── cryif/
│   │   ├── cryif_core.h/c
│   │   ├── cryif_hsm.h/c         # HSM驱动接口
│   │   └── cryif_tee.h/c         # TEE驱动接口
│   └── keym/
│       ├── keym_core.h/c
│       ├── keym_slots.h/c
│       ├── keym_derivation.h/c
│       └── keym_rotation.h/c
│
├── ota/                          # 线路C: OTA (后续)
│   └── (待详细设计)
│
└── interfaces/                   # 共享接口
    ├── pdur_interface.h
    ├── crypto_interface.h
    └── diagnostic_types.h
```

---

## 7. 验收标准

### 7.1 UDS诊断栈

- [ ] 通过ISO 15765-2一致性测试
- [ ] 支持CAN FD (64字节) 诊断
- [ ] DoIP Vehicle Discovery正常工作
- [ ] DTC管理符合OBD-II/SAE J1979
- [ ] 诊断数据可通过DDS订阅

### 7.2 SecOC安全栈

- [ ] SecOC MAC验证通过率 > 99.99%
- [ ] 支持新鲜度同步 (Master-Slave模式)
- [ ] HSM硬件加速接口正常
- [ ] 密钥轮换不影响通信
- [ ] 与DDS Security证书共享正常

### 7.3 集成测试

- [ ] SecOC保护的诊断报文传输
- [ ] UDS固件更新 + 安全启动验证
- [ ] 诊断DTC上报至DDS Topic
- [ ] HSM故障自动降级至软件加密

---

## 8. 附录

### 参考规范

- ISO 14229-1:2020 (UDS)
- ISO 15765-2:2016 (DoCAN/IsoTp)
- ISO 13400-2:2019 (DoIP)
- AUTOSAR SecOC SWS 4.4.0
- AUTOSAR CSM SWS 4.4.0
- AUTOSAR CryIf SWS 4.4.0
- AUTOSAR KeyM SWS 4.4.0
- ISO/SAE 21434:2021 (汽车网络安全)

### 相关文档

- `src/dds/security/README.md` - DDS-Security实现
- `src/safety/saferam/README.md` - SafeRAM内存保护
- `src/ethernet/DRIVER_DESIGN.md` - 以太网驱动设计

---

**批准**: 用户确认  
**下一步**: 调用writing-plans创建详细实施计划
