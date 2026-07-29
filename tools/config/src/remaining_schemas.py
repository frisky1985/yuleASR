"""Remaining module configuration schemas - WdgIf, Dem, EcuM, BswM, WdgM."""
from module_registry import register_module


def register_remaining_modules():
    # -- ECUAL --
    register_module("WdgIf", "ECUAL", {
        "WdgIfMaxConfiguredTriggers": {"type": "int", "default": 1, "description": "Max watchdog trigger devices"},
        "WdgIfVersionInfoApi": {"type": "bool", "default": False, "description": "Version info API"},
        "WdgIfDevErrorDetect": {"type": "bool", "default": True, "description": "DET support"},
    })

    # -- Services --
    register_module("Dem", "Services", {
        "DemMaxEventCount": {"type": "int", "default": 64, "description": "Max DTC events"},
        "DemMaxDebounceCount": {"type": "int", "default": 16, "description": "Max debounce counters"},
        "DemOperationCycleCount": {"type": "int", "default": 4, "description": "Operation cycles"},
        "DemVersionInfoApi": {"type": "bool", "default": False, "description": "Version info API"},
        "DemDevErrorDetect": {"type": "bool", "default": True, "description": "DET support"},
    })
    register_module("EcuM", "Services", {
        "EcuMMaxWakeupSources": {"type": "int", "default": 4, "description": "Wakeup sources"},
        "EcuMShutdownTarget": {"type": "int", "default": 0, "description": "Shutdown target (0=Off, 1=Reset, 2=Sleep)"},
        "EcuMVersionInfoApi": {"type": "bool", "default": False, "description": "Version info API"},
        "EcuMDevErrorDetect": {"type": "bool", "default": True, "description": "DET support"},
    })
    register_module("BswM", "Services", {
        "BswMMaxModeCount": {"type": "int", "default": 8, "description": "Max BSW modes"},
        "BswMMaxLogicExpressionCount": {"type": "int", "default": 16, "description": "Logic expressions"},
        "BswMVersionInfoApi": {"type": "bool", "default": False, "description": "Version info API"},
        "BswMDevErrorDetect": {"type": "bool", "default": True, "description": "DET support"},
    })
    register_module("WdgM", "Services", {
        "WdgMMaxSupervisedEntities": {"type": "int", "default": 4, "description": "Supervised entities"},
        "WdgMMaxAliveSupervisionCycles": {"type": "int", "default": 8, "description": "Alive supervision cycles"},
        "WdgMMaxDeadlineSupervisionCycles": {"type": "int", "default": 8, "description": "Deadline supervision cycles"},
        "WdgMVersionInfoApi": {"type": "bool", "default": False, "description": "Version info API"},
        "WdgMDevErrorDetect": {"type": "bool", "default": True, "description": "DET support"},
    })
