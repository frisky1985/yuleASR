/*==================================================================================================
* Project              : YuleTech AutoSAR BSW
* Platform             : NXP i.MX8M Mini
* Dependencies         : ...
*
* Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
* All rights reserved.
*
* SPDX-License-Identifier: MIT
*
*================================================================================================*/

/**
 * @file LinMaster_Diagnostic.c
 * @brief LIN Master诊断主节点模块 (UDS客户端) 实现
 * @version 1.0.0
 */

#include "LinMaster_Diagnostic.h"
#include "LinMaster_Tp.h"
#include <string.h>

/* 内部状态 */
static LinMaster_Diag_RuntimeType DiagRuntime;
static boolean DiagInitialized = FALSE;
static uint8 TxBuffer[LINMASTER_DIAG_MAX_BUFFER_SIZE + 10]; /* 发送缓冲区 */
static uint8 RxBuffer[LINMASTER_DIAG_MAX_BUFFER_SIZE + 10]; /* 接收缓冲区 */
static uint16 TxLength = 0;
static uint16 RxLength = 0;
static boolean RxDataPending = FALSE;  /* 有待处理的接收数据 */

/* 内部回调函数 */
static void LinMaster_Diag_TpRxCallback(uint8 ChannelId, const uint8* DataPtr, uint16 Length);
static void LinMaster_Diag_ResetState(void);
static LinMaster_Diag_StatusType LinMaster_Diag_ProcessResponse(const uint8* DataPtr, uint16 Length);
static void LinMaster_Diag_BuildRequest(const LinMaster_Diag_RequestType* RequestPtr);

/**
 * @brief 初始化诊断模块
 */
LinMaster_Diag_StatusType LinMaster_Diag_Init(void)
{
    (void)memset(&DiagRuntime, 0, sizeof(LinMaster_Diag_RuntimeType));
    (void)memset(TxBuffer, 0, sizeof(TxBuffer));
    (void)memset(RxBuffer, 0, sizeof(RxBuffer));
    
    DiagRuntime.State = LINMASTER_DIAG_STATE_IDLE;
    DiagRuntime.CurrentSession = LINMASTER_DIAG_SESSION_DEFAULT;
    DiagRuntime.SecurityLevel = LINMASTER_DIAG_SECURITY_LOCKED;
    DiagRuntime.TimeoutValue = LINMASTER_DIAG_DEFAULT_TIMEOUT;
    DiagRuntime.TimeoutTimer = 0;
    DiagRuntime.RequestPending = FALSE;
    DiagRuntime.ResponseReady = FALSE;
    DiagRuntime.LastError = LINMASTER_DIAG_OK;
    DiagRuntime.Callback = NULL;
    
    TxLength = 0;
    RxLength = 0;
    RxDataPending = FALSE;
    DiagInitialized = TRUE;
    
    /* 注册TP层接收回调 */
    LinMaster_Tp_RegisterRxCallback(LinMaster_Diag_TpRxCallback);
    
    return LINMASTER_DIAG_OK;
}

/**
 * @brief 反初始化诊断模块
 */
void LinMaster_Diag_DeInit(void)
{
    DiagInitialized = FALSE;
}

/**
 * @brief TP层接收回调函数
 */
static void LinMaster_Diag_TpRxCallback(uint8 ChannelId, const uint8* DataPtr, uint16 Length)
{
    (void)ChannelId;
    
    if (DataPtr == NULL || Length == 0 || Length > LINMASTER_DIAG_MAX_BUFFER_SIZE) {
        return;
    }
    
    /* 复制接收数据 */
    (void)memcpy(RxBuffer, DataPtr, Length);
    RxLength = Length;
    RxDataPending = TRUE;
}

/**
 * @brief 重置状态机
 */
static void LinMaster_Diag_ResetState(void)
{
    DiagRuntime.State = LINMASTER_DIAG_STATE_IDLE;
    DiagRuntime.RequestPending = FALSE;
    DiagRuntime.ResponseReady = FALSE;
    DiagRuntime.TimeoutTimer = 0;
    TxLength = 0;
    RxLength = 0;
}

/**
 * @brief 构建诊断请求报文
 */
static void LinMaster_Diag_BuildRequest(const LinMaster_Diag_RequestType* RequestPtr)
{
    uint16 i;
    
    if (RequestPtr == NULL) {
        return;
    }
    
    /* SID */
    TxBuffer[0] = RequestPtr->Sid;
    TxLength = 1;
    
    /* 子功能 */
    TxBuffer[1] = RequestPtr->SubFunction;
    TxLength++;
    
    /* 数据 */
    if (RequestPtr->Length > 0 && RequestPtr->Length <= LINMASTER_DIAG_MAX_BUFFER_SIZE) {
        for (i = 0; i < RequestPtr->Length; i++) {
            TxBuffer[TxLength + i] = RequestPtr->Data[i];
        }
        TxLength += RequestPtr->Length;
    }
    
    /* 保存请求信息 */
    (void)memcpy(&DiagRuntime.PendingRequest, RequestPtr, sizeof(LinMaster_Diag_RequestType));
}

/**
 * @brief 处理响应报文
 */
static LinMaster_Diag_StatusType LinMaster_Diag_ProcessResponse(const uint8* DataPtr, uint16 Length)
{
    if (DataPtr == NULL || Length == 0) {
        return LINMASTER_DIAG_E_NOT_OK;
    }
    
    (void)memset(&DiagRuntime.PendingResponse, 0, sizeof(LinMaster_Diag_ResponseType));
    
    /* 检查是否为负响应 (0x7F) */
    if (DataPtr[0] == 0x7F) {
        if (Length >= 3) {
            DiagRuntime.PendingResponse.IsNegative = TRUE;
            DiagRuntime.PendingResponse.ResponseSid = DataPtr[1];
            DiagRuntime.PendingResponse.Nrc = DataPtr[2];
            DiagRuntime.PendingResponse.Length = 0;
            DiagRuntime.LastError = LINMASTER_DIAG_E_NEGATIVE_RESPONSE;
            return LINMASTER_DIAG_E_NEGATIVE_RESPONSE;
        }
        return LINMASTER_DIAG_E_NOT_OK;
    }
    
    /* 正响应 */
    DiagRuntime.PendingResponse.IsNegative = FALSE;
    DiagRuntime.PendingResponse.ResponseSid = DataPtr[0];
    DiagRuntime.PendingResponse.Nrc = 0;
    
    if (Length > 1) {
        uint16 dataLen = Length - 1;
        if (dataLen > LINMASTER_DIAG_MAX_BUFFER_SIZE) {
            dataLen = LINMASTER_DIAG_MAX_BUFFER_SIZE;
        }
        (void)memcpy(DiagRuntime.PendingResponse.Data, &DataPtr[1], dataLen);
        DiagRuntime.PendingResponse.Length = dataLen;
    } else {
        DiagRuntime.PendingResponse.Length = 0;
    }
    
    return LINMASTER_DIAG_OK;
}

/**
 * @brief 发送诊断请求
 */
LinMaster_Diag_StatusType LinMaster_Diag_SendRequest(
    const LinMaster_Diag_RequestType* RequestPtr
)
{
    LinMaster_Diag_StatusType status;
    
    if (!DiagInitialized) {
        return LINMASTER_DIAG_E_NOT_INITIALIZED;
    }
    
    if (RequestPtr == NULL) {
        return LINMASTER_DIAG_E_INVALID_PARAM;
    }
    
    /* 检查是否忙 */
    if (DiagRuntime.State != LINMASTER_DIAG_STATE_IDLE) {
        return LINMASTER_DIAG_E_BUSY;
    }
    
    /* 检查参数 */
    if (RequestPtr->Length > LINMASTER_DIAG_MAX_BUFFER_SIZE) {
        return LINMASTER_DIAG_E_INVALID_PARAM;
    }
    
    /* 构建请求 */
    LinMaster_Diag_BuildRequest(RequestPtr);
    
    if (TxLength == 0) {
        return LINMASTER_DIAG_E_NOT_OK;
    }
    
    /* 设置状态为发送请求 */
    DiagRuntime.State = LINMASTER_DIAG_STATE_TX_REQUEST;
    DiagRuntime.RequestPending = TRUE;
    DiagRuntime.ResponseReady = FALSE;
    DiagRuntime.LastError = LINMASTER_DIAG_OK;
    
/* 通过TP层发送 */
            /* 注意: LIN诊断请求帧ID由TP层管理 (0x3C) */
    
    return LINMASTER_DIAG_OK;
}

/**
 * @brief 获取诊断响应
 */
LinMaster_Diag_StatusType LinMaster_Diag_GetResponse(
    LinMaster_Diag_ResponseType* ResponsePtr
)
{
    if (!DiagInitialized) {
        return LINMASTER_DIAG_E_NOT_INITIALIZED;
    }
    
    if (ResponsePtr == NULL) {
        return LINMASTER_DIAG_E_INVALID_PARAM;
    }
    
    if (!DiagRuntime.ResponseReady) {
        return LINMASTER_DIAG_E_BUSY;
    }
    
    /* 复制响应数据 */
    (void)memcpy(ResponsePtr, &DiagRuntime.PendingResponse, sizeof(LinMaster_Diag_ResponseType));
    
    /* 重置状态 */
    DiagRuntime.State = LINMASTER_DIAG_STATE_IDLE;
    DiagRuntime.ResponseReady = FALSE;
    DiagRuntime.RequestPending = FALSE;
    
    if (ResponsePtr->IsNegative) {
        return LINMASTER_DIAG_E_NEGATIVE_RESPONSE;
    }
    
    return LINMASTER_DIAG_OK;
}

/**
 * @brief 注册回调函数
 */
void LinMaster_Diag_RegisterCallback(
    LinMaster_Diag_CallbackFuncType Callback
)
{
    if (!DiagInitialized) {
        return;
    }
    
    DiagRuntime.Callback = Callback;
}

/**
 * @brief 检查请求是否完成
 */
boolean LinMaster_Diag_IsRequestComplete(void)
{
    if (!DiagInitialized) {
        return TRUE;
    }
    
    return (DiagRuntime.State == LINMASTER_DIAG_STATE_COMPLETE) ||
           (DiagRuntime.State == LINMASTER_DIAG_STATE_ERROR) ||
           (DiagRuntime.State == LINMASTER_DIAG_STATE_IDLE && !DiagRuntime.RequestPending);
}

/**
 * @brief 获取最后错误码
 */
LinMaster_Diag_StatusType LinMaster_Diag_GetLastError(void)
{
    return DiagRuntime.LastError;
}

/**
 * @brief 设置响应超时时间
 */
void LinMaster_Diag_SetTimeout(uint32 TimeoutMs)
{
    if (!DiagInitialized) {
        return;
    }
    
    if (TimeoutMs > 0 && TimeoutMs <= LINMASTER_DIAG_P2_MAX_TIMEOUT) {
        DiagRuntime.TimeoutValue = TimeoutMs;
    }
}

/**
 * @brief 取消当前请求
 */
void LinMaster_Diag_CancelRequest(void)
{
    if (!DiagInitialized) {
        return;
    }
    
    LinMaster_Diag_ResetState();
    DiagRuntime.LastError = LINMASTER_DIAG_E_NOT_OK;
}

/**
 * @brief 诊断主函数 - 状态机管理
 */
void LinMaster_Diag_MainFunction(void)
{
    LinMaster_Diag_StatusType status;
    
    if (!DiagInitialized) {
        return;
    }
    
    switch (DiagRuntime.State) {
        case LINMASTER_DIAG_STATE_IDLE:
            /* 空闲状态，检查是否有待发送请求 */
            if (DiagRuntime.RequestPending) {
                DiagRuntime.State = LINMASTER_DIAG_STATE_TX_REQUEST;
            }
            break;
            
        case LINMASTER_DIAG_STATE_TX_REQUEST:
            /* 发送诊断请求 (0x3C) */
            status = (LinMaster_Diag_StatusType)LinMaster_Tp_Transmit(TxLength, TxBuffer);
            
            if (status == LINMASTER_DIAG_OK) {
                /* 发送成功，切换到等待响应状态 */
                DiagRuntime.State = LINMASTER_DIAG_STATE_WAIT_RESPONSE;
                DiagRuntime.TimeoutTimer = 0;
            } else if (status != LINMASTER_DIAG_E_BUSY) {
                /* 发送失败 */
                DiagRuntime.LastError = LINMASTER_DIAG_E_NOT_OK;
                DiagRuntime.State = LINMASTER_DIAG_STATE_ERROR;
                
                if (DiagRuntime.Callback != NULL) {
                    DiagRuntime.Callback(LINMASTER_DIAG_E_NOT_OK, NULL);
                }
            }
            break;
            
        case LINMASTER_DIAG_STATE_WAIT_RESPONSE:
            /* 等待响应，检查超时 */
            DiagRuntime.TimeoutTimer++;
            
            if (DiagRuntime.TimeoutTimer >= DiagRuntime.TimeoutValue) {
                /* 超时 */
                DiagRuntime.LastError = LINMASTER_DIAG_E_TIMEOUT;
                DiagRuntime.State = LINMASTER_DIAG_STATE_ERROR;
                RxDataPending = FALSE;
                
                if (DiagRuntime.Callback != NULL) {
                    DiagRuntime.Callback(LINMASTER_DIAG_E_TIMEOUT, NULL);
                }
            } else if (RxDataPending) {
                /* 收到响应 */
                RxDataPending = FALSE;
                DiagRuntime.State = LINMASTER_DIAG_STATE_RX_RESPONSE;
            }
            break;
            
        case LINMASTER_DIAG_STATE_RX_RESPONSE:
            /* 处理响应 */
            status = LinMaster_Diag_ProcessResponse(RxBuffer, RxLength);
            
            if (status == LINMASTER_DIAG_OK) {
                DiagRuntime.State = LINMASTER_DIAG_STATE_COMPLETE;
                DiagRuntime.ResponseReady = TRUE;
                DiagRuntime.LastError = LINMASTER_DIAG_OK;
            } else {
                DiagRuntime.State = LINMASTER_DIAG_STATE_ERROR;
                DiagRuntime.LastError = status;
            }
            
            /* 触发回调 */
            if (DiagRuntime.Callback != NULL) {
                DiagRuntime.Callback(status, &DiagRuntime.PendingResponse);
            }
            break;
            
        case LINMASTER_DIAG_STATE_COMPLETE:
            /* 请求完成，等待用户获取响应 */
            break;
            
        case LINMASTER_DIAG_STATE_ERROR:
            /* 错误状态，等待复位 */
            break;
            
        default:
            break;
    }
}

/* ==================== 常用服务封装 ==================== */

/**
 * @brief 诊断会话控制 (0x10)
 */
LinMaster_Diag_StatusType LinMaster_Diag_SessionControl(
    uint8 SessionType
)
{
    LinMaster_Diag_RequestType request;
    
    if (!DiagInitialized) {
        return LINMASTER_DIAG_E_NOT_INITIALIZED;
    }
    
    (void)memset(&request, 0, sizeof(request));
    request.Sid = LINMASTER_DIAG_SID_DIAGNOSTIC_SESSION_CONTROL;
    request.SubFunction = SessionType & 0x7F;
    request.Length = 0;
    request.IsFunctional = FALSE;
    
    return LinMaster_Diag_SendRequest(&request);
}

/**
 * @brief ECU复位 (0x11)
 */
LinMaster_Diag_StatusType LinMaster_Diag_EcuReset(
    uint8 ResetType
)
{
    LinMaster_Diag_RequestType request;
    
    if (!DiagInitialized) {
        return LINMASTER_DIAG_E_NOT_INITIALIZED;
    }
    
    (void)memset(&request, 0, sizeof(request));
    request.Sid = LINMASTER_DIAG_SID_ECU_RESET;
    request.SubFunction = ResetType & 0x7F;
    request.Length = 0;
    request.IsFunctional = FALSE;
    
    return LinMaster_Diag_SendRequest(&request);
}

/**
 * @brief 读取数据By标识符 (0x22)
 */
LinMaster_Diag_StatusType LinMaster_Diag_ReadDataById(
    uint16 Did
)
{
    LinMaster_Diag_RequestType request;
    
    if (!DiagInitialized) {
        return LINMASTER_DIAG_E_NOT_INITIALIZED;
    }
    
    (void)memset(&request, 0, sizeof(request));
    request.Sid = LINMASTER_DIAG_SID_READ_DATA_BY_IDENTIFIER;
    request.SubFunction = 0; /* 此服务没有子功能 */
    
    /* DID是高低字节 */
    request.Data[0] = (uint8)(Did >> 8);
    request.Data[1] = (uint8)(Did & 0xFF);
    request.Length = 2;
    request.IsFunctional = FALSE;
    
    return LinMaster_Diag_SendRequest(&request);
}

/**
 * @brief 写入数据By标识符 (0x2E)
 */
LinMaster_Diag_StatusType LinMaster_Diag_WriteDataById(
    uint16 Did,
    const uint8* DataPtr,
    uint8 Length
)
{
    LinMaster_Diag_RequestType request;
    uint8 i;
    
    if (!DiagInitialized) {
        return LINMASTER_DIAG_E_NOT_INITIALIZED;
    }
    
    if (DataPtr == NULL || Length == 0 || Length > (LINMASTER_DIAG_MAX_BUFFER_SIZE - 2)) {
        return LINMASTER_DIAG_E_INVALID_PARAM;
    }
    
    (void)memset(&request, 0, sizeof(request));
    request.Sid = LINMASTER_DIAG_SID_WRITE_DATA_BY_IDENTIFIER;
    request.SubFunction = 0;
    
    /* DID是高低字节 */
    request.Data[0] = (uint8)(Did >> 8);
    request.Data[1] = (uint8)(Did & 0xFF);
    
    /* 数据 */
    for (i = 0; i < Length; i++) {
        request.Data[2 + i] = DataPtr[i];
    }
    request.Length = 2 + Length;
    request.IsFunctional = FALSE;
    
    return LinMaster_Diag_SendRequest(&request);
}

/**
 * @brief 安全访问 (0x27)
 */
LinMaster_Diag_StatusType LinMaster_Diag_SecurityAccess(
    uint8 SubFunc,
    const uint8* KeyPtr,
    uint8 KeyLen
)
{
    LinMaster_Diag_RequestType request;
    uint8 i;
    
    if (!DiagInitialized) {
        return LINMASTER_DIAG_E_NOT_INITIALIZED;
    }
    
    (void)memset(&request, 0, sizeof(request));
    request.Sid = LINMASTER_DIAG_SID_SECURITY_ACCESS;
    request.SubFunction = SubFunc & 0x7F;
    
    /* 如果是发送密钥，添加密钥数据 */
    if (KeyPtr != NULL && KeyLen > 0 && KeyLen <= LINMASTER_DIAG_MAX_BUFFER_SIZE) {
        for (i = 0; i < KeyLen; i++) {
            request.Data[i] = KeyPtr[i];
        }
        request.Length = KeyLen;
    } else {
        request.Length = 0;
    }
    
    request.IsFunctional = FALSE;
    
    return LinMaster_Diag_SendRequest(&request);
}

/**
 * @brief 例行程序控制 (0x31)
 */
LinMaster_Diag_StatusType LinMaster_Diag_RoutineControl(
    uint8 SubFunc,
    uint16 Rid,
    const uint8* DataPtr,
    uint8 Length
)
{
    LinMaster_Diag_RequestType request;
    uint8 i;
    
    if (!DiagInitialized) {
        return LINMASTER_DIAG_E_NOT_INITIALIZED;
    }
    
    if (Length > (LINMASTER_DIAG_MAX_BUFFER_SIZE - 2)) {
        return LINMASTER_DIAG_E_INVALID_PARAM;
    }
    
    (void)memset(&request, 0, sizeof(request));
    request.Sid = LINMASTER_DIAG_SID_ROUTINE_CONTROL;
    request.SubFunction = SubFunc & 0x7F;
    
    /* RID是高低字节 */
    request.Data[0] = (uint8)(Rid >> 8);
    request.Data[1] = (uint8)(Rid & 0xFF);
    
    /* 可选数据 */
    if (DataPtr != NULL && Length > 0) {
        for (i = 0; i < Length; i++) {
            request.Data[2 + i] = DataPtr[i];
        }
        request.Length = 2 + Length;
    } else {
        request.Length = 2;
    }
    
    request.IsFunctional = FALSE;
    
    return LinMaster_Diag_SendRequest(&request);
}

/**
 * @brief 清除诊断信息 (0x14)
 */
LinMaster_Diag_StatusType LinMaster_Diag_ClearDiagnosticInformation(
    uint32 GroupOfDTC
)
{
    LinMaster_Diag_RequestType request;
    
    if (!DiagInitialized) {
        return LINMASTER_DIAG_E_NOT_INITIALIZED;
    }
    
    (void)memset(&request, 0, sizeof(request));
    request.Sid = LINMASTER_DIAG_SID_CLEAR_DIAGNOSTIC_INFORMATION;
    request.SubFunction = 0;
    
    /* GroupOfDTC是3字节 */
    request.Data[0] = (uint8)((GroupOfDTC >> 16) & 0xFF);
    request.Data[1] = (uint8)((GroupOfDTC >> 8) & 0xFF);
    request.Data[2] = (uint8)(GroupOfDTC & 0xFF);
    request.Length = 3;
    request.IsFunctional = FALSE;
    
    return LinMaster_Diag_SendRequest(&request);
}

/**
 * @brief 读取DTC信息 (0x19)
 */
LinMaster_Diag_StatusType LinMaster_Diag_ReadDTCInformation(
    uint8 SubFunc,
    uint8 DTCStatusMask
)
{
    LinMaster_Diag_RequestType request;
    
    if (!DiagInitialized) {
        return LINMASTER_DIAG_E_NOT_INITIALIZED;
    }
    
    (void)memset(&request, 0, sizeof(request));
    request.Sid = LINMASTER_DIAG_SID_READ_DTC_INFORMATION;
    request.SubFunction = SubFunc & 0x7F;
    
    /* 根据子功能可能需要DTCStatusMask */
    request.Data[0] = DTCStatusMask;
    request.Length = 1;
    request.IsFunctional = FALSE;
    
    return LinMaster_Diag_SendRequest(&request);
}

/**
 * @brief TesterPresent (0x3E)
 */
LinMaster_Diag_StatusType LinMaster_Diag_TesterPresent(
    uint8 SubFunc
)
{
    LinMaster_Diag_RequestType request;
    
    if (!DiagInitialized) {
        return LINMASTER_DIAG_E_NOT_INITIALIZED;
    }
    
    (void)memset(&request, 0, sizeof(request));
    request.Sid = LINMASTER_DIAG_SID_TESTER_PRESENT;
    request.SubFunction = SubFunc & 0x7F;
    request.Length = 0;
    request.IsFunctional = FALSE;
    
    return LinMaster_Diag_SendRequest(&request);
}

/**
 * @brief 通信控制 (0x28)
 */
LinMaster_Diag_StatusType LinMaster_Diag_CommunicationControl(
    uint8 SubFunc,
    uint8 CommunicationType
)
{
    LinMaster_Diag_RequestType request;
    
    if (!DiagInitialized) {
        return LINMASTER_DIAG_E_NOT_INITIALIZED;
    }
    
    (void)memset(&request, 0, sizeof(request));
    request.Sid = LINMASTER_DIAG_SID_COMMUNICATION_CONTROL;
    request.SubFunction = SubFunc & 0x7F;
    
    /* 通信类型 */
    request.Data[0] = CommunicationType;
    request.Length = 1;
    request.IsFunctional = FALSE;
    
    return LinMaster_Diag_SendRequest(&request);
}
