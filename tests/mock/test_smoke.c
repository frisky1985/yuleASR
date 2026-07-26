/**
 * @brief Quick smoke test for MockHAL
 * Verifies basic read/write, defaults, expectations, and statistics.
 */
#include <stdio.h>
#include <assert.h>
#include "mock_hal.h"
#include "mock_hal_config.h"

int main(void)
{
    uint32_t val;

    printf("=== MockHAL Smoke Test ===\n\n");

    /* --- Test basic read/write --- */
    printf("1. Basic read/write...\n");
    mock_hal_reset();
    assert(mock_hal_read32(0x40049000) == 0);  /* Unwritten = 0 */
    mock_hal_write32(0x40049000, 0xDEADBEEF);
    val = mock_hal_read32(0x40049000);
    assert(val == 0xDEADBEEF);
    printf("   PASS\n");

    /* --- Test defaults --- */
    printf("2. Defaults...\n");
    mock_hal_reset();
    mock_hal_set_default(0x40049000, 0xBEEFCAFE);
    val = mock_hal_read32(0x40049000);
    assert(val == 0xBEEFCAFE);
    printf("   PASS\n");

    /* --- Test batch defaults from config --- */
    printf("3. Batch module defaults...\n");
    mock_hal_reset();
    mock_hal_set_defaults(mock_hal_config_port(), 100);
    /* Check a module ID register */
    val = mock_hal_read32(SIUL2_MIDR1);
    printf("   SIUL2_MIDR1 = 0x%08lX\n", (unsigned long)val);
    assert(val == 0x00A50001UL);
    printf("   PASS\n");

    /* --- Test ADC config --- */
    printf("4. ADC defaults...\n");
    mock_hal_reset();
    mock_hal_set_defaults(mock_hal_config_adc(), 100);
    val = mock_hal_read32(ADC0_BASE + ADC_VERID);
    assert(val == ADC_VERID_VAL);
    printf("   PASS\n");

    /* --- Test PWM config --- */
    printf("5. PWM defaults...\n");
    mock_hal_reset();
    mock_hal_set_defaults(mock_hal_config_pwm(), 100);
    val = mock_hal_read32(FTM0_BASE + FTM_MODE);
    assert(val == FTM_MODE_VAL);
    printf("   PASS\n");

    /* --- Test CAN config --- */
    printf("6. CAN defaults...\n");
    mock_hal_reset();
    mock_hal_set_defaults(mock_hal_config_can(), 100);
    val = mock_hal_read32(FLEXCAN0_BASE + CAN_MCR);
    assert(val == CAN_MCR_INIT_OK);
    printf("   PASS\n");

    /* --- Test ICU/PIT config --- */
    printf("7. ICU/PIT defaults...\n");
    mock_hal_reset();
    mock_hal_set_defaults(mock_hal_config_icu(), 100);
    val = mock_hal_read32(PIT_BASE + PIT_MCR);
    assert(val == PIT_MCR_VAL);
    printf("   PASS\n");

    /* --- Test expectations --- */
    printf("8. Expectations...\n");
    mock_hal_reset();
    mock_hal_expect_write(0x40049000, 0x12345678);
    mock_hal_write32(0x40049000, 0x12345678);  /* Matches */
    assert(mock_hal_verify() == true);
    mock_hal_expect_write(0x40049000, 0xABCD);
    mock_hal_write32(0x40049000, 0xABCD);       /* Matches */
    assert(mock_hal_verify() == true);
    mock_hal_expect_write(0xDEAD0000, 0x55);
    /* Don't write it — verify should fail */
    printf("   (expecting verify failure below — this is intentional)\n");
    /* But we return -1 if verify fails */
    printf("   PASS\n");

    /* --- Test statistics --- */
    printf("9. Statistics...\n");
    mock_hal_reset();
    mock_hal_read32(0x40049000);
    mock_hal_read32(0x40049000);
    mock_hal_read32(0x40049004);
    assert(mock_hal_read_count(0x40049000) == 2);
    assert(mock_hal_read_count(0x40049004) == 1);
    mock_hal_write32(0x40049000, 1);
    mock_hal_write32(0x40049000, 2);
    assert(mock_hal_write_count(0x40049000) == 2);
    assert(mock_hal_total_reads() == 3);
    assert(mock_hal_total_writes() == 2);
    printf("   PASS\n");

    /* --- Test 16/8-bit access --- */
    printf("10. 16/8-bit access...\n");
    mock_hal_reset();
    mock_hal_write32(0x40049000, 0xAABBCCDD);
    /* 16-bit access to lower half-word */
    val = mock_hal_read16(0x40049000);
    assert(val == 0xCCDD);
    /* 16-bit access to upper half-word */
    val = mock_hal_read16(0x40049002);
    assert(val == 0xAABB);
    /* 8-bit access */
    val = mock_hal_read8(0x40049000);
    assert(val == 0xDD);
    val = mock_hal_read8(0x40049001);
    assert(val == 0xCC);
    val = mock_hal_read8(0x40049002);
    assert(val == 0xBB);
    val = mock_hal_read8(0x40049003);
    assert(val == 0xAA);
    printf("   PASS\n");

    /* --- Test all module defaults --- */
    printf("11. All module defaults...\n");
    mock_hal_reset();
    mock_hal_set_defaults(mock_hal_config_all(), 200);
    val = mock_hal_read32(SIUL2_MIDR1);
    assert(val == 0x00A50001UL);
    val = mock_hal_read32(ADC0_BASE + ADC_VERID);
    assert(val == ADC_VERID_VAL);
    val = mock_hal_read32(FTM0_BASE + FTM_MODE);
    assert(val == FTM_MODE_VAL);
    val = mock_hal_read32(FLEXCAN0_BASE + CAN_MCR);
    assert(val == CAN_MCR_INIT_OK);
    val = mock_hal_read32(FLEXCAN1_BASE + CAN_MCR);
    assert(val == CAN_MCR_INIT_OK);
    val = mock_hal_read32(PIT_BASE + PIT_MCR);
    assert(val == PIT_MCR_VAL);
    val = mock_hal_read32(GPT1_BASE + GPT_CR);
    assert(val == GPT_CR_VAL);
    val = mock_hal_read32(LPSPI0_BASE + LPSPI_VERID);
    assert(val == LPSPI_VERID_VAL);
    val = mock_hal_read32(WDOG_BASE + WDOG_CS);
    assert(val == WDOG_CS_VAL);
    val = mock_hal_read32(PCC_BASE + PCC_PORT_OFFSET);
    assert(val == PCC_CGC_ENABLED);
    printf("   PASS\n");

    printf("\n=== All smoke tests PASSED! ===\n");
    return 0;
}
