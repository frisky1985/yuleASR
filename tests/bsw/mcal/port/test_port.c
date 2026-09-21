/**
 * @file test_port.c
 * @brief Port Driver Unit Tests — Substantiated
 * @req SWS_Port
 *
 * Substantiation: DET parameter validation (module/service/error IDs), register
 * write verification via mock_registers (i.MX8M Mini, non-S32K312 build):
 * IOMUXC mux mode mapping (IOMUXC base 0x30330000, mux +0x00 / pad +0x204,
 * index (port*32+pin)*4), pad control bits (SRE/DSE/PUS/PUE), GPIO1 GDIR/DR
 * (base 0x30200000) direction and initial level handling.
 *
 * Pin encoding: port = Pin>>8, pinNum = Pin&0xFF. NOTE: the DET pin range
 * check is Pin >= PORT_TOTAL_NUM_PINS (256), so only Port A pins (0..255)
 * pass validation — PORT_B_PIN_0 (256) is rejected with PORT_E_PARAM_PIN.
 */

// @tests src/bsw/mcal/port/src/Port.c  @tests src/bsw/mcal/port/include/Port.h
#include "unity.h"
#include "mock_registers.h"
#include "mock_det.h"

#include "Port.h"
#include "Port_Cfg.h"

/* Register bases (non-S32K312 build) — must match Port.c */
#define PORT_IOMUXC_BASE        (0x30330000UL)
#define PORT_GPIO1_BASE         (0x30200000UL)
#define PORT_GPIO_DR_OFF        (0x00U)
#define PORT_GPIO_GDIR_OFF      (0x04U)

#define PORT_MUX_REG(pin)   (PORT_IOMUXC_BASE + (((uint32)((pin) >> 8) * 32U + ((pin) & 0xFFU)) * 4U))
#define PORT_PAD_REG(pin)   (PORT_IOMUXC_BASE + 0x204UL + (((uint32)((pin) >> 8) * 32U + ((pin) & 0xFFU)) * 4U))

/* mux mode values written by Port_ConfigurePinMux */
#define PORT_MUX_VAL_GPIO       (5U)
#define PORT_MUX_VAL_ALT0       (0U)
#define PORT_MUX_VAL_ALT1       (1U)
#define PORT_MUX_VAL_ALT2       (2U)
#define PORT_MUX_VAL_ALT3       (3U)

/* pad values composed by Port_ConfigurePinPad */
#define PORT_PAD_GPIO_NO_PULL   (0x02UL) /* SRE slow | DSE | keeper          */
#define PORT_PAD_GPIO_PULLUP    (0x1AUL) /* SRE slow | DSE | PUS up | PUE    */
#define PORT_PAD_ALT_PULLDOWN   (0x13UL) /* SRE fast | DSE | PUE (down)      */
#define PORT_PAD_ALT_NO_PULL    (0x03UL) /* SRE fast | DSE | keeper          */

/*
 * Configured pins (all on Port A, Pin = pinNum):
 *   pin 3: GPIO output, initial level high, direction/mode changeable
 *   pin 4: GPIO input,  pull-up, direction changeable, mode fixed
 *   pin 5: UART (ALT1), pull-down, direction fixed, mode changeable
 *   pin 6: CAN  (ALT2), direction and mode fixed
 *   pin 7: GPIO output, initial level low, direction and mode fixed
 */
#define TEST_PIN_GPIO_OUT_HIGH  (3U)
#define TEST_PIN_GPIO_IN_PULLUP (4U)
#define TEST_PIN_UART           (5U)
#define TEST_PIN_CAN            (6U)
#define TEST_PIN_GPIO_OUT_LOW   (7U)

static Port_PinConfigType testPinConfigs[5];
static Port_ConfigType testConfig;

/* Port.c keeps the initialized flag in a static variable; Port_DeInit returns
 * the driver to the uninitialized state so it can be re-initialized. The
 * tests track the current state locally (pattern of test_can.c/test_gpt.c). */
static boolean port_driverReady = FALSE;

static void test_Port_SetupConfig(void)
{
    /* pin 3: GPIO output, high initial level */
    testPinConfigs[0].Pin = TEST_PIN_GPIO_OUT_HIGH;
    testPinConfigs[0].Direction = PORT_PIN_OUT;
    testPinConfigs[0].Mode = PORT_PIN_MODE_GPIO;
    testPinConfigs[0].DirectionChangeable = TRUE;
    testPinConfigs[0].ModeChangeable = TRUE;
    testPinConfigs[0].InitialLevel = PORT_PIN_LEVEL_HIGH;
    testPinConfigs[0].PullUpEnable = FALSE;
    testPinConfigs[0].PullDownEnable = FALSE;

    /* pin 4: GPIO input with pull-up */
    testPinConfigs[1].Pin = TEST_PIN_GPIO_IN_PULLUP;
    testPinConfigs[1].Direction = PORT_PIN_IN;
    testPinConfigs[1].Mode = PORT_PIN_MODE_GPIO;
    testPinConfigs[1].DirectionChangeable = TRUE;
    testPinConfigs[1].ModeChangeable = FALSE;
    testPinConfigs[1].InitialLevel = PORT_PIN_LEVEL_LOW;
    testPinConfigs[1].PullUpEnable = TRUE;
    testPinConfigs[1].PullDownEnable = FALSE;

    /* pin 5: UART mode with pull-down */
    testPinConfigs[2].Pin = TEST_PIN_UART;
    testPinConfigs[2].Direction = PORT_PIN_OUT;
    testPinConfigs[2].Mode = PORT_PIN_MODE_UART;
    testPinConfigs[2].DirectionChangeable = FALSE;
    testPinConfigs[2].ModeChangeable = TRUE;
    testPinConfigs[2].InitialLevel = PORT_PIN_LEVEL_LOW;
    testPinConfigs[2].PullUpEnable = FALSE;
    testPinConfigs[2].PullDownEnable = TRUE;

    /* pin 6: CAN mode, nothing changeable */
    testPinConfigs[3].Pin = TEST_PIN_CAN;
    testPinConfigs[3].Direction = PORT_PIN_IN;
    testPinConfigs[3].Mode = PORT_PIN_MODE_CAN;
    testPinConfigs[3].DirectionChangeable = FALSE;
    testPinConfigs[3].ModeChangeable = FALSE;
    testPinConfigs[3].InitialLevel = PORT_PIN_LEVEL_LOW;
    testPinConfigs[3].PullUpEnable = FALSE;
    testPinConfigs[3].PullDownEnable = FALSE;

    /* pin 7: GPIO output, low initial level */
    testPinConfigs[4].Pin = TEST_PIN_GPIO_OUT_LOW;
    testPinConfigs[4].Direction = PORT_PIN_OUT;
    testPinConfigs[4].Mode = PORT_PIN_MODE_GPIO;
    testPinConfigs[4].DirectionChangeable = FALSE;
    testPinConfigs[4].ModeChangeable = FALSE;
    testPinConfigs[4].InitialLevel = PORT_PIN_LEVEL_LOW;
    testPinConfigs[4].PullUpEnable = FALSE;
    testPinConfigs[4].PullDownEnable = FALSE;

    testConfig.NumPins = 5U;
    testConfig.PinConfigs = testPinConfigs;
}

static void test_Port_EnsureInitialized(void)
{
    if (!port_driverReady) {
        test_Port_SetupConfig();
        Port_Init(&testConfig);
        Det_Mock_Reset();
        port_driverReady = TRUE;
    }
}

static void test_Port_ResetDriver(void)
{
    if (port_driverReady) {
        Port_DeInit();
        port_driverReady = FALSE;
    }
    Det_Mock_Reset();
}

void setUp(void)
{
    MockRegisters_Reset();
    Det_Mock_Reset();
}

void tearDown(void) {}

/* --- Uninitialized driver (runner executes these first) --- */

/** @req SWS_Port_00001 */
void test_Port_Init_BeforeInit_NullPtr_ShouldReportDet(void) {
    Port_Init(NULL_PTR);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PORT_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(PORT_SID_INIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PORT_E_PARAM_CONFIG, Det_MockData.ErrorId);
}

/** @req SWS_Port_00003 */
void test_Port_SetPinDirection_BeforeInit_ShouldReportDet(void) {
    Port_SetPinDirection(TEST_PIN_GPIO_OUT_HIGH, PORT_PIN_IN);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PORT_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(PORT_SID_SET_PIN_DIRECTION, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PORT_E_UNINIT, Det_MockData.ErrorId);
    /* rejected before touching the hardware */
    TEST_ASSERT_EQUAL_UINT32(0U, MockRegisters_Read32(PORT_GPIO1_BASE + PORT_GPIO_GDIR_OFF));
}

/** @req SWS_Port_00006 */
void test_Port_SetPinMode_BeforeInit_ShouldReportDet(void) {
    Port_SetPinMode(TEST_PIN_UART, PORT_PIN_MODE_SPI);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PORT_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(PORT_SID_SET_PIN_MODE, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PORT_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Port_00004 */
void test_Port_RefreshPortDirection_BeforeInit_ShouldReportDet(void) {
    Port_RefreshPortDirection();
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PORT_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(PORT_SID_REFRESH_PORT_DIRECTION, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PORT_E_UNINIT, Det_MockData.ErrorId);
}

/** @req SWS_Port_00002 */
void test_Port_DeInit_BeforeInit_ShouldReportDet(void) {
    Port_DeInit();
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PORT_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(PORT_SID_DEINIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PORT_E_UNINIT, Det_MockData.ErrorId);
}

/* --- Init --- */

/** @req SWS_Port_00001 */
void test_Port_Init_ValidConfig_ShouldProgramPinRegisters(void) {
    test_Port_ResetDriver();   /* in case an earlier test already initialized */
    test_Port_SetupConfig();
    Port_Init(&testConfig);
    port_driverReady = TRUE;
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);

    /* mux mode mapping: GPIO=5, UART(ALT1)=1, CAN(ALT2)=2 */
    TEST_ASSERT_EQUAL_UINT32(PORT_MUX_VAL_GPIO,
                             MockRegisters_Read32(PORT_MUX_REG(TEST_PIN_GPIO_OUT_HIGH)));
    TEST_ASSERT_EQUAL_UINT32(PORT_MUX_VAL_GPIO,
                             MockRegisters_Read32(PORT_MUX_REG(TEST_PIN_GPIO_IN_PULLUP)));
    TEST_ASSERT_EQUAL_UINT32(PORT_MUX_VAL_ALT1,
                             MockRegisters_Read32(PORT_MUX_REG(TEST_PIN_UART)));
    TEST_ASSERT_EQUAL_UINT32(PORT_MUX_VAL_ALT2,
                             MockRegisters_Read32(PORT_MUX_REG(TEST_PIN_CAN)));
    TEST_ASSERT_EQUAL_UINT32(PORT_MUX_VAL_GPIO,
                             MockRegisters_Read32(PORT_MUX_REG(TEST_PIN_GPIO_OUT_LOW)));

    /* pad control: GPIO slow slew, alternate fast slew, pull config */
    TEST_ASSERT_EQUAL_UINT32(PORT_PAD_GPIO_NO_PULL,
                             MockRegisters_Read32(PORT_PAD_REG(TEST_PIN_GPIO_OUT_HIGH)));
    TEST_ASSERT_EQUAL_UINT32(PORT_PAD_GPIO_PULLUP,
                             MockRegisters_Read32(PORT_PAD_REG(TEST_PIN_GPIO_IN_PULLUP)));
    TEST_ASSERT_EQUAL_UINT32(PORT_PAD_ALT_PULLDOWN,
                             MockRegisters_Read32(PORT_PAD_REG(TEST_PIN_UART)));
    TEST_ASSERT_EQUAL_UINT32(PORT_PAD_ALT_NO_PULL,
                             MockRegisters_Read32(PORT_PAD_REG(TEST_PIN_CAN)));
    TEST_ASSERT_EQUAL_UINT32(PORT_PAD_GPIO_NO_PULL,
                             MockRegisters_Read32(PORT_PAD_REG(TEST_PIN_GPIO_OUT_LOW)));

    /* GPIO direction: outputs pin3 and pin7 set, input pin4 clear */
    TEST_ASSERT_EQUAL_UINT32(((1UL << TEST_PIN_GPIO_OUT_HIGH) | (1UL << TEST_PIN_GPIO_OUT_LOW)),
                             MockRegisters_Read32(PORT_GPIO1_BASE + PORT_GPIO_GDIR_OFF));
    /* initial output level: pin3 high, pin7 low */
    TEST_ASSERT_EQUAL_UINT32((1UL << TEST_PIN_GPIO_OUT_HIGH),
                             MockRegisters_Read32(PORT_GPIO1_BASE + PORT_GPIO_DR_OFF));
}

/** @req SWS_Port_00001 */
void test_Port_Init_DoubleInit_ShouldReportDet(void) {
    test_Port_EnsureInitialized();
    /* registers are blank in this test (setUp) — a second Init must abort
     * on DET before writing anything */
    Port_Init(&testConfig);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PORT_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(PORT_SID_INIT, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PORT_E_ALREADY_INITIALIZED, Det_MockData.ErrorId);
    TEST_ASSERT_EQUAL_UINT32(0U, MockRegisters_Read32(PORT_MUX_REG(TEST_PIN_GPIO_OUT_HIGH)));
    TEST_ASSERT_EQUAL_UINT32(0U, MockRegisters_Read32(PORT_GPIO1_BASE + PORT_GPIO_GDIR_OFF));
}

/** @req SWS_Port_00002 */
void test_Port_DeInit_AfterInit_ShouldAllowReInit(void) {
    test_Port_EnsureInitialized();
    Port_DeInit();
    port_driverReady = FALSE;
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
    /* re-initialization is accepted after DeInit */
    test_Port_SetupConfig();
    Port_Init(&testConfig);
    port_driverReady = TRUE;
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL_UINT32(PORT_MUX_VAL_GPIO,
                             MockRegisters_Read32(PORT_MUX_REG(TEST_PIN_GPIO_OUT_HIGH)));
    TEST_ASSERT_EQUAL_UINT32(((1UL << TEST_PIN_GPIO_OUT_HIGH) | (1UL << TEST_PIN_GPIO_OUT_LOW)),
                             MockRegisters_Read32(PORT_GPIO1_BASE + PORT_GPIO_GDIR_OFF));
}

/* --- SetPinDirection --- */

/** @req SWS_Port_00003 */
void test_Port_SetPinDirection_Changeable_ShouldUpdateGdir(void) {
    test_Port_EnsureInitialized();
    /* pin3 is configured direction-changeable; drive it to input */
    MockRegisters_Write32(PORT_GPIO1_BASE + PORT_GPIO_GDIR_OFF, (1UL << TEST_PIN_GPIO_OUT_HIGH));
    Port_SetPinDirection(TEST_PIN_GPIO_OUT_HIGH, PORT_PIN_IN);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL_UINT32(0U, MockRegisters_Read32(PORT_GPIO1_BASE + PORT_GPIO_GDIR_OFF));
    /* pin4 is configured direction-changeable; drive it to output */
    Port_SetPinDirection(TEST_PIN_GPIO_IN_PULLUP, PORT_PIN_OUT);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL_UINT32((1UL << TEST_PIN_GPIO_IN_PULLUP),
                             MockRegisters_Read32(PORT_GPIO1_BASE + PORT_GPIO_GDIR_OFF));
}

/** @req SWS_Port_00003 */
void test_Port_SetPinDirection_Unchangeable_ShouldReportDet(void) {
    test_Port_EnsureInitialized();
    MockRegisters_Write32(PORT_GPIO1_BASE + PORT_GPIO_GDIR_OFF, 0U);
    /* pin5 is configured direction-fixed */
    Port_SetPinDirection(TEST_PIN_UART, PORT_PIN_OUT);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PORT_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(PORT_SID_SET_PIN_DIRECTION, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PORT_E_DIRECTION_UNCHANGEABLE, Det_MockData.ErrorId);
    TEST_ASSERT_EQUAL_UINT32(0U, MockRegisters_Read32(PORT_GPIO1_BASE + PORT_GPIO_GDIR_OFF));
}

/** @req SWS_Port_00003 */
void test_Port_SetPinDirection_PinNotInConfig_ShouldReportDet(void) {
    test_Port_EnsureInitialized();
    /* pin 20 is not part of the configuration -> treated as unchangeable */
    Port_SetPinDirection(20U, PORT_PIN_OUT);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PORT_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(PORT_SID_SET_PIN_DIRECTION, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PORT_E_DIRECTION_UNCHANGEABLE, Det_MockData.ErrorId);
}

/** @req SWS_Port_00003 */
void test_Port_SetPinDirection_InvalidPin_ShouldReportDet(void) {
    test_Port_EnsureInitialized();
    /* Pin range check is Pin >= PORT_TOTAL_NUM_PINS (256): only Port A pins
     * (0..255) are valid; PORT_B_PIN_0 (== 256) is already out of range. */
    Port_SetPinDirection(PORT_TOTAL_NUM_PINS, PORT_PIN_OUT);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PORT_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(PORT_SID_SET_PIN_DIRECTION, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PORT_E_PARAM_PIN, Det_MockData.ErrorId);
    TEST_ASSERT_EQUAL_UINT32(PORT_TOTAL_NUM_PINS, (uint32)PORT_B_PIN_0);
}

/* --- SetPinMode --- */

/** @req SWS_Port_00006 */
void test_Port_SetPinMode_Changeable_ShouldUpdateMux(void) {
    test_Port_EnsureInitialized();
    /* pin5 is configured mode-changeable; keep upper bits, switch to SPI (ALT3) */
    uint32 muxAddr = PORT_MUX_REG(TEST_PIN_UART);
    MockRegisters_Write32(muxAddr, 0xFFU);
    Port_SetPinMode(TEST_PIN_UART, PORT_PIN_MODE_SPI);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL_UINT32(0xF8UL | PORT_MUX_VAL_ALT3, MockRegisters_Read32(muxAddr));
}

/** @req SWS_Port_00006 */
void test_Port_SetPinMode_Unchangeable_ShouldReportDet(void) {
    test_Port_EnsureInitialized();
    uint32 muxAddr = PORT_MUX_REG(TEST_PIN_CAN);
    MockRegisters_Write32(muxAddr, PORT_MUX_VAL_ALT2);
    /* pin6 is configured mode-fixed */
    Port_SetPinMode(TEST_PIN_CAN, PORT_PIN_MODE_GPIO);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PORT_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(PORT_SID_SET_PIN_MODE, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PORT_E_MODE_UNCHANGEABLE, Det_MockData.ErrorId);
    TEST_ASSERT_EQUAL_UINT32(PORT_MUX_VAL_ALT2, MockRegisters_Read32(muxAddr));
}

/** @req SWS_Port_00006 */
void test_Port_SetPinMode_InvalidMode_ShouldReportDet(void) {
    test_Port_EnsureInitialized();
    /* valid modes are 0..PORT_PIN_MODE_DISABLED (15) */
    Port_SetPinMode(TEST_PIN_UART, (Port_PinModeType)(PORT_PIN_MODE_DISABLED + 1U));
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PORT_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(PORT_SID_SET_PIN_MODE, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PORT_E_PARAM_INVALID_MODE, Det_MockData.ErrorId);
}

/** @req SWS_Port_00006 */
void test_Port_SetPinMode_InvalidPin_ShouldReportDet(void) {
    test_Port_EnsureInitialized();
    Port_SetPinMode(PORT_TOTAL_NUM_PINS, PORT_PIN_MODE_GPIO);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PORT_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(PORT_SID_SET_PIN_MODE, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PORT_E_PARAM_PIN, Det_MockData.ErrorId);
}

/* --- RefreshPortDirection --- */

/** @req SWS_Port_00004 */
void test_Port_RefreshPortDirection_ShouldRestoreConfiguredDirections(void) {
    test_Port_EnsureInitialized();
    /* invert the configured GPIO directions and set a foreign bit (pin0);
     * non-GPIO pins (5/6) are never touched by the refresh */
    MockRegisters_Write32(PORT_GPIO1_BASE + PORT_GPIO_GDIR_OFF,
                          (1UL << TEST_PIN_GPIO_IN_PULLUP) | (1UL << 0U));
    Port_RefreshPortDirection();
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
    /* pin3 -> output, pin4 -> input, foreign bit0 preserved, pin7 -> output */
    TEST_ASSERT_EQUAL_UINT32((1UL << TEST_PIN_GPIO_OUT_HIGH) | (1UL << TEST_PIN_GPIO_OUT_LOW) | (1UL << 0U),
                             MockRegisters_Read32(PORT_GPIO1_BASE + PORT_GPIO_GDIR_OFF));
}

/* --- GetVersionInfo --- */

/** @req SWS_Port_00005 */
void test_Port_GetVersionInfo_ValidPtr_ShouldFillFields(void) {
    Std_VersionInfoType info;
    info.vendorID = 0U;
    info.moduleID = 0U;
    info.sw_major_version = 0U;
    info.sw_minor_version = 0U;
    info.sw_patch_version = 0U;
    Port_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT32(0U, Det_MockData.CallCount);
    TEST_ASSERT_EQUAL_UINT16(PORT_VENDOR_ID, info.vendorID);
    TEST_ASSERT_EQUAL_UINT16(PORT_MODULE_ID, info.moduleID);
    TEST_ASSERT_EQUAL_UINT8(PORT_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT8(PORT_SW_MINOR_VERSION, info.sw_minor_version);
    TEST_ASSERT_EQUAL_UINT8(PORT_SW_PATCH_VERSION, info.sw_patch_version);
}

/** @req SWS_Port_00005 */
void test_Port_GetVersionInfo_NullPtr_ShouldReportDet(void) {
    Port_GetVersionInfo(NULL_PTR);
    TEST_ASSERT_TRUE(Det_MockData.LastCallValid);
    TEST_ASSERT_EQUAL(PORT_MODULE_ID, Det_MockData.ModuleId);
    TEST_ASSERT_EQUAL(PORT_SID_GET_VERSION_INFO, Det_MockData.ApiId);
    TEST_ASSERT_EQUAL(PORT_E_PARAM_POINTER, Det_MockData.ErrorId);
}
