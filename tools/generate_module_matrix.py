#!/usr/bin/env python3
"""generate_module_matrix.py — yuleASR 模块关联矩阵 (PR-0.2)

产出 (reports/baseline/):
  module-matrix.csv           每模块一行: layer,module,dir,loc,canonical,
                              duplicate_candidates,template_present,template_same_layer,
                              test_files,coverage_line,coverage_branch,
                              misra_required,misra_deviated,requirement_count,
                              human_reviewed_requirements,status
  module-matrix-summary.json  统计聚合 (行数、各状态标志计数、重复组清单)

用法:
  python3 tools/generate_module_matrix.py
  python3 tools/generate_module_matrix.py --baseline reports/baseline   # 指定基线目录
"""
from __future__ import annotations

import csv
import fnmatch
import json
import os
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASELINE = Path(os.environ.get("BASELINE_DIR", ROOT / "reports" / "baseline"))

LAYERS = ("mcal", "ecual", "services", "os")
SINGLE_MODULE_LAYERS = {"os", "boot", "cdd"}  # no module subdirs; layer IS the module
COVERAGE_LOW = 60.0  # < 60% 视为低覆盖 (与 ci-config threshold_line 一致)

# ── helpers ─────────────────────────────────────────────────────

def module_loc(mod_dir: Path) -> int:
    """Count non-blank, non-comment lines in *.c under mod_dir/src."""
    total = 0
    src = mod_dir / "src"
    if not src.is_dir():
        return 0
    for f in src.glob("*.c"):
        for line in f.read_text(errors="ignore").splitlines():
            s = line.strip()
            if s and not s.startswith("/*") and not s.startswith("*") and not s.startswith("//"):
                total += 1
    return total


def find_templates(tpl_root: Path) -> dict[str, str]:
    """Map template stem (e.g. 'CanIf') -> layer dir name."""
    out: dict[str, str] = {}
    if not tpl_root.is_dir():
        return out
    for layer in ("mcal", "ecual", "services", "os", "integration"):
        d = tpl_root / layer
        if not d.is_dir():
            continue
        for f in d.glob("*.j2"):
            stem = f.name.split("_Cfg")[0]
            out[stem] = layer
    return out


def module_from_path(p: str) -> tuple[str | None, str | None]:
    """Map a repo-relative source path to (layer, module)."""
    parts = p.split("/")
    # src/bsw/{layer}/{module}/...
    try:
        i = parts.index("bsw")
        if len(parts) > i + 1 and parts[i + 1] in LAYERS:
            layer = parts[i + 1]
            if layer in SINGLE_MODULE_LAYERS:
                return layer, layer
            if len(parts) > i + 2:
                return layer, parts[i + 2]
        if len(parts) > i + 1 and parts[i + 1] in ("boot", "cdd"):
            return parts[i + 1], parts[i + 1]
    except ValueError:
        pass
    return None, None


def load_deviations() -> list[dict]:
    """Load approved MISRA deviations from ci-config.yaml (file patterns)."""
    try:
        import yaml
    except ImportError:
        return []
    cfg = yaml.safe_load((ROOT / ".yuleosh" / "ci-config.yaml").read_text())
    devs = []
    misra = cfg.get("misra", {}) or {}
    for d in misra.get("deviations", []) or []:
        if d.get("status") == "approved":
            devs.append({"rule": d.get("rule", ""), "pattern": d.get("file", "")})
    safety = (misra.get("profiles", {}) or {}).get("safety", {}) or {}
    for d in safety.get("deviations", []) or []:
        if d.get("status") == "approved":
            for scope in str(d.get("scope", "")).split(";"):
                if scope.strip():
                    devs.append({"rule": d.get("rule", ""), "pattern": scope.strip()})
    return devs


def dev_covers(pattern: str, path: str) -> bool:
    cands = {pattern}
    if pattern.startswith("**/"):
        cands.add(pattern[3:])
    if pattern.startswith("*/"):
        cands.add(pattern[2:])
    return any(fnmatch.fnmatch(path, c) for c in cands)


# ── main ────────────────────────────────────────────────────────

def main() -> int:
    bsw = ROOT / "src" / "bsw"
    templates = find_templates(ROOT / "tools" / "yuleasr-configurator" / "templates")

    # Discover modules: src/bsw/{layer}/{module} (or src/bsw/{layer} for single-module layers)
    modules: list[dict] = []
    name_index: dict[str, list[str]] = defaultdict(list)  # lower_name -> ["layer/module"]
    for layer in LAYERS:
        ldir = bsw / layer
        if not ldir.is_dir():
            continue
        if layer in SINGLE_MODULE_LAYERS:
            name_index[layer.lower()].append(f"{layer}/{layer}")
        else:
            for mod_dir in sorted(ldir.iterdir()):
                if not mod_dir.is_dir() or mod_dir.name == "include":
                    continue
                name_index[mod_dir.name.lower()].append(f"{layer}/{mod_dir.name}")
    # boot / cdd are special (no layer subdir under bsw/)
    for special in ("boot", "cdd"):
        d = bsw / special
        if d.is_dir():
            name_index[special].append(f"{special}/{special}")

    # Build rows
    rows = []
    for layer in LAYERS:
        ldir = bsw / layer
        if not ldir.is_dir():
            continue
        if layer in SINGLE_MODULE_LAYERS:
            rows.append(build_row(layer, layer, ldir, templates, name_index))
        else:
            for mod_dir in sorted(ldir.iterdir()):
                if not mod_dir.is_dir() or mod_dir.name == "include":
                    continue
                rows.append(build_row(layer, mod_dir.name, mod_dir, templates, name_index))
    for special in ("boot", "cdd"):
        d = bsw / special
        if d.is_dir():
            rows.append(build_row(special, special, d, templates, name_index))

    # Enrich with coverage + MISRA + requirements
    enrich_coverage(rows)
    enrich_misra(rows)
    enrich_requirements(rows)
    apply_status_flags(rows)

    # Write CSV
    BASELINE.mkdir(parents=True, exist_ok=True)
    csv_path = BASELINE / "module-matrix.csv"
    fields = ["layer", "module", "dir", "loc", "canonical", "duplicate_candidates",
              "template_present", "template_same_layer", "test_files",
              "coverage_line", "coverage_branch",
              "misra_required", "misra_deviated",
              "requirement_count", "human_reviewed_requirements", "status"]
    with open(csv_path, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        for r in rows:
            w.writerow({k: r.get(k, "") for k in fields})

    # Summary
    summary = build_summary(rows, name_index)
    (BASELINE / "module-matrix-summary.json").write_text(json.dumps(summary, indent=2))

    print(f"Wrote {csv_path} ({len(rows)} rows)")
    print(f"Wrote {BASELINE / 'module-matrix-summary.json'}")
    print(f"  unique modules: {summary['unique_modules']}")
    print(f"  duplicate impl groups: {len(summary['duplicate_groups'])}")
    print(f"  name collision groups: {len(summary['name_collision_groups'])}")
    for flag, cnt in sorted(summary["status_counts"].items()):
        print(f"  {flag}: {cnt}")
    return 0


def build_row(layer: str, name: str, mod_dir: Path, templates: dict,
              name_index: dict) -> dict:
    loc = module_loc(mod_dir)
    stem = name  # template lookup uses the dir name as-is
    tpl_layer = templates.get(stem)
    # also try case-insensitive match
    if tpl_layer is None:
        for k, v in templates.items():
            if k.lower() == stem.lower():
                tpl_layer = v
                break
    canonical = len(name_index.get(name.lower(), [])) == 1
    dups = [d for d in name_index.get(name.lower(), []) if d != f"{layer}/{name}"]
    # test files (module-specific dirs only)
    test_files = 0
    for tdir in (ROOT / "tests" / "bsw" / layer / name,
                 ROOT / "tests" / "unit" / name):
        if tdir.is_dir():
            test_files += len(list(tdir.glob("test_*.c"))) + len(list(tdir.glob("*_test.c")))
    # boot/cdd have inline tests under src/bsw/{name}/test
    inline = mod_dir / "test"
    if inline.is_dir():
        test_files += len(list(inline.glob("test_*.c")))
    return {
        "layer": layer, "module": name, "dir": str(mod_dir.relative_to(ROOT)),
        "loc": loc, "canonical": canonical,
        "duplicate_candidates": ";".join(dups),
        "template_present": tpl_layer is not None,
        "template_same_layer": (tpl_layer == layer) if tpl_layer else False,
        "test_files": test_files,
        "coverage_line": None, "coverage_branch": None,
        "misra_required": 0, "misra_deviated": 0,
        "requirement_count": 0, "human_reviewed_requirements": 0,
        "status": [],
    }


def enrich_coverage(rows: list[dict]) -> None:
    gcovr_json = BASELINE / "gcovr.json"
    if not gcovr_json.is_file():
        return
    data = json.loads(gcovr_json.read_text())
    # gcovr 8.x: each file has lines: [{line_number, count, branches: [{count}]}]
    fcov: dict[str, tuple[float, float]] = {}
    for f in data.get("files", []):
        fp = f.get("file", "")
        lf = lh = bf = bh = 0
        for ln in f.get("lines", []):
            lf += 1
            if ln.get("count", 0) > 0:
                lh += 1
            for br in ln.get("branches", []):
                bf += 1
                if br.get("count", 0) > 0:
                    bh += 1
        lp = round(lh * 100.0 / lf, 1) if lf else 0.0
        bp = round(bh * 100.0 / bf, 1) if bf else 0.0
        fcov[fp] = (lp, bp)
    # aggregate per module
    mod_cov: dict[tuple[str, str], list[tuple[float, float]]] = defaultdict(list)
    for fp, (lp, bp) in fcov.items():
        layer, mod = module_from_path(fp)
        if layer and mod:
            mod_cov[(layer, mod)].append((lp, bp))
    for r in rows:
        key = (r["layer"], r["module"])
        if key in mod_cov and mod_cov[key]:
            vals = mod_cov[key]
            r["coverage_line"] = round(sum(v[0] for v in vals) / len(vals), 1)
            r["coverage_branch"] = round(sum(v[1] for v in vals) / len(vals), 1)


def enrich_misra(rows: list[dict]) -> None:
    scan = BASELINE / "misra-scan.json"
    if not scan.is_file():
        return
    data = json.loads(scan.read_text())
    detail = data.get("detail", {})  # rule -> [(file, line, col)]
    devs = load_deviations()
    # per-module required / deviated counters
    mod_req: dict[tuple[str, str], int] = Counter()
    mod_dev: dict[tuple[str, str], int] = Counter()
    for rule, items in detail.items():
        for fname, _line, _col in items:
            layer, mod = module_from_path(fname)
            if not layer or not mod:
                continue
            covered = any(d["rule"] == rule and dev_covers(d["pattern"], fname) for d in devs)
            if covered:
                mod_dev[(layer, mod)] += 1
            else:
                mod_req[(layer, mod)] += 1
    for r in rows:
        key = (r["layer"], r["module"])
        r["misra_required"] = mod_req.get(key, 0)
        r["misra_deviated"] = mod_dev.get(key, 0)


def enrich_requirements(rows: list[dict]) -> None:
    matrix = ROOT / ".yuleosh" / "audit" / "acceptance-matrix.md"
    if not matrix.is_file():
        return
    # parse markdown table
    req_counts: dict[str, int] = Counter()
    reviewed: dict[str, int] = Counter()
    for line in matrix.read_text().splitlines():
        if not line.startswith("|"):
            continue
        cells = [c.strip() for c in line.strip("|").split("|")]
        if len(cells) < 2:
            continue
        req_id = cells[0]
        if not re.match(r"^[A-Z]+-SHALL-\d+$", req_id):
            continue
        # module prefix before -SHALL
        prefix = req_id.split("-SHALL")[0].lower()
        status = cells[-1] if cells else ""
        req_counts[prefix] += 1
        if "✅" in status:
            reviewed[prefix] += 1
    for r in rows:
        key = r["module"].lower()
        r["requirement_count"] = req_counts.get(key, 0)
        r["human_reviewed_requirements"] = reviewed.get(key, 0)


def apply_status_flags(rows: list[dict]) -> None:
    for r in rows:
        flags = []
        if not r["template_present"]:
            flags.append("NO_TEMPLATE")
        elif not r["template_same_layer"]:
            flags.append("TEMPLATE_LAYER_MISMATCH")
        if r["test_files"] == 0:
            flags.append("NO_TEST")
        if r["coverage_line"] is None:
            flags.append("COVERAGE_MISSING_FROM_REPORT")
        elif r["coverage_line"] == 0:
            flags.append("COVERAGE_ZERO")
        elif r["coverage_line"] < COVERAGE_LOW:
            flags.append("COVERAGE_LOW")
        if r["misra_required"] > 0:
            flags.append("MISRA_REQUIRED")
        elif r["misra_required"] == 0 and not (BASELINE / "misra-scan.json").is_file():
            flags.append("MISRA_NOT_SCANNED")
        if r["requirement_count"] == 0:
            flags.append("NO_REQUIREMENT")
        if r["human_reviewed_requirements"] < r["requirement_count"]:
            flags.append("NO_HUMAN_REVIEW")
        if not r["canonical"]:
            # exact duplicate vs name collision
            dups = r["duplicate_candidates"].split(";") if r["duplicate_candidates"] else []
            exact = any(d.split("/")[1].lower() == r["module"].lower() for d in dups)
            if exact:
                flags.append("DUPLICATE_IMPL")
            else:
                flags.append("NAME_COLLISION")
        r["status"] = flags


def build_summary(rows: list[dict], name_index: dict) -> dict:
    status_counts = Counter()
    for r in rows:
        for f in r["status"]:
            status_counts[f] += 1
    # duplicate groups: same lower-case name in multiple dirs
    dup_groups = {k: v for k, v in name_index.items() if len(v) > 1}
    exact_dups = {}
    collisions = {}
    for k, locs in dup_groups.items():
        names = {l.split("/")[1] for l in locs}
        if len(names) == 1:
            exact_dups[k] = locs
        else:
            collisions[k] = locs
    unique_modules = len({r["module"].lower() for r in rows})
    return {
        "total_rows": len(rows),
        "unique_modules": unique_modules,
        "duplicate_groups": exact_dups,
        "name_collision_groups": collisions,
        "status_counts": dict(status_counts),
    }


if __name__ == "__main__":
    sys.exit(main())
