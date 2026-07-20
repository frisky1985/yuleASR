#include "Boot_Update.h"
#include "Boot_Flash.h"
#include "Boot_Image.h"
#include "Boot_Verify.h"
#include <string.h>

#if defined(MBEDTLS_USE)
#include "mbedtls/sha256.h"
#endif

/* Internal update context */
typedef struct {
    uint32_t       slot_addr;
    Boot_ImageType image_type;
#if defined(MBEDTLS_USE)
    mbedtls_sha256_context hash_ctx;
    boolean        hash_active;
#endif
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
        (void)Boot_Update_Abort();
    }

    (void)memset(&g_ctx, 0, sizeof(g_ctx));
    g_ctx.slot_addr    = slot_addr;
    g_ctx.image_type   = image_type;
    g_ctx.active       = TRUE;

#if defined(MBEDTLS_USE)
    mbedtls_sha256_init(&g_ctx.hash_ctx);
    mbedtls_sha256_starts(&g_ctx.hash_ctx, 0);
    g_ctx.hash_active = TRUE;
#endif

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
    if (g_ctx_valid == 0U) {
        return BOOT_E_NOT_INIT;
    }

    uint32_t write_addr = g_ctx.slot_addr + offset;

    /* Update running hash incrementally */
#if defined(MBEDTLS_USE)
    if (g_ctx.hash_active) {
        mbedtls_sha256_update(&g_ctx.hash_ctx, data, length);
    }
#else
    /* Without mbedTLS, hash the entire payload at Finalize time */
#endif

    Boot_Result ret = Boot_Flash_Write(write_addr, data, length);
    if (ret == BOOT_OK) {
        g_ctx.bytes_written += length;
    }
    return ret;
}

Boot_Result Boot_Update_Finalize(Boot_ImageType image_type, uint32_t version)
{
    if (g_ctx_valid == 0U) {
        return BOOT_E_NOT_INIT;
    }

    /* 1. Build and write image header */
    Boot_ImageHeader hdr;
    (void)memset(&hdr, 0, sizeof(hdr));
    hdr.magic        = BOOT_IMAGE_MAGIC;
    hdr.image_type   = (uint32_t)image_type;
    hdr.version      = version;
    hdr.payload_size = g_ctx.bytes_written;

    /* Obtain the complete payload hash */
#if defined(MBEDTLS_USE)
    if (g_ctx.hash_active) {
        mbedtls_sha256_finish(&g_ctx.hash_ctx, hdr.hash);
        mbedtls_sha256_free(&g_ctx.hash_ctx);
        g_ctx.hash_active = FALSE;
    }
#else
    {
        /* Without incremental hash, read entire payload and hash once */
        uint8_t page_buf[256];
        uint32_t remaining = g_ctx.bytes_written;
        uint32_t off = 0U;
        uint32_t total = 0U;
        while (remaining > 0U) {
            uint32_t chunk = (remaining < sizeof(page_buf)) ? remaining : sizeof(page_buf);
            (void)Boot_Flash_Read(g_ctx.slot_addr + sizeof(Boot_ImageHeader) + off, page_buf, chunk);
            off += chunk;
            remaining -= chunk;
            total += chunk;
        }
        Boot_Verify_Hash(page_buf, total, hdr.hash);
    }
#endif

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
    (void)memset(&g_ctx, 0, sizeof(g_ctx));
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
