/*******************************************************************************
 * @file dds_monitor_cli.c
 * @brief DDS监控工具CLI - 统一监控工具入口
 *
 * 功能特点：
 * - 实时监控模式
 * - 报告生成
 * - 交互式命令
 * - 车载UDS协议支持
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

/* 包含监控头文件 */
#include "qos_monitor.h"
#include "health_check.h"
#include "../dds_analyzer/dds_analyzer.h"
#include "../../src/common/log/dds_log.h"

/*==============================================================================
 * 版本和常量
 *============================================================================*/
#define DDS_MONITOR_CLI_VERSION     "1.0.0"
#define MAX_LINE_LENGTH             1024
#define REFRESH_INTERVAL_MS         1000

/*==============================================================================
 * 运行模式
 *============================================================================*/
typedef enum {
    MODE_NONE,
    MODE_REALTIME,      /* 实时监控 */
    MODE_ONESHOT,       /* 单次检查 */
    MODE_CAPTURE,       /* 报文捕获 */
    MODE_REPORT,        /* 报告生成 */
    MODE_INTERACTIVE    /* 交互模式 */
} run_mode_t;

/*==============================================================================
 * CLI配置
 *============================================================================*/
typedef struct {
    run_mode_t          mode;
    char                interface[32];
    char                output_file[256];
    char                input_file[256];
    uint32_t            duration_sec;
    uint32_t            refresh_ms;
    bool                verbose;
    bool                use_json;
    bool                enable_qos;
    bool                enable_health;
    bool                enable_analyzer;
    char                filter[256];
    char                topic[128];
} cli_config_t;

/*==============================================================================
 * 全局状态
 *============================================================================*/
static volatile bool g_running = false;
static volatile bool g_refresh_needed = false;
static cli_config_t g_config = {0};

/*==============================================================================
 * 前向声明
 *============================================================================*/
static void print_usage(const char* program);
static void print_version(void);
static int parse_args(int argc, char* argv[], cli_config_t* config);
static void signal_handler(int sig);
static void setup_signal_handlers(void);
static uint64_t get_timestamp_ms(void);

/* 模式函数 */
static int run_realtime_mode(const cli_config_t* config);
static int run_oneshot_mode(const cli_config_t* config);
static int run_capture_mode(const cli_config_t* config);
static int run_report_mode(const cli_config_t* config);
static int run_interactive_mode(const cli_config_t* config);

/* 显示函数 */
static void clear_screen(void);
static void print_header(void);
static void print_qos_status(const dds_qos_status_t* status, const char* name);
static void print_health_report(const dds_health_report_t* report);
static void print_capture_stats(const dds_traffic_stats_t* stats);

/* 回调函数 */
static void qos_callback(const dds_qos_status_t* status, void* user_data);
static void health_callback(const dds_health_report_t* report, void* user_data);
static void alarm_callback(const dds_health_alarm_t* alarm, void* user_data);
static void packet_callback(const dds_packet_info_t* packet, void* user_data);
static void stats_callback(const dds_traffic_stats_t* stats, void* user_data);

/*==============================================================================
 * 工具函数
 *============================================================================*/

static uint64_t get_timestamp_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static void clear_screen(void) {
    printf("\033[2J\033[H");
}

static void print_header(void) {
    printf("================================================================================\n");
    printf("                    DDS Monitor CLI v%s\n", DDS_MONITOR_CLI_VERSION);
    printf("================================================================================\n");
    printf("  Mode: %s | Interface: %s | Refresh: %ums\n",
           g_config.mode == MODE_REALTIME ? "REALTIME" :
           g_config.mode == MODE_ONESHOT ? "ONESHOT" :
           g_config.mode == MODE_CAPTURE ? "CAPTURE" :
           g_config.mode == MODE_REPORT ? "REPORT" : "INTERACTIVE",
           strlen(g_config.interface) > 0 ? g_config.interface : "default",
           g_config.refresh_ms);
    printf("--------------------------------------------------------------------------------\n");
}

/*==============================================================================
 * 帮助和版本
 *============================================================================*/

static void print_usage(const char* program) {
    printf("Usage: %s [OPTIONS]\n\n", program);
    printf("DDS Monitor CLI - 车载DDS监控诊断工具\n\n");
    printf("Options:\n");
    printf("  -m, --mode MODE          运行模式: realtime, oneshot, capture, report, interactive\n");
    printf("  -i, --interface IF       网络接口名称 (默认: any)\n");
    printf("  -o, --output FILE        输出文件\n");
    printf("  -f, --file FILE          输入文件 (pcap)\n");
    printf("  -d, --duration SEC       捕获/监控持续时间 (秒)\n");
    printf("  -r, --refresh MS         刷新间隔 (毫秒, 默认: 1000)\n");
    printf("  -t, --topic TOPIC        过滤主题名\n");
    printf("  --filter FILTER          BPF过滤器表达式\n");
    printf("  --qos                    启用QoS监控\n");
    printf("  --health                 启用健康检查\n");
    printf("  --analyzer               启用流量分析\n");
    printf("  --json                   输出JSON格式\n");
    printf("  -v, --verbose            详细输出\n");
    printf("  -h, --help               显示帮助\n");
    printf("  --version                显示版本\n");
    printf("\nExamples:\n");
    printf("  %s -m realtime --qos --health\n", program);
    printf("  %s -m capture -i eth0 -d 60 -o capture.pcap\n", program);
    printf("  %s -m oneshot --health -v\n", program);
    printf("  %s -m report --qos --json -o report.json\n", program);
}

static void print_version(void) {
    printf("DDS Monitor CLI version %s\n", DDS_MONITOR_CLI_VERSION);
    printf("  QoS Monitor:    %d.%d.%d\n", 
           DDS_QOS_MONITOR_VERSION_MAJOR,
           DDS_QOS_MONITOR_VERSION_MINOR,
           DDS_QOS_MONITOR_VERSION_PATCH);
    printf("  Health Check:   %d.%d.%d\n",
           DDS_HEALTH_VERSION_MAJOR,
           DDS_HEALTH_VERSION_MINOR,
           DDS_HEALTH_VERSION_PATCH);
    printf("  DDS Analyzer:   %d.%d.%d\n",
           DDS_ANALYZER_VERSION_MAJOR,
           DDS_ANALYZER_VERSION_MINOR,
           DDS_ANALYZER_VERSION_PATCH);
}

/*==============================================================================
 * 参数解析
 *============================================================================*/

static int parse_args(int argc, char* argv[], cli_config_t* config) {
    /* 默认配置 */
    memset(config, 0, sizeof(cli_config_t));
    config->mode = MODE_NONE;
    config->refresh_ms = REFRESH_INTERVAL_MS;
    config->duration_sec = 0;
    config->verbose = false;
    config->use_json = false;
    config->enable_qos = false;
    config->enable_health = false;
    config->enable_analyzer = false;
    strcpy(config->interface, "any");
    
    static struct option long_options[] = {
        {"mode",        required_argument, 0, 'm'},
        {"interface",   required_argument, 0, 'i'},
        {"output",      required_argument, 0, 'o'},
        {"file",        required_argument, 0, 'f'},
        {"duration",    required_argument, 0, 'd'},
        {"refresh",     required_argument, 0, 'r'},
        {"topic",       required_argument, 0, 't'},
        {"filter",      required_argument, 0, 1},
        {"qos",         no_argument,       0, 2},
        {"health",      no_argument,       0, 3},
        {"analyzer",    no_argument,       0, 4},
        {"json",        no_argument,       0, 5},
        {"verbose",     no_argument,       0, 'v'},
        {"help",        no_argument,       0, 'h'},
        {"version",     no_argument,       0, 6},
        {0, 0, 0, 0}
    };
    
    int c;
    int option_index = 0;
    
    while ((c = getopt_long(argc, argv, "m:i:o:f:d:r:t:vh",
                            long_options, &option_index)) != -1) {
        switch (c) {
            case 'm':
                if (strcmp(optarg, "realtime") == 0) {
                    config->mode = MODE_REALTIME;
                } else if (strcmp(optarg, "oneshot") == 0) {
                    config->mode = MODE_ONESHOT;
                } else if (strcmp(optarg, "capture") == 0) {
                    config->mode = MODE_CAPTURE;
                } else if (strcmp(optarg, "report") == 0) {
                    config->mode = MODE_REPORT;
                } else if (strcmp(optarg, "interactive") == 0) {
                    config->mode = MODE_INTERACTIVE;
                } else {
                    fprintf(stderr, "Error: Unknown mode '%s'\n", optarg);
                    return -1;
                }
                break;
                
            case 'i':
                strncpy(config->interface, optarg, sizeof(config->interface) - 1);
                break;
                
            case 'o':
                strncpy(config->output_file, optarg, sizeof(config->output_file) - 1);
                break;
                
            case 'f':
                strncpy(config->input_file, optarg, sizeof(config->input_file) - 1);
                break;
                
            case 'd':
                config->duration_sec = atoi(optarg);
                break;
                
            case 'r':
                config->refresh_ms = atoi(optarg);
                break;
                
            case 't':
                strncpy(config->topic, optarg, sizeof(config->topic) - 1);
                break;
                
            case 1: /* --filter */
                strncpy(config->filter, optarg, sizeof(config->filter) - 1);
                break;
                
            case 2: /* --qos */
                config->enable_qos = true;
                break;
                
            case 3: /* --health */
                config->enable_health = true;
                break;
                
            case 4: /* --analyzer */
                config->enable_analyzer = true;
                break;
                
            case 5: /* --json */
                config->use_json = true;
                break;
                
            case 'v':
                config->verbose = true;
                break;
                
            case 'h':
                print_usage(argv[0]);
                exit(0);
                
            case 6: /* --version */
                print_version();
                exit(0);
                
            default:
                return -1;
        }
    }
    
    /* 验证必需参数 */
    if (config->mode == MODE_NONE) {
        /* 默认为interactive模式 */
        config->mode = MODE_INTERACTIVE;
    }
    
    return 0;
}

/*==============================================================================
 * 信号处理
 *============================================================================*/

static void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        g_running = false;
    }
}

static void setup_signal_handlers(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);
}

/*==============================================================================
 * 显示函数
 *============================================================================*/

static void print_qos_status(const dds_qos_status_t* status, const char* name) {
    printf("\n--- QoS Status: %s ---\n", name ? name : "Unknown");
    printf("Overall Score: %u/100\n", status->overall_score);
    printf("  Latency:     %u/100\n", status->latency_score);
    printf("  Throughput:  %u/100\n", status->throughput_score);
    printf("  Reliability: %u/100\n", status->reliability_score);
    
    printf("\nLatency:\n");
    printf("  Min: %lu us, Max: %lu us\n",
           (unsigned long)status->latency.min_us,
           (unsigned long)status->latency.max_us);
    printf("  Avg: %.2f us, P99: %lu us\n",
           status->latency.avg_us,
           (unsigned long)status->latency.p99_us);
    printf("  P999: %lu us\n", (unsigned long)status->latency.p999_us);
    
    printf("\nThroughput:\n");
    printf("  Current: %.2f Mbps, Peak: %.2f Mbps\n",
           status->throughput.current_mbps,
           status->throughput.peak_mbps);
    printf("  Messages: %lu\n", (unsigned long)status->throughput.total_messages);
    
    printf("\nLoss Statistics:\n");
    printf("  Loss Rate: %u ppm\n", status->loss.loss_rate_ppm);
    printf("  Health Score: %u/100\n", status->loss.health_score);
    printf("  Connection: %s\n", status->loss.connection_healthy ? "HEALTHY" : "DEGRADED");
    
    if (status->has_warnings || status->has_errors || status->has_critical) {
        printf("\nAlerts:\n");
        if (status->has_critical) {
            printf("  [CRITICAL] %u issues\n", status->critical_count);
        }
        if (status->has_errors) {
            printf("  [ERROR] %u issues\n", status->error_count);
        }
        if (status->has_warnings) {
            printf("  [WARNING] %u issues\n", status->warning_count);
        }
    }
}

static void print_health_report(const dds_health_report_t* report) {
    printf("\n--- Health Report ---\n");
    printf("Overall Status: %s\n",
           report->overall_status == DDS_HEALTH_HEALTHY ? "HEALTHY" :
           report->overall_status == DDS_HEALTH_DEGRADED ? "DEGRADED" :
           report->overall_status == DDS_HEALTH_UNHEALTHY ? "UNHEALTHY" :
           report->overall_status == DDS_HEALTH_CRITICAL ? "CRITICAL" : "UNKNOWN");
    printf("Overall Score: %u/100\n", report->overall_score);
    printf("Uptime: %lu seconds\n", (unsigned long)report->uptime_seconds);
    
    printf("\nNodes:\n");
    printf("  Healthy:   %u\n", report->healthy_nodes);
    printf("  Degraded:  %u\n", report->degraded_nodes);
    printf("  Unhealthy: %u\n", report->unhealthy_nodes);
    printf("  Offline:   %u\n", report->offline_nodes);
    
    printf("\nTopics:\n");
    printf("  Matched:   %u\n", report->matched_topics);
    printf("  Unmatched: %u\n", report->unmatched_topics);
    
    printf("\nConnections:\n");
    printf("  Healthy:   %u\n", report->healthy_connections);
    printf("  Degraded:  %u\n", report->degraded_connections);
    
    printf("\nActive Alarms: %u\n", report->active_alarms);
    if (report->critical_count > 0) {
        printf("  Critical: %u\n", report->critical_count);
    }
    if (report->error_count > 0) {
        printf("  Error: %u\n", report->error_count);
    }
    if (report->warning_count > 0) {
        printf("  Warning: %u\n", report->warning_count);
    }
}

static void print_capture_stats(const dds_traffic_stats_t* stats) {
    printf("\n--- Capture Statistics ---\n");
    printf("Total Packets: %lu\n", (unsigned long)stats->total_packets);
    printf("Total Bytes: %lu\n", (unsigned long)stats->total_bytes);
    printf("RTPS Packets: %lu\n", (unsigned long)stats->rtps_packets);
    printf("Packets/sec: %lu\n", (unsigned long)stats->packets_per_sec);
    printf("Bytes/sec: %lu\n", (unsigned long)stats->bytes_per_sec);
    printf("Avg Packet Size: %.2f\n", stats->avg_packet_size);
    printf("\nRTPS Submessages:\n");
    printf("  DATA: %lu\n", (unsigned long)stats->data_submsg_count);
    printf("  HEARTBEAT: %lu\n", (unsigned long)stats->heartbeat_count);
    printf("  ACKNACK: %lu\n", (unsigned long)stats->acknack_count);
    printf("  GAP: %lu\n", (unsigned long)stats->gap_count);
}

/*==============================================================================
 * 回调函数
 *============================================================================*/

static void qos_callback(const dds_qos_status_t* status, void* user_data) {
    (void)user_data;
    if (g_config.verbose) {
        print_qos_status(status, "Monitor");
    }
    g_refresh_needed = true;
}

static void health_callback(const dds_health_report_t* report, void* user_data) {
    (void)user_data;
    if (g_config.verbose) {
        print_health_report(report);
    }
    g_refresh_needed = true;
}

static void alarm_callback(const dds_health_alarm_t* alarm, void* user_data) {
    (void)user_data;
    printf("\n[!] ALARM: %s\n", alarm_type_to_string(alarm->type));
    printf("    Severity: %s\n",
           alarm->severity == DDS_ALARM_CRITICAL ? "CRITICAL" :
           alarm->severity == DDS_ALARM_ERROR ? "ERROR" : "WARNING");
    printf("    Description: %s\n", alarm->description);
    printf("    Entity: %s\n", alarm->affected_entity);
}

static void packet_callback(const dds_packet_info_t* packet, void* user_data) {
    (void)user_data;
    if (g_config.verbose && packet->is_rtps) {
        printf("[RTPS] %s:%u -> %s:%u | Seq: %lu | Len: %u\n",
               packet->src_ip, packet->src_port,
               packet->dst_ip, packet->dst_port,
               (unsigned long)packet->sequence_num,
               packet->payload_len);
    }
}

static void stats_callback(const dds_traffic_stats_t* stats, void* user_data) {
    (void)user_data;
    g_refresh_needed = true;
}

/*==============================================================================
 * 运行模式实现
 *============================================================================*/

static int run_realtime_mode(const cli_config_t* config) {
    printf("Starting realtime monitoring mode...\n");
    printf("Press Ctrl+C to stop.\n\n");
    
    /* 初始化系统 */
    dds_qos_monitor_init(NULL);
    dds_health_init(NULL);
    
    /* 创建监控实例 */
    dds_qos_obj_info_t qos_info = {
        .type = DDS_QOS_OBJ_PARTICIPANT,
        .name = "DDS_Monitor",
        .topic_name = ""
    };
    dds_qos_monitor_t* qos_monitor = dds_qos_monitor_create(&qos_info);
    
    /* 注册回调 */
    dds_qos_register_callback(qos_callback, config->refresh_ms, NULL);
    dds_health_register_status_callback(health_callback, config->refresh_ms, NULL);
    dds_health_register_alarm_callback(alarm_callback, NULL);
    
    /* 启动捕获（如果启用了analyzer） */
    dds_capture_session_t* capture = NULL;
    if (config->enable_analyzer) {
        dds_capture_config_t capture_config = {
            .promiscuous_mode = true,
            .max_duration_sec = config->duration_sec
        };
        strncpy(capture_config.interface_name, config->interface, 
                sizeof(capture_config.interface_name) - 1);
        
        capture = dds_capture_create(&capture_config);
        if (capture) {
            dds_capture_set_packet_callback(capture, packet_callback, NULL);
            dds_capture_set_stats_callback(capture, stats_callback, config->refresh_ms);
            dds_capture_start(capture);
        }
    }
    
    g_running = true;
    uint64_t start_time = get_timestamp_ms();
    
    while (g_running) {
        /* 清屏和显示 */
        if (g_refresh_needed) {
            clear_screen();
            print_header();
            
            /* 获取并显示QoS状态 */
            if (config->enable_qos && qos_monitor) {
                dds_qos_status_t status;
                dds_qos_get_status(qos_monitor, &status);
                print_qos_status(&status, "Realtime Monitor");
            }
            
            /* 获取并显示健康报告 */
            if (config->enable_health) {
                dds_health_report_t report;
                dds_health_get_report(&report);
                print_health_report(&report);
            }
            
            /* 显示捕获统计 */
            if (config->enable_analyzer && capture) {
                dds_traffic_stats_t stats;
                dds_capture_get_stats(capture, &stats);
                print_capture_stats(&stats);
            }
            
            g_refresh_needed = false;
        }
        
        /* 检查超时 */
        if (config->duration_sec > 0) {
            uint64_t elapsed = (get_timestamp_ms() - start_time) / 1000;
            if (elapsed >= config->duration_sec) {
                g_running = false;
            }
        }
        
        usleep(10000);  /* 10ms */
    }
    
    /* 清理 */
    if (capture) {
        dds_capture_stop(capture);
        dds_capture_destroy(capture);
    }
    
    dds_qos_monitor_destroy(qos_monitor);
    dds_health_deinit();
    dds_qos_monitor_deinit();
    
    printf("\nMonitoring stopped.\n");
    return 0;
}

static int run_oneshot_mode(const cli_config_t* config) {
    printf("Running oneshot health check...\n\n");
    
    /* 初始化 */
    dds_health_init(NULL);
    
    /* 执行健康检查 */
    dds_health_checker_t* checker = dds_health_checker_create("oneshot");
    dds_health_perform_check(checker);
    
    /* 获取并显示报告 */
    dds_health_report_t report;
    dds_health_get_report(&report);
    
    if (config->use_json) {
        char buffer[8192];
        dds_health_generate_json_report(buffer, sizeof(buffer));
        printf("%s\n", buffer);
    } else {
        print_health_report(&report);
    }
    
    /* 检查告警 */
    dds_health_alarm_t alarms[16];
    uint32_t alarm_count;
    dds_health_get_active_alarms(alarms, 16, &alarm_count);
    
    if (alarm_count > 0) {
        printf("\n--- Active Alarms (%u) ---\n", alarm_count);
        for (uint32_t i = 0; i < alarm_count && i < 16; i++) {
            printf("  [%s] %s: %s\n",
                   alarms[i].severity == DDS_ALARM_CRITICAL ? "CRITICAL" :
                   alarms[i].severity == DDS_ALARM_ERROR ? "ERROR" : "WARNING",
                   alarm_type_to_string(alarms[i].type),
                   alarms[i].description);
        }
    }
    
    /* 清理 */
    dds_health_checker_destroy(checker);
    dds_health_deinit();
    
    /* 返回码基于状态 */
    return (report.overall_status == DDS_HEALTH_HEALTHY) ? 0 :
           (report.overall_status == DDS_HEALTH_DEGRADED) ? 1 :
           (report.overall_status == DDS_HEALTH_UNHEALTHY) ? 2 : 3;
}

static int run_capture_mode(const cli_config_t* config) {
    printf("Starting packet capture on interface '%s'...\n", config->interface);
    if (config->duration_sec > 0) {
        printf("Duration: %u seconds\n", config->duration_sec);
    }
    printf("Press Ctrl+C to stop.\n\n");
    
    /* 初始化 */
    dds_analyzer_init();
    
    /* 配置捕获 */
    dds_capture_config_t capture_config = {
        .promiscuous_mode = true,
        .save_to_file = (strlen(config->output_file) > 0),
        .max_duration_sec = config->duration_sec
    };
    strncpy(capture_config.interface_name, config->interface,
            sizeof(capture_config.interface_name) - 1);
    if (strlen(config->output_file) > 0) {
        strncpy(capture_config.output_file, config->output_file,
                sizeof(capture_config.output_file) - 1);
    }
    if (strlen(config->filter) > 0) {
        strncpy(capture_config.pcap_filter, config->filter,
                sizeof(capture_config.pcap_filter) - 1);
    }
    
    /* 创建并启动捕获 */
    dds_capture_session_t* capture = dds_capture_create(&capture_config);
    if (!capture) {
        fprintf(stderr, "Error: Failed to create capture session\n");
        return -1;
    }
    
    dds_capture_set_packet_callback(capture, packet_callback, NULL);
    dds_capture_set_stats_callback(capture, stats_callback, 1000);
    
    if (dds_capture_start(capture) != 0) {
        fprintf(stderr, "Error: Failed to start capture\n");
        dds_capture_destroy(capture);
        return -1;
    }
    
    g_running = true;
    uint64_t last_stats_time = get_timestamp_ms();
    
    while (g_running) {
        dds_traffic_stats_t stats;
        dds_capture_get_stats(capture, &stats);
        
        /* 每秒显示统计 */
        uint64_t now = get_timestamp_ms();
        if (now - last_stats_time >= 1000) {
            clear_screen();
            print_header();
            print_capture_stats(&stats);
            last_stats_time = now;
        }
        
        /* 检查超时 */
        if (config->duration_sec > 0 && 
            stats.capture_duration_us / 1000000 >= config->duration_sec) {
            g_running = false;
        }
        
        usleep(10000);
    }
    
    /* 显示最终统计 */
    dds_traffic_stats_t final_stats;
    dds_capture_get_stats(capture, &final_stats);
    
    printf("\n--- Final Statistics ---\n");
    print_capture_stats(&final_stats);
    
    if (strlen(config->output_file) > 0) {
        printf("\nCapture saved to: %s\n", config->output_file);
    }
    
    /* 清理 */
    dds_capture_stop(capture);
    dds_capture_destroy(capture);
    dds_analyzer_deinit();
    
    return 0;
}

static int run_report_mode(const cli_config_t* config) {
    printf("Generating report...\n\n");
    
    /* 初始化 */
    dds_qos_monitor_init(NULL);
    dds_health_init(NULL);
    
    /* 创建监控实例 */
    dds_qos_obj_info_t qos_info = {
        .type = DDS_QOS_OBJ_PARTICIPANT,
        .name = "Report_Monitor",
        .topic_name = config->topic
    };
    dds_qos_monitor_t* qos_monitor = dds_qos_monitor_create(&qos_info);
    
    /* 等待一段时间采集数据 */
    printf("Collecting data");
    uint32_t collect_time = (config->duration_sec > 0) ? config->duration_sec : 5;
    for (uint32_t i = 0; i < collect_time && g_running; i++) {
        printf(".");
        fflush(stdout);
        sleep(1);
    }
    printf("\n\n");
    
    /* 生成报告 */
    if (strlen(config->output_file) > 0) {
        FILE* fp = fopen(config->output_file, "w");
        if (!fp) {
            fprintf(stderr, "Error: Cannot open output file '%s'\n", config->output_file);
            return -1;
        }
        
        if (config->use_json) {
            /* JSON报告 */
            fprintf(fp, "{\n  \"qos_report\": ");
            
            if (config->enable_qos && qos_monitor) {
                char buffer[8192];
                dds_qos_generate_json_report(qos_monitor, buffer, sizeof(buffer));
                fprintf(fp, "%s,\n", buffer);
            } else {
                fprintf(fp, "null,\n");
            }
            
            fprintf(fp, "  \"health_report\": ");
            
            if (config->enable_health) {
                char buffer[8192];
                dds_health_generate_json_report(buffer, sizeof(buffer));
                fprintf(fp, "%s\n", buffer);
            } else {
                fprintf(fp, "null\n");
            }
            
            fprintf(fp, "}\n");
        } else {
            /* 文本报告 */
            if (config->enable_qos && qos_monitor) {
                char buffer[4096];
                dds_qos_generate_text_report(qos_monitor, buffer, sizeof(buffer));
                fprintf(fp, "%s\n", buffer);
            }
            
            if (config->enable_health) {
                char buffer[4096];
                dds_health_generate_text_report(buffer, sizeof(buffer));
                fprintf(fp, "%s\n", buffer);
            }
        }
        
        fclose(fp);
        printf("Report saved to: %s\n", config->output_file);
    } else {
        /* 输出到控制台 */
        if (config->enable_qos && qos_monitor) {
            dds_qos_status_t status;
            dds_qos_get_status(qos_monitor, &status);
            print_qos_status(&status, "Report");
        }
        
        if (config->enable_health) {
            dds_health_report_t report;
            dds_health_get_report(&report);
            print_health_report(&report);
        }
    }
    
    /* 清理 */
    dds_qos_monitor_destroy(qos_monitor);
    dds_health_deinit();
    dds_qos_monitor_deinit();
    
    return 0;
}

static int run_interactive_mode(const cli_config_t* config) {
    printf("DDS Monitor CLI - Interactive Mode\n");
    printf("Type 'help' for available commands, 'quit' to exit.\n\n");
    
    /* 初始化 */
    dds_qos_monitor_init(NULL);
    dds_health_init(NULL);
    
    char line[MAX_LINE_LENGTH];
    
    while (g_running) {
        printf("dds-monitor> ");
        fflush(stdout);
        
        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }
        
        /* 移除换行 */
        line[strcspn(line, "\n")] = '\0';
        
        /* 解析命令 */
        char cmd[32];
        if (sscanf(line, "%31s", cmd) != 1) {
            continue;
        }
        
        if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            break;
        } else if (strcmp(cmd, "help") == 0) {
            printf("Available commands:\n");
            printf("  status      - Show current status\n");
            printf("  qos         - Show QoS statistics\n");
            printf("  health      - Show health report\n");
            printf("  nodes       - List all nodes\n");
            printf("  topics      - List all topics\n");
            printf("  alarms      - Show active alarms\n");
            printf("  check       - Perform health check\n");
            printf("  clear       - Clear screen\n");
            printf("  help        - Show this help\n");
            printf("  quit/exit   - Exit program\n");
        } else if (strcmp(cmd, "clear") == 0) {
            clear_screen();
        } else if (strcmp(cmd, "status") == 0) {
            dds_health_report_t report;
            dds_health_get_report(&report);
            print_health_report(&report);
        } else if (strcmp(cmd, "qos") == 0) {
            /* 创建临时监控器 */
            dds_qos_obj_info_t qos_info = {
                .type = DDS_QOS_OBJ_PARTICIPANT,
                .name = "Interactive_Monitor",
                .topic_name = ""
            };
            dds_qos_monitor_t* monitor = dds_qos_monitor_create(&qos_info);
            
            /* 等待2秒采集数据 */
            printf("Collecting data for 2 seconds...\n");
            sleep(2);
            
            dds_qos_status_t status;
            dds_qos_get_status(monitor, &status);
            print_qos_status(&status, "Interactive");
            
            dds_qos_monitor_destroy(monitor);
        } else if (strcmp(cmd, "health") == 0) {
            dds_health_report_t report;
            dds_health_get_report(&report);
            print_health_report(&report);
        } else if (strcmp(cmd, "nodes") == 0) {
            dds_node_health_t nodes[32];
            uint32_t count;
            dds_health_get_all_nodes(nodes, 32, &count);
            
            printf("\nRegistered Nodes (%u):\n", count);
            for (uint32_t i = 0; i < count; i++) {
                printf("  %s [%s]: %s\n",
                       nodes[i].node_name,
                       nodes[i].guid,
                       nodes[i].status == DDS_HEALTH_HEALTHY ? "HEALTHY" :
                       nodes[i].status == DDS_HEALTH_DEGRADED ? "DEGRADED" :
                       nodes[i].status == DDS_HEALTH_UNHEALTHY ? "UNHEALTHY" :
                       nodes[i].status == DDS_HEALTH_CRITICAL ? "CRITICAL" : "OFFLINE");
            }
        } else if (strcmp(cmd, "topics") == 0) {
            dds_topic_match_t topics[32];
            uint32_t count;
            dds_health_get_all_topics(topics, 32, &count);
            
            printf("\nRegistered Topics (%u):\n", count);
            for (uint32_t i = 0; i < count; i++) {
                printf("  %s: %s\n",
                       topics[i].topic_name,
                       topics[i].has_matched ? "MATCHED" : "UNMATCHED");
            }
        } else if (strcmp(cmd, "alarms") == 0) {
            dds_health_alarm_t alarms[16];
            uint32_t count;
            dds_health_get_active_alarms(alarms, 16, &count);
            
            if (count == 0) {
                printf("No active alarms.\n");
            } else {
                printf("\nActive Alarms (%u):\n", count);
                for (uint32_t i = 0; i < count; i++) {
                    printf("  [%u] %s: %s\n",
                           alarms[i].alarm_id,
                           alarm_type_to_string(alarms[i].type),
                           alarms[i].description);
                }
            }
        } else if (strcmp(cmd, "check") == 0) {
            printf("Performing health check...\n");
            dds_health_checker_t* checker = dds_health_checker_create("interactive");
            dds_health_perform_check(checker);
            dds_health_checker_destroy(checker);
            
            dds_health_report_t report;
            dds_health_get_report(&report);
            print_health_report(&report);
        } else {
            printf("Unknown command: %s\n", cmd);
            printf("Type 'help' for available commands.\n");
        }
    }
    
    /* 清理 */
    dds_health_deinit();
    dds_qos_monitor_deinit();
    
    printf("\nGoodbye!\n");
    return 0;
}

/*==============================================================================
 * 主函数
 *============================================================================*/

int main(int argc, char* argv[]) {
    /* 初始化日志系统 */
    dds_log_config_t log_config = {
        .level = DDS_LOG_LEVEL_INFO,
        .enable_console = true,
        .enable_file = false,
        .enable_color = true
    };
    dds_log_init(&log_config);
    
    DDS_LOG_INFO("DDS_MONITOR", "CLI", "DDS Monitor CLI starting v%s", DDS_MONITOR_CLI_VERSION);
    
    /* 解析参数 */
    if (parse_args(argc, argv, &g_config) != 0) {
        print_usage(argv[0]);
        dds_log_deinit();
        return 1;
    }
    
    /* 设置信号处理 */
    setup_signal_handlers();
    g_running = true;
    
    int result = 0;
    
    /* 执行对应模式 */
    switch (g_config.mode) {
        case MODE_REALTIME:
            result = run_realtime_mode(&g_config);
            break;
            
        case MODE_ONESHOT:
            result = run_oneshot_mode(&g_config);
            break;
            
        case MODE_CAPTURE:
            result = run_capture_mode(&g_config);
            break;
            
        case MODE_REPORT:
            result = run_report_mode(&g_config);
            break;
            
        case MODE_INTERACTIVE:
            result = run_interactive_mode(&g_config);
            break;
            
        default:
            fprintf(stderr, "Error: Unknown mode\n");
            print_usage(argv[0]);
            result = 1;
    }
    
    DDS_LOG_INFO("DDS_MONITOR", "CLI", "DDS Monitor CLI exiting with code %d", result);
    dds_log_deinit();
    
    return result;
}
