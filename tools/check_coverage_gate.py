#!/usr/bin/env python3
"""
Coverage Gate Check (P0-4) — Updated for yuleOSH CI

Supports both:
  - gcovr JSON format (format_version 0.14+)  [from ci.yml gcovr pipeline]
  - lcov-based JSON format (lines.found/hit)  [from batch10_coverage.sh / local runs]

Uses .yuleosh/ci-config.yaml for thresholds, with these defaults:
  Line:      20.0 (Phase 2 minimum target)
  Branch:    10.0 (Phase 2 minimum target)
  Hard gate: 35.0 (c_fail_under — blocks CI pipeline)

Usage:
    python3 tools/check_coverage_gate.py [path/to/coverage.json]
"""

import json
import os
import re
import sys

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
COVERAGE_JSON = os.path.join(BASE_DIR, ".yuleosh", "reports", "c-coverage.json")
CI_CONFIG = os.path.join(BASE_DIR, ".yuleosh", "ci-config.yaml")


def compute_totals_gcovr(report):
    """
    Compute totals from gcovr format_version 0.14+ JSON.

    gcovr JSON schema:
      {
        "files": [
          {
            "file": "src/bsw/mcal/dio/Dio.c",
            "lines": [{"count": N, "branches": [{"count": N}, ...]}, ...],
            "functions": [{"execution_count": N, ...}, ...]
          }
        ]
      }
    """
    files = report.get("files", [])
    total_lines_found = 0
    total_lines_hit = 0
    total_branches_found = 0
    total_branches_hit = 0
    total_functions_found = 0
    total_functions_hit = 0

    for f in files:
        lines = f.get("lines", [])
        functions = f.get("functions", [])

        for line_data in lines:
            count = line_data.get("count", 0)
            total_lines_found += 1
            if count > 0:
                total_lines_hit += 1

            for branch in line_data.get("branches", []):
                total_branches_found += 1
                if branch.get("count", 0) > 0:
                    total_branches_hit += 1

        for func_data in functions:
            total_functions_found += 1
            if func_data.get("execution_count", 0) > 0:
                total_functions_hit += 1

    return {
        "lines": {"found": total_lines_found, "hit": total_lines_hit},
        "functions": {"found": total_functions_found, "hit": total_functions_hit},
        "branches": {"found": total_branches_found, "hit": total_branches_hit},
    }


def compute_totals_lcov(report):
    """
    Compute totals from lcov-based JSON (generate_c_coverage_json.py format).

    lcov-based JSON schema:
      {
        "files": [
          {
            "file": "src/bsw/mcal/dio/Dio.c",
            "lines": {"found": N, "hit": N},
            "functions": {"found": N, "hit": N},
            "branches": {"found": N, "hit": N}
          }
        ],
        "totals": {
          "lines": {"found": N, "hit": N},
          ...
        }
      }
    """
    files = report.get("files", [])
    totals_inline = report.get("totals", None)

    if totals_inline:
        # Use pre-computed totals if available (lcov-based format)
        return {
            "lines": totals_inline.get("lines", {"found": 0, "hit": 0}),
            "functions": totals_inline.get("functions", {"found": 0, "hit": 0}),
            "branches": totals_inline.get("branches", {"found": 0, "hit": 0}),
        }

    # Fallback: compute from per-file data
    total_lines_found = 0
    total_lines_hit = 0
    total_branches_found = 0
    total_branches_hit = 0
    total_functions_found = 0
    total_functions_hit = 0

    for f in files:
        lines = f.get("lines", {})
        if isinstance(lines, dict) and "found" in lines:
            total_lines_found += lines.get("found", 0)
            total_lines_hit += lines.get("hit", 0)
            functions = f.get("functions", {})
            if isinstance(functions, dict):
                total_functions_found += functions.get("found", 0)
                total_functions_hit += functions.get("hit", 0)
            branches = f.get("branches", {})
            if isinstance(branches, dict):
                total_branches_found += branches.get("found", 0)
                total_branches_hit += branches.get("hit", 0)

    return {
        "lines": {"found": total_lines_found, "hit": total_lines_hit},
        "functions": {"found": total_functions_found, "hit": total_functions_hit},
        "branches": {"found": total_branches_found, "hit": total_branches_hit},
    }


def detect_format(report):
    """
    Auto-detect whether the JSON file is gcovr or lcov format.
    Returns 'gcovr' or 'lcov'.
    """
    if "files" not in report:
        return "unknown"

    if "totals" in report:
        # lcov-based format has a 'totals' key at top level
        return "lcov"

    # Check first file's lines structure
    files = report.get("files", [])
    if not files:
        return "lcov"  # empty files, try lcov

    first_file = files[0]
    lines = first_file.get("lines", [])

    if isinstance(lines, list):
        return "gcovr"
    elif isinstance(lines, dict) and "found" in lines:
        return "lcov"

    # Fallback: check for gcovr-specific keys
    if "format_version" in report or "gcovr_version" in report:
        return "gcovr"

    return "lcov"  # default to lcov


def check_coverage_gate(report_path=COVERAGE_JSON, thresholds=None):
    if thresholds is None:
        # Phase 2 defaults from ci-config.yaml
        thresholds = {"line": 20.0, "branch": 10.0}

    with open(report_path, "r") as f:
        report = json.load(f)

    # Auto-detect format
    fmt = detect_format(report)

    if fmt == "gcovr":
        totals = compute_totals_gcovr(report)
    elif fmt == "lcov":
        totals = compute_totals_lcov(report)
    else:
        print(f"  ⚠️  Unknown format — trying gcovr parser")
        totals = compute_totals_gcovr(report)

    total_files = len(report.get("files", []))

    line_found = totals["lines"]["found"]
    line_hit = totals["lines"]["hit"]
    branch_found = totals["branches"]["found"]
    branch_hit = totals["branches"]["hit"]
    func_found = totals["functions"]["found"]
    func_hit = totals["functions"]["hit"]

    line_rate = line_hit / max(line_found, 1) if line_found > 0 else 0.0
    branch_rate = branch_hit / max(branch_found, 1) if branch_found > 0 else 0.0
    func_rate = func_hit / max(func_found, 1) if func_found > 0 else 0.0

    print(f"Coverage Gate Check")
    print(f"  Format:   {fmt.upper()}")
    print(f"  Files:    {total_files}")
    print(f"  Lines:    {line_hit}/{line_found} ({line_rate:.1%})")
    print(f"  Functions:{func_hit}/{func_found} ({func_rate:.1%})")
    print(f"  Branches: {branch_hit}/{branch_found} ({branch_rate:.1%})")
    print(f"  Thresholds:")
    print(f"    Line:   {thresholds['line']}%")
    print(f"    Branch: {thresholds['branch']}%")

    if line_found == 0:
        print(f"  ❌ Gate FAILED: 0 lines measured — coverage collection is broken")
        print(f"     Check that test executables link against production libraries")
        print(f"     and that --coverage flags are applied during compilation.")
        return False

    # Filter to only src/ files for the source code check
    src_files = [f for f in report.get("files", [])
                 if f.get("file", "").startswith("src/")]
    if src_files:
        print(f"  Source files (src/): {len(src_files)}")
    else:
        print(f"  ⚠️  No src/ files in report — only test/tool files?")
        print(f"     Check that gcovr --filter src/.* is correct.")

    line_rate_pct = line_rate * 100
    branch_rate_pct = branch_rate * 100

    passed = True
    if line_rate_pct < thresholds["line"]:
        print(f"  ❌ Line rate {line_rate_pct:.1f}% < threshold {thresholds['line']}%")
        passed = False
    else:
        print(f"  ✅ Line rate {line_rate_pct:.1f}% >= threshold {thresholds['line']}%")

    if branch_rate_pct < thresholds["branch"]:
        print(f"  ⚠️  Branch rate {branch_rate_pct:.1f}% < threshold {thresholds['branch']}%")
        # Branch may be non-blocking for Phase 2
        print(f"     (Non-blocking in Phase 2 — will become blocking in Phase 3)")
    else:
        print(f"  ✅ Branch rate {branch_rate_pct:.1f}% >= threshold {thresholds['branch']}%")

    if passed:
        print(f"  ✅ Gate PASSED")
    else:
        print(f"  ❌ Gate FAILED")

    return passed


if __name__ == "__main__":
    check_json = COVERAGE_JSON
    if len(sys.argv) > 1:
        check_json = sys.argv[1]

    # Defaults: Phase 2 line=20%, branch=10%
    # Overridden by ci-config.yaml if present
    # Hard gate (c_fail_under) is enforced by ci.yml inline, not here
    thresholds = {"line": 20.0, "branch": 10.0}
    if os.path.isfile(CI_CONFIG):
        with open(CI_CONFIG) as f:
            text = f.read()
        # Read threshold_line (Phase 2 minimum target)
        m = re.search(r"threshold_line:\s*([\d.]+)", text)
        if m:
            thresholds["line"] = float(m.group(1))
        # Read threshold_branch (Phase 2 minimum target)
        m = re.search(r"threshold_branch:\s*([\d.]+)", text)
        if m:
            thresholds["branch"] = float(m.group(1))

    result = check_coverage_gate(check_json, thresholds)
    sys.exit(0 if result else 1)
