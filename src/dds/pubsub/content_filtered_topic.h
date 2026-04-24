/**
 * @file content_filtered_topic.h
 * @brief DDS内容过滤主题 - 支持SQL92过滤表达式
 * @version 1.0
 * @date 2026-04-24
 * 
 * 车载要求：
 * - 确定性过滤执行时间 < 50us
 * - 支持ASIL-D安全等级
 * - 并行多过滤器支持
 */

#ifndef CONTENT_FILTERED_TOPIC_H
#define CONTENT_FILTERED_TOPIC_H

#include "../../common/types/eth_types.h"
#include "../core/dds_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 常量定义
 * ============================================================================ */

/** 最大过滤条件长度 */
#define CFT_MAX_FILTER_LENGTH      512
/** 最大参数字符串长度 */
#define CFT_MAX_PARAM_LENGTH       256
/** 最大并行过滤器数量 */
#define CFT_MAX_PARALLEL_FILTERS   8
/** 最大字段深度 */
#define CFT_MAX_FIELD_DEPTH        8
/** 最大谓词数量 */
#define CFT_MAX_PREDICATES         16
/** 最大表达式复杂度(操作符数量) */
#define CFT_MAX_EXPRESSION_OPS     32

/* ============================================================================
 * 内容过滤主题类型定义
 * ============================================================================ */

/** 过滤操作符类型 */
typedef enum {
    CFT_OP_EQ = 0,           /**< 等于 = */
    CFT_OP_NE,               /**< 不等于 != */
    CFT_OP_LT,               /**< 小于 < */
    CFT_OP_LE,               /**< 小于等于 <= */
    CFT_OP_GT,               /**< 大于 > */
    CFT_OP_GE,               /**< 大于等于 >= */
    CFT_OP_LIKE,             /**< 模糊匹配 LIKE */
    CFT_OP_BETWEEN,          /**< 范围 BETWEEN */
    CFT_OP_IN,               /**< 在集合中 IN */
    CFT_OP_AND,              /**< 逻辑与 AND */
    CFT_OP_OR,               /**< 逻辑或 OR */
    CFT_OP_NOT,              /**< 逻辑非 NOT */
} cft_operator_t;

/** 字段数据类型 */
typedef enum {
    CFT_TYPE_INT8 = 0,
    CFT_TYPE_UINT8,
    CFT_TYPE_INT16,
    CFT_TYPE_UINT16,
    CFT_TYPE_INT32,
    CFT_TYPE_UINT32,
    CFT_TYPE_INT64,
    CFT_TYPE_UINT64,
    CFT_TYPE_FLOAT32,
    CFT_TYPE_FLOAT64,
    CFT_TYPE_BOOLEAN,
    CFT_TYPE_STRING,
    CFT_TYPE_ENUM,
} cft_field_type_t;

/** 过滤谓词(基本条件单元) */
typedef struct {
    char field_name[64];              /**< 字段名 */
    uint8_t field_depth;              /**< 字段嵌套深度 */
    cft_operator_t op;                /**< 操作符 */
    cft_field_type_t value_type;      /**< 值类型 */
    union {
        int64_t i64_val;
        uint64_t u64_val;
        double f64_val;
        bool bool_val;
        char str_val[128];
        struct {
            int64_t min_val;
            int64_t max_val;
        } range;
    } value;                          /**< 比较值 */
    bool is_param;                    /**< 是否为参数化(?) */
    uint8_t param_index;              /**< 参数索引 */
} cft_predicate_t;

/** 表达式AST节点 */
typedef struct cft_ast_node {
    cft_operator_t op;                /**< 节点操作符 */
    bool is_leaf;                     /**< 是否为叶子节点(谓词) */
    union {
        cft_predicate_t predicate;    /**< 叶子节点: 谓词 */
        struct {
            struct cft_ast_node *left;
            struct cft_ast_node *right;
        } branch;                     /**< 分支节点: 左右子树 */
    } data;
} cft_ast_node_t;

/** 内容过滤主题配置 */
typedef struct {
    char filter_expression[CFT_MAX_FILTER_LENGTH];  /**< 过滤表达式(SQL) */
    char expression_params[CFT_MAX_PREDICATES][CFT_MAX_PARAM_LENGTH]; /**< 参数值 */
    uint8_t param_count;              /**< 参数数量 */
    bool enable_optimization;         /**< 启用查询优化 */
    uint32_t max_exec_time_us;        /**< 最大执行时间限制(微秒) */
} cft_config_t;

/** 内容过滤主题句柄(不透明) */
typedef struct cft_handle cft_handle_t;

/** 字段访问器 - 用于从数据中提取字段值 */
typedef struct {
    char field_name[64];
    cft_field_type_t type;
    uint32_t offset;                  /**< 字段在结构体中的偏移 */
    uint32_t size;                    /**< 字段大小 */
} cft_field_accessor_t;

/** 类型描述符 - 定义数据类型的字段结构 */
typedef struct {
    cft_field_accessor_t fields[CFT_MAX_FIELD_DEPTH];
    uint8_t field_count;
    uint32_t type_size;
} cft_type_descriptor_t;

/** 过滤统计信息 */
typedef struct {
    uint64_t total_samples;           /**< 总样本数 */
    uint64_t filtered_samples;        /**< 过滤通过的样本数 */
    uint64_t rejected_samples;        /**< 被拒绝的样本数 */
    uint32_t avg_exec_time_us;        /**< 平均执行时间(微秒) */
    uint32_t max_exec_time_us;        /**< 最大执行时间(微秒) */
    uint32_t timeout_count;           /**< 超时次数 */
} cft_stats_t;

/* ============================================================================
 * API函数声明
 * ============================================================================ */

/**
 * @brief 初始化内容过滤主题模块
 * @return ETH_OK成功
 */
eth_status_t cft_init(void);

/**
 * @brief 反初始化内容过滤主题模块
 */
void cft_deinit(void);

/**
 * @brief 创建内容过滤主题
 * @param related_topic 关联的基础主题
 * @param name 过滤主题名称
 * @param config 过滤配置
 * @param type_desc 数据类型描述符
 * @return 过滤主题句柄
 */
cft_handle_t* cft_create(dds_topic_t *related_topic, 
                          const char *name,
                          const cft_config_t *config,
                          const cft_type_descriptor_t *type_desc);

/**
 * @brief 删除内容过滤主题
 * @param cft 过滤主题句柄
 * @return ETH_OK成功
 */
eth_status_t cft_delete(cft_handle_t *cft);

/**
 * @brief 设置过滤表达式参数
 * @param cft 过滤主题句柄
 * @param params 参数数组
 * @param param_count 参数数量
 * @return ETH_OK成功
 */
eth_status_t cft_set_parameters(cft_handle_t *cft, 
                                  const char **params, 
                                  uint8_t param_count);

/**
 * @brief 动态更新过滤表达式
 * @param cft 过滤主题句柄
 * @param expression 新的过滤表达式
 * @return ETH_OK成功
 */
eth_status_t cft_update_expression(cft_handle_t *cft, const char *expression);

/**
 * @brief 评估样本是否匹配过滤条件
 * @param cft 过滤主题句柄
 * @param sample 样本数据指针
 * @param sample_size 样本大小
 * @param match 输出: 是否匹配
 * @return ETH_OK成功
 */
eth_status_t cft_evaluate(cft_handle_t *cft, 
                          const void *sample, 
                          uint32_t sample_size,
                          bool *match);

/**
 * @brief 批量评估多个样本
 * @param cft 过滤主题句柄
 * @param samples 样本数组
 * @param sample_sizes 样本大小数组
 * @param count 样本数量
 * @param matches 匹配结果数组输出
 * @return ETH_OK成功
 */
eth_status_t cft_evaluate_batch(cft_handle_t *cft,
                                const void **samples,
                                const uint32_t *sample_sizes,
                                uint32_t count,
                                bool *matches);

/**
 * @brief 并行评估 - 使用多过滤器
 * @param cfts 过滤器句柄数组
 * @param cft_count 过滤器数量
 * @param sample 样本数据
 * @param sample_size 样本大小
 * @param match_results 各过滤器匹配结果
 * @return ETH_OK成功
 */
eth_status_t cft_evaluate_parallel(cft_handle_t **cfts,
                                   uint8_t cft_count,
                                   const void *sample,
                                   uint32_t sample_size,
                                   bool *match_results);

/**
 * @brief 获取过滤统计信息
 * @param cft 过滤主题句柄
 * @param stats 统计信息输出
 * @return ETH_OK成功
 */
eth_status_t cft_get_stats(cft_handle_t *cft, cft_stats_t *stats);

/**
 * @brief 重置统计信息
 * @param cft 过滤主题句柄
 * @return ETH_OK成功
 */
eth_status_t cft_reset_stats(cft_handle_t *cft);

/**
 * @brief 解析SQL过滤表达式为AST
 * @param expression SQL表达式字符串
 * @param ast_root AST根节点输出
 * @return ETH_OK成功
 */
eth_status_t cft_parse_expression(const char *expression, cft_ast_node_t **ast_root);

/**
 * @brief 释放AST节点
 * @param node AST节点
 */
void cft_free_ast(cft_ast_node_t *node);

/**
 * @brief 编译类型描述符(优化字段访问)
 * @param type_desc 类型描述符
 * @return ETH_OK成功
 */
eth_status_t cft_compile_type_descriptor(cft_type_descriptor_t *type_desc);

/**
 * @brief 获取字段值(支持嵌套字段)
 * @param sample 样本数据
 * @param accessor 字段访问器
 * @param out_value 输出值
 * @return ETH_OK成功
 */
eth_status_t cft_get_field_value(const void *sample,
                                  const cft_field_accessor_t *accessor,
                                  void *out_value);

/**
 * @brief 启用ASIL安全模式(确定性执行)
 * @param cft 过滤主题句柄
 * @param asil_level ASIL等级(0-4)
 * @return ETH_OK成功
 */
eth_status_t cft_enable_asil_mode(cft_handle_t *cft, uint8_t asil_level);

/**
 * @brief 预编译过滤表达式(提升性能)
 * @param cft 过滤主题句柄
 * @return ETH_OK成功
 */
eth_status_t cft_compile_expression(cft_handle_t *cft);

#ifdef __cplusplus
}
#endif

#endif /* CONTENT_FILTERED_TOPIC_H */
