/**
 * @file dds_reader.c
 * @brief DDS DataReader 细粒度数据路径实现
 * @version 1.0
 * @date 2026-08-04
 */
#include "dds_reader.h"
#include <string.h>

eth_status_t dds_reader_set_data_callback(dds_data_reader_t *reader,
                                          dds_data_callback_t callback,
                                          void *user_data)
{
    if (reader == NULL) {
        return ETH_INVALID_PARAM;
    }
    reader->data_callback = callback;
    reader->data_callback_user_data = user_data;
    return ETH_OK;
}

eth_status_t dds_reader_deliver_sample(dds_data_reader_t *reader,
                                       const uint8_t *data,
                                       uint32_t len)
{
    if ((reader == NULL) || !reader->active || (data == NULL) || (len == 0U)) {
        return ETH_INVALID_PARAM;
    }

    /* 写入接收缓冲 (环形语义简化: 覆盖式单缓冲) */
    if ((reader->receive_buffer == NULL) ||
        (reader->receive_buffer_size < len)) {
        return ETH_NO_MEMORY;
    }

    memcpy(reader->receive_buffer, data, len);
    reader->samples_received++;
    reader->last_read_time = dds_get_time();

    /* 触发数据回调 */
    if (reader->data_callback != NULL) {
        reader->data_callback(data, len, reader->data_callback_user_data);
    }
    return ETH_OK;
}

uint32_t dds_reader_get_samples_received(dds_data_reader_t *reader)
{
    if (reader == NULL) {
        return 0;
    }
    return reader->samples_received;
}
