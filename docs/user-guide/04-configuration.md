# 第4章: 配置指南

**目标**: 详细介绍XML/JSON配置和工具使用。

---

## 4.1 配置系统概述

ETH-DDS支持多种配置方式:

| 配置方式 | 格式 | 用途 | 工具 |
|---------|------|------|------|
| XML配置 | .xml | 高级配置，完整功能 | 配置编辑器 |
| JSON配置 | .json | 简洁配置，程序读取 | 任意JSON编辑器 |
| 命令行 | CLI | 运行时参数 | dds_config |
| 环境变量 | ENV | 调试/开发 | shell |

## 4.2 XML配置

### 4.2.1 DDS域配置

```xml
<?xml version="1.0" encoding="UTF-8"?>
<dds xmlns="http://www.omg.org/dds/"
     xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
     xsi:schemaLocation="http://www.omg.org/dds/ dds_configuration.xsd">
    
    <!-- 参与者配置 -->
    <domain id="0">
        <participant name="VehicleGateway">
            <!-- QoS策略 -->
            <qos>
                <reliability kind="RELIABLE" max_blocking_time="1s"/>
                <durability kind="TRANSIENT_LOCAL"/>
                <history kind="KEEP_LAST" depth="10"/>
                <deadline period="100ms"/>
                <latency_budget duration="10ms"/>
                <lifespan duration="5s"/>
                <ownership kind="EXCLUSIVE"/>
                <liveliness kind="AUTOMATIC" lease_duration="10s"/>
            </qos>
            
            <!-- 安全配置 -->
            <security enabled="true">
                <identity certificate="certs/vehicle_cert.pem"
                         private_key="certs/vehicle_key.pem"
                         ca_certificate="certs/ca_cert.pem"/>
                <access_control permissions="acl/permissions.xml"/>
                <cryptography algorithm="AES-256-GCM"/>
            </security>
        </participant>
    </domain>
</dds>
```

### 4.2.2 主题配置

```xml
<topic name="VehicleStatus">
    <type>
        <struct name="VehicleStatusData">
            <member name="vehicleId" type="string" max_length="32"/>
            <member name="timestamp" type="uint64"/>
            <member name="speed" type="float32"/>
            <member name="position" type="sequence">
                <element type="float64"/>
            </member>
            <member name="status" type="enum">
                <enumerator name="IDLE" value="0"/>
                <enumerator name="DRIVING" value="1"/>
                <enumerator name="STOPPED" value="2"/>
            </member>
        </struct>
    </type>
    
    <qos>
        <reliability kind="RELIABLE"/>
        <durability kind="TRANSIENT_LOCAL"/>
        <history kind="KEEP_LAST" depth="5"/>
        <deadline period="50ms"/>
    </qos>
</topic>
```

### 4.2.3 安全配置

```xml
<security xmlns="http://www.omg.org/dds-security/">
    <!-- 认证配置 -->
    <authentication plugin="dds.security.auth.pkcs11">
        <pkcs11>
            <library path="/usr/lib/libpkcs11.so"/>
            <slot id="0"/>
            <token label="VehicleHSM"/>
            <pin env="HSM_PIN"/>
            <certificate label="VehicleCert"/>
            <key label="VehicleKey"/>
        </pkcs11>
    </authentication>
    
    <!-- 访问控制 -->
    <access_control plugin="dds.security.access.file">
        <permissions_file path="acl/permissions.xml"/>
        <governance_file path="acl/governance.xml"/>
    </access_control>
    
    <!-- 加密配置 -->
    <cryptography plugin="dds.security.crypto.aes">
        <master_key_algorithm>AES-256-GCM</master_key_algorithm>
        <session_key_rotation interval="3600"/>
    </cryptography>
</security>
```

### 4.2.4 TSN配置

```xml
<tsn_config>
    <!-- 802.1Qbv 增量门控制 -->
    <tas enabled="true">
        <gate_control_list>
            <entry>
                <time_offset>0</time_offset>
                <gate_states>11111111</gate_states>
            </entry>
            <entry>
                <time_offset>500</time_offset>
                <gate_states>11111110</gate_states>
            </entry>
            <entry>
                <time_offset>900</time_offset>
                <gate_states>11111111</gate_states>
            </entry>
        </gate_control_list>
        <cycle_time>1000</cycle_time>
    </tas>
    
    <!-- CBS配置 -->
    <cbs enabled="true">
        <stream id="1">
            <idle_slope>100000</idle_slope>
            <send_slope>-900000</send_slope>
            <hi_credit>10000</hi_credit>
            <lo_credit>-10000</lo_credit>
        </stream>
    </cbs>
    
    <!-- gPTP配置 -->
    <gptp enabled="true">
        <domain number="0"/>
        <priority1>248</priority1>
        <priority2>248</priority2>
        <clock_class>248</clock_class>
        <clock_accuracy>0x20</clock_accuracy>
        <sync_interval>-3</sync_interval>
    </gptp>
</tsn_config>
```

### 4.2.5 UDS诊断配置

```xml
<uds_config>
    <!-- DCM配置 -->
    <dcm>
        <protocol name="UDS_CAN">
            <can>
                <channel id="0" baudrate="500000"/>
                <tx_pdu id="0x7E0"/>
                <rx_pdu id="0x7E8"/>
            </can>
        </protocol>
        
        <services>
            <service id="0x10" name="DiagnosticSessionControl">
                <subfunctions>
                    <subfunction id="0x01" name="DEFAULT"/>
                    <subfunction id="0x02" name="PROGRAMMING"/>
                    <subfunction id="0x03" name="EXTENDED"/>
                </subfunctions>
            </service>
            <service id="0x22" name="ReadDataByIdentifier"/>
            <service id="0x2E" name="WriteDataByIdentifier"/>
            <service id="0x31" name="RoutineControl"/>
            <service id="0x34" name="RequestDownload"/>
            <service id="0x36" name="TransferData"/>
            <service id="0x37" name="RequestTransferExit"/>
        </services>
        
        <security_levels>
            <level id="1" name="Locked"/>
            <level id="2" name="Unlocked"/>
        </security_levels>
    </dcm>
    
    <!-- DEM配置 -->
    <dem>
        <events>
            <event id="0x0001" name="EngineFault">
                <debounce algorithm="COUNTER" fail_threshold="3" pass_threshold="3"/>
                <dtc id="0xP0101"/>
                <monitoring kind="CONTINUOUS"/>
            </event>
            <event id="0x0002" name="TransmissionFault">
                <debounce algorithm="TIME" fail_time="100ms" pass_time="100ms"/>
                <dtc id="0xP0705"/>
                <monitoring kind="CYCLIC" period="100ms"/>
            </event>
        </events>
        
        <dtcs>
            <dtc id="0xP0101" description="Mass Air Flow Circuit"/>
            <dtc id="0xP0705" description="Transmission Range Sensor"/>
        </dtcs>
    </dem>
    
    <!-- DoIP配置 -->
    <doip>
        <vehicle_identification>
            <vin>1HGCM82633A123456</vin>
            <eid>001122334455</eid>
            <gid>66778899AABB</gid>
        </vehicle_identification>
        <logical_address>0x0E00</logical_address>
        <activation_line enabled="true"/>
    </doip>
</uds_config>
```

## 4.3 JSON配置

### 4.3.1 基础配置

```json
{
    "version": "2.0.0",
    "dds": {
        "domain": {
            "id": 0,
            "participant": {
                "name": "VehicleGateway",
                "qos": {
                    "reliability": {
                        "kind": "RELIABLE",
                        "max_blocking_time": "1s"
                    },
                    "durability": "TRANSIENT_LOCAL",
                    "history": {
                        "kind": "KEEP_LAST",
                        "depth": 10
                    },
                    "deadline": "100ms",
                    "latency_budget": "10ms"
                }
            }
        }
    },
    "topics": [
        {
            "name": "VehicleStatus",
            "type": "VehicleStatusData",
            "qos": {
                "reliability": "RELIABLE",
                "durability": "TRANSIENT_LOCAL"
            }
        },
        {
            "name": "SensorData",
            "type": "SensorData",
            "qos": {
                "reliability": "BEST_EFFORT",
                "deadline": "10ms"
            }
        }
    ]
}
```

### 4.3.2 安全配置

```json
{
    "security": {
        "enabled": true,
        "authentication": {
            "plugin": "pkcs11",
            "pkcs11": {
                "library": "/usr/lib/libpkcs11.so",
                "slot": 0,
                "token": "VehicleHSM",
                "pin_env": "HSM_PIN"
            }
        },
        "access_control": {
            "plugin": "file",
            "permissions": "acl/permissions.json",
            "governance": "acl/governance.json"
        },
        "cryptography": {
            "algorithm": "AES-256-GCM",
            "key_rotation_interval": 3600
        }
    }
}
```

### 4.3.3 网络配置

```json
{
    "network": {
        "interfaces": [
            {
                "name": "eth0",
                "type": "ethernet",
                "ip": "192.168.1.100",
                "netmask": "255.255.255.0",
                "multicast": {
                    "enabled": true,
                    "address": "239.255.0.1",
                    "ttl": 1
                }
            }
        ],
        "transport": {
            "udp": {
                "enabled": true,
                "port_base": 7410,
                "port_range": 10
            },
            "shm": {
                "enabled": true,
                "segment_size": "1MB"
            }
        }
    }
}
```

## 4.4 配置工具使用

### 4.4.1 命从l工具

```bash
# 验证XML配置
./tools/dds_config --validate dds_config.xml

# 转换XML到JSON
./tools/dds_config --convert dds_config.xml --output dds_config.json

# 生成C代码
./tools/dds_config --generate-code dds_config.xml --output generated/

# 导出配置报告
./tools/dds_config --report dds_config.xml --output report.html
```

### 4.4.2 配置验证

```bash
# 验证配置有效性
./tools/dds_config --validate config.xml

# 详细输出
./tools/dds_config --validate config.xml --verbose

# 验证特定模块
./tools/dds_config --validate config.xml --module dds
./tools/dds_config --validate config.xml --module security
./tools/dds_config --validate config.xml --module tsn
```

### 4.4.3 配置导出

```bash
# 导出为C头文件
./tools/dds_config --export config.xml --format c --output config.h

# 导出为Python
./tools/dds_config --export config.xml --format python --output config.py

# 导出配置文档
./tools/dds_config --export config.xml --format markdown --output config.md
```

## 4.5 运行时配置

### 4.5.1 环境变量

```bash
# 设置域ID
export DDS_DOMAIN_ID=0

# 设置配置文件路径
export DDS_CONFIG_FILE=/etc/dds/config.xml

# 设置日志级别
export DDS_LOG_LEVEL=INFO  # DEBUG, INFO, WARN, ERROR

# 设置日志输出
export DDS_LOG_FILE=/var/log/dds.log

# 启用调试模式
export DDS_DEBUG=1

# 设置网络接口
export DDS_NETWORK_INTERFACE=eth0
```

### 4.5.2 程序化配置

```c
#include "dds/dds.h"

int main() {
    dds_entity_t participant;
    dds_qos_t *qos;
    
    // 创建自定义QoS
    qos = dds_create_qos();
    
    // 设置可靠性
    dds_qset_reliability(qos, DDS_RELIABILITY_RELIABLE, DDS_SECS(1));
    
    // 设置持久性
    dds_qset_durability(qos, DDS_DURABILITY_TRANSIENT_LOCAL);
    
    // 设置历史
    dds_qset_history(qos, DDS_HISTORY_KEEP_LAST, 10);
    
    // 设置截止时间
    dds_qset_deadline(qos, DDS_MSECS(100));
    
    // 创建参与者
    participant = dds_create_participant(DDS_DOMAIN_DEFAULT, qos, NULL);
    
    // 清理
    dds_delete_qos(qos);
    
    return 0;
}
```

## 4.6 配置最佳实践

### 4.6.1 性能优化

```xml
<!-- 高性能配置 -->
<dds>
    <performance>
        <!-- 内存池配置 -->
        <memory_pool>
            <global_size>64MB</global_size>
            <max_samples>10000</max_samples>
        </memory_pool>
        
        <!-- 缓冲区配置 -->
        <buffers>
            <send_buffer>256KB</send_buffer>
            <receive_buffer>256KB</receive_buffer>
        </buffers>
        
        <!-- 线程配置 -->
        <threads>
            <receive priority="HIGH"/>
            <transmit priority="HIGH"/>
            <discovery priority="NORMAL"/>
        </threads>
    </performance>
</dds>
```

### 4.6.2 安全强化

```xml
<!-- 安全配置最佳实践 -->
<security>
    <!-- 强制认证 -->
    <authentication>
        <require_mutual_auth>true</require_mutual_auth>
        <certificate_validation>STRICT</certificate_validation>
        <revocation_check enabled="true"/>
    </authentication>
    
    <!-- 加密设置 -->
    <encryption>
        <minimum_algorithm>AES-256-GCM</minimum_algorithm>
        <key_rotation enabled="true" interval="3600"/>
        <perfect_forward_secrecy enabled="true"/>
    </encryption>
    
    <!-- 审计日志 -->
    <audit>
        <enabled>true</enabled>
        <log_level>INFO</log_level>
        <log_rotation enabled="true" max_size="100MB" max_files="10"/>
    </audit>
</security>
```

---

**版本**: 2.0.0  
**上一章**: @ref api_reference  
**下一章**: @ref debugging
