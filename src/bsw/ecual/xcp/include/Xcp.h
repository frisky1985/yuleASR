/** @file Xcp.h
 * @brief AUTOSAR XCP module header file
 * @details Universal Calibration Protocol (XCP) - ASAM standard
 * @copyright YuleTech AutoSAR BSW Platform
 */

#ifndef XCP_H
#define XCP_H

#include "Std_Types.h"

/*============================================================================
 *                          VERSION INFORMATION
 *===========================================================================*/
#define XCP_VENDOR_ID         (0x1Fu)    /* YuleTech */
#define XCP_MODULE_ID         (0xD0u)    /* XCP module ID */

#define XCP_SW_MAJOR_VERSION  (1u)
#define XCP_SW_MINOR_VERSION  (0u)
#define XCP_SW_PATCH_VERSION  (0u)

/*============================================================================
 *                          ERROR CODES
 *===========================================================================*/
#define E_OK_OK                ((uint8)0x00u) /* No error */
#define E_ERR_CMD_UNKNOWN      ((uint8)0x20u) /* Unknown command */
#define E_ERR_CMD_SYNTAX       ((uint8)0x21u) /* Command syntax error */
#define E_ERR_OUT_OF_RANGE     ((uint8)0x22u) /* Parameter out of range */
#define E_ERR_WRITE_PROTECTED  ((uint8)0x23u) /* Write protected */
#define E_ERR_ACCESS_DENIED    ((uint8)0x24u) /* Access denied */
#define E_ERR_ACCESS_LOCKED    ((uint8)0x25u) /* Access locked */
#define E_ERR_RESOURCE_BUSY    ((uint8)0x26u) /* Resource busy */
#define E_ERR_KEY_REQ          ((uint8)0x27u) /* Key request */
#define E_ERR_SEQ              ((uint8)0x28u) /* Sequence error */
#define E_ERR_DAQ_CONFIG       ((uint8)0x29u) /* DAQ configuration error */

/*============================================================================
 *                          XCP PACKET TYPES
 *===========================================================================*/
#define XCP_CMD_PACKET      (0u)
#define XCP_RES_PACKET      (1u)
#define XCP_ERR_PACKET      (2u)
#define XCP_EV_PACKET       (3u)
#define XCP_SERV_PACKET     (4u)

/*============================================================================
 *                          XCP COMMANDS
 *===========================================================================*/
/* Standard commands */
#define XCP_CMD_CONNECT         (0xFFu)
#define XCP_CMD_DISCONNECT      (0xFEu)
#define XCP_CMD_GET_STATUS      (0xFDu)
#define XCP_CMD_SYNCH           (0xFCu)
#define XCP_CMD_GET_COMM_MODE_INFO (0xFBu)
#define XCP_CMD_GET_ID          (0xFAu)
#define XCP_CMD_SET_REQUEST     (0xF9u)
#define XCP_CMD_GET_SEED        (0xF8u)
#define XCP_CMD_UNLOCK          (0xF7u)
#define XCP_CMD_SET_MTA         (0xF6u)
#define XCP_CMD_UPLOAD          (0xF5u)
#define XCP_CMD_SHORT_UPLOAD    (0xF4u)
#define XCP_CMD_BUILD_CHECKSUM  (0xF3u)
#define XCP_CMD_TRANSPORT_LAYER_CMD (0xF2u)
#define XCP_CMD_USER_CMD        (0xF1u)

/* Calibration commands */
#define XCP_CMD_DOWNLOAD        (0xF0u)
#define XCP_CMD_DOWNLOAD_NEXT   (0xEFu)
#define XCP_CMD_DOWNLOAD_MAX    (0xEEu)
#define XCP_CMD_SHORT_DOWNLOAD  (0xEDu)
#define XCP_CMD_MODIFY_BITS     (0xECu)
#define XCP_CMD_SET_CAL_PAGE    (0xEBu)
#define XCP_CMD_GET_CAL_PAGE    (0xEAu)

/* Page switching commands */
#define XCP_CMD_GET_PAG_PROCESSOR_INFO  (0xE9u)
#define XCP_CMD_GET_SEGMENT_INFO        (0xE8u)
#define XCP_CMD_GET_PAGE_INFO           (0xE7u)
#define XCP_CMD_SET_SEGMENT_MODE        (0xE6u)
#define XCP_CMD_GET_SEGMENT_MODE        (0xE5u)
#define XCP_CMD_COPY_CAL_PAGE           (0xE4u)

/* DAQ commands */
#define XCP_CMD_FREE_DAQ        (0xD6u)
#define XCP_CMD_ALLOC_DAQ       (0xD5u)
#define XCP_CMD_ALLOC_ODT       (0xD4u)
#define XCP_CMD_ALLOC_ODT_ENTRY (0xD3u)
#define XCP_CMD_SET_DAQ_PTR     (0xE2u)
#define XCP_CMD_WRITE_DAQ       (0xE1u)
#define XCP_CMD_SET_DAQ_LIST_MODE   (0xE0u)
#define XCP_CMD_GET_DAQ_LIST_MODE   (0xDFu)
#define XCP_CMD_START_STOP_DAQ_LIST (0xDEu)
#define XCP_CMD_START_STOP_SYNCH    (0xDDu)
#define XCP_CMD_GET_DAQ_CLOCK       (0xDCu)
#define XCP_CMD_READ_DAQ            (0xDBu)
#define XCP_CMD_GET_DAQ_PROCESSOR_INFO  (0xDAu)
#define XCP_CMD_GET_DAQ_RESOLUTION_INFO (0xD9u)
#define XCP_CMD_GET_DAQ_LIST_INFO       (0xD8u)
#define XCP_CMD_GET_DAQ_EVENT_INFO      (0xD7u)

/*============================================================================
 *                          DATA TYPES
 *===========================================================================*/

typedef uint8 Xcp_StatusType;
typedef uint8 Xcp_CtoType;
typedef uint8 Xcp_DtoType;

/* XCP connection state */
typedef enum {
    XCP_STATE_DISCONNECTED = 0,
    XCP_STATE_CONNECTED    = 1,
    XCP_STATE_RESUME       = 2
} Xcp_ConnectionStateType;

/* XCP session status */
typedef uint8 Xcp_SessionStatusType;
#define XCP_SESSION_RESUME              (0x01u)
#define XCP_SESSION_STORE_CAL_REQ       (0x04u)
#define XCP_SESSION_STORE_DAQ_REQ       (0x40u)
#define XCP_SESSION_CLEAR_DAQ_REQ       (0x80u)

/* Calibration page mode */
typedef uint8 Xcp_PageModeType;
#define XCP_PAGE_MODE_ECU       (0x01u)
#define XCP_PAGE_MODE_XCP       (0x02u)

/* DAQ list mode */
typedef uint8 Xcp_DaqListModeType;
#define XCP_DAQ_MODE_SELECTED   (0x01u)
#define XCP_DAQ_MODE_STARTED    (0x02u)
#define XCP_DAQ_MODE_TIMESTAMP  (0x10u)
#define XCP_DAQ_MODE_PID_OFF    (0x20u)
#define XCP_DAQ_MODE_RUNNING    (0x40u)
#define XCP_DAQ_MODE_RESUME     (0x80u)

/* DAQ list state */
typedef enum {
    XCP_DAQ_STATE_STOPPED = 0,
    XCP_DAQ_STATE_RUNNING = 1
} Xcp_DaqStateType;

/*============================================================================
 *                          XCP PACKET STRUCTURES
 *===========================================================================*/

/* CTO (Command Transfer Object) - max 8 bytes */
typedef struct {
    uint8 data[8];
} Xcp_CtoPacketType;

/* DTO (Data Transfer Object) - max 8 bytes */
typedef struct {
    uint8 data[8];
} Xcp_DtoPacketType;

/* MTA (Memory Transfer Address) */
typedef struct {
    uint32 address;
    uint8  extension;
} Xcp_MtaType;

/* DAQ entry */
typedef struct {
    uint32 address;
    uint8  extension;
    uint8  size;
} Xcp_DaqEntryType;

/* ODT (Object Descriptor Table) */
typedef struct {
    Xcp_DaqEntryType *entries;
    uint8             entryCount;
    uint8             filled;
} Xcp_OdtType;

/* DAQ list */
typedef struct {
    Xcp_OdtType      *odts;
    uint8             odtCount;
    uint8             eventChannel;
    uint8             prescaler;
    Xcp_DaqListModeType mode;
    Xcp_DaqStateType  state;
    boolean           selected;
} Xcp_DaqListType;

/*============================================================================
 *                          CALLBACK TYPES
 *===========================================================================*/
typedef void (*Xcp_TxConfirmationType)(void);
typedef void (*Xcp_RxIndicationType)(const uint8 *data, uint16 length);

/*============================================================================
 *                          FUNCTION PROTOTYPES
 *===========================================================================*/

/* Initialization */
extern void Xcp_Init(const void *config);
extern void Xcp_DeInit(void);

/* Main function - cyclic call */
extern void Xcp_MainFunction(void);

/* Reception handling */
extern void Xcp_RxIndication(const uint8 *data, uint16 length);

/* Transmission */
extern void Xcp_TxConfirmation(void);

/* Internal protocol handlers */
extern void Xcp_ProcessCommand(const uint8 *cmd, uint8 len);
extern void Xcp_SendResponse(const uint8 *data, uint8 len);
extern void Xcp_SendError(uint8 errorCode);

/* Command handlers */
extern void Xcp_CmdConnect(const uint8 *cmd);
extern void Xcp_CmdDisconnect(const uint8 *cmd);
extern void Xcp_CmdGetStatus(const uint8 *cmd);
extern void Xcp_CmdSynch(const uint8 *cmd);
extern void Xcp_CmdSetMta(const uint8 *cmd);
extern void Xcp_CmdUpload(const uint8 *cmd);
extern void Xcp_CmdShortUpload(const uint8 *cmd);
extern void Xcp_CmdDownload(const uint8 *cmd);
extern void Xcp_CmdSetCalPage(const uint8 *cmd);
extern void Xcp_CmdGetCalPage(const uint8 *cmd);
extern void Xcp_CmdCopyCalPage(const uint8 *cmd);
extern void Xcp_CmdAllocDaq(const uint8 *cmd);
extern void Xcp_CmdAllocOdt(const uint8 *cmd);
extern void Xcp_CmdAllocOdtEntry(const uint8 *cmd);
extern void Xcp_CmdFreeDaq(const uint8 *cmd);
extern void Xcp_CmdSetDaqPtr(const uint8 *cmd);
extern void Xcp_CmdWriteDaq(const uint8 *cmd);
extern void Xcp_CmdSetDaqListMode(const uint8 *cmd);
extern void Xcp_CmdStartStopDaqList(const uint8 *cmd);
extern void Xcp_CmdStartStopSynch(const uint8 *cmd);
extern void Xcp_CmdGetDaqProcessorInfo(const uint8 *cmd);

/* DAQ processing */
extern void Xcp_DaqProcessor(void);
extern void Xcp_DaqTrigger(uint8 eventChannel);
extern void Xcp_SendDaqPacket(const Xcp_DaqListType *daqList, uint8 odtIndex);

/* Calibration page switching */
extern uint8 Xcp_SetCalPage(uint8 segment, uint8 page, uint8 mode);
extern uint8 Xcp_GetCalPage(uint8 segment, uint8 mode);
extern uint8 Xcp_CopyCalPage(uint8 srcSeg, uint8 srcPage, uint8 destSeg, uint8 destPage);

/* Utility functions */
extern uint8 Xcp_ChecksumCalculate(const uint8 *data, uint32 size);
extern void Xcp_MtaSet(uint32 address, uint8 extension);
extern uint8 Xcp_MtaRead(uint8 *buffer, uint8 count);
extern uint8 Xcp_MtaWrite(const uint8 *buffer, uint8 count);

/*============================================================================
 *                          EXTERN DECLARATIONS
 *===========================================================================*/
extern const Xcp_DaqListType *Xcp_DaqLists;
extern const uint8 Xcp_MaxDaqLists;
extern const uint8 Xcp_MaxEvents;
extern Xcp_ConnectionStateType Xcp_ConnectionState;
extern Xcp_SessionStatusType Xcp_SessionStatus;
extern Xcp_MtaType Xcp_Mta;

#endif /* XCP_H */
