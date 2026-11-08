/******************************************************************************
 * @file test_adaptive_exec.c
 * @brief AUTOSAR Adaptive exec 平台服务单元测试 (进程/资源/沙箱)
 *
 * 仅覆盖纯逻辑分支, 平台调用均为生产代码内置 stub。
 ******************************************************************************/

#include "ara/exec/process_manager.h"
#include "ara/exec/resource_manager.h"
#include "ara/exec/sandbox.h"

#include <stdio.h>
#include <string.h>

static int g_failures = 0;

#define CHECK(cond)                                                         \
    do {                                                                    \
        if (!(cond)) {                                                      \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++g_failures;                                                   \
        }                                                                   \
    } while (0)

/*=========================== 回调桩 ===========================*/

static unsigned g_state_calls = 0U;
static unsigned g_health_calls = 0U;
static uint8_t g_health_value = 0U;

static void process_state_cb(ProcessIdType pid, ProcessStateType state)
{
    (void)pid;
    (void)state;
    ++g_state_calls;
}

static void process_health_cb(ProcessIdType pid, uint8_t* status)
{
    (void)pid;
    ++g_health_calls;
    if (status != NULL) {
        *status = g_health_value;
    }
}

/*=========================== ProcessManager ===========================*/

static void test_process_not_initialized(void)
{
    ProcessConfigType cfg;
    ProcessIdType pid = 0U;
    ProcessInfoType info;
    ProcessIdType pids[4];

    CHECK(ProcessManager_Deinit() == E_OK);
    CHECK(ProcessManager_IsInitialized() == false);

    CHECK(ProcessManager_CreateConfig(NULL) == E_NOT_OK);
    CHECK(ProcessManager_CreateConfig(&cfg) == E_OK);
    CHECK(cfg.startTimeoutMs == 5000U);
    CHECK(cfg.stackSize == EXEC_DEFAULT_STACK_SIZE);
    CHECK(cfg.priority == EXEC_DEFAULT_PRIORITY);

    CHECK(ProcessManager_Start(&cfg, &pid) == E_NOT_OK);
    CHECK(ProcessManager_Stop(1U, TERMINATION_GRACEFUL) == E_NOT_OK);
    CHECK(ProcessManager_Restart(1U) == E_NOT_OK);
    CHECK(ProcessManager_Kill(1U) == E_NOT_OK);
    CHECK(ProcessManager_GetState(1U) == PROCESS_STATE_ERROR);
    CHECK(ProcessManager_GetInfo(1U, &info) == E_NOT_OK);
    CHECK(ProcessManager_IsRunning(1U) == false);
    CHECK(ProcessManager_GetAllProcesses(pids, 4U) == 0U);
    CHECK(ProcessManager_RegisterHealthCheck(1U, process_health_cb) == E_NOT_OK);
    CHECK(ProcessManager_UnregisterHealthCheck(1U) == E_NOT_OK);
    CHECK(ProcessManager_RegisterStateCallback(1U, process_state_cb) == E_NOT_OK);
    ProcessManager_MainFunction();
    CHECK(ProcessManager_GetNextPid() >= 1U);
}

static void test_process_basic(void)
{
    ProcessConfigType cfg;
    ProcessIdType pid = 0U;
    ProcessIdType pid2 = 0U;
    ProcessIdType pids[4];
    ProcessInfoType info;
    ProcessIdType base;

    CHECK(ProcessManager_Init() == E_OK);
    CHECK(ProcessManager_IsInitialized() == true);
    CHECK(ProcessManager_Init() == E_OK);

    CHECK(ProcessManager_CreateConfig(&cfg) == E_OK);
    CHECK(ProcessManager_Start(NULL, &pid) == E_NOT_OK);
    CHECK(ProcessManager_Start(&cfg, NULL) == E_NOT_OK);

    /* 校验失败分支 */
    CHECK(ProcessManager_Start(&cfg, &pid) == E_NOT_OK); /* executable 为空 */
    (void)strncpy(cfg.executable, "/bin/sh", sizeof(cfg.executable) - 1U);
    cfg.argumentCount = EXEC_MAX_ARGUMENTS + 1U;
    CHECK(ProcessManager_Start(&cfg, &pid) == E_NOT_OK);
    cfg.argumentCount = 0U;
    cfg.environmentCount = EXEC_MAX_ENV_VARS + 1U;
    CHECK(ProcessManager_Start(&cfg, &pid) == E_NOT_OK);
    cfg.environmentCount = 0U;

    base = ProcessManager_GetNextPid();
    CHECK(ProcessManager_Start(&cfg, &pid) == E_OK);
    CHECK(pid == base);
    CHECK(ProcessManager_GetState(pid) == PROCESS_STATE_RUNNING);
    CHECK(ProcessManager_IsRunning(pid) == true);
    CHECK(ProcessManager_GetInfo(pid, &info) == E_OK);
    CHECK(info.pid == pid);
    CHECK(info.state == PROCESS_STATE_RUNNING);

    CHECK(ProcessManager_GetAllProcesses(pids, 4U) == 1U);
    CHECK(pids[0] == pid);
    CHECK(ProcessManager_GetAllProcesses(NULL, 4U) == 0U);
    CHECK(ProcessManager_GetAllProcesses(pids, 0U) == 0U);

    CHECK(ProcessManager_RegisterHealthCheck(pid, process_health_cb) == E_OK);
    CHECK(ProcessManager_RegisterStateCallback(pid, process_state_cb) == E_OK);
    CHECK(ProcessManager_RegisterHealthCheck(999U, process_health_cb) == E_NOT_OK);
    CHECK(ProcessManager_RegisterStateCallback(999U, process_state_cb) == E_NOT_OK);

    /* 健康检查正常 */
    g_health_value = 0U;
    ProcessManager_MainFunction();
    CHECK(g_health_calls == 1U);
    CHECK(ProcessManager_GetState(pid) == PROCESS_STATE_RUNNING);

    /* 健康检查失败但 autoRestart=false -> 不重启 */
    g_health_value = 1U;
    ProcessManager_MainFunction();
    CHECK(ProcessManager_GetState(pid) == PROCESS_STATE_RUNNING);

    /* 注销回调并停止 */
    CHECK(ProcessManager_UnregisterHealthCheck(pid) == E_OK);
    CHECK(ProcessManager_UnregisterHealthCheck(999U) == E_NOT_OK);
    CHECK(ProcessManager_Stop(pid, TERMINATION_GRACEFUL) == E_OK);
    CHECK(g_state_calls >= 1U);
    CHECK(ProcessManager_GetState(pid) == PROCESS_STATE_ERROR);
    CHECK(ProcessManager_IsRunning(pid) == false);
    CHECK(ProcessManager_Stop(pid, TERMINATION_GRACEFUL) == E_NOT_OK);

    /* Kill */
    CHECK(ProcessManager_Start(&cfg, &pid2) == E_OK);
    CHECK(ProcessManager_Kill(pid2) == E_OK);
    CHECK(ProcessManager_Kill(pid2) == E_NOT_OK);
    CHECK(ProcessManager_GetState(pid2) == PROCESS_STATE_ERROR);

    /* Restart: 生成新 pid, 旧 pid 失效 */
    CHECK(ProcessManager_Start(&cfg, &pid2) == E_OK);
    base = pid2;
    CHECK(ProcessManager_Restart(base) == E_OK);
    CHECK(ProcessManager_GetState(base) == PROCESS_STATE_ERROR);
    CHECK(ProcessManager_GetAllProcesses(pids, 4U) == 1U);
    CHECK(pids[0] != base);
    CHECK(ProcessManager_Restart(4242U) == E_NOT_OK);

    CHECK(ProcessManager_Deinit() == E_OK);
    CHECK(ProcessManager_IsInitialized() == false);
    CHECK(ProcessManager_Deinit() == E_OK);
}

static void test_process_restart_on_health_failure(void)
{
    ProcessConfigType cfg;
    ProcessIdType pid = 0U;
    ProcessIdType pids[4];
    ProcessInfoType info;
    ProcessIdType restarted;

    CHECK(ProcessManager_Init() == E_OK);
    CHECK(ProcessManager_CreateConfig(&cfg) == E_OK);
    (void)strncpy(cfg.executable, "/bin/true", sizeof(cfg.executable) - 1U);
    cfg.autoRestart = true;

    CHECK(ProcessManager_Start(&cfg, &pid) == E_OK);
    CHECK(ProcessManager_RegisterHealthCheck(pid, process_health_cb) == E_OK);

    g_health_value = 1U;
    ProcessManager_MainFunction(); /* 触发 autoRestart */

    CHECK(ProcessManager_GetAllProcesses(pids, 4U) == 1U);
    restarted = pids[0];
    CHECK(restarted != pid);
    CHECK(ProcessManager_GetInfo(restarted, &info) == E_OK);
    CHECK(info.restartCount == 1U);
    CHECK(ProcessManager_GetState(restarted) == PROCESS_STATE_RUNNING);

    /* 新进程无健康检查回调, 再次 MainFunction 安全 */
    ProcessManager_MainFunction();
    CHECK(ProcessManager_GetState(restarted) == PROCESS_STATE_RUNNING);

    ProcessManager_Deinit();
}

static void test_process_exhaustion(void)
{
    ProcessConfigType cfg;
    ProcessIdType pid = 0U;
    ProcessIdType pids[EXEC_MAX_PROCESSES];

    CHECK(ProcessManager_Init() == E_OK);
    CHECK(ProcessManager_CreateConfig(&cfg) == E_OK);
    (void)strncpy(cfg.executable, "/bin/true", sizeof(cfg.executable) - 1U);

    for (uint32_t i = 0U; i < EXEC_MAX_PROCESSES; ++i) {
        CHECK(ProcessManager_Start(&cfg, &pid) == E_OK);
    }
    CHECK(ProcessManager_Start(&cfg, &pid) == E_NOT_OK); /* 无空闲槽 */
    CHECK(ProcessManager_GetAllProcesses(pids, EXEC_MAX_PROCESSES) == EXEC_MAX_PROCESSES);
    CHECK(ProcessManager_GetAllProcesses(pids, 5U) == 5U);

    ProcessManager_Deinit();
}

/*=========================== ResourceManager ===========================*/

static void test_resource_not_initialized(void)
{
    ResourceLimitType limit;
    ResourceUsageType usage;

    CHECK(ResourceManager_Deinit() == E_OK);
    CHECK(ResourceManager_IsInitialized() == false);

    (void)memset(&limit, 0, sizeof(limit));
    CHECK(ResourceManager_SetLimit(1U, &limit) == E_NOT_OK);
    CHECK(ResourceManager_GetLimit(1U, RESOURCE_TYPE_CPU, &limit) == E_NOT_OK);
    CHECK(ResourceManager_GetUsage(1U, &usage) == E_NOT_OK);
    CHECK(ResourceManager_GetPeakUsage(1U, &usage) == E_NOT_OK);
    CHECK(ResourceManager_ResetPeakUsage(1U) == E_NOT_OK);
    CHECK(ResourceManager_EnableMonitoring(1U) == E_NOT_OK);
    CHECK(ResourceManager_DisableMonitoring(1U) == E_NOT_OK);
    CHECK(ResourceManager_IsWithinLimits(1U) == false);
    CHECK(ResourceManager_GetSystemUsage(&usage) == E_NOT_OK);
    ResourceManager_MainFunction();
}

static void test_resource_basic(void)
{
    ResourceLimitType limit;
    ResourceLimitType got;
    ResourceUsageType usage;

    CHECK(ResourceManager_Init() == E_OK);
    CHECK(ResourceManager_IsInitialized() == true);
    CHECK(ResourceManager_Init() == E_OK);

    CHECK(ResourceManager_SetLimit(1U, NULL) == E_NOT_OK);
    CHECK(ResourceManager_EnableMonitoring(1U) == E_OK);
    CHECK(ResourceManager_EnableMonitoring(1U) == E_OK);

    /* 默认限制已填充 */
    CHECK(ResourceManager_GetLimit(1U, RESOURCE_TYPE_CPU, &got) == E_OK);
    CHECK(got.enabled == true);
    CHECK(got.softLimit == RESOURCE_DEFAULT_CPU_LIMIT);

    /* soft > hard -> 拒绝 */
    (void)memset(&limit, 0, sizeof(limit));
    limit.type = RESOURCE_TYPE_CPU;
    limit.softLimit = 200U;
    limit.hardLimit = 100U;
    limit.enabled = true;
    CHECK(ResourceManager_SetLimit(1U, &limit) == E_NOT_OK);

    limit.softLimit = 50U;
    CHECK(ResourceManager_SetLimit(1U, &limit) == E_OK);
    CHECK(ResourceManager_GetLimit(1U, RESOURCE_TYPE_CPU, &got) == E_OK);
    CHECK(got.softLimit == 50U);
    CHECK(got.hardLimit == 100U);

    CHECK(ResourceManager_GetUsage(1U, &usage) == E_OK);
    CHECK(ResourceManager_GetPeakUsage(1U, &usage) == E_OK);
    CHECK(ResourceManager_ResetPeakUsage(1U) == E_OK);
    CHECK(ResourceManager_IsWithinLimits(1U) == true);

    ResourceManager_MainFunction();

    CHECK(ResourceManager_GetSystemUsage(&usage) == E_OK);
    CHECK(ResourceManager_GetSystemUsage(NULL) == E_NOT_OK);
    CHECK(ResourceManager_GetPeakUsage(1U, NULL) == E_NOT_OK);
    CHECK(ResourceManager_GetUsage(1U, NULL) == E_NOT_OK);
    CHECK(ResourceManager_GetLimit(1U, RESOURCE_TYPE_CPU, NULL) == E_NOT_OK);

    /* 未监控 pid: 直接走平台 stub */
    CHECK(ResourceManager_GetLimit(77U, RESOURCE_TYPE_MEMORY, &got) == E_OK);
    CHECK(ResourceManager_GetUsage(77U, &usage) == E_OK);
    CHECK(ResourceManager_GetPeakUsage(77U, &usage) == E_NOT_OK);
    CHECK(ResourceManager_ResetPeakUsage(77U) == E_NOT_OK);
    CHECK(ResourceManager_IsWithinLimits(77U) == true);

    CHECK(ResourceManager_DisableMonitoring(1U) == E_OK);
    CHECK(ResourceManager_DisableMonitoring(1U) == E_NOT_OK);

    CHECK(ResourceManager_Deinit() == E_OK);
    CHECK(ResourceManager_IsInitialized() == false);
    CHECK(ResourceManager_Deinit() == E_OK);
}

static void test_resource_exhaustion(void)
{
    ResourceLimitType limit;

    CHECK(ResourceManager_Init() == E_OK);
    for (uint32_t i = 0U; i < EXEC_MAX_PROCESSES; ++i) {
        CHECK(ResourceManager_EnableMonitoring(i + 1U) == E_OK);
    }
    CHECK(ResourceManager_EnableMonitoring(EXEC_MAX_PROCESSES + 1U) == E_NOT_OK);

    (void)memset(&limit, 0, sizeof(limit));
    limit.type = RESOURCE_TYPE_MEMORY;
    CHECK(ResourceManager_SetLimit(EXEC_MAX_PROCESSES + 1U, &limit) == E_NOT_OK);

    ResourceManager_Deinit();
}

/*=========================== Sandbox ===========================*/

static void test_sandbox_not_initialized(void)
{
    SandboxConfigType cfg;
    SandboxStatusType status;

    CHECK(Sandbox_Deinit() == E_OK);
    CHECK(Sandbox_IsInitialized() == false);

    CHECK(Sandbox_CreateDefaultConfig(NULL) == E_NOT_OK);
    CHECK(Sandbox_Enable(1U, NULL) == E_NOT_OK);
    CHECK(Sandbox_Disable(1U) == E_NOT_OK);
    CHECK(Sandbox_IsActive(1U) == false);
    CHECK(Sandbox_GetStatus(1U, &status) == E_NOT_OK);
    CHECK(Sandbox_ValidateConfig(NULL) == false);
    (void)memset(&cfg, 0, sizeof(cfg));
    (void)strncpy(cfg.rootPath, "/root", sizeof(cfg.rootPath) - 1U);
    CHECK(Sandbox_Enable(1U, &cfg) == E_NOT_OK);
    Sandbox_MainFunction();
}

static void test_sandbox_config(void)
{
    SandboxConfigType cfg;
    SandboxConfigType bad;
    char long_path[300];

    CHECK(Sandbox_Init() == E_OK);
    CHECK(Sandbox_IsInitialized() == true);
    CHECK(Sandbox_Init() == E_OK);

    CHECK(Sandbox_CreateDefaultConfig(&cfg) == E_OK);
    CHECK(strcmp(cfg.rootPath, SANDBOX_DEFAULT_ROOT_PATH) == 0);
    CHECK(cfg.mountCount == 5U);
    CHECK(cfg.isolationLevel == SANDBOX_ISOLATION_FILESYSTEM);
    CHECK(cfg.networkRestricted == true);
    CHECK(Sandbox_ValidateConfig(&cfg) == true);

    /* AddMountPoint 参数校验 */
    CHECK(Sandbox_AddMountPoint(NULL, "/a", "/b", true) == E_NOT_OK);
    CHECK(Sandbox_AddMountPoint(&cfg, NULL, "/b", true) == E_NOT_OK);
    CHECK(Sandbox_AddMountPoint(&cfg, "/a", NULL, true) == E_NOT_OK);
    CHECK(Sandbox_AddMountPoint(&cfg, "", "/b", true) == E_NOT_OK);
    (void)memset(long_path, 'a', sizeof(long_path));
    long_path[256] = '\0';
    CHECK(Sandbox_AddMountPoint(&cfg, long_path, "/b", true) == E_NOT_OK);

    CHECK(Sandbox_AddMountPoint(&cfg, "/opt", "/opt", true) == E_OK);
    CHECK(Sandbox_AddMountPoint(&cfg, "/etc", "/etc", true) == E_OK);
    CHECK(Sandbox_AddMountPoint(&cfg, "/home", "/home", true) == E_OK);
    CHECK(cfg.mountCount == 8U);
    CHECK(Sandbox_AddMountPoint(&cfg, "/var", "/var", true) == E_NOT_OK);

    /* AddDeviceAccess */
    CHECK(Sandbox_AddDeviceAccess(NULL, "/dev/null", true, true) == E_NOT_OK);
    CHECK(Sandbox_AddDeviceAccess(&cfg, NULL, true, true) == E_NOT_OK);
    CHECK(Sandbox_AddDeviceAccess(&cfg, "", true, true) == E_NOT_OK);
    for (uint32_t i = 0U; i < SANDBOX_MAX_DEVICES; ++i) {
        CHECK(Sandbox_AddDeviceAccess(&cfg, "/dev/null", true, i % 2U == 0U) == E_OK);
    }
    CHECK(Sandbox_AddDeviceAccess(&cfg, "/dev/null", true, true) == E_NOT_OK);

    /* AddSyscallFilter */
    CHECK(Sandbox_AddSyscallFilter(NULL, 1U, true) == E_NOT_OK);
    for (uint32_t i = 0U; i < SANDBOX_MAX_SYSCALL_FILTERS; ++i) {
        CHECK(Sandbox_AddSyscallFilter(&cfg, i, (i % 2U) == 0U) == E_OK);
    }
    CHECK(Sandbox_AddSyscallFilter(&cfg, 999U, true) == E_NOT_OK);

    CHECK(Sandbox_ValidateConfig(&cfg) == true);

    /* 非法配置分支 */
    bad = cfg;
    bad.rootPath[0] = '\0';
    CHECK(Sandbox_ValidateConfig(&bad) == false);

    bad = cfg;
    bad.mountCount = SANDBOX_MAX_MOUNT_POINTS + 1U;
    CHECK(Sandbox_ValidateConfig(&bad) == false);

    bad = cfg;
    bad.deviceCount = SANDBOX_MAX_DEVICES + 1U;
    CHECK(Sandbox_ValidateConfig(&bad) == false);

    bad = cfg;
    bad.syscallCount = SANDBOX_MAX_SYSCALL_FILTERS + 1U;
    CHECK(Sandbox_ValidateConfig(&bad) == false);

    bad = cfg;
    bad.mounts[0].source[0] = '\0';
    CHECK(Sandbox_ValidateConfig(&bad) == false);

    bad = cfg;
    bad.devices[0].device[0] = '\0';
    CHECK(Sandbox_ValidateConfig(&bad) == false);

    /* 隔离应用接口 */
    CHECK(Sandbox_ApplyFilesystemIsolation(1U, NULL) == E_NOT_OK);
    CHECK(Sandbox_ApplyFilesystemIsolation(1U, &cfg) == E_OK);
    CHECK(Sandbox_ApplyNetworkIsolation(1U, true) == E_OK);
    CHECK(Sandbox_ApplySyscallFiltering(1U, NULL) == E_OK);

    bad = cfg;
    bad.syscallCount = 0U;
    CHECK(Sandbox_ApplySyscallFiltering(1U, &bad) == E_OK);
    CHECK(Sandbox_ApplySyscallFiltering(1U, &cfg) == E_OK);

    CHECK(Sandbox_Deinit() == E_OK);
    CHECK(Sandbox_IsInitialized() == false);
    CHECK(Sandbox_Deinit() == E_OK);
}

static void test_sandbox_enable_disable(void)
{
    SandboxConfigType cfg;
    SandboxConfigType simple;
    SandboxStatusType status;
    SandboxConfigType all;

    CHECK(Sandbox_Init() == E_OK);

    /* 默认配置: 文件系统 + 网络隔离 */
    CHECK(Sandbox_CreateDefaultConfig(&cfg) == E_OK);
    CHECK(Sandbox_Enable(1U, &cfg) == E_OK);
    CHECK(Sandbox_IsActive(1U) == true);
    CHECK(Sandbox_IsActive(2U) == false);
    CHECK(Sandbox_GetStatus(1U, &status) == E_OK);
    CHECK(status.pid == 1U);
    CHECK(status.isActive == true);
    CHECK(strcmp(status.actualRootPath, SANDBOX_DEFAULT_ROOT_PATH) == 0);
    CHECK(Sandbox_Enable(1U, &cfg) == E_NOT_OK); /* 已激活 */

    /* 非法配置 */
    simple = cfg;
    simple.rootPath[0] = '\0';
    CHECK(Sandbox_Enable(3U, &simple) == E_NOT_OK);

    /* 无任何隔离 */
    (void)memset(&simple, 0, sizeof(simple));
    (void)strncpy(simple.rootPath, "/srv/root", sizeof(simple.rootPath) - 1U);
    simple.isolationLevel = SANDBOX_ISOLATION_NONE;
    simple.networkRestricted = false;
    simple.syscallCount = 0U;
    CHECK(Sandbox_Enable(4U, &simple) == E_OK);
    CHECK(Sandbox_IsActive(4U) == true);

    /* 全部隔离 + 系统调用过滤 */
    all = simple;
    all.isolationLevel = SANDBOX_ISOLATION_ALL;
    all.networkRestricted = true;
    CHECK(Sandbox_AddSyscallFilter(&all, 42U, true) == E_OK);
    CHECK(Sandbox_Enable(5U, &all) == E_OK);
    CHECK(Sandbox_GetStatus(5U, &status) == E_OK);
    CHECK(status.activeIsolation == SANDBOX_ISOLATION_ALL);

    Sandbox_MainFunction();

    CHECK(Sandbox_GetStatus(5U, NULL) == E_NOT_OK);
    CHECK(Sandbox_GetStatus(99U, &status) == E_NOT_OK);

    CHECK(Sandbox_Disable(1U) == E_OK);
    CHECK(Sandbox_IsActive(1U) == false);
    CHECK(Sandbox_Disable(1U) == E_NOT_OK);
    CHECK(Sandbox_Disable(99U) == E_NOT_OK);

    /* Deinit 时仍有活跃沙箱 */
    CHECK(Sandbox_Deinit() == E_OK);
    CHECK(Sandbox_IsInitialized() == false);
    CHECK(Sandbox_IsActive(4U) == false);
}

static void test_sandbox_exhaustion(void)
{
    SandboxConfigType cfg;

    CHECK(Sandbox_Init() == E_OK);
    CHECK(Sandbox_CreateDefaultConfig(&cfg) == E_OK);
    for (uint32_t i = 0U; i < EXEC_MAX_PROCESSES; ++i) {
        CHECK(Sandbox_Enable(i + 1U, &cfg) == E_OK);
    }
    CHECK(Sandbox_Enable(EXEC_MAX_PROCESSES + 1U, &cfg) == E_NOT_OK);

    Sandbox_Deinit();
}

int main(void)
{
    test_process_not_initialized();
    test_process_basic();
    test_process_restart_on_health_failure();
    test_process_exhaustion();

    test_resource_not_initialized();
    test_resource_basic();
    test_resource_exhaustion();

    test_sandbox_not_initialized();
    test_sandbox_config();
    test_sandbox_enable_disable();
    test_sandbox_exhaustion();

    if (g_failures != 0) {
        fprintf(stderr, "adaptive exec tests failed: %d\n", g_failures);
        return 1;
    }
    printf("adaptive exec tests passed\n");
    return 0;
}
