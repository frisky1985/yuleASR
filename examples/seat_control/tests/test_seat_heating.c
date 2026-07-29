/**
 * @file test_seat_heating.c
 * @brief Unit tests — Seat Heating PWM control (C99, no dynamic memory)
 *
 * Tests the heating subsystem:
 *   - Init → HEAT_OFF
 *   - 3-level switching (OFF / LOW / HIGH)
 *   - Invalid level rejection
 *   - 10-minute auto-off timeout
 *   - Remaining time calculation
 *   - LED indicator state
 *
 * Compile with: gcc -std=c99 -Wall -Wextra -I tests/mocks -I config -I include \
 *               -o test_seat_heating \
 *               tests/mocks/mock_bsw.c src/SeatHeating.c tests/test_seat_heating.c
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "SeatHeating.h"
#include "Seat_Cfg.h"
#include "Dio_Cfg.h"

#include "Dio.h"
#include "Pwm.h"
/* Mock control functions declared in mock BSW headers above */

/*==============================================================================================
 * Constants for timeout test
 *==============================================================================================*/
#define TICKS_PER_SEC       (100U)               /* 10ms ticks per second */
#define TIMEOUT_TICKS       (SEAT_HEATER_TIMEOUT_MS / 10U)  /* 600000/10 = 60000 ticks */

/*==============================================================================================
 * Test: Init — heater off
 *=============================================================================================*/
static void test_init_off(void)
{
    printf("  [test_init] GIVEN SeatHeating_Init "
           "THEN level=OFF, LED=OFF, PWM=0\n");

    mock_All_Reset();
    SeatHeating_Init();

    assert(SeatHeating_GetLevel() == HEAT_OFF);
    assert(SeatHeating_GetRemainingTime() == 0U);
    assert(mock_Dio_GetWriteChannel(DioConf_DioChannel_SeatLedHeat) == STD_LOW);
    assert(mock_Pwm_GetDutyCycle((Pwm_ChannelType)4U) == 0U);

    printf("    PASS\n");
}

/*==============================================================================================
 * Test: Set level LOW
 *=============================================================================================*/
static void test_set_low(void)
{
    printf("  [test_set_level_low] GIVEN SetLevel(HEAT_LOW) "
           "THEN level=LOW, PWM=40%%, LED=ON\n");

    mock_All_Reset();
    SeatHeating_Init();

    Std_ReturnType ret = SeatHeating_SetLevel(HEAT_LOW);
    assert(ret == E_OK);
    assert(SeatHeating_GetLevel() == HEAT_LOW);
    assert(mock_Pwm_GetDutyCycle((Pwm_ChannelType)4U) == SEAT_HEATER_LOW_PERCENT);
    assert(mock_Dio_GetWriteChannel(DioConf_DioChannel_SeatLedHeat) == STD_HIGH);

    printf("    PASS\n");
}

/*==============================================================================================
 * Test: Set level HIGH
 *=============================================================================================*/
static void test_set_high(void)
{
    printf("  [test_set_level_high] GIVEN SetLevel(HEAT_HIGH) "
           "THEN level=HIGH, PWM=80%%, LED=ON\n");

    mock_All_Reset();
    SeatHeating_Init();

    Std_ReturnType ret = SeatHeating_SetLevel(HEAT_HIGH);
    assert(ret == E_OK);
    assert(SeatHeating_GetLevel() == HEAT_HIGH);
    assert(mock_Pwm_GetDutyCycle((Pwm_ChannelType)4U) == SEAT_HEATER_HIGH_PERCENT);
    assert(mock_Dio_GetWriteChannel(DioConf_DioChannel_SeatLedHeat) == STD_HIGH);

    printf("    PASS\n");
}

/*==============================================================================================
 * Test: Set level OFF
 *=============================================================================================*/
static void test_set_off(void)
{
    printf("  [test_set_level_off] GIVEN SetLevel(HEAT_OFF) "
           "THEN level=OFF, PWM=0, LED=OFF\n");

    mock_All_Reset();
    SeatHeating_Init();

    SeatHeating_SetLevel(HEAT_HIGH);
    assert(SeatHeating_GetLevel() == HEAT_HIGH);

    SeatHeating_SetLevel(HEAT_OFF);
    assert(SeatHeating_GetLevel() == HEAT_OFF);
    assert(mock_Pwm_GetDutyCycle((Pwm_ChannelType)4U) == 0U);
    assert(mock_Dio_GetWriteChannel(DioConf_DioChannel_SeatLedHeat) == STD_LOW);

    printf("    PASS\n");
}

/*==============================================================================================
 * Test: Invalid level → E_NOT_OK
 *=============================================================================================*/
static void test_invalid_level(void)
{
    printf("  [test_invalid_level] GIVEN level > HEAT_HIGH "
           "WHEN SetLevel THEN E_NOT_OK, level unchanged\n");

    mock_All_Reset();
    SeatHeating_Init();

    Std_ReturnType ret = SeatHeating_SetLevel((SeatHeatLevelType)99);
    assert(ret == E_NOT_OK);
    assert(SeatHeating_GetLevel() == HEAT_OFF);

    /* Set to HIGH first, then invalid */
    SeatHeating_SetLevel(HEAT_HIGH);
    ret = SeatHeating_SetLevel((SeatHeatLevelType)99);
    assert(ret == E_NOT_OK);
    assert(SeatHeating_GetLevel() == HEAT_HIGH);

    printf("    PASS\n");
}

/*==============================================================================================
 * Test: 10-minute timeout — auto-off
 *=============================================================================================*/
static void test_timeout(void)
{
    printf("  [test_timeout] GIVEN heater set to HIGH "
           "WHEN running for 10 minutes THEN auto-off\n");

    mock_All_Reset();
    SeatHeating_Init();

    SeatHeating_SetLevel(HEAT_HIGH);
    assert(SeatHeating_GetLevel() == HEAT_HIGH);

    /* Verify remaining time is non-zero */
    assert(SeatHeating_GetRemainingTime() > 0U);

    /* Run MainFunction for TIMEOUT_TICKS - 1 (just before timeout) */
    uint32 i;
    for (i = 0U; i < TIMEOUT_TICKS - 1U; i++) {
        SeatHeating_MainFunction();
    }

    /* Should still be ON */
    assert(SeatHeating_GetLevel() == HEAT_HIGH);

    /* One more tick triggers timeout */
    SeatHeating_MainFunction();

    /* Should now be OFF due to timeout */
    assert(SeatHeating_GetLevel() == HEAT_OFF);
    assert(SeatHeating_GetRemainingTime() == 0U);
    assert(mock_Pwm_GetDutyCycle((Pwm_ChannelType)4U) == 0U);
    assert(mock_Dio_GetWriteChannel(DioConf_DioChannel_SeatLedHeat) == STD_LOW);

    printf("    PASS\n");
}

/*==============================================================================================
 * Test: Turning heater off resets timer
 *=============================================================================================*/
static void test_off_resets_timer(void)
{
    printf("  [test_off_resets_timer] GIVEN heater turned on then off "
           "WHEN timeout check THEN timer reset\n");

    mock_All_Reset();
    SeatHeating_Init();

    /* Run for 5 minutes, then turn off */
    SeatHeating_SetLevel(HEAT_LOW);
    uint32 half_ticks = TIMEOUT_TICKS / 2U;

    uint32 i;
    for (i = 0U; i < half_ticks; i++) {
        SeatHeating_MainFunction();
    }

    assert(SeatHeating_GetLevel() == HEAT_LOW);

    /* Turn off */
    SeatHeating_SetLevel(HEAT_OFF);
    assert(SeatHeating_GetLevel() == HEAT_OFF);

    /* Run another 6 minutes — no auto-off because timer was reset */
    for (i = 0U; i < half_ticks + 100U; i++) {
        SeatHeating_MainFunction();
    }

    /* Should still be OFF (explicitly turned off, timer reset) */
    assert(SeatHeating_GetLevel() == HEAT_OFF);

    printf("    PASS\n");
}

/*==============================================================================================
 * Test: Turn back on after timeout
 *=============================================================================================*/
static void test_reheat_after_timeout(void)
{
    printf("  [test_reheat_after_timeout] GIVEN heater timed out "
           "WHEN SetLevel(HIGH) THEN heater turns on again\n");

    mock_All_Reset();
    SeatHeating_Init();

    /* Run to timeout */
    SeatHeating_SetLevel(HEAT_HIGH);

    uint32 i;
    for (i = 0U; i < TIMEOUT_TICKS; i++) {
        SeatHeating_MainFunction();
    }

    assert(SeatHeating_GetLevel() == HEAT_OFF);

    /* Turn back on */
    SeatHeating_SetLevel(HEAT_HIGH);
    assert(SeatHeating_GetLevel() == HEAT_HIGH);
    assert(mock_Pwm_GetDutyCycle((Pwm_ChannelType)4U) == SEAT_HEATER_HIGH_PERCENT);

    /* Run just shy of timeout again */
    for (i = 0U; i < TIMEOUT_TICKS - 1U; i++) {
        SeatHeating_MainFunction();
    }
    assert(SeatHeating_GetLevel() == HEAT_HIGH);

    /* Should time out again */
    SeatHeating_MainFunction();
    assert(SeatHeating_GetLevel() == HEAT_OFF);

    printf("    PASS\n");
}

/*==============================================================================================
 * Test: Remaining time calculation
 *=============================================================================================*/
static void test_remaining_time(void)
{
    printf("  [test_remaining_time] GIVEN heater just turned on "
           "WHEN GetRemainingTime THEN ~600 seconds\n");

    mock_All_Reset();
    SeatHeating_Init();

    SeatHeating_SetLevel(HEAT_HIGH);

    /* 600000ms / 10ms per tick / 100 = 600s */
    uint32 remaining = SeatHeating_GetRemainingTime();
    assert(remaining == 600U);

    /* After 1 second (100 ticks), remaining ≈ 599s */
    uint32 i;
    for (i = 0U; i < TICKS_PER_SEC; i++) {
        SeatHeating_MainFunction();
    }

    remaining = SeatHeating_GetRemainingTime();
    assert(remaining == 599U);

    printf("    PASS\n");
}

/*==============================================================================================
 * Main — test runner
 *=============================================================================================*/
int main(void)
{
    printf("\n=== Test Suite: Seat Heating Control ===\n\n");

    test_init_off();
    test_set_low();
    test_set_high();
    test_set_off();
    test_invalid_level();
    test_timeout();
    test_off_resets_timer();
    test_reheat_after_timeout();
    test_remaining_time();

    printf("\n=== ALL seat heating tests PASSED ===\n\n");
    return 0;
}
