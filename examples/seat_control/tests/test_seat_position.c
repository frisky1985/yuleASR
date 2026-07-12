/**
 * @file test_seat_position.c
 * @brief Unit tests — Seat Position PID control (C99, no dynamic memory)
 *
 * Tests the closed-loop position controller:
 *   - Target position within range → E_OK
 *   - Target out of range → E_NOT_OK
 *   - PID convergence when ADC reads target value
 *   - Relative jog within limits
 *   - Multi-axis operation
 *
 * Compile with: gcc -std=c99 -Wall -Wextra -I tests/mocks -I config -I include \
 *               -o test_seat_position \
 *               tests/mocks/mock_bsw.c src/SeatPosition.c tests/test_seat_position.c
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "SeatPosition.h"
#include "Seat_Cfg.h"
#include "Dio_Cfg.h"

#include "Dio.h"
#include "Adc.h"
#include "Pwm.h"

/* Mock control functions — declared in mock BSW headers (Dio.h, Adc.h, Pwm.h) */

/*==============================================================================================
 * Test: Init — all axes at zero position, PID initialized
 *==============================================================================================*/
static void test_init(void)
{
    printf("  [test_init] GIVEN SeatPosition_Init "
           "THEN all axes = 0, not moving\n");

    mock_All_Reset();

    /* Set ADC to 0 so init position reads as zero */
    mock_Adc_SetChannel(0U, 0U);
    mock_Adc_SetChannel(1U, 0U);
    mock_Adc_SetChannel(2U, 0U);
    mock_Adc_SetChannel(3U, 0U);

    SeatPosition_Init();

    assert(SeatPosition_ReadHorizontal() == 0);
    assert(SeatPosition_ReadRecline() == 0);
    assert(SeatPosition_ReadHeight() == 0);
    assert(SeatPosition_ReadTilt() == 0);
    assert(SeatPosition_IsMoving() == FALSE);

    printf("    PASS\n");
}

/*==============================================================================================
 * Test: MoveHorizontal — valid target → E_OK
 *=============================================================================================*/
static void test_move_horizontal_valid(void)
{
    printf("  [test_move_horizontal_valid] GIVEN target within range "
           "WHEN MoveHorizontal THEN E_OK and axis enabled\n");

    mock_All_Reset();
    SeatPosition_Init();

    Std_ReturnType ret = SeatPosition_MoveHorizontal(100);
    assert(ret == E_OK);

    /* After Process() PID will compute and axis should be moving */
    SeatPosition_Process();
    assert(SeatPosition_IsMoving() == TRUE);

    printf("    PASS\n");
}

/*==============================================================================================
 * Test: MoveHorizontal — invalid target → E_NOT_OK
 *=============================================================================================*/
static void test_move_horizontal_invalid(void)
{
    printf("  [test_move_horizontal_invalid] GIVEN target out of range "
           "WHEN MoveHorizontal THEN E_NOT_OK\n");

    mock_All_Reset();
    SeatPosition_Init();

    /* Above max */
    Std_ReturnType ret = SeatPosition_MoveHorizontal((int16)(SEAT_MAX_HORIZONTAL_MM + 1));
    assert(ret == E_NOT_OK);

    /* Below min (negative) */
    ret = SeatPosition_MoveHorizontal(-10);
    assert(ret == E_NOT_OK);

    /* After Init, ADC=2048→current=118. MoveHorizontal(0) starts moving toward 0. */
    ret = SeatPosition_MoveHorizontal(0);
    assert(ret == E_OK);

    /* Set ADC to produce exactly 0 → target reached */
    mock_Adc_SetChannel(0U, 0U);
    SeatPosition_Process();
    assert(SeatPosition_IsMoving() == FALSE);
    assert(SeatPosition_ReadHorizontal() == 0);

    /* Move to upper boundary */
    ret = SeatPosition_MoveHorizontal((int16)SEAT_MAX_HORIZONTAL_MM);
    assert(ret == E_OK);
    /* Set ADC to map to 230mm (need ADC=3966 due to integer rounding: 3966*58/1000=230) */
    mock_Adc_SetChannel(0U, 3966U);
    SeatPosition_Process();
    assert(SeatPosition_IsMoving() == FALSE);
    assert(SeatPosition_ReadHorizontal() == (int16)SEAT_MAX_HORIZONTAL_MM);

    /* Stop for cleanup */
    SeatPosition_StopAll();

    printf("    PASS\n");
}

/*==============================================================================================
 * Test: MoveRecline invalid
 *=============================================================================================*/
static void test_move_recline_invalid(void)
{
    printf("  [test_move_recline_invalid] GIVEN target out of range "
           "WHEN MoveRecline THEN E_NOT_OK\n");

    mock_All_Reset();
    SeatPosition_Init();

    assert(SeatPosition_MoveRecline(-1) == E_NOT_OK);
    assert(SeatPosition_MoveRecline((int16)(SEAT_MAX_RECLINE_DEG + 1)) == E_NOT_OK);
    assert(SeatPosition_MoveRecline(30) == E_OK);

    printf("    PASS\n");
}

/*==============================================================================================
 * Test: MoveHeight invalid
 *=============================================================================================*/
static void test_move_height_invalid(void)
{
    printf("  [test_move_height_invalid] GIVEN height out of range "
           "WHEN MoveHeight THEN E_NOT_OK\n");

    mock_All_Reset();
    SeatPosition_Init();

    assert(SeatPosition_MoveHeight((int16)(SEAT_MAX_HEIGHT_MM + 5)) == E_NOT_OK);
    assert(SeatPosition_MoveHeight(25) == E_OK);

    printf("    PASS\n");
}

/*==============================================================================================
 * Test: MoveTilt invalid
 *=============================================================================================*/
static void test_move_tilt_invalid(void)
{
    printf("  [test_move_tilt_invalid] GIVEN tilt out of range "
           "WHEN MoveTilt THEN E_NOT_OK\n");

    mock_All_Reset();
    SeatPosition_Init();

    assert(SeatPosition_MoveTilt((int16)(SEAT_MAX_TILT_DEG + 1)) == E_NOT_OK);
    assert(SeatPosition_MoveTilt(10) == E_OK);

    printf("    PASS\n");
}

/*==============================================================================================
 * Test: Target reached — PID convergences when ADC matches target
 *=============================================================================================*/
static void test_target_reached(void)
{
    printf("  [test_target_reached] GIVEN ADC returns target position "
           "WHEN Process THEN axis stops (position reached)\n");

    mock_All_Reset();
    SeatPosition_Init();

    /* Set target to 100mm */
    Std_ReturnType ret = SeatPosition_MoveHorizontal(100);
    assert(ret == E_OK);

    /* Process with mid-scale ADC (2048 raw):
     * 2048 * 58 / 1000 = 118mm current vs 100mm target → PID moves */
    mock_Adc_SetChannel(0U, 2048U);  /* ADC_CHANNEL_HORIZONTAL_POS */
    SeatPosition_Process();
    assert(SeatPosition_IsMoving() == TRUE);

    /* Now set ADC to produce exactly target: 100mm
     * Need 1725 LSB: 1725*58/1000 = 100 */
    mock_Adc_SetChannel(0U, 1725U);
    SeatPosition_Process();

    /* After PID update: output = P*(100 - 100) = 0, within deadband, axis stops */
    assert(SeatPosition_IsMoving() == FALSE);
    assert(SeatPosition_ReadHorizontal() == 100);

    printf("    PASS\n");
}

/*==============================================================================================
 * Test: Target reached by mock ADC at recline axis
 *=============================================================================================*/
static void test_target_reached_recline(void)
{
    printf("  [test_target_reached_recline] GIVEN ADC matches recline target "
           "WHEN Process THEN axis stops\n");

    mock_All_Reset();
    SeatPosition_Init();

    Std_ReturnType ret2 = SeatPosition_MoveRecline(30);
    assert(ret2 == E_OK);

    /* 30 deg * 1000 / 15 = 2000 LSB → should yield 30 deg reading */
    mock_Adc_SetChannel(1U, 2000U);  /* ADC_CHANNEL_RECLINE_POS */
    SeatPosition_Process();

    /* 2000 * 15 / 1000 = 30 deg = target → deadband → stop */
    assert(SeatPosition_IsMoving() == FALSE);
    assert(SeatPosition_ReadRecline() == 30);

    printf("    PASS\n");
}

/*==============================================================================================
 * Test: JogHorizontal — relative movement
 *=============================================================================================*/
static void test_jog_horizontal(void)
{
    printf("  [test_jog_horizontal] GIVEN jog forward 10mm "
           "THEN target = current + 10\n");

    mock_All_Reset();
    SeatPosition_Init();

    /* Initial position from ADC=2048: 2048*58/1000=118 */
    mock_Adc_SetChannel(0U, 2048U);
    SeatPosition_Process();

    int16 pos_before = SeatPosition_ReadHorizontal();

    /* Jog relative */
    Std_ReturnType ret = SeatPosition_JogHorizontal(10);
    assert(ret == E_OK);

    /* Axis should be moving toward new target = pos_before + 10 */
    assert(SeatPosition_IsMoving() == TRUE);

    /* Simulate reaching target by setting ADC to produce 128mm (118+10)
     * Compute: want adc*58/1000 = 128 → adc = 128*1000/58 = 2207 (exact: 2207*58/1000=128) */
    mock_Adc_SetChannel(0U, 2207U);
    SeatPosition_Process();

    assert(SeatPosition_IsMoving() == FALSE);
    assert(SeatPosition_ReadHorizontal() == pos_before + 10);

    printf("    PASS\n");
}

/*==============================================================================================
 * Test: StopAll stops all axes
 *=============================================================================================*/
static void test_stop_all(void)
{
    printf("  [test_stop_all] GIVEN multiple axes moving "
           "WHEN StopAll THEN all stopped\n");

    mock_All_Reset();
    SeatPosition_Init();

    SeatPosition_MoveHorizontal(100);
    SeatPosition_MoveRecline(30);
    SeatPosition_Process();

    assert(SeatPosition_IsMoving() == TRUE);

    SeatPosition_StopAll();

    assert(SeatPosition_IsMoving() == FALSE);

    printf("    PASS\n");
}

/*==============================================================================================
 * Test: Out-of-range jog is rejected
 *=============================================================================================*/
static void test_jog_out_of_range(void)
{
    printf("  [test_jog_out_of_range] GIVEN current near max "
           "WHEN jog beyond limit THEN E_NOT_OK\n");

    mock_All_Reset();
    SeatPosition_Init();

    /* Simulate current at 225mm by setting ADC */
    /* 225 * 1000 / 58 ≈ 3879 LSB */
    mock_Adc_SetChannel(0U, 3879U);
    SeatPosition_Process();

    /* Jog +10 from ~225 → ~235, but max is 230 → should fail */
    Std_ReturnType ret = SeatPosition_JogHorizontal(10);
    assert(ret == E_NOT_OK);

    /* Jog -5 from ~225 → ~220, valid */
    ret = SeatPosition_JogHorizontal(-5);
    assert(ret == E_OK);

    printf("    PASS\n");
}

/*==============================================================================================
 * Main — test runner
 *=============================================================================================*/
int main(void)
{
    printf("\n=== Test Suite: Seat Position PID Control ===\n\n");

    test_init();
    test_move_horizontal_valid();
    test_move_horizontal_invalid();
    test_move_recline_invalid();
    test_move_height_invalid();
    test_move_tilt_invalid();
    test_target_reached();
    test_target_reached_recline();
    test_jog_horizontal();
    test_stop_all();
    test_jog_out_of_range();

    printf("\n=== ALL seat position tests PASSED ===\n\n");
    return 0;
}
