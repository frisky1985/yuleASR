/**
 * @file dds_subscriber.h
 * @brief DDS 订阅者细粒度管理 API
 * @version 1.0
 * @date 2026-08-04
 */
#ifndef DDS_SUBSCRIBER_H
#define DDS_SUBSCRIBER_H

#include "../core/dds_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 查找订阅者 (在参与者范围内) */
dds_subscriber_t* dds_subscriber_find(dds_domain_participant_t *participant,
                                      const rtps_guid_t *guid);

/** 获取订阅者下 reader 数量 */
uint32_t dds_subscriber_get_reader_count(dds_subscriber_t *subscriber);

#ifdef __cplusplus
}
#endif

#endif /* DDS_SUBSCRIBER_H */
