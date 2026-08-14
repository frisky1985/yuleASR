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
        assert "det" in modules
        assert "pdur" in modules
        assert "com" in modules
        assert "dcm" in modules

    def test_get_mcal_module_list(self):
        """MCAL module list should contain hardware abstraction modules."""
        from yuleasr_monitor import get_mcal_module_list
        modules = get_mcal_module_list()
        assert isinstance(modules, list)
        assert len(modules) > 10
        # Check for representative modules
        assert "adc" in modules
        assert "can" in modules
        assert "dio" in modules
        assert "spi" in modules

    def test_get_ecual_module_list(self):
        """ECUAL module list should contain ECU abstraction modules."""
        from yuleasr_monitor import get_ecual_module_list
        modules = get_ecual_module_list()
        assert isinstance(modules, list)
        assert len(modules) > 10
        # Check for representative modules
        assert "ethif" in modules
        assert "canif" in modules
        assert "fee" in modules
        assert "iohwab" in modules

    def test_get_bsw_module_list_contains_expected(self):
        """Verify BSW list has key AUTOSAR service modules."""
        from yuleasr_monitor import get_bsw_module_list
        modules = get_bsw_module_list()
        essential = {"det", "dcm", "dem", "com", "pdur", "nvm", "ecum", "wdgm", "schm"}
        assert essential.issubset(set(modules)), f"Missing: {essential - set(modules)}"

    def test_get_mcal_module_list_contains_expected(self):
        """Verify MCAL list has key hardware driver modules."""
        from yuleasr_monitor import get_mcal_module_list
        modules = get_mcal_module_list()
        essential = {"adc", "can", "dio", "eth", "spi", "gpt", "pwm"}
        assert essential.issubset(set(modules)), f"Missing: {essential - set(modules)}"

    def test_lists_have_no_duplicates(self):
        """Each module list should have unique entries."""
        from yuleasr_monitor import get_bsw_module_list, get_mcal_module_list, get_ecual_module_list
        for name, fn in [("bsw", get_bsw_module_list), ("mcal", get_mcal_module_list), ("ecual", get_ecual_module_list)]:
            modules = fn()
            assert len(modules) == len(set(modules)), f"{name} list has duplicates"
