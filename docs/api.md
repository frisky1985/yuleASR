# ETH-DDS Integration API文档

## 目录

- [概述](#概述)
- [数据类型](#数据类型)
- [以太网API](#以太网api)
- [DDS API](#dds-api)
- [传输层API](#传输层api)
- [工具函数](#工具函数)
- [错误处理](#错误处理)

## 概述

ETH-DDS Integration库提供一套统一的API，用于在嵌入式系统中实现以太网与DDS中间件的集成。

## 数据类型

### 状态码

```c
typedef enum {
    ETH_OK = 0,              /* 成功 */
    ETH_ERROR = -1,          /* 通用错误 */
    ETH_TIMEOUT = -2,        /* 超时 */
    ETH_INVALID_PARAM = -3,  /* 无效参数 */
    ETH_NO_MEMORY = -4,      /* 内存不足 */
    ETH_BUSY = -5,           /* 忙碌 */
    ETH_NOT_INIT = -6,       /* 未初始化 */
} eth_status_t;
```

### MAC地址

```c
typedef uint8_t eth_mac_addr_t[6];

/* 宏使用示例 */
eth_mac_addr_t mac = ETH_MAC_ADDR(0x00, 0x11, 0x22, 0x33, 0x44, 0x55);
```

### IP地址

```c
typedef uint32_t eth_ip_addr_t;

/* 宏使用示例 */
eth_ip_addr_t ip = ETH_IP_ADDR(192, 168, 1, 1);  /* 192.168.1.1 */
```

### 数据缓冲区

```c
typedef struct {
    uint8_t *data;       /* 数据指针 */
    uint32_t len;        /* 当前长度 */
    uint32_t max_len;    /* 最大长度 */
} eth_buffer_t;
```

### 以太网配置

```c
typedef struct {
    eth_mac_addr_t mac_addr;           /* MAC地址 */
    eth_mode_t mode;                   /* 工作模式 */
    bool enable_dma;                   /* 使能DMA */
    bool enable_checksum_offload;      /* 使能校验和卸载 */
    uint32_t rx_buffer_size;           /* 接收缓冲区大小 */
    uint32_t tx_buffer_size;           /* 发送缓冲区大小 */
    uint8_t rx_desc_count;             /* 接收描述符数量 */
    uint8_t tx_desc_count;             /* 发送描述符数量 */
} eth_config_t;
```

### DDS QoS配置

```c
typedef struct {
    dds_qos_reliability_t reliability;    /* 可靠性策略 */
    dds_qos_durability_t durability;      /* 耐久性策略 */
    uint32_t deadline_ms;                 /* 截止时间(ms) */
    uint32_t latency_budget_ms;           /* 延迟预算(ms) */
    uint32_t history_depth;               /* 历史深度 */
} dds_qos_t;
```

## 以太网API

### 驱动初始化

```c
/**
 * @brief 初始化以太网驱动
 * @param config 驱动配置参数
 * @return eth_status_t 操作状态
 */
eth_status_t eth_driver_init(const eth_config_t *config);
```

**使用示例:**
```c
eth_config_t config = {
    .mac_addr = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55},
    .mode = ETH_MODE_100M_FULL,
    .enable_dma = true,
    .enable_checksum_offload = true,
    .rx_buffer_size = 1536,
    .tx_buffer_size = 1536,
    .rx_desc_count = 8,
    .tx_desc_count = 8
};

eth_status_t status = eth_driver_init(&config);
if (status != ETH_OK) {
    // 处理错误
}
```

### 数据发送

```c
/**
 * @brief 发送以太网数据帧
 * @param buffer 数据缓冲区
 * @param timeout_ms 超时时间(毫秒)
 * @return eth_status_t 操作状态
 */
eth_status_t eth_send(eth_buffer_t *buffer, uint32_t timeout_ms);
```

### 数据接收

```c
/**
 * @brief 接收以太网数据帧
 * @param buffer 数据缓冲区
 * @param timeout_ms 超时时间(毫秒)
 * @return eth_status_t 操作状态
 */
eth_status_t eth_receive(eth_buffer_t *buffer, uint32_t timeout_ms);
```

### 连接状态

```c
/**
 * @brief 获取当前连接状态
 * @return eth_link_status_t 连接状态
 */
eth_link_status_t eth_get_link_status(void);
```

## DDS API

### 参与者管理

```c
/**
 * @brief 创建DDS参与者
 * @param domain_id 域ID
 * @param participant 返回的参与者句柄
 * @return eth_status_t 操作状态
 */
eth_status_t dds_participant_create(dds_domain_id_t domain_id, 
                                     dds_participant_t **participant);

/**
 * @brief 删除DDS参与者
 * @param participant 参与者句柄
 * @return eth_status_t 操作状态
 */
eth_status_t dds_participant_delete(dds_participant_t *participant);
```

### 主题管理

```c
/**
 * @brief 创建DDS主题
 * @param participant 参与者句柄
 * @param topic_name 主题名称
 * @param type_name 数据类型名称
 * @param topic 返回的主题句柄
 * @return eth_status_t 操作状态
 */
eth_status_t dds_topic_create(dds_participant_t *participant,
                               const char *topic_name,
                               const char *type_name,
                               dds_topic_t **topic);

/**
 * @brief 删除DDS主题
 * @param topic 主题句柄
 * @return eth_status_t 操作状态
 */
eth_status_t dds_topic_delete(dds_topic_t *topic);
```

### 发布者管理

```c
/**
 * @brief 创建DDS数据写入者
 * @param topic 主题句柄
 * @param qos QoS配置
 * @param writer 返回的写入者句柄
 * @return eth_status_t 操作状态
 */
eth_status_t dds_writer_create(dds_topic_t *topic,
                                const dds_qos_t *qos,
                                dds_writer_t **writer);

/**
 * @brief 写入数据
 * @param writer 写入者句柄
 * @param data 数据指针
 * @param size 数据大小
 * @param timeout_ms 超时时间
 * @return eth_status_t 操作状态
 */
eth_status_t dds_write(dds_writer_t *writer,
                        const void *data,
                        uint32_t size,
                        uint32_t timeout_ms);
```

### 订阅者管理

```c
/**
 * @brief 创建DDS数据读取者
 * @param topic 主题句柄
 * @param qos QoS配置
 * @param reader 返回的读取者句柄
 * @return eth_status_t 操作状态
 */
eth_status_t dds_reader_create(dds_topic_t *topic,
                                const dds_qos_t *qos,
                                dds_reader_t **reader);

/**
 * @brief 读取数据
 * @param reader 读取者句柄
 * @param data 数据缓冲区
 * @param size 缓冲区大小
 * @param timeout_ms 超时时间
 * @return eth_status_t 操作状态
 */
eth_status_t dds_read(dds_reader_t *reader,
                       void *data,
                       uint32_t *size,
                       uint32_t timeout_ms);

/**
 * @brief 设置数据接收回调
 * @param reader 读取者句柄
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return eth_status_t 操作状态
 */
eth_status_t dds_reader_set_callback(dds_reader_t *reader,
                                      dds_data_callback_t callback,
                                      void *user_data);
```

## 传输层API

### UDP传输

```c
/**
 * @brief 创建UDP传输
 * @param local_addr 本地地址
 * @param local_port 本地端口
 * @param transport 返回的传输句柄
 * @return eth_status_t 操作状态
 */
eth_status_t udp_transport_create(const char *local_addr,
                                   uint16_t local_port,
                                   transport_handle_t *transport);

/**
 * @brief 发送UDP数据
 * @param transport 传输句柄
 * @param dst_addr 目标地址
 * @param dst_port 目标端口
 * @param data 数据缓冲区
 * @param len 数据长度
 * @return eth_status_t 操作状态
 */
eth_status_t udp_send(transport_handle_t transport,
                       const char *dst_addr,
                       uint16_t dst_port,
                       const uint8_t *data,
                       uint32_t len);
```

### TCP传输

```c
/**
 * @brief 创建TCP服务器
 * @param local_addr 监听地址
 * @param local_port 监听端口
 * @param transport 返回的传输句柄
 * @return eth_status_t 操作状态
 */
eth_status_t tcp_server_create(const char *local_addr,
                                uint16_t local_port,
                                transport_handle_t *transport);

/**
 * @brief 连接TCP服务器
 * @param server_addr 服务器地址
 * @param server_port 服务器端口
 * @param transport 返回的传输句柄
 * @return eth_status_t 操作状态
 */
eth_status_t tcp_connect(const char *server_addr,
                          uint16_t server_port,
                          transport_handle_t *transport);
```

### 共享内存传输

```c
/**
 * @brief 创建共享内存传输
 * @param shm_name 共享内存名称
 * @param shm_size 共享内存大小
 * @param transport 返回的传输句柄
 * @return eth_status_t 操作状态
 */
eth_status_t shm_transport_create(const char *shm_name,
                                   uint32_t shm_size,
                                   transport_handle_t *transport);
```

## 工具函数

### CRC计算

```c
/**
 * @brief 计算CRC32校验和
 * @param data 数据指针
 * @param len 数据长度
 * @return uint32_t CRC32值
 */
uint32_t eth_crc32(const uint8_t *data, uint32_t len);
```

**使用示例:**
```c
uint8_t data[] = "Hello, World!";
uint32_t crc = eth_crc32(data, sizeof(data) - 1);
printf("CRC32: 0x%08X\n", crc);
```

### 字节序转换

```c
/* 主机到网络 */
uint32_t eth_htonl(uint32_t hostlong);
uint16_t eth_htons(uint16_t hostshort);

/* 网络到主机 */
uint32_t eth_ntohl(uint32_t netlong);
uint16_t eth_ntohs(uint16_t netshort);
```

### MAC地址操作

```c
/**
 * @brief MAC地址转字符串
 * @param mac MAC地址
 * @param str 输出字符串缓冲区
 * @param str_len 缓冲区长度（至少18字节）
 * @return eth_status_t 操作状态
 */
eth_status_t eth_mac_to_string(const eth_mac_addr_t mac, 
                                char *str, 
                                size_t str_len);

/**
 * @brief 字符串转MAC地址
 * @param str 输入字符串（格式: "00:11:22:33:44:55"）
 * @param mac 输出MAC地址
 * @return eth_status_t 操作状态
 */
eth_status_t eth_mac_from_string(const char *str, eth_mac_addr_t mac);
```

### IP地址操作

```c
/**
 * @brief IP地址转字符串
 * @param ip IP地址
 * @param str 输出字符串缓冲区
 * @param str_len 缓冲区长度（至少16字节）
 * @return eth_status_t 操作状态
 */
eth_status_t eth_ip_to_string(eth_ip_addr_t ip, char *str, size_t str_len);

/**
 * @brief 字符串转IP地址
 * @param str 输入字符串（格式: "192.168.1.1"）
 * @param ip 输出IP地址
 * @return eth_status_t 操作状态
 */
eth_status_t eth_ip_from_string(const char *str, eth_ip_addr_t *ip);
```

### 安全内存操作

```c
/**
 * @brief 安全内存复制
 * @param dst 目标缓冲区
 * @param src 源缓冲区
 * @param dst_size 目标缓冲区大小
 * @param src_size 源数据大小
 * @return eth_status_t 操作状态
 */
eth_status_t eth_safe_memcpy(void *dst, 
                              const void *src, 
                              size_t dst_size, 
                              size_t src_size);
```

### 时间操作

```c
/**
 * @brief 获取当前时间戳(毫秒)
 * @return uint64_t 时间戳
 */
uint64_t eth_get_timestamp_ms(void);

/**
 * @brief 延时指定毫秒数
 * @param ms 延时毫秒数
 */
void eth_delay_ms(uint32_t ms);
```

## 错误处理

### 错误码映射

```c
const char* eth_status_to_string(eth_status_t status)
{
    switch (status) {
        case ETH_OK:            return "Success";
        case ETH_ERROR:         return "General error";
        case ETH_TIMEOUT:       return "Timeout";
        case ETH_INVALID_PARAM: return "Invalid parameter";
        case ETH_NO_MEMORY:     return "No memory";
        case ETH_BUSY:          return "Busy";
        case ETH_NOT_INIT:      return "Not initialized";
        default:                return "Unknown error";
    }
}
```

### 错误处理模式

```c
/* 同步错误处理 */
eth_status_t status = eth_driver_init(&config);
if (status != ETH_OK) {
    printf("Error: %s\n", eth_status_to_string(status));
    return status;
}

/* 异步错误处理 */
eth_status_t status = eth_receive(&buffer, 100);
if (status == ETH_TIMEOUT) {
    // 处理超时 - 可以重试
} else if (status == ETH_ERROR) {
    // 处理错误 - 需要修复
}
```

## 版本信息

```c
#define ETH_DDS_VERSION_MAJOR 1
#define ETH_DDS_VERSION_MINOR 0
#define ETH_DDS_VERSION_PATCH 0

const char* eth_dds_get_version_string(void);
uint32_t eth_dds_get_version_number(void);
```

---

*最后更新: 2026-04-24*
