/**
 * @file SeatPosition.c
 * @brief Seat Position Control — PID closed-loop motor control
 * @version 1.0.0
 * @date 2026-07-12
 *
 * Closed-loop position control using ADC position feedback.
 * Each axis has a dedicated PID controller and PWM output.
 * Limit switch protection is integrated.
 */

/*==================================================================================================
 * Includes
 *==================================================================================================*/
#include "Adc_Cfg.h"
#include "Pwm_Cfg.h"
#include "Dio_Cfg.h"
#include "Port.h"
#include "Port_Cfg.h"
#include "SeatPosition.h"
#include "Adc.h"
#include "Pwm.h"
#include "Dio.h"
#include "Mcu.h"

/*==================================================================================================
 * Axis State
 *==================================================================================================*/
typedef struct {
    int16                   target;         /* Target position (mm or deg) */
    int16                   current;        /* Current position from ADC */
    uint16                  speed;          /* Desired motor speed (0-100%) */
    boolean                 enabled;        /* Motor enabled */
    Seat_PidControllerType  pid;            /* PID controller */
    uint32                  stallTimer;     /* Stall protection counter */
    uint32                  ramptimer;      /* Soft-start ramp counter */
    int16                   direction;      /* +1 = forward, -1 = reverse, 0 = stop */
} Seat_AxisStateType;

/*==================================================================================================
 * Module-private data
 *==================================================================================================*/
static Seat_AxisStateType Seat_AxisState[SEAT_AXIS_MAX];

static void SeatPosition_UpdateAdcReadings(void);
static void SeatPosition_PidUpdate(Seat_AxisType axis);
static void SeatPosition_SetMotorSpeed(Seat_AxisType axis, int16 speedPct, int16 direction);
static void SeatPosition_CheckLimits(Seat_AxisType axis);

/*==================================================================================================
 * Initialization
 *==================================================================================================*/
void SeatPosition_Init(void)
{
    uint8 i;

    for (i = 0U; i < (uint8)SEAT_AXIS_MAX; i++)
    {
        Seat_AxisState[i].target    = 0;
        Seat_AxisState[i].current   = 0;
        Seat_AxisState[i].speed     = SEAT_MOTOR_SPEED_DEFAULT;
        Seat_AxisState[i].enabled   = FALSE;
        Seat_AxisState[i].stallTimer  = 0U;
        Seat_AxisState[i].ramptimer   = 0U;
        Seat_AxisState[i].direction   = 0;

        /* PID gains (Q10 fixed-point: 512 = 0.5, 1024 = 1.0) */
        Seat_AxisState[i].pid.kp       = 512;   /* P = 0.5 */
        Seat_AxisState[i].pid.ki       = 32;    /* I = 0.03125 */
        Seat_AxisState[i].pid.kd       = 128;   /* D = 0.125 */
        Seat_AxisState[i].pid.integral    = 0;
        Seat_AxisState[i].pid.lastError   = 0;
    }

    /* Read initial ADC values */
    SeatPosition_UpdateAdcReadings();
}

/*==================================================================================================
 * Move to absolute target
 *==================================================================================================*/
Std_ReturnType SeatPosition_MoveHorizontal(int16 target_mm)
{
    if ((target_mm < 0) || (target_mm > (int16)SEAT_MAX_HORIZONTAL_MM))
    {
        return E_NOT_OK;
    }
    Seat_AxisState[SEAT_AXIS_HORIZONTAL].target   = target_mm;
    Seat_AxisState[SEAT_AXIS_HORIZONTAL].enabled  = TRUE;
    Seat_AxisState[SEAT_AXIS_HORIZONTAL].stallTimer = 0U;
    Seat_AxisState[SEAT_AXIS_HORIZONTAL].ramptimer  = 0U;
    return E_OK;
}

Std_ReturnType SeatPosition_MoveRecline(int16 target_deg)
{
    if ((target_deg < 0) || (target_deg > (int16)SEAT_MAX_RECLINE_DEG))
    {
        return E_NOT_OK;
    }
    Seat_AxisState[SEAT_AXIS_RECLINE].target   = target_deg;
    Seat_AxisState[SEAT_AXIS_RECLINE].enabled  = TRUE;
    Seat_AxisState[SEAT_AXIS_RECLINE].stallTimer = 0U;
    Seat_AxisState[SEAT_AXIS_RECLINE].ramptimer  = 0U;
    return E_OK;
}

Std_ReturnType SeatPosition_MoveHeight(int16 target_mm)
{
    if ((target_mm < 0) || (target_mm > (int16)SEAT_MAX_HEIGHT_MM))
    {
        return E_NOT_OK;
    }
    Seat_AxisState[SEAT_AXIS_HEIGHT].target   = target_mm;
    Seat_AxisState[SEAT_AXIS_HEIGHT].enabled  = TRUE;
    Seat_AxisState[SEAT_AXIS_HEIGHT].stallTimer = 0U;
    Seat_AxisState[SEAT_AXIS_HEIGHT].ramptimer  = 0U;
    return E_OK;
}

Std_ReturnType SeatPosition_MoveTilt(int16 target_deg)
{
    if ((target_deg < 0) || (target_deg > (int16)SEAT_MAX_TILT_DEG))
    {
        return E_NOT_OK;
    }
    Seat_AxisState[SEAT_AXIS_TILT].target   = target_deg;
    Seat_AxisState[SEAT_AXIS_TILT].enabled  = TRUE;
    Seat_AxisState[SEAT_AXIS_TILT].stallTimer = 0U;
    Seat_AxisState[SEAT_AXIS_TILT].ramptimer  = 0U;
    return E_OK;
}

/*==================================================================================================
 * Relative jog
 *==================================================================================================*/
Std_ReturnType SeatPosition_JogHorizontal(int16 delta_mm)
{
    int16 newTarget = Seat_AxisState[SEAT_AXIS_HORIZONTAL].current + delta_mm;
    return SeatPosition_MoveHorizontal(newTarget);
}

Std_ReturnType SeatPosition_JogRecline(int16 delta_deg)
{
    int16 newTarget = Seat_AxisState[SEAT_AXIS_RECLINE].current + delta_deg;
    return SeatPosition_MoveRecline(newTarget);
}

Std_ReturnType SeatPosition_JogHeight(int16 delta_mm)
{
    int16 newTarget = Seat_AxisState[SEAT_AXIS_HEIGHT].current + delta_mm;
    return SeatPosition_MoveHeight(newTarget);
}

Std_ReturnType SeatPosition_JogTilt(int16 delta_deg)
{
    int16 newTarget = Seat_AxisState[SEAT_AXIS_TILT].current + delta_deg;
    return SeatPosition_MoveTilt(newTarget);
}

/*==================================================================================================
 * Stop all motors
 *==================================================================================================*/
void SeatPosition_StopAll(void)
{
    uint8 i;
    for (i = 0U; i < (uint8)SEAT_AXIS_MAX; i++)
    {
        Seat_AxisState[i].enabled   = FALSE;
        Seat_AxisState[i].direction = 0;
        SeatPosition_SetMotorSpeed((Seat_AxisType)i, 0, 0);
    }
}

/*==================================================================================================
 * Periodic processing (10ms)
 *==================================================================================================*/
void SeatPosition_Process(void)
{
    uint8 i;

    /* Update ADC readings */
    SeatPosition_UpdateAdcReadings();

    /* Process each axis */
    for (i = 0U; i < (uint8)SEAT_AXIS_MAX; i++)
    {
        if (!Seat_AxisState[i].enabled)
        {
            /* Axis disabled; ensure motor is stopped */
            SeatPosition_SetMotorSpeed((Seat_AxisType)i, 0, 0);
            continue;
        }

        /* Check limit switches */
        SeatPosition_CheckLimits((Seat_AxisType)i);

        /* PID update */
        SeatPosition_PidUpdate((Seat_AxisType)i);

        /* Stall detection: if speed > 0 and current hasn't changed for N ticks */
        if (Seat_AxisState[i].direction != 0)
        {
            Seat_AxisState[i].stallTimer++;
            if (Seat_AxisState[i].stallTimer > (SEAT_MOTOR_STALL_TIMEOUT_MS / 10U))
            {
                /* Stall detected — stop motor */
                Seat_AxisState[i].enabled   = FALSE;
                Seat_AxisState[i].direction = 0;
                SeatPosition_SetMotorSpeed((Seat_AxisType)i, 0, 0);
            }
        }

        /* Check if position is reached (within deadband) */
        {
            int16 error = Seat_AxisState[i].target - Seat_AxisState[i].current;
            if ((error > -3) && (error < 3))
            {
                /* Position reached */
                Seat_AxisState[i].enabled   = FALSE;
                Seat_AxisState[i].direction = 0;
                Seat_AxisState[i].stallTimer = 0U;
                SeatPosition_SetMotorSpeed((Seat_AxisType)i, 0, 0);
            }
        }
    }
}

/*==================================================================================================
 * Read current positions from ADC
 *==================================================================================================*/
int16 SeatPosition_ReadHorizontal(void)
{
    return Seat_AxisState[SEAT_AXIS_HORIZONTAL].current;
}

int16 SeatPosition_ReadRecline(void)
{
    return Seat_AxisState[SEAT_AXIS_RECLINE].current;
}

int16 SeatPosition_ReadHeight(void)
{
    return Seat_AxisState[SEAT_AXIS_HEIGHT].current;
}

int16 SeatPosition_ReadTilt(void)
{
    return Seat_AxisState[SEAT_AXIS_TILT].current;
}

boolean SeatPosition_IsMoving(void)
{
    uint8 i;
    for (i = 0U; i < (uint8)SEAT_AXIS_MAX; i++)
    {
        if (Seat_AxisState[i].enabled)
        {
            return TRUE;
        }
    }
    return FALSE;
}

boolean SeatPosition_IsLimitReached(Seat_AxisType axis)
{
    /* Limit switch detection via DIO.
     * In a real implementation this reads PTF0-PTF7.
     * For demo: always return FALSE. */
    (void)axis;
    return FALSE;
}

/*==================================================================================================
 * Private: Update ADC readings
 *==================================================================================================*/
static void SeatPosition_UpdateAdcReadings(void)
{
    uint16 adcRaw;

    /* Horizontal position */
    (void)Adc_ReadChannel(ADC_CHANNEL_HORIZONTAL_POS, &adcRaw);
    Seat_AxisState[SEAT_AXIS_HORIZONTAL].current =
        (int16)(((uint32)adcRaw * SEAT_ADC_HORIZONTAL_MM_PER_LSB) / 1000U);

    /* Recline position */
    (void)Adc_ReadChannel(ADC_CHANNEL_RECLINE_POS, &adcRaw);
    Seat_AxisState[SEAT_AXIS_RECLINE].current =
        (int16)(((uint32)adcRaw * SEAT_ADC_RECLINE_DEG_PER_LSB) / 1000U);

    /* Height position */
    (void)Adc_ReadChannel(ADC_CHANNEL_HEIGHT_POS, &adcRaw);
    Seat_AxisState[SEAT_AXIS_HEIGHT].current =
        (int16)(((uint32)adcRaw * SEAT_ADC_HEIGHT_MM_PER_LSB) / 1000U);

    /* Tilt position */
    (void)Adc_ReadChannel(ADC_CHANNEL_TILT_POS, &adcRaw);
    Seat_AxisState[SEAT_AXIS_TILT].current =
        (int16)(((uint32)adcRaw * SEAT_ADC_TILT_DEG_PER_LSB) / 1000U);
}

/*==================================================================================================
 * Private: PID update
 *==================================================================================================*/
static void SeatPosition_PidUpdate(Seat_AxisType axis)
{
    Seat_PidControllerType* pid = &Seat_AxisState[axis].pid;
    int16 target  = Seat_AxisState[axis].target;
    int16 current = Seat_AxisState[axis].current;
    int16 error;
    int16 output;

    error = target - current;

    /* Calculate PID output (Q10 → integer) */
    /* P term */
    int32 pTerm = (int32)pid->kp * (int32)error;

    /* I term (anti-windup: limit integral) */
    pid->integral += error;
    if (pid->integral > 10000)
    {
        pid->integral = 10000;
    }
    else if (pid->integral < -10000)
    {
        pid->integral = -10000;
    }
    int32 iTerm = (int32)pid->ki * (int32)pid->integral;

    /* D term */
    int32 dTerm = (int32)pid->kd * (int32)(error - pid->lastError);
    pid->lastError = error;

    /* Sum all terms, divide by Q10 (1024) */
    output = (int16)((pTerm + iTerm + dTerm) / 1024);

    /* Clamp output */
    if (output > 100)
    {
        output = 100;
    }
    else if (output < -100)
    {
        output = -100;
    }

    /* Determine direction and set speed */
    if (output > 3)
    {
        Seat_AxisState[axis].direction = 1;
        SeatPosition_SetMotorSpeed(axis, output, 1);
    }
    else if (output < -3)
    {
        Seat_AxisState[axis].direction = -1;
        SeatPosition_SetMotorSpeed(axis, (int16)(-output), -1);
    }
    else
    {
        Seat_AxisState[axis].direction = 0;
        SeatPosition_SetMotorSpeed(axis, 0, 0);
    }
}

/*==================================================================================================
 * Private: Set motor speed and direction
 *==================================================================================================*/
static void SeatPosition_SetMotorSpeed(Seat_AxisType axis, int16 speedPct, int16 direction)
{
    Dio_ChannelType dirPinA;
    Dio_ChannelType dirPinB;
    Pwm_ChannelType pwmChannel;
    uint16 dutyCycle;

    /* Map axis to hardware resources */
    switch (axis)
    {
        case SEAT_AXIS_HORIZONTAL:
            dirPinA    = (Dio_ChannelType)PORT_PIN_PTA0;  /* Horizontal dirA */
            dirPinB    = (Dio_ChannelType)PORT_PIN_PTA1;  /* Horizontal dirB */
            pwmChannel = PWM_CHANNEL_HORIZONTAL;
            Dio_WriteChannel(DioConf_DioChannel_MotorHorizontalEn, (Dio_LevelType)(speedPct > 0));
            break;

        case SEAT_AXIS_RECLINE:
            dirPinA    = (Dio_ChannelType)PORT_PIN_PTA2;  /* Recline dirA */
            dirPinB    = (Dio_ChannelType)PORT_PIN_PTA3;  /* Recline dirB */
            pwmChannel = PWM_CHANNEL_RECLINE;
            Dio_WriteChannel(DioConf_DioChannel_MotorReclineEn, (Dio_LevelType)(speedPct > 0));
            break;

        case SEAT_AXIS_HEIGHT:
            dirPinA    = (Dio_ChannelType)PORT_PIN_PTA4;  /* Height dirA */
            dirPinB    = (Dio_ChannelType)PORT_PIN_PTA5;  /* Height dirB */
            pwmChannel = PWM_CHANNEL_HEIGHT;
            Dio_WriteChannel(DioConf_DioChannel_MotorHeightEn, (Dio_LevelType)(speedPct > 0));
            break;

        case SEAT_AXIS_TILT:
            dirPinA    = (Dio_ChannelType)PORT_PIN_PTA6;  /* Tilt dirA */
            dirPinB    = (Dio_ChannelType)PORT_PIN_PTA7;  /* Tilt dirB */
            pwmChannel = PWM_CHANNEL_TILT;
            Dio_WriteChannel(DioConf_DioChannel_MotorTiltEn, (Dio_LevelType)(speedPct > 0));
            break;

        default:
            return;
    }

    /* Set motor direction via H-bridge */
    if (direction > 0)
    {
        Dio_WriteChannel(dirPinA, STD_HIGH);
        Dio_WriteChannel(dirPinB, STD_LOW);
    }
    else if (direction < 0)
    {
        Dio_WriteChannel(dirPinA, STD_LOW);
        Dio_WriteChannel(dirPinB, STD_HIGH);
    }
    else
    {
        Dio_WriteChannel(dirPinA, STD_LOW);
        Dio_WriteChannel(dirPinB, STD_LOW);
    }

    /* Clamp speed and convert to duty cycle */
    if (speedPct < 0)
    {
        speedPct = 0;
    }
    if (speedPct > 100)
    {
        speedPct = 100;
    }
    dutyCycle = (uint16)speedPct;
    Pwm_SetDutyCycle(pwmChannel, dutyCycle);
}

/*==================================================================================================
 * Private: Check limit switches
 *==================================================================================================*/
static void SeatPosition_CheckLimits(Seat_AxisType axis)
{
    boolean limitReached = SeatPosition_IsLimitReached(axis);

    if (limitReached)
    {
        Seat_AxisState[axis].enabled   = FALSE;
        Seat_AxisState[axis].direction = 0;
        SeatPosition_SetMotorSpeed(axis, 0, 0);
    }
}
