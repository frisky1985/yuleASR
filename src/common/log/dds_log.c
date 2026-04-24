/*******************************************************************************
 * @file dds_log.c
 * @brief DDS日志系统实现
 ******************************************************************************/

#include "dds_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <dirent.h>
#include <ctype.h>
#include <zlib.h>

/*==============================================================================
 * 宏和常量
 *============================================================================*/
#define DDS_LOG_MAGIC           0x4444534C  /* "DDSL" */
#define DDS_LOG_VERSION         1
#define DDS_LOG_LOCK_TIMEOUT_MS 100

/* 颜色代码 */
#define COLOR_RESET     "\033[0m"
#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_BLUE      "\033[34m"
#define COLOR_MAGENTA   "\033[35m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_WHITE     "\033[37m"

/*==============================================================================
 * 数据结构
 *============================================================================*/

/* 异步日志队列节点 */
typedef struct log_queue_node {
    dds_log_entry_t         entry;
    struct log_queue_node*  next;
} log_queue_node_t;

/* 异步日志队列 */
typedef struct {
    log_queue_node_t*   head;
    log_queue_node_t*   tail;
    uint32_t            size;
    uint32_t            capacity;
    pthread_mutex_t     mutex;
    pthread_cond_t      cond_not_empty;
    pthread_cond_t      cond_not_full;
    volatile bool       shutdown;
} log_queue_t;

/* 模块级别映射节点 */
typedef struct module_level_node {
    char                        module[DDS_LOG_MAX_MODULE_LEN];
    dds_log_level_t             level;
    struct module_level_node*   next;
} module_level_node_t;

/* 回调注册节点 */
typedef struct callback_node {
    void*                   callback;
    void*                   user_data;
    struct callback_node*   next;
} callback_node_t;

/* 全局日志状态 */
typedef struct {
    /* 状态 */
    volatile bool           initialized;
    uint32_t                magic;
    
    /* 配置 */
    dds_log_config_t        config;
    
    /* 线程同步 */
    pthread_mutex_t         mutex;
    pthread_rwlock_t        rwlock;
    
    /* 文件 */
    FILE*                   log_fp;
    char                    current_file[DDS_LOG_MAX_PATH_LEN];
    uint32_t                current_file_size;
    uint32_t                file_index;
    
    /* 异步队列 */
    log_queue_t             queue;
    pthread_t               flush_thread;
    volatile bool           flush_running;
    
    /* 模块级别 */
    module_level_node_t*    module_levels;
    
    /* 回调 */
    callback_node_t*        output_callbacks;
    callback_node_t*        audit_callbacks;
    dds_ota_upload_callback_t ota_callback;
    void*                   ota_user_data;
    dds_log_fatal_handler_t fatal_handler;
    
    /* UDS */
    char                    uds_session[DDS_LOG_UDS_SESSION_ID_LEN];
    pthread_mutex_t         uds_mutex;
    
    /* 统计 */
    dds_log_stats_t         stats;
    uint32_t                sequence_counter;
    pthread_mutex_t         stats_mutex;
    
    /* 审计 */
    bool                    audit_enabled;
    uint64_t                audit_mask;
} dds_log_state_t;

/*==============================================================================
 * 全局变量
 *============================================================================*/
static dds_log_state_t g_log_state = {0};
static __thread uint32_t thread_id = 0;
static uint32_t thread_counter = 0;
static pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;

/*==============================================================================
 * 内部函数前向声明
 *============================================================================*/
static const char* log_level_to_string(dds_log_level_t level);
static const char* log_level_to_color(dds_log_level_t level);
static void* flush_thread_func(void* arg);
static int rotate_log_file(void);
static void format_log_entry(const dds_log_entry_t* entry, char* buffer, size_t size, bool use_color);
static int write_to_file(const char* buffer);
static int queue_push(log_queue_t* queue, const dds_log_entry_t* entry);
static int queue_pop(log_queue_t* queue, dds_log_entry_t* entry);
static void queue_destroy(log_queue_t* queue);
static uint64_t get_timestamp_ns(void);

/*==============================================================================
 * 工具函数实现
 *============================================================================*/

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static uint32_t get_thread_id(void) {
    if (thread_id == 0) {
        pthread_mutex_lock(&thread_mutex);
        thread_id = ++thread_counter;
        pthread_mutex_unlock(&thread_mutex);
    }
    return thread_id;
}

static const char* log_level_to_string(dds_log_level_t level) {
    switch (level) {
        case DDS_LOG_LEVEL_FATAL: return DDS_LOG_LEVEL_FATAL_NAME;
        case DDS_LOG_LEVEL_ERROR: return DDS_LOG_LEVEL_ERROR_NAME;
        case DDS_LOG_LEVEL_WARN:  return DDS_LOG_LEVEL_WARN_NAME;
        case DDS_LOG_LEVEL_INFO:  return DDS_LOG_LEVEL_INFO_NAME;
        case DDS_LOG_LEVEL_DEBUG: return DDS_LOG_LEVEL_DEBUG_NAME;
        default: return "UNKNOWN";
    }
}

static const char* log_level_to_color(dds_log_level_t level) {
    switch (level) {
        case DDS_LOG_LEVEL_FATAL: return COLOR_MAGENTA;
        case DDS_LOG_LEVEL_ERROR: return COLOR_RED;
        case DDS_LOG_LEVEL_WARN:  return COLOR_YELLOW;
        case DDS_LOG_LEVEL_INFO:  return COLOR_GREEN;
        case DDS_LOG_LEVEL_DEBUG: return COLOR_CYAN;
        default: return COLOR_RESET;
    }
}

static const char* log_type_to_string(dds_log_type_t type) {
    switch (type) {
        case DDS_LOG_TYPE_RUNTIME:    return "RT";
        case DDS_LOG_TYPE_AUDIT:      return "AUDIT";
        case DDS_LOG_TYPE_SECURITY:   return "SEC";
        case DDS_LOG_TYPE_DIAGNOSTIC: return "DIAG";
        case DDS_LOG_TYPE_OTA:        return "OTA";
        default: return "UNK";
    }
}

/*==============================================================================
 * 队列操作
 *============================================================================*/

static int queue_init(log_queue_t* queue, uint32_t capacity) {
    memset(queue, 0, sizeof(log_queue_t));
    queue->capacity = capacity;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond_not_empty, NULL);
    pthread_cond_init(&queue->cond_not_full, NULL);
    return 0;
}

static int queue_push(log_queue_t* queue, const dds_log_entry_t* entry) {
    pthread_mutex_lock(&queue->mutex);
    
    if (queue->size >= queue->capacity) {
        pthread_mutex_unlock(&queue->mutex);
        return -1; /* 队列满 */
    }
    
    log_queue_node_t* node = malloc(sizeof(log_queue_node_t));
    if (!node) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    
    memcpy(&node->entry, entry, sizeof(dds_log_entry_t));
    node->next = NULL;
    
    if (queue->tail) {
        queue->tail->next = node;
    } else {
        queue->head = node;
    }
    queue->tail = node;
    queue->size++;
    
    pthread_cond_signal(&queue->cond_not_empty);
    pthread_mutex_unlock(&queue->mutex);
    
    return 0;
}

static int queue_pop(log_queue_t* queue, dds_log_entry_t* entry) {
    pthread_mutex_lock(&queue->mutex);
    
    while (queue->size == 0 && !queue->shutdown) {
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_nsec += 100 * 1000000; /* 100ms */
        if (timeout.tv_nsec >= 1000000000) {
            timeout.tv_sec++;
            timeout.tv_nsec -= 1000000000;
        }
        pthread_cond_timedwait(&queue->cond_not_empty, &queue->mutex, &timeout);
    }
    
    if (queue->size == 0) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    
    log_queue_node_t* node = queue->head;
    queue->head = node->next;
    if (!queue->head) {
        queue->tail = NULL;
    }
    queue->size--;
    
    memcpy(entry, &node->entry, sizeof(dds_log_entry_t));
    free(node);
    
    pthread_cond_signal(&queue->cond_not_full);
    pthread_mutex_unlock(&queue->mutex);
    
    return 0;
}

static void queue_destroy(log_queue_t* queue) {
    queue->shutdown = true;
    pthread_cond_broadcast(&queue->cond_not_empty);
    pthread_cond_broadcast(&queue->cond_not_full);
    
    pthread_mutex_lock(&queue->mutex);
    while (queue->head) {
        log_queue_node_t* node = queue->head;
        queue->head = node->next;
        free(node);
    }
    queue->size = 0;
    pthread_mutex_unlock(&queue->mutex);
    
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->cond_not_empty);
    pthread_cond_destroy(&queue->cond_not_full);
}

/*==============================================================================
 * 文件操作
 *============================================================================*/

static int ensure_directory(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return (S_ISDIR(st.st_mode)) ? 0 : -1;
    }
    return mkdir(path, 0755);
}

static int open_new_log_file(void) {
    if (g_log_state.log_fp) {
        fclose(g_log_state.log_fp);
        g_log_state.log_fp = NULL;
    }
    
    char filename[DDS_LOG_MAX_PATH_LEN];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    
    snprintf(filename, sizeof(filename), "%s/%s.%04d%02d%02d_%02d%02d%02d_%03d.log",
             g_log_state.config.log_dir,
             g_log_state.config.log_name,
             tm_info->tm_year + 1900,
             tm_info->tm_mon + 1,
             tm_info->tm_mday,
             tm_info->tm_hour,
             tm_info->tm_min,
             tm_info->tm_sec,
             g_log_state.file_index++);
    
    g_log_state.log_fp = fopen(filename, "w");
    if (!g_log_state.log_fp) {
        return -1;
    }
    
    strncpy(g_log_state.current_file, filename, DDS_LOG_MAX_PATH_LEN - 1);
    g_log_state.current_file_size = 0;
    
    /* 写入文件头 */
    fprintf(g_log_state.log_fp, "# DDS Log File v%d\n", DDS_LOG_VERSION);
    fprintf(g_log_state.log_fp, "# ECU: %s\n", g_log_state.config.ecu_id);
    fprintf(g_log_state.log_fp, "# Created: %04d-%02d-%02d %02d:%02d:%02d\n\n",
            tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
            tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    fflush(g_log_state.log_fp);
    
    return 0;
}

static int rotate_log_file(void) {
    if (!g_log_state.config.enable_rotation) {
        return 0;
    }
    
    return open_new_log_file();
}

static int write_to_file(const char* buffer) {
    if (!g_log_state.log_fp) {
        return -1;
    }
    
    size_t len = strlen(buffer);
    
    /* 检查是否需要轮转 */
    if (g_log_state.config.enable_rotation &&
        g_log_state.current_file_size + len > g_log_state.config.max_file_size) {
        rotate_log_file();
    }
    
    if (fputs(buffer, g_log_state.log_fp) == EOF) {
        return -1;
    }
    
    g_log_state.current_file_size += len;
    
    pthread_mutex_lock(&g_log_state.stats_mutex);
    g_log_state.stats.bytes_written += len;
    pthread_mutex_unlock(&g_log_state.stats_mutex);
    
    return 0;
}

/*==============================================================================
 * 格式化和输出
 *============================================================================*/

static void format_log_entry(const dds_log_entry_t* entry, char* buffer, size_t size, bool use_color) {
    char time_str[32];
    struct tm* tm_info = localtime(&entry->timestamp.tv_sec);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    int offset = 0;
    
    /* 时间戳 */
    if (g_log_state.config.enable_timestamp) {
        offset += snprintf(buffer + offset, size - offset,
            "%s[%s.%03ld]%s ",
            use_color ? COLOR_WHITE : "",
            time_str, entry->timestamp.tv_nsec / 1000000,
            use_color ? COLOR_RESET : "");
    }
    
    /* 日志级别和类型 */
    const char* level_str = log_level_to_string(entry->level);
    const char* color = use_color ? log_level_to_color(entry->level) : "";
    const char* reset = use_color ? COLOR_RESET : "";
    
    offset += snprintf(buffer + offset, size - offset,
        "%s[%s]%s<%s> ", color, level_str, reset, log_type_to_string(entry->type));
    
    /* ECU ID */
    if (strlen(entry->ecu_id) > 0) {
        offset += snprintf(buffer + offset, size - offset, "[%s] ", entry->ecu_id);
    }
    
    /* 模块和标签 */
    if (strlen(entry->module) > 0) {
        offset += snprintf(buffer + offset, size - offset, "[%s]", entry->module);
    }
    if (strlen(entry->tag) > 0) {
        offset += snprintf(buffer + offset, size - offset, "[%s]", entry->tag);
    }
    
    /* 位置信息 */
    if (g_log_state.config.enable_location && strlen(entry->file) > 0) {
        const char* filename = strrchr(entry->file, '/');
        if (!filename) filename = strrchr(entry->file, '\\');
        if (!filename) filename = entry->file;
        else filename++;
        offset += snprintf(buffer + offset, size - offset, " %s:%d", filename, entry->line);
    }
    
    /* 线程ID */
    if (g_log_state.config.enable_thread_id) {
        offset += snprintf(buffer + offset, size - offset, " [TID:%u]", entry->thread_id);
    }
    
    /* 序列号（审计日志） */
    if (entry->type == DDS_LOG_TYPE_AUDIT) {
        offset += snprintf(buffer + offset, size - offset, " [SEQ:%u]", entry->sequence_num);
    }
    
    /* 消息内容 */
    offset += snprintf(buffer + offset, size - offset, " %s\n", entry->message);
}

static void output_log(const dds_log_entry_t* entry) {
    char buffer[DDS_LOG_MAX_MESSAGE_LEN * 2];
    
    /* 控制台输出 */
    if (g_log_state.config.enable_console) {
        format_log_entry(entry, buffer, sizeof(buffer), g_log_state.config.enable_color);
        fputs(buffer, (entry->level <= DDS_LOG_LEVEL_ERROR) ? stderr : stdout);
    }
    
    /* 文件输出 */
    if (g_log_state.config.enable_file) {
        format_log_entry(entry, buffer, sizeof(buffer), false);
        pthread_mutex_lock(&g_log_state.mutex);
        write_to_file(buffer);
        pthread_mutex_unlock(&g_log_state.mutex);
    }
    
    /* 回调输出 */
    callback_node_t* node = g_log_state.output_callbacks;
    while (node) {
        dds_log_output_callback_t cb = (dds_log_output_callback_t)node->callback;
        if (cb) cb(entry, node->user_data);
        node = node->next;
    }
    
    /* 更新统计 */
    pthread_mutex_lock(&g_log_state.stats_mutex);
    g_log_state.stats.total_logs++;
    pthread_mutex_unlock(&g_log_state.stats_mutex);
}

static void* flush_thread_func(void* arg) {
    (void)arg;
    dds_log_entry_t entry;
    
    while (g_log_state.flush_running) {
        while (queue_pop(&g_log_state.queue, &entry) == 0) {
            output_log(&entry);
        }
        
        /* 定期刷新文件 */
        pthread_mutex_lock(&g_log_state.mutex);
        if (g_log_state.log_fp) {
            fflush(g_log_state.log_fp);
        }
        pthread_mutex_unlock(&g_log_state.mutex);
        
        pthread_mutex_lock(&g_log_state.stats_mutex);
        g_log_state.stats.last_flush_time = get_timestamp_ns();
        pthread_mutex_unlock(&g_log_state.stats_mutex);
        
        usleep(g_log_state.config.flush_interval_ms * 1000);
    }
    
    /* 清空队列 */
    while (queue_pop(&g_log_state.queue, &entry) == 0) {
        output_log(&entry);
    }
    
    return NULL;
}

/*==============================================================================
 * 公共接口实现
 *============================================================================*/

int dds_log_init(const dds_log_config_t* config) {
    if (g_log_state.initialized) {
        return -1;
    }
    
    memset(&g_log_state, 0, sizeof(g_log_state));
    
    /* 复制配置 */
    if (config) {
        memcpy(&g_log_state.config, config, sizeof(dds_log_config_t));
    } else {
        /* 默认配置 */
        g_log_state.config.level = DDS_LOG_LEVEL_INFO;
        g_log_state.config.enable_async = false;
        g_log_state.config.enable_console = true;
        g_log_state.config.enable_file = false;
        g_log_state.config.enable_rotation = false;
        g_log_state.config.enable_color = true;
        g_log_state.config.enable_timestamp = true;
        g_log_state.config.enable_location = true;
        g_log_state.config.enable_thread_id = false;
        g_log_state.config.max_file_size = DDS_LOG_FILE_SIZE_MAX;
        g_log_state.config.max_files = DDS_LOG_MAX_FILES;
        g_log_state.config.queue_size = DDS_LOG_QUEUE_SIZE;
        g_log_state.config.flush_interval_ms = 100;
        strcpy(g_log_state.config.log_name, "dds");
    }
    
    /* 初始化同步 */
    pthread_mutex_init(&g_log_state.mutex, NULL);
    pthread_rwlock_init(&g_log_state.rwlock, NULL);
    pthread_mutex_init(&g_log_state.stats_mutex, NULL);
    pthread_mutex_init(&g_log_state.uds_mutex, NULL);
    
    /* 创建日志目录 */
    if (g_log_state.config.enable_file) {
        if (ensure_directory(g_log_state.config.log_dir) != 0) {
            fprintf(stderr, "Failed to create log directory: %s\n", g_log_state.config.log_dir);
        }
        if (open_new_log_file() != 0) {
            fprintf(stderr, "Failed to open log file\n");
        }
    }
    
    /* 初始化异步队列 */
    if (g_log_state.config.enable_async) {
        queue_init(&g_log_state.queue, g_log_state.config.queue_size);
        g_log_state.flush_running = true;
        pthread_create(&g_log_state.flush_thread, NULL, flush_thread_func, NULL);
    }
    
    /* 设置ECU ID */
    if (strlen(g_log_state.config.ecu_id) == 0) {
        snprintf(g_log_state.config.ecu_id, DDS_LOG_ECUID_LEN, "ECU%04X", getpid() & 0xFFFF);
    }
    
    g_log_state.magic = DDS_LOG_MAGIC;
    g_log_state.initialized = true;
    
    DDS_LOG_INFO("DDS_LOG", "INIT", "DDS Log System initialized v%d.%d.%d",
                 DDS_LOG_VERSION_MAJOR, DDS_LOG_VERSION_MINOR, DDS_LOG_VERSION_PATCH);
    
    return 0;
}

void dds_log_deinit(void) {
    if (!g_log_state.initialized) {
        return;
    }
    
    DDS_LOG_INFO("DDS_LOG", "SHUTDOWN", "DDS Log System shutting down");
    
    /* 停止刷新线程 */
    if (g_log_state.config.enable_async) {
        g_log_state.flush_running = false;
        pthread_join(g_log_state.flush_thread, NULL);
        queue_destroy(&g_log_state.queue);
    }
    
    /* 关闭文件 */
    pthread_mutex_lock(&g_log_state.mutex);
    if (g_log_state.log_fp) {
        fflush(g_log_state.log_fp);
        fclose(g_log_state.log_fp);
        g_log_state.log_fp = NULL;
    }
    pthread_mutex_unlock(&g_log_state.mutex);
    
    /* 清理模块级别 */
    while (g_log_state.module_levels) {
        module_level_node_t* node = g_log_state.module_levels;
        g_log_state.module_levels = node->next;
        free(node);
    }
    
    /* 清理回调 */
    while (g_log_state.output_callbacks) {
        callback_node_t* node = g_log_state.output_callbacks;
        g_log_state.output_callbacks = node->next;
        free(node);
    }
    while (g_log_state.audit_callbacks) {
        callback_node_t* node = g_log_state.audit_callbacks;
        g_log_state.audit_callbacks = node->next;
        free(node);
    }
    
    /* 销毁同步 */
    pthread_mutex_destroy(&g_log_state.mutex);
    pthread_rwlock_destroy(&g_log_state.rwlock);
    pthread_mutex_destroy(&g_log_state.stats_mutex);
    pthread_mutex_destroy(&g_log_state.uds_mutex);
    
    g_log_state.initialized = false;
    g_log_state.magic = 0;
}

bool dds_log_is_initialized(void) {
    return g_log_state.initialized;
}

void dds_log_flush(void) {
    if (!g_log_state.initialized) return;
    
    if (g_log_state.config.enable_async) {
        /* 等待队列清空 */
        while (g_log_state.queue.size > 0) {
            usleep(1000);
        }
    }
    
    pthread_mutex_lock(&g_log_state.mutex);
    if (g_log_state.log_fp) {
        fflush(g_log_state.log_fp);
    }
    pthread_mutex_unlock(&g_log_state.mutex);
}

void dds_log_vwrite(dds_log_level_t level, dds_log_type_t type,
                    const char* module, const char* tag,
                    const char* file, int line,
                    const char* fmt, va_list args) {
    if (!g_log_state.initialized) {
        return;
    }
    
    /* 检查级别 */
    if (level > g_log_state.config.level) {
        return;
    }
    
    /* 检查模块级别 */
    pthread_rwlock_rdlock(&g_log_state.rwlock);
    module_level_node_t* mod_node = g_log_state.module_levels;
    while (mod_node) {
        if (strcmp(mod_node->module, module) == 0) {
            if (level > mod_node->level) {
                pthread_rwlock_unlock(&g_log_state.rwlock);
                return;
            }
            break;
        }
        mod_node = mod_node->next;
    }
    pthread_rwlock_unlock(&g_log_state.rwlock);
    
    /* 构建日志条目 */
    dds_log_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    
    entry.timestamp_ns = get_timestamp_ns();
    clock_gettime(CLOCK_REALTIME, &entry.timestamp);
    entry.level = level;
    entry.type = type;
    entry.line = line;
    entry.thread_id = get_thread_id();
    
    if (module) strncpy(entry.module, module, DDS_LOG_MAX_MODULE_LEN - 1);
    if (tag) strncpy(entry.tag, tag, DDS_LOG_MAX_TAG_LEN - 1);
    if (file) strncpy(entry.file, file, DDS_LOG_MAX_FILE_NAME - 1);
    
    /* 复制ECU ID */
    strncpy(entry.ecu_id, g_log_state.config.ecu_id, DDS_LOG_ECUID_LEN - 1);
    
    /* UDS会话 */
    pthread_mutex_lock(&g_log_state.uds_mutex);
    strncpy(entry.uds_session, g_log_state.uds_session, DDS_LOG_UDS_SESSION_ID_LEN - 1);
    pthread_mutex_unlock(&g_log_state.uds_mutex);
    
    /* 序列号 */
    entry.sequence_num = __sync_fetch_and_add(&g_log_state.sequence_counter, 1);
    
    /* 格式化消息 */
    vsnprintf(entry.message, DDS_LOG_MAX_MESSAGE_LEN - 1, fmt, args);
    
    /* 输出 */
    if (g_log_state.config.enable_async) {
        if (queue_push(&g_log_state.queue, &entry) != 0) {
            pthread_mutex_lock(&g_log_state.stats_mutex);
            g_log_state.stats.dropped_logs++;
            pthread_mutex_unlock(&g_log_state.stats_mutex);
        }
    } else {
        output_log(&entry);
    }
    
    /* 致命错误处理 */
    if (level == DDS_LOG_LEVEL_FATAL) {
        dds_log_flush();
        if (g_log_state.fatal_handler) {
            g_log_state.fatal_handler(&entry);
        }
        abort();
    }
}

void dds_log_write(dds_log_level_t level, dds_log_type_t type,
                   const char* module, const char* tag,
                   const char* file, int line,
                   const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    dds_log_vwrite(level, type, module, tag, file, line, fmt, args);
    va_end(args);
}

/*==============================================================================
 * 审计日志实现
 *============================================================================*/

void dds_log_audit(dds_audit_event_type_t event_type,
                   const char* file, int line,
                   const char* fmt, ...) {
    if (!g_log_state.initialized || !g_log_state.config.enable_audit) {
        return;
    }
    
    /* 检查审计事件类型 */
    if (!(g_log_state.audit_mask & event_type)) {
        return;
    }
    
    /* 构建消息 */
    va_list args;
    char message[DDS_LOG_MAX_MESSAGE_LEN];
    va_start(args, fmt);
    vsnprintf(message, sizeof(message) - 1, fmt, args);
    va_end(args);
    
    /* 发送日志 */
    dds_log_write(DDS_LOG_LEVEL_INFO, DDS_LOG_TYPE_AUDIT, "AUDIT", "", file, line,
                  "[EVENT:0x%04X] %s", event_type, message);
    
    /* 触发审计回调 */
    dds_audit_event_t event;
    event.timestamp_ns = get_timestamp_ns();
    event.event_type = event_type;
    strncpy(event.ecu_id, g_log_state.config.ecu_id, DDS_LOG_ECUID_LEN - 1);
    pthread_mutex_lock(&g_log_state.uds_mutex);
    strncpy(event.uds_session, g_log_state.uds_session, DDS_LOG_UDS_SESSION_ID_LEN - 1);
    pthread_mutex_unlock(&g_log_state.uds_mutex);
    strncpy(event.description, message, DDS_LOG_MAX_MESSAGE_LEN - 1);
    event.severity = (event_type & DDS_AUDIT_EVENT_SECURITY_ALERT) ? 2 : 1;
    
    callback_node_t* node = g_log_state.audit_callbacks;
    while (node) {
        dds_audit_callback_t cb = (dds_audit_callback_t)node->callback;
        if (cb) cb(&event, node->user_data);
        node = node->next;
    }
}

int dds_log_register_audit_callback(dds_audit_callback_t callback, void* user_data) {
    if (!callback) return -1;
    
    callback_node_t* node = malloc(sizeof(callback_node_t));
    if (!node) return -1;
    
    node->callback = callback;
    node->user_data = user_data;
    node->next = g_log_state.audit_callbacks;
    g_log_state.audit_callbacks = node;
    
    return 0;
}

void dds_log_unregister_audit_callback(dds_audit_callback_t callback) {
    callback_node_t** current = &g_log_state.audit_callbacks;
    while (*current) {
        if ((*current)->callback == callback) {
            callback_node_t* to_delete = *current;
            *current = (*current)->next;
            free(to_delete);
            return;
        }
        current = &(*current)->next;
    }
}

/*==============================================================================
 * 配置接口实现
 *============================================================================*/

void dds_log_set_level(dds_log_level_t level) {
    if (level >= DDS_LOG_LEVEL_MAX) return;
    g_log_state.config.level = level;
}

dds_log_level_t dds_log_get_level(void) {
    return g_log_state.config.level;
}

void dds_log_set_module_level(const char* module, dds_log_level_t level) {
    if (!module || level >= DDS_LOG_LEVEL_MAX) return;
    
    pthread_rwlock_wrlock(&g_log_state.rwlock);
    
    /* 查找现有 */
    module_level_node_t* node = g_log_state.module_levels;
    while (node) {
        if (strcmp(node->module, module) == 0) {
            node->level = level;
            pthread_rwlock_unlock(&g_log_state.rwlock);
            return;
        }
        node = node->next;
    }
    
    /* 创建新节点 */
    node = malloc(sizeof(module_level_node_t));
    if (node) {
        strncpy(node->module, module, DDS_LOG_MAX_MODULE_LEN - 1);
        node->level = level;
        node->next = g_log_state.module_levels;
        g_log_state.module_levels = node;
    }
    
    pthread_rwlock_unlock(&g_log_state.rwlock);
}

dds_log_level_t dds_log_get_module_level(const char* module) {
    if (!module) return g_log_state.config.level;
    
    pthread_rwlock_rdlock(&g_log_state.rwlock);
    module_level_node_t* node = g_log_state.module_levels;
    while (node) {
        if (strcmp(node->module, module) == 0) {
            dds_log_level_t level = node->level;
            pthread_rwlock_unlock(&g_log_state.rwlock);
            return level;
        }
        node = node->next;
    }
    pthread_rwlock_unlock(&g_log_state.rwlock);
    
    return g_log_state.config.level;
}

void dds_log_enable_console(bool enable) {
    g_log_state.config.enable_console = enable;
}

void dds_log_enable_file(bool enable) {
    g_log_state.config.enable_file = enable;
}

/*==============================================================================
 * OTA日志上报实现
 *============================================================================*/

void dds_log_set_ota_callback(dds_ota_upload_callback_t callback, void* user_data) {
    g_log_state.ota_callback = callback;
    g_log_state.ota_user_data = user_data;
}

int dds_log_trigger_ota_upload(const char* session_id, bool compress) {
    if (!g_log_state.initialized) return -1;
    
    /* 构建OTA日志条目 */
    dds_ota_log_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.timestamp_ns = get_timestamp_ns();
    strncpy(entry.ecu_id, g_log_state.config.ecu_id, DDS_LOG_ECUID_LEN - 1);
    if (session_id) {
        strncpy(entry.uds_session, session_id, DDS_LOG_UDS_SESSION_ID_LEN - 1);
    }
    
    /* 读取日志文件 */
    pthread_mutex_lock(&g_log_state.mutex);
    if (g_log_state.log_fp) {
        fflush(g_log_state.log_fp);
    }
    pthread_mutex_unlock(&g_log_state.mutex);
    
    /* 统计文件 */
    /* 简化版本：实际应用需要读取和压缩日志文件 */
    entry.log_count = (uint32_t)g_log_state.stats.total_logs;
    entry.log_size = (uint32_t)g_log_state.stats.bytes_written;
    
    /* 触发回调 */
    if (g_log_state.ota_callback) {
        g_log_state.ota_callback(&entry, true, g_log_state.ota_user_data);
    }
    
    DDS_LOG_INFO("DDS_LOG", "OTA", "OTA log upload triggered, session=%s, compress=%d",
                 session_id ? session_id : "none", compress);
    
    return 0;
}

int dds_log_get_ota_buffer(uint8_t* buffer, uint32_t buffer_size, uint32_t* actual_size) {
    if (!buffer || !actual_size || buffer_size == 0) return -1;
    
    /* 简化版本：实际应用需要读取压缩日志数据 */
    *actual_size = 0;
    return 0;
}

/*==============================================================================
 * UDS接口实现
 *============================================================================*/

void dds_log_set_uds_session(const char* session_id) {
    pthread_mutex_lock(&g_log_state.uds_mutex);
    if (session_id) {
        strncpy(g_log_state.uds_session, session_id, DDS_LOG_UDS_SESSION_ID_LEN - 1);
        DDS_LOG_INFO("DDS_LOG", "UDS", "UDS session started: %s", session_id);
    } else {
        memset(g_log_state.uds_session, 0, sizeof(g_log_state.uds_session));
    }
    pthread_mutex_unlock(&g_log_state.uds_mutex);
}

void dds_log_clear_uds_session(void) {
    pthread_mutex_lock(&g_log_state.uds_mutex);
    if (strlen(g_log_state.uds_session) > 0) {
        DDS_LOG_INFO("DDS_LOG", "UDS", "UDS session ended: %s", g_log_state.uds_session);
    }
    memset(g_log_state.uds_session, 0, sizeof(g_log_state.uds_session));
    pthread_mutex_unlock(&g_log_state.uds_mutex);
}

void dds_log_uds_event(uint8_t service_id, uint8_t response_code, const char* description) {
    if (!g_log_state.config.enable_uds_trace) return;
    
    DDS_LOG_DIAG(DDS_LOG_LEVEL_DEBUG, "UDS_SVC",
                 "SID=0x%02X, RSP=0x%02X, %s",
                 service_id, response_code, description ? description : "");
}

/*==============================================================================
 * 统计和状态接口实现
 *============================================================================*/

void dds_log_get_stats(dds_log_stats_t* stats) {
    if (!stats) return;
    pthread_mutex_lock(&g_log_state.stats_mutex);
    memcpy(stats, &g_log_state.stats, sizeof(dds_log_stats_t));
    stats->current_queue_size = g_log_state.queue.size;
    stats->current_file_size = g_log_state.current_file_size;
    pthread_mutex_unlock(&g_log_state.stats_mutex);
}

int dds_log_get_current_file(char* path, size_t path_size) {
    if (!path || path_size == 0) return -1;
    
    pthread_mutex_lock(&g_log_state.mutex);
    if (strlen(g_log_state.current_file) == 0) {
        pthread_mutex_unlock(&g_log_state.mutex);
        return -1;
    }
    strncpy(path, g_log_state.current_file, path_size - 1);
    pthread_mutex_unlock(&g_log_state.mutex);
    
    return 0;
}

int dds_log_get_file_list(char** files, uint32_t max_files, uint32_t* actual_count) {
    if (!files || !actual_count || max_files == 0) return -1;
    
    *actual_count = 0;
    
    DIR* dir = opendir(g_log_state.config.log_dir);
    if (!dir) return -1;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL && *actual_count < max_files) {
        if (strstr(entry->d_name, ".log")) {
            files[*actual_count] = malloc(DDS_LOG_MAX_PATH_LEN);
            if (files[*actual_count]) {
                snprintf(files[*actual_count], DDS_LOG_MAX_PATH_LEN, "%s/%s",
                         g_log_state.config.log_dir, entry->d_name);
                (*actual_count)++;
            }
        }
    }
    
    closedir(dir);
    return 0;
}

/*==============================================================================
 * 高级功能实现
 *============================================================================*/

int dds_log_register_output_callback(dds_log_output_callback_t callback, void* user_data) {
    if (!callback) return -1;
    
    callback_node_t* node = malloc(sizeof(callback_node_t));
    if (!node) return -1;
    
    node->callback = callback;
    node->user_data = user_data;
    node->next = g_log_state.output_callbacks;
    g_log_state.output_callbacks = node;
    
    return 0;
}

void dds_log_unregister_output_callback(dds_log_output_callback_t callback) {
    callback_node_t** current = &g_log_state.output_callbacks;
    while (*current) {
        if ((*current)->callback == callback) {
            callback_node_t* to_delete = *current;
            *current = (*current)->next;
            free(to_delete);
            return;
        }
        current = &(*current)->next;
    }
}

void dds_log_set_fatal_handler(dds_log_fatal_handler_t handler) {
    g_log_state.fatal_handler = handler;
}

int dds_log_export_to_pcap(const char* log_file, const char* pcap_file) {
    /* 简化版本：实际应用需要解析日志并生成pcap */
    (void)log_file;
    (void)pcap_file;
    return 0;
}
