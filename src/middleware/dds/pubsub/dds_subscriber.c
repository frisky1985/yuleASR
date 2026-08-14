/**
 * @file dds_subscriber.c
 * @brief DDS 订阅者细粒度管理实现
 * @version 1.0
 * @date 2026-08-04
 */
#include "dds_subscriber.h"

dds_subscriber_t* dds_subscriber_find(dds_domain_participant_t *participant,
                                      const rtps_guid_t *guid)
{
    if ((participant == NULL) || (guid == NULL)) {
        return NULL;
    }
    dds_subscriber_t *sub = participant->subscribers;
    while (sub != NULL) {
        if (rtps_guid_equal(&sub->guid, guid) && sub->active) {
            return sub;
        }
        sub = sub->next;
    }
    return NULL;
}

uint32_t dds_subscriber_get_reader_count(dds_subscriber_t *subscriber)
{
    if (subscriber == NULL) {
        return 0;
    }
    return subscriber->reader_count;
}
