/**
 * @file dds_publisher.h
 * @brief DDS 发布者细粒度管理 API
 * @version 1.0
 * @date 2026-08-04
 */
#ifndef DDS_PUBLISHER_H
#define DDS_PUBLISHER_H

#include "../core/dds_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 查找发布者 (在参与者范围内) */
dds_publisher_t* dds_publisher_find(dds_domain_participant_t *participant,
                                    const rtps_guid_t *guid);

/** 获取发布者下 writer 数量 */
uint32_t dds_publisher_get_writer_count(dds_publisher_t *publisher);

#ifdef __cplusplus
}
#endif

#endif /* DDS_PUBLISHER_H */
