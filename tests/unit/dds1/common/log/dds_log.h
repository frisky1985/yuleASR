/**
 * @file dds_log.h  (tests/unit/dds1/common/log/dds_log.h)
 * @brief 测试侧日志桩头。通过 -I tests/unit/dds1/common/log 使
 *        pubsub 模块的 #include "../../common/log/dds_log.h" 命中本文件。
 *
 * NOTE: pubsub 的 4 个源文件使用 DDS_LOG_MODULE_CORE 宏(产品中无定义)以及
 * 单参数 DDS_LOG_INFO("...") 调用(与产品宏的 2+N 参数不匹配), 无法用产品
 * dds_log.h 编译。此处用宽松可变参宏替代。(产品缺陷, 已记录)
 */
#ifndef DDS1_TEST_DDS_LOG_STUB_H
#define DDS1_TEST_DDS_LOG_STUB_H

#define DDS_LOG_MODULE_CORE  "dds"
#define DDS_LOG_MODULE_COMM  "comm"
#define DDS_LOG_MODULE_PUBSUB "pubsub"

#define DDS_LOG_TRACE(...) ((void)0)
#define DDS_LOG_DEBUG(...) ((void)0)
#define DDS_LOG_INFO(...)  ((void)0)
#define DDS_LOG_WARN(...)  ((void)0)
#define DDS_LOG_ERROR(...) ((void)0)
#define DDS_LOG_AUDIT(...) ((void)0)

#endif /* DDS1_TEST_DDS_LOG_STUB_H */
