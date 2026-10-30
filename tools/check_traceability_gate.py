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

Review record schema (one requirement may carry multiple review rounds):
    {
      "status":   "pending" | "approved" | "rejected" | "changes_required",
      "reviewer": "<name or email>" | null,      # required when approved
      "date":     "YYYY-MM-DD" | null,           # required when approved
      "findings": ["..."],                       # free-text list
      "decision": "approve" | "request_changes" | "reject" | null
    }

A requirement counts as reviewed (has_review == True) iff it has at least one
review with status == "approved". "pending" placeholders are honest gap
markers — they are NOT approvals and must not be counted.

Usage:
    python3 tools/check_traceability_gate.py [report.json] [--require-review]
"""
import json
import os
import re
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_REPORT = ROOT / ".yuleosh" / "evidence-bundle" / "traceability" / "traceability-report.json"
MAX_REQS_PER_FILE = 20  # a single .c承接 >20 SHALLs is almost certainly a fallback map

# Review status vocabulary
REVIEW_STATUS_APPROVED = "approved"
REVIEW_STATUSES = {"pending", "approved", "rejected", "changes_required"}
_DATE_RE = re.compile(r"^\d{4}-\d{2}-\d{2}$")


def pending_review_record() -> dict:
    """Return an honest 'review not yet performed' placeholder record."""
    return {
        "status": "pending",
        "reviewer": None,
        "date": None,
        "findings": [],
        "decision": None,
    }


def _is_approved_review(rev: dict) -> bool:
    return isinstance(rev, dict) and rev.get("status") == REVIEW_STATUS_APPROVED


def has_approved_review(req: dict) -> bool:
    """True iff the requirement has at least one approved review record."""
    reviews = req.get("reviews") or []
    return any(_is_approved_review(r) for r in reviews)


def _validate_review_record(rid: str, rev: dict) -> list:
    """Return a list of failure strings for one review record (empty if ok)."""
    fails = []
    if not isinstance(rev, dict):
        return [f"{rid}: review entry is not an object"]
    status = rev.get("status")
    if status not in REVIEW_STATUSES:
        fails.append(f"{rid}: review has invalid status {status!r} "
                     f"(expected one of {sorted(REVIEW_STATUSES)})")
    if status == REVIEW_STATUS_APPROVED:
        reviewer = rev.get("reviewer")
        date = rev.get("date")
        if not reviewer:
            fails.append(f"{rid}: approved review is missing reviewer")
        if not date or not _DATE_RE.match(str(date)):
            fails.append(f"{rid}: approved review is missing a valid date (YYYY-MM-DD)")
    return fails


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

    review_invalid = 0
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

        # 4) review records: validate schema & approved-review必填字段
        reviews = r.get("reviews") or []
        for rev in reviews:
            rev_fails = _validate_review_record(rid, rev)
            if rev_fails:
                fails.extend(rev_fails)
                review_invalid += 1

        # 5) has_review flag must agree with the actual approved-review set
        reviewed = has_approved_review(r)
        if bool(r.get("has_review")) != reviewed:
            fails.append(f"{rid}: has_review flag {r.get('has_review')} inconsistent "
                         f"with approved reviews (counted={reviewed})")

        for f in code_files:
            if _is_valid_code_file(f):
                file_req_count[f] += 1

    # 6) single-file承接上限
    overload = [(f, n) for f, n in file_req_count.items() if n > MAX_REQS_PER_FILE]
    for f, n in sorted(overload, key=lambda x: -x[1]):
        fails.append(f"file承接过载: {f} maps to {n} requirements (> {MAX_REQS_PER_FILE})")

    # 7) review gap: requirements WITHOUT any approved review (informational
    #    unless --require-review is passed). pending placeholders are honest
    #    gaps and must not be counted as reviews.
    review_gap = sum(1 for r in reqs if not has_approved_review(r))
    reviewed_count = len(reqs) - review_gap
    if require_review and review_gap:
        fails.append(f"{review_gap} requirement(s) have no approved review "
                     f"({reviewed_count} reviewed)")

    summary = {
        "total": len(reqs),
        "invalid_mappings": len([f for f in fails if "code_files contains" in f]),
        "flag_inconsistencies": len([f for f in fails if "has_code" in f or "has_test" in f or "has_review" in f]),
        "overloaded_files": len(overload),
        "review_invalid": review_invalid,
        "reviewed": reviewed_count,
        "review_gap": review_gap,
    }
    return not fails, fails, summary


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    flags = [a for a in sys.argv[1:] if a.startswith("--")]
    require_review = "--require-review" in flags
    path = Path(args[0]) if args else DEFAULT_REPORT
    if not path.is_file():
        print(f"FATAL: traceability report not found: {path}")
        sys.exit(2)

    report = json.loads(path.read_text())
    ok, fails, summary = validate(report, require_review)

    review_blocking = "BLOCKING (--require-review)" if require_review else "non-blocking"
    print(f"Traceability Gate — {path.relative_to(ROOT) if path.is_relative_to(ROOT) else path}")
    print(f"  requirements:       {summary['total']}")
    print(f"  invalid mappings:   {summary['invalid_mappings']}")
    print(f"  flag inconsistencies:{summary['flag_inconsistencies']}")
    print(f"  overloaded files:   {summary['overloaded_files']}")
    print(f"  reviewed:           {summary['reviewed']}/{summary['total']}")
    print(f"  review invalid:     {summary['review_invalid']}")
    print(f"  review gap:         {summary['review_gap']} ({review_blocking})")
    for f in fails:
        print(f"  ❌ {f}")
    print(f"  {'✅ Gate PASSED' if ok else '❌ Gate FAILED'}")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
