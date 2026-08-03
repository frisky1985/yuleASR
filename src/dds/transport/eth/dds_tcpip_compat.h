/**
 * @file dds_tcpip_compat.h
 * @brief TCP/IP 兼容适配层 — 桥接 dds_eth_transport 自定义 API 与 tcpip 真实 API
 * @version 1.0
 * @date 2026-08-04
 *
 * dds_eth_transport.c (归档) 调用的 tcpip API 签名与 tcpip_socket.h/tcpip_udp.h
 * 实际声明不一致。本层提供 transport 期望的 API 名/签名，内部转发到真实实现。
 *
 * 注意: 本头不 include tcpip_socket.h (会与同名函数冲突), 仅引入类型定义。
 */
#ifndef DDS_TCPIP_COMPAT_H
#define DDS_TCPIP_COMPAT_H

#include <stdint.h>
#include "../../../tcpip/tcpip_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* transport.c 期望的 API (兼容签名, 实现在 dds_tcpip_compat.c) */

/* 签名与真实头一致, 直接转发 */
tcpip_error_t tcpip_socket_create(tcpip_socket_type_t type,
                                  tcpip_socket_id_t *socket_id);

tcpip_error_t tcpip_socket_close(tcpip_socket_id_t socket_id);

tcpip_error_t tcpip_socket_bind(tcpip_socket_id_t socket_id,
                                const tcpip_sockaddr_t *addr);

tcpip_error_t tcpip_udp_sendto(tcpip_socket_id_t socket_id,
                               const tcpip_sockaddr_t *dest,
                               const uint8_t *data,
                               uint16_t len);

tcpip_error_t tcpip_udp_recvfrom(tcpip_socket_id_t socket_id,
                                 tcpip_sockaddr_t *src,
                                 uint8_t *data,
                                 uint16_t *len,
                                 uint32_t flags);

tcpip_error_t tcpip_udp_join_multicast(tcpip_socket_id_t socket_id,
                                       tcpip_ip_addr_t mcast_addr);

tcpip_error_t tcpip_udp_leave_multicast(tcpip_socket_id_t socket_id,
                                        tcpip_ip_addr_t mcast_addr);

#ifdef __cplusplus
}
#endif

#endif /* DDS_TCPIP_COMPAT_H */
