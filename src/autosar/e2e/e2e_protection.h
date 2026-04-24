/******************************************************************************
 * @file    e2e_protection.h
 * @brief   AUTOSAR E2E (End-to-End) Protection Header
 *
 * AUTOSAR R22-11 compliant
 * ASIL-D Safety Level
 *
 * Supports E2E Profiles:
 *   - Profile 1: CRC8
 *   - Profile 2: CRC8 + Counter
 *   - Profile 4: CRC32
 *   - Profile 5: CRC16
 *   - Profile 6: CRC16 + Counter
 *   - Profile 7: CRC32 + Counter
 *   - Profile 11: Dynamic CRC8 + Sequence
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/
#ifndef E2E_PROTECTION_H
#define E2E_PROTECTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "autosar_types.h"
#include "autosar_errors.h"

/* E2E版本 */
#define E2E_MAJOR_VERSION               22
#define E2E_MINOR_VERSION               11
#define E2E_PATCH_VERSION               0

/* E2E Profile IDs */
#define E2E_PROFILE_01                  0x01U   /* CRC8 */
#define E2E_PROFILE_02                  0x02U   /* CRC8 + Counter */
#define E2E_PROFILE_04                  0x04U   /* CRC32 */
#define E2E_PROFILE_05                  0x05U   /* CRC16 */
#define E2E_PROFILE_06                  0x06U   /* CRC16 + Counter */
#define E2E_PROFILE_07                  0x07U   /* CRC32 + Counter */
#define E2E_PROFILE_11                  0x0BU   /* Dynamic CRC8 + Sequence */
#define E2E_PROFILE_22                  0x16U   /* Dynamic CRC16 + Sequence */

/* E2E头部大小 */
#define E2E_P01_HEADER_SIZE             1U      /* CRC8 */
#define E2E_P02_HEADER_SIZE             2U      /* CRC8 + Counter */
#define E2E_P04_HEADER_SIZE             4U      /* CRC32 */
#define E2E_P05_HEADER_SIZE             2U      /* CRC16 */
#define E2E_P06_HEADER_SIZE             4U      /* CRC16 + Counter + 2 reserved */
#define E2E_P07_HEADER_SIZE             8U      /* CRC32 + Counter + 4 reserved */
#define E2E_P11_HEADER_SIZE             3U      /* Dynamic CRC8 + Seq */
#define E2E_MAX_OVERHEAD                16U

/* 计数器最大值 */
#define E2E_COUNTER_MAX_4BIT            15U
#define E2E_COUNTER_MAX_8BIT            255U

/* E2E错误状态 */
#define E2E_ERROR_NONE                  0x0000U
#define E2E_ERROR_CRC                   0x0001U
#define E2E_ERROR_COUNTER               0x0002U
#define E2E_ERROR_SEQUENCE              0x0004U
#define E2E_ERROR_TIMEOUT               0x0008U
#define E2E_ERROR_INIT                  0x0010U
#define E2E_ERROR_LENGTH                0x0020U

/* E2E检查结果状态 */
typedef enum {
    E2E_P_OK = 0,           /* 正常 */
    E2E_P_REPEATED,         /* 重复消息 */
    E2E_P_WRONGSEQUENCE,    /* 序列错误 */
    E2E_P_ERROR,            /* 严重错误 */
    E2E_P_NOTAVAILABLE,     /* 数据不可用 */
    E2E_P_NONEWDATA         /* 无新数据 */
} E2E_PCheckStatusType;

/* CRC多项式类型 */
typedef enum {
    E2E_CRC_SAEJ1850 = 0,   /* SAE J1850 polynomial: 0x1D */
    E2E_CRC_CCITT,          /* CCITT polynomial: 0x1021 */
    E2E_CRC_CRC32           /* CRC32 polynomial: 0x04C11DB7 */
} E2E_CrcPolynomialType;

/******************************************************************************
 * Data Types
 ******************************************************************************/

/* E2E Profile 1 配置 */
typedef struct {
    uint16_t dataId;            /* Data ID */
    uint16_t dataIdMode;        /* Data ID模式 */
    uint16_t dataLength;        /* 数据长度 */
    uint8_t  crcOffset;         /* CRC偏移 */
    uint8_t  dataIdNibbleOffset;/* Data ID nibble偏移 */
} E2E_P01ConfigType;

/* E2E Profile 1 检查状态 */
typedef struct {
    uint8_t counter;            /* 当前计数器值 */
    E2E_PCheckStatusType status;/* 检查状态 */
} E2E_P01CheckStateType;

/* E2E Profile 2 配置 */
typedef struct {
    uint16_t dataId;            /* Data ID */
    uint16_t dataLength;        /* 数据长度 */
    uint8_t  crcOffset;         /* CRC偏移 */
    uint8_t  counterOffset;     /* 计数器偏移 */
} E2E_P02ConfigType;

/* E2E Profile 2 检查状态 */
typedef struct {
    uint8_t counter;            /* 当前计数器值 */
    uint8_t maxDeltaCounter;    /* 最大计数器差值 */
    E2E_PCheckStatusType status; /* 检查状态 */
    bool newDataAvailable;      /* 新数据可用 */
    bool synced;                /* 已同步 */
} E2E_P02CheckStateType;

/* E2E Profile 4 配置 */
typedef struct {
    uint32_t dataId;            /* Data ID (32-bit) */
    uint16_t dataLength;        /* 数据长度 */
    uint8_t  crcOffset;         /* CRC偏移 */
    uint8_t  dataIdMode;        /* Data ID模式 */
} E2E_P04ConfigType;

/* E2E Profile 4 检查状态 */
typedef struct {
    uint8_t counter;            /* 计数器（从CRC提取） */
    E2E_PCheckStatusType status;/* 检查状态 */
} E2E_P04CheckStateType;

/* E2E Profile 5 配置 */
typedef struct {
    uint16_t dataId;            /* Data ID */
    uint16_t dataLength;        /* 数据长度 */
    uint8_t  crcOffset;         /* CRC偏移 */
    uint8_t  dataIdMode;        /* Data ID模式 */
} E2E_P05ConfigType;

/* E2E Profile 5 检查状态 */
typedef struct {
    uint8_t counter;            /* 计数器 */
    E2E_PCheckStatusType status;/* 检查状态 */
} E2E_P05CheckStateType;

/* E2E Profile 6 配置 */
typedef struct {
    uint16_t dataId;            /* Data ID */
    uint16_t dataLength;        /* 数据长度 */
    uint8_t  crcOffset;         /* CRC偏移 */
    uint8_t  counterOffset;     /* 计数器偏移 */
} E2E_P06ConfigType;

/* E2E Profile 6 检查状态 */
typedef struct {
    uint8_t counter;
    uint8_t maxDeltaCounter;
    E2E_PCheckStatusType status;
    bool synced;
} E2E_P06CheckStateType;

/* E2E Profile 7 配置 */
typedef struct {
    uint32_t dataId;            /* Data ID (32-bit) */
    uint16_t dataLength;        /* 数据长度 */
    uint8_t  crcOffset;         /* CRC偏移 */
    uint8_t  counterOffset;     /* 计数器偏移 */
} E2E_P07ConfigType;

/* E2E Profile 7 检查状态 */
typedef struct {
    uint8_t counter;
    uint8_t maxDeltaCounter;
    E2E_PCheckStatusType status;
    bool synced;
} E2E_P07CheckStateType;

/* E2E Profile 11 配置（动态） */
typedef struct {
    uint16_t dataId;
    uint16_t dataLength;
    uint16_t maxDataLength;     /* 最大数据长度 */
    uint16_t minDataLength;     /* 最小数据长度 */
    uint8_t  crcOffset;
    uint8_t  counterOffset;
    uint8_t  dataIdOffset;      /* Data ID偏移 */
} E2E_P11ConfigType;

/* E2E Profile 11 检查状态 */
typedef struct {
    uint8_t counter;
    uint8_t maxDeltaCounter;
    uint16_t lastDataId;
    E2E_PCheckStatusType status;
    bool synced;
} E2E_P11CheckStateType;

/* 通用E2E上下文 */
typedef struct {
    uint8_t profile;            /* E2E Profile ID */
    union {
        E2E_P01ConfigType p01;
        E2E_P02ConfigType p02;
        E2E_P04ConfigType p04;
        E2E_P05ConfigType p05;
        E2E_P06ConfigType p06;
        E2E_P07ConfigType p07;
        E2E_P11ConfigType p11;
    } config;
    union {
        E2E_P01CheckStateType p01;
        E2E_P02CheckStateType p02;
        E2E_P04CheckStateType p04;
        E2E_P05CheckStateType p05;
        E2E_P06CheckStateType p06;
        E2E_P07CheckStateType p07;
        E2E_P11CheckStateType p11;
    } state;
    uint32_t initMagic;
    bool initialized;
} E2E_ContextType;

/* E2E检查结果 */
typedef struct {
    E2E_PCheckStatusType status;
    uint16_t errorFlags;
    uint8_t counter;
    uint32_t timestamp;
} E2E_CheckResultType;

/******************************************************************************
 * Function Prototypes - Initialization
 ******************************************************************************/

/**
 * @brief 初始化E2E模块
 */
Std_ReturnType E2E_Init(void);

/**
 * @brief 反初始化E2E模块
 */
Std_ReturnType E2E_Deinit(void);

/**
 * @brief 检查E2E模块是否已初始化
 */
bool E2E_IsInitialized(void);

/**
 * @brief 初始化E2E上下文
 */
Std_ReturnType E2E_InitContext(
    E2E_ContextType* context,
    uint8_t profile);

/**
 * @brief 反初始化E2E上下文
 */
Std_ReturnType E2E_DeinitContext(E2E_ContextType* context);

/******************************************************************************
 * Function Prototypes - Profile 1 (CRC8)
 ******************************************************************************/

/**
 * @brief Profile 1 保护数据
 */
Std_ReturnType E2E_P01_Protect(
    E2E_ContextType* context,
    void* data,
    uint32_t* length);

/**
 * @brief Profile 1 校验数据
 */
Std_ReturnType E2E_P01_Check(
    E2E_ContextType* context,
    const void* data,
    uint32_t length,
    uint16_t* status);

/******************************************************************************
 * Function Prototypes - Profile 2 (CRC8 + Counter)
 ******************************************************************************/

/**
 * @brief Profile 2 保护数据
 */
Std_ReturnType E2E_P02_Protect(
    E2E_ContextType* context,
    void* data,
    uint32_t* length);

/**
 * @brief Profile 2 校验数据
 */
Std_ReturnType E2E_P02_Check(
    E2E_ContextType* context,
    const void* data,
    uint32_t length,
    uint16_t* status);

/******************************************************************************
 * Function Prototypes - Profile 4 (CRC32)
 ******************************************************************************/

/**
 * @brief Profile 4 保护数据
 */
Std_ReturnType E2E_P04_Protect(
    E2E_ContextType* context,
    void* data,
    uint32_t* length);

/**
 * @brief Profile 4 校验数据
 */
Std_ReturnType E2E_P04_Check(
    E2E_ContextType* context,
    const void* data,
    uint32_t length,
    uint16_t* status);

/******************************************************************************
 * Function Prototypes - Profile 5 (CRC16)
 ******************************************************************************/

/**
 * @brief Profile 5 保护数据
 */
Std_ReturnType E2E_P05_Protect(
    E2E_ContextType* context,
    void* data,
    uint32_t* length);

/**
 * @brief Profile 5 校验数据
 */
Std_ReturnType E2E_P05_Check(
    E2E_ContextType* context,
    const void* data,
    uint32_t length,
    uint16_t* status);

/******************************************************************************
 * Function Prototypes - Profile 6 (CRC16 + Counter)
 ******************************************************************************/

/**
 * @brief Profile 6 保护数据
 */
Std_ReturnType E2E_P06_Protect(
    E2E_ContextType* context,
    void* data,
    uint32_t* length);

/**
 * @brief Profile 6 校验数据
 */
Std_ReturnType E2E_P06_Check(
    E2E_ContextType* context,
    const void* data,
    uint32_t length,
    uint16_t* status);

/******************************************************************************
 * Function Prototypes - Profile 7 (CRC32 + Counter)
 ******************************************************************************/

/**
 * @brief Profile 7 保护数据
 */
Std_ReturnType E2E_P07_Protect(
    E2E_ContextType* context,
    void* data,
    uint32_t* length);

/**
 * @brief Profile 7 校验数据
 */
Std_ReturnType E2E_P07_Check(
    E2E_ContextType* context,
    const void* data,
    uint32_t length,
    uint16_t* status);

/******************************************************************************
 * Function Prototypes - Profile 11 (Dynamic CRC8 + Sequence)
 ******************************************************************************/

/**
 * @brief Profile 11 保护数据
 */
Std_ReturnType E2E_P11_Protect(
    E2E_ContextType* context,
    void* data,
    uint32_t* length);

/**
 * @brief Profile 11 校验数据
 */
Std_ReturnType E2E_P11_Check(
    E2E_ContextType* context,
    const void* data,
    uint32_t length,
    uint16_t* status);

/******************************************************************************
 * Function Prototypes - Generic Interface
 ******************************************************************************/

/**
 * @brief 通用保护接口
 */
Std_ReturnType E2E_Protect(
    E2E_ContextType* context,
    void* data,
    uint32_t* length,
    uint8_t profile);

/**
 * @brief 通用校验接口
 */
Std_ReturnType E2E_Check(
    E2E_ContextType* context,
    const void* data,
    uint32_t length,
    uint16_t* status,
    uint8_t profile);

/**
 * @brief 获取Profile头部大小
 */
uint32_t E2E_GetHeaderSize(uint8_t profile);

/**
 * @brief 获取Profile名称
 */
const char* E2E_GetProfileName(uint8_t profile);

/******************************************************************************
 * Function Prototypes - CRC Calculation
 ******************************************************************************/

/**
 * @brief 计算CRC8 (SAE J1850)
 */
uint8_t E2E_CalculateCRC8(
    const uint8_t* data,
    uint32_t length,
    uint8_t initialCrc,
    const uint8_t* crcTable);

/**
 * @brief 计算CRC16 (CCITT)
 */
uint16_t E2E_CalculateCRC16(
    const uint8_t* data,
    uint32_t length,
    uint16_t initialCrc);

/**
 * @brief 计算CRC32 (IEEE 802.3)
 */
uint32_t E2E_CalculateCRC32(
    const uint8_t* data,
    uint32_t length,
    uint32_t initialCrc);

/**
 * @brief 初始化CRC表
 */
void E2E_InitCRC8Table(uint8_t* table, uint8_t polynomial);
void E2E_InitCRC16Table(uint16_t* table, uint16_t polynomial);
void E2E_InitCRC32Table(uint32_t* table, uint32_t polynomial);

/******************************************************************************
 * Function Prototypes - Utility
 ******************************************************************************/

/**
 * @brief 获取E2E版本
 */
const char* E2E_GetVersion(void);

/**
 * @brief 转换状态码为字符串
 */
const char* E2E_StatusToString(E2E_PCheckStatusType status);

/**
 * @brief 验证Data ID
 */
bool E2E_ValidateDataID(uint32_t dataId, uint8_t profile);

/**
 * @brief 验证数据长度
 */
bool E2E_ValidateLength(uint32_t length, uint8_t profile);

#ifdef __cplusplus
}
#endif

#endif /* E2E_PROTECTION_H */