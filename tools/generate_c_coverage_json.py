#!/usr/bin/env python3
"""
Generate .yuleosh/reports/c-coverage.json from a gcovr JSON or lcov .info file.

Trust model (WP2): the output is reproducible evidence, so every report carries
a provenance block (git_sha + tool versions + generated_at) and all file paths
are normalized to repo-relative form. Any absolute path that cannot be made
relative to the repo root is a hard error — it means the data was generated
elsewhere and cannot be trusted.

Usage:
    python3 tools/generate_c_coverage_json.py <input> [output_path] [--root DIR]
"""
import json
import os
import re
import subprocess
import sys
from datetime import datetime, timezone

PROJECT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def _norm_path(p: str, root: str) -> str:
    """Make a path repo-relative; raise if it escapes the repo root."""
    if os.path.isabs(p):
        rel = os.path.relpath(p, root)
        if rel.startswith(".."):
            raise ValueError(f"path outside repo root: {p}")
        return rel.replace(os.sep, "/")
    return p.replace(os.sep, "/")


def _provenance(root: str) -> dict:
    try:
        sha = subprocess.check_output(["git", "rev-parse", "HEAD"],
                                      cwd=root, text=True).strip()
        dirty = bool(subprocess.check_output(["git", "status", "--porcelain"],
                                             cwd=root, text=True).strip())
    except Exception:
        sha, dirty = "unknown", False
    tool = {}
    for name, cmd in (("gcovr", ["gcovr", "--version"]), ("gcc", ["gcc", "--version"])):
        try:
            out = subprocess.check_output(cmd, text=True, stderr=subprocess.STDOUT)
            tool[name] = out.splitlines()[0].strip()
        except Exception:
            tool[name] = "unknown"
    return {
        "schema": 2,
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "git_sha": sha,
        "git_dirty": dirty,
        "root": root,
        "tool": tool,
    }


def _parse_gcovr(report: dict, root: str):
    files = []
    for f in report.get("files", []):
        fpath = _norm_path(f.get("file", ""), root)
        fl = {"found": 0, "hit": 0}
        fb = {"found": 0, "hit": 0}
        ff = {"found": 0, "hit": 0}
        for ln in f.get("lines", []):
            fl["found"] += 1
            if ln.get("count", 0) > 0:
                fl["hit"] += 1
            for br in ln.get("branches", []):
                fb["found"] += 1
                if br.get("count", 0) > 0:
                    fb["hit"] += 1
        for fn in f.get("functions", []):
            ff["found"] += 1
            if fn.get("execution_count", 0) > 0:
                ff["hit"] += 1
        files.append({"file": fpath, "lines": fl, "functions": ff, "branches": fb})
    totals = {"lines": {"found": 0, "hit": 0},
              "functions": {"found": 0, "hit": 0},
              "branches": {"found": 0, "hit": 0}}
    for f in files:
        for k in ("lines", "functions", "branches"):
            totals[k]["found"] += f[k]["found"]
            totals[k]["hit"] += f[k]["hit"]
    return files, totals


def parse_lcov_info(info_file: str, root: str = PROJECT) -> dict:
    """Parse an lcov .info file and produce a coverage report dict."""
    files = []
    totals = {
        'lines': {'found': 0, 'hit': 0},
        'functions': {'found': 0, 'hit': 0},
        'branches': {'found': 0, 'hit': 0},
    }
    current = None

    def _finalize():
        nonlocal current
        if current is None:
            return
        for k in ['lines', 'functions', 'branches']:
            totals[k]['found'] += current[k]['found']
            totals[k]['hit'] += current[k]['hit']
        files.append(current)
        current = None

    with open(info_file) as f:
        for line in f:
            stripped = line.strip()

            m_sf = re.match(r'^SF:(.+)$', stripped)
            if m_sf:
                _finalize()
                current = {
                    'file': _norm_path(m_sf.group(1), root),
                    'lines': {'found': 0, 'hit': 0},
                    'functions': {'found': 0, 'hit': 0},
                    'branches': {'found': 0, 'hit': 0},
                }
                continue

            if current is None:
                continue

            m_da = re.match(r'^DA:(\d+),(\d+)$', stripped)
            if m_da:
                count = int(m_da.group(2))
                current['lines']['found'] += 1
                if count > 0:
                    current['lines']['hit'] += 1
                continue

            m_fnf = re.match(r'^FNF:(\d+)$', stripped)
            if m_fnf:
                current['functions']['found'] = int(m_fnf.group(1))
                continue

            m_fnh = re.match(r'^FNH:(\d+)$', stripped)
            if m_fnh:
                current['functions']['hit'] = int(m_fnh.group(1))
                continue

            m_brf = re.match(r'^BRF:(\d+)$', stripped)
            if m_brf:
                current['branches']['found'] = int(m_brf.group(1))
                continue

            m_brh = re.match(r'^BRH:(\d+)$', stripped)
            if m_brh:
                current['branches']['hit'] = int(m_brh.group(1))
                continue

            if stripped == 'end_of_record':
                _finalize()

    _finalize()
    return files, totals


def build_report(files, totals, root):
    lf, lt = totals['lines']['found'], totals['lines']['hit']
    bf, bt = totals['branches']['found'], totals['branches']['hit']
    return {
        'success': True,
        'line_rate': round(lt / max(lf, 1) * 100, 2),
        'branch_rate': round(bt / max(bf, 1) * 100, 2),
        'total_files': len(files),
        'totals': totals,
        'files': [
            {
                'file': f['file'],
                'line_rate': round(f['lines']['hit'] / max(f['lines']['found'], 1) * 100, 2),
                'branch_rate': round(f['branches']['hit'] / max(f['branches']['found'], 1) * 100, 2),
                'lines': f['lines'],
                'functions': f['functions'],
            }
            for f in files
        ],
        'provenance': _provenance(root),
    }


if __name__ == '__main__':
    args = sys.argv[1:]
    root = PROJECT
    if '--root' in args:
        i = args.index('--root')
        root = os.path.abspath(args[i + 1])
        del args[i:i + 2]
    if not args:
        print("Usage: python3 tools/generate_c_coverage_json.py <input> [output] [--root DIR]")
        sys.exit(1)

    inp = args[0]
    out = args[1] if len(args) > 1 else os.path.join(PROJECT, ".yuleosh", "reports", "c-coverage.json")

    if inp.endswith(".json"):
        with open(inp) as f:
            data = json.load(f)
        files, totals = _parse_gcovr(data, root)
    else:
        files, totals = parse_lcov_info(inp, root)

    report = build_report(files, totals, root)
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, 'w') as f:
        json.dump(report, f, indent=2)

    s = report['totals']['lines']
    print(f"Coverage report saved: {out}")
    print(f"  Lines: {s['hit']}/{s['found']} ({report['line_rate']}%)")
    print(f"  Files: {report['total_files']}")
    print(f"  git_sha: {report['provenance']['git_sha'][:8]}")
