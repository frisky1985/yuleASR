/**
 * @file test_mcal_port.c
 * @brief Port unit test — links real Port.c + Port_Lcfg.c production code
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "mock_registers.h"
#include "Port.h"

static const uint32 PORT_IOMUXC_BASE = 0x30330000UL;
static const uint32 PORT_SW_MUX_CTL_OFF = 0x0000U;
static const uint32 PORT_SW_PAD_CTL_OFF = 0x0204U;
static const uint32 PORT_GPIO1_BASE = 0x30200000UL;
static const uint32 PORT_GPIO_GDIR_OFF = 0x04U;
static const uint32 PORT_GPIO_DR_OFF = 0x00U;

extern void Port_Init(const Port_ConfigType* ConfigPtr);
extern void Port_RefreshPortDirection(void);
extern void Port_SetPinDirection(Port_PinType Pin, Port_PinDirectionType Direction);
extern void Port_GetVersionInfo(Std_VersionInfoType* versioninfo);
extern void Port_SetPinMode(Port_PinType Pin, Port_PinModeType Mode);

void setUp(void) { MockRegisters_Reset(); }
void tearDown(void) {}

/* ========= Port_Init ========= */
void test_Port_Init_NullConfig(void) {
    Port_Init(NULL); /* Should report DET error, return */
}

void test_Port_Init_Basic(void) {
    /* Minimal config with one GPIO output pin */
    Port_ConfigType cfg;
    memset(&cfg, 0, sizeof(cfg));
    Port_Init(&cfg); /* cfg.NumPins = 0, should work */
}

void test_Port_Init_WithGpioOutPin(void) {
    /* Test with 1 pin that configures GPIO direction */
    Port_PinConfigType pinCfg[] = {{
        .Pin = 0x0001, .Mode = PORT_PIN_MODE_GPIO,
        .Direction = PORT_PIN_OUT, .InitialLevel = PORT_PIN_LEVEL_HIGH,
        .DirectionChangeable = TRUE, .ModeChangeable = TRUE
    }};
    Port_ConfigType cfg = { .NumPins = 1, .PinConfigs = pinCfg };
    Port_Init(&cfg);
    uint32 gdir = MockRegisters_Read32(PORT_GPIO1_BASE + PORT_GPIO_GDIR_OFF);
    TEST_ASSERT_TRUE(gdir & 0x02);
    uint32 dr = MockRegisters_Read32(PORT_GPIO1_BASE + PORT_GPIO_DR_OFF);
    TEST_ASSERT_TRUE(dr & 0x02);
}

void test_Port_Init_WithGpioInPin(void) {
    Port_PinConfigType pinCfg[] = {{
        .Pin = 0x0001, .Mode = PORT_PIN_MODE_GPIO,
        .Direction = PORT_PIN_IN, .InitialLevel = PORT_PIN_LEVEL_LOW,
        .DirectionChangeable = TRUE, .ModeChangeable = TRUE
    }};
    Port_ConfigType cfg = { .NumPins = 1, .PinConfigs = pinCfg };
    Port_Init(&cfg);
    uint32 gdir = MockRegisters_Read32(PORT_GPIO1_BASE + PORT_GPIO_GDIR_OFF);
    TEST_ASSERT_FALSE(gdir & 0x02);
}

void test_Port_Init_DoubleInit(void) {
    Port_ConfigType cfg; memset(&cfg, 0, sizeof(cfg));
    Port_Init(&cfg);
    Port_Init(&cfg); /* Should report already-initialized error */
}

/* ========= Port_SetPinDirection ========= */
void test_Port_SetPinDirection_BeforeInit(void) {
    Port_SetPinDirection(0, PORT_PIN_OUT); /* Should fail uninit */
}

void test_Port_SetPinDirection_Valid(void) {
    Port_PinConfigType pinCfg[] = {{
        .Pin = 0x0001, .Mode = PORT_PIN_MODE_GPIO,
        .Direction = PORT_PIN_OUT, .DirectionChangeable = TRUE,
        .ModeChangeable = TRUE
    }};
    Port_ConfigType cfg = { .NumPins = 1, .PinConfigs = pinCfg };
    Port_Init(&cfg);
    Port_SetPinDirection(0x0001, PORT_PIN_IN);
    uint32 gdir = MockRegisters_Read32(PORT_GPIO1_BASE + PORT_GPIO_GDIR_OFF);
    TEST_ASSERT_FALSE(gdir & 0x02);
}

void test_Port_SetPinDirection_InvalidPin(void) {
    Port_PinConfigType pinCfg[] = {{
        .Pin = 0, .Mode = PORT_PIN_MODE_GPIO,
        .Direction = PORT_PIN_OUT, .DirectionChangeable = TRUE,
        .ModeChangeable = TRUE
    }};
    Port_ConfigType cfg = { .NumPins = 1, .PinConfigs = pinCfg };
    Port_Init(&cfg);
    /* PORT_TOTAL_NUM_PINS - if not defined, defaults. Use a large invalid pin */
    Port_SetPinDirection(0xFFFF, PORT_PIN_OUT);
}

void test_Port_SetPinDirection_NotChangeable(void) {
    Port_PinConfigType pinCfg[] = {{
        .Pin = 0x0001, .Mode = PORT_PIN_MODE_GPIO,
        .Direction = PORT_PIN_OUT, .DirectionChangeable = FALSE,
        .ModeChangeable = TRUE
    }};
    Port_ConfigType cfg = { .NumPins = 1, .PinConfigs = pinCfg };
    Port_Init(&cfg);
    Port_SetPinDirection(0x0001, PORT_PIN_IN); /* Should fail, not changeable */
}

/* ========= Port_RefreshPortDirection ========= */
void test_Port_RefreshPortDirection_BeforeInit(void) {
    Port_RefreshPortDirection(); /* Should fail uninit */
}

void test_Port_RefreshPortDirection_AfterInit(void) {
    Port_PinConfigType pinCfg[] = {{
        .Pin = 0x0001, .Mode = PORT_PIN_MODE_GPIO,
        .Direction = PORT_PIN_OUT, .DirectionChangeable = TRUE,
        .ModeChangeable = TRUE
    }};
    Port_ConfigType cfg = { .NumPins = 1, .PinConfigs = pinCfg };
    Port_Init(&cfg);
    MockRegisters_Write32(PORT_GPIO1_BASE + PORT_GPIO_GDIR_OFF, 0);
    Port_RefreshPortDirection();
    uint32 gdir = MockRegisters_Read32(PORT_GPIO1_BASE + PORT_GPIO_GDIR_OFF);
    TEST_ASSERT_TRUE(gdir & 0x02);
}

/* ========= Port_GetVersionInfo ========= */
void test_Port_GetVersionInfo_Valid(void) {
    Std_VersionInfoType vi; memset(&vi, 0, sizeof(vi));
    Port_GetVersionInfo(&vi);
    TEST_ASSERT_EQUAL(0x0055U, vi.vendorID);
    TEST_ASSERT_EQUAL(0x0074U, vi.moduleID);
}

void test_Port_GetVersionInfo_Null(void) {
    Port_GetVersionInfo(NULL); /* Should not crash */
}

/* ========= Port_SetPinMode ========= */
void test_Port_SetPinMode_BeforeInit(void) {
    Port_SetPinMode(0, PORT_PIN_MODE_GPIO); /* Should fail uninit */
}

void test_Port_SetPinMode_Valid(void) {
    Port_PinConfigType pinCfg[] = {{
        .Pin = 0x0001, .Mode = PORT_PIN_MODE_GPIO,
        .Direction = PORT_PIN_OUT, .ModeChangeable = TRUE,
        .DirectionChangeable = TRUE
    }};
    Port_ConfigType cfg = { .NumPins = 1, .PinConfigs = pinCfg };
    Port_Init(&cfg);
    Port_SetPinMode(0x0001, PORT_PIN_MODE_CAN);
    /* Verify mux register was written */
    uint32 muxAddr = PORT_IOMUXC_BASE + PORT_SW_MUX_CTL_OFF + ((0*32+1)*4);
    uint32 muxVal = MockRegisters_Read32(muxAddr);
    TEST_ASSERT_EQUAL(2, muxVal & 0x07); /* ALT2 = 2 for CAN */
}

void test_Port_SetPinMode_ModeNotChangeable(void) {
    Port_PinConfigType pinCfg[] = {{
        .Pin = 0x0001, .Mode = PORT_PIN_MODE_GPIO,
        .Direction = PORT_PIN_OUT, .ModeChangeable = FALSE,
        .DirectionChangeable = TRUE
    }};
    Port_ConfigType cfg = { .NumPins = 1, .PinConfigs = pinCfg };
    Port_Init(&cfg);
    Port_SetPinMode(0x0001, PORT_PIN_MODE_CAN); /* Should fail, not changeable */
}

void test_Port_SetPinMode_InvalidPin(void) {
    Port_PinConfigType pinCfg[] = {{
        .Pin = 0, .Mode = PORT_PIN_MODE_GPIO,
        .Direction = PORT_PIN_OUT, .ModeChangeable = TRUE,
        .DirectionChangeable = TRUE
    }};
    Port_ConfigType cfg = { .NumPins = 1, .PinConfigs = pinCfg };
    Port_Init(&cfg);
    Port_SetPinMode(0xFFFF, PORT_PIN_MODE_GPIO); /* Should fail invalid pin */
}

/* ========= Main ========= */
int main(void) {
    UnityBegin();
    UnityRunTest(test_Port_Init_NullConfig, "Init NULL config", __LINE__);
    UnityRunTest(test_Port_Init_Basic, "Init basic", __LINE__);
    UnityRunTest(test_Port_Init_WithGpioOutPin, "Init GPIO out", __LINE__);
    UnityRunTest(test_Port_Init_WithGpioInPin, "Init GPIO in", __LINE__);
    UnityRunTest(test_Port_Init_DoubleInit, "Double init", __LINE__);
    UnityRunTest(test_Port_SetPinDirection_BeforeInit, "SetPinDir before init", __LINE__);
    UnityRunTest(test_Port_SetPinDirection_Valid, "SetPinDir valid", __LINE__);
    UnityRunTest(test_Port_SetPinDirection_InvalidPin, "SetPinDir invalid pin", __LINE__);
    UnityRunTest(test_Port_SetPinDirection_NotChangeable, "SetPinDir not changeable", __LINE__);
    UnityRunTest(test_Port_RefreshPortDirection_BeforeInit, "RefreshDir before init", __LINE__);
    UnityRunTest(test_Port_RefreshPortDirection_AfterInit, "RefreshDir after init", __LINE__);
    UnityRunTest(test_Port_GetVersionInfo_Valid, "GetVersionInfo valid", __LINE__);
    UnityRunTest(test_Port_GetVersionInfo_Null, "GetVersionInfo null", __LINE__);
    UnityRunTest(test_Port_SetPinMode_BeforeInit, "SetPinMode before init", __LINE__);
    UnityRunTest(test_Port_SetPinMode_Valid, "SetPinMode valid", __LINE__);
    UnityRunTest(test_Port_SetPinMode_ModeNotChangeable, "SetPinMode not changeable", __LINE__);
    UnityRunTest(test_Port_SetPinMode_InvalidPin, "SetPinMode invalid pin", __LINE__);
    return UnityEnd();
}
