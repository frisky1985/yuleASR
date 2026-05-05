/* DEM stub */
#ifndef DEM_H
#define DEM_H
#include "Std_Types.h"
#define DEM_DTC_GROUP_ALL 0
#define DEM_DTC_KIND_ALL_DTCS 0
typedef uint8 Dem_EventStatusExtendedType;
static inline uint8 Dem_GetNumberOfStoredDTCs(void) { return 0; }
static inline Std_ReturnType Dem_ClearDTC(uint16 a, uint8 b) { return E_OK; }
#endif
