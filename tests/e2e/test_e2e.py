"""
E2E Test Runner - pytest entry point.
Compiles and runs C-based E2E tests from test_e2e_*.c files.
"""

import subprocess
import pytest
from pathlib import Path


E2E_DIR = Path(__file__).parent
# Project root: traverse up from tests/e2e/ to project root (two levels)
PROJECT_DIR = E2E_DIR.parent.parent  # tests/e2e/ -> tests/ -> ./
SOURCES_DIR = PROJECT_DIR / "src"

# Include paths — core + key module include dirs for AUTOSAR header resolution
INCLUDE_DIRS = [
    str(PROJECT_DIR / "include"),
    str(PROJECT_DIR / "include" / "autosar"),
    str(PROJECT_DIR / "src"),
    str(PROJECT_DIR / "src" / "bsw" / "os" / "include"),
    str(PROJECT_DIR / "src" / "bsw" / "services" / "det" / "include"),
    str(PROJECT_DIR / "src" / "bsw" / "services" / "crc" / "include"),
    str(PROJECT_DIR / "src" / "bsw" / "services" / "com" / "include"),
    str(PROJECT_DIR / "src" / "bsw" / "services" / "dcm" / "include"),
    str(PROJECT_DIR / "src" / "bsw" / "services" / "dem" / "include"),
    str(PROJECT_DIR / "src" / "rte" / "include"),
    str(PROJECT_DIR / "src" / "bsw" / "mcal" / "lin" / "include"),
]

# Source files needed by real AUTOSAR C API tests (for linking)
REAL_TEST_SOURCES = {
    "test_e2e_det_real": [
        str(SOURCES_DIR / "bsw" / "services" / "det" / "src" / "Det.c"),
    ],
    "test_e2e_crc_real": [
        str(SOURCES_DIR / "bsw" / "services" / "crc" / "src" / "Crc.c"),
        str(SOURCES_DIR / "bsw" / "services" / "crc" / "src" / "Crc_Lcfg.c"),
    ],
}


def _compile_test(source_path, output_path):
    """Compile a single C E2E test with project include paths and link deps."""
    cmd = ["gcc", "-Wall", "-Wextra", "--coverage", "-o", str(output_path), str(source_path)]
    # Add module source files for real API tests that need linking
    stem = source_path.stem
    if stem in REAL_TEST_SOURCES:
        for src_file in REAL_TEST_SOURCES[stem]:
            if Path(src_file).exists():
                cmd.append(src_file)
    for inc in INCLUDE_DIRS:
        cmd.extend(["-I", inc])
    # Force-include Compiler.h for NULL_PTR on native builds
    compiler_h = str(PROJECT_DIR / "include" / "autosar" / "Compiler.h")
    if Path(compiler_h).exists():
        cmd.extend(["-include", compiler_h])
    result = subprocess.run(
        cmd,
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
