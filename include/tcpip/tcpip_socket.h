/**
 * @file tcpip_socket.h
 * @brief Socket API接口
 * @version 1.0
 * @date 2026-04-26
 *
 * @note 符合AUTOSAR TcpIp模块规范
 * @note MISRA C:2012 compliant
 * @note POSIX Socket API风格
 */

#ifndef TCPIP_SOCKET_H
#define TCPIP_SOCKET_H

#include "tcpip_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Socket事件定义
 * ============================================================================ */

/** Socket事件类型 */
#define TCPIP_SOCK_EVT_CONNECTED      0x01u   /**< 连接已建立 */
#define TCPIP_SOCK_EVT_DISCONNECTED   0x02u   /**< 连接已断开 */
#define TCPIP_SOCK_EVT_DATA_RECEIVED  0x04u   /**< 数据已到达 */
#define TCPIP_SOCK_EVT_DATA_SENT      0x08u   /**< 数据已发送 */
#define TCPIP_SOCK_EVT_ACCEPT         0x10u   /**< 有新连接请求 */
#define TCPIP_SOCK_EVT_ERROR          0x20u   /**< 错误发生 */

/* ============================================================================
 * Socket生命周期API
 * ============================================================================ */

/**
 * @brief 创建Socket
 * @param type Socket类型 (TCP/UDP)
 * @param socket_id Socket ID输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_create(tcpip_socket_type_t type,
                                   tcpip_socket_id_t *socket_id);

/**
 * @brief 关闭Socket
 * @param socket_id Socket ID
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_close(tcpip_socket_id_t socket_id);

/**
 * @brief 强制关闭Socket
 * @param socket_id Socket ID
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_abort(tcpip_socket_id_t socket_id);

/**
 * @brief 绑定地址和端口
 * @param socket_id Socket ID
 * @param local_addr 本地IP地址 (0 = ANY)
 * @param local_port 本地端口 (0 = 自动分配)
 * @param assigned_port 分配的端口号输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_bind(tcpip_socket_id_t socket_id,
                                 tcpip_ip_addr_t local_addr,
                                 tcpip_port_t local_port,
                                 tcpip_port_t *assigned_port);

/* ============================================================================
 * TCP Socket API
 * ============================================================================ */

/**
 * @brief 监听入站连接
 * @param socket_id Socket ID
 * @param backlog 最大待处理连接数
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_listen(tcpip_socket_id_t socket_id,
                                   uint8_t backlog);

/**
 * @brief 接受入站连接
 * @param socket_id 监听Socket ID
 * @param new_socket 新连接Socket ID输出
 * @param remote_addr 远程地址输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_accept(tcpip_socket_id_t socket_id,
                                   tcpip_socket_id_t *new_socket,
                                   tcpip_sockaddr_t *remote_addr);

/**
 * @brief 主动连接到远程服务器
 * @param socket_id Socket ID
 * @param remote_addr 远程地址
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_connect(tcpip_socket_id_t socket_id,
                                    const tcpip_sockaddr_t *remote_addr);

/**
 * @brief 断开连接
 * @param socket_id Socket ID
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_disconnect(tcpip_socket_id_t socket_id);

/**
 * @brief 检查连接状态
 * @param socket_id Socket ID
 * @param connected 连接状态输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_is_connected(tcpip_socket_id_t socket_id,
                                         bool *connected);

/* ============================================================================
 * 数据传输API
 * ============================================================================ */

/**
 * @brief 发送数据
 * @param socket_id Socket ID
 * @param data 数据指针
 * @param len 数据长度
 * @param sent_len 实际发送长度输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_send(tcpip_socket_id_t socket_id,
                                 const uint8_t *data,
                                 uint16_t len,
                                 uint16_t *sent_len);

/**
 * @brief 接收数据
 * @param socket_id Socket ID
 * @param buffer 缓冲区
 * @param buffer_size 缓冲区大小
 * @param received_len 实际接收长度输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_recv(tcpip_socket_id_t socket_id,
                                 uint8_t *buffer,
                                 uint16_t buffer_size,
                                 uint16_t *received_len);

/**
 * @brief 发送数据到指定地址 (UDP用)
 * @param socket_id Socket ID
 * @param dest_addr 目标地址
 * @param data 数据指针
 * @param len 数据长度
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_sendto(tcpip_socket_id_t socket_id,
                                   const tcpip_sockaddr_t *dest_addr,
                                   const uint8_t *data,
                                   uint16_t len);

/**
 * @brief 从指定地址接收数据 (UDP用)
 * @param socket_id Socket ID
 * @param src_addr 源地址输出
 * @param buffer 缓冲区
 * @param buffer_size 缓冲区大小
 * @param received_len 实际接收长度输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_recvfrom(tcpip_socket_id_t socket_id,
                                     tcpip_sockaddr_t *src_addr,
                                     uint8_t *buffer,
                                     uint16_t buffer_size,
                                     uint16_t *received_len);

/**
 * @brief 检查是否有可读数据
 * @param socket_id Socket ID
 * @param available_len 可读长度输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_get_rx_avail(tcpip_socket_id_t socket_id,
                                         uint16_t *available_len);

/**
 * @brief 检查是否可发送
 * @param socket_id Socket ID
 * @param tx_space 发送空间输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_get_tx_space(tcpip_socket_id_t socket_id,
                                         uint16_t *tx_space);

/* ============================================================================
 * Socket选项配置API
 * ============================================================================ */

/**
 * @brief 设置Socket选项
 * @param socket_id Socket ID
 * @param option 选项名
 * @param value 选项值指针
 * @param value_len 选项值长度
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_setopt(tcpip_socket_id_t socket_id,
                                   uint8_t option,
                                   const void *value,
                                   uint8_t value_len);

/**
 * @brief 获取Socket选项
 * @param socket_id Socket ID
 * @param option 选项名
 * @param value 选项值指针
 * @param value_len 选项值长度输入/输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_getopt(tcpip_socket_id_t socket_id,
                                   uint8_t option,
                                   void *value,
                                   uint8_t *value_len);

/** Socket选项定义 */
#define TCPIP_SOCK_OPT_BROADCAST      0x01u   /**< 广播使能 */
#define TCPIP_SOCK_OPT_REUSEADDR      0x02u   /**< 地址重用 */
#define TCPIP_SOCK_OPT_KEEPALIVE      0x03u   /**< 保活使能 */
#define TCPIP_SOCK_OPT_RCVTIMEO       0x04u   /**< 接收超时 */
#define TCPIP_SOCK_OPT_SNDTIMEO       0x05u   /**< 发送超时 */
#define TCPIP_SOCK_OPT_RCVBUF         0x06u   /**< 接收缓冲区大小 */
#define TCPIP_SOCK_OPT_SNDBUF         0x07u   /**< 发送缓冲区大小 */
#define TCPIP_SOCK_OPT_NODELAY        0x08u   /**< 禁用Nagle */
#define TCPIP_SOCK_OPT_MCAST_TTL      0x09u   /**< 多播TTL */
#define TCPIP_SOCK_OPT_MCAST_LOOP     0x0Au   /**< 多播环回 */

/* ============================================================================
 * Socket事件回调API
 * ============================================================================ */

/**
 * @brief 注册Socket事件回调
 * @param socket_id Socket ID
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_register_callback(tcpip_socket_id_t socket_id,
                                              tcpip_socket_event_cb_t callback,
                                              void *user_data);

/**
 * @brief 注销Socket事件回调
 * @param socket_id Socket ID
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_unregister_callback(tcpip_socket_id_t socket_id);

/**
 * @brief 设置Socket为非阻塞模式
 * @param socket_id Socket ID
 * @param nonblock true非阻塞，false阻塞
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_set_nonblocking(tcpip_socket_id_t socket_id,
                                            bool nonblock);

/* ============================================================================
 * Socket信息查询API
 * ============================================================================ */

/**
 * @brief 获取Socket状态
 * @param socket_id Socket ID
 * @param state 状态输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_get_state(tcpip_socket_id_t socket_id,
                                      tcpip_socket_state_t *state);

/**
 * @brief 获取Socket类型
 * @param socket_id Socket ID
 * @param type 类型输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_get_type(tcpip_socket_id_t socket_id,
                                     tcpip_socket_type_t *type);

/**
 * @brief 获取本地地址
 * @param socket_id Socket ID
 * @param local_addr 本地地址输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_get_local_addr(tcpip_socket_id_t socket_id,
                                           tcpip_sockaddr_t *local_addr);

/**
 * @brief 获取远程地址
 * @param socket_id Socket ID
 * @param remote_addr 远程地址输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_get_remote_addr(tcpip_socket_id_t socket_id,
                                            tcpip_sockaddr_t *remote_addr);

/* ============================================================================
 * Socket集操作API (Select机制)
 * ============================================================================ */

/** Socket集合结构 */
typedef struct {
    uint32_t bitmap;  /**< Socket位图 (max 32 sockets) */
} tcpip_fd_set_t;

/**
 * @brief 清空Socket集合
 * @param set Socket集合
 */
void tcpip_fd_zero(tcpip_fd_set_t *set);

/**
 * @brief 设置Socket到集合
 * @param set Socket集合
 * @param socket_id Socket ID
 */
void tcpip_fd_set(tcpip_fd_set_t *set, tcpip_socket_id_t socket_id);

/**
 * @brief 从集合清除Socket
 * @param set Socket集合
 * @param socket_id Socket ID
 */
void tcpip_fd_clr(tcpip_fd_set_t *set, tcpip_socket_id_t socket_id);

/**
 * @brief 检查Socket是否在集合中
 * @param set Socket集合
 * @param socket_id Socket ID
 * @return true在集合中
 */
bool tcpip_fd_isset(tcpip_fd_set_t *set, tcpip_socket_id_t socket_id);

/**
 * @brief 多路复用等待Socket事件
 * @param nfds 最大Socket ID + 1
 * @param readfds 可读Socket集合
 * @param writefds 可写Socket集合
 * @param exceptfds 异常Socket集合
 * @param timeout_ms 超时(毫秒, 0=立即返回, 0xFFFFFFFF=永久阻塞)
 * @param ready_count 准备就绪的Socket数输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_select(uint8_t nfds,
                            tcpip_fd_set_t *readfds,
                            tcpip_fd_set_t *writefds,
                            tcpip_fd_set_t *exceptfds,
                            uint32_t timeout_ms,
                            uint8_t *ready_count);

/* ============================================================================
 * Socket管理内部API
 * ============================================================================ */

/**
 * @brief 初始化Socket层
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_init(void);

/**
 * @brief 反初始化Socket层
 */
void tcpip_socket_deinit(void);

/**
 * @brief Socket主循环处理
 * @param elapsed_ms 经过毫秒数
 */
void tcpip_socket_main_function(uint32_t elapsed_ms);

/**
 * @brief 触发Socket事件
 * @param socket_id Socket ID
 * @param event 事件类型
 */
void tcpip_socket_trigger_event(tcpip_socket_id_t socket_id, uint8_t event);

/**
 * @brief 获取Socket配置信息
 * @param socket_id Socket ID
 * @param config 配置输出
 * @return TCPIP_OK成功
 */
tcpip_error_t tcpip_socket_get_config(tcpip_socket_id_t socket_id,
                                       tcpip_socket_config_t *config);

/**
 * @brief 验证Socket ID有效性
 * @param socket_id Socket ID
 * @return TCPIP_OK有效
 */
tcpip_error_t tcpip_socket_validate(tcpip_socket_id_t socket_id);

/**
 * @brief 获取空闲Socket数
 * @return 空闲Socket数
 */
uint8_t tcpip_socket_get_free_count(void);

/**
 * @brief 获取活动Socket数
 * @return 活动Socket数
 */
uint8_t tcpip_socket_get_active_count(void);

#ifdef __cplusplus
}
#endif

#endif /* TCPIP_SOCKET_H */
