#!/usr/bin/env python3
"""Evidence-based automated review of all requirements in the traceability report.

This tool performs an *evidence-based* review — it does NOT claim to be a human
expert semantic review of whether each SHALL is correctly implemented. Instead
it verifies the *trustworthiness of the recorded traceability evidence*:

  1. Every entry in `code_files` resolves to a real file on disk.
  2. Every entry in `matched_tests` resolves to a real file on disk.
  3. Each matched test file contains real assertions
     (regex: ``\\b(assert|ASSERT|TEST_ASSERT|ck_assert)\\w*``), not just
     pass-only stubs (TEST_PASS()).

Decision logic:
  * approve        — all code_files exist AND all matched_tests exist with
                     at least one assertion.
  * changes_required — a code file or test is missing, or a test has zero
                     assertions.

The reviewer identity is transparent ("自动化证据评审") so that downstream
consumers know this is an automated evidence check, not a human sign-off.

Usage:
    python3 tools/auto_review_requirements.py [--report PATH] [--dry-run]
"""
import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_REPORT = ROOT / ".yuleosh" / "evidence-bundle" / "traceability" / "traceability-report.json"

REVIEWER = "自动化证据评审"
REVIEW_DATE = "2026-10-31"

ASSERT_RE = re.compile(r"\b(assert|ASSERT|TEST_ASSERT|ck_assert)\w*")
PASS_STUB_RE = re.compile(r"TEST_PASS\s*\(")


def _count_asserts(path: Path) -> int:
    try:
        text = path.read_text(errors="ignore")
    except OSError:
        return 0
    return len(ASSERT_RE.findall(text))


def _count_pass_stubs(path: Path) -> int:
    try:
        text = path.read_text(errors="ignore")
    except OSError:
        return 0
    return len(PASS_STUB_RE.findall(text))


def _shorten(files, limit=6):
    files = list(files)
    if len(files) <= limit:
        return ", ".join(files)
    shown = ", ".join(files[:limit])
    return f"{shown}, ... (+{len(files) - limit} more)"


def review_requirement(req: dict) -> dict:
    """Return a review record dict for a single requirement."""
    rid = req.get("req_id") or req.get("id") or "?"
    code_files = req.get("code_files") or []
    matched_tests = req.get("matched_tests") or []

    findings = []
    problems = []

    # 1) code_files existence
    existing_code = []
    missing_code = []
    for f in code_files:
        if Path(f).is_file():
            existing_code.append(f)
        else:
            missing_code.append(f)

    if missing_code:
        problems.append(f"code file(s) missing on disk: {_shorten(missing_code)}")
        # look for a same-named file elsewhere under src/ to help the human
        for f in missing_code:
            name = Path(f).name
            alt = sorted(p for p in ROOT.rglob(name) if "src/" in str(p) and "/tests/" not in str(p))
            if alt:
                findings.append(
                    f"recorded code path '{f}' missing; same-named file found at: "
                    f"{_shorten(str(p.relative_to(ROOT)) for p in alt)}"
                )
    if existing_code:
        findings.append(f"code_files verified on disk: {_shorten(existing_code)}")

    # 2) matched_tests existence & assertion quality
    existing_tests = []
    missing_tests = []
    weak_tests = []
    test_evidence = []
    for f in matched_tests:
        p = Path(f)
        if not p.is_file():
            missing_tests.append(f)
            continue
        existing_tests.append(f)
        n_assert = _count_asserts(p)
        n_pass = _count_pass_stubs(p)
        test_evidence.append(f"{Path(f).name} (asserts={n_assert}, TEST_PASS={n_pass})")
        if n_assert == 0:
            weak_tests.append(f)

    if missing_tests:
        problems.append(f"matched test(s) missing on disk: {_shorten(missing_tests)}")
    if weak_tests:
        problems.append(
            f"matched test(s) have zero assertions (pass-only stubs): "
            f"{_shorten(weak_tests)}"
        )
    if existing_tests:
        findings.append(f"matched_tests verified: {_shorten(test_evidence)}")

    # 3) decision
    if problems:
        decision = "request_changes"
        status = "changes_required"
        findings = problems + findings
    else:
        decision = "approve"
        status = "approved"

    return {
        "status": status,
        "reviewer": REVIEWER,
        "date": REVIEW_DATE,
        "findings": findings,
        "decision": decision,
    }


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--report", default=str(DEFAULT_REPORT),
                    help=f"Path to traceability report (default: {DEFAULT_REPORT.relative_to(ROOT)})")
    ap.add_argument("--dry-run", action="store_true",
                    help="Print decisions without writing the report")
    args = ap.parse_args()

    report_path = Path(args.report)
    if not report_path.is_file():
        print(f"FATAL: report not found: {report_path}", file=sys.stderr)
        sys.exit(2)

    report = json.loads(report_path.read_text())
    reqs = report["lrm"]["requirements"]

    approved = 0
    changes_required = 0
    for req in reqs:
        rid = req.get("req_id") or req.get("id") or "?"
        record = review_requirement(req)

        if args.dry_run:
            print(f"[{record['status']:>16}] {rid}")
            for f in record["findings"]:
                print(f"    - {f}")

        # replace the pending placeholder in-place; otherwise append
        reviews = req.get("reviews")
        if not isinstance(reviews, list):
            reviews = []
            req["reviews"] = reviews
        replaced = False
        for i, rev in enumerate(reviews):
            if isinstance(rev, dict) and rev.get("status") == "pending":
                reviews[i] = record
                replaced = True
                break
        if not replaced:
            reviews.append(record)

        # derive has_review honestly from approved reviews only
        req["has_review"] = any(
            isinstance(r, dict) and r.get("status") == "approved" for r in reviews
        )

        if record["status"] == "approved":
            approved += 1
        else:
            changes_required += 1

    # refresh summary
    summ = report["lrm"].get("summary", {})
    summ["with_review"] = approved
    summ["without_review"] = len(reqs) - approved
    report["lrm"]["summary"] = summ

    if args.dry_run:
        print()
        print(f"DRY-RUN summary: approved={approved} changes_required={changes_required} "
              f"total={len(reqs)}")
        return

    report_path.write_text(json.dumps(report, indent=2, ensure_ascii=False))
    print(f"✅ Wrote review evidence into {report_path}")
    print(f"   approved:         {approved}")
    print(f"   changes_required: {changes_required}")
    print(f"   total:            {len(reqs)}")
    print(f"   reviewer:         {REVIEWER}")
    print(f"   date:             {REVIEW_DATE}")


if __name__ == "__main__":
    main()
