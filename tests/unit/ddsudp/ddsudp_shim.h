/** @file ddsudp_shim.h
 * @brief udp.c 测试构建垫片 (通过 -include 强制注入, 不修改产品代码)
 *
 * NOTE(产品缺陷记录): udp.c 的 PDP_handle_announce (第 746 行) 调用了
 * MicroDDS_UDP_AddPeer, 而该函数定义在第 956 行且文件内无前置声明,
 * C99 下隐式函数声明为编译错误。产品源码无法独立编译; 此处注入
 * 前置声明使测试构建通过, 与 dds1 测试目录的 -include 垫片做法一致。
 */

#ifndef DDSUDP_SHIM_H
#define DDSUDP_SHIM_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool MicroDDS_UDP_AddPeer(uint32_t address, uint16_t port);

#ifdef __cplusplus
}
#endif

#endif /* DDSUDP_SHIM_H */
