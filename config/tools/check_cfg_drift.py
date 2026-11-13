#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
yuleASR configuration drift checker
===================================

Renders the pre-compile configuration headers from the single source of truth
(``config/bsw_config.json`` + ``config/templates/``) using the authoritative
generator, then compares them against the **handwritten** ``*_Cfg.h`` headers
that live in the source tree (``src/``).

The build keeps the handwritten headers authoritative (``-idirafter``), so a
generated header is expected to be a *superset* of its in-tree counterpart.
Consequently this tool does **not** flag macros that exist on only one side —
it flags a macro only when it is defined in BOTH headers but with a DIFFERENT
value.  That is the signal that the configuration data and the C sources have
drifted apart and need reconciling.

Usage:
    python config/tools/check_cfg_drift.py                 # full report, exit 1 on drift
    python config/tools/check_cfg_drift.py --report-only   # always exit 0 (CI report mode)
    python config/tools/check_cfg_drift.py --module Can --module Gpt
    python config/tools/check_cfg_drift.py --json drift.json

Exit codes:
    0  no drift (or --report-only)
    1  drift detected
    2  usage/environment error
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Dict, List, Optional, Tuple

REPO_ROOT = Path(__file__).resolve().parents[2]
GENERATOR = REPO_ROOT / "config" / "tools" / "generate_bsw_config.py"
SRC_ROOT = REPO_ROOT / "src"

# Object-like macro definition: `#define NAME value` (function-like `NAME(...)`
# is intentionally skipped — configuration headers use object-like macros).
_DEFINE_RE = re.compile(r"^[ \t]*#[ \t]*define[ \t]+([A-Za-z_][A-Za-z0-9_]*)[ \t]+(.*?)[ \t]*$")


def parse_defines(path: Path) -> Dict[str, str]:
    """Return {macro_name: normalized_value} for object-like #defines.

    Include guards (value-less) and function-like macros are ignored.  Runs of
    whitespace inside the value are collapsed so that alignment padding in the
    generated headers does not register as a false drift.
    """
    defines: Dict[str, str] = {}
    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return defines
    for line in text.splitlines():
        m = _DEFINE_RE.match(line)
        if not m:
            continue
        name, value = m.group(1), m.group(2)
        if not value:
            continue  # include guard / value-less define
        # Strip trailing block/line comments that trail the value.
        value = re.sub(r"/\*.*?\*/", "", value)
        value = re.sub(r"//.*$", "", value).strip()
        value = re.sub(r"\s+", " ", value)
        if value:
            defines[name] = value
    return defines


def index_intree_headers(root: Path) -> Dict[str, Path]:
    """Index in-tree *_Cfg.h headers by lowercase basename (case-insensitive)."""
    index: Dict[str, Path] = {}
    for header in sorted(root.rglob("*_Cfg.h")):
        index.setdefault(header.name.lower(), header)
    return index


def run_generator(output_dir: Path, modules: Optional[List[str]]) -> None:
    cmd = [sys.executable, str(GENERATOR), "--no-rte", "--output", str(output_dir)]
    for m in modules or []:
        cmd += ["--module", m]
    proc = subprocess.run(cmd, cwd=str(REPO_ROOT), capture_output=True, text=True)
    if proc.returncode != 0 and not list(output_dir.glob("*.h")):
        sys.stderr.write(proc.stdout + proc.stderr)
        raise SystemExit("[ENV-ERROR] configuration generator produced no output")


def compare_module(
    generated: Path, intree: Optional[Path]
) -> Tuple[List[Tuple[str, str, str]], int, int]:
    """Compare one generated header with its in-tree counterpart.

    Returns (mismatches, generated_only_count, intree_only_count) where each
    mismatch is (macro, generated_value, intree_value).
    """
    gen_defs = parse_defines(generated)
    if intree is None:
        return [], len(gen_defs), 0
    tree_defs = parse_defines(intree)
    mismatches = [
        (name, gen_defs[name], tree_defs[name])
        for name in sorted(gen_defs.keys() & tree_defs.keys())
        if gen_defs[name] != tree_defs[name]
    ]
    gen_only = len(gen_defs.keys() - tree_defs.keys())
    tree_only = len(tree_defs.keys() - gen_defs.keys())
    return mismatches, gen_only, tree_only


def main(argv: Optional[List[str]] = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--module", action="append", dest="modules", metavar="NAME",
                    help="restrict to a module (repeatable)")
    ap.add_argument("--report-only", action="store_true",
                    help="print the report but always exit 0 (CI report mode)")
    ap.add_argument("--json", type=Path, default=None, help="write a JSON report to this path")
    args = ap.parse_args(argv)

    if not GENERATOR.exists():
        print(f"[ENV-ERROR] generator not found: {GENERATOR}", file=sys.stderr)
        return 2
    if not SRC_ROOT.is_dir():
        print(f"[ENV-ERROR] source root not found: {SRC_ROOT}", file=sys.stderr)
        return 2

    intree_index = index_intree_headers(SRC_ROOT)
    report = {"modules": [], "total_mismatches": 0, "modules_with_drift": 0}

    with tempfile.TemporaryDirectory() as tmp:
        out_dir = Path(tmp) / "generated"
        run_generator(out_dir, args.modules)

        generated_files = sorted(out_dir.glob("*_Cfg.h"))
        if args.modules:
            wanted = {m.lower() for m in args.modules}
            generated_files = [
                g for g in generated_files
                if g.name.split("_Cfg")[0].lower() in wanted
            ]

        for gen in generated_files:
            intree = intree_index.get(gen.name.lower())
            mismatches, gen_only, tree_only = compare_module(gen, intree)
            entry = {
                "module": gen.name.split("_Cfg")[0],
                "generated": str(gen.relative_to(out_dir)),
                "intree": str(intree.relative_to(REPO_ROOT)) if intree else None,
                "mismatches": [
                    {"macro": n, "generated": g, "intree": t} for n, g, t in mismatches
                ],
                "generated_only_macros": gen_only,
                "intree_only_macros": tree_only,
            }
            report["modules"].append(entry)
            if mismatches:
                report["modules_with_drift"] += 1
                report["total_mismatches"] += len(mismatches)

    # ---- human-readable report -------------------------------------------
    print("=" * 72)
    print("yuleASR configuration drift report (generated vs in-tree *_Cfg.h)")
    print("=" * 72)
    for entry in report["modules"]:
        if not entry["mismatches"]:
            continue
        print(f"\n[{entry['module']}]  {entry['intree'] or '(no in-tree header)'}")
        for mm in entry["mismatches"]:
            print(f"    {mm['macro']:<40} generated={mm['generated']:<12} intree={mm['intree']}")

    compared = sum(1 for e in report["modules"] if e["intree"])
    print("\n" + "-" * 72)
    print(f"modules compared (with in-tree header): {compared}")
    print(f"modules with drift:                     {report['modules_with_drift']}")
    print(f"total macro value mismatches:           {report['total_mismatches']}")

    if args.json:
        args.json.write_text(json.dumps(report, indent=2), encoding="utf-8")
        print(f"\nJSON report written to {args.json}")

    if report["total_mismatches"] == 0:
        print("\n[OK] No macro value drift between configuration and source tree.")
        return 0
    if args.report_only:
        print("\n[REPORT] Drift detected (report-only mode, exit 0).")
        return 0
    print("\n[FAIL] Configuration drift detected — reconcile config data with sources.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
