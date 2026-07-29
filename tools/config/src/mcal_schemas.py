"""MCAL module configuration schemas - Part 1."""
from module_registry import register_module


def register_mcal_modules():
    # 8 MCAL modules
    register_module("Port", "MCAL", {
        "PortPinMode": {"type": "int", "default": 0, "description": "Port pin mode"},
        "PortPinDirection": {"type": "int", "default": 0, "description": "Port pin direction"},
        "PortPinLevelValue": {"type": "int", "default": 0, "description": "Port pin level"},
        "PortPinOutputType": {"type": "int", "default": 0, "description": "Output type"},
        "PortVersionInfoApi": {"type": "bool", "default": False, "description": "Version info API"},
    })
    register_module("Dio", "MCAL", {
        "DioChannelGroupCount": {"type": "int", "default": 1, "description": "Channel groups"},
        "DioVersionInfoApi": {"type": "bool", "default": False, "description": "Version info"},
        "DioFlipChannelApi": {"type": "bool", "default": False, "description": "Flip channel API"},
        "DioMaskedWritePortApi": {"type": "bool", "default": False, "description": "Masked write"},
    })
    register_module("Can", "MCAL", {
        "CanControllerCount": {"type": "int", "default": 2, "description": "Number of CAN controllers"},
        "CanBaudrate": {"type": "int", "default": 500000, "description": "Baudrate in bps"},
        "CanVersionInfoApi": {"type": "bool", "default": False, "description": "Version info"},
        "CanSetBaudrateApi": {"type": "bool", "default": False, "description": "Set baudrate API"},
        "CanMainFunctionPeriod": {"type": "float", "default": 0.01, "description": "Main function period"},
    })
    register_module("Lin", "MCAL", {
        "LinChannelCount": {"type": "int", "default": 1, "description": "Number of LIN channels"},
        "LinBaudrate": {"type": "int", "default": 19200, "description": "LIN baudrate"},
        "LinVersionInfoApi": {"type": "bool", "default": False, "description": "Version info API"},
        "LinWakeupSupport": {"type": "bool", "default": True, "description": "Wakeup support"},
    })
    register_module("Spi", "MCAL", {
        "SpiChannelCount": {"type": "int", "default": 1, "description": "SPI channels"},
        "SpiJobCount": {"type": "int", "default": 1, "description": "SPI jobs"},
        "SpiSequenceCount": {"type": "int", "default": 1, "description": "SPI sequences"},
        "SpiVersionInfoApi": {"type": "bool", "default": False, "description": "Version info"},
    })
    register_module("Gpt", "MCAL", {
        "GptChannelCount": {"type": "int", "default": 4, "description": "GPT channels"},
        "GptPrescaler": {"type": "int", "default": 1, "description": "Timer prescaler"},
        "GptVersionInfoApi": {"type": "bool", "default": False, "description": "Version info API"},
        "GptWakeupFunctionality": {"type": "bool", "default": False, "description": "Wakeup support"},
    })
    register_module("Mcu", "MCAL", {
        "McuClockSettingCount": {"type": "int", "default": 1, "description": "Clock settings"},
        "McuRamSectors": {"type": "int", "default": 1, "description": "RAM sectors"},
        "McuDefaultSpeed": {"type": "int", "default": 16000000, "description": "Default speed (Hz)"},
        "McuVersionInfoApi": {"type": "bool", "default": False, "description": "Version info"},
    })
    register_module("Adc", "MCAL", {
        "AdcChannelCount": {"type": "int", "default": 4, "description": "ADC channels"},
        "AdcResolution": {"type": "int", "default": 12, "description": "Resolution (bits)"},
        "AdcGroupCount": {"type": "int", "default": 1, "description": "Groups"},
        "AdcVersionInfoApi": {"type": "bool", "default": False, "description": "Version info"},
    })

    # Additional MCAL modules to reach 30 total
    register_module("Icu", "MCAL", {
        "IcuChannelCount": {"type": "int", "default": 4, "description": "ICU input capture channels"},
        "IcuVersionInfoApi": {"type": "bool", "default": False, "description": "Version info API"},
        "IcuSignalMeasurement": {"type": "bool", "default": False, "description": "Signal measurement"},
    })
    register_module("Pwm", "MCAL", {
        "PwmChannelCount": {"type": "int", "default": 4, "description": "PWM channels"},
        "PwmPeriod": {"type": "int", "default": 1000, "description": "PWM period (us)"},
        "PwmVersionInfoApi": {"type": "bool", "default": False, "description": "Version info API"},
    })
    register_module("Fls", "MCAL", {
        "FlsSectorCount": {"type": "int", "default": 8, "description": "Flash sectors"},
        "FlsPageSize": {"type": "int", "default": 256, "description": "Flash page size"},
        "FlsVersionInfoApi": {"type": "bool", "default": False, "description": "Version info API"},
    })
    register_module("Crc", "MCAL", {
        "CrcComputationWidth": {"type": "int", "default": 32, "description": "CRC width (8/16/32)"},
        "CrcVersionInfoApi": {"type": "bool", "default": False, "description": "Version info API"},
        "CrcHwUnitCount": {"type": "int", "default": 1, "description": "Hardware CRC units"},
    })
