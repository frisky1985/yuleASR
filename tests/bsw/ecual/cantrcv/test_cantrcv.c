/**
 * @file test_cantrcv.c
 * @brief CanTrcv (CAN Transceiver Driver) host unit tests — white-box.
 *
 * The production CanTrcv.c is compiled into this translation unit so that the
 * static helpers (CanTrcv_HwSetMode / CanTrcv_HwGetMode /
 * CanTrcv_CheckWakeupInternal) and the static runtime state can be exercised
 * without touching production code.
 *
 * Hardware dependencies are replaced by DIO / EcuM / DET stubs defined below.
 *
 * @tests src/bsw/ecual/cantrcv/src/CanTrcv.c
 * @tests src/bsw/ecual/cantrcv/include/CanTrcv.h
 */

#include "unity.h"

#include "../../../../src/bsw/ecual/cantrcv/src/CanTrcv.c"

/*==================================================================================================
 * Test doubles
 *================================================================================================*/

/** @brief Simulated DIO pin levels (indexed by channel id). */
static Dio_LevelType DioMock_Levels[64];
static uint16        DioMock_WriteCount;
static Dio_ChannelType DioMock_LastWriteChannel;
static Dio_LevelType   DioMock_LastWriteLevel;

/** @brief Simulated EcuM wake-up reporting. */
static uint16 EcuMock_CallCount;
static EcuM_WakeupSourceType EcuMock_LastSource;

/** @brief DET reporting capture. */
static uint8 DetMock_CallCount;
static uint8 DetMock_LastApiId;
static uint8 DetMock_LastErrorId;

/** @brief Configuration pointer owned by the test (production Lcfg not linked). */
const CanTrcv_ConfigType* CanTrcv_ConfigPtr;

Dio_LevelType Dio_ReadChannel(Dio_ChannelType ChannelId)
{
    if (ChannelId < (Dio_ChannelType)(sizeof(DioMock_Levels) / sizeof(DioMock_Levels[0])))
    {
        return DioMock_Levels[ChannelId];
    }
    return STD_HIGH;
}

void Dio_WriteChannel(Dio_ChannelType ChannelId, Dio_LevelType Level)
{
    if (ChannelId < (Dio_ChannelType)(sizeof(DioMock_Levels) / sizeof(DioMock_Levels[0])))
    {
        DioMock_Levels[ChannelId] = Level;
    }
    DioMock_WriteCount++;
    DioMock_LastWriteChannel = ChannelId;
    DioMock_LastWriteLevel = Level;
}

void EcuM_SetWakeupEvent(EcuM_WakeupSourceType sources)
{
    EcuMock_CallCount++;
    EcuMock_LastSource = sources;
}

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    DetMock_CallCount++;
    DetMock_LastApiId = ApiId;
    DetMock_LastErrorId = ErrorId;
    return E_OK;
}

/*==================================================================================================
 * Test configuration
 *================================================================================================*/

/* Channel 0: TJA1043, inverted STB, DIO controlled.
 * Channel 1: TJA1042, SPI flagged (empty SPI branch), non-inverted STB. */
static const CanTrcv_ChannelConfigType TestChannelsA[CANTRCV_MAX_CHANNELS] =
{
    {
        0U, CANTRCV_TJA1043,
        { 10U, 11U, 12U, TRUE },
        FALSE, 0U, 0U, TRUE, FALSE, ECUM_WKSOURCE_CAN, 1U, 0U
    },
    {
        1U, CANTRCV_TJA1042,
        { 20U, 21U, 22U, FALSE },
        TRUE, 1U, 1U, TRUE, FALSE, ECUM_WKSOURCE_CAN1, 1U, 0U
    }
};

/* Channel 0: TLE6250 (not DIO controlled by the driver switch).
 * Channel 1: UJA1168 with no control pins. */
static const CanTrcv_ChannelConfigType TestChannelsB[CANTRCV_MAX_CHANNELS] =
{
    {
        0U, CANTRCV_TLE6250,
        { 20U, 21U, 22U, FALSE },
        FALSE, 0U, 0U, FALSE, FALSE, ECUM_WKSOURCE_CAN, 1U, 0U
    },
    {
        1U, CANTRCV_UJA1168,
        { DIO_INVALID_CHANNEL, DIO_INVALID_CHANNEL, DIO_INVALID_CHANNEL, FALSE },
        FALSE, 0U, 0U, FALSE, FALSE, ECUM_WKSOURCE_CAN1, 1U, 0U
    }
};

/* Channel 0: DIO-controlled type but both control pins unconfigured. */
static const CanTrcv_ChannelConfigType TestChannelsC[CANTRCV_MAX_CHANNELS] =
{
    {
        0U, CANTRCV_TJA1043,
        { DIO_INVALID_CHANNEL, DIO_INVALID_CHANNEL, 12U, FALSE },
        FALSE, 0U, 0U, FALSE, FALSE, ECUM_WKSOURCE_CAN, 1U, 0U
    },
    {
        1U, CANTRCV_GENERIC,
        { DIO_INVALID_CHANNEL, DIO_INVALID_CHANNEL, 22U, FALSE },
        FALSE, 0U, 0U, FALSE, FALSE, ECUM_WKSOURCE_CAN1, 1U, 0U
    }
};

static const CanTrcv_GeneralConfigType TestGeneralConfig =
{
    CANTRCV_MAX_CHANNELS, TRUE, TRUE, TRUE, CANTRCV_MAIN_FUNCTION_PERIOD_MS
};

static const CanTrcv_ConfigType TestConfigA =
{
    &TestGeneralConfig, TestChannelsA, CANTRCV_MAX_CHANNELS
};

static const CanTrcv_ConfigType TestConfigB =
{
    &TestGeneralConfig, TestChannelsB, CANTRCV_MAX_CHANNELS
};

static const CanTrcv_ConfigType TestConfigC =
{
    &TestGeneralConfig, TestChannelsC, CANTRCV_MAX_CHANNELS
};

/*==================================================================================================
 * Helpers
 *================================================================================================*/

static void Test_Reset(void)
{
    uint8 i;

    for (i = 0U; i < (uint8)(sizeof(DioMock_Levels) / sizeof(DioMock_Levels[0])); i++)
    {
        DioMock_Levels[i] = STD_HIGH;
    }
    DioMock_WriteCount = 0U;
    DioMock_LastWriteChannel = 0U;
    DioMock_LastWriteLevel = STD_LOW;

    EcuMock_CallCount = 0U;
    EcuMock_LastSource = 0U;

    DetMock_CallCount = 0U;
    DetMock_LastApiId = 0xFFU;
    DetMock_LastErrorId = 0xFFU;

    CanTrcv_ConfigPtr = &TestConfigA;
    CanTrcv_InitStatus = FALSE;

    for (i = 0U; i < CANTRCV_MAX_CHANNELS; i++)
    {
        CanTrcv_Runtime[i].CurrentMode = CANTRCV_TRCVMODE_SLEEP;
        CanTrcv_Runtime[i].WakeupMode = CANTRCV_WUMODE_ENABLE;
        CanTrcv_Runtime[i].WakeupReason = CANTRCV_WU_NOT_SUPPORTED;
        CanTrcv_Runtime[i].WakeupPending = FALSE;
        CanTrcv_Runtime[i].WakeupByBusEnabled = TRUE;
    }
}

/*==================================================================================================
 * Static helper coverage
 *================================================================================================*/

static void test_HwSetMode_ValidModes(void)
{
    Test_Reset();
    CanTrcv_ConfigPtr = &TestConfigA;

    /* TJA1043 with inverted STB: NORMAL -> STB low, EN high. */
    CanTrcv_HwSetMode(0U, CANTRCV_TRCVMODE_NORMAL);
    TEST_ASSERT_EQUAL_UINT8(STD_LOW, DioMock_Levels[10]);
    TEST_ASSERT_EQUAL_UINT8(STD_HIGH, DioMock_Levels[11]);
    TEST_ASSERT_EQUAL_UINT16(2U, DioMock_WriteCount);

    /* STANDBY -> STB high, EN high. */
    DioMock_WriteCount = 0U;
    CanTrcv_HwSetMode(0U, CANTRCV_TRCVMODE_STANDBY);
    TEST_ASSERT_EQUAL_UINT8(STD_HIGH, DioMock_Levels[10]);
    TEST_ASSERT_EQUAL_UINT8(STD_HIGH, DioMock_Levels[11]);
    TEST_ASSERT_EQUAL_UINT16(2U, DioMock_WriteCount);

    /* SLEEP -> STB high, EN low. */
    DioMock_WriteCount = 0U;
    CanTrcv_HwSetMode(0U, CANTRCV_TRCVMODE_SLEEP);
    TEST_ASSERT_EQUAL_UINT8(STD_HIGH, DioMock_Levels[10]);
    TEST_ASSERT_EQUAL_UINT8(STD_LOW, DioMock_Levels[11]);

    /* Out-of-range mode hits the default branch (no pin change). */
    DioMock_WriteCount = 0U;
    CanTrcv_HwSetMode(0U, (CanTrcv_TrcvModeType)99);
    TEST_ASSERT_EQUAL_UINT16(0U, DioMock_WriteCount);

    /* TJA1042 non-inverted: NORMAL -> STB high, EN high. */
    DioMock_WriteCount = 0U;
    CanTrcv_HwSetMode(1U, CANTRCV_TRCVMODE_NORMAL);
    TEST_ASSERT_EQUAL_UINT8(STD_HIGH, DioMock_Levels[20]);
    TEST_ASSERT_EQUAL_UINT8(STD_HIGH, DioMock_Levels[21]);

    /* TJA1042 SLEEP -> STB low, EN low. */
    CanTrcv_HwSetMode(1U, CANTRCV_TRCVMODE_SLEEP);
    TEST_ASSERT_EQUAL_UINT8(STD_LOW, DioMock_Levels[20]);
    TEST_ASSERT_EQUAL_UINT8(STD_LOW, DioMock_Levels[21]);
}

static void test_HwSetMode_InvalidAndSpiBranches(void)
{
    Test_Reset();

    /* TLE6250 is not driven by the DIO switch inside HwSetMode. */
    CanTrcv_ConfigPtr = &TestConfigB;
    DioMock_WriteCount = 0U;
    CanTrcv_HwSetMode(0U, CANTRCV_TRCVMODE_NORMAL);
    CanTrcv_HwSetMode(0U, CANTRCV_TRCVMODE_SLEEP);
    TEST_ASSERT_EQUAL_UINT16(0U, DioMock_WriteCount);

    /* UJA1168 / no pins -> nothing written either. */
    CanTrcv_HwSetMode(1U, CANTRCV_TRCVMODE_NORMAL);
    TEST_ASSERT_EQUAL_UINT16(0U, DioMock_WriteCount);

    /* DIO-controlled type with unconfigured pins must skip the writes. */
    CanTrcv_ConfigPtr = &TestConfigC;
    DioMock_WriteCount = 0U;
    CanTrcv_HwSetMode(0U, CANTRCV_TRCVMODE_NORMAL);
    CanTrcv_HwSetMode(0U, CANTRCV_TRCVMODE_STANDBY);
    CanTrcv_HwSetMode(0U, CANTRCV_TRCVMODE_SLEEP);
    CanTrcv_HwSetMode(1U, CANTRCV_TRCVMODE_NORMAL);
    TEST_ASSERT_EQUAL_UINT16(0U, DioMock_WriteCount);
}

static void test_HwGetMode(void)
{
    CanTrcv_TrcvModeType mode;

    Test_Reset();
    CanTrcv_ConfigPtr = &TestConfigA;

    /* TJA1043: ERR high + STB high -> NORMAL. */
    DioMock_Levels[12] = STD_HIGH;
    DioMock_Levels[10] = STD_HIGH;
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_TRCVMODE_NORMAL, CanTrcv_HwGetMode(0U));

    /* STB low -> SLEEP. */
    DioMock_Levels[10] = STD_LOW;
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_TRCVMODE_SLEEP, CanTrcv_HwGetMode(0U));

    /* ERR low with STB high -> STANDBY. */
    DioMock_Levels[12] = STD_LOW;
    DioMock_Levels[10] = STD_HIGH;
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_TRCVMODE_STANDBY, CanTrcv_HwGetMode(0U));

    /* TJA1042 uses the same detection logic. */
    DioMock_Levels[22] = STD_HIGH;
    DioMock_Levels[20] = STD_HIGH;
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_TRCVMODE_NORMAL, CanTrcv_HwGetMode(1U));
    DioMock_Levels[20] = STD_LOW;
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_TRCVMODE_SLEEP, CanTrcv_HwGetMode(1U));

    /* Other hardware types return the cached runtime mode. */
    CanTrcv_ConfigPtr = &TestConfigB;
    CanTrcv_Runtime[0].CurrentMode = CANTRCV_TRCVMODE_STANDBY;
    mode = CanTrcv_HwGetMode(0U);
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_TRCVMODE_STANDBY, mode);
}

/*==================================================================================================
 * Public API coverage
 *================================================================================================*/

static void test_Init_ParameterChecks(void)
{
    Test_Reset();

    CanTrcv_Init(NULL_PTR);
    TEST_ASSERT_FALSE(CanTrcv_InitStatus);
    TEST_ASSERT_EQUAL_UINT8(1U, DetMock_CallCount);
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_SID_INIT, DetMock_LastApiId);
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_E_PARAM_POINTER, DetMock_LastErrorId);
}

static void test_Init_ConfiguresHardware(void)
{
    uint8 i;

    Test_Reset();
    CanTrcv_Init(&TestConfigA);

    TEST_ASSERT_TRUE(CanTrcv_InitStatus);
    for (i = 0U; i < CANTRCV_MAX_CHANNELS; i++)
    {
        TEST_ASSERT_EQUAL_UINT8(CANTRCV_TRCVMODE_SLEEP, CanTrcv_Runtime[i].CurrentMode);
        TEST_ASSERT_EQUAL_UINT8(CANTRCV_WUMODE_ENABLE, CanTrcv_Runtime[i].WakeupMode);
        TEST_ASSERT_EQUAL_UINT8(CANTRCV_WU_NOT_SUPPORTED, CanTrcv_Runtime[i].WakeupReason);
        TEST_ASSERT_FALSE(CanTrcv_Runtime[i].WakeupPending);
        TEST_ASSERT_TRUE(CanTrcv_Runtime[i].WakeupByBusEnabled);
    }
    /* Sleep was applied: ch0 STB high (inverted), EN low; ch1 STB low, EN low. */
    TEST_ASSERT_EQUAL_UINT8(STD_HIGH, DioMock_Levels[10]);
    TEST_ASSERT_EQUAL_UINT8(STD_LOW, DioMock_Levels[11]);
    TEST_ASSERT_EQUAL_UINT8(STD_LOW, DioMock_Levels[20]);
    TEST_ASSERT_EQUAL_UINT8(STD_LOW, DioMock_Levels[21]);

    /* Second init is accepted by the production code (no DET expected). */
    CanTrcv_Init(&TestConfigA);
    TEST_ASSERT_EQUAL_UINT8(0U, DetMock_CallCount);
}

static void test_DeInit(void)
{
    Test_Reset();

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, CanTrcv_DeInit());
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_SID_DEINIT, DetMock_LastApiId);
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_E_UNINIT, DetMock_LastErrorId);

    CanTrcv_Init(&TestConfigA);
    TEST_ASSERT_EQUAL_UINT8(E_OK, CanTrcv_DeInit());
    TEST_ASSERT_FALSE(CanTrcv_InitStatus);
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_TRCVMODE_SLEEP, CanTrcv_Runtime[0].CurrentMode);
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_TRCVMODE_SLEEP, CanTrcv_Runtime[1].CurrentMode);

    /* De-init again must report UNINIT. */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, CanTrcv_DeInit());
}

static void test_SetOpMode(void)
{
    Test_Reset();

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, CanTrcv_SetOpMode(0U, CANTRCV_TRCVMODE_NORMAL));
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_SID_SETOPMODE, DetMock_LastApiId);
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_E_UNINIT, DetMock_LastErrorId);

    CanTrcv_Init(&TestConfigA);
    DetMock_CallCount = 0U;

    /* Invalid channel. */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, CanTrcv_SetOpMode(CANTRCV_MAX_CHANNELS, CANTRCV_TRCVMODE_NORMAL));
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_E_INVALID_CHANNEL, DetMock_LastErrorId);

    /* Invalid mode. */
    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, CanTrcv_SetOpMode(0U, (CanTrcv_TrcvModeType)3));
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_E_PARAM_TRCV_OPMODE, DetMock_LastErrorId);

    /* Valid NORMAL clears pending wake-up state. */
    CanTrcv_Runtime[0].WakeupPending = TRUE;
    CanTrcv_Runtime[0].WakeupReason = CANTRCV_WU_BY_BUS;
    TEST_ASSERT_EQUAL_UINT8(E_OK, CanTrcv_SetOpMode(0U, CANTRCV_TRCVMODE_NORMAL));
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_TRCVMODE_NORMAL, CanTrcv_Runtime[0].CurrentMode);
    TEST_ASSERT_FALSE(CanTrcv_Runtime[0].WakeupPending);
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_WU_NOT_SUPPORTED, CanTrcv_Runtime[0].WakeupReason);
    TEST_ASSERT_EQUAL_UINT8(STD_LOW, DioMock_Levels[10]);
    TEST_ASSERT_EQUAL_UINT8(STD_HIGH, DioMock_Levels[11]);

    /* STANDBY and SLEEP are accepted too. */
    TEST_ASSERT_EQUAL_UINT8(E_OK, CanTrcv_SetOpMode(0U, CANTRCV_TRCVMODE_STANDBY));
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_TRCVMODE_STANDBY, CanTrcv_Runtime[0].CurrentMode);
    TEST_ASSERT_EQUAL_UINT8(E_OK, CanTrcv_SetOpMode(1U, CANTRCV_TRCVMODE_SLEEP));
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_TRCVMODE_SLEEP, CanTrcv_Runtime[1].CurrentMode);
}

static void test_GetOpMode(void)
{
    CanTrcv_TrcvModeType mode = CANTRCV_TRCVMODE_SLEEP;

    Test_Reset();

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, CanTrcv_GetOpMode(0U, &mode));
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_SID_GETOPMODE, DetMock_LastApiId);
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_E_UNINIT, DetMock_LastErrorId);

    CanTrcv_Init(&TestConfigA);
    DetMock_CallCount = 0U;

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, CanTrcv_GetOpMode(CANTRCV_MAX_CHANNELS, &mode));
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_E_INVALID_CHANNEL, DetMock_LastErrorId);

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, CanTrcv_GetOpMode(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_E_PARAM_POINTER, DetMock_LastErrorId);

    TEST_ASSERT_EQUAL_UINT8(E_OK, CanTrcv_SetOpMode(0U, CANTRCV_TRCVMODE_NORMAL));
    TEST_ASSERT_EQUAL_UINT8(E_OK, CanTrcv_GetOpMode(0U, &mode));
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_TRCVMODE_NORMAL, mode);
}

static void test_GetBusWuReason(void)
{
    CanTrcv_TrcvWakeupReasonType reason = CANTRCV_WU_ERROR;

    Test_Reset();

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, CanTrcv_GetBusWuReason(0U, &reason));
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_SID_GETBUSWUREASON, DetMock_LastApiId);
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_E_UNINIT, DetMock_LastErrorId);

    CanTrcv_Init(&TestConfigA);
    DetMock_CallCount = 0U;

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, CanTrcv_GetBusWuReason(CANTRCV_MAX_CHANNELS, &reason));
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_E_INVALID_CHANNEL, DetMock_LastErrorId);

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, CanTrcv_GetBusWuReason(0U, NULL_PTR));
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_E_PARAM_POINTER, DetMock_LastErrorId);

    TEST_ASSERT_EQUAL_UINT8(E_OK, CanTrcv_GetBusWuReason(0U, &reason));
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_WU_NOT_SUPPORTED, reason);

    CanTrcv_Runtime[1].WakeupReason = CANTRCV_WU_BY_BUS;
    TEST_ASSERT_EQUAL_UINT8(E_OK, CanTrcv_GetBusWuReason(1U, &reason));
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_WU_BY_BUS, reason);
}

static void test_SetWakeupMode(void)
{
    Test_Reset();

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, CanTrcv_SetWakeupMode(0U, CANTRCV_WUMODE_ENABLE));
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_SID_SETWAKEUPMODE, DetMock_LastApiId);
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_E_UNINIT, DetMock_LastErrorId);

    CanTrcv_Init(&TestConfigA);
    DetMock_CallCount = 0U;

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, CanTrcv_SetWakeupMode(CANTRCV_MAX_CHANNELS, CANTRCV_WUMODE_ENABLE));
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_E_INVALID_CHANNEL, DetMock_LastErrorId);

    TEST_ASSERT_EQUAL_UINT8(E_NOT_OK, CanTrcv_SetWakeupMode(0U, (CanTrcv_TrcvWakeupModeType)3));
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_E_PARAM_TRCV_WAKEUP_MODE, DetMock_LastErrorId);

    TEST_ASSERT_EQUAL_UINT8(E_OK, CanTrcv_SetWakeupMode(0U, CANTRCV_WUMODE_ENABLE));
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_WUMODE_ENABLE, CanTrcv_Runtime[0].WakeupMode);
    TEST_ASSERT_TRUE(CanTrcv_Runtime[0].WakeupByBusEnabled);

    TEST_ASSERT_EQUAL_UINT8(E_OK, CanTrcv_SetWakeupMode(0U, CANTRCV_WUMODE_DISABLE));
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_WUMODE_DISABLE, CanTrcv_Runtime[0].WakeupMode);
    TEST_ASSERT_FALSE(CanTrcv_Runtime[0].WakeupByBusEnabled);

    CanTrcv_Runtime[0].WakeupPending = TRUE;
    CanTrcv_Runtime[0].WakeupReason = CANTRCV_WU_BY_BUS;
    TEST_ASSERT_EQUAL_UINT8(E_OK, CanTrcv_SetWakeupMode(0U, CANTRCV_WUMODE_CLEAR));
    TEST_ASSERT_FALSE(CanTrcv_Runtime[0].WakeupPending);
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_WU_NOT_SUPPORTED, CanTrcv_Runtime[0].WakeupReason);
}

static void test_CheckWakeupInternal(void)
{
    Test_Reset();
    CanTrcv_Init(&TestConfigA);

    /* Wake-up detection disabled -> ERR level ignored. */
    CanTrcv_Runtime[0].WakeupMode = CANTRCV_WUMODE_DISABLE;
    DioMock_Levels[12] = STD_LOW;
    CanTrcv_CheckWakeupInternal(0U);
    TEST_ASSERT_FALSE(CanTrcv_Runtime[0].WakeupPending);
    TEST_ASSERT_EQUAL_UINT16(0U, EcuMock_CallCount);

    /* Enabled but ERR high -> no wake-up. */
    CanTrcv_Runtime[0].WakeupMode = CANTRCV_WUMODE_ENABLE;
    DioMock_Levels[12] = STD_HIGH;
    CanTrcv_CheckWakeupInternal(0U);
    TEST_ASSERT_FALSE(CanTrcv_Runtime[0].WakeupPending);
    TEST_ASSERT_EQUAL_UINT16(0U, EcuMock_CallCount);

    /* Enabled and ERR low -> bus wake-up reported to EcuM. */
    DioMock_Levels[12] = STD_LOW;
    CanTrcv_CheckWakeupInternal(0U);
    TEST_ASSERT_TRUE(CanTrcv_Runtime[0].WakeupPending);
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_WU_BY_BUS, CanTrcv_Runtime[0].WakeupReason);
    TEST_ASSERT_EQUAL_UINT16(1U, EcuMock_CallCount);
    TEST_ASSERT_EQUAL_UINT32(ECUM_WKSOURCE_CAN, EcuMock_LastSource);
}

static void test_MainFunction(void)
{
    Test_Reset();

    /* Before init the cyclic function returns immediately. */
    CanTrcv_MainFunction();
    TEST_ASSERT_EQUAL_UINT16(0U, EcuMock_CallCount);
    TEST_ASSERT_EQUAL_UINT8(0U, DetMock_CallCount);

    CanTrcv_Init(&TestConfigA);

    /* Both channels enabled and ERR low -> two wake-up reports. */
    DioMock_Levels[12] = STD_LOW;
    DioMock_Levels[22] = STD_LOW;
    CanTrcv_MainFunction();
    TEST_ASSERT_EQUAL_UINT16(2U, EcuMock_CallCount);
    TEST_ASSERT_TRUE(CanTrcv_Runtime[0].WakeupPending);
    TEST_ASSERT_TRUE(CanTrcv_Runtime[1].WakeupPending);

    /* Disable channel 0 -> only channel 1 reports. */
    EcuMock_CallCount = 0U;
    CanTrcv_Runtime[0].WakeupPending = FALSE;
    (void)CanTrcv_SetWakeupMode(0U, CANTRCV_WUMODE_DISABLE);
    CanTrcv_MainFunction();
    TEST_ASSERT_EQUAL_UINT16(1U, EcuMock_CallCount);
    TEST_ASSERT_EQUAL_UINT32(ECUM_WKSOURCE_CAN1, EcuMock_LastSource);

    /* ERR high on both -> nothing reported. */
    EcuMock_CallCount = 0U;
    DioMock_Levels[22] = STD_HIGH;
    CanTrcv_MainFunction();
    TEST_ASSERT_EQUAL_UINT16(0U, EcuMock_CallCount);
}

static void test_GetVersionInfo(void)
{
    Std_VersionInfoType versionInfo;

    Test_Reset();

    CanTrcv_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_SID_GETVERSIONINFO, DetMock_LastApiId);
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_E_PARAM_POINTER, DetMock_LastErrorId);

    versionInfo.vendorID = 0U;
    versionInfo.moduleID = 0U;
    CanTrcv_GetVersionInfo(&versionInfo);
    TEST_ASSERT_EQUAL_UINT16(CANTRCV_VENDOR_ID, versionInfo.vendorID);
    TEST_ASSERT_EQUAL_UINT16(CANTRCV_MODULE_ID, versionInfo.moduleID);
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_SW_MAJOR_VERSION, versionInfo.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_SW_MINOR_VERSION, versionInfo.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(CANTRCV_SW_PATCH_VERSION, versionInfo.sw_patch_version);
}

static void test_CheckWakeupByTransceiver(void)
{
    Test_Reset();

    /* Before init: returns without evaluating any pin. */
    (void)CanTrcv_CheckWakeupByTransceiver(0U);
    TEST_ASSERT_EQUAL_UINT16(0U, EcuMock_CallCount);
    TEST_ASSERT_EQUAL_UINT8(0U, DetMock_CallCount);

    CanTrcv_Init(&TestConfigA);
    DioMock_Levels[12] = STD_LOW;

    /* Valid channel: wake-up evaluated and reported. */
    (void)CanTrcv_CheckWakeupByTransceiver(0U);
    TEST_ASSERT_TRUE(CanTrcv_Runtime[0].WakeupPending);
    TEST_ASSERT_EQUAL_UINT16(1U, EcuMock_CallCount);
    TEST_ASSERT_EQUAL_UINT32(ECUM_WKSOURCE_CAN, EcuMock_LastSource);

    /* Out-of-range channel: silently ignored. */
    EcuMock_CallCount = 0U;
    CanTrcv_Runtime[0].WakeupPending = FALSE;
    (void)CanTrcv_CheckWakeupByTransceiver(CANTRCV_MAX_CHANNELS);
    TEST_ASSERT_FALSE(CanTrcv_Runtime[0].WakeupPending);
    TEST_ASSERT_EQUAL_UINT16(0U, EcuMock_CallCount);
}

/*==================================================================================================
 * Runner
 *================================================================================================*/

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_HwSetMode_ValidModes);
    RUN_TEST(test_HwSetMode_InvalidAndSpiBranches);
    RUN_TEST(test_HwGetMode);
    RUN_TEST(test_Init_ParameterChecks);
    RUN_TEST(test_Init_ConfiguresHardware);
    RUN_TEST(test_DeInit);
    RUN_TEST(test_SetOpMode);
    RUN_TEST(test_GetOpMode);
    RUN_TEST(test_GetBusWuReason);
    RUN_TEST(test_SetWakeupMode);
    RUN_TEST(test_CheckWakeupInternal);
    RUN_TEST(test_MainFunction);
    RUN_TEST(test_GetVersionInfo);
    RUN_TEST(test_CheckWakeupByTransceiver);

    return UNITY_END();
}
