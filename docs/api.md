# ETH-DDS Integration API文档

## 目录

- [概述](#概述)
- [数据类型](#数据类型)
- [以太网API](#以太网api)
- [DDS API](#dds-api)
- [传输层API](#传输层api)
- [UDS诊断服务API](#uds诊断服务api)
- [工具函数](#工具函数)
- [错误处理](#错误处理)

## 概述

ETH-DDS Integration库提供一套统一的API，用于在嵌入式系统中实现以太网与DDS中间件的集成。

## UDS诊断服务API

### 内存访问服务 (0x23/0x3D)

#### 0x23 Read Memory By Address

```c
/**
 * @brief 处理ReadMemoryByAddress (0x23) 服务请求
 *
 * 该服务允许客户端从指定的内存地址读取数据。
 * 支持的地址格式: 1字节、2字节、4字节地址。
 * 支持的大小格式: 1字节、2字节、4字节大小。
 *
 * @param request 请求消息结构体指针
 *                - data[0]: Service ID (0x23)
 *                - data[1]: Format Identifier (addressLength | sizeLength)
 *                - data[2..n]: Memory Address (MSB first)
 *                - data[n+1..m]: Memory Size (MSB first)
 * @param response 响应消息结构体指针
 *                 - data[0]: Response SID (0x63)
 *                 - data[1..n]: Read data
 * @return Dcm_ReturnType 服务处理结果
 * @retval DCM_E_OK 操作成功
 * @retval DCM_E_NOT_OK 操作失败
 * @retval DCM_E_REQUEST_OUT_OF_RANGE 请求超出范围
 * @retval DCM_E_SECURITY_ACCESS_DENIED 安全访问被拒绝
 *
 * @requirement ISO 14229-1:2020 Section 10.4
 *
 * @example
 * @code
 * // 读取0x20000000地址的4字节数据
 * uint8_t req_data[] = {
 *     0x23,              // SID
 *     0x44,              // 4-byte address, 4-byte size
 *     0x20, 0x00, 0x00, 0x00,  // Address: 0x20000000
 *     0x00, 0x00, 0x00, 0x04   // Size: 4
 * };
 * Dcm_RequestType request = {
 *     .data = req_data,
 *     .length = sizeof(req_data)
 * };
 * Dcm_ResponseType response;
 * Dcm_ReturnType result = Dcm_ReadMemoryByAddress(&request, &response);
 * @endcode
 */
Dcm_ReturnType Dcm_ReadMemoryByAddress(
    const Dcm_RequestType *request,
    Dcm_ResponseType *response
);
```

#### 内存读取函数

```c
/**
 * @brief 从内存地址读取数据
 *
 * 该函数执行实际的内存读取操作。会检查区域权限并调用
 * 配置的回调函数或执行默认读取。
 *
 * @param memoryAddress 源内存地址
 * @param data 存储读取数据的缓冲区
 * @param length 要读取的数据长度
 * @return Dcm_ReturnType 操作结果
 * @retval DCM_E_OK 读取成功
 * @retval DCM_E_NOT_OK 读取失败（地址不在有效区域）
 *
 * @warning 必须先通过 Dcm_IsMemoryAddressReadable() 验证地址可读
 */
Dcm_ReturnType Dcm_ReadMemory(
    uint32_t memoryAddress,
    uint8_t *data,
    uint32_t length
);

/**
 * @brief 检查内存地址是否可读
 *
 * 检查指定地址和长度是否在可读的内存区域内，
 * 并验证安全级别和会话要求。
 *
 * @param memoryAddress 要检查的内存地址
 * @param length 读取长度
 * @return bool 如果可读返回 true
 * @retval true 地址可读
 * @retval false 地址不可读（不在有效区域或无读权限）
 */
bool Dcm_IsMemoryAddressReadable(uint32_t memoryAddress, uint32_t length);
```

#### 格式标识符解析

```c
/**
 * @brief 解析地址和长度格式标识符
 *
 * 格式标识符编码：
 * - 高半字节: 地址长度 (0x1=1字节, 0x2=2字节, 0x4=4字节)
 * - 低半字节: 大小长度 (0x1=1字节, 0x2=2字节, 0x4=4字节)
 *
 * @param formatId 格式标识符字节
 * @param addressLength 输出: 地址长度（1、2或4）
 * @param sizeLength 输出: 大小长度（1、2或4）
 * @return Dcm_ReturnType 解析结果
 * @retval DCM_E_OK 格式有效
 * @retval DCM_E_NOT_OK 格式无效
 *
 * @example
 * @code
 * uint8_t addrLen, sizeLen;
 * Dcm_ReturnType result = Dcm_ParseMemoryFormat(0x44, &addrLen, &sizeLen);
 * // addrLen = 4, sizeLen = 4
 * @endcode
 */
Dcm_ReturnType Dcm_ParseMemoryFormat(
    uint8_t formatId,
    uint8_t *addressLength,
    uint8_t *sizeLength
);

/**
 * @brief 从字节数组解析内存地址
 *
 * 按大端字节序解析地址。
 *
 * @param data 字节数组（MSB在前）
 * @param length 数组长度（1、2或4）
 * @return uint32_t 解析后的地址
 *
 * @example
 * @code
 * uint8_t addr[] = {0x20, 0x00, 0x00, 0x00};
 * uint32_t address = Dcm_ParseMemoryAddress(addr, 4);
 * // address = 0x20000000
 * @endcode
 */
uint32_t Dcm_ParseMemoryAddress(const uint8_t *data, uint8_t length);

/**
 * @brief 从字节数组解析内存大小
 *
 * @param data 字节数组（MSB在前）
 * @param length 数组长度（1、2或4）
 * @return uint32_t 解析后的大小
 */
uint32_t Dcm_ParseMemorySize(const uint8_t *data, uint8_t length);
```

#### 0x3D Write Memory By Address

```c
/**
 * @brief 处理WriteMemoryByAddress (0x3D) 服务请求
 *
 * 该服务允许客户端写数据到指定的内存地址。
 * 需要更高的安全级别，特别是对于Flash操作。
 *
 * @param request 请求消息结构体指针
 *                - data[0]: Service ID (0x3D)
 *                - data[1]: Format Identifier
 *                - data[2..n]: Memory Address
 *                - data[n+1..m]: Memory Size
 *                - data[m+1..p]: Data to write
 * @param response 响应消息结构体指针
 * @return Dcm_ReturnType 服务处理结果
 *
 * @requirement ISO 14229-1:2020 Section 10.18
 */
Dcm_ReturnType Dcm_WriteMemoryByAddress(
    const Dcm_RequestType *request,
    Dcm_ResponseType *response
);

/**
 * @brief 写数据到内存地址
 *
 * @param memoryAddress 目标内存地址
 * @param data 要写入的数据
 * @param length 数据长度
 * @return Dcm_ReturnType 操作结果
 */
Dcm_ReturnType Dcm_WriteMemory(
    uint32_t memoryAddress,
    const uint8_t *data,
    uint32_t length
);

/**
 * @brief 检查内存地址是否可写
 *
 * @param memoryAddress 要检查的内存地址
 * @param length 写入长度
 * @return bool 如果可写返回 true
 */
bool Dcm_IsMemoryAddressWritable(uint32_t memoryAddress, uint32_t length);
```

### 内存服务配置

```c
/**
 * @brief 内存区域类型枚举
 */
typedef enum {
    DCM_MEM_REGION_RAM = 0,         /*!< RAM区域 */
    DCM_MEM_REGION_FLASH,           /*!< Flash/EEPROM区域 */
    DCM_MEM_REGION_REGISTER,        /*!< 硬件寄存器 */
    DCM_MEM_REGION_RESERVED         /*!< 保留/受保护区域 */
} Dcm_MemoryRegionEnum;

/**
 * @brief 内存区域配置结构体
 */
typedef struct {
    uint32_t startAddress;              /*!< 区域起始地址 */
    uint32_t endAddress;                /*!< 区域结束地址 */
    Dcm_MemoryRegionEnum regionType;    /*!< 区域类型 */
    uint8_t requiredSecurityLevel;      /*!< 所需安全级别 */
    bool writeAllowed;                  /*!< 是否允许写入 */
    bool readAllowed;                   /*!< 是否允许读取 */
    bool eraseRequired;                 /*!< 写入前是否需要擦除 */
    uint32_t alignment;                 /*!< 写入对齐要求 */
    const char *description;            /*!< 区域描述 */
} Dcm_MemoryRegionConfigType;

/**
 * @brief 内存读取回调函数类型
 */
typedef Dcm_ReturnType (*Dcm_MemoryReadCallback)(
    uint32_t memoryAddress,
    uint8_t *data,
    uint32_t length,
    Dcm_MemoryRegionEnum regionType
);

/**
 * @brief 内存写入回调函数类型
 */
typedef Dcm_ReturnType (*Dcm_MemoryWriteCallback)(
    uint32_t memoryAddress,
    const uint8_t *data,
    uint32_t length,
    Dcm_MemoryRegionEnum regionType
);
```

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
