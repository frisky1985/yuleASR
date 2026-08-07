/**
 * @file dds_entity.c
 * @brief DDS 实体通用管理实现
 * @version 1.0
 * @date 2026-08-04
 *
 * 实现 dds_entity_delete: 对 publisher/subscriber/topic/writer/reader
 * 句柄执行释放 (participant 由 dds_domain.c 的 dds_delete 处理)。
 *
 * 静态分配改造 (ISO 26262 / AUTOSAR R21-11):
 * 实体全部来自 dds_domain.c 的静态实体池, 释放 = 池槽回收。
 * 此处将句柄归还给对应池 (地址范围判定)。
 */
#include "dds_entity.h"

/* dds_domain.c 提供的静态池释放接口 (同文件内可见) */
void dds_entity_pool_free_publisher(void *ptr);
void dds_entity_pool_free_subscriber(void *ptr);
void dds_entity_pool_free_topic(void *ptr);
void dds_entity_pool_free_writer(void *ptr);
void dds_entity_pool_free_reader(void *ptr);

/* 句柄 → 内部实体转换 (与 dds_domain.c 相同的指针直通约定) */

dds_ReturnCode_t dds_entity_delete(dds_EntityHandleType entity)
{
    if (entity == DDS_ENTITY_INVALID) {
        return DDS_RETCODE_BAD_PARAMETER;
    }

    void *ptr = (void*)(uintptr_t)entity;

    /* 按池回收 (非本池指针则跳过, 与 dds_domain.c 池地址范围判定一致) */
    dds_entity_pool_free_writer(ptr);
    dds_entity_pool_free_reader(ptr);
    dds_entity_pool_free_topic(ptr);
    dds_entity_pool_free_publisher(ptr);
    dds_entity_pool_free_subscriber(ptr);

    return DDS_RETCODE_OK;
}
