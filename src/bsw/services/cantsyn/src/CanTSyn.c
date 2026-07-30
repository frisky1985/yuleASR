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
 * @file CanTSyn.c
 * @brief CAN Time Synchronization implementation
 * @details Implements AUTOSAR CAN Time Synchronization module
 *          based on AUTOSAR_SWS_CANTimeSynchronization
 * 
 * Features:
 * - Global Time Synchronization
 * - Time Master functionality
 * - Time Slave functionality
 * - SYNC/FUP message handling
 * - OCS (Offset Correction Scale) handling
 * - Integration with StbM (Synchronized Time Base Manager)
 */

#include "CanTSyn.h"
#include "CanTSyn_Cfg.h"
#include "CanIf.h"
#include "StbM.h"
#include "Os.h"

/*******************************************************************************
 * Local Definitions
 ******************************************************************************/
#define CANTSYN_MODULE_ID                   (0xDAU)
#define CANTSYN_VENDOR_ID                   (0x00U)
#define CANTSYN_INSTANCE_ID                 (0x00U)

#define CANTSYN_SID_INIT                    (0x01U)
#define CANTSYN_SID_GETVERSIONINFO          (0x02U)
#define CANTSYN_SID_TRANSMIT                (0x03U)
#define CANTSYN_SID_RXINDICATION            (0x04U)
#define CANTSYN_SID_TXCONFIRMATION          (0x05U)
#define CANTSYN_SID_MAINFUNCTION            (0x06U)

/* SYNC Message Types */
#define CANTSYN_SYNC_MSG_TYPE               (0x10U)
#define CANTSYN_OFS_MSG_TYPE                (0x20U)

/* Time Domain IDs */
#define CANTSYN_TIME_DOMAIN_0               (0x00U)
#define CANTSYN_TIME_DOMAIN_1               (0x01U)
#define CANTSYN_TIME_DOMAIN_2               (0x02U)
#define CANTSYN_TIME_DOMAIN_3               (0x03U)

/* Message Lengths */
#define CANTSYN_SYNC_MSG_LENGTH             (16U)
#define CANTSYN_OFS_MSG_LENGTH              (12U)

/* User Bytes Count */
#define CANTSYN_SYNC_USER_BYTES             (3U)
#define CANTSYN_OFS_USER_BYTES              (2U)

/* Nanoseconds to microseconds conversion */
#define CANTSYN_NS_TO_US(ns)                ((ns) / 1000U)
#define CANTSYN_US_TO_NS(us)                ((us) * 1000U)

/*******************************************************************************
 * Local Type Definitions
 ******************************************************************************/

typedef enum
{
    CANTSYN_STATE_UNINIT = 0,
    CANTSYN_STATE_INIT
} CanTSyn_StateType;

typedef enum
{
    CANTSYN_TX_IDLE = 0,
    CANTSYN_TX_BUSY
} CanTSyn_TxStateType;

typedef struct
{
    uint8   TimeDomainId;
    uint32  SequenceCounter;
    StbM_TimeStampType TxTimeStamp;
    StbM_TimeStampType RxTimeStamp;
    uint8   UserData[3];
    uint8   TimeBaseStatus;
} CanTSyn_TimeDomainInfoType;

typedef struct
{
    CanTSyn_StateType       State;
    CanTSyn_TxStateType     TxState;
    uint32                  TxCounter;
    uint32                  RxCounter;
    StbM_SynchronizedTimeBaseType TimeBaseRef;
} CanTSyn_InternalType;

/*******************************************************************************
 * Local Variables
 ******************************************************************************/
static CanTSyn_InternalType CanTSyn_Internal = {
    .State = CANTSYN_STATE_UNINIT,
    .TxState = CANTSYN_TX_IDLE,
    .TxCounter = 0U,
    .RxCounter = 0U,
    .TimeBaseRef = {0}
};

static CanTSyn_TimeDomainInfoType CanTSyn_TimeDomains[CANTSYN_NUMBER_OF_TIME_DOMAINS];

/*******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/
static void CanTSyn_PrepareSyncMessage(
    uint8 TimeDomainId,
    PduInfoType* PduInfoPtr,
    StbM_TimeStampType* TimeStampPtr);

static void CanTSyn_PrepareOfsMessage(
    uint8 TimeDomainId,
    PduInfoType* PduInfoPtr,
    StbM_TimeStampType* TimeStampPtr);

static void CanTSyn_ProcessSyncMessage(
    uint8 TimeDomainId,
    const PduInfoType* PduInfoPtr);

static void CanTSyn_ProcessOfsMessage(
    uint8 TimeDomainId,
    const PduInfoType* PduInfoPtr);

static Std_ReturnType CanTSyn_GetCurrentTime(
    uint8 TimeBaseId,
    StbM_TimeStampType* TimeStampPtr,
    StbM_UserDataType* UserDataPtr);

/*******************************************************************************
 * API Functions
 ******************************************************************************/

/**
 * @brief Initialize CanTSyn module
 */
void CanTSyn_Init(const CanTSyn_ConfigType* ConfigPtr)
{
    uint8 i;
    
    #if (CANTSYN_DEV_ERROR_DETECT == STD_ON)
    if (CanTSyn_Internal.State == CANTSYN_STATE_INIT)
    {
        Det_ReportError(CANTSYN_MODULE_ID, CANTSYN_INSTANCE_ID, 
                        CANTSYN_SID_INIT, CANTSYN_E_ALREADY_INITIALIZED);
        return;
    }
    
    if (ConfigPtr == NULL_PTR)
    {
        Det_ReportError(CANTSYN_MODULE_ID, CANTSYN_INSTANCE_ID,
                        CANTSYN_SID_INIT, CANTSYN_E_PARAM_POINTER);
        return;
    }
    #endif
    
    /* Initialize time domains */
    for (i = 0U; i < CANTSYN_NUMBER_OF_TIME_DOMAINS; i++)
    {
        CanTSyn_TimeDomains[i].TimeDomainId = i;
        CanTSyn_TimeDomains[i].SequenceCounter = 0U;
        CanTSyn_TimeDomains[i].TimeBaseStatus = 0U;
        CanTSyn_TimeDomains[i].UserData[0] = 0U;
        CanTSyn_TimeDomains[i].UserData[1] = 0U;
        CanTSyn_TimeDomains[i].UserData[2] = 0U;
    }
    
    CanTSyn_Internal.State = CANTSYN_STATE_INIT;
    CanTSyn_Internal.TxState = CANTSYN_TX_IDLE;
    CanTSyn_Internal.TxCounter = 0U;
    CanTSyn_Internal.RxCounter = 0U;
}

/**
 * @brief Get version information
 */
#if (CANTSYN_VERSION_INFO_API == STD_ON)
void CanTSyn_GetVersionInfo(Std_VersionInfoType* VersionInfo)
{
    #if (CANTSYN_DEV_ERROR_DETECT == STD_ON)
    if (VersionInfo == NULL_PTR)
    {
        Det_ReportError(CANTSYN_MODULE_ID, CANTSYN_INSTANCE_ID,
                        CANTSYN_SID_GETVERSIONINFO, CANTSYN_E_PARAM_POINTER);
        return;
    }
    #endif
    
    VersionInfo->vendorID = CANTSYN_VENDOR_ID;
    VersionInfo->moduleID = CANTSYN_MODULE_ID;
    VersionInfo->sw_major_version = CANTSYN_SW_MAJOR_VERSION;
    VersionInfo->sw_minor_version = CANTSYN_SW_MINOR_VERSION;
    VersionInfo->sw_patch_version = CANTSYN_SW_PATCH_VERSION;
}
#endif

/**
 * @brief Transmission confirmation callback
 */
void CanTSyn_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    #if (CANTSYN_DEV_ERROR_DETECT == STD_ON)
    if (CanTSyn_Internal.State != CANTSYN_STATE_INIT)
    {
        Det_ReportError(CANTSYN_MODULE_ID, CANTSYN_INSTANCE_ID,
                        CANTSYN_SID_TXCONFIRMATION, CANTSYN_E_UNINIT);
        return;
    }
    
    if (TxPduId >= CANTSYN_NUMBER_OF_PDUS)
    {
        Det_ReportError(CANTSYN_MODULE_ID, CANTSYN_INSTANCE_ID,
                        CANTSYN_SID_TXCONFIRMATION, CANTSYN_E_INVALID_PDU_SDU_ID);
        return;
    }
    #endif
    
    if (result == E_OK)
    {
        CanTSyn_Internal.TxCounter++;
        CanTSyn_Internal.TxState = CANTSYN_TX_IDLE;
    }
}

/**
 * @brief Reception indication callback
 */
void CanTSyn_RxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    uint8 TimeDomainId;
    uint8 MsgType;
    
    #if (CANTSYN_DEV_ERROR_DETECT == STD_ON)
    if (CanTSyn_Internal.State != CANTSYN_STATE_INIT)
    {
        Det_ReportError(CANTSYN_MODULE_ID, CANTSYN_INSTANCE_ID,
                        CANTSYN_SID_RXINDICATION, CANTSYN_E_UNINIT);
        return;
    }
    
    if (RxPduId >= CANTSYN_NUMBER_OF_PDUS)
    {
        Det_ReportError(CANTSYN_MODULE_ID, CANTSYN_INSTANCE_ID,
                        CANTSYN_SID_RXINDICATION, CANTSYN_E_INVALID_PDU_SDU_ID);
        return;
    }
    
    if (PduInfoPtr == NULL_PTR)
    {
        Det_ReportError(CANTSYN_MODULE_ID, CANTSYN_INSTANCE_ID,
                        CANTSYN_SID_RXINDICATION, CANTSYN_E_PARAM_POINTER);
        return;
    }
    #endif
    
    CanTSyn_Internal.RxCounter++;
    
    /* Extract message type and time domain */
    TimeDomainId = (PduInfoPtr->SduDataPtr[0] >> 4U) & 0x0FU;
    MsgType = PduInfoPtr->SduDataPtr[0] & 0x0FU;
    
    if (TimeDomainId >= CANTSYN_NUMBER_OF_TIME_DOMAINS)
    {
        return; /* Invalid time domain */
    }
    
    /* Process based on message type */
    if (MsgType == CANTSYN_SYNC_MSG_TYPE)
    {
        CanTSyn_ProcessSyncMessage(TimeDomainId, PduInfoPtr);
    }
    else if (MsgType == CANTSYN_OFS_MSG_TYPE)
    {
        CanTSyn_ProcessOfsMessage(TimeDomainId, PduInfoPtr);
    }
}

/*******************************************************************************
 * Local Functions
 ******************************************************************************/

static void CanTSyn_PrepareSyncMessage(
    uint8 TimeDomainId,
    PduInfoType* PduInfoPtr,
    StbM_TimeStampType* TimeStampPtr)
{
    uint8 i;
    uint8* DataPtr = PduInfoPtr->SduDataPtr;
    
    /* Byte 0: Type (0x10) + Time Domain ID */
    DataPtr[0] = CANTSYN_SYNC_MSG_TYPE | (TimeDomainId << 4U);
    
    /* Bytes 1-4: Nanoseconds (32-bit, big-endian) */
    DataPtr[1] = (uint8)(TimeStampPtr->nanoseconds >> 24U);
    DataPtr[2] = (uint8)(TimeStampPtr->nanoseconds >> 16U);
    DataPtr[3] = (uint8)(TimeStampPtr->nanoseconds >> 8U);
    DataPtr[4] = (uint8)(TimeStampPtr->nanoseconds);
    
    /* Bytes 5-8: Seconds (32-bit, big-endian) */
    DataPtr[5] = (uint8)(TimeStampPtr->seconds >> 24U);
    DataPtr[6] = (uint8)(TimeStampPtr->seconds >> 16U);
    DataPtr[7] = (uint8)(TimeStampPtr->seconds >> 8U);
    DataPtr[8] = (uint8)(TimeStampPtr->seconds);
    
    /* Byte 9: User Data Byte 0 */
    DataPtr[9] = CanTSyn_TimeDomains[TimeDomainId].UserData[0];
    
    /* Byte 10: User Data Byte 1 */
    DataPtr[10] = CanTSyn_TimeDomains[TimeDomainId].UserData[1];
    
    /* Byte 11: User Data Byte 2 + Sequence Counter */
    DataPtr[11] = (CanTSyn_TimeDomains[TimeDomainId].UserData[2] & 0xF0U) |
                  (CanTSyn_TimeDomains[TimeDomainId].SequenceCounter & 0x0FU);
    
    /* Bytes 12-15: Partial Seconds */
    DataPtr[12] = 0x00U;
    DataPtr[13] = 0x00U;
    DataPtr[14] = 0x00U;
    DataPtr[15] = 0x00U;
    
    PduInfoPtr->SduLength = CANTSYN_SYNC_MSG_LENGTH;
}

static void CanTSyn_PrepareOfsMessage(
    uint8 TimeDomainId,
    PduInfoType* PduInfoPtr,
    StbM_TimeStampType* TimeStampPtr)
{
    uint8* DataPtr = PduInfoPtr->SduDataPtr;
    
    /* Byte 0: Type (0x20) + Time Domain ID */
    DataPtr[0] = CANTSYN_OFS_MSG_TYPE | (TimeDomainId << 4U);
    
    /* Bytes 1-4: Nanoseconds (32-bit, big-endian) */
    DataPtr[1] = (uint8)(TimeStampPtr->nanoseconds >> 24U);
    DataPtr[2] = (uint8)(TimeStampPtr->nanoseconds >> 16U);
    DataPtr[3] = (uint8)(TimeStampPtr->nanoseconds >> 8U);
    DataPtr[4] = (uint8)(TimeStampPtr->nanoseconds);
    
    /* Bytes 5-8: Seconds (32-bit, big-endian) */
    DataPtr[5] = (uint8)(TimeStampPtr->seconds >> 24U);
    DataPtr[6] = (uint8)(TimeStampPtr->seconds >> 16U);
    DataPtr[7] = (uint8)(TimeStampPtr->seconds >> 8U);
    DataPtr[8] = (uint8)(TimeStampPtr->seconds);
    
    /* Byte 9: Time Base Status */
    DataPtr[9] = TimeStampPtr->timeBaseStatus;
    
    /* Byte 10: User Data Byte 0 */
    DataPtr[10] = CanTSyn_TimeDomains[TimeDomainId].UserData[0];
    
    /* Byte 11: User Data Byte 1 + Sequence Counter */
    DataPtr[11] = (CanTSyn_TimeDomains[TimeDomainId].UserData[1] & 0xF0U) |
                  (CanTSyn_TimeDomains[TimeDomainId].SequenceCounter & 0x0FU);
    
    PduInfoPtr->SduLength = CANTSYN_OFS_MSG_LENGTH;
}

static void CanTSyn_ProcessSyncMessage(
    uint8 TimeDomainId,
    const PduInfoType* PduInfoPtr)
{
    StbM_TimeStampType RxTimeStamp;
    const uint8* DataPtr = PduInfoPtr->SduDataPtr;
    
    /* Extract nanoseconds (bytes 1-4, big-endian) */
    RxTimeStamp.nanoseconds = ((uint32)DataPtr[1] << 24U) |
                              ((uint32)DataPtr[2] << 16U) |
                              ((uint32)DataPtr[3] << 8U) |
                              (uint32)DataPtr[4];
    
    /* Extract seconds (bytes 5-8, big-endian) */
    RxTimeStamp.seconds = ((uint32)DataPtr[5] << 24U) |
                          ((uint32)DataPtr[6] << 16U) |
                          ((uint32)DataPtr[7] << 8U) |
                          (uint32)DataPtr[8];
    
    /* Store received timestamp */
    CanTSyn_TimeDomains[TimeDomainId].RxTimeStamp = RxTimeStamp;
    
    /* Extract user data */
    CanTSyn_TimeDomains[TimeDomainId].UserData[0] = DataPtr[9];
    CanTSyn_TimeDomains[TimeDomainId].UserData[1] = DataPtr[10];
    CanTSyn_TimeDomains[TimeDomainId].UserData[2] = (DataPtr[11] >> 4U) & 0x0FU;
    
    /* Update sequence counter */
    CanTSyn_TimeDomains[TimeDomainId].SequenceCounter = DataPtr[11] & 0x0FU;
    
    /* Update StbM with received time */
    #if (CANTSYN_TIME_MASTER_SUPPORT == STD_OFF)
    StbM_SetGlobalTime(TimeDomainId, &RxTimeStamp, NULL_PTR);
    #endif
}

static void CanTSyn_ProcessOfsMessage(
    uint8 TimeDomainId,
    const PduInfoType* PduInfoPtr)
{
    StbM_TimeStampType OffsetTimeStamp;
    const uint8* DataPtr = PduInfoPtr->SduDataPtr;
    
    /* Extract nanoseconds */
    OffsetTimeStamp.nanoseconds = ((uint32)DataPtr[1] << 24U) |
                                  ((uint32)DataPtr[2] << 16U) |
                                  ((uint32)DataPtr[3] << 8U) |
                                  (uint32)DataPtr[4];
    
    /* Extract seconds */
    OffsetTimeStamp.seconds = ((uint32)DataPtr[5] << 24U) |
                              ((uint32)DataPtr[6] << 16U) |
                              ((uint32)DataPtr[7] << 8U) |
                              (uint32)DataPtr[8];
    
    /* Extract time base status */
    OffsetTimeStamp.timeBaseStatus = DataPtr[9];
    
    /* Extract user data */
    CanTSyn_TimeDomains[TimeDomainId].UserData[0] = DataPtr[10];
    CanTSyn_TimeDomains[TimeDomainId].UserData[1] = (DataPtr[11] >> 4U) & 0x0FU;
    
    /* Update sequence counter */
    CanTSyn_TimeDomains[TimeDomainId].SequenceCounter = DataPtr[11] & 0x0FU;
    
    /* Apply offset correction via StbM */
    #if (CANTSYN_TIME_MASTER_SUPPORT == STD_OFF)
    StbM_UpdateGlobalTimeOffset(TimeDomainId, &OffsetTimeStamp);
    #endif
}

static Std_ReturnType CanTSyn_GetCurrentTime(
    uint8 TimeBaseId,
    StbM_TimeStampType* TimeStampPtr,
    StbM_UserDataType* UserDataPtr)
{
    Std_ReturnType RetVal;
    
    RetVal = StbM_GetCurrentTime(TimeBaseId, TimeStampPtr, UserDataPtr);
    
    if (RetVal == E_OK)
    {
        /* Update local user data cache */
        CanTSyn_TimeDomains[TimeBaseId].UserData[0] = UserDataPtr->userData[0];
        CanTSyn_TimeDomains[TimeBaseId].UserData[1] = UserDataPtr->userData[1];
        CanTSyn_TimeDomains[TimeBaseId].UserData[2] = UserDataPtr->userByteCount;
    }
    
    return RetVal;
}

/*******************************************************************************
 * Main Function
 ******************************************************************************/

void CanTSyn_MainFunction(void)
{
    uint8 i;
    PduInfoType PduInfo;
    uint8 TxBuffer[CANTSYN_SYNC_MSG_LENGTH];
    Std_ReturnType RetVal;
    StbM_TimeStampType CurrentTime;
    StbM_UserDataType UserData;
    
    #if (CANTSYN_DEV_ERROR_DETECT == STD_ON)
    if (CanTSyn_Internal.State != CANTSYN_STATE_INIT)
    {
        Det_ReportError(CANTSYN_MODULE_ID, CANTSYN_INSTANCE_ID,
                        CANTSYN_SID_MAINFUNCTION, CANTSYN_E_UNINIT);
        return;
    }
    #endif
    
    PduInfo.SduDataPtr = TxBuffer;
    
    /* Process each time domain */
    for (i = 0U; i < CANTSYN_NUMBER_OF_TIME_DOMAINS; i++)
    {
        #if (CANTSYN_TIME_MASTER_SUPPORT == STD_ON)
        /* Time Master: Send SYNC messages periodically */
        if (CanTSyn_TimeDomainConfig[i].IsTimeMaster == TRUE)
        {
            /* Get current time from StbM */
            RetVal = CanTSyn_GetCurrentTime(i, &CurrentTime, &UserData);
            
            if (RetVal == E_OK)
            {
                /* Prepare SYNC message */
                CanTSyn_PrepareSyncMessage(i, &PduInfo, &CurrentTime);
                
                /* Transmit via CanIf */
                if (CanTSyn_Internal.TxState == CANTSYN_TX_IDLE)
                {
                    RetVal = CanIf_Transmit(CanTSyn_TimeDomainConfig[i].TxPduId, &PduInfo);
                    
                    if (RetVal == E_OK)
                    {
                        CanTSyn_Internal.TxState = CANTSYN_TX_BUSY;
                        
                        /* Increment sequence counter */
                        CanTSyn_TimeDomains[i].SequenceCounter++;
                        if (CanTSyn_TimeDomains[i].SequenceCounter >= 16U)
                        {
                            CanTSyn_TimeDomains[i].SequenceCounter = 0U;
                        }
                    }
                }
            }
        }
        #endif
    }
}

/*******************************************************************************
 * End of File
 ******************************************************************************/
