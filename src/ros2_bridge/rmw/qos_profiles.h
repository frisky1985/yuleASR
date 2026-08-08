#ifndef RMW_QOS_PROFILES_H
#define RMW_QOS_PROFILES_H
#include "types.h"

static inline rmw_qos_profile_t rmw_qos_profile_default(void) {
    rmw_qos_profile_t qos = {0};
    qos.reliability = RMW_QOS_POLICY_RELIABILITY_RELIABLE;
    qos.durability = RMW_QOS_POLICY_DURABILITY_VOLATILE;
    qos.history = RMW_QOS_POLICY_HISTORY_KEEP_LAST;
    qos.depth = 10;
    return qos;
}

static inline rmw_qos_profile_t rmw_qos_profile_sensor_data(void) {
    rmw_qos_profile_t qos = {0};
    qos.reliability = RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT;
    qos.durability = RMW_QOS_POLICY_DURABILITY_VOLATILE;
    qos.history = RMW_QOS_POLICY_HISTORY_KEEP_LAST;
    qos.depth = 5;
    return qos;
}

#endif
