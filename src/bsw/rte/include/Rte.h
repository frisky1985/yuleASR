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

#endif /* RTE_H */
