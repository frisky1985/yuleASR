/**
 * @file dds_entity.h
 * @brief DDS 实体通用管理 API
 * @version 1.0
 * @date 2026-08-04
 *
 * 提供通用实体删除/查询能力 (dds_delete 的分发目标)。
 */
#ifndef DDS_ENTITY_H
#define DDS_ENTITY_H

#include "dds_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 通用实体删除 (非 participant 类型: publisher/subscriber/topic/writer/reader)
 * @param entity 实体句柄
 * @return DDS_RETCODE_OK 成功
 */
dds_ReturnCode_t dds_entity_delete(dds_EntityHandleType entity);

#ifdef __cplusplus
}
#endif

#endif /* DDS_ENTITY_H */
