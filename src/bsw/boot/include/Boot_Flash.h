#ifndef BOOT_FLASH_H
#define BOOT_FLASH_H

#include "Boot_Types.h"

Boot_Result Boot_Flash_Init(void);
Boot_Result Boot_Flash_Erase(uint32_t address, uint32_t size);
Boot_Result Boot_Flash_Write(uint32_t dst_addr,
                             const uint8_t *src,
                             uint32_t length);
Boot_Result Boot_Flash_Read(uint32_t src_addr,
                            uint8_t *dst,
                            uint32_t length);
Boot_Result Boot_Flash_SetProtection(uint32_t  address,
                                     uint32_t  size,
                                     boolean   protect);

#endif /* BOOT_FLASH_H */