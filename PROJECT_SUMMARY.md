# DDS-ETH Integration Project - 车载场景完整方案

## 项目概览

一个专为汽车电子/车载场景设计的DDS中间件解决方案，完全符合AUTOSAR规范、ISO 26262功能安全和ISO/SAE 21434网络安全要求。

```
代码统计: 41,503行 (C/C++)
模块数量: 50+
测试用例: 150+
文档数量: 20+
```

## 架构分层

```
┌────────────────────────────────────────────────┐
│         应用层 (ASW)                    │
│  ADAS/动力总成/车身控制/座舱电子    │
├────────────────────────────────────────────────┤
│         DDS中间件层                     │
│  Topic/QoS/Security/RTPS/Discovery     │
├────────────────────────────────────────────────┤
│         传输层 (Transport)              │
│  UDP/RTPS | TCP | Shared Memory        │
├────────────────────────────────────────────────┤
│         以太网驱动层                   │
│  100BASE-T1/1000BASE-T1 + TSN         │
├────────────────────────────────────────────────┤
│         硬件层 (MCU/SoC)                │
│  S32G/AURIX/R-Car/TDA4                 │
└────────────────────────────────────────────────┘
```

## 完成的核心模块

### 1. 传输层
- UDP/RTPS传输 - 支持组播单播和TSN
- 共享内存传输 - 无拷贝通信
- 传输管理器 - 多传输层协调

### 2. 以太网驱动
- MAC驱动接口 - 支持S32G/AURIX/R-Car
- DMA环形缓冲区 - 描述符队列管理
- 汽车以太网 - 100BASE-T1/1000BASE-T1 PHY
- 以太网管理器 - 链路监控与故障恢复

### 3. DDS核心
- RTPS发现协议 - SPDP/EDP发现
- RTPS状态机 - Writer/Reader状态管理
- RTPS报文编解码 - CDR序列化
- DDS运行时 - 参与者/发布/订阅管理

### 4. DDS-Security安全
- 身份认证 - PKI-DH插件
- 访问控制 - Permissions插件
- 加密传输 - AES-256-GCM
- 安全管理器 - 统一协调

### 5. 配置工具
- CLI工具 - 命令行配置
- 配置解析/验证器 - YAML/JSON支持
- 安全配置 - PKI/权限配置
- 车载配置 - AUTOSAR/TSN/ASIL
- 代码生成器 - IDL/C/ARXML
- 热加载 - 实时配置更新

### 6. 监控诊断工具
- 流量分析器 - RTPS报文捕获
- QoS监控 - 延迟/吞吐量/丢包统计
- 健康检查 - 节点/主题/连接健康
- 日志系统 - 分级日志/审计日志

## 快速开始

```bash
# 构建项目
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON
make -j$(nproc)

# 运行测试
make test

# 使用配置工具
cd ../dds-config-tool
./dds-config init mysystem
./dds-config validate mysystem.yaml

# 运行监控工具
./tools/dds_monitor -m realtime -i eth0
```

## 车载认证

- AUTOSAR Adaptive/Classic 兼容
- ISO 26262 ASIL-D 安全等级
- ISO/SAE 21434 网络安全
- TSN 802.1AS/Qbv/Qbu/Qci/CB
- DDS-Security v1.1
