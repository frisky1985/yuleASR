/*==================================================================================================* Project : YuleTech
 * AutoSAR BSW* Platform             : NXP i.MX8M Mini* Dependencies         : ...** Copyright (c) 2026 Shanghai Yule
 * Electronics Technology Co., Ltd.* All rights reserved.** SPDX-License-Identifier:
 * MIT**================================================================================================*/
/** * @file LinSlave_Uds.c * @brief LIN UDS (统一诊断服务) 框架实现 * @version 1.0.0 */
#include "LinSlave_Uds.h"
#include "LinSlave_Tp.h"
#include <string.h>
/* 内部状态 */
static LinSlave_Uds_RuntimeType UdsRuntime;
static LinSlave_Uds_ServiceConfigType UdsServiceTable[LINSLAVE_UDS_MAX_SID_SERVICES];
static uint8 UdsServiceCount = 0;
static boolean UdsInitialized = FALSE;
/* 前向声明 */
static LinSlave_Uds_StatusType LinSlave_Uds_ProcessService(const LinSlave_Uds_RequestType* Request,
                                                           LinSlave_Uds_ResponseType* Response);
static LinSlave_Uds_ServiceConfigType* LinSlave_Uds_FindService(uint8 Sid);
static boolean LinSlave_Uds_CheckSession(const LinSlave_Uds_ServiceConfigType* Service);
static boolean LinSlave_Uds_CheckSecurity(LinSlave_Uds_ServiceConfigType* Service);
/** * @brief UDS初始化 */
/** @req SWS_Lin_00160 */
LinSlave_Uds_StatusType LinSlave_Uds_Init(void)
{
    (void)memset(&UdsRuntime, 0, sizeof(LinSlave_Uds_RuntimeType));
    (void)memset(UdsServiceTable, 0, sizeof(UdsServiceTable));
    UdsRuntime.State = LINSLAVE_UDS_STATE_IDLE;
    UdsRuntime.CurrentSession = LINSLAVE_UDS_SESSION_DEFAULT;
    UdsRuntime.SecurityLevel = LINSLAVE_UDS_SECURITY_LOCKED;
    UdsRuntime.S3ServerTimer = 0;
    UdsRuntime.TesterPresentReceived = FALSE;
    UdsServiceCount = 0;
    UdsInitialized = TRUE;
    return LINSLAVE_UDS_OK;
}
/** * @brief UDS反初始化 */
/** @req SWS_Lin_00161 */
void LinSlave_Uds_DeInit(void)
{
    UdsInitialized = FALSE;
}
/** * @brief 查找服务配置 */
static LinSlave_Uds_ServiceConfigType* LinSlave_Uds_FindService(uint8 Sid)
{
    uint8 i;
    for (i = 0U; i < UdsServiceCount; i++)
    {
        if (UdsServiceTable[i].Sid == Sid)
        {
            return &UdsServiceTable[i];
        }
    }
    return NULL_PTR;
}
/** * @brief 检查会话权限 */
static boolean LinSlave_Uds_CheckSession(const LinSlave_Uds_ServiceConfigType* Service)
{
    if (!Service->NeedsSession)
    {
        return TRUE;
    }
    return (UdsRuntime.CurrentSession >= Service->MinSession);
}
/** * @brief 检查安全权限 */
static boolean LinSlave_Uds_CheckSecurity(LinSlave_Uds_ServiceConfigType* Service)
{
    if (!Service->NeedsSecurity)
    {
        return TRUE;
    }
    return (UdsRuntime.SecurityLevel >= Service->MinSecurityLevel);
}
/** * @brief 处理请求 */
/** @req SWS_Lin_00162 */
LinSlave_Uds_StatusType LinSlave_Uds_ProcessRequest(const uint8* DataPtr, uint16 Length)
{
    LinSlave_Uds_RequestType Request;
    LinSlave_Uds_ResponseType Response;
    LinSlave_Uds_StatusType Status;
    LinSlave_Uds_ServiceConfigType* Service;
    if (!UdsInitialized || DataPtr == NULL_PTR || Length == 0U)
    {
        return LINSLAVE_UDS_E_NOT_OK;
    }
    /* 解析请求 */
    Request.Sid = DataPtr[0];
    Request.SubFunction = (Length > 1) ? DataPtr[1] : 0;
    Request.DataPtr = (Length > 1) ? &DataPtr[1] : NULL_PTR;
    Request.DataLength = (Length > 1) ? Length - 1 : 0;
    /* 重置S3计时器 */
    UdsRuntime.S3ServerTimer = 0;
    /* 检查TesterPresent */
    if (Request.Sid == LINSLAVE_UDS_SID_TESTER_PRESENT)
    {
        UdsRuntime.TesterPresentReceived = TRUE;
    }
    /* 查找服务 */
    Service = LinSlave_Uds_FindService(Request.Sid);
    if (Service == NULL_PTR)
    {
        /* 服务不支持 */
        /** @req SWS_Lin_00164 */
        (void)LinSlave_Uds_SendNegativeResponse(Request.Sid, LINSLAVE_UDS_NRC_SERVICE_NOT_SUPPORTED);
        return LINSLAVE_UDS_E_INVALID_SID;
    }
    /* 检查会话 */
    if (!LinSlave_Uds_CheckSession(Service))
    {
        /** @req SWS_Lin_00164 */
        (void)LinSlave_Uds_SendNegativeResponse(Request.Sid, LINSLAVE_UDS_NRC_SERVICE_NOT_SUPPORTED);
        return LINSLAVE_UDS_E_SESSION_NOT_SUPPORTED;
    }
    /* 检查安全 */
    if (!LinSlave_Uds_CheckSecurity(Service))
    {
        /** @req SWS_Lin_00164 */
        (void)LinSlave_Uds_SendNegativeResponse(Request.Sid, LINSLAVE_UDS_NRC_SECURITY_ACCESS_DENIED);
        return LINSLAVE_UDS_E_SECURITY_DENIED;
    }
    /* 准备响应 */
    (void)memset(&Response, 0, sizeof(Response));
    Response.DataPtr = UdsRuntime.ResponseBuffer;
    Response.DataLength = 0;
    Response.IsNegative = FALSE;
    UdsRuntime.State = LINSLAVE_UDS_STATE_PROCESSING;
    /* 调用服务处理函数 */
    Status = Service->Handler(&Request, &Response);
    UdsRuntime.State = LINSLAVE_UDS_STATE_IDLE;
    return Status;
}
/** * @brief 发送正响响应 */
/** @req SWS_Lin_00163 */
LinSlave_Uds_StatusType LinSlave_Uds_SendPositiveResponse(const LinSlave_Uds_ResponseType* ResponsePtr)
{
    uint8 ResponseBuffer[LINSLAVE_UDS_MAX_DSL_BUFFER];
    uint16 ResponseLength;
    if (!UdsInitialized || ResponsePtr == NULL_PTR)
    {
        return LINSLAVE_UDS_E_NOT_OK;
    }
    /* 构建响应消息 */
    ResponseBuffer[0] = ResponsePtr->Sid + 0x40;
    /* 正响响应: SID + 0x40 */
    if (ResponsePtr->DataLength > 0U && ResponsePtr->DataPtr != NULL_PTR)
    {
        (void)memcpy(&ResponseBuffer[1], ResponsePtr->DataPtr, ResponsePtr->DataLength);
        ResponseLength = ResponsePtr->DataLength + 1;
    }
    else
    {
        ResponseLength = 1;
    }
    /* 通过TP层发送 */
    (void)LinSlave_Tp_Transmit(0, ResponseBuffer, ResponseLength);
    return LINSLAVE_UDS_OK;
}
/** * @brief 发送负响响应 */
/** @req SWS_Lin_00164 */
LinSlave_Uds_StatusType LinSlave_Uds_SendNegativeResponse(uint8 Sid, uint8 Nrc)
{
    uint8 ResponseBuffer[3];
    if (UdsInitialized == 0U)
    {
        return LINSLAVE_UDS_E_NOT_OK;
    }
    /* 构建负响响应 */
    ResponseBuffer[0] = 0x7F;
    /* 负响响应服务 */
    ResponseBuffer[1] = Sid;
    /* 请求的SID */
    ResponseBuffer[2] = Nrc;
    /* 负响响应码 */
    /* 通过TP层发送 */
    (void)LinSlave_Tp_Transmit(0, ResponseBuffer, 3);
    return LINSLAVE_UDS_OK;
}
/** * @brief 注册服务处理函数 */
/** @req SWS_Lin_00165 */
LinSlave_Uds_StatusType LinSlave_Uds_RegisterService(const LinSlave_Uds_ServiceConfigType* ServiceConfig)
{
    if (!UdsInitialized || ServiceConfig == NULL_PTR)
    {
        return LINSLAVE_UDS_E_NOT_OK;
    }
    if (UdsServiceCount >= LINSLAVE_UDS_MAX_SID_SERVICES)
    {
        return LINSLAVE_UDS_E_NOT_OK;
    }
    if (ServiceConfig->Handler == NULL_PTR)
    {
        return LINSLAVE_UDS_E_NOT_OK;
    }
    /* 检查是否已存在 */
    if (LinSlave_Uds_FindService(ServiceConfig->Sid) != NULL_PTR)
    {
        return LINSLAVE_UDS_E_NOT_OK;
        /* 服务已存在 */
    }
    (void)memcpy(&UdsServiceTable[UdsServiceCount], ServiceConfig, sizeof(LinSlave_Uds_ServiceConfigType));
    UdsServiceCount++;
    return LINSLAVE_UDS_OK;
}
/** * @brief UDS主函数 */
/** @req SWS_Lin_00166 */
void LinSlave_Uds_MainFunction(void)
{
    if (UdsInitialized == 0U)
    {
        return;
    }
    /* S3计时器管理 */
    UdsRuntime.S3ServerTimer++;
    /* 检查S3超时 (S3Server: 通常为5000ms) */
    if (UdsRuntime.S3ServerTimer > 5000)
    {
        /* 超时，返回默认会话 */
        UdsRuntime.CurrentSession = LINSLAVE_UDS_SESSION_DEFAULT;
        UdsRuntime.SecurityLevel = LINSLAVE_UDS_SECURITY_LOCKED;
        UdsRuntime.S3ServerTimer = 0;
        UdsRuntime.TesterPresentReceived = FALSE;
    }
}
/** * @brief 获取当前会话类型 */
/** @req SWS_Lin_00167 */
LinSlave_Uds_SessionType LinSlave_Uds_GetSessionType(void)
{
    return UdsRuntime.CurrentSession;
}
/** * @brief 获取当前安全级别 */
/** @req SWS_Lin_00168 */
LinSlave_Uds_SecurityLevelType LinSlave_Uds_GetSecurityLevel(void)
{
    return UdsRuntime.SecurityLevel;
}
/** * @brief 设置会话类型 */
/** @req SWS_Lin_00169 */
void LinSlave_Uds_SetSessionType(LinSlave_Uds_SessionType SessionType)
{
    UdsRuntime.CurrentSession = SessionType;
    UdsRuntime.S3ServerTimer = 0;
    /* 重置计时器 */
}
/** * @brief 设置安全级别 */
/** @req SWS_Lin_00170 */
void LinSlave_Uds_SetSecurityLevel(LinSlave_Uds_SecurityLevelType SecurityLevel)
{
    UdsRuntime.SecurityLevel = SecurityLevel;
}
/** * @brief 检查会话超时 */
/** @req SWS_Lin_00171 */
boolean LinSlave_Uds_IsSessionTimeout(void)
{
    return (UdsRuntime.S3ServerTimer > 5000);
}
/* ==================== 默认服务处理函数 ==================== */
/** * @brief 诊断会话控制服务 (0x10) */
static LinSlave_Uds_StatusType Uds_SessionControl(const LinSlave_Uds_RequestType* Request,
                                                  LinSlave_Uds_ResponseType* Response)
{
    uint8 SessionType = Request->SubFunction & 0x7F;
    /* 设置会话 */
    /** @req SWS_Lin_00169 */
    LinSlave_Uds_SetSessionType((LinSlave_Uds_SessionType)SessionType);
    /* 构建响应 */
    Response->Sid = LINSLAVE_UDS_SID_DIAGNOSTIC_SESSION_CONTROL;
    Response->DataPtr[0] = SessionType;
    Response->DataPtr[1] = 0x00;
    /* P2Server_high */
    Response->DataPtr[2] = 0x32;
    /* P2Server_low (50ms) */
    Response->DataPtr[3] = 0x01;
    /* P2*Server_high */
    Response->DataPtr[4] = 0xF4;
    /* P2*Server_low (5000ms) */
    Response->DataLength = 5;
    Response->IsNegative = FALSE;
    /** @req SWS_Lin_00163 */
    (void)LinSlave_Uds_SendPositiveResponse(Response);
    return LINSLAVE_UDS_OK;
}
/** * @brief ECU复位服务 (0x11) */
static LinSlave_Uds_StatusType Uds_EcuReset(const LinSlave_Uds_RequestType* Request,
                                            LinSlave_Uds_ResponseType* Response)
{
    uint8 ResetType = Request->SubFunction & 0x7F;
    (void)ResetType;
    /* 根据复位类型执行不同操作 */
    Response->Sid = LINSLAVE_UDS_SID_ECU_RESET;
    Response->DataPtr[0] = ResetType;
    Response->DataLength = 1;
    Response->IsNegative = FALSE;
    /** @req SWS_Lin_00163 */
    (void)LinSlave_Uds_SendPositiveResponse(Response);
    return LINSLAVE_UDS_OK;
}
/** * @brief TesterPresent服务 (0x3E) */
static LinSlave_Uds_StatusType Uds_TesterPresent(const LinSlave_Uds_RequestType* Request,
                                                 LinSlave_Uds_ResponseType* Response)
{
    uint8 SubFunction = Request->SubFunction & 0x7F;
    /* 重置S3计时器 */
    UdsRuntime.S3ServerTimer = 0;
    UdsRuntime.TesterPresentReceived = TRUE;
    /* 如果子功能不是0x80 (无响应)，则发送响应 */
    if (SubFunction != 0x80)
    {
        Response->Sid = LINSLAVE_UDS_SID_TESTER_PRESENT;
        Response->DataPtr[0] = SubFunction;
        Response->DataLength = 1;
        Response->IsNegative = FALSE;
        /** @req SWS_Lin_00163 */
        (void)LinSlave_Uds_SendPositiveResponse(Response);
    }
    return LINSLAVE_UDS_OK;
}
/** * @brief 注册默认UDS服务 */
/** @req SWS_Lin_00172 */
void LinSlave_Uds_RegisterDefaultServices(void)
{
    LinSlave_Uds_ServiceConfigType Service;
    /* 注册诊断会话控制 */
    Service.Sid = LINSLAVE_UDS_SID_DIAGNOSTIC_SESSION_CONTROL;
    Service.Handler = Uds_SessionControl;
    Service.NeedsSecurity = FALSE;
    Service.NeedsSession = FALSE;
    Service.MinSession = LINSLAVE_UDS_SESSION_DEFAULT;
    Service.MinSecurityLevel = LINSLAVE_UDS_SECURITY_LOCKED;
    /** @req SWS_Lin_00165 */
    (void)LinSlave_Uds_RegisterService(&Service);
    /* 注册ECU复位 */
    Service.Sid = LINSLAVE_UDS_SID_ECU_RESET;
    Service.Handler = Uds_EcuReset;
    Service.NeedsSecurity = FALSE;
    Service.NeedsSession = TRUE;
    Service.MinSession = LINSLAVE_UDS_SESSION_EXTENDED;
    Service.MinSecurityLevel = LINSLAVE_UDS_SECURITY_LOCKED;
    /** @req SWS_Lin_00165 */
    (void)LinSlave_Uds_RegisterService(&Service);
    /* 注册TesterPresent */
    Service.Sid = LINSLAVE_UDS_SID_TESTER_PRESENT;
    Service.Handler = Uds_TesterPresent;
    Service.NeedsSecurity = FALSE;
    Service.NeedsSession = FALSE;
    Service.MinSession = LINSLAVE_UDS_SESSION_DEFAULT;
    Service.MinSecurityLevel = LINSLAVE_UDS_SECURITY_LOCKED;
    /** @req SWS_Lin_00165 */
    (void)LinSlave_Uds_RegisterService(&Service);
}
