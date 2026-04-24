/*******************************************************************************
 * @file dds_analyzer.c
 * @brief DDS网络流量分析器实现
 ******************************************************************************/

#include "dds_analyzer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

/*==============================================================================
 * 内部定义
 *============================================================================*/
#define DDS_ANALYZER_MAGIC      0x44445341  /* "DDSA" */
#define DEFAULT_SNAPLEN         65535
#define DEFAULT_TIMEOUT_MS      1000
#define STATS_UPDATE_INTERVAL_MS 1000

/* EtherType定义 */
#define ETHERTYPE_IPv4          0x0800
#define ETHERTYPE_IPv6          0x86DD

/* UDS常量 */
#define UDS_SID_DIAG_SESSION_CONTROL    0x10
#define UDS_SID_ECU_RESET               0x11
#define UDS_SID_READ_DATA_BY_ID         0x22
#define UDS_SID_WRITE_DATA_BY_ID        0x2E

/*==============================================================================
 * 捕获会话结构
 *============================================================================*/
struct dds_capture_session {
    /* 状态 */
    uint32_t                magic;
    volatile bool           running;
    volatile bool           stop_requested;
    
    /* 配置 */
    dds_capture_config_t    config;
    
    /* pcap处理 */
    pcap_t*                 pcap_handle;
    pcap_dumper_t*          pcap_dumper;
    struct bpf_program      bpf_program;
    
    /* 线程 */
    pthread_t               capture_thread;
    pthread_mutex_t         mutex;
    
    /* 统计 */
    dds_traffic_stats_t     stats;
    pthread_mutex_t         stats_mutex;
    
    /* 回调 */
    dds_packet_callback_t   packet_callback;
    void*                   packet_user_data;
    dds_stats_callback_t    stats_callback;
    uint64_t                stats_interval_ms;
    void*                   stats_user_data;
    dds_error_callback_t    error_callback;
    void*                   error_user_data;
    
    /* 过滤器 */
    dds_filter_t*           filters;
    uint32_t                filter_count;
    uint32_t                filter_capacity;
    
    /* 错误 */
    int                     last_error;
    char                    error_msg[256];
    
    /* 缓存 */
    dds_packet_info_t*      packet_buffer;
    uint32_t                buffer_size;
    uint32_t                buffer_head;
    uint32_t                buffer_tail;
};

/*==============================================================================
 * 全局状态
 *============================================================================*/
static volatile bool g_analyzer_initialized = false;
static pthread_mutex_t g_init_mutex = PTHREAD_MUTEX_INITIALIZER;

/*==============================================================================
 * 内部函数前向声明
 *============================================================================*/
static void* capture_thread_func(void* arg);
static void process_packet(u_char* user, const struct pcap_pkthdr* header, const u_char* data);
static int parse_ethernet_frame(const u_char* data, uint32_t len, dds_packet_info_t* info);
static bool apply_filters(dds_capture_session_t* session, const dds_packet_info_t* info);
static void update_stats(dds_capture_session_t* session, const dds_packet_info_t* info);
static void report_error(dds_capture_session_t* session, int code, const char* msg);

/*==============================================================================
 * 初始化和生命周期
 *============================================================================*/

int dds_analyzer_init(void) {
    pthread_mutex_lock(&g_init_mutex);
    
    if (g_analyzer_initialized) {
        pthread_mutex_unlock(&g_init_mutex);
        return 0;
    }
    
    /* 初始化pcap库 */
    char errbuf[PCAP_ERRBUF_SIZE];
    if (pcap_init(PCAP_CHAR_ENC_UTF_8, errbuf) != 0) {
        pthread_mutex_unlock(&g_init_mutex);
        return -1;
    }
    
    g_analyzer_initialized = true;
    pthread_mutex_unlock(&g_init_mutex);
    
    return 0;
}

void dds_analyzer_deinit(void) {
    pthread_mutex_lock(&g_init_mutex);
    g_analyzer_initialized = false;
    pthread_mutex_unlock(&g_init_mutex);
}

void dds_analyzer_get_version(int* major, int* minor, int* patch) {
    if (major) *major = DDS_ANALYZER_VERSION_MAJOR;
    if (minor) *minor = DDS_ANALYZER_VERSION_MINOR;
    if (patch) *patch = DDS_ANALYZER_VERSION_PATCH;
}

/*==============================================================================
 * 捕获会话管理
 *============================================================================*/

dds_capture_session_t* dds_capture_create(const dds_capture_config_t* config) {
    if (!g_analyzer_initialized) {
        if (dds_analyzer_init() != 0) {
            return NULL;
        }
    }
    
    dds_capture_session_t* session = calloc(1, sizeof(dds_capture_session_t));
    if (!session) {
        return NULL;
    }
    
    session->magic = DDS_ANALYZER_MAGIC;
    session->running = false;
    session->stop_requested = false;
    
    /* 复制配置 */
    if (config) {
        memcpy(&session->config, config, sizeof(dds_capture_config_t));
    } else {
        /* 默认配置 */
        strcpy(session->config.interface_name, "any");
        session->config.promiscuous_mode = true;
        session->config.snaplen = DEFAULT_SNAPLEN;
        session->config.timeout_ms = DEFAULT_TIMEOUT_MS;
        session->config.save_to_file = false;
    }
    
    /* 初始化同步 */
    pthread_mutex_init(&session->mutex, NULL);
    pthread_mutex_init(&session->stats_mutex, NULL);
    
    /* 分配过滤器数组 */
    session->filter_capacity = 16;
    session->filters = calloc(session->filter_capacity, sizeof(dds_filter_t));
    
    /* 复制过滤器 */
    if (config && config->filters && config->filter_count > 0) {
        uint32_t count = config->filter_count < session->filter_capacity ? 
                         config->filter_count : session->filter_capacity;
        memcpy(session->filters, config->filters, count * sizeof(dds_filter_t));
        session->filter_count = count;
    }
    
    return session;
}

void dds_capture_destroy(dds_capture_session_t* session) {
    if (!session || session->magic != DDS_ANALYZER_MAGIC) {
        return;
    }
    
    dds_capture_stop(session);
    
    /* 清理资源 */
    if (session->pcap_dumper) {
        pcap_dump_close(session->pcap_dumper);
    }
    if (session->pcap_handle) {
        pcap_close(session->pcap_handle);
    }
    
    free(session->filters);
    pthread_mutex_destroy(&session->mutex);
    pthread_mutex_destroy(&session->stats_mutex);
    
    session->magic = 0;
    free(session);
}

int dds_capture_start(dds_capture_session_t* session) {
    if (!session || session->magic != DDS_ANALYZER_MAGIC) {
        return -1;
    }
    
    pthread_mutex_lock(&session->mutex);
    
    if (session->running) {
        pthread_mutex_unlock(&session->mutex);
        return -1;
    }
    
    char errbuf[PCAP_ERRBUF_SIZE];
    
    /* 打开pcap设备 */
    session->pcap_handle = pcap_open_live(
        session->config.interface_name,
        session->config.snaplen,
        session->config.promiscuous_mode ? 1 : 0,
        session->config.timeout_ms,
        errbuf
    );
    
    if (!session->pcap_handle) {
        strncpy(session->error_msg, errbuf, sizeof(session->error_msg) - 1);
        session->last_error = -1;
        pthread_mutex_unlock(&session->mutex);
        return -1;
    }
    
    /* 设置BPF过滤器 */
    if (strlen(session->config.pcap_filter) > 0) {
        if (pcap_compile(session->pcap_handle, &session->bpf_program,
                         session->config.pcap_filter, 1, PCAP_NETMASK_UNKNOWN) == 0) {
            pcap_setfilter(session->pcap_handle, &session->bpf_program);
        }
    }
    
    /* 创建pcap dump文件 */
    if (session->config.save_to_file && strlen(session->config.output_file) > 0) {
        session->pcap_dumper = pcap_dump_open(session->pcap_handle, session->config.output_file);
        if (!session->pcap_dumper) {
            strncpy(session->error_msg, pcap_geterr(session->pcap_handle), sizeof(session->error_msg) - 1);
            pcap_close(session->pcap_handle);
            session->pcap_handle = NULL;
            pthread_mutex_unlock(&session->mutex);
            return -1;
        }
    }
    
    /* 重置统计 */
    memset(&session->stats, 0, sizeof(session->stats));
    session->stats.first_packet_time = 0;
    
    /* 启动捕获线程 */
    session->running = true;
    session->stop_requested = false;
    
    if (pthread_create(&session->capture_thread, NULL, capture_thread_func, session) != 0) {
        session->running = false;
        if (session->pcap_dumper) {
            pcap_dump_close(session->pcap_dumper);
            session->pcap_dumper = NULL;
        }
        pcap_close(session->pcap_handle);
        session->pcap_handle = NULL;
        pthread_mutex_unlock(&session->mutex);
        return -1;
    }
    
    pthread_mutex_unlock(&session->mutex);
    
    return 0;
}

int dds_capture_stop(dds_capture_session_t* session) {
    if (!session || session->magic != DDS_ANALYZER_MAGIC) {
        return -1;
    }
    
    pthread_mutex_lock(&session->mutex);
    
    if (!session->running) {
        pthread_mutex_unlock(&session->mutex);
        return 0;
    }
    
    session->stop_requested = true;
    
    /* 中断pcap循环 */
    if (session->pcap_handle) {
        pcap_breakloop(session->pcap_handle);
    }
    
    pthread_mutex_unlock(&session->mutex);
    
    /* 等待捕获线程结束 */
    pthread_join(session->capture_thread, NULL);
    
    pthread_mutex_lock(&session->mutex);
    session->running = false;
    
    if (session->pcap_dumper) {
        pcap_dump_close(session->pcap_dumper);
        session->pcap_dumper = NULL;
    }
    
    if (session->pcap_handle) {
        pcap_close(session->pcap_handle);
        session->pcap_handle = NULL;
    }
    
    pthread_mutex_unlock(&session->mutex);
    
    return 0;
}

bool dds_capture_is_running(const dds_capture_session_t* session) {
    if (!session || session->magic != DDS_ANALYZER_MAGIC) {
        return false;
    }
    return session->running;
}

int dds_capture_wait(dds_capture_session_t* session, int timeout_ms) {
    if (!session || session->magic != DDS_ANALYZER_MAGIC) {
        return -1;
    }
    
    /* 等待条件或超时 */
    int elapsed = 0;
    while (session->running && elapsed < timeout_ms) {
        usleep(10000);  /* 10ms */
        elapsed += 10;
    }
    
    return session->running ? -1 : 0;
}

/*==============================================================================
 * 捕获线程
 *============================================================================*/

static void* capture_thread_func(void* arg) {
    dds_capture_session_t* session = (dds_capture_session_t*)arg;
    
    uint64_t last_stats_time = 0;
    uint64_t start_time = 0;
    
    while (!session->stop_requested) {
        struct pcap_pkthdr* header;
        const u_char* data;
        int ret = pcap_next_ex(session->pcap_handle, &header, &data);
        
        if (ret == -2) {  /* EOF */
            break;
        }
        if (ret == -1) {  /* 错误 */
            report_error(session, -1, pcap_geterr(session->pcap_handle));
            break;
        }
        if (ret == 0) {   /* 超时 */
            continue;
        }
        
        /* 记录时间 */
        uint64_t now = (uint64_t)header->ts.tv_sec * 1000000ULL + header->ts.tv_usec;
        if (start_time == 0) {
            start_time = now;
        }
        
        /* 检查捕获限制 */
        if (session->config.max_packets > 0 && 
            session->stats.total_packets >= session->config.max_packets) {
            break;
        }
        if (session->config.max_bytes > 0 && 
            session->stats.total_bytes >= session->config.max_bytes) {
            break;
        }
        if (session->config.max_duration_sec > 0) {
            uint64_t duration = (now - start_time) / 1000000;
            if (duration >= session->config.max_duration_sec) {
                break;
            }
        }
        
        /* 处理报文 */
        process_packet((u_char*)session, header, data);
        
        /* 定期统计回调 */
        if (session->stats_callback && session->stats_interval_ms > 0) {
            if (now - last_stats_time >= session->stats_interval_ms * 1000) {
                dds_traffic_stats_t stats;
                dds_capture_get_stats(session, &stats);
                session->stats_callback(&stats, session->stats_user_data);
                last_stats_time = now;
            }
        }
    }
    
    session->running = false;
    return NULL;
}

static void process_packet(u_char* user, const struct pcap_pkthdr* header, const u_char* data) {
    dds_capture_session_t* session = (dds_capture_session_t*)user;
    
    /* 保存到pcap文件 */
    if (session->pcap_dumper) {
        pcap_dump(user, header, data);
    }
    
    /* 解析报文 */
    dds_packet_info_t info;
    memset(&info, 0, sizeof(info));
    
    info.timestamp = header->ts;
    info.timestamp_ns = (uint64_t)header->ts.tv_sec * 1000000000ULL + header->ts.tv_usec * 1000;
    info.packet_len = header->len;
    info.raw_data = data;
    info.raw_len = header->caplen;
    
    /* 解析以太网帧 */
    if (parse_ethernet_frame(data, header->caplen, &info) != 0) {
        pthread_mutex_lock(&session->stats_mutex);
        session->stats.malformed_packets++;
        pthread_mutex_unlock(&session->stats_mutex);
        return;
    }
    
    /* 检查是否是RTPS */
    if (info.payload_len >= 4) {
        const uint8_t* payload = data + (info.packet_len - info.payload_len);
        if (memcmp(payload, "RTPS", 4) == 0) {
            info.is_rtps = true;
            dds_parse_rtps_packet(payload, info.payload_len, &info);
        }
    }
    
    /* 应用过滤器 */
    if (!apply_filters(session, &info)) {
        return;
    }
    
    /* 更新统计 */
    update_stats(session, &info);
    
    /* 调用回调 */
    if (session->packet_callback) {
        session->packet_callback(&info, session->packet_user_data);
    }
}

/*==============================================================================
 * 报文解析
 *============================================================================*/

static int parse_ethernet_frame(const u_char* data, uint32_t len, dds_packet_info_t* info) {
    if (len < sizeof(struct ether_header)) {
        return -1;
    }
    
    const struct ether_header* eth = (const struct ether_header*)data;
    uint16_t ether_type = ntohs(eth->ether_type);
    
    uint32_t offset = sizeof(struct ether_header);
    
    /* 处理VLAN */
    if (ether_type == 0x8100) {
        offset += 4;
        if (len < offset + 2) return -1;
        ether_type = ntohs(*(uint16_t*)(data + offset - 2));
    }
    
    /* 处理IP */
    if (ether_type == ETHERTYPE_IPv4) {
        if (len < offset + sizeof(struct ip)) return -1;
        const struct ip* ip_hdr = (const struct ip*)(data + offset);
        
        inet_ntop(AF_INET, &ip_hdr->ip_src, info->src_ip, sizeof(info->src_ip));
        inet_ntop(AF_INET, &ip_hdr->ip_dst, info->dst_ip, sizeof(info->dst_ip));
        
        uint32_t ip_header_len = ip_hdr->ip_hl * 4;
        offset += ip_header_len;
        
        /* 处理UDP */
        if (ip_hdr->ip_p == IPPROTO_UDP) {
            if (len < offset + sizeof(struct udphdr)) return -1;
            const struct udphdr* udp_hdr = (const struct udphdr*)(data + offset);
            
            info->src_port = ntohs(udp_hdr->uh_sport);
            info->dst_port = ntohs(udp_hdr->uh_dport);
            info->payload_len = ntohs(udp_hdr->uh_ulen) - sizeof(struct udphdr);
        }
    }
    
    return 0;
}

int dds_parse_rtps_packet(const uint8_t* data, uint32_t len, dds_packet_info_t* info) {
    if (len < sizeof(rtps_header_t)) {
        return -1;
    }
    
    const rtps_header_t* header = (const rtps_header_t*)data;
    
    /* 验证magic */
    if (memcmp(header->magic, "RTPS", 4) != 0) {
        return -1;
    }
    
    /* 复制RTPS头 */
    memcpy(&info->rtps_header, header, sizeof(rtps_header_t));
    
    /* 解析子消息 */
    uint32_t offset = sizeof(rtps_header_t);
    info->submsg_count = 0;
    
    while (offset < len && info->submsg_count < 16) {
        rtps_submsg_header_t submsg_hdr;
        uint32_t next_offset;
        
        if (dds_parse_rtps_submsg(data, len, offset, &submsg_hdr, &next_offset) != 0) {
            break;
        }
        
        info->submsg_types[info->submsg_count++] = submsg_hdr.submsg_id;
        
        /* 处理DATA子消息 */
        if (submsg_hdr.submsg_id == RTPS_SUBMSG_DATA && offset + 24 <= len) {
            const uint8_t* submsg_data = data + offset + 4;
            /* 跳过extra_flags和octets_to_inline_qos */
            submsg_data += 4;
            
            /* 读取reader和writer GUID */
            memcpy(&info->dst_guid, submsg_data, sizeof(rtps_guid_t));
            submsg_data += sizeof(rtps_guid_t);
            memcpy(&info->src_guid, submsg_data, sizeof(rtps_guid_t));
            submsg_data += sizeof(rtps_guid_t);
            
            /* 读取序列号 */
            info->sequence_num = ((uint64_t)submsg_data[0] << 56) |
                                 ((uint64_t)submsg_data[1] << 48) |
                                 ((uint64_t)submsg_data[2] << 40) |
                                 ((uint64_t)submsg_data[3] << 32) |
                                 ((uint64_t)submsg_data[4] << 24) |
                                 ((uint64_t)submsg_data[5] << 16) |
                                 ((uint64_t)submsg_data[6] << 8) |
                                 submsg_data[7];
        }
        
        offset = next_offset;
    }
    
    return 0;
}

int dds_parse_rtps_submsg(const uint8_t* data, uint32_t len, uint32_t offset,
                          rtps_submsg_header_t* header, uint32_t* next_offset) {
    if (offset + sizeof(rtps_submsg_header_t) > len) {
        return -1;
    }
    
    const rtps_submsg_header_t* hdr = (const rtps_submsg_header_t*)(data + offset);
    header->submsg_id = hdr->submsg_id;
    header->flags = hdr->flags;
    header->submsg_length = ntohs(hdr->submsg_length);
    
    uint32_t submsg_len = header->submsg_length;
    if (submsg_len == 0) {
        /* 剩余数据 */
        submsg_len = len - offset - sizeof(rtps_submsg_header_t);
    }
    
    *next_offset = offset + sizeof(rtps_submsg_header_t) + submsg_len;
    
    /* 对齐 */
    if (*next_offset % 4 != 0) {
        *next_offset += 4 - (*next_offset % 4);
    }
    
    return 0;
}

/*==============================================================================
 * 过滤器实现
 *============================================================================*/

static bool apply_filters(dds_capture_session_t* session, const dds_packet_info_t* info) {
    if (session->filter_count == 0) {
        return true;
    }
    
    for (uint32_t i = 0; i < session->filter_count; i++) {
        const dds_filter_t* filter = &session->filters[i];
        bool match = false;
        
        switch (filter->type) {
            case DDS_FILTER_IP:
                match = (strcmp(info->src_ip, filter->value.ip) == 0 ||
                        strcmp(info->dst_ip, filter->value.ip) == 0);
                break;
                
            case DDS_FILTER_PORT:
                match = (info->src_port == filter->value.port ||
                        info->dst_port == filter->value.port);
                break;
                
            case DDS_FILTER_TOPIC:
                match = (strstr(info->topic_name, filter->value.topic) != NULL);
                break;
                
            case DDS_FILTER_SUBMSG_TYPE:
                for (int j = 0; j < info->submsg_count; j++) {
                    if (info->submsg_types[j] == filter->value.submsg_type) {
                        match = true;
                        break;
                    }
                }
                break;
                
            default:
                match = true;
                break;
        }
        
        if (filter->invert) {
            match = !match;
        }
        
        if (!match) {
            return false;
        }
    }
    
    return true;
}

int dds_capture_add_filter(dds_capture_session_t* session, const dds_filter_t* filter) {
    if (!session || !filter || session->magic != DDS_ANALYZER_MAGIC) {
        return -1;
    }
    
    pthread_mutex_lock(&session->mutex);
    
    if (session->filter_count >= session->filter_capacity) {
        uint32_t new_capacity = session->filter_capacity * 2;
        dds_filter_t* new_filters = realloc(session->filters, 
                                            new_capacity * sizeof(dds_filter_t));
        if (!new_filters) {
            pthread_mutex_unlock(&session->mutex);
            return -1;
        }
        session->filters = new_filters;
        session->filter_capacity = new_capacity;
    }
    
    memcpy(&session->filters[session->filter_count++], filter, sizeof(dds_filter_t));
    
    pthread_mutex_unlock(&session->mutex);
    
    return 0;
}

void dds_capture_clear_filters(dds_capture_session_t* session) {
    if (!session || session->magic != DDS_ANALYZER_MAGIC) {
        return;
    }
    
    pthread_mutex_lock(&session->mutex);
    session->filter_count = 0;
    pthread_mutex_unlock(&session->mutex);
}

int dds_capture_set_bpf_filter(dds_capture_session_t* session, const char* filter) {
    if (!session || !filter || session->magic != DDS_ANALYZER_MAGIC) {
        return -1;
    }
    
    pthread_mutex_lock(&session->mutex);
    strncpy(session->config.pcap_filter, filter, DDS_ANALYZER_MAX_FILTER_LEN - 1);
    pthread_mutex_unlock(&session->mutex);
    
    return 0;
}

/*==============================================================================
 * 回调设置
 *============================================================================*/

void dds_capture_set_packet_callback(dds_capture_session_t* session,
                                     dds_packet_callback_t callback,
                                     void* user_data) {
    if (!session || session->magic != DDS_ANALYZER_MAGIC) return;
    
    pthread_mutex_lock(&session->mutex);
    session->packet_callback = callback;
    session->packet_user_data = user_data;
    pthread_mutex_unlock(&session->mutex);
}

void dds_capture_set_stats_callback(dds_capture_session_t* session,
                                    dds_stats_callback_t callback,
                                    uint64_t interval_ms) {
    if (!session || session->magic != DDS_ANALYZER_MAGIC) return;
    
    pthread_mutex_lock(&session->mutex);
    session->stats_callback = callback;
    session->stats_interval_ms = interval_ms;
    pthread_mutex_unlock(&session->mutex);
}

void dds_capture_set_error_callback(dds_capture_session_t* session,
                                    dds_error_callback_t callback,
                                    void* user_data) {
    if (!session || session->magic != DDS_ANALYZER_MAGIC) return;
    
    pthread_mutex_lock(&session->mutex);
    session->error_callback = callback;
    session->error_user_data = user_data;
    pthread_mutex_unlock(&session->mutex);
}

/*==============================================================================
 * 统计和状态
 *============================================================================*/

static void update_stats(dds_capture_session_t* session, const dds_packet_info_t* info) {
    pthread_mutex_lock(&session->stats_mutex);
    
    session->stats.total_packets++;
    session->stats.total_bytes += info->packet_len;
    
    if (info->is_rtps) {
        session->stats.rtps_packets++;
        session->stats.rtps_bytes += info->payload_len;
        
        /* 统计子消息 */
        for (int i = 0; i < info->submsg_count; i++) {
            switch (info->submsg_types[i]) {
                case RTPS_SUBMSG_DATA:
                    session->stats.data_submsg_count++;
                    break;
                case RTPS_SUBMSG_HEARTBEAT:
                    session->stats.heartbeat_count++;
                    break;
                case RTPS_SUBMSG_ACKNACK:
                    session->stats.acknack_count++;
                    break;
                case RTPS_SUBMSG_GAP:
                    session->stats.gap_count++;
                    break;
            }
        }
    }
    
    /* 更新时间 */
    uint64_t now = info->timestamp.tv_sec * 1000000ULL + info->timestamp.tv_usec;
    if (session->stats.first_packet_time == 0) {
        session->stats.first_packet_time = now;
    }
    session->stats.last_packet_time = now;
    
    uint64_t duration = now - session->stats.first_packet_time;
    if (duration > 0) {
        session->stats.capture_duration_us = duration;
        session->stats.packets_per_sec = (session->stats.total_packets * 1000000ULL) / duration;
        session->stats.bytes_per_sec = (session->stats.total_bytes * 1000000ULL) / duration;
        session->stats.avg_packet_size = (double)session->stats.total_bytes / session->stats.total_packets;
    }
    
    pthread_mutex_unlock(&session->stats_mutex);
}

void dds_capture_get_stats(dds_capture_session_t* session, dds_traffic_stats_t* stats) {
    if (!session || !stats || session->magic != DDS_ANALYZER_MAGIC) return;
    
    pthread_mutex_lock(&session->stats_mutex);
    memcpy(stats, &session->stats, sizeof(dds_traffic_stats_t));
    pthread_mutex_unlock(&session->stats_mutex);
}

void dds_capture_reset_stats(dds_capture_session_t* session) {
    if (!session || session->magic != DDS_ANALYZER_MAGIC) return;
    
    pthread_mutex_lock(&session->stats_mutex);
    memset(&session->stats, 0, sizeof(session->stats));
    pthread_mutex_unlock(&session->stats_mutex);
}

int dds_capture_get_last_error(dds_capture_session_t* session, char* msg, size_t msg_size) {
    if (!session || !msg || msg_size == 0 || session->magic != DDS_ANALYZER_MAGIC) {
        return -1;
    }
    
    pthread_mutex_lock(&session->mutex);
    strncpy(msg, session->error_msg, msg_size - 1);
    msg[msg_size - 1] = '\0';
    int err = session->last_error;
    pthread_mutex_unlock(&session->mutex);
    
    return err;
}

static void report_error(dds_capture_session_t* session, int code, const char* msg) {
    pthread_mutex_lock(&session->mutex);
    session->last_error = code;
    strncpy(session->error_msg, msg, sizeof(session->error_msg) - 1);
    pthread_mutex_unlock(&session->mutex);
    
    if (session->error_callback) {
        session->error_callback(code, msg, session->error_user_data);
    }
}

/*==============================================================================
 * pcap文件操作
 *============================================================================*/

int dds_capture_save_to_file(dds_capture_session_t* session, const char* filename) {
    if (!session || !filename || session->magic != DDS_ANALYZER_MAGIC) {
        return -1;
    }
    
    /* 实时保存在捕获线程中完成 */
    pthread_mutex_lock(&session->mutex);
    if (!session->running) {
        pthread_mutex_unlock(&session->mutex);
        return -1;
    }
    strncpy(session->config.output_file, filename, sizeof(session->config.output_file) - 1);
    session->config.save_to_file = true;
    pthread_mutex_unlock(&session->mutex);
    
    return 0;
}

int dds_capture_load_from_file(const char* filename,
                               dds_packet_callback_t callback,
                               void* user_data) {
    if (!filename) return -1;
    
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* handle = pcap_open_offline(filename, errbuf);
    if (!handle) {
        return -1;
    }
    
    struct pcap_pkthdr* header;
    const u_char* data;
    
    while (pcap_next_ex(handle, &header, &data) == 1) {
        if (callback) {
            dds_packet_info_t info;
            memset(&info, 0, sizeof(info));
            info.timestamp = header->ts;
            info.timestamp_ns = (uint64_t)header->ts.tv_sec * 1000000000ULL + 
                               header->ts.tv_usec * 1000;
            info.packet_len = header->len;
            info.raw_data = data;
            info.raw_len = header->caplen;
            
            parse_ethernet_frame(data, header->caplen, &info);
            
            callback(&info, user_data);
        }
    }
    
    pcap_close(handle);
    return 0;
}

int dds_capture_export_pcap(dds_capture_session_t* session, const char* filename,
                            uint64_t start_time, uint64_t end_time) {
    (void)session;
    (void)filename;
    (void)start_time;
    (void)end_time;
    /* 实现较为复杂，简化处理 */
    return 0;
}

int dds_capture_merge_pcap(const char** input_files, uint32_t file_count, const char* output_file) {
    if (!input_files || file_count == 0 || !output_file) return -1;
    
    pcap_t* out_handle = NULL;
    pcap_dumper_t* dumper = NULL;
    
    for (uint32_t i = 0; i < file_count; i++) {
        char errbuf[PCAP_ERRBUF_SIZE];
        pcap_t* in_handle = pcap_open_offline(input_files[i], errbuf);
        if (!in_handle) continue;
        
        if (!out_handle) {
            out_handle = pcap_open_dead(pcap_datalink(in_handle), 65535);
            dumper = pcap_dump_open(out_handle, output_file);
        }
        
        if (dumper) {
            struct pcap_pkthdr* header;
            const u_char* data;
            while (pcap_next_ex(in_handle, &header, &data) == 1) {
                pcap_dump((u_char*)dumper, header, data);
            }
        }
        
        pcap_close(in_handle);
    }
    
    if (dumper) pcap_dump_close(dumper);
    if (out_handle) pcap_close(out_handle);
    
    return 0;
}

/*==============================================================================
 * 工具函数
 *============================================================================*/

void dds_decode_entity_id(uint32_t entity_id, rtps_entity_id_t* decoded) {
    if (!decoded) return;
    
    decoded->key[0] = (entity_id >> 24) & 0xFF;
    decoded->key[1] = (entity_id >> 16) & 0xFF;
    decoded->key[2] = (entity_id >> 8) & 0xFF;
    decoded->kind = entity_id & 0xFF;
}

const char* dds_get_entity_type_name(rtps_entity_type_t type) {
    switch (type) {
        case RTPS_ENTITY_PARTICIPANT:   return "Participant";
        case RTPS_ENTITY_WRITER:        return "Writer";
        case RTPS_ENTITY_READER:        return "Reader";
        case RTPS_ENTITY_TOPIC:         return "Topic";
        case RTPS_ENTITY_PUBLISHER:     return "Publisher";
        case RTPS_ENTITY_SUBSCRIBER:    return "Subscriber";
        default:                        return "Unknown";
    }
}

const char* dds_get_submsg_type_name(uint8_t type) {
    switch (type) {
        case RTPS_SUBMSG_PAD:           return "PAD";
        case RTPS_SUBMSG_ACKNACK:       return "ACKNACK";
        case RTPS_SUBMSG_HEARTBEAT:     return "HEARTBEAT";
        case RTPS_SUBMSG_GAP:           return "GAP";
        case RTPS_SUBMSG_INFO_TS:       return "INFO_TS";
        case RTPS_SUBMSG_INFO_SRC:      return "INFO_SRC";
        case RTPS_SUBMSG_INFO_DST:      return "INFO_DST";
        case RTPS_SUBMSG_DATA:          return "DATA";
        case RTPS_SUBMSG_DATA_FRAG:     return "DATA_FRAG";
        case RTPS_SUBMSG_NOKEY_DATA:    return "NOKEY_DATA";
        case RTPS_SUBMSG_SEC_BODY:      return "SEC_BODY";
        case RTPS_SUBMSG_SEC_PREFIX:    return "SEC_PREFIX";
        case RTPS_SUBMSG_SRTPS_PREFIX:  return "SRTPS_PREFIX";
        default:                        return "UNKNOWN";
    }
}

void dds_guid_to_string(const rtps_guid_t* guid, char* str, size_t str_size) {
    if (!guid || !str || str_size == 0) return;
    
    snprintf(str, str_size, "%08x.%08x.%08x.%08x",
             guid->prefix[0], guid->prefix[1], guid->prefix[2],
             guid->entity_id);
}

/*==============================================================================
 * UDS支持
 *============================================================================*/

int dds_decode_uds_encapsulation(const uint8_t* data, uint32_t len,
                                 uint8_t* uds_service_id, uint8_t* uds_response_code,
                                 uint8_t** payload, uint32_t* payload_len) {
    if (!data || len < 4) return -1;
    
    /* 检查UDS封装标识 */
    if (data[0] != 0xAA || data[1] != 0x55) {
        return -1;
    }
    
    if (uds_service_id) *uds_service_id = data[2];
    if (uds_response_code) *uds_response_code = data[3];
    
    if (payload) *payload = (uint8_t*)(data + 4);
    if (payload_len) *payload_len = len - 4;
    
    return 0;
}

int dds_decode_can_bridge(const uint8_t* data, uint32_t len,
                          uint32_t* can_id, uint8_t* can_dlc, uint8_t* can_data) {
    if (!data || len < 8) return -1;
    
    /* CAN桥接格式: CAN_ID(4) + DLC(1) + DATA(8) */
    if (can_id) {
        *can_id = ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
                  ((uint32_t)data[2] << 8) | data[3];
    }
    
    if (can_dlc) {
        *can_dlc = data[4] & 0x0F;
        if (*can_dlc > 8) *can_dlc = 8;
    }
    
    if (can_data) {
        memcpy(can_data, data + 5, 8);
    }
    
    return 0;
}

/*==============================================================================
 * 高级分析（简化版本）
 *============================================================================*/

int dds_analyze_topics(dds_capture_session_t* session,
                       char** topic_names, uint64_t* packet_counts, uint32_t max_topics) {
    (void)session;
    (void)topic_names;
    (void)packet_counts;
    (void)max_topics;
    return 0;
}

int dds_analyze_nodes(dds_capture_session_t* session, rtps_guid_t* node_guids, uint32_t max_nodes) {
    (void)session;
    (void)node_guids;
    (void)max_nodes;
    return 0;
}

int dds_generate_traffic_graph(dds_capture_session_t* session,
                               uint64_t* timestamps, uint64_t* packet_counts,
                               uint64_t* byte_counts, uint32_t* data_points) {
    (void)session;
    (void)timestamps;
    (void)packet_counts;
    (void)byte_counts;
    (void)data_points;
    return 0;
}

int dds_detect_anomalies(dds_capture_session_t* session,
                         char** anomaly_descriptions, uint32_t max_anomalies) {
    (void)session;
    (void)anomaly_descriptions;
    (void)max_anomalies;
    return 0;
}
