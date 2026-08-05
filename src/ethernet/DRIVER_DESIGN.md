# 车载以太网驱动层设计文档

## 概述

本文档描述了eth-dds-integration项目的车载以太网驱动层实现，包括MAC驱动、DMA管理、汽车以太网PHY支持和以太网管理器。

## 架构

```
src/ethernet/
├── eth_manager.h/c          # 以太网管理器 - 链路监控、自动协商、错误统计
├── driver/
│   ├── eth_driver.h           # 原有以太网驱动接口
│   ├── eth_mac_driver.h/c     # MAC驱动 - 抽象MAC层接口
│   ├── eth_dma.h/c            # DMA管理 - 环形描述符队列
│   └── eth_automotive.h/c     # 汽车以太网特定 - PHY驱动、TSN预留
├── stack/                   # 协议栈(如lwIP适配层)
└── api/                     # 应用程序接口
```

## 模块详细说明

### 1. MAC驱动 (eth_mac_driver.h/c)

#### 功能
- 提供抽象的MAC层接口
- 支持多种车载MAC芯片（NXP S32、Infineon AURIX、TI CPSW、Renesas R-Car等）
- 符合AUTOSAR MCAL规范
- 支持功能安全ASIL等级

#### 主要接口
```c
eth_status_t eth_mac_init(const eth_mac_config_t *config);
void eth_mac_deinit(void);
eth_status_t eth_mac_start(void);
eth_status_t eth_mac_stop(void);
eth_status_t eth_mac_transmit(const uint8_t *data, uint32_t len, uint32_t timeout_ms);
eth_status_t eth_mac_receive(uint8_t *buffer, uint32_t max_len, uint32_t *received_len, uint32_t timeout_ms);
```

#### 支持的MAC芯片
| 芯片 | 描述 | 状态 |
|------|------|------|
| Generic | 通用MAC控制器 | 完成(模拟) |
| DW GMAC | Synopsys DesignWare GMAC | 存根 |
| NXP S32 | NXP S32系列 | 存根 |
| Infineon AURIX | AURIX TC3xx | 存根 |
| TI CPSW | Jacinto/TDA4 | 存根 |
| Renesas R-Car | R-Car GbE | 存根 |

### 2. DMA管理 (eth_dma.h/c)

#### 功能
- 环形描述符队列管理
- 收发分离的缓冲区设计
- 支持中断和轮询模式
- 数据对齐和缓存管理

#### 主要特性
- 默认收发描述符数: 16
- 最大描述符数: 256
- 默认缓冲区大小: 1536字节(支持VLAN标签)
- 支持巨型帧: 最大9018字节
- DMA对齐: 4字节

#### 主要接口
```c
eth_status_t eth_dma_init(const eth_dma_config_t *config);
eth_status_t eth_dma_rx_get_packet(eth_dma_rx_packet_t *packet, uint32_t timeout_ms);
eth_status_t eth_dma_tx_queue_packet(const eth_dma_tx_packet_t *packet, uint32_t timeout_ms);
eth_status_t eth_dma_start(void);
eth_status_t eth_dma_stop(void);
```

### 3. 汽车以太网 (eth_automotive.h/c)

#### 功能
- 支持100BASE-T1 (IEEE 802.3bw)
- 支持1000BASE-T1 (IEEE 802.3bp)
- TSN支持预留 (IEEE 802.1AS/Qbv/Qbu/Qci/CBS)
- PHY驱动抽象层
- 诊断和错误统计

#### 支持的PHY芯片

##### 100BASE-T1 PHY
| 芯片 | 厂商 | 描述 | 状态 |
|------|------|------|------|
| TJA1101 | NXP | 100Mbps单端口 | 存根 |
| TJA1102 | NXP | 100Mbps双端口 | 存根 |
| DP83TC811 | TI | 100Mbps汽车以太网PHY | 存根 |

##### 1000BASE-T1 PHY
| 芯片 | 厂商 | 描述 | 状态 |
|------|------|------|------|
| TJA1103 | NXP | 1Gbps汽车以太网PHY | 存根 |
| DP83TG720 | TI | 1Gbps汽车以太网PHY | 存根 |
| 88Q2112 | Marvell | 1Gbps汽车以太网PHY | 存根 |
| BCM89811 | Broadcom | 1Gbps汽车以太网PHY | 存根 |
| RTL9010 | Realtek | 1Gbps汽车以太网PHY | 存根 |

#### TSN预留功能
- IEEE 802.1AS: gPTP时间同步
- IEEE 802.1Qbv: 增量门控制列表
- IEEE 802.1Qbu: 帧预充
- IEEE 802.1Qci: 流过滤
- IEEE 802.1Qav: 信用型带宽
- IEEE 802.1CB: 帧复制和消除

### 4. 以太网管理器 (eth_manager.h/c)

#### 功能
- 链路状态监控
- 自动协商管理
- 错误统计和诊断
- 自动恢复机制
- 吞吐量计算

#### 状态机
```
UNINIT -> INIT -> LINK_DOWN -> LINK_UP
              \          /
               -> ERROR
```

#### 主要接口
```c
eth_status_t eth_manager_init(const eth_manager_config_t *config);
eth_status_t eth_manager_start(void);
eth_status_t eth_manager_stop(void);
eth_status_t eth_manager_check_link(eth_link_status_t *status);
eth_status_t eth_manager_run_diagnostics(automotive_phy_diagnostics_t *diagnostics);
eth_status_t eth_manager_auto_recovery(void);
```

## 符合的汽车标准

### AUTOSAR MCAL规范
- 命名规范符合AUTOSAR标准
- 错误处理符合AUTOSAR风格
- 支持ASIL等级配置

### 功能安全
- 支持ASIL-QM到ASIL-D等级
- 内存保护(ECC/CRC32)
- 安全监控机制

### 电磁兼容(EMC)
- 信号斜率控制
- EMC滤波支持
- 设计符合CISPR 25标准

### 温度范围
- 支持汽车级温度范围: -40°C ~ 125°C
- 高温保护机制

## 使用示例

### 基本初始化
```c
#include "ethernet/eth_manager.h"

void eth_init_example(void)
{
    eth_manager_config_t config = {0};

    /* 配置MAC */
    config.mac_config.mac_addr[0] = 0x00;
    config.mac_config.mac_addr[1] = 0x11;
    config.mac_config.mac_addr[2] = 0x22;
    config.mac_config.mac_addr[3] = 0x33;
    config.mac_config.mac_addr[4] = 0x44;
    config.mac_config.mac_addr[5] = 0x55;
    config.mac_config.mac_type = ETH_MAC_TYPE_GENERIC;
    config.mac_config.speed_mode = ETH_MODE_100M_FULL;
    config.mac_config.asil_level = ETH_ASIL_B;

    /* 配置DMA */
    config.dma_config.rx_desc_count = 16;
    config.dma_config.tx_desc_count = 16;
    config.dma_config.buffer_size = 1536;

    /* 配置PHY - 100BASE-T1 */
    config.phy_config.phy_type = AUTOMOTIVE_PHY_TJA1101;
    config.phy_config.standard = AUTOMOTIVE_ETH_STANDARD_100BASE_T1;
    config.phy_config.phy_addr = 1;
    config.phy_config.enable_master_mode = true;

    /* 配置管理器 */
    config.link_monitor.enable_link_monitoring = true;
    config.negotiation.enable_auto_negotiation = false;

    /* 初始化 */
    eth_status_t status = eth_manager_init(&config);
    if (status != ETH_OK) {
        /* 处理错误 */
        return;
    }

    /* 启动 */
    status = eth_manager_start();
    if (status != ETH_OK) {
        /* 处理错误 */
        eth_manager_deinit();
        return;
    }

    /* 等待链路连接 */
    status = eth_manager_wait_for_link(5000);
    if (status == ETH_OK) {
        /* 链路已连接 */
    }
}
```

### 数据收发
```c
void eth_send_receive_example(void)
{
    /* 发送数据 */
    uint8_t tx_data[] = "Hello Automotive Ethernet";
    eth_status_t status = eth_mac_transmit(tx_data, sizeof(tx_data), 1000);

    /* 接收数据 */
    uint8_t rx_buffer[1536];
    uint32_t rx_len;
    status = eth_mac_receive(rx_buffer, sizeof(rx_buffer), &rx_len, 1000);
}
```

### 诊断和错误处理
```c
void eth_diagnostics_example(void)
{
    /* 运行诊断 */
    automotive_phy_diagnostics_t diag;
    eth_status_t status = eth_manager_run_diagnostics(&diag);
    if (status == ETH_OK) {
        if (!diag.link_pass) {
            /* 链路故障 */
        }
        if (!diag.cable_ok) {
            /* 电缆故障 */
        }
    }

    /* 获取统计 */
    eth_manager_stats_t stats;
    eth_manager_get_stats(&stats);

    /* 获取错误 */
    eth_error_info_t error;
    eth_manager_get_last_error(&error);
}
```

## 测试

测试文仴位于: `tests/unit/test_eth_driver.c`

运行测试:
```bash
mkdir build && cd build
cmake ..
make test_eth_driver
./tests/test_eth_driver
```

## 未来工作

1. 实现特定MAC芯片的硬件抽象层
2. 完善PHY驱动实现(TJA1101/DP83TC811等)
3. 实现TSN功能(gPTP、门控、流过滤)
4. 添加更多汽车以太网标准支持(2.5G/5G/10G)
5. 完善诊断和自动恢复机制
6. 添加更多单元测试和集成测试
