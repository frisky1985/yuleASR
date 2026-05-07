/******************************************************************************
 * @file        E2E.h
 * @brief       E2E End-to-End Protection Module API
 * @module      Services (E2E)
 * @version     1.0.0
 * @precondition This module shall be initialized before use
 * @copyright   (c) 2024 Nous Research
 * @implements  E2E AUTOSAR Specification 4.4.0
 ******************************************************************************/

#ifndef E2E_H
#define E2E_H

/******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "Std_Types.h"
#include "E2E_Cfg.h"

/******************************************************************************
 * VERSION IDENTIFICATION
 ******************************************************************************/
#define E2E_VENDOR_ID           0x00U
#define E2E_MODULE_ID           0x56U
#define E2E_MAJOR_VERSION       1U
#define E2E_MINOR_VERSION       0U
#define E2E_PATCH_VERSION       0U

/******************************************************************************
 * ERROR CODES
 ******************************************************************************/
#define E2E_E_PARAM_POINTER     0x01U
#define E2E_E_PARAM_CONFIG      0x02U
#define E2E_E_PARAM_DATA        0x03U
#define E2E_E_INVALID_PROFILE   0x04U
#define E2E_E_UNINIT            0x05U
#define E2E_E_OUT_OF_RANGE      0x06U

/******************************************************************************
 * DET ERROR REPORTING
 ******************************************************************************/
#if (E2E_DEV_ERROR_DETECT == STD_ON)
    #define E2E_REPORT_ERROR(ApiId, ErrorId)    \
        (void)Det_ReportError(E2E_MODULE_ID, 0U, (ApiId), (ErrorId))
    #define E2E_CHECK_INITIALIZED(ApiId)        \
        do { if (E2E_ModuleInitialized != TRUE) { \
            E2E_REPORT_ERROR((ApiId), E2E_E_UNINIT); \
            return E_NOT_OK; } } while(0)
#else
    #define E2E_REPORT_ERROR(ApiId, ErrorId)
    #define E2E_CHECK_INITIALIZED(ApiId)
#endif

/******************************************************************************
 * E2E RETURN CODES
 ******************************************************************************/
#define E2E_E_OK                0x00U
#define E2E_E_CRCERROR          0x01U
#define E2E_E_WRONGSEQUENCE     0x02U
#define E2E_E_OKBACKWARDS       0x03U
#define E2E_E_OKREPEATED        0x04U
#define E2E_E_WRONGCRC          0x05U

/******************************************************************************
 * E2E PROFILES
 ******************************************************************************/
#define E2E_P_NONE              0x00U
#define E2E_P01                 0x01U
#define E2E_P02                 0x02U
#define E2E_P04                 0x04U
#define E2E_P05                 0x05U
#define E2E_P06                 0x06U
#define E2E_P07                 0x07U
#define E2E_P01_DATA            0x11U
#define E2E_P02_DATA            0x12U
#define E2E_P04_DATA            0x14U
#define E2E_P05_DATA            0x15U
#define E2E_P06_DATA            0x16U
#define E2E_P07_DATA            0x17U

/******************************************************************************
 * CRC PARAMETERS
 ******************************************************************************/
#define E2E_CRC8_SAEJ1850       0x1DU
#define E2E_CRC8_AUTOSAR        0x2FU
#define E2E_CRC8_CCITT          0x07U
#define E2E_CRC16_CCITT         0x1021U
#define E2E_CRC16_IBM           0x8005U
#define E2E_CRC32_AUTOSAR       0xF4ACFB13UL

/******************************************************************************
 * DATA TYPES
 ******************************************************************************/

typedef uint8 E2E_PProfileType;
typedef uint8 E2E_PCheckStatusType;

typedef struct
{
    uint8 Counter;
    uint8 CRC;
    uint8 DataID;
} E2E_P01HeaderType;

typedef struct
{
    uint8 Counter;
    uint16 CRC;
} E2E_P02HeaderType;

typedef struct
{
    uint32 Counter;
    uint32 CRC;
    uint16 DataID;
} E2E_P04HeaderType;

typedef struct
{
    uint8 Counter;
    uint8 CRC;
} E2E_P05HeaderType;

typedef struct
{
    uint8 Counter;
    uint32 CRC;
    uint16 DataID;
} E2E_P06HeaderType;

typedef struct
{
    uint32 Counter;
    uint32 CRC;
    uint32 DataID;
} E2E_P07HeaderType;

typedef struct
{
    uint8 ProfileVariant;
    uint8 CounterOffset;
    uint8 CRCOffset;
    uint8 DataIDOffset;
    uint8 CounterSize;
    uint8 CRCSize;
    uint8 DataIDSize;
    uint8 DataLength;
    uint16 DataID;
    boolean DataIDNibble;
    boolean InvertBitOrder;
} E2E_P01ConfigType;

typedef struct
{
    uint8 ProfileVariant;
    uint8 CounterOffset;
    uint8 CRCOffset;
    uint8 CounterSize;
    uint8 CRCSize;
    uint8 DataLength;
    boolean IncludeDataID;
    uint16 DataIDList[16];
} E2E_P02ConfigType;

typedef struct
{
    uint8 ProfileVariant;
    uint16 Offset;
    uint16 DataLength;
    uint32 DataID;
    boolean MinDeltaCounterInit;
    uint8 MaxDeltaCounterInit;
    uint8 MaxNoNewOrRepeatedData;
    uint8 SyncCounterInit;
    boolean NoNewOrRepeatedDataCounter;
} E2E_P04ConfigType;

typedef struct
{
    uint8 ProfileVariant;
    uint16 DataLength;
    uint8 CounterOffset;
    uint8 CRCOffset;
    uint8 E2ELength;
    uint8 DataID;
    uint16 Offset;
} E2E_P05ConfigType;

typedef struct
{
    uint8 ProfileVariant;
    uint16 Offset;
    uint16 DataLength;
    uint16 DataID;
    uint8 MinDeltaCounterInit;
    uint8 MaxDeltaCounterInit;
    uint8 MaxNoNewOrRepeatedData;
    uint8 SyncCounterInit;
    uint8 NoNewOrRepeatedDataCounter;
} E2E_P06ConfigType;

typedef struct
{
    uint8 ProfileVariant;
    uint16 Offset;
    uint16 DataLength;
    uint32 DataID;
    uint8 MinDeltaCounterInit;
    uint8 MaxDeltaCounterInit;
    uint8 MaxNoNewOrRepeatedData;
    uint8 SyncCounterInit;
    uint8 NoNewOrRepeatedDataCounter;
} E2E_P07ConfigType;

typedef union
{
    E2E_P01ConfigType P01;
    E2E_P02ConfigType P02;
    E2E_P04ConfigType P04;
    E2E_P05ConfigType P05;
    E2E_P06ConfigType P06;
    E2E_P07ConfigType P07;
} E2E_ProfileConfigType;

typedef struct
{
    uint8 Counter;
    uint8 AliveCounter;
    uint8 MaxDeltaCounter;
    boolean NewDataAvailable;
    uint8 LostData;
    uint8 Status;
    uint8 NoNewOrRepeatedDataCounter;
    uint8 SyncCounter;
} E2E_P01StateType;

typedef struct
{
    uint8 Counter;
    uint8 LastValidCounter;
    uint8 MaxDeltaCounter;
    boolean NewDataAvailable;
    uint8 LostData;
    uint8 Status;
    uint8 NoNewOrRepeatedDataCounter;
    uint8 SyncCounter;
    uint8 DataIDListIndex;
} E2E_P02StateType;

typedef struct
{
    uint32 Counter;
    uint32 LastValidCounter;
    uint32 MaxDeltaCounter;
    boolean NewDataAvailable;
    uint32 LostData;
    uint8 Status;
    uint8 NoNewOrRepeatedDataCounter;
    uint8 SyncCounter;
    boolean WaitForFirstData;
} E2E_P04StateType;

typedef struct
{
    uint8 Counter;
    uint8 CRC;
} E2E_P05StateType;

typedef struct
{
    uint32 Counter;
    uint32 LastValidCounter;
    uint8 MaxDeltaCounter;
    boolean NewDataAvailable;
    uint32 LostData;
    uint8 Status;
    uint8 NoNewOrRepeatedDataCounter;
    uint8 SyncCounter;
    boolean WaitForFirstData;
} E2E_P06StateType;

typedef struct
{
    uint32 Counter;
    uint32 LastValidCounter;
    uint8 MaxDeltaCounter;
    boolean NewDataAvailable;
    uint32 LostData;
    uint8 Status;
    uint8 NoNewOrRepeatedDataCounter;
    uint8 SyncCounter;
    boolean WaitForFirstData;
} E2E_P07StateType;

typedef union
{
    E2E_P01StateType P01;
    E2E_P02StateType P02;
    E2E_P04StateType P04;
    E2E_P05StateType P05;
    E2E_P06StateType P06;
    E2E_P07StateType P07;
} E2E_ProfileStateType;

typedef struct
{
    E2E_PProfileType Profile;
    E2E_ProfileConfigType Config;
    E2E_ProfileStateType State;
    uint16 DataLength;
    uint8 DataID;
    boolean Enabled;
} E2E_ConfigType;

typedef enum
{
    E2E_STATE_INIT = 0U,
    E2E_STATE_VALID = 1U,
    E2E_STATE_INVALID = 2U,
    E2E_STATE_DEINIT = 3U
} E2E_StateMachineType;

typedef struct
{
    uint8 Counter;
    uint16 DataID;
    uint8 Profile;
    uint8 Status;
    boolean Valid;
} E2E_CheckResultType;

/******************************************************************************
 * EXTERNAL DECLARATIONS
 ******************************************************************************/
extern VAR(boolean, E2E_VAR) E2E_ModuleInitialized;
extern CONST(E2E_ConfigType, E2E_CONST) E2E_ChannelConfig[E2E_MAX_CHANNELS];
extern CONST(uint8, E2E_CONST) E2E_CRC8_Table[256];
extern CONST(uint16, E2E_CONST) E2E_CRC16_Table[256];
extern CONST(uint32, E2E_CONST) E2E_CRC32_Table[256];

/******************************************************************************
 * API FUNCTIONS
 ******************************************************************************/

#define E2E_START_SEC_CODE
#include "MemMap.h"

extern FUNC(Std_ReturnType, E2E_CODE) E2E_Init(
    P2CONST(E2E_ConfigType, AUTOMATIC, E2E_APPL_CONST) ConfigPtr
);

extern FUNC(Std_ReturnType, E2E_CODE) E2E_DeInit(
    void
);

extern FUNC(Std_ReturnType, E2E_CODE) E2E_Pack(
    VAR(uint16, AUTOMATIC) ChannelId,
    P2VAR(uint8, AUTOMATIC, E2E_APPL_DATA) DataPtr,
    VAR(uint16, AUTOMATIC) Length,
    P2VAR(E2E_PCheckStatusType, AUTOMATIC, E2E_APPL_DATA) StatusPtr
);

extern FUNC(Std_ReturnType, E2E_CODE) E2E_Unpack(
    VAR(uint16, AUTOMATIC) ChannelId,
    P2CONST(uint8, AUTOMATIC, E2E_APPL_CONST) DataPtr,
    VAR(uint16, AUTOMATIC) Length,
    P2VAR(E2E_PCheckStatusType, AUTOMATIC, E2E_APPL_DATA) StatusPtr
);

extern FUNC(Std_ReturnType, E2E_CODE) E2E_Check(
    VAR(uint16, AUTOMATIC) ChannelId,
    P2CONST(uint8, AUTOMATIC, E2E_APPL_CONST) DataPtr,
    VAR(uint16, AUTOMATIC) Length,
    P2VAR(E2E_CheckResultType, AUTOMATIC, E2E_APPL_DATA) ResultPtr
);

extern FUNC(uint8, E2E_CODE) E2E_CalculateCRC8(
    P2CONST(uint8, AUTOMATIC, E2E_APPL_CONST) DataPtr,
    VAR(uint32, AUTOMATIC) Length,
    VAR(uint8, AUTOMATIC) StartValue,
    VAR(uint8, AUTOMATIC) Polynomial
);

extern FUNC(uint16, E2E_CODE) E2E_CalculateCRC16(
    P2CONST(uint8, AUTOMATIC, E2E_APPL_CONST) DataPtr,
    VAR(uint32, AUTOMATIC) Length,
    VAR(uint16, AUTOMATIC) StartValue,
    VAR(uint16, AUTOMATIC) Polynomial
);

extern FUNC(uint32, E2E_CODE) E2E_CalculateCRC32(
    P2CONST(uint8, AUTOMATIC, E2E_APPL_CONST) DataPtr,
    VAR(uint32, AUTOMATIC) Length,
    VAR(uint32, AUTOMATIC) StartValue,
    VAR(uint32, AUTOMATIC) Polynomial
);

extern FUNC(Std_ReturnType, E2E_CODE) E2E_P01Protect(
    P2VAR(E2E_P01ConfigType, AUTOMATIC, E2E_APPL_DATA) ConfigPtr,
    P2VAR(E2E_P01StateType, AUTOMATIC, E2E_APPL_DATA) StatePtr,
    P2VAR(uint8, AUTOMATIC, E2E_APPL_DATA) DataPtr
);

extern FUNC(Std_ReturnType, E2E_CODE) E2E_P01Check(
    P2CONST(E2E_P01ConfigType, AUTOMATIC, E2E_APPL_CONST) ConfigPtr,
    P2VAR(E2E_P01StateType, AUTOMATIC, E2E_APPL_DATA) StatePtr,
    P2CONST(uint8, AUTOMATIC, E2E_APPL_CONST) DataPtr
);

extern FUNC(Std_ReturnType, E2E_CODE) E2E_P02Protect(
    P2VAR(E2E_P02ConfigType, AUTOMATIC, E2E_APPL_DATA) ConfigPtr,
    P2VAR(E2E_P02StateType, AUTOMATIC, E2E_APPL_DATA) StatePtr,
    P2VAR(uint8, AUTOMATIC, E2E_APPL_DATA) DataPtr
);

extern FUNC(Std_ReturnType, E2E_CODE) E2E_P02Check(
    P2CONST(E2E_P02ConfigType, AUTOMATIC, E2E_APPL_CONST) ConfigPtr,
    P2VAR(E2E_P02StateType, AUTOMATIC, E2E_APPL_DATA) StatePtr,
    P2CONST(uint8, AUTOMATIC, E2E_APPL_CONST) DataPtr
);

extern FUNC(Std_ReturnType, E2E_CODE) E2E_P04Protect(
    P2VAR(E2E_P04ConfigType, AUTOMATIC, E2E_APPL_DATA) ConfigPtr,
    P2VAR(E2E_P04StateType, AUTOMATIC, E2E_APPL_DATA) StatePtr,
    P2VAR(uint8, AUTOMATIC, E2E_APPL_DATA) DataPtr
);

extern FUNC(Std_ReturnType, E2E_CODE) E2E_P04Check(
    P2CONST(E2E_P04ConfigType, AUTOMATIC, E2E_APPL_CONST) ConfigPtr,
    P2VAR(E2E_P04StateType, AUTOMATIC, E2E_APPL_DATA) StatePtr,
    P2CONST(uint8, AUTOMATIC, E2E_APPL_CONST) DataPtr
);

extern FUNC(Std_ReturnType, E2E_CODE) E2E_P05Protect(
    P2VAR(E2E_P05ConfigType, AUTOMATIC, E2E_APPL_DATA) ConfigPtr,
    P2VAR(E2E_P05StateType, AUTOMATIC, E2E_APPL_DATA) StatePtr,
    P2VAR(uint8, AUTOMATIC, E2E_APPL_DATA) DataPtr
);

extern FUNC(Std_ReturnType, E2E_CODE) E2E_P05Check(
    P2CONST(E2E_P05ConfigType, AUTOMATIC, E2E_APPL_CONST) ConfigPtr,
    P2VAR(E2E_P05StateType, AUTOMATIC, E2E_APPL_DATA) StatePtr,
    P2CONST(uint8, AUTOMATIC, E2E_APPL_CONST) DataPtr
);

extern FUNC(Std_ReturnType, E2E_CODE) E2E_P06Protect(
    P2VAR(E2E_P06ConfigType, AUTOMATIC, E2E_APPL_DATA) ConfigPtr,
    P2VAR(E2E_P06StateType, AUTOMATIC, E2E_APPL_DATA) StatePtr,
    P2VAR(uint8, AUTOMATIC, E2E_APPL_DATA) DataPtr
);

extern FUNC(Std_ReturnType, E2E_CODE) E2E_P06Check(
    P2CONST(E2E_P06ConfigType, AUTOMATIC, E2E_APPL_CONST) ConfigPtr,
    P2VAR(E2E_P06StateType, AUTOMATIC, E2E_APPL_DATA) StatePtr,
    P2CONST(uint8, AUTOMATIC, E2E_APPL_CONST) DataPtr
);

extern FUNC(Std_ReturnType, E2E_CODE) E2E_P07Protect(
    P2VAR(E2E_P07ConfigType, AUTOMATIC, E2E_APPL_DATA) ConfigPtr,
    P2VAR(E2E_P07StateType, AUTOMATIC, E2E_APPL_DATA) StatePtr,
    P2VAR(uint8, AUTOMATIC, E2E_APPL_DATA) DataPtr
);

extern FUNC(Std_ReturnType, E2E_CODE) E2E_P07Check(
    P2CONST(E2E_P07ConfigType, AUTOMATIC, E2E_APPL_CONST) ConfigPtr,
    P2VAR(E2E_P07StateType, AUTOMATIC, E2E_APPL_DATA) StatePtr,
    P2CONST(uint8, AUTOMATIC, E2E_APPL_CONST) DataPtr
);

extern FUNC(void, E2E_CODE) E2E_GetVersionInfo(
    P2VAR(Std_VersionInfoType, AUTOMATIC, E2E_APPL_DATA) VersionInfo
);

#define E2E_STOP_SEC_CODE
#include "MemMap.h"

/******************************************************************************
 * CALLBACK FUNCTIONS
 ******************************************************************************/

#define E2E_START_SEC_CALLOUT_CODE
#include "MemMap.h"

extern FUNC(void, E2E_CALLOUT_CODE) E2E_ProtectionCallback(
    VAR(uint16, AUTOMATIC) ChannelId,
    VAR(uint8, AUTOMATIC) Status
);

extern FUNC(void, E2E_CALLOUT_CODE) E2E_CheckCallback(
    VAR(uint16, AUTOMATIC) ChannelId,
    P2VAR(E2E_CheckResultType, AUTOMATIC, E2E_APPL_DATA) ResultPtr
);

#define E2E_STOP_SEC_CALLOUT_CODE
#include "MemMap.h"

#endif /* E2E_H */
