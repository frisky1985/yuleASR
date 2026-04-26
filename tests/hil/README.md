# HIL测试框架 (Hardware-in-the-Loop Test Framework)

基于硬件在环的端到端测试框架，支持UDS诊断、E2E保护、OTA升级和SecOC安全测试。

## 目录

- [HIL测试框架 (Hardware-in-the-Loop Test Framework)](#hil测试框架-hardware-in-the-loop-test-framework)
  - [目录](#目录)
  - [特性](#特性)
  - [架构](#架构)
  - [快速开始](#快速开始)
    - [安装依赖](#安装依赖)
    - [运行测试](#运行测试)
  - [硬件支持](#硬件支持)
  - [模拟模式](#模拟模式)
  - [测试序列](#测试序列)
    - [UDS测试序列](#uds测试序列)
    - [E2E测试序列](#e2e测试序列)
    - [OTA测试序列](#ota测试序列)
    - [SecOC测试序列](#secoc测试序列)
  - [测试报告](#测试报告)
  - [CI/CD集成](#cicd集成)
  - [目录结构](#目录结构)
  - [配置](#配置)
  - [开发指南](#开发指南)
  - [参考标准](#参考标准)
  - [许可证](#许可证)

## 特性

- **硬件抽象层**: 支持PCAN/Vector/Kvaser CAN接口、Raw Socket以太网
- **模拟模式**: 无需硬件即可运行测试 (POSIX环境兼容)
- **测试序列**: UDS诊断、E2E保护、OTA流程、SecOC安全
- **报告生成**: HTML/PDF/JSON/JUnit XML/Markdown
- **CI集成**: GitHub Actions工作流支持

## 架构

```
+---------------------------------------------------+
|                  HIL Test Framework                |
+---------------------------------------------------+
|  Python Host Side    |      C Hardware Abstraction |
+----------------------+-----------------------------+
|  - CAN Interface     |      - CAN (PCAN/Vector)    |
|  - Ethernet (Scapy)  |      - Ethernet (Raw Sock)  |
|  - UDS Client        |      - Simulation Mode      |
|  - DoIP Client       |                             |
+----------------------+-----------------------------+
|                  Test Sequences                    |
+---------------------------------------------------+
|  UDS Tests  |  E2E Tests  |  OTA Tests  |  SecOC   |
+---------------------------------------------------+
```

## 快速开始

### 安装依赖

```bash
# Python依赖
pip install python-can scapy pytest jinja2 weasyprint

# Linux CAN工具 (如需硬件测试)
sudo apt-get install can-utils

# C编译工具
sudo apt-get install build-essential cmake
```

### 运行测试

```bash
# 进入HIL测试目录
cd tests/hil

# 运行所有测试序列
python -m pytest test_sequences/ -v

# 运行特定测试
python test_sequences/uds_sequences.py
python test_sequences/e2e_sequences.py
python test_sequences/ota_sequences.py
python test_sequences/secoc_sequences.py

# 生成报告
python report_generator.py
```

## 硬件支持

### CAN接口

| 接口类型 | 支持状态 | 说明 |
|---------|---------|------|
| PCAN | ✅ 完全支持 | PEAK PCAN-USB |
| Vector | ✅ 完全支持 | Vector VN1610/VN1630 |
| Kvaser | ✅ 完全支持 | Kvaser LeafLight |
| SocketCAN | ✅ 完全支持 | Linux内核CAN |

### 以太网接口

| 接口类型 | 支持状态 | 说明 |
|---------|---------|------|
| Raw Socket | ✅ 支持 | Linux原始套接字 |
| pcap | ✅ 支持 | libpcap捕获 |
| DoIP | ✅ 支持 | ISO 13400-2 |

## 模拟模式

在没有硬件的情况下，框架可以在模拟模式下运行:

```python
from hil_host import create_hil_manager

# 创建模拟模式管理器
manager = create_hil_manager(simulation_mode=True)

# 设置模拟参数 (HIL接口函数)
from hil_host import hil_sim_set_latency, hil_sim_set_packet_loss_rate

hil_sim_set_latency(10)  # 10ms延迟
hil_sim_set_packet_loss_rate(0.01)  # 1%丢包率
```

## 测试序列

### UDS测试序列

基于ISO 14229-1标准的诊断测试:

| 测试项 | 说明 |
|--------|------|
| Session Control | 会话切换 (Default/Extended/Programming) |
| ECU Reset | 硬复位/软复位 |
| Security Access | Seed-Key机制、错误尝试次数 |
| Data Read/Write | VIN读取、DID读写 |
| DTC | 故障码读取/清除 |
| Negative Response | 缺省响应码验证 |

### E2E测试序列

基于AUTOSAR E2E R22-11的端到端保护测试:

| Profile | 说明 | 测试覆盖 |
|---------|------|---------|
| P01 | CRC8 | ✅ CRC计算/验证/错误检测 |
| P02 | CRC8+Counter | ✅ 计数器同步/状态机 |
| P04 | CRC32 | ✅ 大数据保护 |
| P05 | CRC16 | ✅ 标准CRC16 |
| P06 | CRC16+Counter | ✅ 完整保护 |
| P07 | CRC32+Counter | ✅ 安全关键数据 |
| P11 | Dynamic CRC8 | ✅ 动态长度 |
| P22 | Dynamic CRC16 | ✅ 大数据 (>4KB) |

### OTA测试序列

基于UNECE R156/ISO 24089的OTA测试:

| 阶段 | 测试项 |
|-----|--------|
| Campaign | 更新活动接收、VIN验证、ECU兼容性 |
| Download | 下载流程、断点续传、丢包处理 |
| Verify | 哈希验证、签名验证、版本检查 |
| Install | 安装流程、进度监控、错误回滚 |
| Activate | 激活流程、启动验证 |
| Rollback | 回滚机制、版本恢复 |

### SecOC测试序列

基于AUTOSAR SecOC 4.4的安全通信测试:

| 测试项 | 说明 |
|--------|------|
| MAC计算 | AES-CMAC/HMAC-SHA验证 |
| MAC验证 | 正确/错误密钥测试 |
| Freshness管理 | 计数器管理、同步机制 |
| 重放攻击检测 | 重复消息检测、序列验证 |
| 密钥管理 | 密钥导入、更新、无效化 |
| 多PDU支持 | 多路独立保护 |

## 测试报告

支持多种报告格式:

```bash
# 生成所有格式报告
python report_generator.py

# 使用API生成报告
from report_generator import ReportGenerator

generator = ReportGenerator(output_dir="reports")
report = generator.generate_report(test_suites, ...)

# 各种格式
generator.generate_html(report)
generator.generate_pdf(report)
generator.generate_json(report)
generator.generate_junit_xml(report)  # 用于CI集成
generator.generate_markdown(report)
```

报告包含:
- 测试摘要 (通过率、执行时间)
- 代码覆盖率 (行、分支、函数)
- 性能指标 (延迟、吞吐量)
- 详细测试结果
- 环境信息

## CI/CD集成

GitHub Actions工作流 (`hil-tests.yml`):

```yaml
# 自动触发
on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]
  schedule:
    - cron: '0 2 * * *'  # 每日凌晨2点

# 测试任务
jobs:
  hil-simulation-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Run HIL Tests
        run: python -m pytest tests/hil/test_sequences/ -v
```

CI功能:
- 每次推送/合并自动运行测试
- 夜间定时测试
- 测试报告自动上传到GitHub Pages
- 代码覆盖率报告

## 目录结构

```
tests/hil/
├── hil_interface.h          # HIL硬件抽象层头文件
├── hil_interface.c          # HIL硬件抽象层实现
├── hil_host.py              # 主机端Python框架
├── report_generator.py      # 报告生成器
├── test_sequences/          # 测试序列
│   ├── uds_sequences.py     # UDS测试
│   ├── e2e_sequences.py     # E2E测试
│   ├── ota_sequences.py     # OTA测试
│   └── secoc_sequences.py   # SecOC测试
├── reports/                 # 测试报告输出
├── CMakeLists.txt          # CMake配置
└── README.md               # 本文档
```

## 配置

### HIL配置

```c
// hil_interface.h
#define HIL_MAX_CAN_INTERFACES      4
#define HIL_MAX_ETH_INTERFACES      2
#define HIL_MAX_MESSAGE_SIZE        4096
#define HIL_DEFAULT_TIMEOUT_MS      1000
```

### UDS配置

```python
# uds_sequences.py
UDS_TX_ID = 0x7E0
UDS_RX_ID = 0x7E8
P2_SERVER_MAX_MS = 50
P2_STAR_SERVER_MAX_MS = 5000
```

### E2E配置

```python
# e2e_sequences.py
E2E_P22_MAX_DATA_LENGTH = 4096
E2E_P22_MIN_DATA_LENGTH = 16
```

## 开发指南

### 添加新测试

1. 在对应的sequences文件中添加测试函数:

```python
def test_my_feature(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: My Feature
    
    Steps:
    1. Step 1
    2. Step 2
    3. Verify result
    """
    # 测试逻辑
    return {'passed': True, 'details': ...}
```

2. 添加到测试序列:

```python
def get_uds_full_test_sequence():
    return [
        ...
        (test_my_feature, "UDS_My_Feature", (), {}),
    ]
```

3. 运行测试:

```bash
python test_sequences/uds_sequences.py
```

### 添加新硬件接口

1. 在`hil_interface.h`中添加接口类型:

```c
typedef enum {
    ...
    HIL_IF_TYPE_CAN_NEW_INTERFACE,
} hil_interface_type_t;
```

2. 在`hil_interface.c`中实现接口函数

## 参考标准

- **UDS**: ISO 14229-1:2020 Unified diagnostic services (UDS)
- **E2E**: AUTOSAR E2E R22-11 End-to-End Protection
- **OTA**: UNECE R156 / ISO 24089 Software update and software update management system
- **SecOC**: AUTOSAR SecOC 4.4 Secure Onboard Communication
- **DoIP**: ISO 13400-2 Diagnostic communication over Internet Protocol

## 许可证

MIT License - 详见项目根目录LICENSE文件

---

**作者**: HIL Test Framework Team  
**版本**: 1.0.0  
**更新日期**: 2026-04-26
