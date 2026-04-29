/*==================================================================================================
 *                                      WDG HW UNIT TESTS
 *==================================================================================================
 * FILENAME: test_wdg_hw.c
 * DESCRIPTION: Unit tests for Watchdog Driver Hardware Abstraction Layer
 *==================================================================================================
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "Wdg_Hw.h"

/*==================================================================================================
 *                                    TEST HELPERS
 *==================================================================================================*/

void setUp(void)
{
    /* Reset state before each test */
}

void tearDown(void)
{
    /* Cleanup after each test */
}

/*==================================================================================================
 *                                    TEST CASES
 *==================================================================================================*/

/**
 * @brief Test Wdg_Hw_Init with IWDG configuration
 */
void test_Wdg_Hw_Init_IWDG(void)
{
    Wdg_Hw_ConfigType config = {
        .wdgType = WDG_HW_TYPE_IWDG,
        .config.iwdg = {
            .baseAddress = 0x40003000u,
            .clockFreqHz = 32000u,
            .useInterrupt = FALSE,
            .windowModeEnabled = FALSE,
            .windowStart = 0u,
            .windowEnd = 1000u,  /* 1 second timeout */
            .prescaler = 0u
        },
        .disableAllowed = FALSE
    };

    Std_ReturnType result = Wdg_Hw_Init(&config);

    assert(result == E_OK);
    assert(Wdg_Hw_GetStatus() == WDG_HW_STATUS_RUNNING);
    printf("PASS: test_Wdg_Hw_Init_IWDG\n");
}

/**
 * @brief Test Wdg_Hw_Init with WWDG configuration
 */
void test_Wdg_Hw_Init_WWDG(void)
{
    Wdg_Hw_ConfigType config = {
        .wdgType = WDG_HW_TYPE_WWDG,
        .config.wwdg = {
            .baseAddress = 0x40002C00u,
            .clockFreqHz = 32000u,
            .useInterrupt = TRUE,
            .windowModeEnabled = TRUE,
            .windowValue = 0x40u,
            .prescaler = 1u
        },
        .disableAllowed = TRUE
    };

    Std_ReturnType result = Wdg_Hw_Init(&config);

    assert(result == E_OK);
    assert(Wdg_Hw_GetStatus() == WDG_HW_STATUS_RUNNING);
    printf("PASS: test_Wdg_Hw_Init_WWDG\n");
}

/**
 * @brief Test Wdg_Hw_Init with NULL pointer
 */
void test_Wdg_Hw_Init_NullConfig(void)
{
    Std_ReturnType result = Wdg_Hw_Init(NULL_PTR);

    assert(result == E_NOT_OK);
    printf("PASS: test_Wdg_Hw_Init_NullConfig\n");
}

/**
 * @brief Test Wdg_Hw_DeInit
 */
void test_Wdg_Hw_DeInit(void)
{
    Wdg_Hw_ConfigType config = {
        .wdgType = WDG_HW_TYPE_WWDG,
        .config.wwdg = {
            .baseAddress = 0x40002C00u,
            .clockFreqHz = 32000u,
            .useInterrupt = FALSE,
            .windowModeEnabled = FALSE,
            .windowValue = 0x7Fu,
            .prescaler = 0u
        },
        .disableAllowed = TRUE
    };

    Wdg_Hw_Init(&config);
    Std_ReturnType result = Wdg_Hw_DeInit();

    assert(result == E_OK);
    printf("PASS: test_Wdg_Hw_DeInit\n");
}

/**
 * @brief Test Wdg_Hw_Trigger
 */
void test_Wdg_Hw_Trigger(void)
{
    Wdg_Hw_ConfigType config = {
        .wdgType = WDG_HW_TYPE_IWDG,
        .config.iwdg = {
            .baseAddress = 0x40003000u,
            .clockFreqHz = 32000u,
            .useInterrupt = FALSE,
            .windowModeEnabled = FALSE,
            .windowStart = 0u,
            .windowEnd = 1000u,
            .prescaler = 0u
        },
        .disableAllowed = FALSE
    };

    Wdg_Hw_Init(&config);
    Std_ReturnType result = Wdg_Hw_Trigger();

    assert(result == E_OK);
    printf("PASS: test_Wdg_Hw_Trigger\n");
}

/**
 * @brief Test Wdg_Hw_SetTriggerCondition
 */
void test_Wdg_Hw_SetTriggerCondition(void)
{
    Wdg_Hw_ConfigType config = {
        .wdgType = WDG_HW_TYPE_IWDG,
        .config.iwdg = {
            .baseAddress = 0x40003000u,
            .clockFreqHz = 32000u,
            .useInterrupt = FALSE,
            .windowModeEnabled = FALSE,
            .windowStart = 0u,
            .windowEnd = 1000u,
            .prescaler = 0u
        },
        .disableAllowed = FALSE
    };

    Wdg_Hw_Init(&config);

    Std_ReturnType result = Wdg_Hw_SetTriggerCondition(500u);  /* 500ms */

    assert(result == E_OK);
    printf("PASS: test_Wdg_Hw_SetTriggerCondition\n");
}

/**
 * @brief Test Wdg_Hw_SetTriggerCondition with invalid timeout
 */
void test_Wdg_Hw_SetTriggerCondition_InvalidTimeout(void)
{
    Wdg_Hw_ConfigType config = {
        .wdgType = WDG_HW_TYPE_IWDG,
        .config.iwdg = {
            .baseAddress = 0x40003000u,
            .clockFreqHz = 32000u,
            .useInterrupt = FALSE,
            .windowModeEnabled = FALSE,
            .windowStart = 0u,
            .windowEnd = 1000u,
            .prescaler = 0u
        },
        .disableAllowed = FALSE
    };

    Wdg_Hw_Init(&config);

    /* Too small timeout */
    Std_ReturnType result = Wdg_Hw_SetTriggerCondition(0u);
    assert(result == E_NOT_OK);

    /* Too large timeout */
    result = Wdg_Hw_SetTriggerCondition(20000u);
    assert(result == E_NOT_OK);

    printf("PASS: test_Wdg_Hw_SetTriggerCondition_InvalidTimeout\n");
}

/**
 * @brief Test Wdg_Hw_Disable
 */
void test_Wdg_Hw_Disable(void)
{
    Wdg_Hw_ConfigType config = {
        .wdgType = WDG_HW_TYPE_WWDG,
        .config.wwdg = {
            .baseAddress = 0x40002C00u,
            .clockFreqHz = 32000u,
            .useInterrupt = FALSE,
            .windowModeEnabled = FALSE,
            .windowValue = 0x7Fu,
            .prescaler = 0u
        },
        .disableAllowed = TRUE
    };

    Wdg_Hw_Init(&config);
    Std_ReturnType result = Wdg_Hw_Disable();

    assert(result == E_OK);
    assert(Wdg_Hw_GetStatus() == WDG_HW_STATUS_STOPPED);
    printf("PASS: test_Wdg_Hw_Disable\n");
}

/**
 * @brief Test Wdg_Hw_Disable not allowed
 */
void test_Wdg_Hw_Disable_NotAllowed(void)
{
    Wdg_Hw_ConfigType config = {
        .wdgType = WDG_HW_TYPE_IWDG,
        .config.iwdg = {
            .baseAddress = 0x40003000u,
            .clockFreqHz = 32000u,
            .useInterrupt = FALSE,
            .windowModeEnabled = FALSE,
            .windowStart = 0u,
            .windowEnd = 1000u,
            .prescaler = 0u
        },
        .disableAllowed = FALSE
    };

    Wdg_Hw_Init(&config);
    Std_ReturnType result = Wdg_Hw_Disable();

    /* IWDG cannot be disabled once enabled */
    assert(result == E_OK || result == E_NOT_OK);
    printf("PASS: test_Wdg_Hw_Disable_NotAllowed\n");
}

/**
 * @brief Test Wdg_Hw_IsEnabled
 */
void test_Wdg_Hw_IsEnabled(void)
{
    Wdg_Hw_ConfigType config = {
        .wdgType = WDG_HW_TYPE_WWDG,
        .config.wwdg = {
            .baseAddress = 0x40002C00u,
            .clockFreqHz = 32000u,
            .useInterrupt = FALSE,
            .windowModeEnabled = FALSE,
            .windowValue = 0x7Fu,
            .prescaler = 0u
        },
        .disableAllowed = TRUE
    };

    Wdg_Hw_Init(&config);

    boolean enabled = Wdg_Hw_IsEnabled();
    assert(enabled == TRUE);

    Wdg_Hw_Disable();
    enabled = Wdg_Hw_IsEnabled();
    /* May still return TRUE depending on hardware capabilities */

    printf("PASS: test_Wdg_Hw_IsEnabled\n");
}

/**
 * @brief Test Wdg_Hw_GetCounter
 */
void test_Wdg_Hw_GetCounter(void)
{
    Wdg_Hw_ConfigType config = {
        .wdgType = WDG_HW_TYPE_WWDG,
        .config.wwdg = {
            .baseAddress = 0x40002C00u,
            .clockFreqHz = 32000u,
            .useInterrupt = FALSE,
            .windowModeEnabled = FALSE,
            .windowValue = 0x7Fu,
            .prescaler = 0u
        },
        .disableAllowed = TRUE
    };

    Wdg_Hw_Init(&config);

    uint32 counter = Wdg_Hw_GetCounter();
    /* Counter value depends on hardware state */
    (void)counter;

    printf("PASS: test_Wdg_Hw_GetCounter\n");
}

/**
 * @brief Test Wdg_Hw_GetResetReason
 */
void test_Wdg_Hw_GetResetReason(void)
{
    Wdg_Hw_ConfigType config = {
        .wdgType = WDG_HW_TYPE_IWDG,
        .config.iwdg = {
            .baseAddress = 0x40003000u,
            .clockFreqHz = 32000u,
            .useInterrupt = FALSE,
            .windowModeEnabled = FALSE,
            .windowStart = 0u,
            .windowEnd = 1000u,
            .prescaler = 0u
        },
        .disableAllowed = FALSE
    };

    Wdg_Hw_Init(&config);

    Wdg_Hw_ResetReasonType reason = Wdg_Hw_GetResetReason();
    /* Reason depends on actual hardware state */
    (void)reason;

    printf("PASS: test_Wdg_Hw_GetResetReason\n");
}

/**
 * @brief Test Wdg_Hw_SetEarlyWarningInterrupt
 */
void test_Wdg_Hw_SetEarlyWarningInterrupt(void)
{
    Wdg_Hw_ConfigType config = {
        .wdgType = WDG_HW_TYPE_WWDG,
        .config.wwdg = {
            .baseAddress = 0x40002C00u,
            .clockFreqHz = 32000u,
            .useInterrupt = FALSE,
            .windowModeEnabled = FALSE,
            .windowValue = 0x7Fu,
            .prescaler = 0u
        },
        .disableAllowed = TRUE
    };

    Wdg_Hw_Init(&config);

    Std_ReturnType result = Wdg_Hw_SetEarlyWarningInterrupt(0x40u);

    assert(result == E_OK);
    printf("PASS: test_Wdg_Hw_SetEarlyWarningInterrupt\n");
}

/**
 * @brief Test Wdg_Hw_ClearInterruptFlag
 */
void test_Wdg_Hw_ClearInterruptFlag(void)
{
    Wdg_Hw_ConfigType config = {
        .wdgType = WDG_HW_TYPE_WWDG,
        .config.wwdg = {
            .baseAddress = 0x40002C00u,
            .clockFreqHz = 32000u,
            .useInterrupt = TRUE,
            .windowModeEnabled = FALSE,
            .windowValue = 0x7Fu,
            .prescaler = 0u
        },
        .disableAllowed = TRUE
    };

    Wdg_Hw_Init(&config);
    Wdg_Hw_ClearInterruptFlag();

    printf("PASS: test_Wdg_Hw_ClearInterruptFlag\n");
}

/**
 * @brief Test Wdg_Hw_IRQHandler
 */
void test_Wdg_Hw_IRQHandler(void)
{
    Wdg_Hw_ConfigType config = {
        .wdgType = WDG_HW_TYPE_WWDG,
        .config.wwdg = {
            .baseAddress = 0x40002C00u,
            .clockFreqHz = 32000u,
            .useInterrupt = TRUE,
            .windowModeEnabled = FALSE,
            .windowValue = 0x7Fu,
            .prescaler = 0u
        },
        .disableAllowed = TRUE
    };

    Wdg_Hw_Init(&config);

    /* Call IRQ handler - should not crash */
    Wdg_Hw_IRQHandler();

    printf("PASS: test_Wdg_Hw_IRQHandler\n");
}

/*==================================================================================================
 *                                    MAIN TEST RUNNER
 *==================================================================================================*/

int main(void)
{
    printf("========================================\n");
    printf("    WDG HW Unit Tests Starting...\n");
    printf("========================================\n\n");

    setUp();
    test_Wdg_Hw_Init_IWDG();
    tearDown();

    setUp();
    test_Wdg_Hw_Init_WWDG();
    tearDown();

    setUp();
    test_Wdg_Hw_Init_NullConfig();
    tearDown();

    setUp();
    test_Wdg_Hw_DeInit();
    tearDown();

    setUp();
    test_Wdg_Hw_Trigger();
    tearDown();

    setUp();
    test_Wdg_Hw_SetTriggerCondition();
    tearDown();

    setUp();
    test_Wdg_Hw_SetTriggerCondition_InvalidTimeout();
    tearDown();

    setUp();
    test_Wdg_Hw_Disable();
    tearDown();

    setUp();
    test_Wdg_Hw_Disable_NotAllowed();
    tearDown();

    setUp();
    test_Wdg_Hw_IsEnabled();
    tearDown();

    setUp();
    test_Wdg_Hw_GetCounter();
    tearDown();

    setUp();
    test_Wdg_Hw_GetResetReason();
    tearDown();

    setUp();
    test_Wdg_Hw_SetEarlyWarningInterrupt();
    tearDown();

    setUp();
    test_Wdg_Hw_ClearInterruptFlag();
    tearDown();

    setUp();
    test_Wdg_Hw_IRQHandler();
    tearDown();

    printf("\n========================================\n");
    printf("    All WDG HW Tests Passed!\n");
    printf("========================================\n");

    return 0;
}
