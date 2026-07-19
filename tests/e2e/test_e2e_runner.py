"""
E2E Test Runner for C/C++ E2E test binaries.
Runs compiled C E2E tests and collects results.
"""

import subprocess
import sys
import os
from pathlib import Path


def run_c_e2e_test(binary_path):
    """Run a C E2E test binary and return (name, passed, output)."""
    name = Path(binary_path).stem
    try:
        result = subprocess.run(
            [binary_path],
            capture_output=True,
            text=True,
            timeout=30,
        )
        output = result.stdout + result.stderr
        passed = result.returncode == 0
        return name, passed, output
    except subprocess.TimeoutExpired:
        return name, False, "TIMEOUT"
    except FileNotFoundError:
        return name, False, "Binary not found"
    except Exception as e:
        return name, False, str(e)


def compile_test(source_path, output_dir):
    """Compile a C E2E test source file."""
    name = Path(source_path).stem
    binary = output_dir / name
    result = subprocess.run(
        ["gcc", "-Wall", "-Wextra", "-o", str(binary), str(source_path)],
        capture_output=True,
        text=True,
        timeout=30,
    )
    if result.returncode != 0:
        return None, result.stderr
    return str(binary), None


def test_e2e_all():
    """Discover and run all C E2E tests."""
    this_dir = Path(__file__).parent
    output_dir = this_dir / "build"
    output_dir.mkdir(exist_ok=True)

    c_sources = sorted(this_dir.glob("test_e2e_*.c"))
    assert len(c_sources) > 0, "No C E2E test sources found"

    compiled = 0
    run_count = 0
    passed_count = 0
    failures = []

    for src in c_sources:
        binary, error = compile_test(src, output_dir)
        if binary is None:
            failures.append((src.stem, "COMPILE FAILED", error or ""))
            continue
        compiled += 1

        name, passed, output = run_c_e2e_test(binary)
        run_count += 1
        if passed:
            passed_count += 1
        else:
            failures.append((name, "FAILED", output))

    # Print results
    print(f"\n{'='*50}")
    print(f"E2E Test Results: {passed_count}/{run_count} passed")
    if failures:
        print(f"\nFailures:")
        for name, status, detail in failures:
            print(f"  ❌ {name}: {status}")
    print(f"{'='*50}\n")

    assert passed_count == run_count, f"{run_count - passed_count} E2E test(s) failed"
    assert compiled > 0, "No E2E tests compiled"

    return True
