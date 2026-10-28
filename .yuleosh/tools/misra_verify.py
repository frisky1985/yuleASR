#!/usr/bin/env python3
"""Verify MISRA required violations for specific files after fixes.

Usage: python3 .yuleosh/tools/misra_verify.py [file...] [--rule RULE] [--all]
Runs cppcheck with the same args as the CI pipeline (review.py) on given
files and prints required violations grouped by rule.
"""
import json
import os
import re
import shutil
import subprocess
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from ci_includes import detect_includes  # noqa: E402

# Repo root derived from this file's location (.yuleosh/tools/ -> repo root).
PROJECT = Path(os.environ.get("YULEASR_ROOT", Path(__file__).resolve().parents[2]))


def run_cppcheck(files, rule_filter=None):
    cmd = [
        "cppcheck", "--addon=misra", "--language=c", "--std=c11",
        "--enable=all",
        "--suppress=missingIncludeSystem", "--suppress=missingInclude",
        "--suppress=normalCheckLevelMaxBranches", "-q",
    ]
    # same defines as CI
    for d in ["STD_ON", "STD_OFF", "STD_HIGH", "STD_LOW", "STD_ACTIVE", "STD_IDLE",
              "NULL_PTR", "TRUE", "FALSE", "E_OK", "E_NOT_OK", "NULL"]:
        cmd.append("-D" + d)
    inc = Path(PROJECT / "tools/analysis/cppcheck-config.h")
    if inc.exists():
        cmd.append("--include=" + str(inc))
        cmd.append("--max-configs=1")
    # same include discovery as the full scan, so counts are comparable
    for d in detect_includes():
        cmd.append("-I" + d)
    for f in files:
        cmd.append(str(f))
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, cwd=str(PROJECT),
                           timeout=300, env=dict(os.environ, PYTHONWARNINGS="ignore"))
    except subprocess.TimeoutExpired:
        print("TIMEOUT")
        return {}
    out = (r.stderr or "") + "\n" + (r.stdout or "")
    if "Bailing out from analysis" in out or "internalError" in out:
        print("FATAL[4]: cppcheck addon failed — result is not valid evidence", file=sys.stderr)
        sys.exit(4)
    # parse misra rules
    results = Counter()
    lines_detail = {}
    for line in out.splitlines():
        m = re.search(r"\[misra-c20\d\d-([0-9.]+)\]", line)
        if m:
            rule = "misra-c2023-" + m.group(1)
            fm = re.match(r"([^:]+):(\d+):", line)
            fname = os.path.basename(fm.group(1)) if fm else "?"
            if rule_filter and rule not in rule_filter:
                continue
            results[rule] += 1
            lines_detail.setdefault(rule, []).append(f"{fname}:{fm.group(2) if fm else '?'}")
    return results, lines_detail


if __name__ == "__main__":
    if not (PROJECT / "src").is_dir():
        print(f"FATAL[2]: repo root not found or missing src/: {PROJECT}", file=sys.stderr)
        sys.exit(2)
    if shutil.which("cppcheck") is None:
        print("FATAL[3]: cppcheck not installed in this environment", file=sys.stderr)
        sys.exit(3)
    args = sys.argv[1:]
    files = []
    rule_filter = None
    for a in args:
        if a == "--all":
            files = [str(p) for p in (PROJECT / "src").rglob("*.c")
                     if "legacy" not in str(p) and "test" not in str(p)]
        elif a.startswith("--rule="):
            rule_filter = a.split("=", 1)[1]
        elif a.startswith("--"):
            continue
        else:
            files.append(a)
    if not files:
        print("usage: misra_verify.py <files...> [--rule=misra-c2023-10.4] [--all]")
        sys.exit(1)
    results, detail = run_cppcheck(files, rule_filter)
    total = sum(results.values())
    print(f"\n=== {len(files)} file(s), {total} violations")
    for rule, n in results.most_common():
        print(f"{rule}: {n}")
        if rule_filter and rule_filter in rule:
            for d in detail[rule][:20]:
                print(f"    {d}")
    # save json — evidence must carry commit provenance and never crash on missing dir
    try:
        git_sha = subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=str(PROJECT), text=True,
            stderr=subprocess.DEVNULL).strip()
    except Exception:
        git_sha = "unknown"
    reports = PROJECT / ".yuleosh" / "reports"
    reports.mkdir(parents=True, exist_ok=True)
    with open(reports / "verify-result.json", "w") as f:
        json.dump({"total": total, "by_rule": dict(results),
                   "scanned_files": len(files), "git_sha": git_sha}, f, indent=2)
