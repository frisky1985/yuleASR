"""
Tests for yuleasr_monitor.py — ensures Python coverage data collection.
"""
import sys
import os

# Add src to path so we can import the module
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "scripts", "analysis"))


class TestYuleasrMonitor:
    """Unit tests for the yuleasr_monitor module."""

    def test_get_bsw_module_list(self):
        """BSW module list should contain key AUTOSAR modules."""
        from yuleasr_monitor import get_bsw_module_list
        modules = get_bsw_module_list()
        assert isinstance(modules, list)
        assert len(modules) > 10
        # Check for representative modules
        assert "Det" in modules
        assert "PduR" in modules
        assert "Com" in modules
        assert "Dcm" in modules

    def test_get_mcal_module_list(self):
        """MCAL module list should contain hardware abstraction modules."""
        from yuleasr_monitor import get_mcal_module_list
        modules = get_mcal_module_list()
        assert isinstance(modules, list)
        assert len(modules) > 10
        # Check for representative modules
        assert "Adc" in modules
        assert "Can" in modules
        assert "Dio" in modules
        assert "Spi" in modules

    def test_get_ecual_module_list(self):
        """ECUAL module list should contain ECU abstraction modules."""
        from yuleasr_monitor import get_ecual_module_list
        modules = get_ecual_module_list()
        assert isinstance(modules, list)
        assert len(modules) > 10
        # Check for representative modules
        assert "EthIf" in modules
        assert "CanIf" in modules
        assert "Fee" in modules
        assert "IoHwAb" in modules

    def test_get_bsw_module_list_contains_expected(self):
        """Verify BSW list has key AUTOSAR service modules."""
        from yuleasr_monitor import get_bsw_module_list
        modules = get_bsw_module_list()
        essential = {"Det", "Dcm", "Dem", "Com", "PduR", "NvM", "EcuM", "WdgM", "SchM"}
        assert essential.issubset(set(modules)), f"Missing: {essential - set(modules)}"

    def test_get_mcal_module_list_contains_expected(self):
        """Verify MCAL list has key hardware driver modules."""
        from yuleasr_monitor import get_mcal_module_list
        modules = get_mcal_module_list()
        essential = {"Adc", "Can", "Dio", "Eth", "Spi", "gpt", "Pwm"}
        assert essential.issubset(set(modules)), f"Missing: {essential - set(modules)}"

    def test_lists_have_no_duplicates(self):
        """Each module list should have unique entries."""
        from yuleasr_monitor import get_bsw_module_list, get_mcal_module_list, get_ecual_module_list
        for name, fn in [("bsw", get_bsw_module_list), ("mcal", get_mcal_module_list), ("ecual", get_ecual_module_list)]:
            modules = fn()
            assert len(modules) == len(set(modules)), f"{name} list has duplicates"
