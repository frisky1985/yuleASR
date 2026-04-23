/**
 * @file BswM.h
 * @brief BSW Mode Manager
 * @version 1.0.0
 */

#ifndef BSWM_H
#define BSWM_H

#include "Std_Types.h"

/* AUTOSAR Version */
#define BSWM_AR_RELEASE_MAJOR_VERSION       4
#define BSWM_AR_RELEASE_MINOR_VERSION       0
#define BSWM_AR_RELEASE_REVISION_VERSION    3

/* Module Version */
#define BSWM_SW_MAJOR_VERSION               1
#define BSWM_SW_MINOR_VERSION               0
#define BSWM_SW_PATCH_VERSION               0

/* Service IDs */
#define BSWM_INIT_SID                       0x00
#define BSWM_DEINIT_SID                     0x01
#define BSWM_REQUESTMODE_SID                0x02
#define BSWM_MAINFUNCTION_SID               0x03

/* Error Codes */
#define BSWM_E_NO_ERROR                     0x00
#define BSWM_E_NULL_POINTER                 0x01
#define BSWM_E_INVALID_PAR                  0x02
#define BSWM_E_NOT_INITIALIZED              0x03

/* Types */
typedef uint8 BswM_UserType;

typedef enum {
    BSWM_GENERICVALUE_MIN = 0,
    BSWM_GENERICVALUE_MAX = 255
} BswM_GenericValueType;

typedef struct {
    uint16 mode;
    uint16 user;
} BswM_ModeDeclarationType;

/* Function Prototypes */
extern void BswM_Init(const void* ConfigPtr);
extern void BswM_Deinit(void);
extern void BswM_RequestMode(BswM_UserType requesting_user, uint16 requested_mode);
extern void BswM_MainFunction(void);

#endif /* BSWM_H */
