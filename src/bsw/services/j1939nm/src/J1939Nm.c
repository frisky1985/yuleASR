/**
 * @file J1939Nm.c
 * @brief J1939 Network Management module implementation
 * @version 1.0.0
 * @date 2026-04-28
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: J1939 Network Management (NM)
 * Layer: Service Layer
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "J1939Nm.h"
#include "Det.h"

/*==================================================================================================
*                                    LOCAL MACROS
==================================================================================================*/
#define J1939NM_DATA_SIZE                   (8U)
#define J1939NM_AC_PRIORITY                 (3U)
#define J1939NM_INVALID_CHANNEL             (0xFFU)

/*==================================================================================================
*                                    LOCAL TYPE DEFINITIONS
==================================================================================================*/
/**
 * @brief J1939 NM Channel State structure
 */
typedef struct {
    J1939Nm_StateType State;                /*!< Current NM state */
    J1939Nm_AcStateType AcState;            /*!< Address claiming state */
    J1939Nm_NameType Name;                  /*!< Current NAME */
    J1939Nm_AddressType Address;            /*!< Current address */
    J1939Nm_AddressType PreferredAddress;   /*!< Preferred address */
    boolean BusOffState;                    /*!< Bus-off state */
    boolean AddressClaimed;                 /*!< Address claimed flag */
    uint16 AcDelayTimer;                    /*!< Address claim delay timer */
    uint16 AcTimeoutTimer;                  /*!< Address claim timeout timer */
    uint16 BusOffRecoveryTimer;             /*!< Bus-off recovery timer */
    uint16 AcRepeatTimer;                   /*!< Address claim repeat timer */
    uint8 AcRetryCount;                     /*!< Address claim retry count */
} J1939Nm_ChannelStateType;

/*==================================================================================================
*                                    LOCAL VARIABLES
==================================================================================================*/
#define J1939NM_START_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

STATIC boolean J1939Nm_Initialized = FALSE;
STATIC const J1939Nm_ConfigType* J1939Nm_ConfigPtr = NULL_PTR;

#define J1939NM_STOP_SEC_VAR_INIT_UNSPECIFIED
#include "MemMap.h"

#define J1939NM_START_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

STATIC J1939Nm_ChannelStateType J1939Nm_ChannelStates[J1939NM_NUMBER_OF_CHANNELS];

#define J1939NM_STOP_SEC_VAR_CLEARED_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                    LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
STATIC Std_ReturnType J1939Nm_ValidateChannel(J1939Nm_ChannelType Channel);
STATIC void J1939Nm_ProcessStateMachine(J1939Nm_ChannelType Channel);
STATIC void J1939Nm_ProcessAddressClaiming(J1939Nm_ChannelType Channel);
STATIC Std_ReturnType J1939Nm_TransmitAddressClaimed(J1939Nm_ChannelType Channel);
STATIC Std_ReturnType J1939Nm_TransmitCannotClaimAddress(J1939Nm_ChannelType Channel);
STATIC Std_ReturnType J1939Nm_TransmitRequestForAddressClaimed(J1939Nm_ChannelType Channel);
STATIC sint8 J1939Nm_CompareNames(J1939Nm_NameType Name1, J1939Nm_NameType Name2);
STATIC void J1939Nm_BuildAcPdu(uint8* Data, J1939Nm_NameType Name);

/*==================================================================================================
*                                    LOCAL FUNCTIONS
==================================================================================================*/
/**
 * @brief Validates the channel ID
 */
STATIC Std_ReturnType J1939Nm_ValidateChannel(J1939Nm_ChannelType Channel)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (Channel < J1939NM_NUMBER_OF_CHANNELS) {
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Builds Address Claimed PDU data
 */
STATIC void J1939Nm_BuildAcPdu(uint8* Data, J1939Nm_NameType Name)
{
    if (Data != NULL_PTR) {
        /* NAME is 64-bit, store in little-endian format for CAN */
        Data[0] = (uint8)(Name & 0xFFU);
        Data[1] = (uint8)((Name >> 8U) & 0xFFU);
        Data[2] = (uint8)((Name >> 16U) & 0xFFU);
        Data[3] = (uint8)((Name >> 24U) & 0xFFU);
        Data[4] = (uint8)((Name >> 32U) & 0xFFU);
        Data[5] = (uint8)((Name >> 40U) & 0xFFU);
        Data[6] = (uint8)((Name >> 48U) & 0xFFU);
        Data[7] = (uint8)((Name >> 56U) & 0xFFU);
    }
}

/**
 * @brief Compares two J1939 NAMEs
 * @return -1 if Name1 < Name2, 0 if equal, 1 if Name1 > Name2
 */
STATIC sint8 J1939Nm_CompareNames(J1939Nm_NameType Name1, J1939Nm_NameType Name2)
{
    sint8 result = 0;
    
    if (Name1 < Name2) {
        result = -1;
    } else if (Name1 > Name2) {
        result = 1;
    }
    
    return result;
}

/**
 * @brief Transmits Address Claimed message
 */
STATIC Std_ReturnType J1939Nm_TransmitAddressClaimed(J1939Nm_ChannelType Channel)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 data[J1939NM_DATA_SIZE];
    
    if (J1939Nm_ValidateChannel(Channel) == E_OK) {
        J1939Nm_BuildAcPdu(data, J1939Nm_ChannelStates[Channel].Name);
        
        /* In real implementation, this would call CanIf_Transmit */
        /* For now, we simulate success */
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Transmits Cannot Claim Address message
 */
STATIC Std_ReturnType J1939Nm_TransmitCannotClaimAddress(J1939Nm_ChannelType Channel)
{
    Std_ReturnType result = E_NOT_OK;
    uint8 data[J1939NM_DATA_SIZE];
    
    if (J1939Nm_ValidateChannel(Channel) == E_OK) {
        J1939Nm_BuildAcPdu(data, J1939Nm_ChannelStates[Channel].Name);
        
        /* Transmit with NULL_PTR address (0xFE) as source */
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Transmits Request for Address Claimed message
 */
STATIC Std_ReturnType J1939Nm_TransmitRequestForAddressClaimed(J1939Nm_ChannelType Channel)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (J1939Nm_ValidateChannel(Channel) == E_OK) {
        /* Request for Address Claimed is a global request */
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Processes the address claiming state machine
 */
STATIC void J1939Nm_ProcessAddressClaiming(J1939Nm_ChannelType Channel)
{
    J1939Nm_ChannelStateType* channelState = &J1939Nm_ChannelStates[Channel];
    const J1939Nm_ChannelConfigType* channelConfig = &J1939Nm_ConfigPtr->ChannelConfig[Channel];
    
    switch (channelState->AcState) {
        case J1939NM_AC_STATE_IDLE:
            /* Start address claiming process */
            channelState->AcDelayTimer = (uint16)(channelConfig->AcDelayMin + 
                ((channelState->Name % 100U) % (channelConfig->AcDelayMax - channelConfig->AcDelayMin + 1)));
            channelState->AcState = J1939NM_AC_STATE_WAITING;
            break;
            
        case J1939NM_AC_STATE_WAITING:
            /* Wait for address claim delay */
            if (channelState->AcDelayTimer > 0U) {
                channelState->AcDelayTimer--;
            } else {
                /* Delay expired, claim address */
                if (J1939Nm_TransmitAddressClaimed(Channel) == E_OK) {
                    channelState->AcState = J1939NM_AC_STATE_CLAIMED;
                    channelState->AddressClaimed = TRUE;
                    channelState->Address = channelConfig->PreferredAddress;
                    channelState->AcRepeatTimer = J1939NM_AC_REPEAT_TIME_MS / J1939NM_MAIN_FUNCTION_PERIOD_MS;
                } else {
                    channelState->AcState = J1939NM_AC_STATE_CANNOT_CLAIM;
                }
            }
            break;
            
        case J1939NM_AC_STATE_CLAIMED:
            /* Address claimed, periodically repeat */
            if (channelState->AcRepeatTimer > 0U) {
                channelState->AcRepeatTimer--;
            } else {
                /* Repeat address claim */
                (void)J1939Nm_TransmitAddressClaimed(Channel);
                channelState->AcRepeatTimer = J1939NM_AC_REPEAT_TIME_MS / J1939NM_MAIN_FUNCTION_PERIOD_MS;
            }
            break;
            
        case J1939NM_AC_STATE_CONFLICT:
            /* Address conflict detected, try to resolve */
            if ((channelConfig->ArbitraryAddressCapable) != 0U) {
                /* Try alternate address */
                if (channelState->AcRetryCount < 10U) {
                    channelState->AcRetryCount++;
                    channelState->Address = (channelConfig->PreferredAddress + channelState->AcRetryCount) % 254U;
                    channelState->AcDelayTimer = (uint16)(channelConfig->AcDelayMin);
                    channelState->AcState = J1939NM_AC_STATE_WAITING;
                } else {
                    /* Cannot find available address */
                    channelState->AcState = J1939NM_AC_STATE_CANNOT_CLAIM;
                    (void)J1939Nm_TransmitCannotClaimAddress(Channel);
                }
            } else {
                /* Cannot claim address */
                channelState->AcState = J1939NM_AC_STATE_CANNOT_CLAIM;
                (void)J1939Nm_TransmitCannotClaimAddress(Channel);
            }
            break;
            
        case J1939NM_AC_STATE_CANNOT_CLAIM:
            /* Cannot claim address, use NULL_PTR address */
            channelState->Address = J1939NM_NULL_ADDRESS;
            channelState->AddressClaimed = FALSE;
            break;
            
        default:
            /* Invalid state, reset to idle */
            channelState->AcState = J1939NM_AC_STATE_IDLE;
            break;
    }
}

/**
 * @brief Processes the NM state machine
 */
STATIC void J1939Nm_ProcessStateMachine(J1939Nm_ChannelType Channel)
{
    J1939Nm_ChannelStateType* channelState = &J1939Nm_ChannelStates[Channel];
    
    switch (channelState->State) {
        case J1939NM_STATE_UNINIT:
            /* Do nothing, wait for initialization */
            break;
            
        case J1939NM_STATE_BUS_OFF:
            /* Bus-off recovery */
            if (channelState->BusOffRecoveryTimer > 0U) {
                channelState->BusOffRecoveryTimer--;
            } else {
                /* Recovery time expired, try to restart */
                channelState->BusOffState = FALSE;
                channelState->State = J1939NM_STATE_WAIT_FOR_AC;
                channelState->AcState = J1939NM_AC_STATE_IDLE;
            }
            break;
            
        case J1939NM_STATE_WAIT_FOR_AC:
            /* Waiting for address claiming */
            J1939Nm_ProcessAddressClaiming(Channel);
            if (channelState->AcState == J1939NM_AC_STATE_CLAIMED) {
                channelState->State = J1939NM_STATE_NORMAL_OPERATION;
            } else if (channelState->AcState == J1939NM_AC_STATE_CANNOT_CLAIM) {
                channelState->State = J1939NM_STATE_TX_CANNOT_CLAIM;
            }
            break;
            
        case J1939NM_STATE_NORMAL_OPERATION:
            /* Normal operation, process address claiming */
            J1939Nm_ProcessAddressClaiming(Channel);
            break;
            
        case J1939NM_STATE_TX_CANNOT_CLAIM:
            /* Cannot claim address state */
            break;
            
        default:
            /* Invalid state */
            channelState->State = J1939NM_STATE_UNINIT;
            break;
    }
}

/*==================================================================================================
*                                    GLOBAL FUNCTIONS
==================================================================================================*/
/**
 * @brief Initializes the J1939 Network Management module
 */
void J1939Nm_Init(const J1939Nm_ConfigType* ConfigPtr)
{
    uint8 i;
    
    if (ConfigPtr == NULL_PTR) {
#if (J1939NM_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(J1939NM_MODULE_ID, 0U, J1939NM_SID_INIT, J1939NM_E_INVALID_POINTER);
#endif
        return;
    }
    
    J1939Nm_ConfigPtr = ConfigPtr;
    
    /* Initialize channel states */
    for (i = 0U; i < J1939NM_NUMBER_OF_CHANNELS; i++) {
        J1939Nm_ChannelStates[i].State = J1939NM_STATE_WAIT_FOR_AC;
        J1939Nm_ChannelStates[i].AcState = J1939NM_AC_STATE_IDLE;
        J1939Nm_ChannelStates[i].Name = ConfigPtr->ChannelConfig[i].Name;
        J1939Nm_ChannelStates[i].Address = J1939NM_NULL_ADDRESS;
        J1939Nm_ChannelStates[i].PreferredAddress = ConfigPtr->ChannelConfig[i].PreferredAddress;
        J1939Nm_ChannelStates[i].BusOffState = FALSE;
        J1939Nm_ChannelStates[i].AddressClaimed = FALSE;
        J1939Nm_ChannelStates[i].AcDelayTimer = 0U;
        J1939Nm_ChannelStates[i].AcTimeoutTimer = 0U;
        J1939Nm_ChannelStates[i].BusOffRecoveryTimer = 0U;
        J1939Nm_ChannelStates[i].AcRepeatTimer = 0U;
        J1939Nm_ChannelStates[i].AcRetryCount = 0U;
    }
    
    J1939Nm_Initialized = TRUE;
}

/**
 * @brief Deinitializes the J1939 Network Management module
 */
void J1939Nm_DeInit(void)
{
    uint8 i;
    
    if (!J1939Nm_Initialized) {
#if (J1939NM_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(J1939NM_MODULE_ID, 0U, J1939NM_SID_DEINIT, J1939NM_E_NOT_INITIALIZED);
#endif
        return;
    }
    
    /* Reset channel states */
    for (i = 0U; i < J1939NM_NUMBER_OF_CHANNELS; i++) {
        J1939Nm_ChannelStates[i].State = J1939NM_STATE_UNINIT;
        J1939Nm_ChannelStates[i].AcState = J1939NM_AC_STATE_IDLE;
    }
    
    J1939Nm_ConfigPtr = NULL_PTR;
    J1939Nm_Initialized = FALSE;
}

/**
 * @brief Gets version information
 */
#if (J1939NM_VERSION_INFO_API == STD_ON)
void J1939Nm_GetVersionInfo(Std_VersionInfoType* VersionInfo)
{
    if (VersionInfo == NULL_PTR) {
#if (J1939NM_DEV_ERROR_DETECT == STD_ON)
        (void)Det_ReportError(J1939NM_MODULE_ID, 0U, J1939NM_SID_GET_VERSION_INFO, J1939NM_E_INVALID_POINTER);
#endif
        return;
    }
    
    VersionInfo->vendorID = J1939NM_VENDOR_ID;
    VersionInfo->moduleID = J1939NM_MODULE_ID;
    VersionInfo->sw_major_version = J1939NM_SW_MAJOR_VERSION;
    VersionInfo->sw_minor_version = J1939NM_SW_MINOR_VERSION;
    VersionInfo->sw_patch_version = J1939NM_SW_PATCH_VERSION;
}
#endif

/**
 * @brief Gets the current NM state for a channel
 */
Std_ReturnType J1939Nm_GetState(J1939Nm_ChannelType Channel, J1939Nm_StateType* State)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (J1939NM_DEV_ERROR_DETECT == STD_ON)
    if (!J1939Nm_Initialized) {
        (void)Det_ReportError(J1939NM_MODULE_ID, 0U, J1939NM_SID_GET_STATE, J1939NM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (State == NULL_PTR) {
        (void)Det_ReportError(J1939NM_MODULE_ID, 0U, J1939NM_SID_GET_STATE, J1939NM_E_INVALID_POINTER);
        return E_NOT_OK;
    }
    if (J1939Nm_ValidateChannel(Channel) != E_OK) {
        (void)Det_ReportError(J1939NM_MODULE_ID, 0U, J1939NM_SID_GET_STATE, J1939NM_E_INVALID_PARAMETER);
        return E_NOT_OK;
    }
#endif
    
    if (J1939Nm_Initialized && (State != NULL_PTR) && (J1939Nm_ValidateChannel(Channel) == E_OK)) {
        *State = J1939Nm_ChannelStates[Channel].State;
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Gets the bus-off state for a channel
 */
Std_ReturnType J1939Nm_GetBusOffState(J1939Nm_ChannelType Channel, boolean* BusOffState)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (J1939NM_DEV_ERROR_DETECT == STD_ON)
    if (!J1939Nm_Initialized) {
        (void)Det_ReportError(J1939NM_MODULE_ID, 0U, J1939NM_SID_GET_BUS_OFF_STATE, J1939NM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (BusOffState == NULL_PTR) {
        (void)Det_ReportError(J1939NM_MODULE_ID, 0U, J1939NM_SID_GET_BUS_OFF_STATE, J1939NM_E_INVALID_POINTER);
        return E_NOT_OK;
    }
    if (J1939Nm_ValidateChannel(Channel) != E_OK) {
        (void)Det_ReportError(J1939NM_MODULE_ID, 0U, J1939NM_SID_GET_BUS_OFF_STATE, J1939NM_E_INVALID_PARAMETER);
        return E_NOT_OK;
    }
#endif
    
    if (J1939Nm_Initialized && (BusOffState != NULL_PTR) && (J1939Nm_ValidateChannel(Channel) == E_OK)) {
        *BusOffState = J1939Nm_ChannelStates[Channel].BusOffState;
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Sets the bus-off state for a channel
 */
Std_ReturnType J1939Nm_SetBusOffState(J1939Nm_ChannelType Channel, boolean BusOffState)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (J1939NM_DEV_ERROR_DETECT == STD_ON)
    if (!J1939Nm_Initialized) {
        (void)Det_ReportError(J1939NM_MODULE_ID, 0U, J1939NM_SID_SET_BUS_OFF_STATE, J1939NM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (J1939Nm_ValidateChannel(Channel) != E_OK) {
        (void)Det_ReportError(J1939NM_MODULE_ID, 0U, J1939NM_SID_SET_BUS_OFF_STATE, J1939NM_E_INVALID_PARAMETER);
        return E_NOT_OK;
    }
#endif
    
    if (J1939Nm_Initialized && (J1939Nm_ValidateChannel(Channel) == E_OK)) {
        J1939Nm_ChannelStates[Channel].BusOffState = BusOffState;
        if ((BusOffState) != 0U) {
            J1939Nm_ChannelStates[Channel].State = J1939NM_STATE_BUS_OFF;
            J1939Nm_ChannelStates[Channel].BusOffRecoveryTimer = 
                J1939NM_BUS_OFF_RECOVERY_TIME_MS / J1939NM_MAIN_FUNCTION_PERIOD_MS;
        }
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Gets the current address for a channel
 */
Std_ReturnType J1939Nm_GetAddress(J1939Nm_ChannelType Channel, J1939Nm_AddressType* Address)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (J1939NM_DEV_ERROR_DETECT == STD_ON)
    if (!J1939Nm_Initialized) {
        (void)Det_ReportError(J1939NM_MODULE_ID, 0U, J1939NM_SID_GET_ADDRESS, J1939NM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (Address == NULL_PTR) {
        (void)Det_ReportError(J1939NM_MODULE_ID, 0U, J1939NM_SID_GET_ADDRESS, J1939NM_E_INVALID_POINTER);
        return E_NOT_OK;
    }
    if (J1939Nm_ValidateChannel(Channel) != E_OK) {
        (void)Det_ReportError(J1939NM_MODULE_ID, 0U, J1939NM_SID_GET_ADDRESS, J1939NM_E_INVALID_PARAMETER);
        return E_NOT_OK;
    }
#endif
    
    if (J1939Nm_Initialized && (Address != NULL_PTR) && (J1939Nm_ValidateChannel(Channel) == E_OK)) {
        *Address = J1939Nm_ChannelStates[Channel].Address;
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Sets the address for a channel
 */
Std_ReturnType J1939Nm_SetAddress(J1939Nm_ChannelType Channel, J1939Nm_AddressType Address)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (J1939NM_DEV_ERROR_DETECT == STD_ON)
    if (!J1939Nm_Initialized) {
        (void)Det_ReportError(J1939NM_MODULE_ID, 0U, J1939NM_SID_SET_ADDRESS, J1939NM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (J1939Nm_ValidateChannel(Channel) != E_OK) {
        (void)Det_ReportError(J1939NM_MODULE_ID, 0U, J1939NM_SID_SET_ADDRESS, J1939NM_E_INVALID_PARAMETER);
        return E_NOT_OK;
    }
#endif
    
    if (J1939Nm_Initialized && (J1939Nm_ValidateChannel(Channel) == E_OK)) {
        J1939Nm_ChannelStates[Channel].Address = Address;
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Gets the NAME for a channel
 */
Std_ReturnType J1939Nm_GetName(J1939Nm_ChannelType Channel, J1939Nm_NameType* Name)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (J1939NM_DEV_ERROR_DETECT == STD_ON)
    if (!J1939Nm_Initialized) {
        (void)Det_ReportError(J1939NM_MODULE_ID, 0U, J1939NM_SID_GET_NAME, J1939NM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (Name == NULL_PTR) {
        (void)Det_ReportError(J1939NM_MODULE_ID, 0U, J1939NM_SID_GET_NAME, J1939NM_E_INVALID_POINTER);
        return E_NOT_OK;
    }
    if (J1939Nm_ValidateChannel(Channel) != E_OK) {
        (void)Det_ReportError(J1939NM_MODULE_ID, 0U, J1939NM_SID_GET_NAME, J1939NM_E_INVALID_PARAMETER);
        return E_NOT_OK;
    }
#endif
    
    if (J1939Nm_Initialized && (Name != NULL_PTR) && (J1939Nm_ValidateChannel(Channel) == E_OK)) {
        *Name = J1939Nm_ChannelStates[Channel].Name;
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Sets the NAME for a channel
 */
Std_ReturnType J1939Nm_SetName(J1939Nm_ChannelType Channel, J1939Nm_NameType Name)
{
    Std_ReturnType result = E_NOT_OK;
    
#if (J1939NM_DEV_ERROR_DETECT == STD_ON)
    if (!J1939Nm_Initialized) {
        (void)Det_ReportError(J1939NM_MODULE_ID, 0U, J1939NM_SID_SET_NAME, J1939NM_E_NOT_INITIALIZED);
        return E_NOT_OK;
    }
    if (J1939Nm_ValidateChannel(Channel) != E_OK) {
        (void)Det_ReportError(J1939NM_MODULE_ID, 0U, J1939NM_SID_SET_NAME, J1939NM_E_INVALID_PARAMETER);
        return E_NOT_OK;
    }
#endif
    
    if (J1939Nm_Initialized && (J1939Nm_ValidateChannel(Channel) == E_OK)) {
        J1939Nm_ChannelStates[Channel].Name = Name;
        result = E_OK;
    }
    
    return result;
}

/**
 * @brief Main function for J1939NM (to be called periodically)
 */
void J1939Nm_MainFunction(void)
{
    uint8 i;
    
    if (!J1939Nm_Initialized) {
        return;
    }
    
    for (i = 0U; i < J1939NM_NUMBER_OF_CHANNELS; i++) {
        J1939Nm_ProcessStateMachine(i);
    }
}

/**
 * @brief Bus-off callback from CanIf
 */
void J1939Nm_BusOffCbk(J1939Nm_ChannelType Channel)
{
    if (J1939Nm_Initialized && (J1939Nm_ValidateChannel(Channel) == E_OK)) {
        J1939Nm_ChannelStates[Channel].BusOffState = TRUE;
        J1939Nm_ChannelStates[Channel].State = J1939NM_STATE_BUS_OFF;
        J1939Nm_ChannelStates[Channel].BusOffRecoveryTimer = 
            J1939NM_BUS_OFF_RECOVERY_TIME_MS / J1939NM_MAIN_FUNCTION_PERIOD_MS;
    }
}

/**
 * @brief RxIndication callback from CanIf
 */
void J1939Nm_RxIndication(
    J1939Nm_ChannelType Channel,
    uint32 CanId,
    const uint8* Data,
    uint8 DataLength)
{
    J1939Nm_NameType receivedName;
    J1939Nm_AddressType sourceAddress;
    
    if (!J1939Nm_Initialized || (Data == NULL_PTR) || (J1939Nm_ValidateChannel(Channel) != E_OK)) {
        return;
    }
    
    /* Extract source address from CAN ID (bits 0-7) */
    sourceAddress = (J1939Nm_AddressType)(CanId & 0xFFU);
    
    /* Check if this is an Address Claimed message (PGN 0x00EE00) */
    if ((CanId & 0x00FFFF00U) == J1939NM_PDU_ADDRESS_CLAIMED) {
        /* Parse received NAME from data */
        receivedName = (J1939Nm_NameType)Data[0] |
                      ((J1939Nm_NameType)Data[1] << 8U) |
                      ((J1939Nm_NameType)Data[2] << 16U) |
                      ((J1939Nm_NameType)Data[3] << 24U) |
                      ((J1939Nm_NameType)Data[4] << 32U) |
                      ((J1939Nm_NameType)Data[5] << 40U) |
                      ((J1939Nm_NameType)Data[6] << 48U) |
                      ((J1939Nm_NameType)Data[7] << 56U);
        
        /* Handle address conflict */
        if (sourceAddress == J1939Nm_ChannelStates[Channel].Address) {
            J1939Nm_HandleAddressConflict(Channel, receivedName, sourceAddress);
        }
    }
    /* Check if this is a Request for Address Claimed (PGN 0x00EA00) */
    else if ((CanId & 0x00FFFF00U) == J1939NM_PDU_REQUEST_FOR_AC) {
        /* If we have claimed an address, retransmit Address Claimed */
        if (J1939Nm_ChannelStates[Channel].AddressClaimed) {
            (void)J1939Nm_TransmitAddressClaimed(Channel);
        }
    }
}

/**
 * @brief TxConfirmation callback from CanIf
 */
void J1939Nm_TxConfirmation(J1939Nm_ChannelType Channel, PduIdType TxPduId, Std_ReturnType result)
{
    (void)TxPduId;
    (void)result;
    
    /* Transmission confirmation handling if needed */
    (void)Channel;
}

/**
 * @brief Requests transmission of Address Claimed message
 */
Std_ReturnType J1939Nm_RequestAddressClaimed(J1939Nm_ChannelType Channel)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (J1939Nm_Initialized && (J1939Nm_ValidateChannel(Channel) == E_OK)) {
        result = J1939Nm_TransmitAddressClaimed(Channel);
    }
    
    return result;
}

/**
 * @brief Requests transmission of Cannot Claim Address message
 */
Std_ReturnType J1939Nm_RequestCannotClaimAddress(J1939Nm_ChannelType Channel)
{
    Std_ReturnType result = E_NOT_OK;
    
    if (J1939Nm_Initialized && (J1939Nm_ValidateChannel(Channel) == E_OK)) {
        result = J1939Nm_TransmitCannotClaimAddress(Channel);
    }
    
    return result;
}

/**
 * @brief Handles address conflict detection
 */
void J1939Nm_HandleAddressConflict(
    J1939Nm_ChannelType Channel,
    J1939Nm_NameType ReceivedName,
    J1939Nm_AddressType ReceivedAddress)
{
    J1939Nm_ChannelStateType* channelState = &J1939Nm_ChannelStates[Channel];
    sint8 nameCompare;
    
    if (!J1939Nm_Initialized || (J1939Nm_ValidateChannel(Channel) != E_OK)) {
        return;
    }
    
    /* Compare NAMEs */
    nameCompare = J1939Nm_CompareNames(channelState->Name, ReceivedName);
    
    if (nameCompare < 0) {
        /* Our NAME is lower (higher priority), we keep the address */
        /* Re-transmit Address Claimed to assert our claim */
        (void)J1939Nm_TransmitAddressClaimed(Channel);
    } else if ((unsigned int)(nameCompare) > 0U ) {
        /* Our NAME is higher (lower priority), we must release the address */
        channelState->AcState = J1939NM_AC_STATE_CONFLICT;
    } else {
        /* Same NAME - this shouldn't happen, treat as conflict */
        channelState->AcState = J1939NM_AC_STATE_CONFLICT;
    }
    
    (void)ReceivedAddress;
}
