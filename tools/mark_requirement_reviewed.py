#!/usr/bin/env python3
"""Record a human review decision for a single requirement.

Reviews are process artifacts — this tool is the ONLY sanctioned way to mark a
requirement as reviewed. It never fabricates approvals: the caller must supply
a real reviewer name and date, and an explicit decision.

For an "approve" decision, --reviewer and --date are mandatory and the
requirement's has_review flag is set to True. For "request_changes" or
"reject", the record is stored but has_review stays False (the gap remains).

Usage:
    python3 tools/mark_requirement_reviewed.py <REQ_ID> \\
        --reviewer "<name>" --date YYYY-MM-DD \\
        [--decision approve|request_changes|reject] \\
        [--findings "finding 1"] [--findings "finding 2"]

Examples:
    # Approve a requirement
    python3 tools/mark_requirement_reviewed.py COM-SHALL-001 \\
        --reviewer "张三" --date 2026-08-25 --decision approve

    # Request changes with findings
    python3 tools/mark_requirement_reviewed.py DCM-SHALL-002 \\
        --reviewer "李四" --date 2026-08-25 --decision request_changes \\
        --findings "Dcm_StartProtocol missing timeout handling"
"""
import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import check_traceability_gate as tg  # noqa: E402

REPORT = ROOT / ".yuleosh" / "evidence-bundle" / "traceability" / "traceability-report.json"
_DATE_RE = re.compile(r"^\d{4}-\d{2}-\d{2}$")
_DECISIONS = {"approve", "request_changes", "reject"}
_STATUS_FOR_DECISION = {
    "approve": "approved",
    "request_changes": "changes_required",
    "reject": "rejected",
}


def _find_req(reqs, rid):
    for r in reqs:
        if (r.get("req_id") or r.get("id")) == rid:
            return r
    return None


def main():
    ap = argparse.ArgumentParser(description="Record a review decision for a requirement.")
    ap.add_argument("req_id", help="Requirement ID (e.g. COM-SHALL-001)")
    ap.add_argument("--reviewer", required=True, help="Reviewer name or email")
    ap.add_argument("--date", required=True, help="Review date (YYYY-MM-DD)")
    ap.add_argument("--decision", choices=sorted(_DECISIONS), default="approve",
                    help="Review decision (default: approve)")
    ap.add_argument("--findings", action="append", default=[],
                    help="Review finding (repeatable)")
    ap.add_argument("--report", default=str(REPORT),
                    help=f"Path to traceability report (default: {REPORT.relative_to(ROOT)})")
    args = ap.parse_args()

    report_path = Path(args.report)
    if not _DATE_RE.match(args.date):
        print(f"FATAL: --date must be YYYY-MM-DD, got {args.date!r}", file=sys.stderr)
        sys.exit(2)
    if not report_path.is_file():
        print(f"FATAL: report not found: {report_path}", file=sys.stderr)
        sys.exit(2)

    report = json.loads(report_path.read_text())
    reqs = report["lrm"]["requirements"]
    req = _find_req(reqs, args.req_id)
    if req is None:
        print(f"FATAL: requirement {args.req_id} not found in report", file=sys.stderr)
        sys.exit(2)

    status = _STATUS_FOR_DECISION[args.decision]
    record = {
        "status": status,
        "reviewer": args.reviewer,
        "date": args.date,
        "findings": list(args.findings),
        "decision": args.decision,
    }

    # Validate the record against the gate's schema before writing
    bad = tg._validate_review_record(args.req_id, record)
    if bad:
        for b in bad:
            print(f"FATAL: {b}", file=sys.stderr)
        sys.exit(2)

    reviews = req.get("reviews")
    if not isinstance(reviews, list):
        reviews = []
        req["reviews"] = reviews

    # Replace a pending placeholder in-place; otherwise append a new round.
    replaced = False
    for i, rev in enumerate(reviews):
        if isinstance(rev, dict) and rev.get("status") == "pending":
            reviews[i] = record
            replaced = True
            break
    if not replaced:
        reviews.append(record)

    # has_review is derived honestly from approved reviews only
    req["has_review"] = tg.has_approved_review(req)

    # Refresh summary
    summ = report["lrm"].get("summary", {})
    summ["with_review"] = sum(1 for r in reqs if tg.has_approved_review(r))
    summ["without_review"] = len(reqs) - summ["with_review"]
    report["lrm"]["summary"] = summ

    report_path.write_text(json.dumps(report, indent=2, ensure_ascii=False))
    print(f"✅ Recorded review for {args.req_id}")
    print(f"   status:   {status}")
    print(f"   reviewer: {args.reviewer}")
    print(f"   date:     {args.date}")
    print(f"   decision: {args.decision}")
    if args.findings:
        print(f"   findings: {len(args.findings)}")
    print(f"   has_review (req): {req['has_review']}")
    print(f"   overall reviewed: {summ['with_review']}/{len(reqs)}")


if __name__ == "__main__":
    main()
