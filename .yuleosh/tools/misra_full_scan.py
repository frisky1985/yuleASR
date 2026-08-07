#!/usr/bin/env python3
"""Full-tree MISRA rescan replicating the CI review.py cppcheck invocation.

Usage: python3 misra_full_scan.py [--rules r1,r2] [--out /tmp/misra-scan.json]
Prints counts per rule for the whole src/ tree (plus scan_dirs from config).
"""
import json
import os
import re
import subprocess
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from ci_includes import detect_includes  # noqa: E402

PROJECT = Path("/Users/stefan/.openclaw/workspace/yuleASR")


def find_c_sources():
    files = []
    for root, dirs, fs in os.walk(PROJECT / "src"):
        dirs[:] = [d for d in dirs if not d.startswith(".") and d != "__pycache__"]
        for f in fs:
            if f.endswith((".c", ".cpp")):
                files.append(os.path.relpath(os.path.join(root, f), PROJECT))
    return files


def run(rules_filter=None, out=None):
    files = find_c_sources()
    cmd = ["cppcheck", "--addon=" + str(PROJECT / ".yuleosh" / "misra-addon-config.json"),
           "--language=c", "--std=c11", "--enable=all",
           "--suppress=missingIncludeSystem", "--suppress=missingInclude",
           "--suppress=normalCheckLevelMaxBranches", "-q",
           "--suppress=misra-config", "--max-configs=1",
           ]
    for d in ["STD_ON", "STD_OFF", "STD_HIGH", "STD_LOW", "STD_ACTIVE", "STD_IDLE",
              "NULL_PTR", "TRUE", "FALSE", "E_OK", "E_NOT_OK", "NULL"]:
        cmd.append("-D" + d)
    inc = PROJECT / "cppcheck-config.h"
    if inc.exists():
        cmd.append("--include=" + str(inc))
    for i in detect_includes():
        cmd.append("-I" + i)
    supp = PROJECT / ".cppcheck_suppressions"
    if supp.exists():
        cmd.append("--suppressions-list=" + str(supp))
    cmd += files
    print(f"scanning {len(files)} files ...", file=sys.stderr)
    r = subprocess.run(cmd, capture_output=True, text=True, cwd=str(PROJECT), timeout=1800)
    out_text = (r.stderr or "") + "\n" + (r.stdout or "")
    counts = Counter()
    detail = {}
    for line in out_text.splitlines():
        m = re.search(r"\[misra-c20\d\d-([0-9.]+)\]", line)
        if m:
            rule = "misra-c2023-" + m.group(1)
            if rules_filter and rule not in rules_filter:
                continue
            counts[rule] += 1
            fm = re.match(r"([^:]+):(\d+):(\d+):", line)
            if fm:
                detail.setdefault(rule, []).append((fm.group(1), int(fm.group(2)), int(fm.group(3))))
    res = {"total": sum(counts.values()), "by_rule": dict(counts), "detail": detail}
    if out:
        with open(out, "w") as f:
            json.dump(res, f, indent=1)
    print(f"total={res['total']}")
    for rule, n in counts.most_common():
        print(f"  {rule}: {n}")
    return res


if __name__ == "__main__":
    args = sys.argv[1:]
    rules = None
    out = None
    for a in args:
        if a.startswith("--rules="):
            rules = a.split("=", 1)[1].split(",")
        elif a.startswith("--out="):
            out = a.split("=", 1)[1]
    run(rules, out)
