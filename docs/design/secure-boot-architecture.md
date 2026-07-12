# Secure Boot Architecture — yuleASR

> **文档版本**: v1.0  
> **适用平台**: S32K312 (Cortex-M7) + HSM  
> **密码学**: ECDSA P-256 + SHA-256  
> **设计目标**: 量产级、快速移植、模块化

---

## 1. 信任链模型

```
BootROM (HSM ROM)
  │  HSM 硬件验证 PBL 签名 (不可绕过)
  ▼
PBL — Primary Bootloader (~4KB, 不可更新)
  │  Reset_Handler → 最小初始化 → 验证 SBL → 跳转
  ▼
SBL — Secondary Bootloader (~64KB, 可通过 UDS 更新)
  │  OS 启动 → 外设初始化 → 验证 Application → 跳转
  ▼
Application (yuleASR BSW + ASW)
  │  运行时完整性校验 (WdgM/Safety)
  ▼
Runtime Monitoring (Crypto Watchdog + 安全监控)
```

### 每级验证责任

| 阶段 | 验证内容 | 验证方式 | 失败处理 |
|------|----------|----------|----------|
| BootROM → PBL | PBL 签名 | HSM PEM 公钥 | 锁定芯片 |
| PBL → SBL | SBL 签名 + 版本号 | 软件 ECDSA | 进入恢复模式 |
| SBL → App | App 签名 + 版本号 | 软件 ECDSA | 回滚或恢复模式 |
| Runtime | 关键段 CRC | WdgM 周期性校验 | 安全复位 |

---

## 2. 固件映像格式

### Flash 布局

```
0x0000_0000 ┌─────────────────────────┐
            │ PBL (Primary Bootloader) │  4KB
0x0000_1000 ├─────────────────────────┤
            │  [预留]                   │
0x0000_2000 ├─────────────────────────┤
            │ SBL Header              │
            │ SBL Payload             │
            │ SBL Signature + Trailer │
0x0001_2000 ├─────────────────────────┤
            │ Application Slot A      │
            │  (Header + Payload + Sig)│
0x0010_0000 ├─────────────────────────┤
            │ Application Slot B      │
            │  (Header + Payload + Sig)│
0x001E_0000 ├─────────────────────────┤
            │ Boot Info Block         │
0x001F_0000 ├─────────────────────────┤
            │ NVM / EEPROM Emul       │  32KB
0x001F_8000 └─────────────────────────┘
```

### Image Header (64 bytes)

```c
typedef struct {
    uint32_t magic;              // 'YBL1' (0x314C4259)
    uint32_t header_crc;         // 头校验
    uint32_t image_type;         // 0x01=SBL, 0x02=App
    uint32_t version;            // 语义版本号 (防回滚)
    uint32_t payload_size;       // 有效载荷字节数
    uint8_t  hash[32];           // SHA-256(payload)
    uint8_t  reserved[12];
} Boot_ImageHeader;
```

### Image Trailer (128 bytes)

```c
typedef struct {
    uint8_t  signature[64];      // ECDSA P-256 签名
    uint32_t signature_algo;     // 0x01=ECDSA_P256
    uint32_t signing_time;       // 签名时间戳 (可选)
    uint8_t  reserved[56];
} Boot_ImageTrailer;
```

---

## 3. 密钥架构

```
┌──────────────────────────────────────────────────┐
│ HSM Key Store (硬件隔离, 软件不可读)               │
│                                                    │
│  Key Slot 0: PBL 公钥    (PEM → HSM 注入)          │
│  Key Slot 1: HSM 制造商密钥 (熔丝, 不可更改)         │
└──────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────┐
│ Software (代码中 DER 编码)                         │
│                                                    │
│  SBL 公钥: 编译进 PBL (Boot_Verify.c)               │
│  App 公钥: 编译进 SBL (Boot_Verify.c)               │
└──────────────────────────────────────────────────┘
```

### 密钥层级

| 密钥 | 存储位置 | 用途 | 更换方式 |
|------|----------|------|----------|
| HSM Root Key | HSM 熔丝 | 验证 PBL 签名 | 芯片生命周期内不可更换 |
| PBL Public Key | HSM Key Slot 0 | 验证 SBL 签名 | OTP, 生产时烧录 |
| SBL Public Key | PBL 固件中 | 验证 App 签名 | 随 PBL 更新 |
| App Public Key | SBL 固件中 | 运行时完整性 | 随 SBL 更新 |

---

## 4. 防回滚机制

```
Boot Info Block (BIB) — 位于 flash 末尾
┌───────────────────────────┐
│ magic         'BIB0'      │
│ pbl_version   1           │
│ sbl_version   3           │
│ app_version   12          │
│ boot_count    42          │
│ max_boot_attempts 5       │
│ status        0x01        │
│ anti_rollback_counter 12  │
│ reserved                 │
│ crc32                    │
└───────────────────────────┘
```

- `anti_rollback_counter` 单调递增，刷旧版本时拒绝
- `boot_count` 每启动 +1，正常确认后清零
- 连续 5 次未确认触发回滚

---

## 5. 固件更新流程 (UDS)

```
Tester                      ECU (SBL)
  │                            │
  │  0x10 0x02 (Programming)   │
  │───────────────────────────>│
  │  0x27 (Security Access)    │
  │───────────────────────────>│
  │  0x34 (RequestDownload)    │  → 声明写入 Slot B
  │───────────────────────────>│
  │  0x36 (TransferData) × N   │  → 逐块写入, 边写边哈希
  │═══════════════════════════>│
  │  0x37 (RequestTransferExit)│
  │───────────────────────────>│
  │  (SBL 验证 Slot B 签名)     │  ← 关键验证点
  │  0x31 0xFF 完整性检查      │
  │───────────────────────────>│
  │  0x11 0x01 (ECUReset)      │  → 重启 → PBL → SBL → Slot B
  │───────────────────────────>│
```

---

## 6. 可移植性设计

| 抽象层 | 平台相关代码 | 通用代码 | 新平台移植 |
|--------|-------------|----------|-----------|
| Boot_Flash | Erase/Write/Read | 校验、重试 | 替换 3 个函数 |
| Boot_Hsm | Init/Sign/Verify | Key slot 管理 | 替换 4 个函数 |
| Boot_Verify | (纯软件) | ECDSA + SHA-256 | 0, 直接复用 |
| Boot_Loader | 启动地址、栈指针 | 状态机 | 替换 2 个宏 |
