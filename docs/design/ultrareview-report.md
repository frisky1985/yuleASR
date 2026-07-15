# Ultrareview Report: yuleASR Secure Boot Framework

> Review date: 2026-07-10
> Framework: local-ultrareview (3-stage pipeline: correctness → security/perf → architecture → synthesis)
> Codebase: `src/bsw/boot/` (18 files, ~3500 lines)
> Target: S32K312 + HSM, ECDSA P-256 + SHA-256

---

## PHASE 1: Correctness Review

### CRITICAL: Boot_Loader hash computation uses flash address as data pointer

**File:** `src/bsw/boot/src/Boot_Loader.c:74`
```c
Boot_Verify_Hash((const uint8_t *)(uint32_t)payload_addr,
                 hdr.payload_size, hash);
```
**What is wrong:** The payload is in **flash**, not RAM. `Boot_Verify_Hash()` receives a flash memory address as if it were a RAM pointer. On ARM Cortex-M7, `SHA256()` via mbedTLS/software will read garbage (flash is not directly byte-addressable through the same load path in all configurations, or the data read will be the literal address bytes, not the content).

**Why it matters:** The hash comparison will **always fail**, making the bootloader reject every firmware image. The host test passed because it used a RAM buffer (g_boot_flash_ram) which shares the same address space.

**Fix:** Read payload into RAM (or use flash-aware hash):
```c
// Option A: Read into RAM (requires heap/page buffer)
uint8_t *payload_buf = malloc_or_use_page_buffer(hdr.payload_size);
Boot_Flash_Read(payload_addr, payload_buf, hdr.payload_size);
Boot_Verify_Hash(payload_buf, hdr.payload_size, hash);

// Option B: Incremental hash (stream from flash)
// Best for constrained embedded: hash page-by-page
Boot_Hash_Init(&hash_ctx);
for (uint32_t off = 0; off < hdr.payload_size; off += PAGE_SIZE) {
    Boot_Flash_Read(payload_addr + off, page_buf, MIN(PAGE_SIZE, hdr.payload_size - off));
    Boot_Hash_Update(&hash_ctx, page_buf, ...);
}
Boot_Hash_Final(&hash_ctx, hash);
```

### CRITICAL: Boot_Update running hash recomputes from scratch each block

**File:** `src/bsw/boot/src/Boot_Update.c:56`
```c
Boot_Verify_Hash(data, length, g_ctx.running_hash);
/* NOTE: In production, use incremental SHA-256. For stub, recompute
   from scratch each time. */
```
**What is wrong:** Each call to `Boot_Update_WriteBlock()` overwrites `running_hash` with the hash of the **single block**, not the hash of all data written so far. After 10 blocks, `running_hash` only has the hash of the 10th block.

**Why it matters:** `Boot_Update_Finalize()` writes `running_hash` into the image header. The hash will be wrong (only last block's hash, not the full image hash). The bootloader will then reject the update.

**Fix:** Use incremental SHA-256:
```c
mbedtls_sha256_context ctx;
mbedtls_sha256_init(&ctx);
mbedtls_sha256_starts(&ctx, 0);

// Per block:
mbedtls_sha256_update(&ctx, data, length);

// On finalize:
mbedtls_sha256_finish(&ctx, g_ctx.running_hash);
```

### WARNING: bib_calc_crc uses XOR sum, not CRC32

**File:** `Boot_Update.c:153` and `Boot_Loader.c:235`

**What is wrong:** The function is named `bib_calc_crc` but implements a simple XOR checksum (`sum += bytes[i]`). This means:
- Two bytes swapping produces the same checksum
- Zero bytes and null bytes are indistinguishable
- The BIB integrity check is weak

**Fix:** Use the same CRC32 table from `Boot_Image.c` for BIB integrity.

### WARNING: save_bib erases entire BIB region before writing

**File:** `Boot_Loader.c:227-233`, `Boot_Update.c:146-151`

**What is wrong:** Erase-then-write on the BIB region means a power failure between erase and write destroys the BIB permanently. This bricks the ECU — no boot count, no slot selection, no anti-rollback state.

**Fix:** Use a double-buffer BIB (two redundant copies) or write BIB update as: write new copy → verify → erase old copy.

---

## PHASE 2: Security & Performance Review

### CRITICAL: No signature verification when mbedTLS is not compiled in

**File:** `src/bsw/boot/src/Boot_Verify.c:80-85`
```c
#else
    (void)hash; (void)signature; (void)pub_key;
    return BOOT_E_SIGNATURE;
#endif
```
**What is wrong:** When `MBEDTLS_USE` is not defined and HSM is unavailable, `Boot_Verify_Signature` returns `BOOT_E_SIGNATURE`. The caller (`Boot_Loader_Main`) treats any error as "goto fail → recovery". This is safe (fails closed) but the intent is unclear — the framework either needs MBEDTLS or HSM, and there's no compile-time check enforcing this.

**Fix:** Add `#if !defined(MBEDTLS_USE) && !defined(BOOT_ALLOW_NO_CRYPTO)` compile error.

### WARNING: Boot_Loader_Jump does not de-init peripherals properly

**File:** `src/bsw/boot/src/Boot_Loader.c:161-188`

**What is wrong:** The function disables interrupts and SysTick, then jumps. It does NOT:
- Disable peripheral clocks (CAN, SPI, DMA may still be running)
- Flush write buffers
- Reset peripheral registers to known state
- Disable MPU

Peripherals left active can corrupt flash during the application's init phase, or cause bus errors if the application has different peripheral configurations.

**Fix:** Add `EcuM_Shutdown()` or a `Boot_Loader_DeinitPeripherals()` function.

### WARNING: Flash read-back verify buffer is stack-allocated 64 bytes

**File:** `src/bsw/boot/src/Boot_Flash.c:56`
```c
uint8_t verify_buf[64];
```
**What is wrong:** The read-back verify loop reads 64 bytes per iteration. For a 1MB flash image, this loops ~16,000 times, each doing a Flash_Read + memcmp. This is very slow (minutes for large images) and drains the watchdog timer.

**Fix:** Increase buffer to match flash page size (typically 256-4096 bytes), or do a CRC-based block verification instead of full read-back.

### WARNING: Boot_Loader_Main does not initialize the stack pointer

**File:** `Boot_Loader.c:26`

**What is wrong:** `Boot_Loader_Main()` calls other functions but does not verify that the stack pointer is valid (PBL may have left a corrupted stack). The function also does not set up a separate error stack for the `fail:` label path.

### SUGGESTION: Public keys are placeholder zeros

**File:** `Boot_Verify.c:30-37`

**What is wrong:** The SBL and App public keys in `Boot_Verify.c` are zero-filled placeholder bytes. If someone compiles and deploys this as-is, **any signature will be verified against an all-zero key**. However, the framework currently fails closed (rejects all), so this is not immediately exploitable — but could become a trap during development if someone temporarily bypasses verification.

---

## PHASE 3: Architecture Review

### WARNING: BIB helper code duplication

**Files:** `Boot_Loader.c:201-244` and `Boot_Update.c:139-164`

**What is wrong:** `bib_read()`, `bib_write()`, and `bib_calc_crc()` / `bib_checksum()` are implemented in **two files independently**. The two implementations of `bib_calc_crc` and `bib_checksum` are identical XOR checksums but named differently. This is a maintenance risk: a fix in one file won't propagate to the other.

**Fix:** Create `Boot_Bib.c/h` with shared BIB operations.

### WARNING: g_ctx is global persistent state for update

**File:** `Boot_Update.c:16`
```c
static UpdateContext g_ctx;
```
**What is wrong:** A single global context means:
1. No support for concurrent updates (unlikely in embedded, but poor practice)
2. No re-entrancy protection
3. State persists between unrelated operations (a failed Prepare pollutes subsequent sessions)

**Fix:** Pass context pointer explicitly as parameter, or add `Boot_Update_Reset()`.

### SUGGESTION: Boot_Flash_SetProtection is a no-op

**File:** `Boot_Flash.c:89-97`

**What is wrong:** The flash protection function returns `BOOT_OK` without actually doing anything. PBL region write protection is essential for secure boot — without it, a runtime exploit can overwrite PBL.

### SUGGESTION: Test coverage is minimal

**Files:** `test/test_boot_verify.c`, `test/test_boot_integration.c`

The host test covers the crypto path and image format validation, but lacks:
- Corrupted BIB recovery test
- Power-fail during update test
- Anti-rollback boundary (version == counter)
- A/B slot swap test
- Recovery mode entry/exit test
- Large payload (>64KB) hash performance test

---

## SYNTHESIS: Priority Implementation Plan

### Tier 1 — Must Fix Before Any Deployment

| # | Issue | File | Effort | Risk |
|---|-------|------|--------|------|
| 1 | Flash address used as RAM pointer for hash | `Boot_Loader.c:74` | 1h | 🔴 Boot always fails |
| 2 | Running hash overwritten each block | `Boot_Update.c:56` | 1h | 🔴 Update always fails |
| 3 | Enable mbedTLS or add compile guard | `Boot_Verify.c` | 0.5h | 🔴 No crypto = no boot |

### Tier 2 — Must Fix Before Production

| # | Issue | File | Effort | Risk |
|---|-------|------|--------|------|
| 4 | BIB XOR checksum → real CRC32 | Both BIB files | 1h | 🟡 Integrity bypass |
| 5 | BIB erase-then-write power fail | Both BIB files | 2h | 🟡 Brick risk |
| 6 | De-init peripherals before jump | `Boot_Loader.c` | 2h | 🟡 Flash corruption |
| 7 | Flash read-back verify buffer | `Boot_Flash.c` | 0.5h | 🟡 Slow boot |

### Tier 3 — Recommended Improvements

| # | Issue | File | Effort |
|---|-------|------|--------|
| 8 | Extract shared BIB into `Boot_Bib.c` | Both | 1h |
| 9 | Implement flash write protection | `Boot_Flash.c` | 2h |
| 10 | Power-fail safe BIB (double buffer) | BIB | 3h |
| 11 | Incremental hash API for update | `Boot_Update.c` | 1h |
| 12 | Comprehensive failure test suite | `test/` | 4h |

---

## Summary

- **CRITICAL (2)**: Hash computation uses flash address as RAM pointer, running hash overwrite
- **HIGH (2)**: Crypto not compiled in (fails closed but risky), no IOMMU/peripheral de-init
- **MEDIUM (3)**: XOR instead of CRC32, erase-then-write power fail, read-back overhead
- **LOW (5)**: Code duplication, stack allocation size, global state, no flash protection, test coverage

**Overall assessment:** The framework is architecturally sound (chain of trust, A/B slots, anti-rollback, ECDSA + SHA-256). The two CRITICAL bugs are in the host test harness vs. real embedded execution path mismatch — the hash functions work correctly but the data source is wrong. These are straightforward to fix.
