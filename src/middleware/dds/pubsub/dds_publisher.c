/**
 * @file dds_publisher.c
 * @brief DDS 发布者细粒度管理实现
 * @version 1.0
 * @date 2026-08-04
 */
/* @req SHALL_DDS */

#include "dds_publisher.h"

dds_publisher_t* dds_publisher_find(dds_domain_participant_t *participant,
                                    const rtps_guid_t *guid)
{
    if ((participant == NULL) || (guid == NULL)) {
        return NULL;
    }
    dds_publisher_t *pub = participant->publishers;
    while (pub != NULL) {
        if (rtps_guid_equal(&pub->guid, guid) && pub->active) {
            return pub;
        }
        pub = pub->next;
    }
    return NULL;
}

uint32_t dds_publisher_get_writer_count(dds_publisher_t *publisher)
{
    if (publisher == NULL) {
        return 0;
    }
    return publisher->writer_count;
}
