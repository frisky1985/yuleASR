#include "Boot_Update.h"
#include "Boot_Flash.h"
#include "Boot_Image.h"
#include "Boot_Verify.h"
#include <string.h>

/* Internal update context */
typedef struct {
    uint32_t       slot_addr;
    Boot_ImageType image_type;
    uint8_t        running_hash[32];  /* SHA-256 over all written data */
    uint32_t       bytes_written;
    boolean        active;
} UpdateContext;

static UpdateContext g_ctx;
static boolean g_ctx_valid = FALSE;

/* Forward declaration for BIB helpers */
static Boot_Result bib_read(Boot_InfoBlock *bib);
static Boot_Result bib_write(const Boot_InfoBlock *bib);
static uint32_t    bib_calc_crc(const Boot_InfoBlock *bib);

Boot_Result Boot_Update_Prepare(uint32_t slot_addr, Boot_ImageType image_type)
{
    if (g_ctx_valid) {
        Boot_Update_Abort();
    }

    memset(&g_ctx, 0, sizeof(g_ctx));
    g_ctx.slot_addr    = slot_addr;
    g_ctx.image_type   = image_type;
    g_ctx.active       = TRUE;

    Boot_Result ret = Boot_Flash_Erase(slot_addr, BOOT_APP_SLOT_A_SIZE);
    if (ret != BOOT_OK) {
        g_ctx_valid = FALSE;
        return ret;
    }

    g_ctx_valid = TRUE;
    return BOOT_OK;
}

Boot_Result Boot_Update_WriteBlock(const uint8_t *data,
                                   uint32_t       offset,
                                   uint32_t       length)
{
    if (!g_ctx_valid) {
        return BOOT_E_NOT_INIT;
    }

    uint32_t write_addr = g_ctx.slot_addr + offset;

    /* Update running hash */
    Boot_Verify_Hash(data, length, g_ctx.running_hash);
    /* NOTE: In production, use incremental SHA-256. For stub, recompute
       from scratch each time. Swap to mbedtls_sha256_update() when available. */

    Boot_Result ret = Boot_Flash_Write(write_addr, data, length);
    if (ret == BOOT_OK) {
        g_ctx.bytes_written += length;
    }
    return ret;
}

Boot_Result Boot_Update_Finalize(Boot_ImageType image_type, uint32_t version)
{
    if (!g_ctx_valid) {
        return BOOT_E_NOT_INIT;
    }

    /* 1. Build and write image header */
    Boot_ImageHeader hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic        = BOOT_IMAGE_MAGIC;
    hdr.image_type   = (uint32_t)image_type;
    hdr.version      = version;
    hdr.payload_size = g_ctx.bytes_written;
    memcpy(hdr.hash, g_ctx.running_hash, 32);
    hdr.header_crc   = Boot_Image_CalcHeaderCrc(&hdr);

    Boot_Result ret = Boot_Flash_Write(g_ctx.slot_addr,
                                       (const uint8_t *)&hdr,
                                       sizeof(hdr));
    if (ret != BOOT_OK) {
        g_ctx_valid = FALSE;
        return ret;
    }

    /* 2. Verify written payload hash */
    uint8_t *payload_buf = NULL; /* In production, read-back from flash and verify */
    /* For stub, trust the hash was correct during write */

    /* 3. Update anti-rollback counter in BIB */
    Boot_InfoBlock bib;
    ret = bib_read(&bib);
    if (ret != BOOT_OK) {
        g_ctx_valid = FALSE;
        return ret;
    }

    if (version <= bib.anti_rollback_counter) {
        return BOOT_E_VERSION;  /* Anti-rollback triggered */
    }

    bib.anti_rollback_counter = version;
    if (image_type == BOOT_IMAGE_SBL) {
        bib.sbl_version = version;
    } else {
        bib.app_version = version;
    }
    bib.crc32 = bib_calc_crc(&bib);

    ret = bib_write(&bib);
    g_ctx_valid = FALSE;
    return ret;
}

Boot_Result Boot_Update_Abort(void)
{
    memset(&g_ctx, 0, sizeof(g_ctx));
    g_ctx_valid = FALSE;
    return BOOT_OK;
}

Boot_Result Boot_Update_SwapSlots(void)
{
    /* In A/B scheme: toggle a flag in BIB to select opposite slot */
    Boot_InfoBlock bib;
    Boot_Result ret = bib_read(&bib);
    if (ret != BOOT_OK) return ret;
    /* Toggle slot selection bit in status */
    bib.status ^= 0x02U;
    bib.crc32 = bib_calc_crc(&bib);
    return bib_write(&bib);
}

/* ---- BIB Helpers ---- */

static Boot_Result bib_read(Boot_InfoBlock *bib)
{
    return Boot_Flash_Read(BOOT_BIB_ADDR, (uint8_t *)bib, sizeof(Boot_InfoBlock));
}

static Boot_Result bib_write(const Boot_InfoBlock *bib)
{
    Boot_Result ret = Boot_Flash_Erase(BOOT_BIB_ADDR, sizeof(Boot_InfoBlock));
    if (ret != BOOT_OK) return ret;
    return Boot_Flash_Write(BOOT_BIB_ADDR, (const uint8_t *)bib, sizeof(Boot_InfoBlock));
}

static uint32_t bib_calc_crc(const Boot_InfoBlock *bib)
{
    /* Simple XOR checksum for BIB integrity — replace with hardware CRC in prod */
    const uint8_t *bytes = (const uint8_t *)bib;
    uint32_t sum = 0U;
    /* CRC over everything except the crc32 field itself */
    uint32_t len = sizeof(Boot_InfoBlock) - sizeof(bib->crc32);
    for (uint32_t i = 0U; i < len; i++) {
        sum += bytes[i];
    }
    return sum;
}
