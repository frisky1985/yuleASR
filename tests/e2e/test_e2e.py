"""
E2E Test Runner - pytest entry point.
Compiles and runs C-based E2E tests from test_e2e_*.c files.
"""

import subprocess
import pytest
from pathlib import Path


E2E_DIR = Path(__file__).parent


def _compile_test(source_path, output_path):
    """Compile a single C E2E test."""
    result = subprocess.run(
        ["gcc", "-Wall", "-Wextra", "-o", str(output_path), str(source_path)],
        capture_output=True, text=True, timeout=30,
    )
    if result.returncode != 0:
        pytest.fail(f"Compile failed for {source_path.name}:\n{result.stderr}")
    return output_path


def _run_test(binary_path):
    """Run a compiled E2E test binary."""
    result = subprocess.run(
        [str(binary_path)], capture_output=True, text=True, timeout=30,
    )
    output = result.stdout + result.stderr
    if result.returncode != 0:
        pytest.fail(f"E2E test {binary_path.stem} failed:\n{output}")
    return output


# Dynamically generate pytest test functions for each .c file
def _make_test(source_path):
    """Create a pytest test function for a C E2E test."""
    test_name = source_path.stem.replace("test_e2e_", "test_")

    def test_func():
        build_dir = E2E_DIR / ".build"
        build_dir.mkdir(exist_ok=True)
        binary = build_dir / source_path.stem
        _compile_test(source_path, binary)
        output = _run_test(binary)
        print(output)

    test_func.__name__ = test_name
    return test_func


# Discover C E2E test sources
_c_sources = sorted(E2E_DIR.glob("test_e2e_*.c"))
for _src in _c_sources:
    _tname = _src.stem.replace("test_e2e_", "test_")
    globals()[_tname] = _make_test(_src)


def test_e2e_sanity():
    """Verify that E2E test sources were discovered."""
    assert len(_c_sources) > 0, "No E2E test sources found"
    print(f"Discovered {len(_c_sources)} E2E test sources: {[s.name for s in _c_sources]}")
