/**
 * @file Port.h
 * @brief Port Driver - AutoSAR MCAL Module
 * @version 1.0.0
 * @date 2026-04-14
 * @author YuleTech
 * 
 * @copyright Copyright (c) 2026 YuleTech
 * 
 * @details Provides access to basic microcontroller port functionality.
 *          This includes the initialization of the port structure and 
 *          the configuration of individual port pins.
 */

#ifndef PORT_H
#define PORT_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Port_Cfg.h"

/*==================================================================================================
*                                      DEFINES AND MACROS
==================================================================================================*/
/** @brief Port Module Vendor ID */
#define PORT_VENDOR_ID                          0x0055U  /* YuleTech */

/** @brief Port Module ID */
#define PORT_MODULE_ID                          0x0074U

/** @brief Port Instance ID */
#define PORT_INSTANCE_ID                        0x00U

/* API Service IDs */
#define PORT_API_ID_INIT                        0x00U
#define PORT_API_ID_SET_PIN_DIRECTION           0x01U
#define PORT_API_ID_REFRESH_PORT_DIRECTION      0x02U
#define PORT_API_ID_GET_VERSION_INFO            0x03U
#define PORT_API_ID_SET_PIN_MODE                0x04U

/* Error Codes */
#define PORT_E_PARAM_PIN                        0x0AU
#define PORT_E_DIRECTION_UNCHANGEABLE           0x0BU
#define PORT_E_PARAM_CONFIG                     0x0CU
#define PORT_E_PARAM_INVALID_MODE               0x0DU
#define PORT_E_MODE_UNCHANGEABLE                0x0EU
#define PORT_E_UNINIT                           0x0FU
#define PORT_E_PARAM_POINTER                    0x10U

/*==================================================================================================
*                                          TYPE DEFINITIONS
==================================================================================================*/
/** @brief Numeric ID of a port pin */
typedef uint16 Port_PinType;

/** @brief Possible directions of a port pin */
typedef enum {
    PORT_PIN_IN = 0,            /**< Port pin direction set as input */
    PORT_PIN_OUT                /**< Port pin direction set as output */
} Port_PinDirectionType;

/** @brief Different port pin modes */
typedef uint8 Port_PinModeType;

/** @brief Port pin level type */
typedef enum {
    PORT_PIN_LEVEL_LOW = 0,     /**< Port pin level low */
    PORT_PIN_LEVEL_HIGH = 1     /**< Port pin level high */
} Port_PinLevelType;

/** @brief Port pin configuration structure */
typedef struct {
    Port_PinType Pin;                           /**< Port pin ID */
    Port_PinDirectionType Direction;            /**< Port pin direction */
    Port_PinModeType Mode;                      /**< Port pin mode */
    boolean DirectionChangeable;                /**< Direction changeable during runtime */
    boolean ModeChangeable;                     /**< Mode changeable during runtime */
    Port_PinLevelType InitialLevel;             /**< Initial pin level for output */
    boolean PullUpEnable;                       /**< Pull-up enable */
    boolean PullDownEnable;                     /**< Pull-down enable */
} Port_PinConfigType;

/** @brief Port configuration structure */
typedef struct {
    uint16 NumPins;                             /**< Number of configured pins */
    const Port_PinConfigType* PinConfig;        /**< Pointer to pin configurations */
} Port_ConfigType;

/*==================================================================================================
*                                      GLOBAL CONSTANTS
==================================================================================================*/
#define PORT_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/** @brief External configuration structure */
extern const Port_ConfigType Port_Config;

#define PORT_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "MemMap.h"

/*==================================================================================================
*                                   FUNCTION PROTOTYPES
==================================================================================================*/
#define PORT_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Initializes the Port Driver module
 * 
 * @param[in] ConfigPtr Pointer to configuration set
 * 
 * @pre None
 * @post Port Driver initialized
 * 
 * @note This function must be called before any other Port function
 */
void Port_Init(const Port_ConfigType* ConfigPtr);

/**
 * @brief Sets the port pin direction
 * 
 * @param[in] Pin Port pin ID number
 * @param[in] Direction Port pin direction (input/output)
 * 
 * @pre Port Driver initialized
 * @post Pin direction set
 * 
 * @note Only available if PORT_SET_PIN_DIRECTION_API is STD_ON
 */
#if (PORT_SET_PIN_DIRECTION_API == STD_ON)
void Port_SetPinDirection(Port_PinType Pin, Port_PinDirectionType Direction);
#endif

/**
 * @brief Refreshes port direction
 * 
 * @pre Port Driver initialized
 * @post All port pins refreshed
 * 
 * @note Used to refresh the direction of all configured ports to the configured direction
 */
void Port_RefreshPortDirection(void);

/**
 * @brief Gets the version information of this module
 * 
 * @param[out] versioninfo Pointer to where to store the version information
 * 
 * @pre None
 * @post Version information stored
 * 
 * @note Only available if PORT_VERSION_INFO_API is STD_ON
 */
#if (PORT_VERSION_INFO_API == STD_ON)
void Port_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/**
 * @brief Sets the port pin mode
 * 
 * @param[in] Pin Port pin ID number
 * @param[in] Mode New port pin mode to be set
 * 
 * @pre Port Driver initialized
 * @post Pin mode set
 * 
 * @note Only available if PORT_SET_PIN_MODE_API is STD_ON
 */
#if (PORT_SET_PIN_MODE_API == STD_ON)
void Port_SetPinMode(Port_PinType Pin, Port_PinModeType Mode);
#endif

#define PORT_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                      INLINE FUNCTIONS
==================================================================================================*/

#endif /* PORT_H */
