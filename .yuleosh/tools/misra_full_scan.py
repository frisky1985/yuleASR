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


def _deviation_covers(pattern: str, path: str) -> bool:
    """Match a deviation glob against a repo-relative path.

    Handles the three forms used in ci-config.yaml: `**/src/**`, `*/src/...`, `src/...`.
    """
    import fnmatch
    cands = {pattern}
    if pattern.startswith("**/"):
        cands.add(pattern[3:])
    if pattern.startswith("*/"):
        cands.add(pattern[2:])
    return any(fnmatch.fnmatch(path, c) for c in cands)


def _load_gate_config():
    """Read the gate thresholds + approved deviation entries (unclassified).

    Returns (gate, raw_devs); raw_devs is a list of dicts with id/rule/pattern/occurrences.
    Classification against observed counts happens in _classify_deviations().
    """
    import yaml
    cfg_path = PROJECT / ".yuleosh" / "ci-config.yaml"
    if not cfg_path.is_file():
        print("FATAL[5]: ci-config.yaml missing — gate cannot be evaluated", file=sys.stderr)
        sys.exit(5)
    cfg = yaml.safe_load(cfg_path.read_text()) or {}
    misra = cfg.get("misra", {}) or {}
    gate = misra.get("gate", {}) or {}
    raw = []
    for d in misra.get("deviations", []) or []:
        if d.get("status") == "approved":
            raw.append({"id": d.get("id", f"{d.get('rule')} @ {d.get('file')}"),
                        "rule": d.get("rule", ""), "pattern": d.get("file", ""),
                        "occurrences": d.get("occurrences")})
    safety = (misra.get("profiles", {}) or {}).get("safety", {}) or {}
    for d in safety.get("deviations", []) or []:
        if d.get("status") == "approved":
            for scope in str(d.get("scope", "")).split(";"):
                if scope.strip():
                    raw.append({"id": d.get("id", f"{d.get('rule')} @ {scope.strip()}"),
                                "rule": d.get("rule", ""), "pattern": scope.strip(),
                                "occurrences": d.get("occurrences")})
    return gate, raw


def _classify_deviations(gate, raw, detail):
    """Split deviations into active / pending / drifted.

    - file- or module-scoped        -> active
    - catch-all without `occurrences` -> pending (exempts nothing)
    - catch-all WITH `occurrences: N`  -> active only when N equals that waiver's
      **net residual**: the number of violations it is the *sole* waiver for.
      A violation covered by two waivers belongs to neither, so overlapping
      waivers cannot hide violations behind each other. Any drift fails the gate.

    Returns (active, pending, drift, sole) where sole maps deviation id -> residual.
    """
    active, pending = [], []
    mode = str(gate.get("wildcard_deviations", "pending")).lower()
    for d in raw:
        if _is_catch_all(d["pattern"]) and mode == "pending" and d.get("occurrences") is None:
            pending.append(d)
        else:
            active.append(d)

    # Coverage index over all active waivers: which waiver ids cover each violation.
    # Built per rule so unrelated rules never collide.
    by_rule_devs = {}
    for d in active:
        by_rule_devs.setdefault(d["rule"], []).append(d)

    sole, drift = Counter(), []
    for rule, devs in by_rule_devs.items():
        bound = [d for d in devs if d.get("occurrences") is not None]
        if not bound:
            continue
        counts = Counter()
        for fname, _line, _col in detail.get(rule, []):
            cover = {d["id"] for d in devs if _deviation_covers(d["pattern"], fname)}
            if len(cover) == 1:
                counts[next(iter(cover))] += 1
        for d in bound:
            sole[d["id"]] = counts[d["id"]]
            declared = int(d["occurrences"])
            if declared != counts[d["id"]]:
                drift.append(f"{d['id']}: {rule} declares occurrences={declared}, "
                             f"net residual={counts[d['id']]} "
                             f"(total for rule={len(detail.get(rule, []))})")
    return active, pending, drift, sole


# Synthetic paths used to decide whether a pattern exempts everything.
_CATCH_ALL_SAMPLES = (
    "src/bsw/services/com/src/Com.c",
    "src/dds/core/dds_core.c",
    "src/zzz/qqq/aaa.c",
)


def _is_catch_all(pattern: str) -> bool:
    """True when the glob would exempt arbitrary files under the whole source tree."""
    if not pattern:
        return True
    return all(_deviation_covers(pattern, p) for p in _CATCH_ALL_SAMPLES)


def gate_check(res):
    """Block on invalid or stale evidence first, then on real violation counts.

    Exit 5 = gate failure. Never returns silently: empty input must never look like a pass.
    """
    gate, raw = _load_gate_config()
    prov = res["provenance"]
    fails = []

    min_files = int(gate.get("min_files_scanned", 1))
    if prov["scanned_files"] < min_files:
        fails.append(f"scanned_files={prov['scanned_files']} < min={min_files} (scan did not run)")
    if gate.get("require_provenance", True):
        try:
            head = subprocess.check_output(["git", "rev-parse", "HEAD"],
                                           cwd=str(PROJECT), text=True).strip()
        except Exception:
            head = "unknown"
        if prov["git_sha"] != head:
            fails.append(f"evidence commit {prov['git_sha'][:8]} != HEAD {head[:8]} (stale evidence)")
        if prov["tool"]["cppcheck"] == "unknown":
            fails.append("missing cppcheck version in provenance")
    if res["total"] == 0:
        fails.append("0 violations reported — empty data is not a pass")

    devs, pending, drift, sole = _classify_deviations(gate, raw, res["detail"])
    fails.extend("deviation drift — " + d for d in drift)

    required, deviated = Counter(), Counter()
    for rule, items in res["detail"].items():
        for fname, _line, _col in items:
            if any(d["rule"] == rule and _deviation_covers(d["pattern"], fname) for d in devs):
                deviated[rule] += 1
            else:
                required[rule] += 1
    req_total = sum(required.values())

    max_req = int(gate.get("max_required", 0))
    if req_total > max_req:
        fails.append(f"required violations {req_total} > gate max_required {max_req}")
    max_total = int(gate.get("max_total", 0))
    if max_total and res["total"] > max_total:
        fails.append(f"total violations {res['total']} > ratchet baseline {max_total}")

    result = {
        "passed": not fails,
        "required": req_total,
        "deviated": sum(deviated.values()),
        "total": res["total"],
        "scanned_files": prov["scanned_files"],
        "git_sha": prov["git_sha"],
        "gates": {"max_required": max_req, "max_total": max_total, "min_files_scanned": min_files},
        "failures": fails,
        "top_required": required.most_common(10),
        "pending_wildcard_deviations": sorted({f"{d['id']}: {d['rule']} @ {d['pattern']}"
                                               for d in pending}),
        "count_bound_deviations": sorted({f"{d['id']}: {d['rule']}="
                                          f"{d['occurrences']} (net residual {sole.get(d['id'], 0)})"
                                          for d in devs if d.get("occurrences") is not None}),
        "deviation_drift": drift,
    }
    out = PROJECT / ".yuleosh" / "reports" / "misra-gate.json"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(result, indent=1))
    for f in fails:
        print("GATE-FAIL: " + f, file=sys.stderr)
    print(f"GATE-{'PASS' if not fails else 'FAIL'}: required={req_total} "
          f"deviated={result['deviated']} total={res['total']} files={prov['scanned_files']}")
    if fails:
        sys.exit(5)
    return result


if __name__ == "__main__":
    args = sys.argv[1:]
    rules = None
    out = None
    gate_report = None
    do_gate = False
    for a in args:
        if a.startswith("--rules="):
            rules = a.split("=", 1)[1].split(",")
        elif a.startswith("--out="):
            out = a.split("=", 1)[1]
        elif a.startswith("--gate-report="):
            gate_report = a.split("=", 1)[1]
        elif a == "--gate":
            do_gate = True
    if gate_report:
        # Validate an archived report instead of the live run — catches stale evidence.
        gate_check(json.loads(Path(gate_report).read_text()))
    else:
        res = run(rules, out)
        if do_gate:
            gate_check(res)
