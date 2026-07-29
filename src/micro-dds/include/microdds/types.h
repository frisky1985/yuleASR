/** @file types.h
 * @brief Micro-DDS 基本类型定义
 *
 * @copyright Copyright (c) 2024 YuleTech
 * @license MIT
 *
 * 遵循MISRA C:2012规范
 */

#ifndef MICRODDS_TYPES_H
#define MICRODDS_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 包含文件
 * ============================================================================ */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ============================================================================
 * 版本定义
 * ============================================================================ */
/** @brief Micro-DDS 主版本号 */
#define MICRODDS_VERSION_MAJOR 0U
/** @brief Micro-DDS 次版本号 */
#define MICRODDS_VERSION_MINOR 1U
/** @brief Micro-DDS 补丁版本号 */
#define MICRODDS_VERSION_PATCH 0U

/* ============================================================================
 * 配置常量
 * ============================================================================ */
/** @brief 最大域参与者数量 */
#ifndef MICRODDS_MAX_PARTICIPANTS
#define MICRODDS_MAX_PARTICIPANTS 4U
#endif

/** @brief 最大主题数量 */
#ifndef MICRODDS_MAX_TOPICS
#define MICRODDS_MAX_TOPICS 8U
#endif

/** @brief 最大发布者数量 */
#ifndef MICRODDS_MAX_PUBLISHERS
#define MICRODDS_MAX_PUBLISHERS 8U
#endif

/** @brief 最大订阅者数量 */
#ifndef MICRODDS_MAX_SUBSCRIBERS
#define MICRODDS_MAX_SUBSCRIBERS 8U
#endif

/** @brief 最大数据写入器数量 */
#ifndef MICRODDS_MAX_DATA_WRITERS
#define MICRODDS_MAX_DATA_WRITERS 16U
#endif

/** @brief 最大数据读取器数量 */
#ifndef MICRODDS_MAX_DATA_READERS
#define MICRODDS_MAX_DATA_READERS 16U
#endif

/** @brief 最大实例数量 */
#ifndef MICRODDS_MAX_INSTANCES
#define MICRODDS_MAX_INSTANCES 32U
#endif

/** @brief 主题名称最大长度 */
#ifndef MICRODDS_TOPIC_NAME_MAX
#define MICRODDS_TOPIC_NAME_MAX 64U
#endif

/** @brief 类型名称最大长度 */
#ifndef MICRODDS_TYPE_NAME_MAX
#define MICRODDS_TYPE_NAME_MAX 64U
#endif

/** @brief 默认域ID */
#define MICRODDS_DEFAULT_DOMAIN_ID 0U

/** @brief 无效句柄值 */
#define MICRODDS_HANDLE_NIL 0U

/* ============================================================================
 * 基本类型定义
 * ============================================================================ */

/** @brief DDS返回码类型 */
typedef enum {
    DDS_RETCODE_OK = 0,                 /**< 成功 */
    DDS_RETCODE_ERROR = 1,              /**< 一般错误 */
    DDS_RETCODE_UNSUPPORTED = 2,        /**< 不支持的操作 */
    DDS_RETCODE_BAD_PARAMETER = 3,      /**< 无效参数 */
    DDS_RETCODE_PRECONDITION_NOT_MET = 4, /**< 前提条件未满足 */
    DDS_RETCODE_OUT_OF_RESOURCES = 5,   /**< 资源不足 */
    DDS_RETCODE_NOT_ENABLED = 6,        /**< 实体未启用 */
    DDS_RETCODE_IMMUTABLE_POLICY = 7,   /**< 不可变更的策略 */
    DDS_RETCODE_INCONSISTENT_POLICY = 8, /**< 不一致的策略 */
    DDS_RETCODE_ALREADY_DELETED = 9,    /**< 实体已被删除 */
    DDS_RETCODE_TIMEOUT = 10,           /**< 超时 */
    DDS_RETCODE_NO_DATA = 11,           /**< 无数据 */
    DDS_RETCODE_ILLEGAL_OPERATION = 12  /**< 非法操作 */
} DDS_ReturnCode_t;

/** @brief 实例状态 */
typedef enum {
    DDS_ALIVE_INSTANCE_STATE = 0x01U,      /**< 活跃实例 */
    DDS_NOT_ALIVE_DISPOSED_INSTANCE_STATE = 0x02U, /**< 已处置实例 */
    DDS_NOT_ALIVE_NO_WRITERS_INSTANCE_STATE = 0x04U /**< 无写入器实例 */
} DDS_InstanceStateKind;

/** @brief 样本状态 */
typedef enum {
    DDS_READ_SAMPLE_STATE = 0x01U,    /**< 已读样本 */
    DDS_NOT_READ_SAMPLE_STATE = 0x02U /**< 未读样本 */
} DDS_SampleStateKind;

/** @brief 视图状态 */
typedef enum {
    DDS_NEW_VIEW_STATE = 0x01U,    /**< 新视图 */
    DDS_NOT_NEW_VIEW_STATE = 0x02U /**< 非新视图 */
} DDS_ViewStateKind;

/** @brief 样本信息结构 */
typedef struct {
    DDS_SampleStateKind sample_state;   /**< 样本状态 */
    DDS_ViewStateKind view_state;       /**< 视图状态 */
    DDS_InstanceStateKind instance_state; /**< 实例状态 */
    int32_t disposed_generation_count;  /**< 处置代计数 */
    int32_t no_writers_generation_count; /**< 无写入器代计数 */
    uint32_t sample_rank;               /**< 样本排名 */
    uint32_t generation_rank;           /**< 代排名 */
    uint32_t absolute_generation_rank;  /**< 绝对代排名 */
    void *source_timestamp;             /**< 源时间戳（平台相关） */
    uint32_t instance_handle;           /**< 实例句柄 */
    uint32_t publication_handle;        /**< 发布句柄 */
} DDS_SampleInfo;

/** @brief 状态掩码（用于选择性通知） */
typedef uint32_t DDS_StatusMask;

/** @brief 状态标识 */
#define DDS_INCONSISTENT_TOPIC_STATUS     0x0001U
#define DDS_OFFERED_DEADLINE_MISSED_STATUS 0x0002U
#define DDS_REQUESTED_DEADLINE_MISSED_STATUS 0x0004U
#define DDS_OFFERED_INCOMPATIBLE_QOS_STATUS 0x0010U
#define DDS_REQUESTED_INCOMPATIBLE_QOS_STATUS 0x0020U
#define DDS_SAMPLE_LOST_STATUS            0x0040U
#define DDS_SAMPLE_REJECTED_STATUS        0x0080U
#define DDS_DATA_ON_READERS_STATUS        0x0100U
#define DDS_DATA_AVAILABLE_STATUS         0x0200U
#define DDS_LIVELINESS_LOST_STATUS        0x0400U
#define DDS_LIVELINESS_CHANGED_STATUS     0x0800U
#define DDS_PUBLICATION_MATCHED_STATUS    0x1000U
#define DDS_SUBSCRIPTION_MATCHED_STATUS   0x2000U

/** @brief 句柄类型 */
typedef uint32_t DDS_InstanceHandle_t;

/** @brief 时间类型（纳秒级） */
typedef int64_t DDS_Time_t;

/** @brief 持续时间类型（纳秒级） */
typedef int64_t DDS_Duration_t;

/** @brief 无限持续时间 */
#define DDS_DURATION_INFINITE ((DDS_Duration_t)0x7FFFFFFFFFFFFFFFLL)
/** @brief 零持续时间 */
#define DDS_DURATION_ZERO ((DDS_Duration_t)0LL)

/* ============================================================================
 * 域ID类型
 * ============================================================================ */
/** @brief 域标识符类型 */
typedef uint32_t DDS_DomainId_t;

/* ============================================================================
 * 序列类型（简化版）
 * ============================================================================ */
/** @brief 通用序列结构 */
typedef struct {
    uint32_t _maximum;  /**< 最大容量 */
    uint32_t _length;   /**< 当前长度 */
    void *_buffer;      /**< 缓冲区指针 */
    bool _release;      /**< 是否释放缓冲区 */
} DDS_Sequence;

/** @brief 字符串序列 */
typedef struct {
    uint32_t _maximum;
    uint32_t _length;
    char **_buffer;
    bool _release;
} DDS_StringSeq;

/* ============================================================================
 * 内联函数定义
 * ============================================================================ */
/**
 * @brief 检查返回码是否表示成功
 * @param rc 返回码
 * @return true 表示成功
 */
static inline bool DDS_RETCODE_IS_OK(DDS_ReturnCode_t rc) {
    return (rc == DDS_RETCODE_OK);
}

/**
 * @brief 检查返回码是否表示错误
 * @param rc 返回码
 * @return true 表示错误
 */
static inline bool DDS_RETCODE_IS_ERROR(DDS_ReturnCode_t rc) {
    return (rc != DDS_RETCODE_OK);
}

#ifdef __cplusplus
}
#endif

#endif /* MICRODDS_TYPES_H */
