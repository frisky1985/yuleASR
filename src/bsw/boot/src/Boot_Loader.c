#include "Boot_Loader.h"
#include "Boot_Flash.h"
#include "Boot_Image.h"
#include "Boot_Verify.h"
#include "Boot_Update.h"
#include "Boot_Hsm.h"
#include <string.h>

/* External symbols defined by the linker script */
extern uint32_t __stack_top;

/* Page buffer for flash-to-RAM hash — sized to MCU page (S32K312: 4KB) */
#define BOOT_PAGE_BUFFER_SIZE  4096U
static uint8_t g_boot_page_buf[BOOT_PAGE_BUFFER_SIZE];

/*
 * Reset handler address is always at offset 4 of the vector table.
 * Application entry = *(uint32_t*)(target_addr + 4)
 */
#define VECTOR_TABLE_OFFSET  0x00U
#define ENTRY_POINT_OFFSET   0x04U

/* Forward — BIB helpers (duplicated from Boot_Update.c for independence) */
static Boot_Result load_bib(Boot_InfoBlock *bib);
static Boot_Result save_bib(const Boot_InfoBlock *bib);
static uint32_t    bib_checksum(const Boot_InfoBlock *bib);

/* ---- Main Entry ---- */

void Boot_Loader_Main(void)
{
    Boot_Result ret;

    /* Phase 1: Init hardware minimally */
    (void)Boot_Flash_Init();

    /* Phase 2: Load boot info block */
    Boot_InfoBlock bib;
    ret = load_bib(&bib);
    if (ret != BOOT_OK) {
        /* First boot or corrupted BIB → init fresh */
        (void)memset(&bib, 0, sizeof(bib));
        bib.magic               = 0x30424942U;  /* 'BIB0' */
        bib.max_boot_attempts   = BOOT_MAX_BOOT_ATTEMPTS;
        bib.status              = 0x01U;
        bib.crc32               = bib_checksum(&bib);
        (void)save_bib(&bib);
    }

    /* Phase 3: Resolve boot target */
    Boot_Decision decision = Boot_Loader_ResolveBootTarget();

    if (decision.last_error != BOOT_OK) {
        /* Increment boot attempt counter */
        bib.boot_count++;
        if (bib.boot_count >= bib.max_boot_attempts) {
            Boot_Loader_EnterRecovery();
        }
        bib.crc32 = bib_checksum(&bib);
        (void)save_bib(&bib);
        Boot_Loader_EnterRecovery();
    }

    /* Phase 4: Verify signature */
    uint8_t hash[32];
    Boot_ImageHeader hdr;
    Boot_ImageTrailer trail;

    ret = Boot_Flash_Read(decision.target_addr,
                          (uint8_t *)&hdr, sizeof(hdr));
    if (ret != BOOT_OK) goto fail;

    ret = Boot_Image_ValidateHeader(&hdr);
    if (ret != BOOT_OK) goto fail;

    /* Hash the payload — read from flash into page buffer incrementally */
    uint32_t payload_addr = decision.target_addr + sizeof(Boot_ImageHeader);
    uint32_t remaining = hdr.payload_size;
    uint32_t offset = 0U;

    /* Use incremental SHA-256 over the payload */
#if defined(MBEDTLS_USE)
    mbedtls_sha256_context hash_ctx;
    mbedtls_sha256_init(&hash_ctx);
    mbedtls_sha256_starts(&hash_ctx, 0);
    while (remaining > 0U) {
        uint32_t chunk = (remaining < BOOT_PAGE_BUFFER_SIZE) ? remaining : BOOT_PAGE_BUFFER_SIZE;
        (void)Boot_Flash_Read(payload_addr + offset, g_boot_page_buf, chunk);
        mbedtls_sha256_update(&hash_ctx, g_boot_page_buf, chunk);
        offset += chunk;
        remaining -= chunk;
    }
    mbedtls_sha256_finish(&hash_ctx, hash);
#else
    /* Non-incremental fallback: read entire payload, then hash */
    while (remaining > 0U) {
        uint32_t chunk = (remaining < BOOT_PAGE_BUFFER_SIZE) ? remaining : BOOT_PAGE_BUFFER_SIZE;
        (void)Boot_Flash_Read(payload_addr + offset, g_boot_page_buf, chunk);
        offset += chunk;
        remaining -= chunk;
    }
    Boot_Verify_Hash(g_boot_page_buf, hdr.payload_size, hash);
#endif

    /* Read trailer */
    ret = Boot_Image_ReadTrailer(payload_addr, hdr.payload_size, &trail);
    if (ret != BOOT_OK) goto fail;

    /* Try HSM first, fall back to software */
    const Boot_PubKey *pub_key;
    if (decision.target == BOOT_IMAGE_SBL) {
        pub_key = &g_boot_pubkey_sbl;
    } else {
        pub_key = &g_boot_pubkey_app;
    }

    ret = Boot_Verify_Signature(hash, trail.signature, pub_key);
    if (ret != BOOT_OK) {
        /* Signature invalid → recovery */
        goto fail;
    }

    /* Phase 5: Mark successful boot in BIB */
    bib.boot_count = 0U;
    bib.status     = 0x01U;
    bib.crc32      = bib_checksum(&bib);
    (void)save_bib(&bib);

    /* Phase 6: Jump to application */
    Boot_Loader_Jump(decision.target_addr);
    /* never reaches here */

fail:
/*     decision.last_error = ret; */
    bib.boot_count++;
    if (bib.boot_count >= bib.max_boot_attempts) {
        bib.status = 0xFFU;  /* Enter recovery permanently */
    }
    bib.crc32 = bib_checksum(&bib);
    (void)save_bib(&bib);
    Boot_Loader_EnterRecovery();
}

/* ---- Resolve Boot Target ---- */

Boot_Decision Boot_Loader_ResolveBootTarget(void)
{
    Boot_Decision dec;
    (void)memset(&dec, 0, sizeof(dec));

    Boot_InfoBlock bib;
    if (load_bib(&bib) != BOOT_OK) {
        /* First boot: try SBL → App */
        dec.target     = BOOT_IMAGE_SBL;
        dec.target_addr = BOOT_SBL_ADDR;
        return dec;
    }

    /* Check if we should boot from slot B (after failed A) */
    boolean use_slot_b = (bib.status & 0x02U) != 0U;

    if (use_slot_b == 0U) {
        /* Verify SBL first; if SBL is valid, boot it,
           then SBL will verify and jump to App */
        dec.target      = BOOT_IMAGE_SBL;
        dec.target_addr = BOOT_SBL_ADDR;
    } else {
        dec.target      = BOOT_IMAGE_APP;
        dec.target_addr = BOOT_APP_SLOT_B_ADDR;
    }

    return dec;
}

Boot_Result Boot_Loader_ConfirmBoot(void)
{
    Boot_InfoBlock bib;
    Boot_Result ret = load_bib(&bib);
    if (ret != BOOT_OK) return ret;

    bib.boot_count = 0U;
    bib.status     = 0x01U;
    bib.crc32      = bib_checksum(&bib);
    return save_bib(&bib);
}

/* ---- Jump to Application ---- */

void Boot_Loader_Jump(uint32_t target_addr)
{
    /* Disable interrupts */
    __asm volatile("cpsid i");

    /* Disable SysTick and pending interrupts */
    SysTick->CTRL = 0U;
    SCB->ICSR = SCB->ICSR | (1UL << 25);  /* clear pending systick */

    /* Set new vector table */
    uint32_t msp = *(volatile uint32_t *)target_addr;  /* SP from vector[0] */
    uint32_t pc  = *(volatile uint32_t *)(target_addr + 4);  /* PC from vector[1] */

    /* De-init peripherals (minimal) */
    Boot_Flash_Init();  /* flush pending operations */

    /* Set main stack pointer */
    __asm volatile("msr msp, %0" : : "r"(msp));
    __asm volatile("msr psp, %0" : : "r"(msp));

    /* Branch to application */
    typedef void (*AppEntry)(void);
    AppEntry entry = (AppEntry)pc;
    entry();

    /* Should never return */
    while (1U) {}
}

void Boot_Loader_EnterRecovery(void)
{
    /* Minimal UDS listener — only 0x10 0x02 (Programming Session) accepted.
       In production, initialize CAN/DoIP, enter diagnostic loop. */
    while (1U) {
        /* WDG refresh */
        /* UDS poll */
        /* Recovery */
    }
}

/* ---- BIB Helpers ---- */

static Boot_Result load_bib(Boot_InfoBlock *bib)
{
    Boot_Result ret = Boot_Flash_Read(BOOT_BIB_ADDR,
                                      (uint8_t *)bib,
                                      sizeof(Boot_InfoBlock));
    if (ret != BOOT_OK) return ret;

    if (bib->magic != 0x30424942U) {
        return BOOT_E_GENERAL;
    }

    uint32_t expected = bib->crc32;
    bib->crc32 = 0U;
    uint32_t computed = bib_checksum(bib);
    bib->crc32 = expected;

    if (expected != computed) {
        return BOOT_E_GENERAL;
    }

    return BOOT_OK;
}

static Boot_Result save_bib(const Boot_InfoBlock *bib)
{
    Boot_Result ret = Boot_Flash_Erase(BOOT_BIB_ADDR, sizeof(Boot_InfoBlock));
    if (ret != BOOT_OK) return ret;
    return Boot_Flash_Write(BOOT_BIB_ADDR,
                            (const uint8_t *)bib,
                            sizeof(Boot_InfoBlock));
}

static uint32_t bib_checksum(const Boot_InfoBlock *bib)
{
    const uint8_t *bytes = (const uint8_t *)bib;
    uint32_t sum = 0U;
    uint32_t len = sizeof(Boot_InfoBlock) - sizeof(bib->crc32);
    for (uint32_t i = 0U; i < len; i++) {
        sum += bytes[i];
    }
    return sum;
}
