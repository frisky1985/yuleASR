#!/usr/bin/env python3
"""Repair traceability code-file mappings (WP4).

Fixes synthetic/fallback mappings (e.g. Python test files listed as
implementation) and flag drift in traceability-report.json.

Strategy:
  * For each requirement, derive the module prefix from its req_id.
  * If the requirement has NO valid .c/.h code_files (empty or contains
    non-implementation files), fill code_files from the verified
    REQ_TO_SOURCE table (only files that actually exist on disk).
  * Recompute has_code ⇔ code_files non-empty and has_test ⇔ tests present.
  * Existing valid mappings are left untouched.

Usage: python3 tools/repair_traceability_mapping.py
"""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import generate_traceability_mapping as gtm  # noqa: E402
import check_traceability_gate as tg  # noqa: E402

REPORT = ROOT / ".yuleosh" / "evidence-bundle" / "traceability" / "traceability-report.json"


def _valid(f: str) -> bool:
    return bool(f) and (f.endswith(".c") or f.endswith(".h")) and f.startswith("src/")


def main():
    report = json.loads(REPORT.read_text())
    reqs = report["lrm"]["requirements"]

    fixed_code = 0
    fixed_flag = 0
    fixed_reviews = 0
    for r in reqs:
        rid = r.get("req_id") or r.get("id")
        prefix = rid.split("-")[0] if "-" in rid else rid
        current = r.get("code_files") or []
        current_valid = [f for f in current if _valid(f)]

        if not current_valid:
            # fall back to verified module mapping
            mapped = gtm.get_requirement_source_files(prefix)
            mapped = [f for f in mapped if _valid(f)]
            if mapped != current:
                r["code_files"] = mapped
                fixed_code += 1

        r["has_code"] = len(r.get("code_files") or []) > 0
        has_tests = bool(r.get("matched_tests") or r.get("test_reports"))
        if r.get("has_test") != has_tests:
            r["has_test"] = has_tests
            fixed_flag += 1

        # Reviews: ensure every requirement carries at least one review record
        # so the gap is visible & trackable. Empty reviews → pending placeholder.
        # has_review is derived from approved reviews only (no fabrication).
        reviews = r.get("reviews")
        if not isinstance(reviews, list):
            reviews = []
            r["reviews"] = reviews
        if not reviews:
            reviews.append(tg.pending_review_record())
            fixed_reviews += 1
        # recompute has_review honestly from approved reviews
        reviewed = tg.has_approved_review(r)
        if bool(r.get("has_review")) != reviewed:
            r["has_review"] = reviewed
            fixed_flag += 1

    # refresh summary counts from actual data
    summ = report["lrm"].get("summary", {})
    summ["with_code"] = sum(1 for r in reqs if r.get("has_code"))
    summ["without_code"] = sum(1 for r in reqs if not r.get("has_code"))
    summ["with_test"] = sum(1 for r in reqs if r.get("has_test"))
    summ["without_test"] = sum(1 for r in reqs if not r.get("has_test"))
    summ["with_review"] = sum(1 for r in reqs if tg.has_approved_review(r))
    summ["without_review"] = sum(1 for r in reqs if not tg.has_approved_review(r))
    report["lrm"]["summary"] = summ

    REPORT.write_text(json.dumps(report, indent=2, ensure_ascii=False))
    print(f"✅ Repaired {REPORT.relative_to(ROOT)}")
    print(f"   code_files fixed: {fixed_code}")
    print(f"   has_* flags fixed: {fixed_flag}")
    print(f"   pending reviews initialized: {fixed_reviews}")
    print(f"   with_code={summ['with_code']}/{len(reqs)}  "
          f"with_test={summ['with_test']}/{len(reqs)}  "
          f"with_review={summ['with_review']}/{len(reqs)}")


if __name__ == "__main__":
    main()
