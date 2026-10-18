"""
MCAL Chip Template Database

Provides chip-specific MCAL module configurations for AUTOSAR projects.
Supports automatic module selection and default parameter configuration
based on target chip selection.
"""

from dataclasses import dataclass, field
from typing import List, Dict, Optional
from enum import Enum


class ChipVendor(Enum):
    """Chip vendors"""
    INFINEON = "infineon"
    NXP = "nxp"
    RENESAS = "renesas"
    ST = "st"
    TI = "ti"


class McalModuleType(Enum):
    """MCAL module types"""
    MCU = "Mcu"
    PORT = "Port"
    CAN = "Can"
    LIN = "Lin"
    SPI = "Spi"
    I2C = "I2c"
    GPT = "Gpt"
    ADC = "Adc"
    PWM = "Pwm"
    ICU = "Icu"
    OCU = "Ocu"
    WDG = "Wdg"
    EEP = "Eep"
    FLS = "Fls"
    ETH = "Eth"
    UART = "Uart"


@dataclass
class McalModuleConfig:
    """MCAL module configuration template"""
    name: str
    label: str
    required: bool = False
    default_params: Dict = field(default_factory=dict)
    dependencies: List[str] = field(default_factory=list)


@dataclass
class ChipModel:
    """Chip model definition"""
    name: str
    label: str
    vendor: ChipVendor
    series: str
    clock_freq_mhz: int
    flash_size_kb: int
    ram_size_kb: int
    mcal_modules: List[McalModuleConfig]
    notes: str = ""


@dataclass
class ChipSeries:
    """Chip series definition"""
    name: str
    label: str
    vendor: ChipVendor
    models: List[ChipModel]


# ============================================================================
# Chip Database
# ============================================================================

CHIP_DATABASE: Dict[ChipVendor, Dict[str, ChipSeries]] = {
    ChipVendor.INFINEON: {
        "tc3xx": ChipSeries(
            name="tc3xx",
            label="AURIX TC3xx",
            vendor=ChipVendor.INFINEON,
            models=[
                ChipModel(
                    name="tc375",
                    label="TC375",
                    vendor=ChipVendor.INFINEON,
                    series="tc3xx",
                    clock_freq_mhz=200,
                    flash_size_kb=3072,
                    ram_size_kb=384,
                    mcal_modules=[
                        McalModuleConfig("Mcu", "MCU Driver", required=True, default_params={
                            "McuClockSettingConfig": [{"McuClockReferencePointFrequency": 200000000}],
                            "McuDefaultMode": "McuModeType.MCU_MODE_RUN",
                        }),
                        McalModuleConfig("Port", "Port Driver", required=True, default_params={
                            "PortConfigSet": [{"PortPinDirection": "PORT_PIN_OUT", "PortPinLevel": "HIGH"}],
                        }),
                        McalModuleConfig("Can", "CAN Driver", default_params={
                            "CanControllerBaudrate": 500000,
                            "CanControllerPropSeg": 6,
                            "CanControllerSeg1": 13,
                            "CanControllerSeg2": 2,
                        }),
                        McalModuleConfig("Spi", "SPI Driver", default_params={
                            "SpiChannelConfig": [{"SpiChannelBaudrate": 1000000}],
                        }),
                        McalModuleConfig("Gpt", "General Purpose Timer", default_params={
                            "GptChannelConfig": [{"GptChannelMode": "GPT_CHANNEL_MODE_NORMAL"}],
                        }),
                        McalModuleConfig("Adc", "ADC Driver", default_params={
                            "AdcGroupConfig": [{"AdcConversionMode": "ADC_CONVERSION_MODE_CONTINUOUS"}],
                        }),
                        McalModuleConfig("Pwm", "PWM Driver"),
                        McalModuleConfig("Wdg", "Watchdog Driver", required=True, default_params={
                            "WdgTimeout": 1000,
                            "WdgTriggerCondition": "WDG_TRIGGER_CONDITION_ALWAYS",
                        }),
                        McalModuleConfig("Icu", "Input Capture Unit"),
                        McalModuleConfig("Lin", "LIN Driver"),
                        McalModuleConfig("Eep", "EEPROM Driver"),
                        McalModuleConfig("Fls", "Flash Driver"),
                    ],
                    notes="AURIX TC375 - Triple-core 32-bit MCU for automotive applications",
                ),
                ChipModel(
                    name="tc397",
                    label="TC397",
                    vendor=ChipVendor.INFINEON,
                    series="tc3xx",
                    clock_freq_mhz=300,
                    flash_size_kb=4096,
                    ram_size_kb=512,
                    mcal_modules=[
                        McalModuleConfig("Mcu", "MCU Driver", required=True),
                        McalModuleConfig("Port", "Port Driver", required=True),
                        McalModuleConfig("Can", "CAN Driver"),
                        McalModuleConfig("Spi", "SPI Driver"),
                        McalModuleConfig("Gpt", "General Purpose Timer"),
                        McalModuleConfig("Adc", "ADC Driver"),
                        McalModuleConfig("Pwm", "PWM Driver"),
                        McalModuleConfig("Wdg", "Watchdog Driver", required=True),
                        McalModuleConfig("Icu", "Input Capture Unit"),
                        McalModuleConfig("Lin", "LIN Driver"),
                        McalModuleConfig("Eep", "EEPROM Driver"),
                        McalModuleConfig("Fls", "Flash Driver"),
                        McalModuleConfig("Eth", "Ethernet Driver", default_params={
                            "EthControllerConfig": [{"EthSpeed": "ETH_SPEED_100MBPS"}],
                        }),
                    ],
                    notes="AURIX TC397 - High-performance triple-core MCU with Ethernet",
                ),
            ],
        ),
    },
    ChipVendor.NXP: {
        "s32k3": ChipSeries(
            name="s32k3",
            label="S32K3",
            vendor=ChipVendor.NXP,
            models=[
                ChipModel(
                    name="s32k344",
                    label="S32K344",
                    vendor=ChipVendor.NXP,
                    series="s32k3",
                    clock_freq_mhz=160,
                    flash_size_kb=512,
                    ram_size_kb=64,
                    mcal_modules=[
                        McalModuleConfig("Mcu", "MCU Driver", required=True),
                        McalModuleConfig("Port", "Port Driver", required=True),
                        McalModuleConfig("Can", "CAN Driver"),
                        McalModuleConfig("Spi", "SPI Driver"),
                        McalModuleConfig("Gpt", "General Purpose Timer"),
                        McalModuleConfig("Adc", "ADC Driver"),
                        McalModuleConfig("Pwm", "PWM Driver"),
                        McalModuleConfig("Wdg", "Watchdog Driver", required=True),
                        McalModuleConfig("Icu", "Input Capture Unit"),
                        McalModuleConfig("Lin", "LIN Driver"),
                        McalModuleConfig("Eep", "EEPROM Driver"),
                    ],
                    notes="S32K344 - Arm Cortex-M7 based MCU for automotive body and gateway",
                ),
                ChipModel(
                    name="s32k358",
                    label="S32K358",
                    vendor=ChipVendor.NXP,
                    series="s32k3",
                    clock_freq_mhz=200,
                    flash_size_kb=2048,
                    ram_size_kb=512,
                    mcal_modules=[
                        McalModuleConfig("Mcu", "MCU Driver", required=True),
                        McalModuleConfig("Port", "Port Driver", required=True),
                        McalModuleConfig("Can", "CAN Driver"),
                        McalModuleConfig("Spi", "SPI Driver"),
                        McalModuleConfig("Gpt", "General Purpose Timer"),
                        McalModuleConfig("Adc", "ADC Driver"),
                        McalModuleConfig("Pwm", "PWM Driver"),
                        McalModuleConfig("Wdg", "Watchdog Driver", required=True),
                        McalModuleConfig("Icu", "Input Capture Unit"),
                        McalModuleConfig("Lin", "LIN Driver"),
                        McalModuleConfig("Eep", "EEPROM Driver"),
                        McalModuleConfig("Eth", "Ethernet Driver"),
                    ],
                    notes="S32K358 - High-performance MCU with Ethernet for domain controllers",
                ),
            ],
        ),
    },
    ChipVendor.RENESAS: {
        "rh850": ChipSeries(
            name="rh850",
            label="RH850",
            vendor=ChipVendor.RENESAS,
            models=[
                ChipModel(
                    name="rh850u2a",
                    label="RH850/U2A",
                    vendor=ChipVendor.RENESAS,
                    series="rh850",
                    clock_freq_mhz=240,
                    flash_size_kb=4096,
                    ram_size_kb=512,
                    mcal_modules=[
                        McalModuleConfig("Mcu", "MCU Driver", required=True),
                        McalModuleConfig("Port", "Port Driver", required=True),
                        McalModuleConfig("Can", "CAN Driver"),
                        McalModuleConfig("Spi", "SPI Driver"),
                        McalModuleConfig("Gpt", "General Purpose Timer"),
                        McalModuleConfig("Adc", "ADC Driver"),
                        McalModuleConfig("Pwm", "PWM Driver"),
                        McalModuleConfig("Wdg", "Watchdog Driver", required=True),
                        McalModuleConfig("Icu", "Input Capture Unit"),
                        McalModuleConfig("Lin", "LIN Driver"),
                        McalModuleConfig("Eep", "EEPROM Driver"),
                        McalModuleConfig("Fls", "Flash Driver"),
                    ],
                    notes="RH850/U2A - 32-bit MCU for automotive body and chassis",
                ),
            ],
        ),
    },
    ChipVendor.ST: {
        "spc58": ChipSeries(
            name="spc58",
            label="SPC58",
            vendor=ChipVendor.ST,
            models=[
                ChipModel(
                    name="spc584b",
                    label="SPC584B",
                    vendor=ChipVendor.ST,
                    series="spc58",
                    clock_freq_mhz=200,
                    flash_size_kb=4096,
                    ram_size_kb=512,
                    mcal_modules=[
                        McalModuleConfig("Mcu", "MCU Driver", required=True),
                        McalModuleConfig("Port", "Port Driver", required=True),
                        McalModuleConfig("Can", "CAN Driver"),
                        McalModuleConfig("Spi", "SPI Driver"),
                        McalModuleConfig("Gpt", "General Purpose Timer"),
                        McalModuleConfig("Adc", "ADC Driver"),
                        McalModuleConfig("Pwm", "PWM Driver"),
                        McalModuleConfig("Wdg", "Watchdog Driver", required=True),
                        McalModuleConfig("Icu", "Input Capture Unit"),
                        McalModuleConfig("Lin", "LIN Driver"),
                        McalModuleConfig("Eep", "EEPROM Driver"),
                    ],
                    notes="SPC584B - Power Architecture based MCU for automotive",
                ),
            ],
        ),
    },
    ChipVendor.TI: {
        "tda4": ChipSeries(
            name="tda4",
            label="TDA4",
            vendor=ChipVendor.TI,
            models=[
                ChipModel(
                    name="tda4vm",
                    label="TDA4VM",
                    vendor=ChipVendor.TI,
                    series="tda4",
                    clock_freq_mhz=1000,
                    flash_size_kb=8192,
                    ram_size_kb=4096,
                    mcal_modules=[
                        McalModuleConfig("Mcu", "MCU Driver", required=True),
                        McalModuleConfig("Port", "Port Driver", required=True),
                        McalModuleConfig("Can", "CAN Driver"),
                        McalModuleConfig("Spi", "SPI Driver"),
                        McalModuleConfig("Gpt", "General Purpose Timer"),
                        McalModuleConfig("Adc", "ADC Driver"),
                        McalModuleConfig("Pwm", "PWM Driver"),
                        McalModuleConfig("Wdg", "Watchdog Driver", required=True),
                        McalModuleConfig("Icu", "Input Capture Unit"),
                        McalModuleConfig("Eth", "Ethernet Driver"),
                    ],
                    notes="TDA4VM - High-performance SoC for ADAS and autonomous driving",
                ),
            ],
        ),
    },
}


# ============================================================================
# Helper Functions
# ============================================================================

def get_vendors() -> List[Dict[str, str]]:
    """Get list of available chip vendors"""
    return [{"value": v.value, "label": v.name.title()} for v in ChipVendor]


def get_series(vendor: str) -> List[Dict[str, str]]:
    """Get chip series for a vendor"""
    try:
        chip_vendor = ChipVendor(vendor)
        series_dict = CHIP_DATABASE.get(chip_vendor, {})
        return [
            {"value": s.name, "label": s.label}
            for s in series_dict.values()
        ]
    except ValueError:
        return []


def get_models(vendor: str, series: str) -> List[Dict[str, any]]:
    """Get chip models for a vendor and series"""
    try:
        chip_vendor = ChipVendor(vendor)
        series_dict = CHIP_DATABASE.get(chip_vendor, {})
        chip_series = series_dict.get(series)
        if not chip_series:
            return []

        return [
            {
                "value": m.name,
                "label": m.label,
                "clock_freq_mhz": m.clock_freq_mhz,
                "flash_size_kb": m.flash_size_kb,
                "ram_size_kb": m.ram_size_kb,
                "module_count": len(m.mcal_modules),
            }
            for m in chip_series.models
        ]
    except ValueError:
        return []


def get_mcal_modules(vendor: str, series: str, model: str) -> List[Dict[str, any]]:
    """Get MCAL modules for a specific chip model"""
    try:
        chip_vendor = ChipVendor(vendor)
        series_dict = CHIP_DATABASE.get(chip_vendor, {})
        chip_series = series_dict.get(series)
        if not chip_series:
            return []

        chip_model = next((m for m in chip_series.models if m.name == model), None)
        if not chip_model:
            return []

        return [
            {
                "name": mod.name,
                "label": mod.label,
                "required": mod.required,
                "default_params": mod.default_params,
                "dependencies": mod.dependencies,
            }
            for mod in chip_model.mcal_modules
        ]
    except ValueError:
        return []


def get_chip_info(vendor: str, series: str, model: str) -> Optional[Dict[str, any]]:
    """Get complete chip information"""
    try:
        chip_vendor = ChipVendor(vendor)
        series_dict = CHIP_DATABASE.get(chip_vendor, {})
        chip_series = series_dict.get(series)
        if not chip_series:
            return None

        chip_model = next((m for m in chip_series.models if m.name == model), None)
        if not chip_model:
            return None

        return {
            "name": chip_model.name,
            "label": chip_model.label,
            "vendor": chip_model.vendor.value,
            "series": chip_model.series,
            "clock_freq_mhz": chip_model.clock_freq_mhz,
            "flash_size_kb": chip_model.flash_size_kb,
            "ram_size_kb": chip_model.ram_size_kb,
            "mcal_modules": [
                {
                    "name": mod.name,
                    "label": mod.label,
                    "required": mod.required,
                }
                for mod in chip_model.mcal_modules
            ],
            "notes": chip_model.notes,
        }
    except ValueError:
        return None


if __name__ == "__main__":
    import json

    # Test: List all vendors
    print("=== Vendors ===")
    print(json.dumps(get_vendors(), indent=2))

    # Test: Get series for Infineon
    print("\n=== Infineon Series ===")
    print(json.dumps(get_series("infineon"), indent=2))

    # Test: Get models for TC3xx
    print("\n=== TC3xx Models ===")
    print(json.dumps(get_models("infineon", "tc3xx"), indent=2))

    # Test: Get MCAL modules for TC375
    print("\n=== TC375 MCAL Modules ===")
    print(json.dumps(get_mcal_modules("infineon", "tc3xx", "tc375"), indent=2))

    # Test: Get chip info
    print("\n=== TC375 Chip Info ===")
    print(json.dumps(get_chip_info("infineon", "tc3xx", "tc375"), indent=2))
