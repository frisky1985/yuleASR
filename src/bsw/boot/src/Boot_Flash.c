#include "Boot_Flash.h"
#include "Flash.h"

/*
 * PORTING: Replace Flash_* calls with target MCU flash driver.
 * S32K312 uses the MCAL Flash driver already in yuleASR.
 */

static boolean g_flash_initialized = FALSE;

Boot_Result Boot_Flash_Init(void)
{
    if (g_flash_initialized) {
        return BOOT_OK;
    }
    Flash_Init();
    g_flash_initialized = TRUE;
    return BOOT_OK;
}

Boot_Result Boot_Flash_Erase(uint32_t address, uint32_t size)
{
    if (g_flash_initialized == 0U) {
        return BOOT_E_NOT_INIT;
    }
    /* Align to sector boundary */
    uint32_t sector_size = Flash_GetSectorSize();
    uint32_t aligned_addr = address & ~(sector_size - 1U);
    uint32_t end = address + size;
    uint32_t aligned_end = (end + sector_size - 1U) & ~(sector_size - 1U);

    for (uint32_t addr = aligned_addr; addr < aligned_end; addr += sector_size) {
        Boot_Result retry;
        for (uint32_t attempt = 0U; attempt < BOOT_MAX_RETRIES; attempt++) {
            if (Flash_EraseSector(addr) == E_OK) {
                retry = BOOT_OK;
                break;
            }
            retry = BOOT_E_FLASH_ERASE;
        }
        if (retry != BOOT_OK) {
            return retry;
        }
    }
    return BOOT_OK;
}

Boot_Result Boot_Flash_Write(uint32_t dst_addr, const uint8_t *src, uint32_t length)
{
    if (g_flash_initialized == 0U) {
        return BOOT_E_NOT_INIT;
    }
    for (uint32_t attempt = 0U; attempt < BOOT_MAX_RETRIES; attempt++) {
        if (Flash_Write(dst_addr, src, length) == E_OK) {
            /* Read-back verify */
            uint8_t verify_buf[64];
            uint32_t chunk;
            boolean match = TRUE;
            for (uint32_t off = 0U; off < length; off += chunk) {
                chunk = (length - off < sizeof(verify_buf)) ? (length - off) : sizeof(verify_buf);
                if (Flash_Read(dst_addr + off, verify_buf, chunk) != E_OK) {
                    match = FALSE;
                    break;
                }
                if (memcmp(verify_buf, src + off, chunk) != 0U ) {
                    match = FALSE;
                    break;
                }
            }
            if (match) {
                return BOOT_OK;
            }
        }
    }
    return BOOT_E_FLASH_WRITE;
}

Boot_Result Boot_Flash_Read(uint32_t src_addr, uint8_t *dst, uint32_t length)
{
    if (g_flash_initialized == 0U) {
        return BOOT_E_NOT_INIT;
    }
    if (Flash_Read(src_addr, dst, length) == E_OK) {
        return BOOT_OK;
    }
    return BOOT_E_FLASH_READ;
}

Boot_Result Boot_Flash_SetProtection(uint32_t address, uint32_t size, boolean protect)
{
    (void)address;
    (void)size;
    (void)protect;
    /* S32K312 CSEc provides hardware flash protection.
       Implement Flash_SetProtection() when CSEc driver is available. */
    return BOOT_OK;
}
