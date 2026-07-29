"""ECUAL module configuration schemas - Part 1 (CanIf, CanTp, EthIf, IoHwAb)."""
from module_registry import register_module


def register_ecual_part1():
    register_module("CanIf", "ECUAL", {
        "CanIfMaxRxPduCount": {"type": "int", "default": 16, "description": "Max RX PDUs"},
        "CanIfMaxTxPduCount": {"type": "int", "default": 16, "description": "Max TX PDUs"},
        "CanIfVersionInfoApi": {"type": "bool", "default": False, "description": "Version info API"},
        "CanIfPublicCanConfigSet": {"type": "int", "default": 0, "description": "CAN config set"},
        "CanIfDevErrorDetect": {"type": "bool", "default": True, "description": "DET support"},
    })
    register_module("CanTp", "ECUAL", {
        "CanTpMaxSduCount": {"type": "int", "default": 4, "description": "Max SDU count"},
        "CanTpChannelCount": {"type": "int", "default": 1, "description": "TP channels"},
        "CanTpRxBufferSize": {"type": "int", "default": 256, "description": "RX buffer (bytes)"},
        "CanTpTxBufferSize": {"type": "int", "default": 256, "description": "TX buffer (bytes)"},
        "CanTpVersionInfoApi": {"type": "bool", "default": False, "description": "Version info"},
    })
    register_module("EthIf", "ECUAL", {
        "EthIfControllerCount": {"type": "int", "default": 1, "description": "ETH controllers"},
        "EthIfRxBufferSize": {"type": "int", "default": 2048, "description": "RX buffer size"},
        "EthIfTxBufferSize": {"type": "int", "default": 2048, "description": "TX buffer size"},
        "EthIfVersionInfoApi": {"type": "bool", "default": False, "description": "Version info"},
        "EthIfDevErrorDetect": {"type": "bool", "default": True, "description": "DET support"},
    })
    register_module("IoHwAb", "ECUAL", {
        "IoHwAbChannelCount": {"type": "int", "default": 8, "description": "I/O channels"},
        "IoHwAbPortPinCount": {"type": "int", "default": 16, "description": "Port pin mappings"},
        "IoHwAbVersionInfoApi": {"type": "bool", "default": False, "description": "Version info"},
        "IoHwAbDevErrorDetect": {"type": "bool", "default": True, "description": "DET support"},
    })
