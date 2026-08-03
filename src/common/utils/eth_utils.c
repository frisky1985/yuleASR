/**
 * @file eth_utils.c
 * @brief 通用工具函数实现
 */

#include "eth_utils.h"
#include <string.h>
#include <stdio.h>

/* MAC地址转换为字符串 */
void eth_mac_to_str(const eth_mac_addr_t mac, char *str, size_t len)
{
    if (len < 18) return; /* xx:xx:xx:xx:xx:xx\0 */
    snprintf(str, len, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

/* 字符串转换为MAC地址 */
eth_status_t eth_str_to_mac(const char *str, eth_mac_addr_t mac)
{
    int ret = sscanf(str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                     &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    return (ret == 6) ? ETH_OK : ETH_ERROR;
}

/* IP地址转换为字符串 */
void eth_ip_to_str(eth_ip_addr_t ip, char *str, size_t len)
{
    if (len < 16) return; /* xxx.xxx.xxx.xxx\0 */
    snprintf(str, len, "%u.%u.%u.%u",
             (unsigned)((ip >> 24) & 0xFF),
             (unsigned)((ip >> 16) & 0xFF),
             (unsigned)((ip >> 8) & 0xFF),
             (unsigned)(ip & 0xFF));
}

/* 字符串转换为IP地址 */
eth_status_t eth_str_to_ip(const char *str, eth_ip_addr_t *ip)
{
    unsigned int a, b, c, d;
    int ret = sscanf(str, "%u.%u.%u.%u", &a, &b, &c, &d);
    if (ret != 4 || a > 255 || b > 255 || c > 255 || d > 255) {
        return ETH_ERROR;
    }
    *ip = (a << 24) | (b << 16) | (c << 8) | d;
    return ETH_OK;
}

/* 计算CRC32校验和 */
uint32_t eth_crc32(const uint8_t *data, uint32_t len)
{
    static const uint32_t crc_table[256] = {
        /* CRC32查表 - 省略定义 */
    };
    uint32_t crc = 0xFFFFFFFF;
    while (len--) {
        crc = (crc >> 8) ^ crc_table[(crc ^ *data++) & 0xFF];
    }
    return crc ^ 0xFFFFFFFF;
}

/* 内存对齐分配 */
void* eth_aligned_malloc(size_t size, size_t alignment)
{
    /* 简化实现，实际项目中需要更完善的实现 */
    (void)alignment;
    return NULL;
}

/* 对齐内存释放 */
void eth_aligned_free(void *ptr)
{
    (void)ptr;
}
