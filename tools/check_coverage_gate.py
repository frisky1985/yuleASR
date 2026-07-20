#!/usr/bin/env python3
"""
Coverage Gate Check (P0-4)

Fixes the bug where line_rate=0.0 bypasses the threshold check.
Properly handles gcovr JSON format (gcovr/format_version 0.14+).
"""

import json
import os
import re
import sys

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
COVERAGE_JSON = os.path.join(BASE_DIR, ".yuleosh", "reports", "c-coverage.json")
CI_CONFIG = os.path.join(BASE_DIR, ".yuleosh", "ci-config.yaml")


def compute_totals(report):
    """Compute totals from gcovr format_version 0.14+ JSON."""
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


def check_coverage_gate(report_path=COVERAGE_JSON, thresholds=None):
    if thresholds is None:
        thresholds = {"line": 60.0, "branch": 60.0}

    with open(report_path, "r") as f:
        report = json.load(f)

    totals = compute_totals(report)
    total_files = len(report.get("files", []))

    line_found = totals["lines"]["found"]
    line_hit = totals["lines"]["hit"]
    branch_found = totals["branches"]["found"]
    branch_hit = totals["branches"]["hit"]

    line_rate = line_hit / max(line_found, 1) if line_found > 0 else 0.0
    branch_rate = branch_hit / max(branch_found, 1) if branch_found > 0 else 0.0

    print(f"Coverage Gate Check")
    print(f"  Files: {total_files}")
    print(f"  Lines: {line_hit}/{line_found} ({line_rate:.1%})")
    print(f"  Branches: {branch_hit}/{branch_found} ({branch_rate:.1%})")
    print(f"  Threshold (line): {thresholds['line']}%")
    print(f"  Threshold (branch): {thresholds['branch']}%")

    # FIX: Properly handle 0.0 case
    # line_rate=0.0 when there ARE files but NO coverage -> FAIL
    # line_rate=0.0 when there are NO files -> FAIL
    if line_found == 0:
        print(f"  ❌ Gate FAILED: 0 lines measured — coverage collection is broken")
        return False

    line_rate_pct = line_rate * 100
    branch_rate_pct = branch_rate * 100

    passed = True
    if line_rate_pct < thresholds["line"]:
        print(f"  ❌ Line rate {line_rate_pct:.1f}% < threshold {thresholds['line']}%")
        passed = False
    else:
        print(f"  ✅ Line rate {line_rate_pct:.1f}% >= threshold {thresholds['line']}%")

    if branch_rate_pct < thresholds["branch"]:
        print(f"  ❌ Branch rate {branch_rate_pct:.1f}% < threshold {thresholds['branch']}%")
        passed = False
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

    thresholds = {"line": 60.0, "branch": 60.0}
    if os.path.isfile(CI_CONFIG):
        with open(CI_CONFIG) as f:
            text = f.read()
        m = re.search(r"threshold_line:\s*([\d.]+)", text)
        if m:
            thresholds["line"] = float(m.group(1))
        m = re.search(r"threshold_branch:\s*([\d.]+)", text)
        if m:
            thresholds["branch"] = float(m.group(1))

    result = check_coverage_gate(check_json, thresholds)
    sys.exit(0 if result else 1)
