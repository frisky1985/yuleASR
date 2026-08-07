#!/usr/bin/env python3
"""Exact replica of CI review.py include-path discovery (yuleASR)."""
import os
from pathlib import Path

PROJECT = Path("/Users/stefan/.openclaw/workspace/yuleASR")
SKIP_PREFIXES = (
    ".git", "build", "website", "node_modules", "__pycache__",
    ".docusaurus", "backups", ".yuleosh", ".osh", ".claude",
    ".build", "CMakeFiles", "coverage-report", "examples",
)


def _scan_include_dirs():
    scan_roots = ["src", "include", "tests", "third_party"]
    found = []
    seen = set()
    for root in scan_roots:
        abs_root = PROJECT / root
        if not abs_root.is_dir():
            continue
        rel_root = os.path.relpath(abs_root, PROJECT)
        if rel_root not in seen:
            seen.add(rel_root)
            found.append(rel_root)
        for dirpath, dirnames, _ in os.walk(abs_root):
            dirnames[:] = [
                d for d in dirnames
                if not d.startswith(".") and d != "__pycache__"
                and not any(d.startswith(p) for p in SKIP_PREFIXES)
            ]
            rel = os.path.relpath(dirpath, PROJECT)
            parts = rel.split(os.sep)
            skip = False
            for p in parts:
                if any(p.startswith(s) for s in SKIP_PREFIXES):
                    skip = True
                    break
            if skip:
                continue
            if os.path.basename(dirpath) == "include" and rel not in seen:
                seen.add(rel)
                found.append(rel)
            if os.path.basename(dirpath) == "src" and len(parts) >= 3 and rel not in seen:
                has_headers = any(f.endswith((".h", ".hpp")) for f in os.listdir(dirpath))
                if has_headers:
                    seen.add(rel)
                    found.append(rel)
    return found


def detect_includes():
    candidates = [
        ".", "src", "include", "inc", "config", "config/common", "tests",
        "tests/unity/src", "third_party", "lib", "common",
    ]
    all_cands = candidates + _scan_include_dirs()
    found = []
    seen = set()
    for c in all_cands:
        full = PROJECT / c
        norm = os.path.normpath(str(full))
        if norm not in seen and full.is_dir():
            seen.add(norm)
            found.append(c)
    return found


if __name__ == "__main__":
    for i in detect_includes():
        print(i)
