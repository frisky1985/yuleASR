/**
 * @file Dio.h
 * @brief DIO Driver - AutoSAR MCAL Module
 * @version 1.0.0
 * @date 2026-04-14
 * @author YuleTech
 *
 * @copyright Copyright (c) 2026 YuleTech
 *
 * @details Provides services for reading and writing to DIO channels,
 *          DIO ports and DIO channel groups.
 */

#ifndef DIO_H
#define DIO_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "Std_Types.h"
#include "Dio_Cfg.h"

/*==================================================================================================
*                                      DEFINES AND MACROS
==================================================================================================*/
/** @brief DIO Module Vendor ID */
#define DIO_VENDOR_ID                           0x0055U  /* YuleTech */

/** @brief DIO Module ID */
#define DIO_MODULE_ID                           0x0020U

/** @brief DIO Instance ID */
#define DIO_INSTANCE_ID                         0x00U

/* API Service IDs */
#define DIO_API_ID_READ_CHANNEL                 0x00U
#define DIO_API_ID_WRITE_CHANNEL                0x01U
#define DIO_API_ID_READ_PORT                    0x02U
#define DIO_API_ID_WRITE_PORT                   0x03U
#define DIO_API_ID_READ_CHANNEL_GROUP           0x04U
#define DIO_API_ID_WRITE_CHANNEL_GROUP          0x05U
#define DIO_API_ID_GET_VERSION_INFO             0x12U
#define DIO_API_ID_FLIP_CHANNEL                 0x11U

/* Error Codes */
#define DIO_E_PARAM_INVALID_CHANNEL_ID          0x0AU
#define DIO_E_PARAM_INVALID_PORT_ID             0x14U
#define DIO_E_PARAM_INVALID_GROUP               0x1FU
#define DIO_E_PARAM_POINTER                     0x20U

/*==================================================================================================
*                                          TYPE DEFINITIONS
==================================================================================================*/
/** @brief Type for the numeric ID of a DIO channel */
typedef uint16 Dio_ChannelType;

/** @brief Type for the numeric ID of a DIO port */
typedef uint8 Dio_PortType;

/** @brief Type for the value of a DIO port */
typedef uint32 Dio_PortLevelType;

/** @brief Type for the possible levels that a DIO channel can have */
typedef enum {
    STD_LOW = 0,                /**< Physical state 0V */
    STD_HIGH = 1                /**< Physical state 5V or 3.3V */
} Dio_LevelType;

/** @brief Type for the definition of a channel group */
typedef struct {
    Dio_PortType port;          /**< Port on which the channel group is defined */
    uint8 offset;               /**< Position of the channel group on the port */
    Dio_PortLevelType mask;     /**< Mask defining the position of the channel group */
} Dio_ChannelGroupType;

/*==================================================================================================
*                                      GLOBAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                      GLOBAL VARIABLES
==================================================================================================*/

/*==================================================================================================
*                                   FUNCTION PROTOTYPES
==================================================================================================*/
#define DIO_START_SEC_CODE
#include "MemMap.h"

/**
 * @brief Returns the value of the specified DIO channel
 *
 * @param[in] ChannelId ID of DIO channel
 * @return Dio_LevelType
 *         - STD_HIGH: Physical level of the corresponding pin is STD_HIGH
 *         - STD_LOW: Physical level of the corresponding pin is STD_LOW
 *
 * @pre None
 * @post None
 */
Dio_LevelType Dio_ReadChannel(Dio_ChannelType ChannelId);

/**
 * @brief Service to set a level of a channel
 *
 * @param[in] ChannelId ID of DIO channel
 * @param[in] Level Value to be written
 *
 * @pre None
 * @post Channel level set
 */
void Dio_WriteChannel(Dio_ChannelType ChannelId, Dio_LevelType Level);

/**
 * @brief Returns the level of all channels of that port
 *
 * @param[in] PortId ID of DIO Port
 * @return Dio_PortLevelType Level of all channels of that port
 *
 * @pre None
 * @post None
 */
Dio_PortLevelType Dio_ReadPort(Dio_PortType PortId);

/**
 * @brief Service to set a value to the port
 *
 * @param[in] PortId ID of DIO Port
 * @param[in] Level Value to be written
 *
 * @pre None
 * @post Port level set
 */
void Dio_WritePort(Dio_PortType PortId, Dio_PortLevelType Level);

/**
 * @brief This Service reads a subset of the adjoining bits of a port
 *
 * @param[in] ChannelGroupIdPtr Pointer to ChannelGroup
 * @return Dio_PortLevelType Level of a subset of the adjoining bits of a port
 *
 * @pre None
 * @post None
 */
Dio_PortLevelType Dio_ReadChannelGroup(const Dio_ChannelGroupType* ChannelGroupIdPtr);

/**
 * @brief Service to set a subset of the adjoining bits of a port to a specified level
 *
 * @param[in] ChannelGroupIdPtr Pointer to ChannelGroup
 * @param[in] Level Value to be written
 *
 * @pre None
 * @post Channel group level set
 */
void Dio_WriteChannelGroup(const Dio_ChannelGroupType* ChannelGroupIdPtr, Dio_PortLevelType Level);

/**
 * @brief Service to get the version information of this module
 *
 * @param[out] versioninfo Pointer to where to store the version information
 *
 * @pre None
 * @post Version information stored
 *
 * @note Only available if DIO_VERSION_INFO_API is STD_ON
 */
#if (DIO_VERSION_INFO_API == STD_ON)
void Dio_GetVersionInfo(Std_VersionInfoType* versioninfo);
#endif

/**
 * @brief Service to flip (change from 1 to 0 or from 0 to 1) the level of a channel
 *
 * @param[in] ChannelId ID of DIO channel
 * @return Dio_LevelType Level of the channel after flip
 *
 * @pre None
 * @post Channel level flipped
 *
 * @note Only available if DIO_FLIP_CHANNEL_API is STD_ON
 */
#if (DIO_FLIP_CHANNEL_API == STD_ON)
Dio_LevelType Dio_FlipChannel(Dio_ChannelType ChannelId);
#endif

#define DIO_STOP_SEC_CODE
#include "MemMap.h"

/*==================================================================================================
*                                      INLINE FUNCTIONS
==================================================================================================*/

#endif /* DIO_H */
