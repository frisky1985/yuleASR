#!/usr/bin/env python3
"""Verify MISRA required violations for specific files after fixes.

Usage: python3 .yuleosh/tools/misra_verify.py [file...] [--rule RULE] [--all]
Runs cppcheck with the same args as the CI pipeline (review.py) on given
files and prints required violations grouped by rule.
"""
import json
import os
import re
import subprocess
import sys
from collections import Counter
from pathlib import Path

PROJECT = Path("/Users/stefan/.openclaw/workspace/yuleASR")


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
    inc = Path(PROJECT / "cppcheck-config.h")
    if inc.exists():
        cmd.append("--include=" + str(inc))
        cmd.append("--max-configs=1")
    # include paths
    for d in ["include", "src", "src/bsw", "src/bsw/mcal", "src/platform",
              "third_party", "tests/mocks"]:
        p = PROJECT / d
        if p.exists():
            cmd.append("-I" + str(p))
    for f in files:
        cmd.append(str(f))
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, cwd=str(PROJECT), timeout=300)
    except subprocess.TimeoutExpired:
        print("TIMEOUT")
        return {}
    out = (r.stderr or "") + "\n" + (r.stdout or "")
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
    # save json
    with open(PROJECT / ".yuleosh" / "reports" / "verify-result.json", "w") as f:
        json.dump({"total": total, "by_rule": dict(results)}, f, indent=2)
