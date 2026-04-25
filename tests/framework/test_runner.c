/**
 * @file test_runner.c
 * @brief 通用测试运行器
 * @version 1.0
 * @date 2026-04-25
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "../unity/unity.h"
#include "../unity/unity_config.h"

/*==============================================================================
 * 测试框架配置
 *==============================================================================*/

/* 最大测试数量 */
#define MAX_TESTS           1000
#define MAX_SUITES          50
#define MAX_TEST_NAME_LEN   128
#define MAX_SUITE_NAME_LEN  64

/* 测试类型 */
typedef enum {
    TEST_TYPE_UNIT = 0,
    TEST_TYPE_INTEGRATION,
    TEST_TYPE_SYSTEM,
    TEST_TYPE_PERFORMANCE,
    TEST_TYPE_HIL,
} test_type_t;

/* 测试状态 */
typedef enum {
    TEST_STATUS_NOT_RUN = 0,
    TEST_STATUS_PASSED,
    TEST_STATUS_FAILED,
    TEST_STATUS_SKIPPED,
    TEST_STATUS_TIMEOUT,
    TEST_STATUS_CRASHED,
} test_status_t;

/* 单个测试定义 */
typedef struct {
    char name[MAX_TEST_NAME_LEN];
    void (*test_func)(void);
    void (*setup_func)(void);
    void (*teardown_func)(void);
    test_type_t type;
    uint32_t timeout_ms;
    bool enabled;
    test_status_t status;
    char error_msg[256];
    double execution_time_ms;
} test_case_t;

/* 测试套件定义 */
typedef struct {
    char name[MAX_SUITE_NAME_LEN];
    test_case_t* tests;
    uint32_t test_count;
    uint32_t test_capacity;
    void (*suite_setup)(void);
    void (*suite_teardown)(void);
    bool enabled;
} test_suite_t;

/* 全局状态 */
static struct {
    test_suite_t suites[MAX_SUITES];
    uint32_t suite_count;
    test_type_t filter_type;
    char filter_name[MAX_TEST_NAME_LEN];
    bool verbose;
    bool color_output;
    bool stop_on_fail;
    uint32_t timeout_default_ms;
    char output_file[256];
    FILE* output_fp;
} runner_state = {
    .filter_type = -1,
    .filter_name = {0},
    .verbose = false,
    .color_output = true,
    .stop_on_fail = false,
    .timeout_default_ms = 5000,
    .output_file = {0},
    .output_fp = NULL
};

/* 当前测试信息 */
static test_case_t* current_test = NULL;
static volatile int test_timed_out = 0;

/*==============================================================================
 * 颜色输出定义
 *==============================================================================*/

#define COLOR_RESET     "\033[0m"
#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_BLUE      "\033[34m"
#define COLOR_MAGENTA   "\033[35m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_WHITE     "\033[37m"
#define COLOR_BOLD      "\033[1m"

/*==============================================================================
 * 内部辅助函数
 *==============================================================================*/

static const char* test_type_to_string(test_type_t type) {
    switch (type) {
        case TEST_TYPE_UNIT:        return "UNIT";
        case TEST_TYPE_INTEGRATION: return "INTG";
        case TEST_TYPE_SYSTEM:      return "SYS";
        case TEST_TYPE_PERFORMANCE: return "PERF";
        case TEST_TYPE_HIL:         return "HIL";
        default:                    return "UNKNOWN";
    }
}

static const char* test_status_to_string(test_status_t status) {
    switch (status) {
        case TEST_STATUS_NOT_RUN:   return "NOT_RUN";
        case TEST_STATUS_PASSED:    return "PASSED";
        case TEST_STATUS_FAILED:    return "FAILED";
        case TEST_STATUS_SKIPPED:   return "SKIPPED";
        case TEST_STATUS_TIMEOUT:   return "TIMEOUT";
        case TEST_STATUS_CRASHED:   return "CRASHED";
        default:                    return "UNKNOWN";
    }
}

static void print_colored(const char* color, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    if (runner_state.color_output) {
        printf("%s", color);
    }
    vprintf(fmt, args);
    if (runner_state.color_output) {
        printf("%s", COLOR_RESET);
    }
    
    va_end(args);
}

static void log_message(const char* level, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    printf("[%s] [%s] ", time_str, level);
    vprintf(fmt, args);
    printf("\n");
    
    if (runner_state.output_fp != NULL) {
        fprintf(runner_state.output_fp, "[%s] [%s] ", time_str, level);
        vfprintf(runner_state.output_fp, fmt, args);
        fprintf(runner_state.output_fp, "\n");
    }
    
    va_end(args);
}

static void timeout_handler(int sig) {
    (void)sig;
    test_timed_out = 1;
    if (current_test != NULL) {
        current_test->status = TEST_STATUS_TIMEOUT;
        strncpy(current_test->error_msg, "Test timed out", sizeof(current_test->error_msg) - 1);
    }
}

static void install_timeout_handler(uint32_t timeout_ms) {
    signal(SIGALRM, timeout_handler);
    ualarm(timeout_ms * 1000, 0);
}

static void uninstall_timeout_handler(void) {
    ualarm(0, 0);
    signal(SIGALRM, SIG_DFL);
}

/*==============================================================================
 * 测试套件API
 *==============================================================================*/

test_suite_t* test_runner_create_suite(const char* name, 
                                        void (*setup)(void),
                                        void (*teardown)(void)) {
    if (runner_state.suite_count >= MAX_SUITES) {
        log_message("ERROR", "Maximum number of test suites reached");
        return NULL;
    }
    
    test_suite_t* suite = &runner_state.suites[runner_state.suite_count++];
    strncpy(suite->name, name, MAX_SUITE_NAME_LEN - 1);
    suite->name[MAX_SUITE_NAME_LEN - 1] = '\0';
    suite->suite_setup = setup;
    suite->suite_teardown = teardown;
    suite->enabled = true;
    suite->test_count = 0;
    suite->test_capacity = 0;
    suite->tests = NULL;
    
    return suite;
}

int test_runner_add_test(test_suite_t* suite,
                          const char* name,
                          void (*test_func)(void),
                          void (*setup_func)(void),
                          void (*teardown_func)(void)) {
    if (suite == NULL || test_func == NULL) {
        return -1;
    }
    
    if (suite->test_count >= suite->test_capacity) {
        uint32_t new_capacity = suite->test_capacity == 0 ? 10 : suite->test_capacity * 2;
        test_case_t* new_tests = realloc(suite->tests, new_capacity * sizeof(test_case_t));
        if (new_tests == NULL) {
            return -1;
        }
        suite->tests = new_tests;
        suite->test_capacity = new_capacity;
    }
    
    test_case_t* test = &suite->tests[suite->test_count++];
    strncpy(test->name, name, MAX_TEST_NAME_LEN - 1);
    test->name[MAX_TEST_NAME_LEN - 1] = '\0';
    test->test_func = test_func;
    test->setup_func = setup_func;
    test->teardown_func = teardown_func;
    test->type = TEST_TYPE_UNIT;
    test->timeout_ms = runner_state.timeout_default_ms;
    test->enabled = true;
    test->status = TEST_STATUS_NOT_RUN;
    test->error_msg[0] = '\0';
    test->execution_time_ms = 0;
    
    return 0;
}

/*==============================================================================
 * 测试执行API
 *==============================================================================*/

static test_status_t run_single_test(test_case_t* test) {
    if (!test->enabled) {
        return TEST_STATUS_SKIPPED;
    }
    
    /* 应用过滤器 */
    if (runner_state.filter_type != (test_type_t)-1 && 
        test->type != runner_state.filter_type) {
        return TEST_STATUS_SKIPPED;
    }
    if (runner_state.filter_name[0] != '\0' &&
        strstr(test->name, runner_state.filter_name) == NULL) {
        return TEST_STATUS_SKIPPED;
    }
    
    current_test = test;
    test_timed_out = 0;
    
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    /* 设置超时 */
    install_timeout_handler(test->timeout_ms);
    
    /* 运行测试 */
    test->status = TEST_STATUS_NOT_RUN;
    
    /* 设置Unity回调 */
    Unity.TestFile = test->name;
    Unity.CurrentDetail1 = NULL;
    Unity.CurrentDetail2 = NULL;
    
    if (test->setup_func != NULL) {
        test->setup_func();
    }
    
    UnityBegin(test->name);
    test->test_func();
    int unity_result = UnityEnd();
    
    if (test->teardown_func != NULL) {
        test->teardown_func();
    }
    
    /* 取消超时 */
    uninstall_timeout_handler();
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    test->execution_time_ms = (end_time.tv_sec - start_time.tv_sec) * 1000.0 +
                              (end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;
    
    if (test_timed_out) {
        test->status = TEST_STATUS_TIMEOUT;
    } else if (unity_result == 0) {
        test->status = TEST_STATUS_PASSED;
    } else {
        test->status = TEST_STATUS_FAILED;
        if (Unity.CurrentTestFailed) {
            strncpy(test->error_msg, Unity.CurrentTestErrorMessage, 
                    sizeof(test->error_msg) - 1);
        }
    }
    
    current_test = NULL;
    return test->status;
}

static void print_test_result(test_case_t* test) {
    const char* type_str = test_type_to_string(test->type);
    
    switch (test->status) {
        case TEST_STATUS_PASSED:
            print_colored(COLOR_GREEN, "[PASS]");
            break;
        case TEST_STATUS_FAILED:
            print_colored(COLOR_RED, "[FAIL]");
            break;
        case TEST_STATUS_SKIPPED:
            print_colored(COLOR_YELLOW, "[SKIP]");
            break;
        case TEST_STATUS_TIMEOUT:
            print_colored(COLOR_MAGENTA, "[TMO]");
            break;
        case TEST_STATUS_CRASHED:
            print_colored(COLOR_RED, "[CRASH]");
            break;
        default:
            print_colored(COLOR_WHITE, "[?]");
            break;
    }
    
    printf(" [%4s] %-50s (%6.2f ms)\n", 
           type_str, test->name, test->execution_time_ms);
    
    if (runner_state.verbose && test->status == TEST_STATUS_FAILED) {
        print_colored(COLOR_RED, "       Error: %s\n", test->error_msg);
    }
}

static int run_suite(test_suite_t* suite) {
    if (!suite->enabled) {
        return 0;
    }
    
    printf("\n");
    print_colored(COLOR_BOLD COLOR_CYAN, "===== Suite: %s =====\n", suite->name);
    
    if (suite->suite_setup != NULL) {
        suite->suite_setup();
    }
    
    int passed = 0, failed = 0, skipped = 0;
    
    for (uint32_t i = 0; i < suite->test_count; i++) {
        test_status_t status = run_single_test(&suite->tests[i]);
        print_test_result(&suite->tests[i]);
        
        switch (status) {
            case TEST_STATUS_PASSED:  passed++; break;
            case TEST_STATUS_FAILED:  failed++; break;
            case TEST_STATUS_SKIPPED: skipped++; break;
            default: break;
        }
        
        if (runner_state.stop_on_fail && status == TEST_STATUS_FAILED) {
            break;
        }
    }
    
    if (suite->suite_teardown != NULL) {
        suite->suite_teardown();
    }
    
    print_colored(COLOR_BOLD, "\nSuite Summary: ");
    print_colored(COLOR_GREEN, "%d passed, ", passed);
    print_colored(COLOR_RED, "%d failed, ", failed);
    print_colored(COLOR_YELLOW, "%d skipped\n", skipped);
    
    return failed;
}

int test_runner_run_all(void) {
    int total_failed = 0;
    
    printf("\n");
    print_colored(COLOR_BOLD COLOR_BLUE, 
                  "================================================\n");
    print_colored(COLOR_BOLD COLOR_BLUE, 
                  "        ETH-DDS Integration Test Runner         \n");
    print_colored(COLOR_BOLD COLOR_BLUE, 
                  "================================================\n");
    
    /* 打开输出文件 */
    if (runner_state.output_file[0] != '\0') {
        runner_state.output_fp = fopen(runner_state.output_file, "w");
        if (runner_state.output_fp == NULL) {
            log_message("WARN", "Failed to open output file: %s", runner_state.output_file);
        }
    }
    
    for (uint32_t i = 0; i < runner_state.suite_count; i++) {
        total_failed += run_suite(&runner_state.suites[i]);
        
        if (runner_state.stop_on_fail && total_failed > 0) {
            break;
        }
    }
    
    /* 总结 */
    printf("\n");
    print_colored(COLOR_BOLD COLOR_BLUE, 
                  "================================================\n");
    if (total_failed == 0) {
        print_colored(COLOR_BOLD COLOR_GREEN, 
                      "           ALL TESTS PASSED!                    \n");
    } else {
        print_colored(COLOR_BOLD COLOR_RED, 
                      "           %d TEST(S) FAILED!                   \n", total_failed);
    }
    print_colored(COLOR_BOLD COLOR_BLUE, 
                  "================================================\n");
    
    /* 关闭输出文件 */
    if (runner_state.output_fp != NULL) {
        fclose(runner_state.output_fp);
        runner_state.output_fp = NULL;
    }
    
    return total_failed;
}

/*==============================================================================
 * 配置API
 *==============================================================================*/

void test_runner_set_filter_type(test_type_t type) {
    runner_state.filter_type = type;
}

void test_runner_set_filter_name(const char* name) {
    if (name != NULL) {
        strncpy(runner_state.filter_name, name, MAX_TEST_NAME_LEN - 1);
        runner_state.filter_name[MAX_TEST_NAME_LEN - 1] = '\0';
    }
}

void test_runner_set_verbose(bool verbose) {
    runner_state.verbose = verbose;
}

void test_runner_set_color(bool color) {
    runner_state.color_output = color;
}

void test_runner_set_stop_on_fail(bool stop) {
    runner_state.stop_on_fail = stop;
}

void test_runner_set_timeout(uint32_t timeout_ms) {
    runner_state.timeout_default_ms = timeout_ms;
}

void test_runner_set_output_file(const char* filename) {
    if (filename != NULL) {
        strncpy(runner_state.output_file, filename, sizeof(runner_state.output_file) - 1);
        runner_state.output_file[sizeof(runner_state.output_file) - 1] = '\0';
    }
}

/*==============================================================================
 * 命令行解析
 *==============================================================================*/

void test_runner_parse_args(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            test_runner_set_verbose(true);
        } else if (strcmp(argv[i], "--no-color") == 0) {
            test_runner_set_color(false);
        } else if (strcmp(argv[i], "--stop-on-fail") == 0) {
            test_runner_set_stop_on_fail(true);
        } else if (strcmp(argv[i], "--timeout") == 0 && i + 1 < argc) {
            test_runner_set_timeout(atoi(argv[++i]));
        } else if (strcmp(argv[i], "--filter") == 0 && i + 1 < argc) {
            test_runner_set_filter_name(argv[++i]);
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            test_runner_set_output_file(argv[++i]);
        } else if (strcmp(argv[i], "--type") == 0 && i + 1 < argc) {
            const char* type_str = argv[++i];
            if (strcmp(type_str, "unit") == 0) {
                test_runner_set_filter_type(TEST_TYPE_UNIT);
            } else if (strcmp(type_str, "integration") == 0) {
                test_runner_set_filter_type(TEST_TYPE_INTEGRATION);
            } else if (strcmp(type_str, "system") == 0) {
                test_runner_set_filter_type(TEST_TYPE_SYSTEM);
            } else if (strcmp(type_str, "performance") == 0) {
                test_runner_set_filter_type(TEST_TYPE_PERFORMANCE);
            } else if (strcmp(type_str, "hil") == 0) {
                test_runner_set_filter_type(TEST_TYPE_HIL);
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  -v, --verbose       Enable verbose output\n");
            printf("  --no-color          Disable colored output\n");
            printf("  --stop-on-fail      Stop on first failure\n");
            printf("  --timeout <ms>      Set default timeout\n");
            printf("  --filter <name>     Filter tests by name\n");
            printf("  --type <type>       Filter by type (unit/integration/system/performance/hil)\n");
            printf("  --output <file>     Write output to file\n");
            printf("  -h, --help          Show this help\n");
            exit(0);
        }
    }
}
