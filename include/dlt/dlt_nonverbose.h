/**
 * @file dlt_nonverbose.h
 * @brief DLT Non-Verbose Mode Support
 * 
 * Non-verbose mode transmits only message ID and raw data,
 * reducing bandwidth usage. Message description is stored
 * in separate FIBEX/ARXML file.
 */

#ifndef DLT_NONVERBOSE_H
#define DLT_NONVERBOSE_H

#include "dlt.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Non-Verbose Configuration                                                 */
/*===========================================================================*/

#define DLT_NONVERBOSE_MAX_MESSAGE_IDS 256
#define DLT_NONVERBOSE_MAX_DESCRIPTION_LEN 64

typedef struct {
    uint32_t message_id;
    char description[DLT_NONVERBOSE_MAX_DESCRIPTION_LEN];
    uint8_t parameter_count;
    uint8_t parameter_types[16];  /* Array of Dlt_PayloadDataType */
} Dlt_NonVerboseMessageType;

typedef struct {
    Dlt_NonVerboseMessageType messages[DLT_NONVERBOSE_MAX_MESSAGE_IDS];
    uint16_t message_count;
    bool lookup_table_enabled;
} Dlt_NonVerboseConfigType;

/*===========================================================================*/
/* API Functions                                                             */
/*===========================================================================*/

/**
 * @brief Initialize non-verbose mode
 */
Dlt_ReturnType Dlt_NonVerbose_Init(const Dlt_NonVerboseConfigType *config);

/**
 * @brief De-initialize non-verbose mode
 */
void Dlt_NonVerbose_DeInit(void);

/**
 * @brief Register a non-verbose message
 */
Dlt_ReturnType Dlt_NonVerbose_RegisterMessage(const Dlt_NonVerboseMessageType *message);

/**
 * @brief Send non-verbose log message
 * 
 * Only transmits message ID and raw parameters
 */
Dlt_ReturnType Dlt_NonVerbose_SendLog(Dlt_ContextDataType *context,
                                       uint32_t message_id,
                                       const void *const *params,
                                       uint8_t param_count);

/**
 * @brief Get message description by ID
 */
const char* Dlt_NonVerbose_GetMessageDescription(uint32_t message_id);

/**
 * @brief Export message catalog to FIBEX format
 */
Dlt_ReturnType Dlt_NonVerbose_ExportFibex(const char *filepath);

/**
 * @brief Enable/disable non-verbose mode for a context
 */
Dlt_ReturnType Dlt_NonVerbose_SetContextMode(Dlt_ContextDataType *context, 
                                              bool non_verbose);

/**
 * @brief Check if non-verbose mode is enabled for a context
 */
bool Dlt_NonVerbose_IsContextNonVerbose(const Dlt_ContextDataType *context);

/*===========================================================================*/
/* Message ID Management                                                     */
/*===========================================================================*/

/**
 * @brief Allocate a unique message ID
 */
uint32_t Dlt_NonVerbose_AllocateMessageId(void);

/**
 * @brief Free a message ID
 */
void Dlt_NonVerbose_FreeMessageId(uint32_t message_id);

/**
 * @brief Check if message ID is valid
 */
bool Dlt_NonVerbose_IsValidMessageId(uint32_t message_id);

#ifdef __cplusplus
}
#endif

#endif /* DLT_NONVERBOSE_H */
