/**
 * @file dds_types.h
 * @brief DDS 类型转发头 — 兼容层
 *
 * 历史原因: rte_dds.h / ara_com_dds.h 引用 "dds_types.h"。
 * 所有 DDS 类型实际定义在 dds_core.h (含 eth_types.h 基础类型)。
 * 本文件保持向后兼容，直接转发。
 */
#ifndef DDS_TYPES_H
#define DDS_TYPES_H

#include "../../dds/core/dds_core.h"

#endif /* DDS_TYPES_H */
