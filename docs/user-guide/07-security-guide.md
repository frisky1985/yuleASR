# 第7章: 安全指南

**目标**: 详细介绍密钥管理和证书配置。

---

## 7.1 安全架构

### 7.1.1 DDS-Security层次

```
┌─────────────────────────────────────────────────────────────────┐
│                    DDS-Security Architecture                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                         │
│  Application Layer                                                      │
│       │                                                                 │
│       │ DomainParticipant                                                 │
│       │       │                                                         │
│       │       v                                                         │
│       │  ┌─────────────────────────────────────────────────────────────────┐   │
│       │  │          Authentication Service                             │   │
│       │  │              │                                              │   │
│       │  │              v                                              │   │
│       │  │  Handshake ──────────────────────────────────────┾  │   │
│       │  │  │           │                                              │   │
│       │  │  v           v                                              │   │
│       │  │  Access Control ─────────────────────────────────────┾  │   │
│       │  │  │           │                                              │   │
│       │  │  v           v                                              │   │
│       │  │  Cryptographic Service ─────────────────────────────────────┾  │   │
│       │  │              │                                              │   │
│       │  │              v                                              │   │
│       │  │  Logging/Monitoring ─────────────────────────────────────┾  │   │
│       │  └─────────────────────────────────────────────────────────────────┘   │
│       │                                                                 │
│       v                                                                 │
│  RTPS Layer                                                           │
│                                                                         │
└─────────────────────────────────────────────────────────────────┘
```

## 7.2 PKI证书管理

### 7.2.1 证书层次结构

```
┌─────────────────────────────────────────────────────────────────┐
│                      PKI Certificate Hierarchy                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                         │
│                    ┌─────────────────────────────────┐                        │
│                    │       Root CA                      │                        │
│                    │  (Self-signed, Offline)          │                        │
│                    └──────────┐──────────────────────┘                        │
│                              │                                          │
│                    ┌────────┴────────────────────────┐                        │
│                    │    Intermediate CA               │                        │
│                    │   (Online, HSM-protected)        │                        │
│                    └────────┐────────────────────────┘                        │
│                              │                                          │
│           ┌────────────┴───────────────┴────────────┐                    │
│           │         │                  │                    │                    │
│    ┌────────┐  ┌────────┐         ┌────────┐             ┌────────┐           │
│    │Vehicle│  │Vehicle│         │Vehicle│             │Vehicle│           │
│    │  ECU 1│  │  ECU 2│         │  ECU 3│             │  ECU N│           │
│    │  Cert│  │  Cert│         │  Cert│             │  Cert│           │
│    └────────┘  └────────┘         └────────┘             └────────┘           │
│                                                                         │
└─────────────────────────────────────────────────────────────────┘
```

### 7.2.2 创建CA证书

```bash
#!/bin/bash
# create_ca.sh - 创建证书颁发机构

# 创建目录
mkdir -p ca/{root,intermediate,certs,crl,newcerts,private}
touch ca/index.txt
echo 1000 > ca/serial

# 创建Root CA私钥
openssl genrsa -aes256 -out ca/private/ca.key.pem 4096
chmod 400 ca/private/ca.key.pem

# 创建Root CA证书
openssl req -config openssl_root.cnf \
    -key ca/private/ca.key.pem \
    -new -x509 -days 7300 -sha256 -extensions v3_ca \
    -out ca/certs/ca.cert.pem
chmod 444 ca/certs/ca.cert.pem

# 创建Intermediate CA私钥
openssl genrsa -aes256 -out ca/private/intermediate.key.pem 4096

# 创建Intermediate CA CSR
openssl req -config openssl_intermediate.cnf \
    -key ca/private/intermediate.key.pem \
    -new -sha256 \
    -out ca/csr/intermediate.csr.pem

# 使用Root CA签署Intermediate CA
openssl ca -config openssl_root.cnf \
    -extensions v3_intermediate_ca \
    -days 3650 -notext -md sha256 \
    -in ca/csr/intermediate.csr.pem \
    -out ca/certs/intermediate.cert.pem
```

### 7.2.3 创建车载证书

```bash
#!/bin/bash
# create_vehicle_cert.sh - 创建车载ECU证书

VEHICLE_ID="$1"
ECU_NAME="$2"

# 创建ECU私钥
openssl genrsa -out "certs/${ECU_NAME}.key.pem" 2048

# 创建CSR
openssl req -config openssl_vehicle.cnf \
    -key "certs/${ECU_NAME}.key.pem" \
    -new -sha256 \
    -out "csr/${ECU_NAME}.csr.pem" \
    -subj "/C=CN/O=VehicleManufacturer/OU=${VEHICLE_ID}/CN=${ECU_NAME}"

# 签署证书
openssl ca -config openssl_intermediate.cnf \
    -extensions vehicle_ecu \
    -days 1095 -notext -md sha256 \
    -in "csr/${ECU_NAME}.csr.pem" \
    -out "certs/${ECU_NAME}.cert.pem"

# 创建证书链
cat certs/${ECU_NAME}.cert.pem \
    intermediate/certs/intermediate.cert.pem \
    ca/certs/ca.cert.pem > certs/${ECU_NAME}.chain.pem
```

## 7.3 密钥管理

### 7.3.1 HSM集成

```c
/**
 * @brief 初始化HSM
 * @param[in] config HSM配置
 * @return 成功返回0
 */
int hsm_init(const HsmConfig *config);

/**
 * @brief 生成密钥对
 * @param[in] algorithm 算法 (RSA, ECC)
 * @param[in] key_size 密钥长度
 * @param[out] key_handle 密钥句柄
 * @return 成功返回0
 */
int hsm_generate_keypair(
    HsmAlgorithm algorithm,
    uint32_t key_size,
    HsmKeyHandle *key_handle);

/**
 * @brief 使用HSM签名
 * @param[in] key_handle 密钥句柄
 * @param[in] data 数据
 * @param[in] data_len 数据长度
 * @param[out] signature 签名缓冲区
 * @param[in,out] sig_len 签名长度指针
 * @return 成功返回0
 */
int hsm_sign(
    HsmKeyHandle key_handle,
    const uint8_t *data,
    size_t data_len,
    uint8_t *signature,
    size_t *sig_len);
```

### 7.3.2 密钥滚动

```bash
#!/bin/bash
# key_rotation.sh - 密钥滚动脚本

CURRENT_KEY_VERSION=$(cat /etc/dds/key_version)
NEW_KEY_VERSION=$((CURRENT_KEY_VERSION + 1))

echo "Rotating keys from version $CURRENT_KEY_VERSION to $NEW_KEY_VERSION"

# 生成新密钥
openssl genrsa -out "/etc/dds/keys/session_key_v${NEW_KEY_VERSION}.pem" 256

# 密钥协商（使用公钥加密传输）
for peer in $(cat /etc/dds/peers); do
    # 获取对端公钥
    peer_key=$(dds_get_peer_key "$peer")
    
    # 加密新密钥
    openssl pkeyutl -encrypt \
        -in "/etc/dds/keys/session_key_v${NEW_KEY_VERSION}.pem" \
        -pubin -inkey <(echo "$peer_key") \
        -out "/tmp/key_v${NEW_KEY_VERSION}_for_${peer}.bin"
    
    # 发送加密密钥
    dds_send_key "$peer" "/tmp/key_v${NEW_KEY_VERSION}_for_${peer}.bin"
done

# 更新版本号
echo "$NEW_KEY_VERSION" > /etc/dds/key_version

# 清理旧密钥
rm -f "/etc/dds/keys/session_key_v${CURRENT_KEY_VERSION}.pem"

echo "Key rotation completed"
```

## 7.4 访问控制

### 7.4.1 权限配置文件

```xml
<?xml version="1.0" encoding="UTF-8"?>
<permissions xmlns="http://www.omg.org/dds-security/">
    <grant name="VehicleGateway">
        <!-- 主体证书 -->
        <subject_name>CN=VehicleGateway,OU=Vehicle001,O=Manufacturer,C=CN</subject_name>
        
        <!-- 有效期 -->
        <validity>
            <not_before>2026-01-01T00:00:00</not_before>
            <not_after>2027-01-01T00:00:00</not_after>
        </validity>
        
        <!-- 允许的DDS域 -->
        <allow_rule>
            <domains>
                <id>0</id>
                <id>1</id>
            </domains>
            
            <!-- 发布权限 -->
            <publish>
                <topic>
                    <name>VehicleStatus</name>
                    <type>VehicleStatusData</type>
                </topic>
                <topic>
                    <name>SensorData/*</name>
                    <type>SensorData</type>
                </topic>
            </publish>
            
            <!-- 订阅权限 -->
            <subscribe>
                <topic>
                    <name>ControlCommand</name>
                    <type>CommandData</type>
                </topic>
            </subscribe>
        </allow_rule>
        
        <!-- 默认拒绝 -->
        <default>DENY</default>
    </grant>
</permissions>
```

### 7.4.2 SecOC密钥配置

```xml
<secoc_config>
    <keys>
        <key id="0x01" name="VehicleBusKey">
            <algorithm>AES-128-CMAC</algorithm>
            <length>16</length>
            <source>HSM</source>
            <slot>1</slot>
        </key>
        <key id="0x02" name="DiagnosticKey">
            <algorithm>AES-128-CMAC</algorithm>
            <length>16</length>
            <source>HSM</source>
            <slot>2</slot>
        </key>
    </keys>
    
    <freshness>
        <algorithm>Counter</algorithm>
        <size>8</size>
        <reset_value>0</reset_value>
        <max_value>0xFFFFFFFF</max_value>
    </freshness>
    
    <authenticators>
        <authenticator id="0x01" key="0x01">
            <truncation>16</truncation>
        </authenticator>
    </authenticators>
</secoc_config>
```

## 7.5 安全审计

### 7.5.1 审计日志配置

```c
// 启用安全审计
dds_security_audit_config_t audit_config = {
    .enabled = true,
    .log_level = DDS_SECURITY_AUDIT_INFO,
    .log_file = "/var/log/dds/security_audit.log",
    .max_file_size = 100 * 1024 * 1024,  // 100MB
    .max_files = 10
};

dds_security_configure_audit(&audit_config);
```

### 7.5.2 审计事件

| 事件类型 | 说明 | 级别 |
|---------|------|------|
| AUTH_HANDSHAKE | 认证握手 | INFO |
| AUTH_FAILURE | 认证失败 | ERROR |
| ACCESS_DENIED | 访问被拒绝 | WARNING |
| ENCRYPTION_FAILURE | 加密失败 | ERROR |
| KEY_ROTATION | 密钥滚动 | INFO |
| CERT_EXPIRED | 证书过期 | WARNING |

## 7.6 安全最佳实践

### 7.6.1 证书管理

- 使用多层次PKI架构
- Root CA离线保存
- Intermediate CA使用HSM保护
- 定期证书审查和更新
- 实施证书吊销列表(CRL)

### 7.6.2 密钥管理

- 密钥存储在HSM中
- 实施密钥滚动机制
- 使用足够长的密钥(256位以上)
- 定期更新会话密钥

### 7.6.3 访问控制

- 实施最小权限原则
- 定期审查权限配置
- 使用细粒度的访问控制(主题/实例级别)

---

**版本**: 2.0.0  
**上一章**: @ref deployment  
**下一章**: 文档主页
