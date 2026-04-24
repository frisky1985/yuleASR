/*******************************************************************************
 * @file dds_analyzer.h
 * @brief DDS网络流量分析器 - RTPS报文捕获和解析
 *
 * 功能特点：
 * - 实时RTPS报文捕获
 * - 报文解析和显示
 * - 过滤和搜索功能
 * - pcap导入/导出
 * - 车载UDS协议支持
 ******************************************************************************/

#ifndef DDS_ANALYZER_H
#define DDS_ANALYZER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <pcap.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 版本和常量
 *============================================================================*/
#define DDS_ANALYZER_VERSION_MAJOR  1
#define DDS_ANALYZER_VERSION_MINOR  0
#define DDS_ANALYZER_VERSION_PATCH  0

#define DDS_ANALYZER_MAX_FILTER_LEN     256
#define DDS_ANALYZER_MAX_TOPIC_LEN      256
#define DDS_ANALYZER_MAX_HOSTNAME_LEN   64
#define DDS_ANALYZER_MAX_PACKET_SIZE    65535
#define DDS_ANALYZER_RING_BUFFER_SIZE   (16 * 1024 * 1024)  /* 16MB */
#define DDS_ANALYZER_MAX_CAPTURE_SEC    3600    /* 最大捕获时间 */

/*==============================================================================
 * RTPS协议定义
 *============================================================================*/
#define RTPS_MAGIC_COOKIE     0x52545053  /* "RTPS" */
#define RTPS_VERSION_MAJOR    2
#define RTPS_VERSION_MINOR    2
#define RTPS_HEADER_SIZE      20

/* RTPS子消息类型 */
typedef enum {
    RTPS_SUBMSG_PAD         = 0x01,
    RTPS_SUBMSG_ACKNACK     = 0x06,
    RTPS_SUBMSG_HEARTBEAT   = 0x07,
    RTPS_SUBMSG_GAP         = 0x08,
    RTPS_SUBMSG_INFO_TS     = 0x09,
    RTPS_SUBMSG_INFO_SRC    = 0x0c,
    RTPS_SUBMSG_INFO_REPLY_IP4 = 0x0d,
    RTPS_SUBMSG_INFO_DST    = 0x0e,
    RTPS_SUBMSG_INFO_REPLY  = 0x0f,
    RTPS_SUBMSG_NACK_FRAG   = 0x12,
    RTPS_SUBMSG_HEARTBEAT_FRAG = 0x13,
    RTPS_SUBMSG_DATA        = 0x15,
    RTPS_SUBMSG_DATA_FRAG   = 0x16,
    RTPS_SUBMSG_NOKEY_DATA  = 0x17,
    RTPS_SUBMSG_ACKNACK_BATCH = 0x19,
    RTPS_SUBMSG_HEARTBEAT_BATCH = 0x1a,
    RTPS_SUBMSG_HEARTBEAT_VIRTUAL = 0x1b,
    RTPS_SUBMSG_SEC_BODY    = 0x30,
    RTPS_SUBMSG_SEC_PREFIX  = 0x31,
    RTPS_SUBMSG_SEC_POSTFIX = 0x32,
    RTPS_SUBMSG_SRTPS_PREFIX = 0x33,
    RTPS_SUBMSG_SRTPS_POSTFIX = 0x34
} rtps_submsg_type_t;

/*==============================================================================
 * 数据结构定义
 *============================================================================*/

/* RTPS导航实体ID */
typedef struct {
    uint32_t    prefix[3];      /* 12字节前缀 */
    uint32_t    entity_id;      /* 4字节实体ID */
} rtps_guid_t;

/* RTPS实体ID分解 */
typedef struct {
    uint8_t     key[3];         /* 3字节key */
    uint8_t     kind;           /* 实体类型 */
} rtps_entity_id_t;

/* 实体类型 */
typedef enum {
    RTPS_ENTITY_UNKNOWN         = 0,
    RTPS_ENTITY_PARTICIPANT     = 1,
    RTPS_ENTITY_WRITER          = 2,
    RTPS_ENTITY_READER          = 3,
    RTPS_ENTITY_TOPIC           = 4,
    RTPS_ENTITY_PUBLISHER       = 5,
    RTPS_ENTITY_SUBSCRIBER      = 6
} rtps_entity_type_t;

/* RTPS报文头 */
typedef struct __attribute__((packed)) {
    char        magic[4];           /* "RTPS" */
    uint8_t     version_major;
    uint8_t     version_minor;
    uint8_t     vendor_id[2];
    rtps_guid_t participant_guid;
} rtps_header_t;

/* RTPS子消息头 */
typedef struct __attribute__((packed)) {
    uint8_t     submsg_id;
    uint8_t     flags;
    uint16_t    submsg_length;
} rtps_submsg_header_t;

/* DATA子消息 */
typedef struct {
    uint16_t    extra_flags;
    uint16_t    octets_to_inline_qos;
    rtps_guid_t reader_guid;
    rtps_guid_t writer_guid;
    uint64_t    sequence_number;
    /* 后面是inline QoS和serialized data */
} rtps_data_submsg_t;

/* 捕获的报文信息 */
typedef struct dds_packet_info {
    uint64_t            timestamp_ns;
    struct timeval      timestamp;
    char                src_ip[16];
    char                dst_ip[16];
    uint16_t            src_port;
    uint16_t            dst_port;
    uint32_t            packet_len;
    uint32_t            payload_len;
    
    /* RTPS信息 */
    bool                is_rtps;
    rtps_header_t       rtps_header;
    uint8_t             submsg_count;
    uint8_t             submsg_types[16];
    
    /* 解析状态 */
    rtps_guid_t         src_guid;
    rtps_guid_t         dst_guid;
    uint64_t            sequence_num;
    char                topic_name[DDS_ANALYZER_MAX_TOPIC_LEN];
    
    /* 原始数据 */
    const uint8_t*      raw_data;
    uint32_t            raw_len;
} dds_packet_info_t;

/*==============================================================================
 * 统计信息
 *============================================================================*/
typedef struct dds_traffic_stats {
    /* 基本统计 */
    uint64_t    total_packets;
    uint64_t    total_bytes;
    uint64_t    rtps_packets;
    uint64_t    rtps_bytes;
    uint64_t    dropped_packets;
    
    /* 时间统计 */
    uint64_t    first_packet_time;
    uint64_t    last_packet_time;
    uint64_t    capture_duration_us;
    
    /* 流量统计 */
    uint64_t    packets_per_sec;
    uint64_t    bytes_per_sec;
    double      avg_packet_size;
    double      peak_bps;
    
    /* RTPS统计 */
    uint64_t    data_submsg_count;
    uint64_t    heartbeat_count;
    uint64_t    acknack_count;
    uint64_t    gap_count;
    
    /* 报错统计 */
    uint64_t    malformed_packets;
    uint64_t    checksum_errors;
} dds_traffic_stats_t;

/*==============================================================================
 * 过滤器类型
 *============================================================================*/
typedef enum {
    DDS_FILTER_NONE         = 0,
    DDS_FILTER_IP           = 1,
    DDS_FILTER_PORT         = 2,
    DDS_FILTER_TOPIC        = 3,
    DDS_FILTER_GUID         = 4,
    DDS_FILTER_SUBMSG_TYPE  = 5,
    DDS_FILTER_EXPRESSION   = 6
} dds_filter_type_t;

typedef struct dds_filter {
    dds_filter_type_t   type;
    bool                invert;         /* 取反 */
    union {
        char            ip[16];
        uint16_t        port;
        char            topic[DDS_ANALYZER_MAX_TOPIC_LEN];
        rtps_guid_t     guid;
        uint8_t         submsg_type;
        char            expression[DDS_ANALYZER_MAX_FILTER_LEN];
    } value;
} dds_filter_t;

/*==============================================================================
 * 捕获配置
 *============================================================================*/
typedef struct dds_capture_config {
    /* 网络接口 */
    char        interface_name[32];
    bool        promiscuous_mode;
    int         snaplen;
    int         timeout_ms;
    
    /* 捕获限制 */
    uint64_t    max_packets;
    uint64_t    max_bytes;
    uint64_t    max_duration_sec;
    
    /* 过滤器 */
    char        pcap_filter[DDS_ANALYZER_MAX_FILTER_LEN];
    dds_filter_t* filters;
    uint32_t    filter_count;
    
    /* 文件 */
    char        output_file[256];
    bool        save_to_file;
    
    /* 车载特定 */
    bool        enable_uds_decode;      /* 解码UDS封装 */
    bool        enable_can_bridge;      /* CAN桥接分析 */
} dds_capture_config_t;

/*==============================================================================
 * 回调函数类型
 *============================================================================*/
/* 报文回调 */
typedef void (*dds_packet_callback_t)(
    const dds_packet_info_t* packet,
    void* user_data
);

/* 统计回调 */
typedef void (*dds_stats_callback_t)(
    const dds_traffic_stats_t* stats,
    void* user_data
);

/* 错误回调 */
typedef void (*dds_error_callback_t)(
    int error_code,
    const char* error_msg,
    void* user_data
);

/*==============================================================================
 * 捕获会话句柄
 *============================================================================*/
typedef struct dds_capture_session dds_capture_session_t;

/*==============================================================================
 * 初始化和生命周期
 *============================================================================*/

/**
 * @brief 初始化DDS分析器库
 */
int dds_analyzer_init(void);

/**
 * @brief 反初始化DDS分析器库
 */
void dds_analyzer_deinit(void);

/**
 * @brief 获取版本信息
 */
void dds_analyzer_get_version(int* major, int* minor, int* patch);

/*==============================================================================
 * 捕获会话管理
 *============================================================================*/

/**
 * @brief 创建捕获会话
 */
dds_capture_session_t* dds_capture_create(const dds_capture_config_t* config);

/**
 * @brief 销毁捕获会话
 */
void dds_capture_destroy(dds_capture_session_t* session);

/**
 * @brief 开始捕获
 */
int dds_capture_start(dds_capture_session_t* session);

/**
 * @brief 停止捕获
 */
int dds_capture_stop(dds_capture_session_t* session);

/**
 * @brief 检查捕获是否运行中
 */
bool dds_capture_is_running(const dds_capture_session_t* session);

/**
 * @brief 等待捕获结束
 */
int dds_capture_wait(dds_capture_session_t* session, int timeout_ms);

/*==============================================================================
 * 回调设置
 *============================================================================*/

/**
 * @brief 设置报文回调
 */
void dds_capture_set_packet_callback(
    dds_capture_session_t* session,
    dds_packet_callback_t callback,
    void* user_data
);

/**
 * @brief 设置统计回调
 */
void dds_capture_set_stats_callback(
    dds_capture_session_t* session,
    dds_stats_callback_t callback,
    uint64_t interval_ms
);

/**
 * @brief 设置错误回调
 */
void dds_capture_set_error_callback(
    dds_capture_session_t* session,
    dds_error_callback_t callback,
    void* user_data
);

/*==============================================================================
 * 过滤器接口
 *============================================================================*/

/**
 * @brief 添加过滤器
 */
int dds_capture_add_filter(dds_capture_session_t* session, const dds_filter_t* filter);

/**
 * @brief 清除所有过滤器
 */
void dds_capture_clear_filters(dds_capture_session_t* session);

/**
 * @brief 应用BPF过滤器
 */
int dds_capture_set_bpf_filter(dds_capture_session_t* session, const char* filter);

/*==============================================================================
 * 数据操作
 *============================================================================*/

/**
 * @brief 获取统计信息
 */
void dds_capture_get_stats(dds_capture_session_t* session, dds_traffic_stats_t* stats);

/**
 * @brief 重置统计
 */
void dds_capture_reset_stats(dds_capture_session_t* session);

/**
 * @brief 获取最后错误
 */
int dds_capture_get_last_error(dds_capture_session_t* session, char* msg, size_t msg_size);

/*==============================================================================
 * pcap文件操作
 *============================================================================*/

/**
 * @brief 保存捕获到pcap文件
 */
int dds_capture_save_to_file(dds_capture_session_t* session, const char* filename);

/**
 * @brief 从pcap文件加载
 */
int dds_capture_load_from_file(
    const char* filename,
    dds_packet_callback_t callback,
    void* user_data
);

/**
 * @brief 将捕获转换为pcap格式
 */
int dds_capture_export_pcap(
    dds_capture_session_t* session,
    const char* filename,
    uint64_t start_time,
    uint64_t end_time
);

/**
 * @brief 合并多个pcap文件
 */
int dds_capture_merge_pcap(
    const char** input_files,
    uint32_t file_count,
    const char* output_file
);

/*==============================================================================
 * RTPS解析
 *============================================================================*/

/**
 * @brief 解析RTPS报文
 */
int dds_parse_rtps_packet(
    const uint8_t* data,
    uint32_t len,
    dds_packet_info_t* info
);

/**
 * @brief 解析RTPS子消息
 */
int dds_parse_rtps_submsg(
    const uint8_t* data,
    uint32_t len,
    uint32_t offset,
    rtps_submsg_header_t* header,
    uint32_t* next_offset
);

/**
 * @brief 解码RTPS实体ID
 */
void dds_decode_entity_id(uint32_t entity_id, rtps_entity_id_t* decoded);

/**
 * @brief 获取实体类型名称
 */
const char* dds_get_entity_type_name(rtps_entity_type_t type);

/**
 * @brief 获取子消息类型名称
 */
const char* dds_get_submsg_type_name(uint8_t type);

/**
 * @brief 将GUID转换为字符串
 */
void dds_guid_to_string(const rtps_guid_t* guid, char* str, size_t str_size);

/*==============================================================================
 * 高级分析
 *============================================================================*/

/**
 * @brief 分析DDS主题流量
 * @return 找到的主题数
 */
int dds_analyze_topics(
    dds_capture_session_t* session,
    char** topic_names,
    uint64_t* packet_counts,
    uint32_t max_topics
);

/**
 * @brief 分析节点通信模式
 * @return 找到的节点数
 */
int dds_analyze_nodes(
    dds_capture_session_t* session,
    rtps_guid_t* node_guids,
    uint32_t max_nodes
);

/**
 * @brief 生成流量图表数据
 */
int dds_generate_traffic_graph(
    dds_capture_session_t* session,
    uint64_t* timestamps,
    uint64_t* packet_counts,
    uint64_t* byte_counts,
    uint32_t* data_points
);

/**
 * @brief 检测RTPS异常
 */
int dds_detect_anomalies(
    dds_capture_session_t* session,
    char** anomaly_descriptions,
    uint32_t max_anomalies
);

/*==============================================================================
 * UDS诊断支持
 *============================================================================*/

/**
 * @brief 解码UDS封装的DDS消息
 */
int dds_decode_uds_encapsulation(
    const uint8_t* data,
    uint32_t len,
    uint8_t* uds_service_id,
    uint8_t* uds_response_code,
    uint8_t** payload,
    uint32_t* payload_len
);

/**
 * @brief 解码CAN桥接DDS消息
 */
int dds_decode_can_bridge(
    const uint8_t* data,
    uint32_t len,
    uint32_t* can_id,
    uint8_t* can_dlc,
    uint8_t* can_data
);

#ifdef __cplusplus
}
#endif

#endif /* DDS_ANALYZER_H */
