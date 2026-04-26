/**
 * @file soad.h
 * @brief AUTOSAR Socket Adapter (SoAd) 核心头文件
 * @version 1.0
 * @date 2026-04-26
 *
 * @note 符合AUTOSAR 4.x规范
 * @note 支持TCP/UDP Socket管理和PDU路由
 */

#ifndef SOAD_H
#define SOAD_H

#include <stdint.h>
#include <stdbool.h>
#include "../common/types/eth_types.h"
#include "../common/autosar_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * AUTOSAR SoAd 版本和配置
 * ============================================================================ */

#define SOAD_VENDOR_ID                    0x00U
#define SOAD_MODULE_ID                    0x50U
#define SOAD_AR_RELEASE_MAJOR_VERSION     4
#define SOAD_AR_RELEASE_MINOR_VERSION     4
#define SOAD_AR_RELEASE_REVISION_VERSION  0
#define SOAD_SW_MAJOR_VERSION             1
#define SOAD_SW_MINOR_VERSION             0
#define SOAD_SW_PATCH_VERSION             0

/* ============================================================================
 * SoAd 配置常量
 * ============================================================================ */

#define SOAD_MAX_SOCKET_CONNECTIONS       16U
#define SOAD_MAX_SOCKET_ROUTING_GROUPS    8U
#define SOAD_MAX_PDU_ROUTES               32U
#define SOAD_MAX_SOCKETS                  8U
#define SOAD_MAX_PDUS_PER_SOCKET          16U
#define SOAD_PDU_HEADER_SIZE              8U
#define SOAD_MAX_PDU_SIZE                 4096U
#define SOAD_SOCKET_RX_BUFFER_SIZE        8192U
#define SOAD_SOCKET_TX_BUFFER_SIZE        8192U

/* ============================================================================
 * SoAd 错误代码
 * ============================================================================ */

typedef enum {
    SOAD_E_NO_ERROR = 0,           /* 无错误 */
    SOAD_E_NOT_INITIALIZED,        /* 未初始化 */
    SOAD_E_ALREADY_INITIALIZED,    /* 已初始化 */
    SOAD_E_INVALID_PARAMETER,      /* 无效参数 */
    SOAD_E_NO_SOCKET,              /* 无可用Socket */
    SOAD_E_SOCKET_IN_USE,          /* Socket正在使用 */
    SOAD_E_SOCKET_NOT_CONNECTED,   /* Socket未连接 */
    SOAD_E_SOCKET_ERROR,           /* Socket错误 */
    SOAD_E_PDU_TOO_LARGE,          /* PDU过大 */
    SOAD_E_PDU_NOT_FOUND,          /* 未找到PDU */
    SOAD_E_ROUTING_ERROR,          /* 路由错误 */
    SOAD_E_TIMEOUT,                /* 超时 */
    SOAD_E_MEMORY_ERROR,           /* 内存错误 */
} SoAd_ReturnType;

/* ============================================================================
 * Socket 类型定义
 * ============================================================================ */

typedef enum {
    SOAD_SOCKET_TYPE_STREAM = 0,   /* TCP Socket */
    SOAD_SOCKET_TYPE_DGRAM,        /* UDP Socket */
} SoAd_SocketType;

typedef enum {
    SOAD_SOCKET_STATE_UNINIT = 0,
    SOAD_SOCKET_STATE_INITIALIZED,
    SOAD_SOCKET_STATE_CONNECTING,
    SOAD_SOCKET_STATE_CONNECTED,
    SOAD_SOCKET_STATE_LISTENING,
    SOAD_SOCKET_STATE_CLOSED,
    SOAD_SOCKET_STATE_ERROR
} SoAd_SocketStateType;

typedef enum {
    SOAD_SO_CON_MODE_TCPIP = 0,    /* 标准TCP/UDP模式 */
    SOAD_SO_CON_MODE_UDP_EXT,      /* UDP扩展模式 */
} SoAd_SoConModeType;

typedef enum {
    SOAD_SOCKET_PROTOCOL_TCP = 0,
    SOAD_SOCKET_PROTOCOL_UDP,
} SoAd_SocketProtocolType;

/* Socket ID */
typedef uint16_t SoAd_SoConIdType;
#define SOAD_INVALID_SOCKET_ID    0xFFFFU

/* PDU ID */
typedef uint16_t SoAd_PduRouteIdType;
#define SOAD_INVALID_PDU_ROUTE_ID 0xFFFFU

/* Socket地址结构 */
typedef struct {
    uint32_t addr;                 /* IP地址 (IPv4) */
    uint16_t port;                 /* 端口 */
    bool is_multicast;             /* 是否多播 */
} SoAd_SocketAddrType;

/* Socket连接配置 */
typedef struct {
    SoAd_SocketType socket_type;   /* Socket类型 */
    SoAd_SocketProtocolType protocol; /* 协议 */
    uint16_t local_port;           /* 本地端口 */
    uint32_t local_ip;             /* 本地IP */
    SoAd_SoConModeType mode;       /* 连接模式 */
    bool auto_connect;             /* 自动连接 */
    uint16_t remote_port;          /* 远程端口 (用于客户端) */
    uint32_t remote_ip;            /* 远程IP (用于客户端) */
    bool enable_keepalive;         /* 启用Keep-alive */
    uint16_t keepalive_time;       /* Keep-alive时间(秒) */
    bool enable_nagle;             /* 启用Nagle算法 */
    uint16_t rx_buffer_size;       /* 接收缓冲区大小 */
    uint16_t tx_buffer_size;       /* 发送缓冲区大小 */
    uint16_t conn_timeout_ms;      /* 连接超时 */
    uint16_t tx_timeout_ms;        /* 发送超时 */
} SoAd_SocketConnectionCfgType;

/* Socket连接状态 */
typedef struct {
    SoAd_SocketStateType state;
    SoAd_SocketAddrType local_addr;
    SoAd_SocketAddrType remote_addr;
    bool is_server;
    uint32_t bytes_sent;
    uint32_t bytes_received;
    uint32_t connect_time;
    uint32_t last_activity;
    uint16_t error_count;
} SoAd_SoConStateType;

/* ============================================================================
 * PDU 路由定义
 * ============================================================================ */

typedef enum {
    SOAD_PDU_DIR_TX = 0,           /* 发送方向 */
    SOAD_PDU_DIR_RX,               /* 接收方向 */
    SOAD_PDU_DIR_TX_RX             /* 双向 */
} SoAd_PduDirectionType;

typedef enum {
    SOAD_UPPER_PDUR = 0,           /* PduR上层 */
    SOAD_UPPER_DOIP,               /* DoIP上层 */
    SOAD_UPPER_SD,                 /* SD上层 */
} SoAd_UpperLayerType;

/* PDU路由配置 */
typedef struct {
    SoAd_PduRouteIdType route_id;  /* 路由ID */
    SoAd_SoConIdType socket_id;    /* 关联Socket ID */
    uint16_t source_pdu_id;        /* 源PDU ID (来自上层) */
    uint16_t dest_pdu_id;          /* 目标PDU ID (发送到上层) */
    SoAd_PduDirectionType direction; /* 方向 */
    SoAd_UpperLayerType upper_layer; /* 上层模块 */
    bool enable_header;            /* 启用PDU头 */
    uint8_t header_size;           /* 头部大小 */
    uint16_t pdu_size;             /* PDU大小 */
    bool use_tp;                   /* 使用Transport Protocol */
} SoAd_PduRouteCfgType;

/* PDU信息 */
typedef struct {
    uint8_t *data;
    uint16_t length;
    uint16_t pdu_id;
    SoAd_PduRouteIdType route_id;
} SoAd_PduInfoType;

/* ============================================================================
 * 回调函数类型
 * ============================================================================ */

/* 上层数据发送确认回调 */
typedef void (*SoAd_IfTxConfirmation_FuncType)(uint16_t pduId);

/* 上层数据提供回调 */
typedef void (*SoAd_IfRxIndication_FuncType)(uint16_t pduId, const uint8_t *data, uint16_t length);

/* 上层TP数据发送确认回调 */
typedef void (*SoAd_TpTxConfirmation_FuncType)(uint16_t pduId, SoAd_ReturnType result);

/* 上层TP数据提供回调 */
typedef Std_ReturnType (*SoAd_TpRxIndication_FuncType)(uint16_t pduId, const uint8_t *data, uint16_t length);

/* 上层缓冲区请求回调 */
typedef Std_ReturnType (*SoAd_CopyTxData_FuncType)(uint16_t pduId, uint8_t *buffer, uint16_t length);

/* Socket状态变化回调 */
typedef void (*SoAd_SoConModeChg_FuncType)(SoAd_SoConIdType soConId, SoAd_SocketStateType newMode);

/* Socket错误回调 */
typedef void (*SoAd_SoConError_FuncType)(SoAd_SoConIdType soConId, uint8_t errorCode);

/* ============================================================================
 * SoAd 配置集合类型
 * ============================================================================ */

typedef struct {
    const SoAd_SocketConnectionCfgType *socket_conns;
    uint16_t num_socket_conns;
    const SoAd_PduRouteCfgType *pdu_routes;
    uint16_t num_pdu_routes;
} SoAd_ConfigType;

/* ============================================================================
 * SoAd API声明
 * ============================================================================ */

/**
 * @brief 初始化SoAd模块
 * @param config SoAd配置
 * @return SOAD_E_NO_ERROR成功
 */
SoAd_ReturnType SoAd_Init(const SoAd_ConfigType *config);

/**
 * @brief 反初始化SoAd模块
 */
void SoAd_DeInit(void);

/**
 * @brief 获取SoAd版本信息
 * @param versioninfo 版本信息输出
 */
void SoAd_GetVersionInfo(Std_VersionInfoType *versioninfo);

/**
 * @brief SoAd主函数 (定期调用)
 */
void SoAd_MainFunction(void);

/* ============================================================================
 * Socket连接管理API
 * ============================================================================ */

/**
 * @brief 打开Socket连接
 * @param soConId Socket连接ID
 * @return SOAD_E_NO_ERROR成功
 */
SoAd_ReturnType SoAd_OpenSoCon(SoAd_SoConIdType soConId);

/**
 * @brief 关闭Socket连接
 * @param soConId Socket连接ID
 * @param abort 是否中止连接
 * @return SOAD_E_NO_ERROR成功
 */
SoAd_ReturnType SoAd_CloseSoCon(SoAd_SoConIdType soConId, bool abort);

/**
 * @brief 获取Socket连接状态
 * @param soConId Socket连接ID
 * @param state 状态输出
 * @return SOAD_E_NO_ERROR成功
 */
SoAd_ReturnType SoAd_GetSoConState(SoAd_SoConIdType soConId, SoAd_SoConStateType *state);

/**
 * @brief 设置Socket连接模式
 * @param soConId Socket连接ID
 * @param mode 连接模式
 * @return SOAD_E_NO_ERROR成功
 */
SoAd_ReturnType SoAd_SetSoConMode(SoAd_SoConIdType soConId, SoAd_SoConModeType mode);

/**
 * @brief 请求IP地址分配
 * @param soConId Socket连接ID
 * @param assigned 是否已分配输出
 * @return SOAD_E_NO_ERROR成功
 */
SoAd_ReturnType SoAd_RequestIpAddrAssignment(SoAd_SoConIdType soConId, bool *assigned);

/**
 * @brief 释放IP地址分配
 * @param soConId Socket连接ID
 * @return SOAD_E_NO_ERROR成功
 */
SoAd_ReturnType SoAd_ReleaseIpAddrAssignment(SoAd_SoConIdType soConId);

/**
 * @brief 获取本地IP地址
 * @param soConId Socket连接ID
 * @param localAddr 地址输出
 * @return SOAD_E_NO_ERROR成功
 */
SoAd_ReturnType SoAd_GetLocalAddr(SoAd_SoConIdType soConId, SoAd_SocketAddrType *localAddr);

/**
 * @brief 获取物理地址
 * @param soConId Socket连接ID
 * @param physAddr MAC地址输出
 * @return SOAD_E_NO_ERROR成功
 */
SoAd_ReturnType SoAd_GetPhysAddr(SoAd_SoConIdType soConId, uint8_t physAddr[6]);

/* ============================================================================
 * PDU传输API
 * ============================================================================ */

/**
 * @brief 通过如果路由发送数据
 * @param pduId PDU ID
 * @param info PDU信息
 * @return SOAD_E_NO_ERROR成功
 */
SoAd_ReturnType SoAd_IfTransmit(uint16_t pduId, const SoAd_PduInfoType *info);

/**
 * @brief 通过TP路由发送数据
 * @param pduId PDU ID
 * @param info PDU信息
 * @param retry 重试信息
 * @param tpDataLength 总TP数据长度
 * @return SOAD_E_NO_ERROR成功
 */
SoAd_ReturnType SoAd_TpTransmit(uint16_t pduId, const SoAd_PduInfoType *info, 
                                 uint8_t retry, uint16_t tpDataLength);

/**
 * @brief 为TP接收提供缓冲区
 * @param pduId PDU ID
 * @param buffer 缓冲区指针
 * @param length 缓冲区长度
 * @return SOAD_E_NO_ERROR成功
 */
SoAd_ReturnType SoAd_TpProvideRxBuffer(uint16_t pduId, uint8_t **buffer, uint16_t length);

/**
 * @brief TP接收确认
 * @param pduId PDU ID
 * @param length 已接收长度
 * @return SOAD_E_NO_ERROR成功
 */
SoAd_ReturnType SoAd_TpRxIndication(uint16_t pduId, uint16_t length);

/* ============================================================================
 * 路由组API
 * ============================================================================ */

/**
 * @brief 打开路由组连接
 * @param routingGroup 路由组ID
 * @return SOAD_E_NO_ERROR成功
 */
SoAd_ReturnType SoAd_OpenRoutingGroup(uint16_t routingGroup);

/**
 * @brief 关闭路由组连接
 * @param routingGroup 路由组ID
 * @return SOAD_E_NO_ERROR成功
 */
SoAd_ReturnType SoAd_CloseRoutingGroup(uint16_t routingGroup);

/**
 * @brief 使能路由组连接
 * @param routingGroup 路由组ID
 * @return SOAD_E_NO_ERROR成功
 */
SoAd_ReturnType SoAd_EnableRouting(uint16_t routingGroup);

/**
 * @brief 禁用路由组连接
 * @param routingGroup 路由组ID
 * @return SOAD_E_NO_ERROR成功
 */
SoAd_ReturnType SoAd_DisableRouting(uint16_t routingGroup);

/**
 * @brief 设置特定PDU的路由状态
 * @param pduId PDU ID
 * @param status 路由状态 (TRUE=使能)
 * @return SOAD_E_NO_ERROR成功
 */
SoAd_ReturnType SoAd_SetPduRoutingStatus(uint16_t pduId, bool status);

/* ============================================================================
 * 标准API映射 (与PduR集成)
 * ============================================================================ */

/**
 * @brief PduR发送确认回调
 * @param pduId PDU ID
 */
void SoAd_IfTxConfirmation(uint16_t pduId);

/**
 * @brief PduR接收指示回调
 * @param pduId PDU ID
 * @param data 数据指针
 * @param length 数据长度
 */
void SoAd_IfRxIndication(uint16_t pduId, const uint8_t *data, uint16_t length);

/**
 * @brief PduR TP发送确认回调
 * @param pduId PDU ID
 * @param result 结果
 */
void SoAd_TpTxConfirmation(uint16_t pduId, SoAd_ReturnType result);

/**
 * @brief PduR TP接收指示回调
 * @param pduId PDU ID
 * @param data 数据指针
 * @param length 数据长度
 * @return E_OK成功
 */
Std_ReturnType SoAd_TpRxIndication(uint16_t pduId, const uint8_t *data, uint16_t length);

/* ============================================================================
 * 内部使用的回调注册API
 * ============================================================================ */

/**
 * @brief 注册上层发送确认回调
 * @param callback 回调函数
 */
void SoAd_RegisterTxConfirmation(SoAd_IfTxConfirmation_FuncType callback);

/**
 * @brief 注册上层接收指示回调
 * @param callback 回调函数
 */
void SoAd_RegisterRxIndication(SoAd_IfRxIndication_FuncType callback);

/**
 * @brief 注册Socket状态变化回调
 * @param callback 回调函数
 */
void SoAd_RegisterSoConModeChg(SoAd_SoConModeChg_FuncType callback);

#ifdef __cplusplus
}
#endif

#endif /* SOAD_H */
