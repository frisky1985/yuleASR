#ifndef BOOT_HSM_H
#define BOOT_HSM_H

#include "Boot_Types.h"
#include <stdint.h>

Boot_Result Boot_Hsm_Init(void);
Boot_Result Boot_Hsm_VerifySignature(const uint8_t *hash,
                                     const uint8_t *signature,
                                     uint32_t       key_slot);
Boot_Result Boot_Hsm_Random(uint8_t *buf, uint32_t len);
boolean Boot_Hsm_IsAvailable(void);

#endif /* BOOT_HSM_H */