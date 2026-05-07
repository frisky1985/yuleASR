/**
 * @file LinMaster_Diagnostic.h
 * @brief LIN Master诊断主节点模块 (UDS客户端)
 * @version 1.0.0
 * @note 基于ISO 14229-1 (UDS) 和 ISO 17987 (LIN)
 */

#ifndef LINMASTER_DIAGNOSTIC_H
#define LINMASTER_DIAGNOSTIC_H

#include "Std_Types.h"
#include "LinMaster_Types.h"
#include "LinMaster_Tp.h"

/* 版本 */
#define LINMASTER_DIAG_MAJOR_VERSION      1
#define LINMASTER_DIAG_MINOR_VERSION      0
#define LINMASTER_DIAG_PATCH_VERSION      0

/* 诊断通信参数 */
#define LINMASTER_DIAG_MAX_BUFFER_SIZE    4095    /* 最大数据长度 */
#define LINMASTER_DIAG_DEFAULT_TIMEOUT    1000    /* 默认响应超时(ms) */
#define LINMASTER_DIAG_P2_MAX_TIMEOUT     5000    /* P2*最大超时时间(ms) */

/* LIN诊断帧ID */
#define LINMASTER_DIAG_REQ_FRAME_ID       0x3C    /* 诊断请求帧ID */
#define LINMASTER_DIAG_RESP_FRAME_ID      0x3D    /* 诊断响应帧ID */

/* ==================== UDS 服务ID (SID) ==================== */
/* 诊断和通信管理 */
#define LINMASTER_DIAG_SID_DIAGNOSTIC_SESSION_CONTROL     0x10
#define LINMASTER_DIAG_SID_ECU_RESET                      0x11
#define LINMASTER_DIAG_SID_SECURITY_ACCESS                0x27
#define LINMASTER_DIAG_SID_COMMUNICATION_CONTROL          0x28
#define LINMASTER_DIAG_SID_TESTER_PRESENT                 0x3E
#define LINMASTER_DIAG_SID_CONTROL_DTC_SETTING            0x85

/* 数据传输 */
#define LINMASTER_DIAG_SID_READ_DATA_BY_IDENTIFIER        0x22
#define LINMASTER_DIAG_SID_READ_MEMORY_BY_ADDRESS         0x23
#define LINMASTER_DIAG_SID_WRITE_DATA_BY_IDENTIFIER       0x2E
#define LINMASTER_DIAG_SID_WRITE_MEMORY_BY_ADDRESS        0x3D

/* 存储传输 */
#define LINMASTER_DIAG_SID_CLEAR_DIAGNOSTIC_INFORMATION   0x14
#define LINMASTER_DIAG_SID_READ_DTC_INFORMATION           0x19
#define LINMASTER_DIAG_SID_REQUEST_DOWNLOAD               0x34
#define LINMASTER_DIAG_SID_REQUEST_UPLOAD                 0x35
#define LINMASTER_DIAG_SID_TRANSFER_DATA                  0x36
#define LINMASTER_DIAG_SID_REQUEST_TRANSFER_EXIT          0x37

/* 输出控制 */
#define LINMASTER_DIAG_SID_INPUT_OUTPUT_CONTROL_BY_IDENTIFIER 0x2F
#define LINMASTER_DIAG_SID_ROUTINE_CONTROL                  0x31

/* ==================== UDS 负响应码 ==================== */
#define LINMASTER_DIAG_NRC_GENERAL_REJECT                 0x10
#define LINMASTER_DIAG_NRC_SERVICE_NOT_SUPPORTED          0x11
#define LINMASTER_DIAG_NRC_SUB_FUNCTION_NOT_SUPPORTED     0x12
#define LINMASTER_DIAG_NRC_INCORRECT_MESSAGE_LENGTH       0x13
#define LINMASTER_DIAG_NRC_CONDITIONS_NOT_CORRECT         0x22
#define LINMASTER_DIAG_NRC_REQUEST_SEQUENCE_ERROR         0x24
#define LINMASTER_DIAG_NRC_REQUEST_OUT_OF_RANGE           0x31
#define LINMASTER_DIAG_NRC_SECURITY_ACCESS_DENIED         0x33
#define LINMASTER_DIAG_NRC_INVALID_KEY                    0x35
#define LINMASTER_DIAG_NRC_EXCEED_NUMBER_OF_ATTEMPTS      0x36
#define LINMASTER_DIAG_NRC_REQUIRED_TIME_DELAY            0x37
#define LINMASTER_DIAG_NRC_UPLOAD_DOWNLOAD_NOT_ACCEPTED   0x70
#define LINMASTER_DIAG_NRC_TRANSFER_DATA_SUSPENDED        0x71
#define LINMASTER_DIAG_NRC_GENERAL_PROGRAMMING_FAILURE    0x72
#define LINMASTER_DIAG_NRC_WRONG_BLOCK_SEQUENCE_COUNTER   0x73

/* ==================== 会话类型 ==================== */
typedef enum {
    LINMASTER_DIAG_SESSION_DEFAULT = 0x01,
    LINMASTER_DIAG_SESSION_PROGRAMMING = 0x02,
    LINMASTER_DIAG_SESSION_EXTENDED = 0x03,
    LINMASTER_DIAG_SESSION_SAFETY_SYSTEM = 0x04
} LinMaster_Diag_SessionType;

/* ==================== 安全级别 ==================== */
typedef enum {
    LINMASTER_DIAG_SECURITY_LOCKED = 0x00,
    LINMASTER_DIAG_SECURITY_LEVEL1 = 0x01,
    LINMASTER_DIAG_SECURITY_LEVEL2 = 0x02,
    LINMASTER_DIAG_SECURITY_LEVEL3 = 0x03
} LinMaster_Diag_SecurityLevelType;

/* ==================== 诊断状态 ==================== */
typedef enum {
    LINMASTER_DIAG_STATE_IDLE = 0,              /* 空闲状态 */
    LINMASTER_DIAG_STATE_TX_REQUEST,            /* 发送请求 */
    LINMASTER_DIAG_STATE_WAIT_RESPONSE,         /* 等待响应 */
    LINMASTER_DIAG_STATE_RX_RESPONSE,           /* 接收响应 */
    LINMASTER_DIAG_STATE_COMPLETE,              /* 完成 */
    LINMASTER_DIAG_STATE_ERROR                  /* 错误 */
} LinMaster_Diag_StateType;

/* ==================== 诊断结果 ==================== */
typedef enum {
    LINMASTER_DIAG_OK = 0,                      /* 成功 */
    LINMASTER_DIAG_E_NOT_OK,                    /* 失败 */
    LINMASTER_DIAG_E_BUSY,                      /* 忙 */
    LINMASTER_DIAG_E_TIMEOUT,                   /* 超时 */
    LINMASTER_DIAG_E_INVALID_PARAM,             /* 无效参数 */
    LINMASTER_DIAG_E_BUFFER_OVERFLOW,           /* 缓冲区溢出 */
    LINMASTER_DIAG_E_NEGATIVE_RESPONSE,         /* 负响应 */
    LINMASTER_DIAG_E_NOT_INITIALIZED            /* 未初始化 */
} LinMaster_Diag_StatusType;

/* ==================== 寻址方式 ==================== */
typedef enum {
    LINMASTER_DIAG_ADDR_PHYSICAL = 0,           /* 物理寻址 */
    LINMASTER_DIAG_ADDR_FUNCTIONAL              /* 功能寻址 */
} LinMaster_Diag_AddrTypeType;

/* ==================== 数据结构 ==================== */
/* UDS请求消息 */
typedef struct {
    uint8 Sid;                                  /* 服务ID */
    uint8 SubFunction;                          /* 子功能 */
    uint8 Data[LINMASTER_DIAG_MAX_BUFFER_SIZE]; /* 数据 */
    uint16 Length;                              /* 总长度 (Sid + Data) */
    boolean IsFunctional;                       /* 是否为功能寻址 */
    uint16 TargetId;                            /* 目标ID (物理寻址时使用) */
} LinMaster_Diag_RequestType;

/* UDS响应消息 */
typedef struct {
    uint8 ResponseSid;                          /* 响应SID */
    uint8 Data[LINMASTER_DIAG_MAX_BUFFER_SIZE]; /* 数据 */
    uint16 Length;                              /* 数据长度 */
    boolean IsNegative;                         /* 是否为负响应 */
    uint8 Nrc;                                  /* 负响应码 */
} LinMaster_Diag_ResponseType;

/* 回调函数类型 */
typedef void (*LinMaster_Diag_CallbackFuncType)(
    LinMaster_Diag_StatusType Status,
    const LinMaster_Diag_ResponseType* ResponsePtr
);

/* 运行时数据结构 */
typedef struct {
    LinMaster_Diag_StateType State;             /* 当前状态 */
    LinMaster_Diag_SessionType CurrentSession;  /* 当前会话 */
    LinMaster_Diag_SecurityLevelType SecurityLevel; /* 当前安全级别 */
    uint32 TimeoutTimer;                        /* 超时计时器 */
    uint32 TimeoutValue;                        /* 超时时间配置 */
    LinMaster_Diag_RequestType PendingRequest;  /* 待发送请求 */
    LinMaster_Diag_ResponseType PendingResponse;/* 待处理响应 */
    boolean RequestPending;                     /* 有请求待处理 */
    boolean ResponseReady;                      /* 响应已就绪 */
    LinMaster_Diag_StatusType LastError;        /* 最后错误码 */
    LinMaster_Diag_CallbackFuncType Callback;   /* 回调函数 */
} LinMaster_Diag_RuntimeType;

/* ==================== API函数声明 ==================== */

/**
 * @brief 初始化诊断模块
 * @return 初始化状态
 */
LinMaster_Diag_StatusType LinMaster_Diag_Init(void);

/**
 * @brief 反初始化诊断模块
 */
void LinMaster_Diag_DeInit(void);

/**
 * @brief 发送诊断请求
 * @param RequestPtr - 请求数据指针
 * @return 发送状态
 */
LinMaster_Diag_StatusType LinMaster_Diag_SendRequest(
    const LinMaster_Diag_RequestType* RequestPtr
);

/**
 * @brief 获取诊断响应
 * @param ResponsePtr - 响应数据指针(输出)
 * @return 获取状态
 */
LinMaster_Diag_StatusType LinMaster_Diag_GetResponse(
    LinMaster_Diag_ResponseType* ResponsePtr
);

/**
 * @brief 注册回调函数
 * @param Callback - 回调函数指针
 */
void LinMaster_Diag_RegisterCallback(
    LinMaster_Diag_CallbackFuncType Callback
);

/**
 * @brief 诊断主函数 - 状态机和超时管理
 * @note 应在定时器中周期调用
 */
void LinMaster_Diag_MainFunction(void);

/**
 * @brief 检查请求是否完成
 * @return TRUE=完成, FALSE=未完成
 */
boolean LinMaster_Diag_IsRequestComplete(void);

/**
 * @brief 获取最后错误码
 * @return 错误码
 */
LinMaster_Diag_StatusType LinMaster_Diag_GetLastError(void);

/**
 * @brief 设置响应超时时间
 * @param TimeoutMs - 超时时间(毫秒)
 */
void LinMaster_Diag_SetTimeout(uint32 TimeoutMs);

/**
 * @brief 取消当前请求
 */
void LinMaster_Diag_CancelRequest(void);

/* ==================== 常用服务封装 ==================== */

/**
 * @brief 诊断会话控制 (0x10)
 * @param SessionType - 会话类型
 * @return 请求状态
 */
LinMaster_Diag_StatusType LinMaster_Diag_SessionControl(
    uint8 SessionType
);

/**
 * @brief ECU复位 (0x11)
 * @param ResetType - 复位类型
 * @return 请求状态
 */
LinMaster_Diag_StatusType LinMaster_Diag_EcuReset(
    uint8 ResetType
);

/**
 * @brief 读取数据By标识符 (0x22)
 * @param Did - 数据标识符
 * @return 请求状态
 */
LinMaster_Diag_StatusType LinMaster_Diag_ReadDataById(
    uint16 Did
);

/**
 * @brief 写入数据By标识符 (0x2E)
 * @param Did - 数据标识符
 * @param DataPtr - 数据指针
 * @param Length - 数据长度
 * @return 请求状态
 */
LinMaster_Diag_StatusType LinMaster_Diag_WriteDataById(
    uint16 Did,
    const uint8* DataPtr,
    uint8 Length
);

/**
 * @brief 安全访问 (0x27)
 * @param SubFunc - 子功能 (请求种子/发送密钥)
 * @param KeyPtr - 密钥数据指针 (发送密钥时使用)
 * @param KeyLen - 密钥长度
 * @return 请求状态
 */
LinMaster_Diag_StatusType LinMaster_Diag_SecurityAccess(
    uint8 SubFunc,
    const uint8* KeyPtr,
    uint8 KeyLen
);

/**
 * @brief 例行程序控制 (0x31)
 * @param SubFunc - 子功能 (启动/停止/请求结果)
 * @param Rid - 例行程序标识符
 * @param DataPtr - 数据指针
 * @param Length - 数据长度
 * @return 请求状态
 */
LinMaster_Diag_StatusType LinMaster_Diag_RoutineControl(
    uint8 SubFunc,
    uint16 Rid,
    const uint8* DataPtr,
    uint8 Length
);

/**
 * @brief 清除诊断信息 (0x14)
 * @param GroupOfDTC - DTC组
 * @return 请求状态
 */
LinMaster_Diag_StatusType LinMaster_Diag_ClearDiagnosticInformation(
    uint32 GroupOfDTC
);

/**
 * @brief 读取DTC信息 (0x19)
 * @param SubFunc - 子功能
 * @param DTCStatusMask - DTC状态掩码
 * @return 请求状态
 */
LinMaster_Diag_StatusType LinMaster_Diag_ReadDTCInformation(
    uint8 SubFunc,
    uint8 DTCStatusMask
);

/**
 * @brief TesterPresent (0x3E)
 * @param SubFunc - 子功能
 * @return 请求状态
 */
LinMaster_Diag_StatusType LinMaster_Diag_TesterPresent(
    uint8 SubFunc
);

/**
 * @brief 通信控制 (0x28)
 * @param SubFunc - 子功能
 * @param CommunicationType - 通信类型
 * @return 请求状态
 */
LinMaster_Diag_StatusType LinMaster_Diag_CommunicationControl(
    uint8 SubFunc,
    uint8 CommunicationType
);

#endif /* LINMASTER_DIAGNOSTIC_H */
