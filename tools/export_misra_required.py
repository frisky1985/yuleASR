#!/usr/bin/env python3
"""导出 MISRA required 违规到 CSV (rule, file, line, reason)。

复用 misra_full_scan.py 的偏差分类逻辑，筛选未被已批准偏差覆盖的违规。

Usage:
    python3 tools/export_misra_required.py [scan.json] [output.csv]
"""
import csv
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / ".yuleosh" / "tools"))
from misra_full_scan import _load_gate_config, _classify_deviations, _deviation_covers  # noqa: E402

DEFAULT_SCAN = ROOT / "reports" / "baseline" / "misra-scan.json"
DEFAULT_OUT = ROOT / "reports" / "misra-required-violations.csv"


def main():
    scan_path = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_SCAN
    out_path = Path(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_OUT

    if not scan_path.is_file():
        print(f"FATAL: scan report not found: {scan_path}", file=sys.stderr)
        sys.exit(2)

    scan = json.loads(scan_path.read_text())
    gate, raw = _load_gate_config()
    devs, pending, drift, sole = _classify_deviations(gate, raw, scan["detail"])

    rows = []
    for rule, items in scan["detail"].items():
        for fname, line, col in items:
            covered = any(
                d["rule"] == rule and _deviation_covers(d["pattern"], fname)
                for d in devs
            )
            if not covered:
                # 尝试在 deviations 中找到该规则的原因（即使不匹配该文件，
                # 也给出规则级说明帮助修复）
                reason = ""
                for d in raw:
                    if d["rule"] == rule:
                        reason = d.get("id", "")
                        break
                rows.append({
                    "rule": rule,
                    "file": fname,
                    "line": line,
                    "column": col,
                    "reason": reason,
                })

    rows.sort(key=lambda r: (r["rule"], r["file"], r["line"]))

    out_path.parent.mkdir(parents=True, exist_ok=True)
    with open(out_path, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=["rule", "file", "line", "column", "reason"])
        w.writeheader()
        w.writerows(rows)

    print(f"Wrote {len(rows)} required violations -> {out_path}")
    by_rule = {}
    for r in rows:
        by_rule[r["rule"]] = by_rule.get(r["rule"], 0) + 1
    for rule, n in sorted(by_rule.items(), key=lambda x: -x[1]):
        print(f"  {rule}: {n}")
    return len(rows)


if __name__ == "__main__":
    n = main()
    sys.exit(0 if n > 0 else 1)
