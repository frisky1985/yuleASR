#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
yuleASR Evidence Pipeline Integration Test — Phase 3.1

Scenario: Load known spec.md + traceability-report.json → run generate_evidence →
check all three evidence output files report the same coverage count.

Asserts that traceability-matrix.md, requirement-coverage.md, and
acceptance-matrix.md agree on the number of covered requirements.
"""

import os
import re
import sys
import json
import subprocess
import tempfile

BASE_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
TRACE_REPORT = os.path.join(BASE_DIR, ".yuleosh", "reports", "traceability-report.json")
EVIDENCE_DIR = os.path.join(BASE_DIR, ".osh", "evidence")
GENERATE_SCRIPT = os.path.join(BASE_DIR, "tools", "generate_evidence.py")


def _load_json(path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def _resolve_matched_tests(req):
    """Mirror tools/generate_evidence.py fallback chain so the expected
    counts match the generator's single source of truth (P1-3 fix)."""
    matched_tests = req.get("matched_tests", [])
    if not matched_tests:
        test_reports = req.get("test_reports", [])
        passed_reports = [
            tr for tr in test_reports
            if isinstance(tr, dict)
            and tr.get("status") == "passed"
            and int(tr.get("passed", 0) or 0) > 0
        ]
        if passed_reports:
            matched_tests = passed_reports
    if not matched_tests:
        if req.get("has_test", False):
            matched_tests = ["(tested)"]
    return matched_tests


def _count_covered(text, pattern):
    """Count matches of a coverage pattern in text."""
    return len(re.findall(pattern, text))


def test_evidence_pipeline_prerequisites():
    """Pre-flight check: all required input files exist."""
    assert os.path.isfile(TRACE_REPORT), f"Missing {TRACE_REPORT}"
    assert os.path.isfile(GENERATE_SCRIPT), f"Missing {GENERATE_SCRIPT}"


def test_evidence_pipeline_execution():
    """Run generate_evidence.py and verify it exits 0."""
    result = subprocess.run(
        [sys.executable, GENERATE_SCRIPT],
        cwd=BASE_DIR,
        capture_output=True,
        text=True,
    )
    print(result.stdout)
    if result.returncode != 0:
        print(result.stderr, file=sys.stderr)
    assert result.returncode == 0, f"generate_evidence.py exited {result.returncode}"


def test_evidence_output_files_exist():
    """All three evidence output files must exist after generation."""
    for fname in ["traceability-matrix.md", "requirement-coverage.md", "acceptance-matrix.md"]:
        path = os.path.join(EVIDENCE_DIR, fname)
        assert os.path.isfile(path), f"Missing evidence output: {path}"


def test_evidence_acceptance_matrix_covered_count():
    """
    Parse acceptance-matrix.md and verify the 'Covered by tests' line
    matches the summary count in traceability-matrix.md.
    """
    # Load the source traceability report for the authoritative count
    trace = _load_json(TRACE_REPORT)
    reqs = trace["lrm"]["requirements"]
    expected_covered = sum(1 for r in reqs if len(_resolve_matched_tests(r)) > 0)
    total = len(reqs)

    # Read acceptance-matrix.md
    am_path = os.path.join(EVIDENCE_DIR, "acceptance-matrix.md")
    with open(am_path, "r", encoding="utf-8") as f:
        am_text = f.read()

    am_match = re.search(r"Covered by tests:\s*(\d+)", am_text)
    assert am_match, "Could not find 'Covered by tests' in acceptance-matrix.md"
    am_covered = int(am_match.group(1))

    assert am_covered == expected_covered, (
        f"acceptance-matrix.md reports {am_covered} covered, "
        f"but traceability-report.json has {expected_covered}"
    )

    print(f"  ✅ acceptance-matrix.md: {am_covered}/{total} covered")


def test_evidence_requirement_coverage_count():
    """
    Parse requirement-coverage.md 'Requirement Coverage' line and verify
    it matches the acceptance-matrix coverage count.
    """
    rc_path = os.path.join(EVIDENCE_DIR, "requirement-coverage.md")
    with open(rc_path, "r", encoding="utf-8") as f:
        rc_text = f.read()

    rc_match = re.search(r"\*\*Requirement Coverage\*\*:\s*(\d+)/(\d+)", rc_text)
    assert rc_match, "Could not find 'Requirement Coverage' in requirement-coverage.md"
    rc_covered = int(rc_match.group(1))
    rc_total = int(rc_match.group(2))

    # Read acceptance-matrix.md for comparison
    am_path = os.path.join(EVIDENCE_DIR, "acceptance-matrix.md")
    with open(am_path, "r", encoding="utf-8") as f:
        am_text = f.read()
    am_match = re.search(r"Covered by tests:\s*(\d+)", am_text)
    am_covered = int(am_match.group(1)) if am_match else -1

    assert rc_covered == am_covered, (
        f"requirement-coverage.md reports {rc_covered} covered, "
        f"but acceptance-matrix.md reports {am_covered}"
    )
    print(f"  ✅ requirement-coverage.md: {rc_covered}/{rc_total} (matches acceptance-matrix)")


def test_evidence_traceability_consistency():
    """
    Parse traceability-matrix.md and verify its covered/uncovered counts
    match the other two evidence files.
    """
    tm_path = os.path.join(EVIDENCE_DIR, "traceability-matrix.md")
    with open(tm_path, "r", encoding="utf-8") as f:
        tm_text = f.read()

    tm_covered = _count_covered(tm_text, r"Status: ✅ Covered")
    tm_uncovered = _count_covered(tm_text, r"Status: ❌ Not Covered")

    # Verify summary line also matches
    summary_match = re.search(r"Requirements with test coverage:\s*(\d+)", tm_text)
    assert summary_match, "Could not find summary coverage in traceability-matrix.md"
    tm_summary = int(summary_match.group(1))

    assert tm_covered == tm_summary, (
        f"traceability-matrix.md summary says {tm_summary} covered, "
        f"but entry count says {tm_covered}"
    )

    # Cross-check with acceptance-matrix
    am_path = os.path.join(EVIDENCE_DIR, "acceptance-matrix.md")
    with open(am_path, "r", encoding="utf-8") as f:
        am_text = f.read()
    am_match = re.search(r"Covered by tests:\s*(\d+)", am_text)
    am_covered = int(am_match.group(1)) if am_match else -1

    assert tm_covered == am_covered, (
        f"traceability-matrix.md shows {tm_covered} covered, "
        f"but acceptance-matrix.md shows {am_covered}"
    )
    print(f"  ✅ traceability-matrix.md: {tm_covered} covered, {tm_uncovered} uncovered")
    print(f"  ✅ All three evidence files agree: {am_covered} covered")


def test_evidence_pipeline_idempotent():
    """
    Run the pipeline twice and verify SHA256s match (idempotency).
    """
    # First run (already done by test_evidence_pipeline_execution)
    # Get baseline hashes
    baseline = {}
    for fname in ["traceability-matrix.md", "requirement-coverage.md", "acceptance-matrix.md"]:
        path = os.path.join(EVIDENCE_DIR, fname)
        with open(path, "rb") as f:
            import hashlib
            baseline[fname] = hashlib.sha256(f.read()).hexdigest()

    # Second run
    result = subprocess.run(
        [sys.executable, GENERATE_SCRIPT],
        cwd=BASE_DIR,
        capture_output=True,
        text=True,
    )
    assert result.returncode == 0, f"Second generate_evidence.py exited {result.returncode}"

    for fname in ["traceability-matrix.md", "requirement-coverage.md", "acceptance-matrix.md"]:
        path = os.path.join(EVIDENCE_DIR, fname)
        with open(path, "rb") as f:
            import hashlib
            sha = hashlib.sha256(f.read()).hexdigest()
        assert sha == baseline[fname], (
            f"{fname} changed after re-running generate_evidence.py (not idempotent)"
        )


if __name__ == "__main__":
    # Manual run outside pytest
    tests = [
        ("Prerequisites", test_evidence_pipeline_prerequisites),
        ("Execution", test_evidence_pipeline_execution),
        ("Output exist", test_evidence_output_files_exist),
        ("Acceptance matrix count", test_evidence_acceptance_matrix_covered_count),
        ("Requirement coverage count", test_evidence_requirement_coverage_count),
        ("Traceability consistency", test_evidence_traceability_consistency),
        ("Idempotency", test_evidence_pipeline_idempotent),
    ]
    failures = 0
    for name, fn in tests:
        try:
            fn()
            print(f"[PASS] {name}")
        except Exception as e:
            print(f"[FAIL] {name}: {e}")
            failures += 1
    sys.exit(failures)
