#!/usr/bin/env python3
"""Generate a human-fillable review checklist from the traceability report.

Reads .yuleosh/evidence-bundle/traceability/traceability-report.json and emits
review-checklist.md listing every requirement with columns for the human
reviewer to fill in (reviewer, date, decision, findings).

The checklist is a working document: humans fill it in, then run
mark_requirement_reviewed.py to record approvals back into the JSON report.
Reviews are never auto-approved — pending status is the honest default.

Usage:
    python3 tools/generate_review_checklist.py
"""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REPORT = ROOT / ".yuleosh" / "evidence-bundle" / "traceability" / "traceability-report.json"
OUT = ROOT / ".yuleosh" / "evidence-bundle" / "traceability" / "review-checklist.md"

MAX_STMT = 80  # truncate long statements in the table


def _truncate(s: str, n: int = MAX_STMT) -> str:
    s = (s or "").replace("\n", " ").strip()
    return s if len(s) <= n else s[: n - 1] + "…"


def _status_badge(rev):
    if not rev:
        return "⭕ pending"
    st = rev.get("status")
    return {
        "pending": "⭕ pending",
        "approved": "✅ approved",
        "rejected": "❌ rejected",
        "changes_required": "🔄 changes_required",
    }.get(st, f"❓ {st}")


def main():
    if not REPORT.is_file():
        print(f"FATAL: report not found: {REPORT}", file=sys.stderr)
        sys.exit(2)
    report = json.loads(REPORT.read_text())
    reqs = report["lrm"]["requirements"]

    lines = []
    lines.append("# Requirement Review Checklist\n")
    lines.append(f"> Auto-generated from `traceability-report.json` ({len(reqs)} requirements).\n")
    lines.append("> **Honesty rule**: reviews are human process artifacts. A requirement is\n")
    lines.append("> counted as reviewed ONLY when a human runs `mark_requirement_reviewed.py`\n")
    lines.append("> with `--decision approve` and a real reviewer + date. Pending rows are\n")
    lines.append("> gaps — do not mark them approved without an actual review.\n")
    lines.append("\n## How to use\n")
    lines.append("1. Pick a requirement row below.\n")
    lines.append("2. Read the statement and its `code_files` / `matched_tests` in the report.\n")
    lines.append("3. Perform the review (code inspection, test check, etc.).\n")
    lines.append("4. Record the decision with the CLI tool:\n")
    lines.append("   ```\n")
    lines.append("   python3 tools/mark_requirement_reviewed.py <REQ_ID> \\\n")
    lines.append("       --reviewer \"<name>\" --date YYYY-MM-DD \\\n")
    lines.append("       --decision approve [--findings \"...\"]\n")
    lines.append("   ```\n")
    lines.append("5. Re-run this generator to refresh the checklist badges.\n")
    lines.append("\n## Checklist\n")
    lines.append("| # | Req ID | Section | Statement | Status | Reviewer | Date | Decision | Findings |")
    lines.append("|--:|:-------|:--------|:----------|:-------|:---------|:-----|:---------|:---------|")

    pending = approved = other = 0
    for i, r in enumerate(reqs, 1):
        rid = r.get("req_id") or r.get("id") or "?"
        section = r.get("section", "")
        stmt = _truncate(r.get("statement", ""))
        revs = r.get("reviews") or []
        latest = revs[-1] if revs else None
        badge = _status_badge(latest)
        reviewer = (latest or {}).get("reviewer") or ""
        date = (latest or {}).get("date") or ""
        decision = (latest or {}).get("decision") or ""
        findings = (latest or {}).get("findings") or []
        findings_str = "; ".join(str(f) for f in findings) if findings else ""
        findings_str = _truncate(findings_str, 40)

        if latest and latest.get("status") == "approved":
            approved += 1
        elif latest and latest.get("status") == "pending":
            pending += 1
        else:
            other += 1

        lines.append(
            f"| {i} | `{rid}` | {section} | {stmt} | {badge} | {reviewer} | {date} | {decision} | {findings_str} |"
        )

    lines.append("")
    lines.append("## Summary\n")
    lines.append(f"- Total requirements: **{len(reqs)}**\n")
    lines.append(f"- ✅ Approved: **{approved}**\n")
    lines.append(f"- ⭕ Pending: **{pending}**\n")
    if other:
        lines.append(f"- ❌/🔄 Rejected / changes-required: **{other}**\n")
    lines.append(f"- Review progress: **{approved}/{len(reqs)}** "
                 f"({100.0 * approved / len(reqs):.1f}%)\n")

    OUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"✅ Wrote {OUT.relative_to(ROOT)}")
    print(f"   total={len(reqs)} approved={approved} pending={pending} other={other}")


if __name__ == "__main__":
    main()
