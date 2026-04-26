#!/usr/bin/env python3
"""
Test runner for DDS Configuration Tool.

Usage:
    python run_tests.py              # Run all tests
    python run_tests.py diagnostic   # Run diagnostic tests only
    python run_tests.py e2e          # Run E2E tests only
    python run_tests.py ota          # Run OTA tests only
    python run_tests.py codegen      # Run code generator tests only
"""

import sys
import unittest
from pathlib import Path

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent))


def run_tests(pattern=None):
    """Run tests with optional pattern filter."""
    loader = unittest.TestLoader()
    
    if pattern:
        # Run specific test module
        test_modules = {
            'diagnostic': 'tests.test_diagnostic_config',
            'e2e': 'tests.test_e2e_config',
            'ota': 'tests.test_ota_config',
            'codegen': 'tests.test_code_generator'
        }
        
        if pattern in test_modules:
            suite = loader.loadTestsFromName(test_modules[pattern])
        else:
            print(f"Unknown test pattern: {pattern}")
            print(f"Available patterns: {', '.join(test_modules.keys())}")
            return 1
    else:
        # Run all tests
        suite = loader.discover('tests', pattern='test_*.py')
    
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)
    
    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__":
    pattern = sys.argv[1] if len(sys.argv) > 1 else None
    sys.exit(run_tests(pattern))
