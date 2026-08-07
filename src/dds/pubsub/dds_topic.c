/**
 * @file dds_topic.c
 * @brief DDS 主题细粒度管理实现
 * @version 1.0
 * @date 2026-08-04
 */
#include "dds_topic.h"
#include <string.h>

dds_topic_t* dds_topic_find(dds_domain_participant_t *participant,
                            const char *topic_name)
{
    if ((participant == NULL) || (topic_name == NULL)) {
        return NULL;
    }
    dds_topic_t *topic = participant->topics;
    while (topic != NULL) {
        if ((strcmp(topic->name, topic_name) == 0) && topic->active) {
            return topic;
        }
        topic = topic->next;
    }
    return NULL;
}

const char* dds_topic_get_name(dds_topic_t *topic)
{
    if (topic == NULL) {
        return NULL;
    }
    return topic->name;
}

const char* dds_topic_get_type_name(dds_topic_t *topic)
{
    if (topic == NULL) {
        return NULL;
    }
    return topic->type_name;
}
