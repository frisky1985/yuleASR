/**
 * @file tcpip_types.h
 * @brief TCP/IP协议栈核心类型定义
 * @version 1.0
 * @date 2026-04-26
 *
 * @note 符合AUTOSAR规范
 * @note MISRA C:2012 compliant
 */

#ifndef TCPIP_TYPES_H
#define TCPIP_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 协议版本与配置
 * ============================================================================ */

/** TCP/IP协议栈版本 */
#define TCPIP_MAJOR_VERSION     1u
#define TCPIP_MINOR_VERSION     0u
#define TCPIP_PATCH_VERSION     0u

/** 配置常量 */
#define TCPIP_MAX_IFACES        4u          /**< 最大接口数 */
#define TCPIP_MAX_SOCKETS       16u         /**< 最大Socket数 */
#define TCPIP_MAX_CONNECTIONS   8u          /**< 最大TCP连接数 */
#define TCPIP_MAX_ROUTES        8u          /**< 最大路由表项 */
#define TCPIP_ARP_CACHE_SIZE    16u         /**< ARP缓存大小 */
#define TCPIP_MAX_DNS_SERVERS   2u          /**< 最大DNS服务器数 */

/** 以太网协议类型 */
#define ETH_TYPE_IP             0x0800u
#define ETH_TYPE_ARP            0x0806u
#define ETH_TYPE_IPV6           0x86DDu

/* ============================================================================
 * IP协议相关定义
 * ============================================================================ */

/** IP地址类型 */
typedef uint32_t tcpip_ip_addr_t;

/** IPv4地址结构 */
typedef struct {
    uint8_t addr[4];
} tcpip_ipv4_addr_t;

/** IP地址配置 */
typedef struct {
    tcpip_ip_addr_t addr;           /**< IP地址 */
    tcpip_ip_addr_t netmask;        /**< 子网掩码 */
    tcpip_ip_addr_t gateway;        /**< 默认网关 */
} tcpip_ip_config_t;

/** IP协议类型 */
typedef enum {
    TCPIP_IP_PROTO_ICMP = 1u,       /**< ICMP协议 */
    TCPIP_IP_PROTO_TCP  = 6u,       /**< TCP协议 */
    TCPIP_IP_PROTO_UDP  = 17u       /**< UDP协议 */
} tcpip_ip_protocol_t;

/** IP包头结构 (20 bytes) */
typedef struct __attribute__((packed)) {
    uint8_t  ver_ihl;               /**< 版本(4) + 头长度(4) */
    uint8_t  tos;                   /**< 服务类型 */
    uint16_t total_len;             /**< 总长度 */
    uint16_t id;                    /**< 标识 */
    uint16_t flags_frag;            /**< 标志(3) + 片偏移(13) */
    uint8_t  ttl;                   /**< 生存时间 */
    uint8_t  protocol;              /**< 协议 */
    uint16_t checksum;              /**< 首部校验和 */
    uint32_t src_addr;              /**< 源IP地址 */
    uint32_t dst_addr;              /**< 目的IP地址 */
} tcpip_ip_header_t;

#define TCPIP_IP_VER_IHL          0x45u   /**< IPv4, 5 * 4 = 20 bytes header */
#define TCPIP_IP_TTL_DEFAULT      64u     /**< 默认TTL */
#define TCPIP_IP_MTU_DEFAULT      1500u   /**< 默认MTU */
#define TCPIP_IP_HEADER_LEN       20u     /**< IP头部长度 */
#define TCPIP_IP_VERSION_4        4u      /**< IPv4版本 */

/* ============================================================================
 * TCP协议相关定义
 * ============================================================================ */

/** TCP端口类型 */
typedef uint16_t tcpip_port_t;

/** TCP连接状态 (AUTOSAR TcpIp_StateType) */
typedef enum {
    TCPIP_TCP_STATE_CLOSED      = 0u,
    TCPIP_TCP_STATE_LISTEN      = 1u,
    TCPIP_TCP_STATE_SYN_SENT    = 2u,
    TCPIP_TCP_STATE_SYN_RECV    = 3u,
    TCPIP_TCP_STATE_ESTABLISHED = 4u,
    TCPIP_TCP_STATE_FIN_WAIT1   = 5u,
    TCPIP_TCP_STATE_FIN_WAIT2   = 6u,
    TCPIP_TCP_STATE_CLOSE_WAIT  = 7u,
    TCPIP_TCP_STATE_CLOSING     = 8u,
    TCPIP_TCP_STATE_LAST_ACK    = 9u,
    TCPIP_TCP_STATE_TIME_WAIT   = 10u
} tcpip_tcp_state_t;

/** TCP标志位 */
#define TCPIP_TCP_FLAG_FIN      0x01u
#define TCPIP_TCP_FLAG_SYN      0x02u
#define TCPIP_TCP_FLAG_RST      0x04u
#define TCPIP_TCP_FLAG_PSH      0x08u
#define TCPIP_TCP_FLAG_ACK      0x10u
#define TCPIP_TCP_FLAG_URG      0x20u

/** TCP包头结构 (20 bytes min) */
typedef struct __attribute__((packed)) {
    uint16_t src_port;              /**< 源端口 */
    uint16_t dst_port;              /**< 目的端口 */
    uint32_t seq_num;               /**< 序列号 */
    uint32_t ack_num;               /**< 确认号 */
    uint8_t  data_offset;           /**< 数据偏移 (4 bits) + 保留 (4 bits) */
    uint8_t  flags;                 /**< 标志位 */
    uint16_t window;                /**< 窗口大小 */
    uint16_t checksum;              /**< 校验和 */
    uint16_t urgent_ptr;            /**< 紧急指针 */
} tcpip_tcp_header_t;

#define TCPIP_TCP_HEADER_LEN        20u
#define TCPIP_TCP_MSS_DEFAULT       1460u
#define TCPIP_TCP_WINDOW_DEFAULT    8192u
#define TCPIP_TCP_MAX_RETRIES       5u
#define TCPIP_TCP_RETRY_TIMEOUT_MS  1000u

/* ============================================================================
 * UDP协议相关定义
 * ============================================================================ */

/** UDP包头结构 (8 bytes) */
typedef struct __attribute__((packed)) {
    uint16_t src_port;              /**< 源端口 */
    uint16_t dst_port;              /**< 目的端口 */
    uint16_t length;                /**< 长度 */
    uint16_t checksum;              /**< 校验和 */
} tcpip_udp_header_t;

#define TCPIP_UDP_HEADER_LEN        8u

/* ============================================================================
 * ICMP协议相关定义
 * ============================================================================ */

/** ICMP类型 */
typedef enum {
    TCPIP_ICMP_ECHO_REPLY   = 0u,
    TCPIP_ICMP_DEST_UNREACH = 3u,
    TCPIP_ICMP_ECHO_REQUEST = 8u
} tcpip_icmp_type_t;

/** ICMP包头结构 (8 bytes) */
typedef struct __attribute__((packed)) {
    uint8_t  type;                  /**< 类型 */
    uint8_t  code;                  /**< 代码 */
    uint16_t checksum;              /**< 校验和 */
    uint16_t id;                    /**< 标识 */
    uint16_t seq;                   /**< 序列号 */
} tcpip_icmp_header_t;

#define TCPIP_ICMP_HEADER_LEN       8u

/* ============================================================================
 * ARP协议相关定义
 * ============================================================================ */

/** ARP操作码 */
typedef enum {
    TCPIP_ARP_OP_REQUEST = 1u,
    TCPIP_ARP_OP_REPLY   = 2u
} tcpip_arp_opcode_t;

/** ARP包结构 */
typedef struct __attribute__((packed)) {
    uint16_t hw_type;               /**< 硬件类型 (Ethernet = 1) */
    uint16_t proto_type;            /**< 协议类型 (IPv4 = 0x0800) */
    uint8_t  hw_len;                /**< 硬件地址长度 (6) */
    uint8_t  proto_len;             /**< 协议地址长度 (4) */
    uint16_t opcode;                /**< 操作码 */
    uint8_t  sender_mac[6];         /**< 发送方MAC */
    uint32_t sender_ip;             /**< 发送方IP */
    uint8_t  target_mac[6];         /**< 目标MAC */
    uint32_t target_ip;             /**< 目标IP */
} tcpip_arp_packet_t;

#define TCPIP_ARP_HW_TYPE_ETH       1u
#define TCPIP_ARP_PROTO_TYPE_IP     0x0800u
#define TCPIP_ARP_HW_LEN            6u
#define TCPIP_ARP_PROTO_LEN         4u
#define TCPIP_ARP_PACKET_LEN        28u

/* ============================================================================
 * DHCP相关定义
 * ============================================================================ */

/** DHCP状态 */
typedef enum {
    TCPIP_DHCP_STATE_IDLE,
    TCPIP_DHCP_STATE_INIT,
    TCPIP_DHCP_STATE_SELECTING,
    TCPIP_DHCP_STATE_REQUESTING,
    TCPIP_DHCP_STATE_BOUND,
    TCPIP_DHCP_STATE_RENEWING,
    TCPIP_DHCP_STATE_REBINDING
} tcpip_dhcp_state_t;

/** DHCP魔术Cookie */
#define TCPIP_DHCP_MAGIC_COOKIE     0x63825363u
#define TCPIP_DHCP_SERVER_PORT      67u
#define TCPIP_DHCP_CLIENT_PORT      68u

/* ============================================================================
 * Socket相关定义
 * ============================================================================ */

/** Socket ID类型 */
typedef uint8_t tcpip_socket_id_t;

/** Socket类型 */
typedef enum {
    TCPIP_SOCK_STREAM = 0u,         /**< TCP流式Socket */
    TCPIP_SOCK_DGRAM  = 1u          /**< UDP数据报Socket */
} tcpip_socket_type_t;

/** Socket状态 */
typedef enum {
    TCPIP_SOCK_STATE_UNUSED     = 0u,
    TCPIP_SOCK_STATE_CREATED    = 1u,
    TCPIP_SOCK_STATE_BOUND      = 2u,
    TCPIP_SOCK_STATE_LISTENING  = 3u,
    TCPIP_SOCK_STATE_CONNECTED  = 4u,
    TCPIP_SOCK_STATE_CLOSING    = 5u
} tcpip_socket_state_t;

/** Socket地址 */
typedef struct {
    tcpip_ip_addr_t addr;
    tcpip_port_t    port;
} tcpip_sockaddr_t;

/** Socket配置 */
typedef struct {
    tcpip_socket_type_t type;
    tcpip_ip_addr_t local_addr;
    tcpip_port_t local_port;
    uint16_t rx_buffer_size;
    uint16_t tx_buffer_size;
} tcpip_socket_config_t;

/* ============================================================================
 * 网络接口定义
 * ============================================================================ */

/** 网络接口状态 */
typedef enum {
    TCPIP_IF_STATE_DOWN = 0u,
    TCPIP_IF_STATE_UP   = 1u
} tcpip_iface_state_t;

/** 网络接口配置 */
typedef struct {
    uint8_t iface_id;
    uint8_t mac_addr[6];
    tcpip_ip_config_t ip_config;
    bool dhcp_enabled;
    uint16_t mtu;
} tcpip_iface_config_t;

/** 网络接口信息 */
typedef struct {
    uint8_t iface_id;
    uint8_t mac_addr[6];
    tcpip_iface_state_t state;
    tcpip_ip_config_t ip_config;
    uint64_t rx_packets;
    uint64_t tx_packets;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint32_t rx_errors;
    uint32_t tx_errors;
} tcpip_iface_info_t;

/* ============================================================================
 * 路由表定义
 * ============================================================================ */

/** 路由表项 */
typedef struct {
    tcpip_ip_addr_t dest_addr;      /**< 目的网络地址 */
    tcpip_ip_addr_t netmask;        /**< 子网掩码 */
    tcpip_ip_addr_t gateway;        /**< 网关地址 (0 = 直连) */
    uint8_t iface_id;               /**< 出接口ID */
    uint8_t metric;                 /**< 度量值 */
    bool is_active;                 /**< 是否激活 */
} tcpip_route_entry_t;

/* ============================================================================
 * 错误码定义
 * ============================================================================ */

/** TCP/IP错误码 */
typedef enum {
    TCPIP_OK                = 0u,
    TCPIP_ERROR             = 1u,
    TCPIP_ERROR_PARAM       = 2u,
    TCPIP_ERROR_NO_MEMORY   = 3u,
    TCPIP_ERROR_NOT_FOUND   = 4u,
    TCPIP_ERROR_BUSY        = 5u,
    TCPIP_ERROR_TIMEOUT     = 6u,
    TCPIP_ERROR_UNREACHABLE = 7u,
    TCPIP_ERROR_NO_ROUTE    = 8u,
    TCPIP_ERROR_CHECKSUM    = 9u,
    TCPIP_ERROR_MTU         = 10u,
    TCPIP_ERROR_SOCKET      = 11u,
    TCPIP_ERROR_CLOSED      = 12u,
    TCPIP_ERROR_CONN_RESET  = 13u,
    TCPIP_ERROR_IN_USE      = 14u
} tcpip_error_t;

/* ============================================================================
 * 回调函数类型定义
 * ============================================================================ */

/** 数据接收回调 */
typedef void (*tcpip_rx_callback_t)(uint8_t iface_id, const uint8_t *data, 
                                     uint16_t len, void *user_data);

/** Socket事件回调 */
typedef void (*tcpip_socket_event_cb_t)(tcpip_socket_id_t sock_id, 
                                         uint8_t event, void *user_data);

/** 状态变化回调 */
typedef void (*tcpip_state_callback_t)(uint8_t iface_id, 
                                        tcpip_iface_state_t state, 
                                        void *user_data);

/* ============================================================================
 * 工具宏
 * ============================================================================ */

/** IP地址转换宏 */
#define TCPIP_IP_ADDR(a,b,c,d) ((((uint32_t)(a))<<24)|(((uint32_t)(b))<<16)|\
                                (((uint32_t)(c))<<8)|((uint32_t)(d)))

#define TCPIP_IP_ANY            0x00000000u
#define TCPIP_IP_BROADCAST      0xFFFFFFFFu
#define TCPIP_IP_LOOPBACK       0x7F000001u  /**< 127.0.0.1 */

/** 字节序转换 (假设小端系统) */
#define TCPIP_HTONS(x) ((uint16_t)(((x)<<8)&0xFF00u)|(((x)>>8)&0x00FFu))
#define TCPIP_NTOHS(x) TCPIP_HTONS(x)
#define TCPIP_HTONL(x) ((((uint32_t)(x)<<24)&0xFF000000u)|\
                        (((uint32_t)(x)<<8)&0x00FF0000u)|\
                        (((uint32_t)(x)>>8)&0x0000FF00u)|\
                        (((uint32_t)(x)>>24)&0x000000FFu))
#define TCPIP_NTOHL(x) TCPIP_HTONL(x)

/** IP地址比较 */
#define TCPIP_IP_IS_EQUAL(a,b)  ((a)==(b))
#define TCPIP_IP_IS_ANY(a)      ((a)==TCPIP_IP_ANY)
#define TCPIP_IP_IS_BROADCAST(a) ((a)==TCPIP_IP_BROADCAST)

/** IP子网检查 */
#define TCPIP_IP_IS_SAME_SUBNET(ip1,ip2,mask) (((ip1)&(mask))==((ip2)&(mask)))

#ifdef __cplusplus
}
#endif

#endif /* TCPIP_TYPES_H */
