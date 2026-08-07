#!/usr/bin/env python3
"""Fix MISRA C:2023 Rule 13.3 — increment/decrement expression with other side effects.

Patterns:
  1. buffer[pos++] = value;            → buffer[pos] = value; pos++;
  2. uint32 idx = g_ctx.counter++;     → uint32 idx = g_ctx.counter; g_ctx.counter++;
  3. ptr = &arr[g_numEntries++];       → ptr = &arr[g_numEntries]; g_numEntries++;
  4. arr[--counter] = value;           → --counter; arr[counter] = value;
  5. a[(*actual_changes)++] = cur;     → a[*actual_changes] = cur; (*actual_changes)++;

Only rewrites single-statement lines where the ++/-- operand is a simple
lvalue (identifier or member chain). Multi-line statements and complex
cases are reported for manual handling.

Usage: python3 fix_13_3.py [--apply] [--check]
"""
import json
import re
import sys
from pathlib import Path

PROJECT = Path("/Users/stefan/.openclaw/workspace/yuleASR")
REPORT = PROJECT / ".yuleosh" / "reports" / "misra-report.json"

LVALUE = r"(?:[A-Za-z_]\w*(?:\s*->\s*[A-Za-z_]\w*|\s*\.\s*[A-Za-z_]\w*|\s*\[[^\]]+\])*)"
POST_INC = re.compile(r"^(?P<prefix>.*?)(?P<lval>" + LVALUE + r")(?P<op>\+\+|--)\s*(?P<rest>;?\s*)$")

def collect_violations():
    with open(REPORT) as f:
        d = json.load(f)
    out = []
    for v in d["violations_raw"]:
        if not isinstance(v, dict):
            continue
        if v.get("rule_id") == "misra-c2023-13.3" and v.get("rule_type") == "required":
            out.append((v["file"], v["line"]))
    return out

def fix_line(line: str) -> str | None:
    """Return fixed single-line code, or None if not auto-fixable."""
    s = line.rstrip("\n")
    stripped = s.strip()
    if not stripped.endswith(";"):
        return None  # multi-line statement, skip
    # Pattern: X[y++] = val;  or  X[y++] += val; (subscript post-inc)
    m = re.match(r"^(?P<lhs>" + LVALUE + r")\s*(?P<assign>[+\-*/%&|^]?=)\s*(?P<rhs>.*?);\s*$", stripped)
    if m:
        lhs, rhs = m.group("lhs"), m.group("rhs")
        # subscript with post inc: arr[idx++] or arr[idx--]
        sm = re.match(r"^(?P<base>" + LVALUE + r")\s*\[\s*(?P<idx>" + LVALUE + r")(?P<op>\+\+|--)\s*\]$", lhs)
        if sm and (not rhs or True):
            base, idx, op = sm.group("base"), sm.group("idx"), sm.group("op")
            indent = s[:len(s) - len(s.lstrip())]
            new = f"{indent}{base}[{idx}] {m.group('assign')} {rhs};"
            new += f"\n{indent}{idx}{op};"
            return new
    # Pattern: lval = X++;  (plain assignment of post-increment value)
    m = re.match(r"^(?P<lhs>" + LVALUE + r")\s*=\s*(?P<rhs>" + LVALUE + r")(?P<op>\+\+|--)\s*;\s*$", stripped)
    if m:
        lhs, rhs, op = m.group("lhs"), m.group("rhs"), m.group("op")
        indent = s[:len(s) - len(s.lstrip())]
        new = f"{indent}{lhs} = {rhs};"
        new += f"\n{indent}{rhs}{op};"
        return new
    # Pattern: ptr = &arr[g_num++];
    m = re.match(r"^(?P<lhs>" + LVALUE + r")\s*=\s*&\s*(?P<base>" + LVALUE + r")\s*\[\s*(?P<idx>" + LVALUE + r")(?P<op>\+\+|--)\s*\]\s*;\s*$", stripped)
    if m:
        lhs, base, idx, op = m.group("lhs"), m.group("base"), m.group("idx"), m.group("op")
        indent = s[:len(s) - len(s.lstrip())]
        new = f"{indent}{lhs} = &{base}[{idx}];"
        new += f"\n{indent}{idx}{op};"
        return new
    # Pattern: arr[--counter] = val; (pre-inc in subscript)
    m = re.match(r"^(?P<base>" + LVALUE + r")\s*\[\s*(?P<op>--|\+\+)\s*(?P<idx>" + LVALUE + r")\s*\]\s*(?P<assign>[+\-*/%&|^]?=)\s*(?P<rhs>.*?);\s*$", stripped)
    if m:
        base, op, idx, assign, rhs = m.group("base"), m.group("op"), m.group("idx"), m.group("assign"), m.group("rhs")
        indent = s[:len(s) - len(s.lstrip())]
        new = f"{indent}{op}{idx};"
        new += f"\n{indent}{base}[{idx}] {assign} {rhs};"
        return new
    return None

def main():
    apply_changes = "--apply" in sys.argv
    violations = collect_violations()
    by_file = {}
    for f, l in violations:
        by_file.setdefault(f, []).append(l)

    auto, manual = [], []
    for f, lines in sorted(by_file.items()):
        src_lines = open(f).read().splitlines()
        for l in sorted(lines):
            line = src_lines[l - 1]
            fixed = fix_line(line)
            if fixed:
                auto.append((f, l, line, fixed))
            else:
                manual.append((f, l, line.strip()))

    print(f"Total 13.3 violations: {len(violations)} in {len(by_file)} files")
    print(f"Auto-fixable: {len(auto)}  Manual: {len(manual)}")
    for f, l, orig, fixed in auto[:5]:
        print(f"  [{Path(f).name}:{l}]")
        print(f"    - {orig.strip()[:90]}")
        print(f"    + {fixed.splitlines()[0].strip()[:90]} / {fixed.splitlines()[-1].strip()[:60]}")
    if manual:
        print("\n-- Manual cases --")
        for f, l, line in manual[:30]:
            print(f"  {Path(f).name}:{l}: {line[:100]}")

    if apply_changes and auto:
        from collections import defaultdict
        changes = defaultdict(list)
        for f, l, orig, fixed in auto:
            changes[f].append((l, fixed))
        for f, edits in changes.items():
            src = open(f).read()
            lines = src.splitlines()
            for l, fixed in sorted(edits, reverse=True):
                lines[l - 1] = fixed
            open(f, "w").write("\n".join(lines))
            print(f"Applied {len(edits)} fixes to {f}")
        print("DONE — run verify + tests")

if __name__ == "__main__":
    main()
