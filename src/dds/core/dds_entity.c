/**
 * @file dds_entity.c
 * @brief DDS 实体通用管理实现
 * @version 1.0
 * @date 2026-08-04
 *
 * 实现 dds_entity_delete: 对 publisher/subscriber/topic/writer/reader
 * 句柄执行释放 (participant 由 dds_domain.c 的 dds_delete 处理)。
 */
#include "dds_entity.h"
#include <stdlib.h>

/* 句柄 → 内部实体转换 (与 dds_domain.c 相同的指针直通约定) */

dds_ReturnCode_t dds_entity_delete(dds_EntityHandleType entity)
{
    if (entity == DDS_ENTITY_INVALID) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    /* 所有非 participant 实体都是 malloc 分配的结构指针。
     * 通过首字段类型区分 (guid 的 entity_id 部分):
     * 简化约定: writer/reader/topic/publisher/subscriber 均直接释放。 */
    void *ptr = (void*)(uintptr_t)entity;
    free(ptr);
    return DDS_RETCODE_OK;
}
