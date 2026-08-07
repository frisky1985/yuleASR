#!/usr/bin/env python3
"""
yuleASR HIL report generator — aggregates pytest/JSON results into a summary.

Reads ``tests/hil/report-full.json`` (pytest-json-report) when present and
writes ``tests/hil/reports/summary.md`` + ``tests/hil/reports/summary.json``.
Used by .github/workflows/hil-tests.yml (hil-full-tests job).
"""

import json
import os
import sys
from pathlib import Path

HIL_DIR = Path(__file__).resolve().parent
REPORTS_DIR = HIL_DIR / "reports"


def load_json(path: Path) -> dict:
    try:
        with open(path, encoding="utf-8") as f:
            return json.load(f)
    except (OSError, json.JSONDecodeError):
        return {}


def main() -> int:
    REPORTS_DIR.mkdir(parents=True, exist_ok=True)

    full = load_json(HIL_DIR / "report-full.json")
    summary = {
        "source": "hil-test-results",
        "generated_at": full.get("created", ""),
        "environment": (full.get("environment") or {}).get("Python", ""),
    }

    if full.get("summary"):
        s = full["summary"]
        summary.update({
            "total": s.get("total", 0),
            "passed": s.get("passed", 0),
            "failed": s.get("failed", 0),
            "skipped": s.get("skipped", 0),
            "status": "passed" if s.get("failed", 1) == 0 else "failed",
        })
    else:
        # Fallback: scan for per-suite JSON files
        total = passed = failed = skipped = 0
        for jf in sorted(HIL_DIR.glob("report-*.json")):
            data = load_json(jf)
            s = data.get("summary", {})
            total += s.get("total", 0)
            passed += s.get("passed", 0)
            failed += s.get("failed", 0)
            skipped += s.get("skipped", 0)
        summary.update({
            "total": total, "passed": passed,
            "failed": failed, "skipped": skipped,
            "status": "passed" if failed == 0 else "failed",
        })

    with open(REPORTS_DIR / "summary.json", "w", encoding="utf-8") as f:
        json.dump(summary, f, indent=2, ensure_ascii=False)

    lines = [
        "# yuleASR HIL Test Summary",
        "",
        f"- Total: {summary.get('total', 0)}",
        f"- Passed: {summary.get('passed', 0)}",
        f"- Failed: {summary.get('failed', 0)}",
        f"- Skipped: {summary.get('skipped', 0)}",
        f"- Status: {summary.get('status', 'unknown')}",
        "",
    ]
    with open(REPORTS_DIR / "summary.md", "w", encoding="utf-8") as f:
        f.write("\n".join(lines))

    print(f"  ✅ HIL summary: {summary.get('passed', 0)}/{summary.get('total', 0)} passed "
          f"→ {REPORTS_DIR}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
