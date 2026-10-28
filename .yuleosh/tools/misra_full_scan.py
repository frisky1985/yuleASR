#!/usr/bin/env python3
"""Full-tree MISRA rescan replicating the CI review.py cppcheck invocation.

Usage: python3 misra_full_scan.py [--rules r1,r2] [--out /tmp/misra-scan.json]
Prints counts per rule for the whole src/ tree (plus scan_dirs from config).
"""
import json
import os
import re
import shutil
import subprocess
import sys
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from ci_includes import detect_includes  # noqa: E402

# Repo root derived from this file's location (.yuleosh/tools/ -> repo root).
# YULEASR_ROOT allows CI / negative tests to point at another tree.
PROJECT = Path(os.environ.get("YULEASR_ROOT", Path(__file__).resolve().parents[2]))


def preflight(root: Path) -> None:
    """Fail loudly instead of silently scanning nothing (the root cause of stale evidence)."""
    if not (root / "src").is_dir():
        print(f"FATAL[2]: repo root not found or missing src/: {root}", file=sys.stderr)
        sys.exit(2)
    if shutil.which("cppcheck") is None:
        print("FATAL[3]: cppcheck not installed in this environment", file=sys.stderr)
        sys.exit(3)


def provenance(scanned: int) -> dict:
    """Every evidence file must be traceable to a commit + tool version + input size."""
    def _git(*args: str) -> str:
        try:
            return subprocess.check_output(
                ["git", *args], cwd=str(PROJECT), text=True, stderr=subprocess.DEVNULL
            ).strip()
        except Exception:
            return "unknown"

    version = "unknown"
    if shutil.which("cppcheck"):
        try:
            version = subprocess.check_output(["cppcheck", "--version"], text=True).strip()
        except Exception:
            pass
    sha = _git("rev-parse", "HEAD")
    return {
        "schema": 2,
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "git_sha": sha,
        "git_dirty": bool(_git("status", "--porcelain")) if sha != "unknown" else None,
        "root": str(PROJECT),
        "scanned_files": scanned,
        "tool": {"cppcheck": version},
    }


def find_c_sources():
    files = []
    for root, dirs, fs in os.walk(PROJECT / "src"):
        dirs[:] = [d for d in dirs if not d.startswith(".") and d != "__pycache__"]
        for f in fs:
            if f.endswith((".c", ".cpp")):
                files.append(os.path.relpath(os.path.join(root, f), PROJECT))
    return files


def run(rules_filter=None, out=None):
    preflight(PROJECT)
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
    inc = PROJECT / "tools/analysis/cppcheck-config.h"
    if inc.exists():
        cmd.append("--include=" + str(inc))
    for i in detect_includes():
        cmd.append("-I" + i)
    supp = PROJECT / ".cppcheck_suppressions"
    if supp.exists():
        cmd.append("--suppressions-list=" + str(supp))
    cmd += files
    print(f"scanning {len(files)} files ...", file=sys.stderr)
    # PYTHONWARNINGS=ignore: Python >=3.13 emits DeprecationWarning from codecs.open()
    # inside cppcheck's misra.py, which makes cppcheck bail out with internalError
    # and silently yield zero violations.
    env = dict(os.environ, PYTHONWARNINGS="ignore")
    r = subprocess.run(cmd, capture_output=True, text=True, cwd=str(PROJECT),
                       timeout=1800, env=env)
    out_text = (r.stderr or "") + "\n" + (r.stdout or "")
    if "Bailing out from analysis" in out_text or "internalError" in out_text:
        print("FATAL[4]: cppcheck addon failed — scan output is not valid evidence",
              file=sys.stderr)
        sys.exit(4)
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
    if not files:
        print("FATAL[2]: no C sources found — scan produced no input", file=sys.stderr)
        sys.exit(2)
    res = {"total": sum(counts.values()), "by_rule": dict(counts), "detail": detail,
           "provenance": provenance(len(files))}
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
