/**
 * @file dds_reader.h
 * @brief DDS DataReader 细粒度数据路径 API
 * @version 1.0
 * @date 2026-08-04
 */
#ifndef DDS_READER_H
#define DDS_READER_H

#include "../core/dds_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 注册数据接收回调 (由 runtime 数据分发调用) */
eth_status_t dds_reader_set_data_callback(dds_data_reader_t *reader,
                                          dds_data_callback_t callback,
                                          void *user_data);

/** 向 reader 投递一个样本 (接收路径入口) */
eth_status_t dds_reader_deliver_sample(dds_data_reader_t *reader,
                                       const uint8_t *data,
                                       uint32_t len);

/** 获取 reader 已收样本数 */
uint32_t dds_reader_get_samples_received(dds_data_reader_t *reader);

#ifdef __cplusplus
}
#endif

#endif /* DDS_READER_H */
