/*
 * CanIf.h
 * CAN Interface Module Header
 * AUTOSAR-compliant implementation
 */

#ifndef CANIF_H
#define CANIF_H

#include "CanIf_Cfg.h"
#include "Can.h"

/*=============================================================================
 * Version information
 *=============================================================================*/

#define CANIF_VENDOR_ID             0x00U
#define CANIF_MODULE_ID             0x1CU
#define CANIF_INSTANCE_ID           0x00U

#define CANIF_SW_MAJOR_VERSION      1U
#define CANIF_SW_MINOR_VERSION      0U
#define CANIF_SW_PATCH_VERSION      0U

/*=============================================================================
 * Type definitions
 *=============================================================================*/

/* L-PDU info type for transmission */
typedef struct
{
    uint8* sdu;              /* Pointer to data buffer */
    uint8 length;            /* Data length (0-8 bytes) */
} CanIf_PduInfoType;

/*=============================================================================
 * Service APIs
 *=============================================================================*/

/**
 * Initializes the CAN Interface module
 * @param configPtr - Pointer to configuration (NULL for post-build selectable)
 */
void CanIf_Init(const void* configPtr);

/**
 * De-initializes the CAN Interface module
 */
void CanIf_DeInit(void);

/**
 * Sets the mode of a CAN controller
 * @param controllerId - CAN controller ID
 * @param mode - Target mode (STARTED/STOPPED/SLEEP)
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType CanIf_SetControllerMode(uint8 controllerId, 
                                        CanIf_ControllerModeType mode);

/**
 * Gets the current mode of a CAN controller
 * @param controllerId - CAN controller ID
 * @param modePtr - Pointer to store the current mode
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType CanIf_GetControllerMode(uint8 controllerId,
                                        CanIf_ControllerModeType* modePtr);

/**
 * Transmits an L-PDU over CAN
 * @param txPduId - Tx L-PDU ID
 * @param pduInfoPtr - Pointer to PDU data
 * @return E_OK if accepted, E_NOT_OK otherwise
 */
Std_ReturnType CanIf_Transmit(CanIf_PduIdType txPduId,
                               const CanIf_PduInfoType* pduInfoPtr);

/**
 * Sets the PDU mode for a controller
 * @param controllerId - CAN controller ID
 * @param pduModeRequest - Requested PDU mode
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType CanIf_SetPduMode(uint8 controllerId, 
                                 CanIf_PduModeType pduModeRequest);

/**
 * Gets the current PDU mode for a controller
 * @param controllerId - CAN controller ID
 * @param pduModePtr - Pointer to store current PDU mode
 * @return E_OK if successful, E_NOT_OK otherwise
 */
Std_ReturnType CanIf_GetPduMode(uint8 controllerId,
                                 CanIf_PduModeType* pduModePtr);

#if (CANIF_VERSION_INFO_API == STD_ON)
/**
 * Returns version information of the CAN Interface module
 * @param versionInfo - Pointer to store version info
 */
void CanIf_GetVersionInfo(Std_VersionInfoType* versionInfo);
#endif

/*=============================================================================
 * Callback Functions (called by CAN Driver)
 *=============================================================================*/

/**
 * Rx indication callback from CAN driver
 * @param hoh - Hardware object handle that received the frame
 * @param canId - CAN identifier of received frame
 * @param canDlc - Data length code
 * @param canSduPtr - Pointer to received data
 */
void CanIf_RxIndication(CanIf_HohType hoh,
                         CanIf_CanIdType canId,
                         uint8 canDlc,
                         const uint8* canSduPtr);

/**
 * Tx confirmation callback from CAN driver
 * @param hth - Hardware transmit handle
 */
void CanIf_TxConfirmation(CanIf_HthType hth);

/**
 * Controller mode indication callback from CAN driver
 * @param controllerId - CAN controller ID
 * @param mode - New controller mode
 */
void CanIf_ControllerModeIndication(uint8 controllerId,
                                     CanIf_ControllerModeType mode);

/**
 * Bus-off indication callback from CAN driver
 * @param controllerId - CAN controller ID
 */
void CanIf_ControllerBusOff(uint8 controllerId);

/*=============================================================================
 * User Callbacks (to be implemented by upper layers)
 *=============================================================================*/

/**
 * User Rx indication callback - must be provided by upper layer
 * @param rxPduId - Rx L-PDU ID
 * @param pduInfoPtr - Pointer to received PDU info
 */
extern void CanIf_UserRxIndication(CanIf_PduIdType rxPduId,
                                    const CanIf_PduInfoType* pduInfoPtr);

/**
 * User Tx confirmation callback - must be provided by upper layer
 * @param txPduId - Tx L-PDU ID
 */
extern void CanIf_UserTxConfirmation(CanIf_PduIdType txPduId);

/**
 * User controller mode indication - optional
 * @param controllerId - CAN controller ID
 * @param mode - New controller mode
 */
extern void CanIf_UserControllerModeIndication(uint8 controllerId,
                                                CanIf_ControllerModeType mode);

/**
 * User bus-off indication - optional
 * @param controllerId - CAN controller ID
 */
extern void CanIf_UserBusOffIndication(uint8 controllerId);

/*=============================================================================
 * Internal Types (exposed for testing/integration)
 *=============================================================================*/

/* Controller runtime state */
typedef struct
{
    CanIf_ControllerModeType mode;
    CanIf_PduModeType pduMode;
    boolean initialized;
} CanIf_ControllerStateType;

/* Module runtime state */
typedef struct
{
    boolean initialized;
    CanIf_ControllerStateType controllerState[CANIF_CONTROLLER_CNT];
} CanIf_StateType;

/* Module state (extern for external access) */
extern CanIf_StateType CanIf_State;

#endif /* CANIF_H */
