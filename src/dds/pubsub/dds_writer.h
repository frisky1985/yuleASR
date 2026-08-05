/**
 * @file dds_writer.h
 * @brief DDS DataWriter 细粒度数据路径 API
 * @version 1.0
 * @date 2026-08-04
 */
#ifndef DDS_WRITER_H
#define DDS_WRITER_H

#include "../core/dds_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 获取 writer 已写样本数 */
uint32_t dds_writer_get_samples_written(dds_data_writer_t *writer);

/** 获取 writer 关联主题 */
dds_topic_t* dds_writer_get_topic(dds_data_writer_t *writer);

/** 注册写完成回调 */
eth_status_t dds_writer_set_write_callback(dds_data_writer_t *writer,
                                           void (*callback)(void *user_data),
                                           void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* DDS_WRITER_H */
