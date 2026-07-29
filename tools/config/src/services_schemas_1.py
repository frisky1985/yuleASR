"""Services module schemas - Part 1 (Com, PduR, NvM, Dcm)."""
from module_registry import register_module


def register_services_part1():
    register_module("Com", "Services", {
        "ComMaxPduCount": {"type": "int", "default": 32, "description": "Max IPDUs"},
        "ComMaxSignalCount": {"type": "int", "default": 128, "description": "Max signals"},
        "ComVersionInfoApi": {"type": "bool", "default": False, "description": "Version info"},
        "ComDevErrorDetect": {"type": "bool", "default": True, "description": "DET support"},
        "ComTimeBasedTxMode": {"type": "bool", "default": False, "description": "Time-based TX"},
    })
    register_module("PduR", "Services", {
        "PduRMaxRoutingTableCount": {"type": "int", "default": 32, "description": "Routing entries"},
        "PduRVersionInfoApi": {"type": "bool", "default": False, "description": "Version info"},
        "PduRDevErrorDetect": {"type": "bool", "default": True, "description": "DET support"},
    })
    register_module("NvM", "Services", {
        "NvMBlockCount": {"type": "int", "default": 32, "description": "NVRAM blocks"},
        "NvMRomBlockCount": {"type": "int", "default": 32, "description": "ROM blocks"},
        "NvMRamBlockCount": {"type": "int", "default": 32, "description": "RAM blocks"},
        "NvMVersionInfoApi": {"type": "bool", "default": False, "description": "Version info"},
        "NvMDevErrorDetect": {"type": "bool", "default": True, "description": "DET support"},
    })
    register_module("Dcm", "Services", {
        "DcmMaxPduCount": {"type": "int", "default": 8, "description": "Max DCM PDUs"},
        "DcmMaxSidCount": {"type": "int", "default": 32, "description": "Max SID entries"},
        "DcmVersionInfoApi": {"type": "bool", "default": False, "description": "Version info"},
        "DcmDevErrorDetect": {"type": "bool", "default": True, "description": "DET support"},
        "DcmDspUdsOnCan": {"type": "bool", "default": True, "description": "UDS on CAN"},
    })
