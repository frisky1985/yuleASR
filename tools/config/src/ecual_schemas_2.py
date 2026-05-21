"""ECUAL module configuration schemas - Part 2 (MemIf, Fee, Ea, FrIf, LinIf)."""
from module_registry import register_module


def register_ecual_part2():
    register_module("MemIf", "ECUAL", {
        "MemIfJobCount": {"type": "int", "default": 1, "description": "Memory interface jobs"},
        "MemIfVersionInfoApi": {"type": "bool", "default": False, "description": "Version info"},
        "MemIfDevErrorDetect": {"type": "bool", "default": True, "description": "DET support"},
    })
    register_module("Fee", "ECUAL", {
        "FeeBlockCount": {"type": "int", "default": 16, "description": "Flash EEPROM blocks"},
        "FeePageSize": {"type": "int", "default": 256, "description": "Page size (bytes)"},
        "FeeVersionInfoApi": {"type": "bool", "default": False, "description": "Version info"},
        "FeeDevErrorDetect": {"type": "bool", "default": True, "description": "DET support"},
    })
    register_module("Ea", "ECUAL", {
        "EaBlockCount": {"type": "int", "default": 8, "description": "EEPROM abstraction blocks"},
        "EaPageSize": {"type": "int", "default": 128, "description": "Page size (bytes)"},
        "EaVersionInfoApi": {"type": "bool", "default": False, "description": "Version info"},
        "EaDevErrorDetect": {"type": "bool", "default": True, "description": "DET support"},
    })
    register_module("FrIf", "ECUAL", {
        "FrIfChannelCount": {"type": "int", "default": 1, "description": "FlexRay channels"},
        "FrIfControllerCount": {"type": "int", "default": 1, "description": "FR controllers"},
        "FrIfVersionInfoApi": {"type": "bool", "default": False, "description": "Version info"},
        "FrIfDevErrorDetect": {"type": "bool", "default": True, "description": "DET support"},
    })
    register_module("LinIf", "ECUAL", {
        "LinIfChannelCount": {"type": "int", "default": 1, "description": "LIN interface channels"},
        "LinIfScheduleTableCount": {"type": "int", "default": 2, "description": "Schedule tables"},
        "LinIfVersionInfoApi": {"type": "bool", "default": False, "description": "Version info"},
        "LinIfDevErrorDetect": {"type": "bool", "default": True, "description": "DET support"},
    })
