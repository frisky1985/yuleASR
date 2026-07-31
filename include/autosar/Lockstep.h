/*==================================================================================================
 * Lockstep.h - Lockstep (dual-core lockstep) monitoring module header
 *
 * Provides the API consumed by WdgM and the platform Lockstep layer.
 * The actual lockstep hardware implementation is platform-specific
 * (see Platform_Lockstep.h / S32K312); this header defines the common
 * interface and configuration types.
 *
 * TODO: wire Lockstep_* API to a real MCAL/platform implementation.
 *================================================================================================*/
#ifndef LOCKSTEP_H
#define LOCKSTEP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Std_Types.h"

/*==================================================================================================
 *                                     VERSION INFORMATION
 *================================================================================================*/
#define LOCKSTEP_VENDOR_ID                       43U
#define LOCKSTEP_MODULE_ID                       180U
#define LOCKSTEP_AR_RELEASE_MAJOR_VERSION        4U
#define LOCKSTEP_AR_RELEASE_MINOR_VERSION        7U
#define LOCKSTEP_AR_RELEASE_REVISION_VERSION     0U
#define LOCKSTEP_SW_MAJOR_VERSION                1U
#define LOCKSTEP_SW_MINOR_VERSION                0U
#define LOCKSTEP_SW_PATCH_VERSION                0U

/*==================================================================================================
 *                                     LOCAL TYPES
 *================================================================================================*/
typedef enum {
    LOCKSTEP_MODE_DISABLED = 0U,
    LOCKSTEP_MODE_ENABLED  = 1U,
    LOCKSTEP_MODE_SPLIT    = 2U,
    LOCKSTEP_MODE_DEBUG    = 3U
} Lockstep_ModeType;

typedef struct {
    boolean     enableLockstep;
    boolean     enableBist;
    boolean     enableEout;
    uint8       mode;
    uint32      checkIntervalMs;
    boolean     runBistOnStart;
    uint32      bistTimeoutUs;
} Lockstep_ConfigType;

/*==================================================================================================
 *                                     FUNCTION PROTOTYPES
 *================================================================================================*/
extern Std_ReturnType Lockstep_Init(const Lockstep_ConfigType* config);
extern void           Lockstep_DeInit(void);
extern Std_ReturnType Lockstep_SetMode(Lockstep_ModeType mode);
extern Std_ReturnType Lockstep_GetStatus(boolean* isActive, boolean* hasError);
extern Std_ReturnType Lockstep_CheckStatus(boolean* mismatchDetected);
extern Std_ReturnType Lockstep_RunBist(uint32 timeoutUs);
extern Std_ReturnType Lockstep_GetBistResult(uint32* results);
extern Std_ReturnType Lockstep_ClearError(void);
extern Std_ReturnType Lockstep_GetResetReason(uint32* resetReason);
extern void           Lockstep_DelayUs(uint32 delayUs);
extern void           Lockstep_EventCallback(uint32 eventId);

#ifdef __cplusplus
}
#endif

#endif /* LOCKSTEP_H */
