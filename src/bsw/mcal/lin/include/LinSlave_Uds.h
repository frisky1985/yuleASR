/**
 * @file LinSlave_Uds.h
 * @brief LIN UDS (统一诊断服务) 框架头文件
 * @version 1.0.0
 * @note 基于ISO 14229-1 (UDS) 和 ISO 17987 (LIN)
 */

#ifndef LINSLAVE_UDS_H
#define LINSLAVE_UDS_H

#include "Std_Types.h"
#include "LinSlave_Types.h"
#include "LinSlave_Tp.h"

/* UDS 版本 */
#define LINSLAVE_UDS_MAJOR_VERSION      1
#define LINSLAVE_UDS_MINOR_VERSION      0
#define LINSLAVE_UDS_PATCH_VERSION      0

/* 诊断通信参数 */
#define LINSLAVE_UDS_MAX_DSL_BUFFER     4095    /* 最大数据长度 */
#define LINSLAVE_UDS_MAX_SID_SERVICES   32      /* 最大服务数 */
#define LINSLAVE_UDS_MAX_DID            256     /* 最大DID数 */

/* ==================== UDS 服务ID (SID) ==================== */
/* 诊断和通信管理 */
#define LINSLAVE_UDS_SID_DIAGNOSTIC_SESSION_CONTROL     0x10
#define LINSLAVE_UDS_SID_ECU_RESET                      0x11
#define LINSLAVE_UDS_SID_SECURITY_ACCESS                0x27
#define LINSLAVE_UDS_SID_COMMUNICATION_CONTROL          0x28
#define LINSLAVE_UDS_SID_TESTER_PRESENT                 0x3E
#define LINSLAVE_UDS_SID_CONTROL_DTC_SETTING            0x85

/* 数据传输 */
#define LINSLAVE_UDS_SID_READ_DATA_BY_IDENTIFIER        0x22
#define LINSLAVE_UDS_SID_READ_MEMORY_BY_ADDRESS         0x23
#define LINSLAVE_UDS_SID_READ_SCALING_DATA_BY_IDENTIFIER 0x24
#define LINSLAVE_UDS_SID_READ_DATA_BY_PERIODIC_IDENTIFIER 0x2A
#define LINSLAVE_UDS_SID_DYNAMICALLY_DEFINE_DATA_IDENTIFIER 0x2C
#define LINSLAVE_UDS_SID_WRITE_DATA_BY_IDENTIFIER       0x2E
#define LINSLAVE_UDS_SID_WRITE_MEMORY_BY_ADDRESS        0x3D

/* 存储传输 */
#define LINSLAVE_UDS_SID_CLEAR_DIAGNOSTIC_INFORMATION   0x14
#define LINSLAVE_UDS_SID_READ_DTC_INFORMATION           0x19
#define LINSLAVE_UDS_SID_REQUEST_DOWNLOAD               0x34
#define LINSLAVE_UDS_SID_REQUEST_UPLOAD                 0x35
#define LINSLAVE_UDS_SID_TRANSFER_DATA                  0x36
#define LINSLAVE_UDS_SID_REQUEST_TRANSFER_EXIT          0x37

/* 输出控制 */
#define LINSLAVE_UDS_SID_INPUT_OUTPUT_CONTROL_BY_IDENTIFIER 0x2F
#define LINSLAVE_UDS_SID_ROUTINE_CONTROL                  0x31

/* ==================== UDS 负响响应码 ==================== */
#define LINSLAVE_UDS_NRC_GENERAL_REJECT                 0x10
#define LINSLAVE_UDS_NRC_SERVICE_NOT_SUPPORTED          0x11
#define LINSLAVE_UDS_NRC_SUB_FUNCTION_NOT_SUPPORTED     0x12
#define LINSLAVE_UDS_NRC_INCORRECT_MESSAGE_LENGTH       0x13
#define LINSLAVE_UDS_NRC_CONDITIONS_NOT_CORRECT         0x22
#define LINSLAVE_UDS_NRC_REQUEST_SEQUENCE_ERROR         0x24
#define LINSLAVE_UDS_NRC_REQUEST_OUT_OF_RANGE           0x31
#define LINSLAVE_UDS_NRC_SECURITY_ACCESS_DENIED         0x33
#define LINSLAVE_UDS_NRC_INVALID_KEY                    0x35
#define LINSLAVE_UDS_NRC_EXCEED_NUMBER_OF_ATTEMPTS      0x36
#define LINSLAVE_UDS_NRC_REQUIRED_TIME_DELAY            0x37
#define LINSLAVE_UDS_NRC_UPLOAD_DOWNLOAD_NOT_ACCEPTED   0x70
#define LINSLAVE_UDS_NRC_TRANSFER_DATA_SUSPENDED        0x71
#define LINSLAVE_UDS_NRC_GENERAL_PROGRAMMING_FAILURE    0x72
#define LINSLAVE_UDS_NRC_WRONG_BLOCK_SEQUENCE_COUNTER   0x73

/* ==================== 会话类型 ==================== */
typedef enum {
    LINSLAVE_UDS_SESSION_DEFAULT = 0x01,
    LINSLAVE_UDS_SESSION_PROGRAMMING = 0x02,
    LINSLAVE_UDS_SESSION_EXTENDED = 0x03,
    LINSLAVE_UDS_SESSION_SAFETY_SYSTEM = 0x04
} LinSlave_Uds_SessionType;

/* ==================== 安全级别 ==================== */
typedef enum {
    LINSLAVE_UDS_SECURITY_LOCKED = 0x00,
    LINSLAVE_UDS_SECURITY_LEVEL1 = 0x01,
    LINSLAVE_UDS_SECURITY_LEVEL2 = 0x02,
    LINSLAVE_UDS_SECURITY_LEVEL3 = 0x03
} LinSlave_Uds_SecurityLevelType;

/* ==================== 消息结构 ==================== */
/* UDS请求消息 */
typedef struct {
    uint8 Sid;                          /* 服务ID */
    uint8 SubFunction;                  /* 子功能 (可选) */
    const uint8* DataPtr;               /* 数据指针 */
    uint16 DataLength;                  /* 数据长度 */
} LinSlave_Uds_RequestType;

/* UDS响应消息 */
typedef struct {
    uint8 Sid;                          /* 服务ID + 0x40 */
    uint8* DataPtr;                     /* 数据指针 */
    uint16 DataLength;                  /* 数据长度 */
    boolean IsNegative;                 /* 是否为负响响应 */
    uint8 NegativeResponseCode;         /* 负响响应码 */
} LinSlave_Uds_ResponseType;

/* ==================== 状态类型 ==================== */
typedef enum {
    LINSLAVE_UDS_STATE_IDLE = 0,
    LINSLAVE_UDS_STATE_RECEIVING,
    LINSLAVE_UDS_STATE_PROCESSING,
    LINSLAVE_UDS_STATE_TRANSMITTING,
    LINSLAVE_UDS_STATE_ERROR
} LinSlave_Uds_StateType;

/* 操作状态 */
typedef enum {
    LINSLAVE_UDS_OK = 0,
    LINSLAVE_UDS_E_NOT_OK,
    LINSLAVE_UDS_E_BUSY,
    LINSLAVE_UDS_E_TIMEOUT,
    LINSLAVE_UDS_E_INVALID_SID,
    LINSLAVE_UDS_E_SECURITY_DENIED,
    LINSLAVE_UDS_E_SESSION_NOT_SUPPORTED
} LinSlave_Uds_StatusType;

/* ==================== 服务处理回调类型 ==================== */
/* 服务处理函数类型 */
typedef LinSlave_Uds_StatusType (*LinSlave_Uds_ServiceHandlerFuncType)(
    const LinSlave_Uds_RequestType* RequestPtr,
    LinSlave_Uds_ResponseType* ResponsePtr
);

/* 服务配置 */
typedef struct {
    uint8 Sid;                                          /* 服务ID */
    LinSlave_Uds_ServiceHandlerFuncType Handler;        /* 处理函数 */
    boolean NeedsSecurity;                              /* 需要安全访问 */
    boolean NeedsSession;                               /* 需要特定会话 */
    LinSlave_Uds_SessionType MinSession;                /* 最低会话级别 */
    LinSlave_Uds_SecurityLevelType MinSecurityLevel;    /* 最低安全级别 */
} LinSlave_Uds_ServiceConfigType;

/* UDS运行时数据 */
typedef struct {
    LinSlave_Uds_StateType State;                       /* 当前状态 */
    LinSlave_Uds_SessionType CurrentSession;            /* 当前会话 */
    LinSlave_Uds_SecurityLevelType SecurityLevel;       /* 当前安全级别 */
    uint32 S3ServerTimer;                               /* S3服务器超时计时器 */
    uint8 RequestBuffer[LINSLAVE_UDS_MAX_DSL_BUFFER];   /* 请求缓冲区 */
    uint8 ResponseBuffer[LINSLAVE_UDS_MAX_DSL_BUFFER];  /* 响应缓冲区 */
    uint16 RequestLength;                               /* 请求长度 */
    uint16 ResponseLength;                              /* 响应长度 */
    boolean TesterPresentReceived;                      /* 是否收到TesterPresent */
} LinSlave_Uds_RuntimeType;

/* ==================== API函数声明 ==================== */

/**
 * @brief 初始化UDS模块
 * @return 初始化状态
 */
LinSlave_Uds_StatusType LinSlave_Uds_Init(void);

/**
 * @brief 反初始化UDS模块
 */
void LinSlave_Uds_DeInit(void);

/**
 * @brief 处理接收到的UDS请求
 * @param DataPtr - 数据指针 (通过TP层接收完成后传入)
 * @param Length - 数据长度
 * @return 处理状态
 * @note 由TP层接收完成回调调用
 */
LinSlave_Uds_StatusType LinSlave_Uds_ProcessRequest(
    const uint8* DataPtr,
    uint16 Length
);

/**
 * @brief UDS主函数 - 处理超时和状态管理
 * @note 应在定时器中周期调用
 */
void LinSlave_Uds_MainFunction(void);

/**
 * @brief 注册服务处理函数
 * @param ServiceConfig - 服务配置
 * @return 注册状态
 */
LinSlave_Uds_StatusType LinSlave_Uds_RegisterService(
    const LinSlave_Uds_ServiceConfigType* ServiceConfig
);

/**
 * @brief 注册默认UDS服务
 * @note 注册常用的默认服务处理函数
 */
void LinSlave_Uds_RegisterDefaultServices(void);

/**
 * @brief 发送正响响应
 * @param ResponsePtr - 响应数据
 * @return 发送状态
 */
LinSlave_Uds_StatusType LinSlave_Uds_SendPositiveResponse(
    const LinSlave_Uds_ResponseType* ResponsePtr
);

/**
 * @brief 发送负响响应
 * @param Sid - 请求的服务ID
 * @param Nrc - 负响响应码
 * @return 发送状态
 */
LinSlave_Uds_StatusType LinSlave_Uds_SendNegativeResponse(
    uint8 Sid,
    uint8 Nrc
);

/**
 * @brief 获取当前会话类型
 * @return 会话类型
 */
LinSlave_Uds_SessionType LinSlave_Uds_GetSessionType(void);

/**
 * @brief 获取当前安全级别
 * @return 安全级别
 */
LinSlave_Uds_SecurityLevelType LinSlave_Uds_GetSecurityLevel(void);

/**
 * @brief 设置会话类型
 * @param SessionType - 会话类型
 */
void LinSlave_Uds_SetSessionType(LinSlave_Uds_SessionType SessionType);

/**
 * @brief 设置安全级别
 * @param SecurityLevel - 安全级别
 */
void LinSlave_Uds_SetSecurityLevel(LinSlave_Uds_SecurityLevelType SecurityLevel);

/**
 * @brief 检查诊断会话是否超时
 * @return TRUE=超时, FALSE=未超时
 */
boolean LinSlave_Uds_IsSessionTimeout(void);

/**
 * @brief 清除诊断信息
 * @param GroupOfDTC - DTC组
 * @return 操作状态
 */
LinSlave_Uds_StatusType LinSlave_Uds_ClearDiagnosticInformation(
    uint32 GroupOfDTC
);

/**
 * @brief 读取DTC信息
 * @param SubFunction - 子功能
 * @param DTCStatusMask - DTC状态掩码
 * @param ResponsePtr - 响应数据指针
 * @return 操作状态
 */
LinSlave_Uds_StatusType LinSlave_Uds_ReadDTCInformation(
    uint8 SubFunction,
    uint8 DTCStatusMask,
    LinSlave_Uds_ResponseType* ResponsePtr
);

#endif /* LINSLAVE_UDS_H */
