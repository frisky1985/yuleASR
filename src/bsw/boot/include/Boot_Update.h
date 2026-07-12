#ifndef BOOT_UPDATE_H
#define BOOT_UPDATE_H

#include "Boot_Types.h"

Boot_Result Boot_Update_Prepare(uint32_t slot_addr, Boot_ImageType image_type);
Boot_Result Boot_Update_WriteBlock(const uint8_t *data,
                                   uint32_t       offset,
                                   uint32_t       length);
Boot_Result Boot_Update_Finalize(Boot_ImageType image_type, uint32_t version);
Boot_Result Boot_Update_Abort(void);
Boot_Result Boot_Update_SwapSlots(void);

#endif /* BOOT_UPDATE_H */