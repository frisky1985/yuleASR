/**
 * @file rtps_message.h
 * @brief RTPS报文编解码 - 报文头、Submessage处理
 * @version 1.0
 * @date 2026-04-24
 *
 * 实现RTPS报文编码/解码：
 * - 报文头解析/构建
 * - Submessage处理
 * - DATA/HEARTBEAT/ACKNACK报文
 * - CDR序列化/反序列化
 * 支持车载场景的确定性编码
 */

#ifndef RTPS_MESSAGE_H
#define RTPS_MESSAGE_H

#include "rtps_discovery.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * RTPS报文格式常量
 * ============================================================================ */

/** RTPS协议标识 */
#define RTPS_MAGIC                      "RTPS"
#define RTPS_MAGIC_SIZE                 4

/** 协议版本 */
#define RTPS_VERSION_MAJOR              2
#define RTPS_VERSION_MINOR              4

/** 报文头大小 */
#define RTPS_HEADER_SIZE                20
#define RTPS_SUBMESSAGE_HEADER_SIZE     4

/** 最大报文大小 */
#define RTPS_MAX_MESSAGE_SIZE           65535
#define RTPS_MAX_SUBMESSAGE_SIZE        64000

/** 序列化编码 */
#define RTPS_CDR_BE                     0x0000  /* 大端CDR */
#define RTPS_CDR_LE                     0x0001  /* 小端CDR */
#define RTPS_PL_CDR_BE                  0x0002  /* 参数列表大端 */
#define RTPS_PL_CDR_LE                  0x0003  /* 参数列表小端 */

/* ============================================================================
 * Submessage类型定义
 * ============================================================================ */

typedef enum {
    RTPS_SUBMESSAGE_ID_PAD            = 0x01,   /* 填充 */
    RTPS_SUBMESSAGE_ID_ACKNACK        = 0x06,   /* ACKNACK */
    RTPS_SUBMESSAGE_ID_HEARTBEAT      = 0x07,   /* HEARTBEAT */
    RTPS_SUBMESSAGE_ID_GAP            = 0x08,   /* GAP */
    RTPS_SUBMESSAGE_ID_INFO_TS        = 0x09,   /* INFO_TIMESTAMP */
    RTPS_SUBMESSAGE_ID_INFO_SRC       = 0x0c,   /* INFO_SOURCE */
    RTPS_SUBMESSAGE_ID_INFO_REPLY_IP4 = 0x0d,   /* INFO_REPLY_IP4 */
    RTPS_SUBMESSAGE_ID_INFO_DST       = 0x0e,   /* INFO_DESTINATION */
    RTPS_SUBMESSAGE_ID_INFO_REPLY     = 0x0f,   /* INFO_REPLY */
    RTPS_SUBMESSAGE_ID_NACK_FRAG      = 0x12,   /* NACK_FRAG */
    RTPS_SUBMESSAGE_ID_HEARTBEAT_FRAG = 0x13,   /* HEARTBEAT_FRAG */
    RTPS_SUBMESSAGE_ID_DATA           = 0x15,   /* DATA */
    RTPS_SUBMESSAGE_ID_DATA_FRAG      = 0x16,   /* DATA_FRAG */
} rtps_submessage_id_t;

/* Submessage标志 */
#define RTPS_SUBMESSAGE_FLAG_ENDIANNESS     0x01  /* 小端标志 */
#define RTPS_SUBMESSAGE_FLAG_INLINE_QOS     0x02  /* 内联QoS */
#define RTPS_SUBMESSAGE_FLAG_DATA_PRESENT   0x04  /* 数据存在 */
#define RTPS_SUBMESSAGE_FLAG_KEY_PRESENT    0x08  /* Key存在 */
#define RTPS_SUBMESSAGE_FLAG_FINAL          0x02  /* 最终HEARTBEAT */
#define RTPS_SUBMESSAGE_FLAG_LIVELINESS     0x04  /* 活跃性 */

/* ============================================================================
 * 数据结构定义
 * ============================================================================ */

/** RTPS报文头 */
typedef struct rtps_header {
    char magic[4];                          /* "RTPS" */
    uint8_t version_major;
    uint8_t version_minor;
    uint8_t vendor_id[2];
    rtps_guid_prefix_t guid_prefix;
} rtps_header_t;

/** Submessage头 */
typedef struct rtps_submessage_header {
    uint8_t submessage_id;
    uint8_t flags;
    uint16_t submessage_length;
} rtps_submessage_header_t;

/** DATA Submessage */
typedef struct rtps_data_submessage {
    rtps_submessage_header_t header;
    uint16_t extra_flags;
    uint16_t octets_to_inline_qos;
    rtps_entity_id_t reader_id;
    rtps_entity_id_t writer_id;
    rtps_sequence_number_t writer_sn;
    /* 可选字段：Inline QoS, Serialized Data */
} rtps_data_submessage_t;

/** HEARTBEAT Submessage */
typedef struct rtps_heartbeat_submessage {
    rtps_submessage_header_t header;
    rtps_entity_id_t reader_id;
    rtps_entity_id_t writer_id;
    rtps_sequence_number_t first_sn;
    rtps_sequence_number_t last_sn;
    uint32_t count;
} rtps_heartbeat_submessage_t;

/** ACKNACK Submessage */
typedef struct rtps_acknack_submessage {
    rtps_submessage_header_t header;
    rtps_entity_id_t reader_id;
    rtps_entity_id_t writer_id;
    rtps_sequence_number_set_t reader_sn_state;
    uint32_t count;
} rtps_acknack_submessage_t;

/** GAP Submessage */
typedef struct rtps_gap_submessage {
    rtps_submessage_header_t header;
    rtps_entity_id_t reader_id;
    rtps_entity_id_t writer_id;
    rtps_sequence_number_t gap_start;
    rtps_sequence_number_set_t gap_list;
} rtps_gap_submessage_t;

/** INFO_DESTINATION Submessage */
typedef struct rtps_info_dst_submessage {
    rtps_submessage_header_t header;
    rtps_guid_prefix_t guid_prefix;
} rtps_info_dst_submessage_t;

/** INFO_TIMESTAMP Submessage */
typedef struct rtps_info_ts_submessage {
    rtps_submessage_header_t header;
    int64_t timestamp;                      /* 可选 */
} rtps_info_ts_submessage_t;

/* ============================================================================
 * 报文构建/解析上下文
 * ============================================================================ */

/** 报文构建器 */
typedef struct rtps_message_builder {
    uint8_t *buffer;
    uint32_t max_size;
    uint32_t current_pos;
    bool little_endian;
    rtps_guid_prefix_t src_guid_prefix;
} rtps_message_builder_t;

/** 报文解析器 */
typedef struct rtps_message_parser {
    const uint8_t *buffer;
    uint32_t buffer_size;
    uint32_t current_pos;
    bool little_endian;
    rtps_header_t header;
} rtps_message_parser_t;

/** 解析结果回调 */
typedef struct rtps_parsed_submessage {
    rtps_submessage_id_t id;
    uint8_t flags;
    const uint8_t *data;
    uint16_t length;
} rtps_parsed_submessage_t;

/* ============================================================================
 * 报文头操作API
 * ============================================================================ */

/**
 * @brief 构建RTPS报文头
 * @param header 报文头结构
 * @param guid_prefix 源GUID前缀
 * @param vendor_id 厂商ID
 * @return ETH_OK成功
 */
eth_status_t rtps_header_build(rtps_header_t *header,
                                const rtps_guid_prefix_t guid_prefix,
                                uint16_t vendor_id);

/**
 * @brief 解析RTPS报文头
 * @param data 原始数据
 * @param len 数据长度
 * @param header 输出报文头
 * @param bytes_consumed 消耗的字节数
 * @return ETH_OK成功
 */
eth_status_t rtps_header_parse(const uint8_t *data,
                                uint32_t len,
                                rtps_header_t *header,
                                uint32_t *bytes_consumed);

/**
 * @brief 验证RTPS报文魔法字
 * @param data 数据
 * @param len 长度
 * @return true是有效RTPS报文
 */
bool rtps_header_check_magic(const uint8_t *data, uint32_t len);

/* ============================================================================
 * 报文构建器API
 * ============================================================================ */

/**
 * @brief 初始化报文构建器
 * @param builder 构建器
 * @param buffer 输出缓冲区
 * @param max_size 最大大小
 * @param guid_prefix 源GUID前缀
 * @param little_endian 是否小端
 * @return ETH_OK成功
 */
eth_status_t rtps_message_builder_init(rtps_message_builder_t *builder,
                                         uint8_t *buffer,
                                         uint32_t max_size,
                                         const rtps_guid_prefix_t guid_prefix,
                                         bool little_endian);

/**
 * @brief 添加DATA Submessage
 * @param builder 构建器
 * @param reader_id 目标Reader ID
 * @param writer_id 源Writer ID
 * @param seq_number 序列号
 * @param data 序列化数据
 * @param data_len 数据长度
 * @param inline_qos 内联QoS（可为NULL）
 * @param inline_qos_len QoS长度
 * @return ETH_OK成功
 */
eth_status_t rtps_message_add_data(rtps_message_builder_t *builder,
                                    const rtps_entity_id_t reader_id,
                                    const rtps_entity_id_t writer_id,
                                    const rtps_sequence_number_t *seq_number,
                                    const uint8_t *data,
                                    uint32_t data_len,
                                    const uint8_t *inline_qos,
                                    uint32_t inline_qos_len);

/**
 * @brief 添加HEARTBEAT Submessage
 * @param builder 构建器
 * @param reader_id 目标Reader ID
 * @param writer_id 源Writer ID
 * @param first_sn 第一个序列号
 * @param last_sn 最后序列号
 * @param count 心跳计数
 * @param final 是否最终心跳
 * @return ETH_OK成功
 */
eth_status_t rtps_message_add_heartbeat(rtps_message_builder_t *builder,
                                         const rtps_entity_id_t reader_id,
                                         const rtps_entity_id_t writer_id,
                                         const rtps_sequence_number_t *first_sn,
                                         const rtps_sequence_number_t *last_sn,
                                         uint32_t count,
                                         bool final);

/**
 * @brief 添加ACKNACK Submessage
 * @param builder 构建器
 * @param reader_id 源Reader ID
 * @param writer_id 目标Writer ID
 * @param bitmap_base 位图基准
 * @param bitmap 位图
 * @param num_bits 位数
 * @param count 计数
 * @return ETH_OK成功
 */
eth_status_t rtps_message_add_acknack(rtps_message_builder_t *builder,
                                       const rtps_entity_id_t reader_id,
                                       const rtps_entity_id_t writer_id,
                                       const rtps_sequence_number_t *bitmap_base,
                                       const uint32_t *bitmap,
                                       uint32_t num_bits,
                                       uint32_t count);

/**
 * @brief 添加GAP Submessage
 * @param builder 构建器
 * @param reader_id 目标Reader ID
 * @param writer_id 源Writer ID
 * @param gap_start 间隙开始序列号
 * @param gap_bitmap 间隙位图
 * @param num_bits 位数
 * @return ETH_OK成功
 */
eth_status_t rtps_message_add_gap(rtps_message_builder_t *builder,
                                   const rtps_entity_id_t reader_id,
                                   const rtps_entity_id_t writer_id,
                                   const rtps_sequence_number_t *gap_start,
                                   const uint32_t *gap_bitmap,
                                   uint32_t num_bits);

/**
 * @brief 添加INFO_DESTINATION Submessage
 * @param builder 构建器
 * @param guid_prefix 目标GUID前缀
 * @return ETH_OK成功
 */
eth_status_t rtps_message_add_info_dst(rtps_message_builder_t *builder,
                                        const rtps_guid_prefix_t guid_prefix);

/**
 * @brief 添加INFO_TIMESTAMP Submessage
 * @param builder 构建器
 * @param timestamp 时间戳（可为-1表示无效）
 * @return ETH_OK成功
 */
eth_status_t rtps_message_add_info_ts(rtps_message_builder_t *builder,
                                       int64_t timestamp);

/**
 * @brief 完成报文构建
 * @param builder 构建器
 * @param final_length 输出最终报文长度
 * @return ETH_OK成功
 */
eth_status_t rtps_message_builder_finalize(rtps_message_builder_t *builder,
                                            uint32_t *final_length);

/* ============================================================================
 * 报文解析器API
 * ============================================================================ */

/**
 * @brief 初始化报文解析器
 * @param parser 解析器
 * @param data 报文数据
 * @param len 数据长度
 * @return ETH_OK成功
 */
eth_status_t rtps_message_parser_init(rtps_message_parser_t *parser,
                                        const uint8_t *data,
                                        uint32_t len);

/**
 * @brief 获取下一个Submessage
 * @param parser 解析器
 * @param submsg 输出Submessage信息
 * @return ETH_OK成功，ETH_TIMEOUT没有更多
 */
eth_status_t rtps_message_parser_next(rtps_message_parser_t *parser,
                                        rtps_parsed_submessage_t *submsg);

/**
 * @brief 解析DATA Submessage
 * @param data 数据
 * @param len 长度
 * @param flags 标志
 * @param little_endian 是否小端
 * @param out_reader_id 输出Reader ID
 * @param out_writer_id 输出Writer ID
 * @param out_seq_number 输出序列号
 * @param out_payload 输出数据起始位置
 * @param out_payload_len 输出数据长度
 * @return ETH_OK成功
 */
eth_status_t rtps_parse_data_submessage(const uint8_t *data,
                                         uint16_t len,
                                         uint8_t flags,
                                         bool little_endian,
                                         rtps_entity_id_t out_reader_id,
                                         rtps_entity_id_t out_writer_id,
                                         rtps_sequence_number_t *out_seq_number,
                                         const uint8_t **out_payload,
                                         uint32_t *out_payload_len);

/**
 * @brief 解析HEARTBEAT Submessage
 * @param data 数据
 * @param len 长度
 * @param flags 标志
 * @param little_endian 是否小端
 * @param out_reader_id 输出Reader ID
 * @param out_writer_id 输出Writer ID
 * @param out_first_sn 输出第一序列号
 * @param out_last_sn 输出最后序列号
 * @param out_count 输出计数
 * @return ETH_OK成功
 */
eth_status_t rtps_parse_heartbeat_submessage(const uint8_t *data,
                                              uint16_t len,
                                              uint8_t flags,
                                              bool little_endian,
                                              rtps_entity_id_t out_reader_id,
                                              rtps_entity_id_t out_writer_id,
                                              rtps_sequence_number_t *out_first_sn,
                                              rtps_sequence_number_t *out_last_sn,
                                              uint32_t *out_count);

/**
 * @brief 解析ACKNACK Submessage
 * @param data 数据
 * @param len 长度
 * @param flags 标志
 * @param little_endian 是否小端
 * @param out_reader_id 输出Reader ID
 * @param out_writer_id 输出Writer ID
 * @param out_bitmap_base 输出位图基准
 * @param out_bitmap 输出位图缓冲区
 * @param out_num_bits 输出位数
 * @param out_count 输出计数
 * @return ETH_OK成功
 */
eth_status_t rtps_parse_acknack_submessage(const uint8_t *data,
                                            uint16_t len,
                                            uint8_t flags,
                                            bool little_endian,
                                            rtps_entity_id_t out_reader_id,
                                            rtps_entity_id_t out_writer_id,
                                            rtps_sequence_number_t *out_bitmap_base,
                                            uint32_t *out_bitmap,
                                            uint32_t *out_num_bits,
                                            uint32_t *out_count);

/* ============================================================================
 * 序列化工具API
 * ============================================================================ */

/**
 * @brief 将序列号转换为64位整数
 * @param seq 序列号
 * @return 64位值
 */
int64_t rtps_seqnum_to_int64(const rtps_sequence_number_t *seq);

/**
 * @brief 将64位整数转换为序列号
 * @param value 64位值
 * @param seq 输出序列号
 */
void rtps_int64_to_seqnum(int64_t value, rtps_sequence_number_t *seq);

/**
 * @brief 序列化GUID到缓冲区
 * @param guid GUID
 * @param buffer 缓冲区
 * @param little_endian 是否小端
 * @return 消耗字节数
 */
uint32_t rtps_serialize_guid(const rtps_guid_t *guid, 
                              uint8_t *buffer, 
                              bool little_endian);

/**
 * @brief 从缓冲区反序列化GUID
 * @param buffer 缓冲区
 * @param len 长度
 * @param little_endian 是否小端
 * @param guid 输出GUID
 * @return ETH_OK成功
 */
eth_status_t rtps_deserialize_guid(const uint8_t *buffer,
                                    uint32_t len,
                                    bool little_endian,
                                    rtps_guid_t *guid);

/**
 * @brief 序列化Entity ID到缓冲区
 * @param entity_id Entity ID
 * @param buffer 缓冲区
 * @return 消耗字节数
 */
uint32_t rtps_serialize_entity_id(const rtps_entity_id_t entity_id, uint8_t *buffer);

/**
 * @brief 从缓冲区反序列化Entity ID
 * @param buffer 缓冲区
 * @param entity_id 输出Entity ID
 * @return ETH_OK成功
 */
eth_status_t rtps_deserialize_entity_id(const uint8_t *buffer, 
                                         rtps_entity_id_t entity_id);

/**
 * @brief 计算校验和（用于数据完整性验证）
 * @param data 数据
 * @param len 长度
 * @return 校验和
 */
uint32_t rtps_calculate_checksum(const uint8_t *data, uint32_t len);

/* ============================================================================
 * 车载优化API
 * ============================================================================ */

/**
 * @brief 构建确定性DATA报文（AUTOSAR模式）
 * @param builder 构建器
 * @param reader_id 目标Reader ID
 * @param writer_id 源Writer ID
 * @param seq_number 序列号
 * @param data 数据
 * @param data_len 长度
 * @param activation_time_us 激活时间（微秒）
 * @return ETH_OK成功
 */
eth_status_t rtps_message_add_data_autosar(rtps_message_builder_t *builder,
                                            const rtps_entity_id_t reader_id,
                                            const rtps_entity_id_t writer_id,
                                            const rtps_sequence_number_t *seq_number,
                                            const uint8_t *data,
                                            uint32_t data_len,
                                            uint32_t activation_time_us);

/**
 * @brief 解析AUTOSAR扩展字段
 * @param data 数据
 * @param len 长度
 * @param out_activation_time_us 输出激活时间
 * @return ETH_OK成功
 */
eth_status_t rtps_parse_autosar_extension(const uint8_t *data,
                                           uint32_t len,
                                           uint32_t *out_activation_time_us);

/**
 * @brief 验证报文完整性
 * @param data 报文数据
 * @param len 长度
 * @return true完整
 */
bool rtps_message_validate(const uint8_t *data, uint32_t len);

/**
 * @brief 获取报文大小估算
 * @param data_len 数据长度
 * @param has_inline_qos 是否有内联QoS
 * @return 估算的报文大小
 */
uint32_t rtps_message_estimate_size(uint32_t data_len, bool has_inline_qos);

#ifdef __cplusplus
}
#endif

#endif /* RTPS_MESSAGE_H */
