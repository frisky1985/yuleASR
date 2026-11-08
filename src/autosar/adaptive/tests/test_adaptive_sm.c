/******************************************************************************
 * @file test_adaptive_sm.c
 * @brief AUTOSAR Adaptive 状态管理服务单元测试 (StateMachine/FG/SM Interface)
 *
 * 仅覆盖公开 API 与纯逻辑分支, 不依赖真实时间源。
 ******************************************************************************/

#include "ara/sm/sm_internal.h"
#include "ara/sm/function_group.h"
#include "ara/sm/sm_interface.h"

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

static unsigned g_machine_cb_calls = 0U;
static unsigned g_fg_cb_calls = 0U;
static unsigned g_completion_calls = 0U;
static SM_RequestStatus g_last_completion = SM_REQUEST_STATUS_PENDING;

static void machine_state_cb(MachineStateType old_state, MachineStateType new_state)
{
    (void)old_state;
    (void)new_state;
    ++g_machine_cb_calls;
}

static void fg_state_cb(FunctionGroupNameType fg, FunctionGroupStateType state)
{
    (void)fg;
    (void)state;
    ++g_fg_cb_calls;
}

static void completion_cb(SM_RequestHandle handle, SM_RequestStatus status)
{
    (void)handle;
    ++g_completion_calls;
    g_last_completion = status;
}

/*=========================== StateMachine ===========================*/

static void test_state_machine_not_initialized(void)
{
    CHECK(StateMachine_IsInitialized() == false);
    CHECK(StateMachine_Deinit() == E_OK);

    CHECK(StateMachine_RequestTransition(MACHINE_STATE_STARTUP) == STATE_REQUEST_REJECTED);
    CHECK(StateMachine_GetCurrentState() == MACHINE_STATE_OFF);
    CHECK(StateMachine_GetPreviousState() == MACHINE_STATE_OFF);
    CHECK(StateMachine_ConfirmTransition(STATE_REQUEST_ACCEPTED) == E_NOT_OK);
    CHECK(StateMachine_SetFGState(FUNCTION_GROUP_POWER_MODE, FUNCTION_GROUP_STATE_ON) == E_NOT_OK);
    CHECK(StateMachine_GetFGState(FUNCTION_GROUP_POWER_MODE) == FUNCTION_GROUP_STATE_OFF);
    CHECK(StateMachine_RegisterCallback(machine_state_cb) == E_NOT_OK);
    CHECK(StateMachine_UnregisterCallback() == E_NOT_OK);
    CHECK(StateMachine_RegisterFGCallback(FUNCTION_GROUP_POWER_MODE, fg_state_cb) == E_NOT_OK);
    CHECK(StateMachine_UnregisterFGCallback(FUNCTION_GROUP_POWER_MODE) == E_NOT_OK);
    CHECK(StateMachine_CancelRequest() == E_NOT_OK);
    CHECK(StateMachine_IsRequestPending() == false);
    StateMachine_MainFunction();

    /* 字符串接口 */
    CHECK(strcmp(StateMachine_GetFGNameString(FUNCTION_GROUP_POWER_MODE), "PowerMode") == 0);
    CHECK(strcmp(StateMachine_GetFGNameString(FUNCTION_GROUP_UPDATE), "Update") == 0);
    CHECK(strcmp(StateMachine_GetFGNameString(FUNCTION_GROUP_COUNT), "Unknown") == 0);

    CHECK(strcmp(StateMachine_GetStateNameString(MACHINE_STATE_OFF), "Off") == 0);
    CHECK(strcmp(StateMachine_GetStateNameString(MACHINE_STATE_STARTUP), "Startup") == 0);
    CHECK(strcmp(StateMachine_GetStateNameString(MACHINE_STATE_DRIVING), "Driving") == 0);
    CHECK(strcmp(StateMachine_GetStateNameString(MACHINE_STATE_PARKING), "Parking") == 0);
    CHECK(strcmp(StateMachine_GetStateNameString(MACHINE_STATE_SHUTDOWN), "Shutdown") == 0);
    CHECK(strcmp(StateMachine_GetStateNameString(MACHINE_STATE_ERROR), "Error") == 0);
    CHECK(strcmp(StateMachine_GetStateNameString((MachineStateType)99), "Unknown") == 0);

    CHECK(strcmp(StateMachine_GetFGStateNameString(FUNCTION_GROUP_STATE_OFF), "Off") == 0);
    CHECK(strcmp(StateMachine_GetFGStateNameString(FUNCTION_GROUP_STATE_STARTING), "Starting") == 0);
    CHECK(strcmp(StateMachine_GetFGStateNameString(FUNCTION_GROUP_STATE_ON), "On") == 0);
    CHECK(strcmp(StateMachine_GetFGStateNameString(FUNCTION_GROUP_STATE_STOPPING), "Stopping") == 0);
    CHECK(strcmp(StateMachine_GetFGStateNameString((FunctionGroupStateType)99), "Unknown") == 0);
}

static void test_state_machine_basic(void)
{
    CHECK(StateMachine_Init() == E_OK);
    CHECK(StateMachine_IsInitialized() == true);
    CHECK(StateMachine_Init() == E_OK);

    /* 迁移规则表 */
    CHECK(StateMachine_IsTransitionAllowed(MACHINE_STATE_OFF, MACHINE_STATE_STARTUP) == true);
    CHECK(StateMachine_IsTransitionAllowed(MACHINE_STATE_OFF, MACHINE_STATE_DRIVING) == false);
    CHECK(StateMachine_IsTransitionAllowed(MACHINE_STATE_STARTUP, MACHINE_STATE_DRIVING) == true);
    CHECK(StateMachine_IsTransitionAllowed(MACHINE_STATE_STARTUP, MACHINE_STATE_SHUTDOWN) == true);
    CHECK(StateMachine_IsTransitionAllowed(MACHINE_STATE_STARTUP, MACHINE_STATE_ERROR) == true);
    CHECK(StateMachine_IsTransitionAllowed(MACHINE_STATE_DRIVING, MACHINE_STATE_PARKING) == true);
    CHECK(StateMachine_IsTransitionAllowed(MACHINE_STATE_DRIVING, MACHINE_STATE_SHUTDOWN) == true);
    CHECK(StateMachine_IsTransitionAllowed(MACHINE_STATE_DRIVING, MACHINE_STATE_ERROR) == true);
    CHECK(StateMachine_IsTransitionAllowed(MACHINE_STATE_PARKING, MACHINE_STATE_DRIVING) == true);
    CHECK(StateMachine_IsTransitionAllowed(MACHINE_STATE_PARKING, MACHINE_STATE_SHUTDOWN) == true);
    CHECK(StateMachine_IsTransitionAllowed(MACHINE_STATE_PARKING, MACHINE_STATE_ERROR) == true);
    CHECK(StateMachine_IsTransitionAllowed(MACHINE_STATE_ERROR, MACHINE_STATE_SHUTDOWN) == true);
    CHECK(StateMachine_IsTransitionAllowed(MACHINE_STATE_ERROR, MACHINE_STATE_STARTUP) == true);
    CHECK(StateMachine_IsTransitionAllowed(MACHINE_STATE_SHUTDOWN, MACHINE_STATE_OFF) == true);
    CHECK(StateMachine_IsTransitionAllowed(MACHINE_STATE_OFF, MACHINE_STATE_OFF) == false);

    /* 已在目标状态 */
    CHECK(StateMachine_RequestTransition(MACHINE_STATE_OFF) == STATE_REQUEST_ACCEPTED);

    CHECK(StateMachine_RegisterCallback(machine_state_cb) == E_OK);
    CHECK(StateMachine_RegisterFGCallback(FUNCTION_GROUP_POWER_MODE, fg_state_cb) == E_OK);
    CHECK(StateMachine_RegisterFGCallback(FUNCTION_GROUP_COUNT, fg_state_cb) == E_NOT_OK);

    /* 正常迁移 + 进行中拒绝 */
    CHECK(StateMachine_RequestTransition(MACHINE_STATE_STARTUP) == STATE_REQUEST_ACCEPTED);
    CHECK(StateMachine_IsRequestPending() == true);
    CHECK(StateMachine_GetPreviousState() == MACHINE_STATE_OFF);
    CHECK(StateMachine_RequestTransition(MACHINE_STATE_DRIVING) == STATE_REQUEST_REJECTED);

    /* 取消 */
    CHECK(StateMachine_CancelRequest() == E_OK);
    CHECK(StateMachine_IsRequestPending() == false);
    CHECK(StateMachine_CancelRequest() == E_NOT_OK);
    CHECK(StateMachine_ConfirmTransition(STATE_REQUEST_ACCEPTED) == E_NOT_OK);

    /* 未被接受的确认不改变状态 */
    CHECK(StateMachine_RequestTransition(MACHINE_STATE_STARTUP) == STATE_REQUEST_ACCEPTED);
    CHECK(StateMachine_ConfirmTransition(STATE_REQUEST_REJECTED) == E_OK);
    CHECK(StateMachine_GetCurrentState() == MACHINE_STATE_OFF);

    /* 正常确认 + 状态回调 */
    g_machine_cb_calls = 0U;
    CHECK(StateMachine_RequestTransition(MACHINE_STATE_STARTUP) == STATE_REQUEST_ACCEPTED);
    CHECK(StateMachine_ConfirmTransition(STATE_REQUEST_ACCEPTED) == E_OK);
    CHECK(StateMachine_GetCurrentState() == MACHINE_STATE_STARTUP);
    CHECK(StateMachine_GetPreviousState() == MACHINE_STATE_OFF);
    CHECK(g_machine_cb_calls == 1U);

    /* 功能组状态 */
    g_fg_cb_calls = 0U;
    CHECK(StateMachine_SetFGState(FUNCTION_GROUP_POWER_MODE, FUNCTION_GROUP_STATE_ON) == E_OK);
    CHECK(g_fg_cb_calls == 1U);
    CHECK(StateMachine_GetFGState(FUNCTION_GROUP_POWER_MODE) == FUNCTION_GROUP_STATE_ON);
    CHECK(StateMachine_SetFGState(FUNCTION_GROUP_POWER_MODE, FUNCTION_GROUP_STATE_ON) == E_OK);
    CHECK(g_fg_cb_calls == 1U); /* 相同状态不回调 */
    CHECK(StateMachine_SetFGState(FUNCTION_GROUP_POWER_MODE, FUNCTION_GROUP_STATE_OFF) == E_OK);
    CHECK(StateMachine_GetFGState(FUNCTION_GROUP_POWER_MODE) == FUNCTION_GROUP_STATE_OFF);
    CHECK(StateMachine_SetFGState(FUNCTION_GROUP_COUNT, FUNCTION_GROUP_STATE_ON) == E_NOT_OK);
    CHECK(StateMachine_GetFGState(FUNCTION_GROUP_COUNT) == FUNCTION_GROUP_STATE_OFF);

    CHECK(StateMachine_UnregisterCallback() == E_OK);
    CHECK(StateMachine_UnregisterCallback() == E_OK);
    CHECK(StateMachine_UnregisterFGCallback(FUNCTION_GROUP_POWER_MODE) == E_OK);
    CHECK(StateMachine_UnregisterFGCallback(FUNCTION_GROUP_COUNT) == E_NOT_OK);

    /* 从非 OFF 状态反初始化 */
    CHECK(StateMachine_Deinit() == E_OK);
    CHECK(StateMachine_IsInitialized() == false);
    CHECK(StateMachine_Deinit() == E_OK);
}

static void test_state_machine_main_function(void)
{
    CHECK(StateMachine_Init() == E_OK);
    CHECK(StateMachine_SetFGState(FUNCTION_GROUP_POWER_MODE, FUNCTION_GROUP_STATE_ON) == E_OK);

    CHECK(StateMachine_RequestTransition(MACHINE_STATE_STARTUP) == STATE_REQUEST_ACCEPTED);
    StateMachine_MainFunction(); /* 所有 FG 稳定 -> 自动确认 */
    CHECK(StateMachine_IsRequestPending() == false);
    CHECK(StateMachine_GetCurrentState() == MACHINE_STATE_STARTUP);

    /* SHUTDOWN 会强制所有功能组 OFF */
    CHECK(StateMachine_RequestTransition(MACHINE_STATE_SHUTDOWN) == STATE_REQUEST_ACCEPTED);
    StateMachine_MainFunction();
    CHECK(StateMachine_GetCurrentState() == MACHINE_STATE_SHUTDOWN);
    CHECK(StateMachine_GetFGState(FUNCTION_GROUP_POWER_MODE) == FUNCTION_GROUP_STATE_OFF);

    /* 无待处理请求时 MainFunction 安全 */
    StateMachine_MainFunction();

    StateMachine_Deinit();
}

/*=========================== FunctionGroup ===========================*/

static void test_function_group_not_initialized(void)
{
    FG_ConfigType cfg;
    FG_MemberType member;
    FG_RuntimeType runtime;

    CHECK(FG_IsInitialized() == false);
    CHECK(FG_Deinit() == E_OK);

    CHECK(FG_CreateConfig(FUNCTION_GROUP_POWER_MODE, &cfg) == E_NOT_OK);
    CHECK(FG_AddMember(&cfg, &member) == E_NOT_OK);
    CHECK(FG_RemoveMember(&cfg, "m") == E_NOT_OK);
    CHECK(FG_GetMember(&cfg, "m", &member) == E_NOT_OK);
    CHECK(FG_Configure(FUNCTION_GROUP_POWER_MODE, &cfg) == E_NOT_OK);
    CHECK(FG_Start(FUNCTION_GROUP_POWER_MODE) == E_NOT_OK);
    CHECK(FG_Stop(FUNCTION_GROUP_POWER_MODE) == E_NOT_OK);
    CHECK(FG_GetRuntimeState(FUNCTION_GROUP_POWER_MODE, &runtime) == E_NOT_OK);
    CHECK(FG_IsStable(FUNCTION_GROUP_POWER_MODE) == false);
    CHECK(FG_WaitForStable(FUNCTION_GROUP_POWER_MODE, 0U) == E_NOT_OK);
    CHECK(FG_NotifyMemberState(FUNCTION_GROUP_POWER_MODE, "m", true) == E_NOT_OK);
    CHECK(FG_GetActiveMemberCount(FUNCTION_GROUP_POWER_MODE) == 0U);
    FG_MainFunction();
}

static void test_function_group_basic(void)
{
    FG_ConfigType cfg;
    FG_MemberType member;
    FG_MemberType out;
    FG_RuntimeType runtime;

    CHECK(FG_Init() == E_OK);
    CHECK(FG_IsInitialized() == true);
    CHECK(FG_Init() == E_OK);

    CHECK(FG_CreateConfig(FUNCTION_GROUP_POWER_MODE, NULL) == E_NOT_OK);
    CHECK(FG_CreateConfig(FUNCTION_GROUP_COUNT, &cfg) == E_NOT_OK);
    CHECK(FG_CreateConfig(FUNCTION_GROUP_POWER_MODE, &cfg) == E_OK);
    CHECK(strcmp(cfg.displayName, "PowerMode") == 0);
    CHECK(cfg.memberCount == 0U);
    CHECK(cfg.transitionTimeoutMs == FG_TRANSITION_TIMEOUT_MS);

    (void)memset(&member, 0, sizeof(member));
    (void)strncpy(member.name, "svc", sizeof(member.name) - 1U);
    member.type = FG_MEMBER_KIND_PROCESS;
    member.isRequired = true;

    CHECK(FG_AddMember(NULL, &member) == E_NOT_OK);
    CHECK(FG_AddMember(&cfg, NULL) == E_NOT_OK);
    CHECK(FG_AddMember(&cfg, &member) == E_OK);
    CHECK(FG_AddMember(&cfg, &member) == E_NOT_OK); /* 重名 */

    /* 填满成员表 */
    for (uint32_t i = 1U; i < FG_MAX_MEMBERS; ++i) {
        FG_MemberType extra;
        (void)memset(&extra, 0, sizeof(extra));
        (void)snprintf(extra.name, sizeof(extra.name), "m%u", i);
        CHECK(FG_AddMember(&cfg, &extra) == E_OK);
    }
    CHECK(cfg.memberCount == FG_MAX_MEMBERS);
    {
        FG_MemberType extra;
        (void)memset(&extra, 0, sizeof(extra));
        (void)strncpy(extra.name, "overflow", sizeof(extra.name) - 1U);
        CHECK(FG_AddMember(&cfg, &extra) == E_NOT_OK);
    }

    /* 查询成员 */
    CHECK(FG_GetMember(&cfg, "svc", &out) == E_OK);
    CHECK(out.type == FG_MEMBER_KIND_PROCESS);
    CHECK(FG_GetMember(&cfg, "nope", &out) == E_NOT_OK);
    CHECK(FG_GetMember(NULL, "svc", &out) == E_NOT_OK);
    CHECK(FG_GetMember(&cfg, NULL, &out) == E_NOT_OK);
    CHECK(FG_GetMember(&cfg, "svc", NULL) == E_NOT_OK);

    /* 删除成员 (触发元素前移) */
    CHECK(FG_RemoveMember(NULL, "svc") == E_NOT_OK);
    CHECK(FG_RemoveMember(&cfg, NULL) == E_NOT_OK);
    CHECK(FG_RemoveMember(&cfg, "nope") == E_NOT_OK);
    CHECK(FG_RemoveMember(&cfg, "svc") == E_OK);
    CHECK(cfg.memberCount == (FG_MAX_MEMBERS - 1U));
    CHECK(FG_GetMember(&cfg, "svc", &out) == E_NOT_OK);

    /* 配置生效 */
    CHECK(FG_Configure(FUNCTION_GROUP_POWER_MODE, NULL) == E_NOT_OK);
    CHECK(FG_Configure(FUNCTION_GROUP_COUNT, &cfg) == E_NOT_OK);
    CHECK(FG_Configure(FUNCTION_GROUP_POWER_MODE, &cfg) == E_OK);

    CHECK(FG_GetRuntimeState(FUNCTION_GROUP_POWER_MODE, &runtime) == E_OK);
    CHECK(runtime.name == FUNCTION_GROUP_POWER_MODE);
    CHECK(runtime.currentState == FUNCTION_GROUP_STATE_OFF);
    CHECK(runtime.isStable == true);
    CHECK(FG_GetRuntimeState(FUNCTION_GROUP_COUNT, &runtime) == E_NOT_OK);
    CHECK(FG_GetRuntimeState(FUNCTION_GROUP_POWER_MODE, NULL) == E_NOT_OK);

    CHECK(FG_IsStable(FUNCTION_GROUP_POWER_MODE) == true);
    CHECK(FG_IsStable(FUNCTION_GROUP_COUNT) == false);
    CHECK(FG_WaitForStable(FUNCTION_GROUP_POWER_MODE, 0U) == E_OK);
    CHECK(FG_WaitForStable(FUNCTION_GROUP_COUNT, 10U) == E_NOT_OK);

    /* 启动 -> STARTING, 迁移期间不可配置 */
    CHECK(FG_Start(FUNCTION_GROUP_COUNT) == E_NOT_OK);
    CHECK(FG_Start(FUNCTION_GROUP_POWER_MODE) == E_OK);
    CHECK(FG_IsStable(FUNCTION_GROUP_POWER_MODE) == false);
    CHECK(FG_GetRuntimeState(FUNCTION_GROUP_POWER_MODE, &runtime) == E_OK);
    CHECK(runtime.transitionInProgress == true);
    CHECK(runtime.currentState == FUNCTION_GROUP_STATE_STARTING);
    CHECK(FG_Configure(FUNCTION_GROUP_POWER_MODE, &cfg) == E_NOT_OK);
    CHECK(FG_Start(FUNCTION_GROUP_POWER_MODE) == E_OK);

    /* 成员状态通知 */
    CHECK(FG_GetActiveMemberCount(FUNCTION_GROUP_POWER_MODE) == 0U);
    CHECK(FG_NotifyMemberState(FUNCTION_GROUP_POWER_MODE, "nope", true) == E_NOT_OK);
    CHECK(FG_NotifyMemberState(FUNCTION_GROUP_COUNT, "m1", true) == E_NOT_OK);
    CHECK(FG_NotifyMemberState(FUNCTION_GROUP_POWER_MODE, NULL, true) == E_NOT_OK);
    CHECK(FG_NotifyMemberState(FUNCTION_GROUP_POWER_MODE, "m1", true) == E_OK);
    CHECK(FG_GetActiveMemberCount(FUNCTION_GROUP_POWER_MODE) == 1U);
    CHECK(FG_NotifyMemberState(FUNCTION_GROUP_POWER_MODE, "m1", true) == E_OK);
    CHECK(FG_GetActiveMemberCount(FUNCTION_GROUP_POWER_MODE) == 1U);
    CHECK(FG_NotifyMemberState(FUNCTION_GROUP_POWER_MODE, "m1", false) == E_OK);
    CHECK(FG_GetActiveMemberCount(FUNCTION_GROUP_POWER_MODE) == 0U);

    /* 停止 */
    CHECK(FG_Stop(FUNCTION_GROUP_COUNT) == E_NOT_OK);
    CHECK(FG_Stop(FUNCTION_GROUP_POWER_MODE) == E_OK);
    CHECK(FG_GetRuntimeState(FUNCTION_GROUP_POWER_MODE, &runtime) == E_OK);
    CHECK(runtime.currentState == FUNCTION_GROUP_STATE_STOPPING);

    CHECK(FG_Deinit() == E_OK);
    CHECK(FG_IsInitialized() == false);
    CHECK(FG_Deinit() == E_OK);
}

static void test_function_group_main_function(void)
{
    FG_ConfigType cfg;
    FG_MemberType member;
    FG_RuntimeType runtime;

    CHECK(FG_Init() == E_OK);

    CHECK(FG_CreateConfig(FUNCTION_GROUP_COMMUNICATION, &cfg) == E_OK);
    (void)memset(&member, 0, sizeof(member));
    (void)strncpy(member.name, "svc", sizeof(member.name) - 1U);
    member.type = FG_MEMBER_KIND_SERVICE;
    CHECK(FG_AddMember(&cfg, &member) == E_OK);
    CHECK(FG_Configure(FUNCTION_GROUP_COMMUNICATION, &cfg) == E_OK);

    /* 成员未就绪 -> 迁移保持 */
    CHECK(FG_Start(FUNCTION_GROUP_COMMUNICATION) == E_OK);
    FG_MainFunction();
    CHECK(FG_IsStable(FUNCTION_GROUP_COMMUNICATION) == false);
    CHECK(FG_GetRuntimeState(FUNCTION_GROUP_COMMUNICATION, &runtime) == E_OK);
    CHECK(runtime.currentState == FUNCTION_GROUP_STATE_STARTING);

    /* 成员就绪 -> 迁移完成 */
    CHECK(FG_NotifyMemberState(FUNCTION_GROUP_COMMUNICATION, "svc", true) == E_OK);
    FG_MainFunction();
    CHECK(FG_IsStable(FUNCTION_GROUP_COMMUNICATION) == true);
    CHECK(FG_GetRuntimeState(FUNCTION_GROUP_COMMUNICATION, &runtime) == E_OK);
    CHECK(runtime.currentState == FUNCTION_GROUP_STATE_ON);

    /* 停止时成员仍活跃 -> 保持 STOPPING */
    CHECK(FG_Stop(FUNCTION_GROUP_COMMUNICATION) == E_OK);
    FG_MainFunction();
    CHECK(FG_GetRuntimeState(FUNCTION_GROUP_COMMUNICATION, &runtime) == E_OK);
    CHECK(runtime.currentState == FUNCTION_GROUP_STATE_STOPPING);

    CHECK(FG_NotifyMemberState(FUNCTION_GROUP_COMMUNICATION, "svc", false) == E_OK);
    FG_MainFunction();
    CHECK(FG_IsStable(FUNCTION_GROUP_COMMUNICATION) == true);
    CHECK(FG_GetRuntimeState(FUNCTION_GROUP_COMMUNICATION, &runtime) == E_OK);
    CHECK(runtime.currentState == FUNCTION_GROUP_STATE_OFF);

    /* 无成员的功能组立即完成迁移 */
    CHECK(FG_Start(FUNCTION_GROUP_DIAGNOSTICS) == E_OK);
    FG_MainFunction();
    CHECK(FG_GetRuntimeState(FUNCTION_GROUP_DIAGNOSTICS, &runtime) == E_OK);
    CHECK(runtime.currentState == FUNCTION_GROUP_STATE_ON);

    FG_Deinit();
}

/*=========================== SM Interface ===========================*/

static void test_sm_interface_not_initialized(void)
{
    uint32_t client_id = 0U;
    SM_RequestType request;
    SM_ResponseType response;

    CHECK(SM_Interface_IsInitialized() == false);
    CHECK(SM_Interface_Deinit() == E_OK);

    CHECK(SM_RegisterClient("c", &client_id) == E_NOT_OK);
    CHECK(SM_UnregisterClient(1U) == E_NOT_OK);
    CHECK(SM_CreateStateRequest(1U, MACHINE_STATE_STARTUP) == 0U);
    CHECK(SM_CreateFGStateRequest(1U, FUNCTION_GROUP_POWER_MODE,
                                  FUNCTION_GROUP_STATE_ON) == 0U);
    CHECK(SM_SubmitRequest(1U) == E_NOT_OK);
    CHECK(SM_CancelRequest(1U) == E_NOT_OK);
    CHECK(SM_GetRequestStatus(1U) == SM_REQUEST_STATUS_FAILED);
    CHECK(SM_ConfirmRequest(1U) == E_NOT_OK);
    CHECK(SM_RejectRequest(1U, -1, "e") == E_NOT_OK);
    CHECK(SM_CompleteRequest(1U, true) == E_NOT_OK);
    CHECK(SM_SetCompletionCallback(1U, completion_cb) == E_NOT_OK);
    CHECK(SM_WaitForCompletion(1U, 0U) == SM_REQUEST_STATUS_FAILED);
    CHECK(SM_GetRequest(1U, &request) == E_NOT_OK);
    CHECK(SM_GetResponse(1U, &response) == E_NOT_OK);
    SM_Interface_MainFunction();
    CHECK(SM_GetActiveRequestCount() == 0U);
    CHECK(SM_CancelClientRequests(1U) == E_NOT_OK);
}

static void test_sm_interface_basic(void)
{
    uint32_t client_a = 0U;
    uint32_t client_b = 0U;
    SM_RequestHandle handle;
    SM_RequestType request;
    SM_ResponseType response;

    CHECK(SM_Interface_Init() == E_OK);
    CHECK(SM_Interface_IsInitialized() == true);
    CHECK(SM_Interface_Init() == E_OK);

    CHECK(SM_RegisterClient(NULL, &client_a) == E_NOT_OK);
    CHECK(SM_RegisterClient("c", NULL) == E_NOT_OK);
    CHECK(SM_RegisterClient("client-a", &client_a) == E_OK);
    CHECK(SM_RegisterClient("client-b", &client_b) == E_OK);
    CHECK(client_a != client_b);

    CHECK(SM_CreateStateRequest(1234U, MACHINE_STATE_STARTUP) == 0U);
    CHECK(SM_CreateFGStateRequest(1234U, FUNCTION_GROUP_POWER_MODE,
                                  FUNCTION_GROUP_STATE_ON) == 0U);
    CHECK(SM_UnregisterClient(1234U) == E_NOT_OK);

    CHECK(SM_UnregisterClient(client_b) == E_OK);
    CHECK(SM_UnregisterClient(client_b) == E_NOT_OK);

    handle = SM_CreateStateRequest(client_a, MACHINE_STATE_STARTUP);
    CHECK(handle != 0U);
    CHECK(SM_GetRequestStatus(handle) == SM_REQUEST_STATUS_PENDING);
    CHECK(SM_GetActiveRequestCount() == 1U);

    CHECK(SM_GetRequest(handle, &request) == E_OK);
    CHECK(request.handle == handle);
    CHECK(request.type == SM_REQUEST_KIND_STATE_TRANSITION);
    CHECK(request.clientId == client_a);
    CHECK(request.data.targetState == MACHINE_STATE_STARTUP);
    CHECK(SM_GetRequest(handle, NULL) == E_NOT_OK);
    CHECK(SM_GetRequest(9999U, &request) == E_NOT_OK);

    CHECK(SM_GetResponse(handle, &response) == E_OK);
    CHECK(response.handle == handle);
    CHECK(response.status == SM_REQUEST_STATUS_PENDING);
    CHECK(SM_GetResponse(handle, NULL) == E_NOT_OK);
    CHECK(SM_GetResponse(9999U, &response) == E_NOT_OK);

    CHECK(SM_SetCompletionCallback(handle, completion_cb) == E_OK);
    CHECK(SM_SetCompletionCallback(9999U, completion_cb) == E_NOT_OK);

    g_completion_calls = 0U;
    CHECK(SM_CancelRequest(handle) == E_OK);
    CHECK(g_completion_calls == 1U);
    CHECK(g_last_completion == SM_REQUEST_STATUS_CANCELLED);
    CHECK(SM_GetRequestStatus(handle) == SM_REQUEST_STATUS_FAILED);
    CHECK(SM_CancelRequest(handle) == E_NOT_OK);
    CHECK(SM_GetActiveRequestCount() == 0U);

    /* MainFunction 无请求时安全 */
    SM_Interface_MainFunction();

    CHECK(SM_UnregisterClient(client_a) == E_OK);
    CHECK(SM_Interface_Deinit() == E_OK);
    CHECK(SM_Interface_IsInitialized() == false);
    CHECK(SM_Interface_Deinit() == E_OK);
}

static void test_sm_interface_requests(void)
{
    uint32_t client = 0U;
    SM_RequestHandle handle;
    SM_RequestHandle handle2;
    SM_RequestHandle handle3;
    SM_RequestHandle handle4;
    SM_RequestHandle handle5;
    SM_RequestStatus status;

    CHECK(StateMachine_Init() == E_OK);
    CHECK(SM_Interface_Init() == E_OK);
    CHECK(SM_RegisterClient("sm-client", &client) == E_OK);

    /* 状态迁移请求 -> 被接受 -> 确认 */
    handle = SM_CreateStateRequest(client, MACHINE_STATE_STARTUP);
    CHECK(handle != 0U);
    CHECK(SM_SetCompletionCallback(handle, completion_cb) == E_OK);
    g_completion_calls = 0U;
    CHECK(SM_SubmitRequest(handle) == E_OK);
    CHECK(SM_GetRequestStatus(handle) == SM_REQUEST_STATUS_ACCEPTED);
    CHECK(SM_SubmitRequest(handle) == E_NOT_OK); /* 非 PENDING */
    StateMachine_MainFunction();                 /* 完成 SM 内部迁移 */
    CHECK(SM_ConfirmRequest(handle) == E_OK);
    CHECK(g_completion_calls == 1U);
    CHECK(g_last_completion == SM_REQUEST_STATUS_COMPLETED);
    CHECK(SM_ConfirmRequest(handle) == E_NOT_OK);

    /* 非法迁移 -> REJECTED, WaitForCompletion 立即返回 */
    handle2 = SM_CreateStateRequest(client, MACHINE_STATE_OFF);
    CHECK(handle2 != 0U);
    CHECK(SM_SubmitRequest(handle2) == E_OK);
    CHECK(SM_GetRequestStatus(handle2) == SM_REQUEST_STATUS_REJECTED);
    status = SM_WaitForCompletion(handle2, 100U);
    CHECK(status == SM_REQUEST_STATUS_REJECTED);
    CHECK(SM_CancelRequest(handle2) == E_OK);

    /* 功能组请求 -> 完成/拒绝/失败 */
    handle3 = SM_CreateFGStateRequest(client, FUNCTION_GROUP_POWER_MODE,
                                      FUNCTION_GROUP_STATE_ON);
    CHECK(handle3 != 0U);
    CHECK(SM_SetCompletionCallback(handle3, completion_cb) == E_OK);
    g_completion_calls = 0U;
    CHECK(SM_SubmitRequest(handle3) == E_OK);
    CHECK(SM_GetRequestStatus(handle3) == SM_REQUEST_STATUS_ACCEPTED);
    CHECK(SM_CompleteRequest(handle3, true) == E_OK);
    CHECK(g_completion_calls == 1U);
    CHECK(g_last_completion == SM_REQUEST_STATUS_COMPLETED);
    CHECK(SM_CompleteRequest(handle3, true) == E_NOT_OK);

    handle4 = SM_CreateFGStateRequest(client, FUNCTION_GROUP_DIAGNOSTICS,
                                      FUNCTION_GROUP_STATE_ON);
    CHECK(handle4 != 0U);
    CHECK(SM_SetCompletionCallback(handle4, completion_cb) == E_OK);
    CHECK(SM_SubmitRequest(handle4) == E_OK);
    g_completion_calls = 0U;
    CHECK(SM_RejectRequest(handle4, -2, "denied") == E_OK);
    CHECK(g_completion_calls == 1U);
    CHECK(g_last_completion == SM_REQUEST_STATUS_REJECTED);
    CHECK(SM_RejectRequest(handle4, -2, "denied") == E_NOT_OK);

    handle5 = SM_CreateFGStateRequest(client, FUNCTION_GROUP_LOGGING,
                                      FUNCTION_GROUP_STATE_ON);
    CHECK(handle5 != 0U);
    CHECK(SM_SetCompletionCallback(handle5, completion_cb) == E_OK);
    CHECK(SM_SubmitRequest(handle5) == E_OK);
    g_completion_calls = 0U;
    CHECK(SM_CompleteRequest(handle5, false) == E_OK);
    CHECK(g_completion_calls == 1U);
    CHECK(g_last_completion == SM_REQUEST_STATUS_FAILED);

    CHECK(SM_SubmitRequest(9999U) == E_NOT_OK);
    CHECK(SM_SetCompletionCallback(9999U, completion_cb) == E_NOT_OK);

    /* 请求槽耗尽 */
    SM_RequestHandle handles[SM_INTERFACE_MAX_REQUESTS];
    for (uint32_t i = 0U; i < SM_INTERFACE_MAX_REQUESTS; ++i) {
        handles[i] = SM_CreateFGStateRequest(client, FUNCTION_GROUP_UPDATE,
                                             FUNCTION_GROUP_STATE_ON);
        CHECK(handles[i] != 0U);
    }
    CHECK(SM_CreateFGStateRequest(client, FUNCTION_GROUP_UPDATE,
                                  FUNCTION_GROUP_STATE_ON) == 0U);
    CHECK(SM_GetActiveRequestCount() == SM_INTERFACE_MAX_REQUESTS);

    /* MainFunction: 未超时的 PENDING 请求保持 */
    SM_Interface_MainFunction();
    CHECK(SM_GetActiveRequestCount() == SM_INTERFACE_MAX_REQUESTS);

    for (uint32_t i = 0U; i < SM_INTERFACE_MAX_REQUESTS; ++i) {
        CHECK(SM_CancelRequest(handles[i]) == E_OK);
    }
    CHECK(SM_GetActiveRequestCount() == 0U);

    /* 按客户端取消 */
    CHECK(SM_CreateFGStateRequest(client, FUNCTION_GROUP_UPDATE,
                                  FUNCTION_GROUP_STATE_ON) != 0U);
    CHECK(SM_CreateFGStateRequest(client, FUNCTION_GROUP_UPDATE,
                                  FUNCTION_GROUP_STATE_ON) != 0U);
    CHECK(SM_GetActiveRequestCount() == 2U);
    CHECK(SM_CancelClientRequests(9999U) == E_NOT_OK);
    CHECK(SM_CancelClientRequests(client) == E_OK);
    CHECK(SM_GetActiveRequestCount() == 0U);

    /* 注销客户端会取消其挂起请求 */
    CHECK(SM_CreateFGStateRequest(client, FUNCTION_GROUP_UPDATE,
                                  FUNCTION_GROUP_STATE_ON) != 0U);
    CHECK(SM_GetActiveRequestCount() == 1U);
    CHECK(SM_UnregisterClient(client) == E_OK);
    CHECK(SM_GetActiveRequestCount() == 0U);

    /* Deinit 时仍有挂起请求 */
    CHECK(SM_RegisterClient("late", &client) == E_OK);
    CHECK(SM_CreateFGStateRequest(client, FUNCTION_GROUP_UPDATE,
                                  FUNCTION_GROUP_STATE_ON) != 0U);
    CHECK(SM_Interface_Deinit() == E_OK);

    CHECK(StateMachine_Deinit() == E_OK);
}

int main(void)
{
    test_state_machine_not_initialized();
    test_state_machine_basic();
    test_state_machine_main_function();

    test_function_group_not_initialized();
    test_function_group_basic();
    test_function_group_main_function();

    test_sm_interface_not_initialized();
    test_sm_interface_basic();
    test_sm_interface_requests();

    if (g_failures != 0) {
        fprintf(stderr, "adaptive sm tests failed: %d\n", g_failures);
        return 1;
    }
    printf("adaptive sm tests passed\n");
    return 0;
}
