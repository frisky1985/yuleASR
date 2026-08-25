/**
 * @file dds_tcpip_compat.c
 * @brief TCP/IP 兼容适配层实现
 * @version 1.0
 * @date 2026-08-04
 *
 * transport.c 期望的 API 签名与 tcpip 真实 API 不一致。本层:
 * - 签名一致的 (create/close): 直接转发
 * - 签名不一致的 (bind/sendto/recvfrom/join/leave): 宏重命名真实函数后实现适配版
 */
/* @req SHALL_DDS */

#include <stdint.h>
#include <stddef.h>

/* 宏重命名真实函数, 避免与适配版同名冲突 */
#define tcpip_socket_bind    __real_tcpip_socket_bind
#define tcpip_udp_send       __real_tcpip_udp_send
#define tcpip_udp_recv       __real_tcpip_udp_recv
#define tcpip_udp_join_multicast __real_tcpip_udp_join_multicast
#define tcpip_udp_leave_multicast __real_tcpip_udp_leave_multicast
#include "../../../tcpip/tcpip_socket.h"
#include "../../../tcpip/tcpip_udp.h"
#undef tcpip_socket_bind
#undef tcpip_udp_send
#undef tcpip_udp_recv
#undef tcpip_udp_join_multicast
#undef tcpip_udp_leave_multicast

#include "dds_tcpip_compat.h"

/* 适配版: transport.c 期望的 bind(socket, &sockaddr) */
tcpip_error_t tcpip_socket_bind(tcpip_socket_id_t socket_id,
                                const tcpip_sockaddr_t *addr)
{
    if (addr == NULL) {
        return TCPIP_ERROR;
    }
    tcpip_port_t assigned = 0;
    return __real_tcpip_socket_bind(socket_id, addr->addr, addr->port, &assigned);
}

tcpip_error_t tcpip_udp_sendto(tcpip_socket_id_t socket_id,
                               const tcpip_sockaddr_t *dest,
                               const uint8_t *data,
                               uint16_t len)
{
    if ((dest == NULL) || (data == NULL)) {
        return TCPIP_ERROR;
    }
    return __real_tcpip_udp_send(socket_id, dest->addr, dest->port, data, len);
}

tcpip_error_t tcpip_udp_recvfrom(tcpip_socket_id_t socket_id,
                                 tcpip_sockaddr_t *src,
                                 uint8_t *data,
                                 uint16_t *len,
                                 uint32_t flags)
{
    (void)flags; /* 非阻塞: tcpip_udp_recv 内部处理 */
    if ((data == NULL) || (len == NULL)) {
        return TCPIP_ERROR;
    }
    return __real_tcpip_udp_recv(socket_id, src, data, *len, len);
}

tcpip_error_t tcpip_udp_join_multicast(tcpip_socket_id_t socket_id,
                                       tcpip_ip_addr_t mcast_addr)
{
    return __real_tcpip_udp_join_multicast(socket_id, mcast_addr);
}

tcpip_error_t tcpip_udp_leave_multicast(tcpip_socket_id_t socket_id,
                                        tcpip_ip_addr_t mcast_addr)
{
    return __real_tcpip_udp_leave_multicast(socket_id, mcast_addr);
}
