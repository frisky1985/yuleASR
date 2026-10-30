"""Coverage evidence trust tests (WP2).

Verifies that check_coverage_gate.validate_evidence rejects untrustworthy
coverage reports (missing provenance, stale commit, absolute paths, empty
data) before any threshold is checked.
"""
import os
import sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(REPO, "tools"))
import check_coverage_gate as cg  # noqa: E402


def _report(**kw):
    base = {
        "total_files": 1,
        "totals": {"lines": {"found": 10, "hit": 5},
                   "functions": {"found": 1, "hit": 1},
                   "branches": {"found": 2, "hit": 1}},
        "files": [{"file": "src/x.c", "lines": {"found": 10, "hit": 5}}],
    }
    base.update(kw)
    return base


def test_provenance_required():
    ok, fails = cg.validate_evidence(_report(), "x")
    assert not ok and any("provenance" in f for f in fails), fails


def test_stale_commit_blocked():
    ok, fails = cg.validate_evidence(
        _report(provenance={"git_sha": "deadbeef", "tool": {"gcovr": "8.6"}}), "x")
    assert not ok and any("stale" in f for f in fails), fails


def test_absolute_path_blocked():
    ok, fails = cg.validate_evidence(
        _report(provenance={"git_sha": __import__("subprocess").check_output(
            ["git", "rev-parse", "HEAD"], cwd=REPO, text=True).strip(),
            "tool": {"gcovr": "8.6"}},
            files=[{"file": "/Users/elsewhere/src/x.c"}]), "x")
    assert not ok and any("absolute" in f for f in fails), fails


def test_empty_report_blocked():
    ok, fails = cg.validate_evidence(_report(total_files=0, files=[]), "x")
    assert not ok and any("total_files=0" in f for f in fails), fails


def test_clean_report_passes_evidence():
    import subprocess
    head = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=REPO, text=True).strip()
    ok, fails = cg.validate_evidence(
        _report(provenance={"git_sha": head, "tool": {"gcovr": "8.6"}}), "x")
    assert ok, fails
    print(f"  ✅ evidence validation passes on a clean, HEAD-bound report")
