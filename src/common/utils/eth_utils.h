/**
 * @file eth_utils.h
 * @brief 通用工具函数头文件
 */

#ifndef ETH_UTILS_H
#define ETH_UTILS_H

#include "../types/eth_types.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MAC地址操作 */
void eth_mac_to_str(const eth_mac_addr_t mac, char *str, size_t len);
eth_status_t eth_str_to_mac(const char *str, eth_mac_addr_t mac);

/* IP地址操作 */
void eth_ip_to_str(eth_ip_addr_t ip, char *str, size_t len);
eth_status_t eth_str_to_ip(const char *str, eth_ip_addr_t *ip);

/* 校验和计算 */
uint32_t eth_crc32(const uint8_t *data, uint32_t len);
uint16_t eth_checksum(const uint8_t *data, uint32_t len);

/* 内存管理 */
void* eth_aligned_malloc(size_t size, size_t alignment);
void eth_aligned_free(void *ptr);

/* 时间工具 */
uint64_t eth_get_timestamp_us(void);
void eth_delay_us(uint32_t us);
void eth_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* ETH_UTILS_H */
