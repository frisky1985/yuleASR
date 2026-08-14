/**
 * @file Rte.h
 * @brief RTE Core API following AutoSAR Classic Platform 4.x standard
 * @version 1.0.0
 * @date 2026-04-14
 * @author Shanghai Yule Electronics Technology Co., Ltd.
 * @copyright Copyright (c) 2026 Shanghai Yule Electronics Technology Co., Ltd.
 *
 * AutoSAR Standard: Runtime Environment (RTE)
 * Layer: RTE (Runtime Environment)
 * Purpose: Core RTE API for component communication
 */

#ifndef RTE_H
#define RTE_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Rte_Type.h"
#include "Compiler.h"

/*==================================================================================================
*                                    VERSION INFORMATION
==================================================================================================*/
#define RTE_VENDOR_ID                   (0x01U) /* YuleTech Vendor ID */
#define RTE_MODULE_ID                   (0x70U) /* RTE Module ID */
#define RTE_AR_RELEASE_MAJOR_VERSION    (0x04U)
#define RTE_AR_RELEASE_MINOR_VERSION    (0x04U)
#define RTE_AR_RELEASE_REVISION_VERSION (0x00U)
#define RTE_SW_MAJOR_VERSION            (0x01U)
#define RTE_SW_MINOR_VERSION            (0x00U)
#define RTE_SW_PATCH_VERSION            (0x00U)

/*==================================================================================================
*                                    RTE SERVICE IDs
==================================================================================================*/
#define RTE_SID_START                   (0x01U)
#define RTE_SID_STOP                    (0x02U)
#define RTE_SID_INIT                    (0x03U)
#define RTE_SID_GETVERSIONINFO          (0x04U)
#define RTE_SID_SWITCHAPI               (0x05U)
#define RTE_SID_MODEAPI                 (0x06U)
#define RTE_SID_IRVAPI                  (0x07U)
#define RTE_SID_PIMREAD                 (0x08U)
#define RTE_SID_PIMWRITE                (0x09U)
#define RTE_SID_CALPRMREAD              (0x0AU)
#define RTE_SID_IOCPREAD                (0x0BU)
#define RTE_SID_IOCPWRITE               (0x0CU)
#define RTE_SID_COMCBK                  (0x0DU)
#define RTE_SID_MAINFUNCTION            (0x0EU)
#define RTE_SID_READ                    (0x0FU)
#define RTE_SID_WRITE                   (0x10U)
#define RTE_SID_CS_CALL                 (0x11U)
#define RTE_SID_CS_OPERATION            (0x12U)

/*==================================================================================================
*                                    RTE DET ERROR CODES
==================================================================================================*/
#define RTE_E_UNINIT                    (0x01U)
#define RTE_E_INVALID                   (0x02U)
#define RTE_E_UNCONNECTED               (0x03U)
#define RTE_E_TIMEOUT                   (0x04U)
#define RTE_E_LIMIT                     (0x05U)
#define RTE_E_NO_DATA                   (0x06U)
#define RTE_E_SEG_FAULT                 (0x07U)
#define RTE_E_OUT_OF_RANGE              (0x08U)
#define RTE_E_SERIALIZATION_ERROR       (0x09U)
#define RTE_E_HARD_TRANSFORMER_ERROR    (0x0AU)
#define RTE_E_SOFT_TRANSFORMER_ERROR    (0x0BU)
#define RTE_E_COM_STOPPED               (0x0CU)
#define RTE_E_IN_EXCLUSIVE_AREA         (0x0DU)
#define RTE_E_INTER_PARTITION_ROUTING_NOT_AVAILABLE (0x0EU)

/*==================================================================================================
*                                    RTE CORE FUNCTION PROTOTYPES
==================================================================================================*/
#define RTE_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the RTE
 * @return RTE status
 */
extern Rte_StatusType Rte_Init(void);

/**
 * @brief Starts the RTE
 * @return RTE status
 */
extern Rte_StatusType Rte_Start(void);

/**
 * @brief Stops the RTE
 * @return RTE status
 */
extern Rte_StatusType Rte_Stop(void);

/**
 * @brief Gets version information
 * @param versioninfo Pointer to version info structure
 */
extern void Rte_GetVersionInfo(Std_VersionInfoType* versioninfo);

/**
 * @brief Initialize a component instance
 * @param componentId Component ID
 * @param numPorts Number of ports
 * @return RTE status
 */
extern Rte_StatusType Rte_InitComponent(uint8 componentId, uint8 numPorts);

/**
 * @brief Connect a port
 * @param portHandle Port handle
 * @param direction Port direction (0=Sender, 1=Receiver)
 * @param dataLength Data length
 * @return RTE status
 */
extern Rte_StatusType Rte_ConnectPort(Rte_PortHandleType portHandle, uint8 direction, uint16 dataLength);

/*==================================================================================================
*                                    RTE EXCLUSIVE AREA API
==================================================================================================*/

/**
 * @brief Enters an exclusive area
 * @param exclusiveArea Handle to exclusive area
 */
extern void Rte_EnterExclusiveArea(Rte_ExclusiveAreaHandleType exclusiveArea);

/**
 * @brief Exits an exclusive area
 * @param exclusiveArea Handle to exclusive area
 */
extern void Rte_ExitExclusiveArea(Rte_ExclusiveAreaHandleType exclusiveArea);

/*==================================================================================================
*                                    RTE MODE MANAGEMENT API
==================================================================================================*/

/**
 * @brief Switches mode
 * @param modeGroup Mode group handle
 * @param mode Mode to switch to
 * @return RTE status
 */
extern Rte_StatusType Rte_Switch(Rte_ModeHandleType modeGroup, uint32 mode);

/**
 * @brief Gets current mode
 * @param modeGroup Mode group handle
 * @param mode Pointer to store current mode
 * @return RTE status
 */
extern Rte_StatusType Rte_Mode(Rte_ModeHandleType modeGroup, uint32* mode);

/**
 * @brief Notifies mode switch completion
 * @param modeGroup Mode group handle
 * @param mode New mode
 */
extern void Rte_SwitchAck(Rte_ModeHandleType modeGroup, uint32 mode);

/*==================================================================================================
*                                    RTE INTER-RUNNABLE VARIABLE API
==================================================================================================*/

/**
 * @brief Reads inter-runnable variable
 * @param irvHandle IRV handle
 * @param data Pointer to store data
 * @return RTE status
 */
extern Rte_StatusType Rte_IrvRead(Rte_IrvHandleType irvHandle, void* data);

/**
 * @brief Writes inter-runnable variable
 * @param irvHandle IRV handle
 * @param data Pointer to data
 * @return RTE status
 */
extern Rte_StatusType Rte_IrvWrite(Rte_IrvHandleType irvHandle, const void* data);

/*==================================================================================================
*                                    RTE PER-INSTANCE MEMORY API
==================================================================================================*/

/**
 * @brief Reads per-instance memory
 * @param pimHandle PIM handle
 * @param data Pointer to store data
 * @return RTE status
 */
extern Rte_StatusType Rte_PimRead(Rte_PimHandleType pimHandle, void* data);

/**
 * @brief Writes per-instance memory
 * @param pimHandle PIM handle
 * @param data Pointer to data
 * @return RTE status
 */
extern Rte_StatusType Rte_PimWrite(Rte_PimHandleType pimHandle, const void* data);

/**
 * @brief Gets PIM address
 * @param pimHandle PIM handle
 * @return Pointer to PIM data
 */
extern void* Rte_PimAddr(Rte_PimHandleType pimHandle);

/*==================================================================================================
*                                    RTE CALIBRATION PARAMETER API
==================================================================================================*/

/**
 * @brief Reads calibration parameter
 * @param calPrmHandle Calibration parameter handle
 * @param data Pointer to store data
 * @return RTE status
 */
extern Rte_StatusType Rte_CalPrmRead(Rte_CalPrmHandleType calPrmHandle, void* data);

/**
 * @brief Gets calibration parameter address
 * @param calPrmHandle Calibration parameter handle
 * @return Pointer to calibration parameter
 */
extern const void* Rte_CalPrmAddr(Rte_CalPrmHandleType calPrmHandle);

/*==================================================================================================
*                                    RTE MEASUREMENT API
==================================================================================================*/

/**
 * @brief Reads measurement data
 * @param measurementHandle Measurement handle
 * @param data Pointer to store data
 * @return RTE status
 */
extern Rte_StatusType Rte_MeasurementRead(Rte_MeasurementHandleType measurementHandle, void* data);

/**
 * @brief Writes measurement data
 * @param measurementHandle Measurement handle
 * @param data Pointer to data
 * @return RTE status
 */
extern Rte_StatusType Rte_MeasurementWrite(Rte_MeasurementHandleType measurementHandle, const void* data);

/*==================================================================================================
*                                    RTE COMPONENT LIFE CYCLE API
==================================================================================================*/

/**
 * @brief Initializes a component instance
 * @param instance Component instance handle
 * @return RTE status
 */
extern Rte_StatusType Rte_ComponentInit(Rte_InstanceHandleType instance);

/**
 * @brief Deinitializes a component instance
 * @param instance Component instance handle
 * @return RTE status
 */
extern Rte_StatusType Rte_ComponentDeinit(Rte_InstanceHandleType instance);

/*==================================================================================================
*                                    RTE RUNNABLE API
==================================================================================================*/

/**
 * @brief Activates a runnable
 * @param instance Component instance handle
 * @param runnableId Runnable ID
 * @return RTE status
 */
extern Rte_StatusType Rte_RunnableActivate(Rte_InstanceHandleType instance, uint8 runnableId);

/**
 * @brief Terminates a runnable
 * @param instance Component instance handle
 * @param runnableId Runnable ID
 * @return RTE status
 */
extern Rte_StatusType Rte_RunnableTerminate(Rte_InstanceHandleType instance, uint8 runnableId);

/**
 * @brief Waits for an event
 * @param instance Component instance handle
 * @param eventMask Event mask to wait for
 * @param timeout Timeout in milliseconds
 * @return RTE status
 */
extern Rte_StatusType Rte_WaitForEvent(Rte_InstanceHandleType instance, Rte_EventType eventMask, uint32 timeout);

/**
 * @brief Sets an event
 * @param instance Component instance handle
 * @param event Event to set
 * @return RTE status
 */
extern Rte_StatusType Rte_SetEvent(Rte_InstanceHandleType instance, Rte_EventType event);

/**
 * @brief Clears an event
 * @param instance Component instance handle
 * @param event Event to clear
 * @return RTE status
 */
extern Rte_StatusType Rte_ClearEvent(Rte_InstanceHandleType instance, Rte_EventType event);

/*==================================================================================================
*                                    RTE MAIN FUNCTION
==================================================================================================*/

/**
 * @brief RTE main function for periodic processing
 */
extern void Rte_MainFunction(void);

/*==================================================================================================
*                                    RTE CALLBACK FUNCTIONS
==================================================================================================*/

/**
 * @brief COM callback for data reception
 * @param dataHandle Data handle
 */
extern void Rte_ComCbk(Rte_DataHandleType dataHandle);

/**
 * @brief COM callback for data reception with timeout
 * @param dataHandle Data handle
 */
extern void Rte_ComCbkTout(Rte_DataHandleType dataHandle);

/**
 * @brief COM callback for data invalidation
 * @param dataHandle Data handle
 */
extern void Rte_ComCbkInv(Rte_DataHandleType dataHandle);

/**
 * @brief COM callback for mode switch notification
 * @param modeGroup Mode group handle
 * @param mode New mode
 */
extern void Rte_ComCbkSwitchAck(Rte_ModeHandleType modeGroup, uint32 mode);

#define RTE_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                    RTE COMPONENT API MACROS
==================================================================================================*/

/**
 * @brief Macro to declare RTE API for a component
 */
#define RTE_COMPONENT_API(componentName) \
    extern Rte_StatusType Rte_##componentName##Init(void); \
    extern Rte_StatusType Rte_##componentName##Start(void); \
    extern Rte_StatusType Rte_##componentName##Stop(void)

/**
 * @brief Macro to declare sender-receiver interface read
 */
#define RTE_SR_READ(componentName, portName, dataElementName, data) \
    Rte_Read_##componentName##_##portName##_##dataElementName(data)

/**
 * @brief Macro to declare sender-receiver interface write
 */
#define RTE_SR_WRITE(componentName, portName, dataElementName, data) \
    Rte_Write_##componentName##_##portName##_##dataElementName(data)

/**
 * @brief Macro to declare client-server interface call
 */
#define RTE_CS_CALL(componentName, portName, operationName, ...) \
    Rte_Call_##componentName##_##portName##_##operationName(__VA_ARGS__)

/**
 * @brief Macro to declare mode switch interface
 */
#define RTE_MODE_SWITCH(componentName, portName, modeGroup, mode) \
    Rte_Switch_##componentName##_##portName##_##modeGroup(mode)

/**
 * @brief Macro to declare trigger interface
 */
#define RTE_TRIGGER(componentName, portName, triggerName) \
    Rte_Trigger_##componentName##_##portName##_##triggerName()

/**
 * @brief Macro to declare parameter interface read
 */
#define RTE_PARAM_READ(componentName, portName, paramName, data) \
    Rte_Param_##componentName##_##portName##_##paramName(data)

/**
 * @brief Macro to declare inter-runnable variable read
 */
#define RTE_IRV_READ(componentName, irvName, data) \
    Rte_IrvRead_##componentName##_##irvName(data)

/**
 * @brief Macro to declare inter-runnable variable write
 */
#define RTE_IRV_WRITE(componentName, irvName, data) \
    Rte_IrvWrite_##componentName##_##irvName(data)

/**
 * @brief Macro to declare per-instance memory read
 */
#define RTE_PIM_READ(componentName, pimName, data) \
    Rte_PimRead_##componentName##_##pimName(data)

/**
 * @brief Macro to declare per-instance memory write
 */
#define RTE_PIM_WRITE(componentName, pimName, data) \
    Rte_PimWrite_##componentName##_##pimName(data)

/**
 * @brief Macro to declare per-instance memory address
 */
#define RTE_PIM_ADDR(componentName, pimName) \
    Rte_PimAddr_##componentName##_##pimName()

/**
 * @brief Macro to declare calibration parameter read
 */
#define RTE_CALPRM_READ(componentName, calPrmName, data) \
    Rte_CalPrmRead_##componentName##_##calPrmName(data)

/**
 * @brief Macro to declare calibration parameter address
 */
#define RTE_CALPRM_ADDR(componentName, calPrmName) \
    Rte_CalPrmAddr_##componentName##_##calPrmName()

/*==================================================================================================
*                                    RTE ASW COMPONENT API DECLARATIONS
*   Generated API for Rte_Read, Rte_Write, and Rte_Call functions
*   for the 8 ASW components.
==================================================================================================*/

/* ---- EngineControl ---- */
extern Std_ReturnType Rte_Read_EngineControl_Port_RPM(uint16* data);
extern Std_ReturnType Rte_Read_EngineControl_Port_Speed(uint16* data);
extern Std_ReturnType Rte_Read_EngineControl_Port_Temperature(uint8* data);
extern Std_ReturnType Rte_Write_EngineControl_Port_Throttle(const uint16* data);
extern Std_ReturnType Rte_Write_EngineControl_Port_FuelInjection(const uint16* data);

/* ---- VehicleDynamics ---- */
extern Std_ReturnType Rte_Read_VehicleDynamics_Port_YawRate(float32* data);
extern Std_ReturnType Rte_Read_VehicleDynamics_Port_WheelSpeed(uint16* data);
extern Std_ReturnType Rte_Read_VehicleDynamics_Port_SteeringAngle(uint16* data);

/* ---- DiagnosticManager ---- */
extern Std_ReturnType Rte_Read_DiagnosticManager_Port_DTCStatus(uint8* data);
extern Std_ReturnType Rte_Call_DiagnosticManager_Port_ClearDTC(uint32 dtc);
extern Std_ReturnType Rte_Call_DiagnosticManager_Port_ReadDTC(uint32 dtc, uint8* status);

/* ---- CommunicationManager ---- */
extern Std_ReturnType Rte_Read_CommunicationManager_Port_VehicleSpeed(uint16* data);
extern Std_ReturnType Rte_Write_CommunicationManager_Port_Status(const uint8* data);

/* ---- StorageManager ---- */
extern Std_ReturnType Rte_Read_StorageManager_Port_Data(uint8* data, uint16* length);
extern Std_ReturnType Rte_Write_StorageManager_Port_Data(const uint8* data, uint16 length);

/* ---- IOControl ---- */
extern Std_ReturnType Rte_Read_IOControl_Port_Input(uint16* data);
extern Std_ReturnType Rte_Write_IOControl_Port_Output(const uint16* data);

/* ---- ModeManager ---- */
extern Std_ReturnType Rte_Read_ModeManager_Port_CurrentMode(uint8* data);
extern Std_ReturnType Rte_Write_ModeManager_Port_TargetMode(const uint8* data);

/* ---- WatchdogManager ---- */
extern Std_ReturnType Rte_Read_WatchdogManager_Port_Status(uint8* data);
extern Std_ReturnType Rte_Call_WatchdogManager_Port_Reset(void);
extern Std_ReturnType Rte_Call_WatchdogManager_Port_Trigger(void);


/*==================================================================================================
*                                    CORE RTE OPERATIONS
*   Implemented in Rte.c / Rte_ComInterface.c / Rte_NvMInterface.c
==================================================================================================*/
extern Std_ReturnType Rte_Read(Rte_PortHandleType portHandle, void* data);
extern Std_ReturnType Rte_Write(Rte_PortHandleType portHandle, const void* data);
extern Std_ReturnType Rte_ComSendSignal(uint16 comSignalId, const void* signalData);
extern Std_ReturnType Rte_NvmReadBlock(uint16 blockId, void* dataPtr);
extern Std_ReturnType Rte_NvmWriteBlock(uint16 blockId, const void* dataPtr);
extern uint32 Rte_GetTime(void);

/*==================================================================================================
*                                    GENERATED SWC PORT API
*   Per-SWC Read/Write/Switch entry points referenced by the SWC interface
*   macros in each component header. Generic pointer signatures are used so
*   the declarations stay valid for all port data types.
==================================================================================================*/
extern Std_ReturnType Rte_Read_SWC_COMMUNICATIONMANAGER_PORT_PDU_DATA_R(void* data);
extern Std_ReturnType Rte_Read_SWC_DIAGNOSTICMANAGER_PORT_DIAG_REQUEST_R(void* data);
extern Std_ReturnType Rte_Read_SWC_ENGINECONTROL_PORT_COOLANT_TEMP_R(void* data);
extern Std_ReturnType Rte_Read_SWC_ENGINECONTROL_PORT_THROTTLE_POS_R(void* data);
extern Std_ReturnType Rte_Read_SWC_ENGINECONTROL_PORT_VEHICLE_SPEED_R(void* data);
extern Std_ReturnType Rte_Read_SWC_IOCONTROL_PORT_ANALOG_INPUT_R(void* data);
extern Std_ReturnType Rte_Read_SWC_IOCONTROL_PORT_DIGITAL_INPUT_R(void* data);
extern Std_ReturnType Rte_Read_SWC_IOCONTROL_PORT_PWM_INPUT_R(void* data);
extern Std_ReturnType Rte_Read_SWC_MODEMANAGER_PORT_MODE_REQUEST_R(void* data);
extern Std_ReturnType Rte_Read_SWC_VEHICLEDYNAMICS_PORT_ACCEL_DATA_R(void* data);
extern Std_ReturnType Rte_Read_SWC_VEHICLEDYNAMICS_PORT_STEERING_ANGLE_R(void* data);
extern Std_ReturnType Rte_Read_SWC_VEHICLEDYNAMICS_PORT_WHEEL_SPEEDS_R(void* data);
extern Std_ReturnType Rte_Read_SWC_WATCHDOGMANAGER_PORT_ALIVE_INDICATION_R(void* data);
extern Std_ReturnType Rte_Switch_SWC_ENGINECONTROL_PORT_MODE_P(uint32 data);
extern Std_ReturnType Rte_Write_SWC_COMMUNICATIONMANAGER_PORT_COMM_STATE_P(const void* data);
extern Std_ReturnType Rte_Write_SWC_COMMUNICATIONMANAGER_PORT_PDU_DATA_P(const void* data);
extern Std_ReturnType Rte_Write_SWC_DIAGNOSTICMANAGER_PORT_DIAG_RESPONSE_P(const void* data);
extern Std_ReturnType Rte_Write_SWC_DIAGNOSTICMANAGER_PORT_SECURITY_P(const void* data);
extern Std_ReturnType Rte_Write_SWC_DIAGNOSTICMANAGER_PORT_SESSION_P(const void* data);
extern Std_ReturnType Rte_Write_SWC_ENGINECONTROL_PORT_ENGINE_CONTROL_P(const void* data);
extern Std_ReturnType Rte_Write_SWC_ENGINECONTROL_PORT_ENGINE_PARAMS_P(const void* data);
extern Std_ReturnType Rte_Write_SWC_ENGINECONTROL_PORT_ENGINE_STATE_P(const void* data);
extern Std_ReturnType Rte_Write_SWC_IOCONTROL_PORT_ANALOG_OUTPUT_P(const void* data);
extern Std_ReturnType Rte_Write_SWC_IOCONTROL_PORT_DIGITAL_OUTPUT_P(const void* data);
extern Std_ReturnType Rte_Write_SWC_IOCONTROL_PORT_IO_STATE_P(const void* data);
extern Std_ReturnType Rte_Write_SWC_IOCONTROL_PORT_PWM_OUTPUT_P(const void* data);
extern Std_ReturnType Rte_Write_SWC_MODEMANAGER_PORT_MODE_NOTIFICATION_P(const void* data);
extern Std_ReturnType Rte_Write_SWC_MODEMANAGER_PORT_SYSTEM_MODE_P(const void* data);
extern Std_ReturnType Rte_Write_SWC_MODEMANAGER_PORT_SYSTEM_STATE_P(const void* data);
extern Std_ReturnType Rte_Write_SWC_STORAGEMANAGER_PORT_BLOCK_STATUS_P(const void* data);
extern Std_ReturnType Rte_Write_SWC_VEHICLEDYNAMICS_PORT_MOTION_DATA_P(const void* data);
extern Std_ReturnType Rte_Write_SWC_VEHICLEDYNAMICS_PORT_VDC_OUTPUT_P(const void* data);
extern Std_ReturnType Rte_Write_SWC_VEHICLEDYNAMICS_PORT_VDC_STATE_P(const void* data);
extern Std_ReturnType Rte_Write_SWC_WATCHDOGMANAGER_PORT_WDG_STATUS_P(const void* data);
extern Std_ReturnType Rte_Write_SWC_WATCHDOGMANAGER_PORT_WDG_TRIGGER_P(const void* data);

#endif /* RTE_H */
