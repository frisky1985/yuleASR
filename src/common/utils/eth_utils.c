/**
 * @file eth_utils.c
 * @brief 通用工具函数实现
 */
/* @req SHALL_UTILS */


#include "eth_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Debug.h"

/* MAC地址转换为字符串 */
void eth_mac_to_str(const eth_mac_addr_t mac, char *str, size_t len)
{
    if (len < 18U) { return; } /* xx:xx:xx:xx:xx:xx\0 */
    (void)Debug_Snprintf(str, len, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

/* 字符串转换为MAC地址 */
eth_status_t eth_str_to_mac(const char *str, eth_mac_addr_t mac)
{
    /* CERT ERR34-C: 用 strtoul 替代 sscanf，显式检测转换错误、越界与分隔符 */
    if ((str == NULL) || (mac == NULL)) {
        return ETH_ERROR;
    }

    const char *p = str;
    char *end = NULL;
    for (int i = 0; i < 6; i++) {
        unsigned long val = strtoul(p, &end, 16);
        if (end == p) {            /* 未消费任何十六进制数字 */
            return ETH_ERROR;
        }
        if (val > 0xFFUL) {        /* 字节越界 */
            return ETH_ERROR;
        }
        mac[i] = (uint8_t)val;
        p = end;
        if (i < 5) {
            if (*p != ':') {
                return ETH_ERROR;
            }
            p++;
        }
    }
    if (*p != '\0') {              /* 尾部多余字符 */
        return ETH_ERROR;
    }
    return ETH_OK;
}

/* IP地址转换为字符串 */
void eth_ip_to_str(eth_ip_addr_t ip, char *str, size_t len)
{
    if (len < 16U) { return; } /* xxx.xxx.xxx.xxx\0 */
    (void)Debug_Snprintf(str, len, "%u.%u.%u.%u",
             (unsigned)((ip >> 24) & 0xFFU),
             (unsigned)((ip >> 16) & 0xFFU),
             (unsigned)((ip >> 8) & 0xFFU),
             (unsigned)(ip & 0xFFU));
}

/* 字符串转换为IP地址 */
eth_status_t eth_str_to_ip(const char *str, eth_ip_addr_t *ip)
{
    /* CERT ERR34-C: 用 strtoul 替代 sscanf，显式检测转换错误、越界与分隔符 */
    if ((str == NULL) || (ip == NULL)) {
        return ETH_ERROR;
    }

    const char *p = str;
    char *end = NULL;
    uint32_t octet[4];
    for (int i = 0; i < 4; i++) {
        unsigned long val = strtoul(p, &end, 10);
        if (end == p) {            /* 未消费任何十进制数字 */
            return ETH_ERROR;
        }
        if (val > 255UL) {         /* 字节越界 */
            return ETH_ERROR;
        }
        octet[i] = (uint32_t)val;
        p = end;
        if (i < 3) {
            if (*p != '.') {
                return ETH_ERROR;
            }
            p++;
        }
    }
    if (*p != '\0') {              /* 尾部多余字符 */
        return ETH_ERROR;
    }
    *ip = (octet[0] << 24) | (octet[1] << 16) | (octet[2] << 8) | octet[3];
    return ETH_OK;
}

/* 计算CRC32校验和 */
uint32_t eth_crc32(const uint8_t *data, uint32_t len)
{
    static const uint32_t crc_table[256] = {
        /* CRC32查表 - 省略定义 */
    };
    uint32_t crc = 0xFFFFFFFF;
    while ((len--) != 0U) {
        crc = (crc >> 8) ^ crc_table[(crc ^ *data) & 0xFFU];
    data++;
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
