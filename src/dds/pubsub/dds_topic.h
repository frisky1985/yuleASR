/**
 * @file dds_topic.h
 * @brief DDS 主题细粒度管理 API
 * @version 1.0
 * @date 2026-08-04
 */
#ifndef DDS_TOPIC_H
#define DDS_TOPIC_H

#include "../core/dds_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 查找主题 (按名称, 在参与者范围内) */
dds_topic_t* dds_topic_find(dds_domain_participant_t *participant,
                            const char *topic_name);

/** 获取主题名称 */
const char* dds_topic_get_name(dds_topic_t *topic);

/** 获取主题类型名 */
const char* dds_topic_get_type_name(dds_topic_t *topic);

#ifdef __cplusplus
}
#endif

#endif /* DDS_TOPIC_H */
