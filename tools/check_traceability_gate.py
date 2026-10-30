#!/usr/bin/env python3
"""Traceability evidence gate (WP4).

A requirement→implementation trace is only trustworthy when:
  * code_files point at real C source (.c/.h under src/), never Python scripts,
    Markdown, or test files;
  * has_code ⇔ code_files is non-empty (no empty-flag drift);
  * has_test ⇔ matched_tests/test_reports is non-empty;
  * no single source file承接 (>承接上限) requirements — that signals a
    synthetic fallback mapping rather than a real implementation link.

Reviews (has_review) are process artifacts we cannot fabricate; the gate
reports the gap but does not fail on it unless --require-review is passed.

Usage:
    python3 tools/check_traceability_gate.py [report.json]
"""
import json
import os
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_REPORT = ROOT / ".yuleosh" / "evidence-bundle" / "traceability" / "traceability-report.json"
MAX_REQS_PER_FILE = 20  # a single .c承接 >20 SHALLs is almost certainly a fallback map


def _is_valid_code_file(f: str) -> bool:
    if not f:
        return False
    if not (f.endswith(".c") or f.endswith(".h")):
        return False
    # implementation lives under src/; test/infra files are not "code"
    return f.startswith("src/") and "tests/" not in f and "test_" not in os.path.basename(f)


def validate(report, require_review=False):
    reqs = report["lrm"]["requirements"]
    fails = []
    file_req_count = Counter()

    for r in reqs:
        rid = r.get("req_id") or r.get("id") or "?"
        code_files = r.get("code_files") or []
        has_code = bool(r.get("has_code"))
        has_tests = bool(r.get("matched_tests") or r.get("test_reports"))
        has_test_flag = bool(r.get("has_test"))

        # 1) code_files must be real C sources
        bad = [f for f in code_files if not _is_valid_code_file(f)]
        if bad:
            fails.append(f"{rid}: code_files contains non-implementation file(s): {bad}")

        # 2) has_code ⇔ code_files present
        if has_code and not code_files:
            fails.append(f"{rid}: has_code=True but code_files is empty")
        if code_files and not has_code:
            fails.append(f"{rid}: code_files present but has_code=False")

        # 3) has_test ⇔ tests present
        if has_test_flag and not has_tests:
            fails.append(f"{rid}: has_test=True but no matched_tests/test_reports")
        if has_tests and not has_test_flag:
            fails.append(f"{rid}: tests present but has_test=False")

        for f in code_files:
            if _is_valid_code_file(f):
                file_req_count[f] += 1

    # 4) single-file承接上限
    overload = [(f, n) for f, n in file_req_count.items() if n > MAX_REQS_PER_FILE]
    for f, n in sorted(overload, key=lambda x: -x[1]):
        fails.append(f"file承接过载: {f} maps to {n} requirements (> {MAX_REQS_PER_FILE})")

    # 5) review gap (informational unless --require-review)
    review_gap = sum(1 for r in reqs if not r.get("has_review") and not r.get("reviews"))
    if require_review and review_gap:
        fails.append(f"{review_gap} requirement(s) have no review evidence")

    summary = {
        "total": len(reqs),
        "invalid_mappings": len([f for f in fails if "code_files contains" in f]),
        "flag_inconsistencies": len([f for f in fails if "has_code" in f or "has_test" in f]),
        "overloaded_files": len(overload),
        "review_gap": review_gap,
    }
    return not fails, fails, summary


def main():
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_REPORT
    require_review = "--require-review" in sys.argv
    if not path.is_file():
        print(f"FATAL: traceability report not found: {path}")
        sys.exit(2)

    report = json.loads(path.read_text())
    ok, fails, summary = validate(report, require_review)

    print(f"Traceability Gate — {path.relative_to(ROOT) if path.is_relative_to(ROOT) else path}")
    print(f"  requirements:       {summary['total']}")
    print(f"  invalid mappings:   {summary['invalid_mappings']}")
    print(f"  flag inconsistencies:{summary['flag_inconsistencies']}")
    print(f"  overloaded files:   {summary['overloaded_files']}")
    print(f"  review gap:         {summary['review_gap']} (non-blocking)")
    for f in fails:
        print(f"  ❌ {f}")
    print(f"  {'✅ Gate PASSED' if ok else '❌ Gate FAILED'}")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
