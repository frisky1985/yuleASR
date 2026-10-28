/**
 * @file dds1_shim.h
 * @brief dds_guid_t 类型垫片。
 * NOTE: ownership.h/persistence.h 使用 dds_guid_t, 但其 include 链
 * (dds_core.h/eth_types.h) 中并未定义该类型 (定义在未被包含的
 * dds_eth_transport.h 中)。测试侧先包含本文件再包含模块头。
 * 布局与 src/middleware/dds/transport/eth/dds_eth_transport.h 一致。
 */
#ifndef DDS1_SHIM_H
#define DDS1_SHIM_H

#include <stdint.h>

#ifndef DDS_GUID_PREFIX_SIZE
#define DDS_GUID_PREFIX_SIZE 12U
#endif

typedef struct {
    uint8_t prefix[DDS_GUID_PREFIX_SIZE];
    uint8_t entity_id[4];
} dds_guid_t;

#endif /* DDS1_SHIM_H */
