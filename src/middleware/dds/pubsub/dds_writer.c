/**
 * @file dds_writer.c
 * @brief DDS DataWriter 细粒度数据路径实现
 * @version 1.0
 * @date 2026-08-04
 */
/* @req SHALL_DDS */

#include "dds_writer.h"

uint32_t dds_writer_get_samples_written(dds_data_writer_t *writer)
{
    if (writer == NULL) {
        return 0;
    }
    return writer->samples_written;
}

dds_topic_t* dds_writer_get_topic(dds_data_writer_t *writer)
{
    if (writer == NULL) {
        return NULL;
    }
    return writer->topic;
}

eth_status_t dds_writer_set_write_callback(dds_data_writer_t *writer,
                                           void (*callback)(void *user_data),
                                           void *user_data)
{
    if (writer == NULL) {
        return ETH_INVALID_PARAM;
    }
    writer->write_callback = callback;
    writer->write_callback_user_data = user_data;
    return ETH_OK;
}
