#!/usr/bin/env python3
"""PR-0.2 — module association matrix.

One machine-generated row per module implementation, so every module's
configuration template / tests / coverage / MISRA / requirements can be
inspected without hand-filling.

No value is invented: a missing source is reported as missing (e.g.
NO_TEMPLATE, NO_TEST, COVERAGE_ZERO, MISRA_NOT_SCANNED), never guessed.

Inputs
  src/bsw/<layer>/<module>/            module discovery
  tools/yuleasr-configurator/templates templates (*_Cfg.h.j2)
  tests/                               test evidence (.c/.cpp)
  reports/baseline/c-coverage.json     coverage per file (capture_baseline.sh)
  reports/baseline/misra-scan.json     MISRA detail  (capture_baseline.sh)
  .yuleosh/ci-config.yaml              deviation patterns for required/deviated split
  .yuleosh/evidence-bundle/traceability/traceability-report.json

Outputs
  reports/baseline/module-matrix.csv
  reports/baseline/module-matrix-summary.json

Usage
  python3 tools/generate_module_matrix.py [--root DIR] [--output PATH]
"""

import argparse
import csv
import fnmatch
import json
import os
import re
import subprocess
import sys
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path

LAYERS = ["mcal", "ecual", "services", "os", "cdd", "boot"]
SINGLE_MODULE_LAYERS = {"os": "Os", "cdd": "Cdd", "boot": "Boot"}
EXCLUDED_DIRS = {"include", "src", "config", "test", "tests", "docs", "doc"}
SOURCE_SUFFIXES = {".c", ".h", ".cpp", ".hpp"}
INCLUDE_RE = re.compile(r'#include\s+["<]([^">]+)[">]')


def git_sha(root: Path) -> str:
    try:
        return subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=root, text=True, stderr=subprocess.DEVNULL
        ).strip()
    except Exception:
        return "unknown"


def discover_modules(root: Path):
    """Return list of dicts: layer, module, path (repo-relative), loc."""
    modules = []
    for layer in LAYERS:
        layer_dir = root / "src" / "bsw" / layer
        if not layer_dir.is_dir():
            continue
        if layer in SINGLE_MODULE_LAYERS:
            name = SINGLE_MODULE_LAYERS[layer]
            modules.append({"layer": layer, "module": name, "dir": layer_dir})
            continue
        for child in sorted(layer_dir.iterdir()):
            if not child.is_dir() or child.name in EXCLUDED_DIRS:
                continue
            sources = [p for p in child.rglob("*") if p.suffix in SOURCE_SUFFIXES]
            if not sources:
                continue
            modules.append({"layer": layer, "module": child.name, "dir": child})
    for mod in modules:
        c_files = [p for p in mod["dir"].rglob("*.c")]
        mod["loc"] = sum(len(p.read_text(errors="ignore").splitlines()) for p in c_files)
        mod["rel"] = mod["dir"].relative_to(root).as_posix() + "/"
    return modules


def template_index(root: Path):
    """Map lower-case module key -> [(template_layer, repo-relative path)]."""
    index = defaultdict(list)
    tdir = root / "tools" / "yuleasr-configurator" / "templates"
    if not tdir.is_dir():
        return index
    for path in tdir.rglob("*_Cfg.h.j2"):
        stem = path.name[: -len("_Cfg.h.j2")].lower()
        index[stem].append((path.parent.name, path.relative_to(root).as_posix()))
    return index


def test_index(root: Path, module_names):
    """Map lower-case module name -> set of test files (path / include / dir evidence)."""
    matched = defaultdict(set)
    tests_root = root / "tests"
    if not tests_root.is_dir():
        return matched
    names = sorted({n.lower() for n in module_names}, key=len, reverse=True)

    for path in tests_root.rglob("*"):
        if not path.is_file() or path.suffix not in {".c", ".cpp"}:
            continue
        rel = path.relative_to(root).as_posix()
        segments = {s.lower() for s in path.relative_to(tests_root).parts}
        lower_name = path.name.lower()
        hits = set()
        for name in names:
            if name in segments:
                hits.add(name)
            if re.search(r"\b" + re.escape(name) + r"\b", lower_name):
                hits.add(name)
        try:
            text = path.read_text(errors="ignore")
        except Exception:
            text = ""
        for inc in INCLUDE_RE.findall(text):
            stem = Path(inc).stem.lower()
            if stem in names:
                hits.add(stem)
        for name in hits:
            matched[name].add(rel)
    return matched


def coverage_index(root: Path):
    path = root / "reports" / "baseline" / "c-coverage.json"
    if not path.exists():
        return None, []
    with open(path) as f:
        data = json.load(f)
    return data, data.get("files", [])


def coverage_for(prefix: str, files):
    found = hit = bfound = bhit = 0
    nfiles = 0
    for entry in files:
        name = entry.get("file", "")
        if not name.startswith(prefix):
            continue
        nfiles += 1
        lines = entry.get("lines", {})
        branches = entry.get("branches", {})
        found += int(lines.get("found", 0))
        hit += int(lines.get("hit", 0))
        bfound += int(branches.get("found", 0))
        bhit += int(branches.get("hit", 0))
    line_rate = (hit / found * 100.0) if found else 0.0
    branch_rate = (bhit / bfound * 100.0) if bfound else 0.0
    return {
        "files": nfiles,
        "line_found": found,
        "line_hit": hit,
        "branch_found": bfound,
        "branch_hit": bhit,
        "line_rate": round(line_rate, 2),
        "branch_rate": round(branch_rate, 2),
    }


def deviation_covers(pattern: str, path: str) -> bool:
    candidates = {pattern}
    if pattern.startswith("**/"):
        candidates.add(pattern[3:])
    if pattern.startswith("*/"):
        candidates.add(pattern[2:])
    if pattern.startswith("src/"):
        candidates.add("**/" + pattern)
    return any(fnmatch.fnmatch(path, cand) for cand in candidates)


def misra_index(root: Path):
    scan_path = root / "reports" / "baseline" / "misra-scan.json"
    if not scan_path.exists():
        return None, {}, {}
    with open(scan_path) as f:
        scan = json.load(f)
    deviations = []
    ci = root / ".yuleosh" / "ci-config.yaml"
    if ci.exists():
        try:
            import yaml  # optional; only used when available
            with open(ci) as f:
                cfg = yaml.safe_load(f) or {}
            raw = (cfg.get("misra", {}) or {}).get("deviations", []) or []
            for item in raw:
                if isinstance(item, dict) and item.get("rule") and item.get("pattern"):
                    deviations.append({"rule": item["rule"], "pattern": item["pattern"]})
        except Exception:
            deviations = []
    required_by_file = defaultdict(int)
    deviated_by_file = defaultdict(int)
    for rule, items in (scan.get("detail") or {}).items():
        for fname, _line, _col in items:
            if any(d["rule"] == rule and deviation_covers(d["pattern"], fname) for d in deviations):
                deviated_by_file[fname] += 1
            else:
                required_by_file[fname] += 1
    return scan, required_by_file, deviated_by_file


def requirement_index(root: Path):
    path = root / ".yuleosh" / "evidence-bundle" / "traceability" / "traceability-report.json"
    if not path.exists():
        return {}, {}
    with open(path) as f:
        data = json.load(f)
    reqs = (((data.get("lrm") or {}).get("requirements")) or [])
    by_file = defaultdict(list)
    human_map = {}
    for req in reqs:
        rid = req.get("req_id") or req.get("id")
        approved = [r for r in (req.get("reviews") or []) if r.get("status") == "approved"]
        human_map[rid] = any(
            r.get("reviewer") and "自动化" not in r["reviewer"] for r in approved
        )
        for code_file in req.get("code_files") or []:
            by_file[code_file].append(rid)
    return by_file, human_map


def module_requirements(prefix: str, by_file):
    req_ids = set()
    for code_file, ids in by_file.items():
        if code_file.startswith(prefix):
            req_ids.update(ids)
    return sorted(req_ids)


def to_bool_int(value: bool) -> int:
    return 1 if value else 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default=None)
    parser.add_argument("--output", default=None)
    args = parser.parse_args()
    root = Path(args.root).resolve() if args.root else Path(__file__).resolve().parents[1]
    out_dir = root / "reports" / "baseline"
    out_dir.mkdir(parents=True, exist_ok=True)
    out_csv = Path(args.output) if args.output else out_dir / "module-matrix.csv"
    out_json = out_dir / "module-matrix-summary.json"

    modules = discover_modules(root)
    templates = template_index(root)
    tests = test_index(root, [m["module"] for m in modules])
    cov_data, cov_files = coverage_index(root)
    misra_scan, misra_required, misra_deviated = misra_index(root)
    req_by_file, req_human_map = requirement_index(root)

    head = git_sha(root)
    coverage_sha = ((cov_data or {}).get("provenance") or {}).get("git_sha")
    misra_sha = ((misra_scan or {}).get("provenance") or {}).get("git_sha")

    by_name = defaultdict(list)
    for mod in modules:
        by_name[mod["module"]].append(mod)
    by_name_lower = defaultdict(list)
    for mod in modules:
        by_name_lower[mod["module"].lower()].append(mod)

    exact_duplicates = {name: mods for name, mods in by_name.items() if len(mods) > 1}
    name_collisions = {
        name: mods for name, mods in by_name_lower.items()
        if len(mods) > 1 and len({m["module"] for m in mods}) > 1
    }

    rows = []
    for mod in sorted(modules, key=lambda m: (m["layer"], m["module"].lower())):
        key = mod["module"].lower()
        prefix = mod["rel"]
        all_templates = templates.get(key, [])
        same_layer_templates = [p for tlayer, p in all_templates if tlayer == mod["layer"]]
        tpaths = same_layer_templates or [p for _tlayer, p in all_templates]
        template_same_layer = bool(same_layer_templates)
        exact_dup = mod["module"] in exact_duplicates
        collision = (not exact_dup) and key in name_collisions
        dup_group = exact_duplicates.get(mod["module"]) or name_collisions.get(key) or []
        test_files = sorted(tests.get(key, set()))
        cov = coverage_for(prefix, cov_files)
        req_ids = module_requirements(prefix, req_by_file)
        human_req = sum(1 for rid in req_ids if req_human_map.get(rid))

        misra_req = sum(n for fname, n in misra_required.items() if fname.startswith(prefix))
        misra_dev = sum(n for fname, n in misra_deviated.items() if fname.startswith(prefix))

        flags = []
        if exact_dup:
            flags.append("DUPLICATE_IMPL")
        elif collision:
            flags.append("NAME_COLLISION")
        if not all_templates:
            flags.append("NO_TEMPLATE")
        elif not template_same_layer:
            flags.append("TEMPLATE_LAYER_MISMATCH")
        if not test_files:
            flags.append("NO_TEST")
        if cov_data is None:
            flags.append("NO_COVERAGE_DATA")
        elif cov["files"] == 0:
            flags.append("COVERAGE_MISSING_FROM_REPORT")
        elif cov["line_found"] == 0 or cov["line_hit"] == 0:
            flags.append("COVERAGE_ZERO")
        elif cov["line_rate"] < 80.0:
            flags.append("COVERAGE_LOW")
        if misra_scan is None:
            flags.append("MISRA_NOT_SCANNED")
        elif misra_req > 0:
            flags.append("MISRA_REQUIRED")
        if not req_ids:
            flags.append("NO_REQUIREMENT")
        elif human_req == 0:
            flags.append("NO_HUMAN_REVIEW")

        rows.append({
            "layer": mod["layer"],
            "module": mod["module"],
            "module_dir": prefix,
            "code_loc": mod["loc"],
            "canonical": "TBD" if (exact_dup or collision) else prefix,
            "duplicate_candidates": ";".join(
                f"{m['rel']}({m['loc']} loc)" for m in dup_group
            ),
            "template_present": to_bool_int(bool(all_templates)),
            "template_same_layer": to_bool_int(template_same_layer),
            "template_paths": ";".join(tpaths),
            "test_files_count": len(test_files),
            "test_files": ";".join(test_files),
            "coverage_files": cov["files"],
            "line_found": cov["line_found"],
            "line_hit": cov["line_hit"],
            "line_rate": cov["line_rate"],
            "branch_found": cov["branch_found"],
            "branch_hit": cov["branch_hit"],
            "branch_rate": cov["branch_rate"],
            "misra_required": misra_req,
            "misra_deviated": misra_dev,
            "requirement_count": len(req_ids),
            "requirement_ids": ";".join(req_ids),
            "human_reviewed_requirements": human_req,
            "status": "OK" if not flags else ";".join(flags),
        })

    fieldnames = list(rows[0].keys()) if rows else []
    with open(out_csv, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    def count_flagged(flag):
        return sum(1 for r in rows if flag in r["status"].split(";"))

    summary = {
        "schema": 1,
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "git_sha": head,
        "sources": {
            "coverage": (cov_data or {}).get("provenance"),
            "misra": (misra_scan or {}).get("provenance"),
        },
        "stale_evidence": {
            "coverage": bool(coverage_sha and coverage_sha != head),
            "misra": bool(misra_sha and misra_sha != head),
        },
        "counts": {
            "implementation_rows": len(rows),
            "unique_modules_exact": len(by_name),
            "unique_modules_case_insensitive": len(by_name_lower),
            "exact_duplicate_groups": len(exact_duplicates),
            "name_collision_groups": len(name_collisions),
            "ok": sum(1 for r in rows if r["status"] == "OK"),
            "no_template": count_flagged("NO_TEMPLATE"),
            "template_layer_mismatch": count_flagged("TEMPLATE_LAYER_MISMATCH"),
            "no_test": count_flagged("NO_TEST"),
            "coverage_zero": count_flagged("COVERAGE_ZERO"),
            "coverage_low": count_flagged("COVERAGE_LOW"),
            "coverage_missing": count_flagged("COVERAGE_MISSING_FROM_REPORT"),
            "misra_required_modules": count_flagged("MISRA_REQUIRED"),
            "misra_not_scanned": count_flagged("MISRA_NOT_SCANNED"),
            "no_requirement": count_flagged("NO_REQUIREMENT"),
            "no_human_review": count_flagged("NO_HUMAN_REVIEW"),
        },
        "exact_duplicates": {
            name: [{"dir": m["rel"], "loc": m["loc"]} for m in mods]
            for name, mods in sorted(exact_duplicates.items())
        },
        "name_collisions": {
            name: [{"module": m["module"], "dir": m["rel"], "loc": m["loc"]} for m in mods]
            for name, mods in sorted(name_collisions.items())
        },
        "layers": {layer: sum(1 for m in modules if m["layer"] == layer) for layer in LAYERS},
        "output_csv": str(out_csv),
    }
    with open(out_json, "w") as f:
        json.dump(summary, f, indent=1)

    c = summary["counts"]
    print(f"module matrix: {c['implementation_rows']} rows / "
          f"{c['unique_modules_exact']} unique names / "
          f"{c['unique_modules_case_insensitive']} case-insensitive")
    print(f"  OK={c['ok']} NO_TEMPLATE={c['no_template']} "
          f"TEMPLATE_LAYER_MISMATCH={c['template_layer_mismatch']} NO_TEST={c['no_test']} "
          f"COVERAGE_ZERO={c['coverage_zero']} COVERAGE_LOW={c['coverage_low']}")
    print(f"  exact_duplicate_groups={c['exact_duplicate_groups']} "
          f"name_collision_groups={c['name_collision_groups']}")
    print(f"  MISRA_REQUIRED_MODULES={c['misra_required_modules']} "
          f"NO_REQUIREMENT={c['no_requirement']} NO_HUMAN_REVIEW={c['no_human_review']}")
    if summary["stale_evidence"]["coverage"] or summary["stale_evidence"]["misra"]:
        print("  WARNING: evidence git_sha differs from HEAD (rerun tools/capture_baseline.sh)")
    print(f"  wrote {out_csv}")
    print(f"  wrote {out_json}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
