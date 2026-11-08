/**
 * @file dds_domain.h
 * @brief DDS 域参与者标准 API
 * @version 1.0
 * @date 2026-08-04
 */
#ifndef DDS_DOMAIN_H
#define DDS_DOMAIN_H

#include "dds_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* dds_create_participant / dds_delete 等标准 API 声明在 dds_core.h */

#ifdef __cplusplus
}
#endif


void dds_entity_pool_free_publisher(void *ptr);
void dds_entity_pool_free_subscriber(void *ptr);
void dds_entity_pool_free_topic(void *ptr);
void dds_entity_pool_free_writer(void *ptr);
void dds_entity_pool_free_reader(void *ptr);
#endif /* DDS_DOMAIN_H */
