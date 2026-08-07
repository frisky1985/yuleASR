#!/usr/bin/env python3
"""Fix MISRA C:2023 Rule 15.6 — body of selection/iteration shall be compound.

Transforms single-statement bodies into braced blocks:
  if (cond) stmt;        -> if (cond) { stmt; }
  while (cond) stmt;     -> while (cond) { stmt; }
  while (cond);          -> while (cond) { }   (empty spin-wait)
  for (...) stmt;        -> for (...) { stmt; }
  do stmt; while (cond); -> do { stmt; } while (cond);

Usage: python3 fix_15_6.py [--apply] [--viol FILE] [--files ...]
"""
import json
import re
import sys
from collections import defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from ctok import tokenize  # noqa: E402
from fix_12_1 import line_col_to_offset  # noqa: E402

PROJECT = Path("/Users/stefan/.openclaw/workspace/yuleASR")
REPORT = PROJECT / ".yuleosh" / "reports" / "misra-report.json"

CTRL = {"if", "else", "while", "for", "do"}


def find_ctrl(toks, line):
    """Find first control keyword token on the line."""
    for i, t in enumerate(toks):
        if t["line"] == line and t["text"] in CTRL:
            return i
    return None


def match_paren(toks, open_idx):
    depth = 0
    for k in range(open_idx, len(toks)):
        if toks[k]["text"] == "(":
            depth += 1
        elif toks[k]["text"] == ")":
            depth -= 1
            if depth == 0:
                return k
    return None


def stmt_end(toks, start, line_limit=None):
    """Return token index of statement terminator ';' starting from `start`
    (or end of compound block). Scans at paren/bracket depth 0."""
    depth = 0
    j = start
    while j < len(toks):
        t = toks[j]
        txt = t["text"]
        if txt == "{":
            depth += 1
        elif txt == "}":
            if depth == 0:
                return j - 1 if j > start else j
            depth -= 1
        elif txt == "(":
            depth += 1
        elif txt == ")":
            if depth == 0:
                return j - 1 if j > start else j
            depth -= 1
        elif txt == "[":
            depth += 1
        elif txt == "]":
            depth -= 1
        elif txt == ";":
            if depth == 0:
                return j
        j += 1
    return None


def fix_file(path, violations):
    text = path.read_text()
    toks = tokenize(text)
    if not toks:
        return text, []
    edits = []  # (offset, insert_text)
    skipped = []
    handled_lines = set()
    for f, line, col in violations:
        if f != str(path) or line in handled_lines:
            continue
        ci = find_ctrl(toks, line)
        if ci is None:
            skipped.append(f"{line}:{col} no ctrl")
            continue
        # skip 'else' alone (its body is usually handled with if)
        if toks[ci]["text"] == "else":
            # check if else-if on same line: then body is the if — leave for if
            ci = ci
        kw = toks[ci]["text"]
        if kw == "do":
            # body is after 'do'
            body_start = ci + 1
            # find 'while' after
            wl = None
            depth = 0
            for k in range(body_start, len(toks)):
                if toks[k]["text"] == "{":
                    depth += 1
                elif toks[k]["text"] == "}":
                    depth -= 1
                elif depth == 0 and toks[k]["text"] == "while":
                    wl = k
                    break
            if wl is None:
                skipped.append(f"{line}:{col} do-while not found")
                continue
            # single-statement body between do and while
            body_end = stmt_end(toks, body_start)
            if body_end is None or body_end >= wl:
                skipped.append(f"{line}:{col} do body complex")
                continue
            b_off = line_col_to_offset(text, toks[body_start]["line"], toks[body_start]["col"])
            e_off = line_col_to_offset(text, toks[body_end]["line"], toks[body_end]["col"]) + len(toks[body_end]["text"])
            edits.append((b_off, "{ "))
            edits.append((e_off, " }"))
            handled_lines.add(line)
            continue
        # if/while/for: find '(' after keyword
        j = ci + 1
        if kw == "else":
            # else-if chain: find 'if' next
            while j < len(toks) and toks[j]["text"] != "if":
                j += 1
            if j >= len(toks):
                skipped.append(f"{line}:{col} else w/o if")
                continue
            ci = j
            kw = "if"
            j = ci + 1
        while j < len(toks) and toks[j]["text"] != "(":
            j += 1
        if j >= len(toks):
            skipped.append(f"{line}:{col} no paren")
            continue
        cp = match_paren(toks, j)
        if cp is None:
            skipped.append(f"{line}:{col} unclosed paren")
            continue
        body_start = cp + 1
        # skip comment tokens
        while body_start < len(toks) and toks[body_start]["kind"] in ("comment", "pp"):
            body_start += 1
        if body_start >= len(toks):
            skipped.append(f"{line}:{col} empty after paren")
            continue
        # already compound?
        if toks[body_start]["text"] == "{":
            handled_lines.add(line)
            continue
        # find statement end
        body_end = stmt_end(toks, body_start)
        if body_end is None:
            skipped.append(f"{line}:{col} no stmt end")
            continue
        b_off = line_col_to_offset(text, toks[body_start]["line"], toks[body_start]["col"])
        e_off = line_col_to_offset(text, toks[body_end]["line"], toks[body_end]["col"]) + len(toks[body_end]["text"])
        edits.append((b_off, "{ "))
        edits.append((e_off, " }"))
        handled_lines.add(line)
    return text, edits


def apply_edits(text, edits):
    by_off = defaultdict(list)
    for off, ins in edits:
        by_off[off].append(ins)
    parts = []
    prev = 0
    for off in sorted(by_off):
        if off > prev:
            parts.append(text[prev:off])
        parts.append("".join(by_off[off]))
        prev = off
    parts.append(text[prev:])
    return "".join(parts)


def main():
    args = sys.argv[1:]
    apply_mode = "--apply" in args
    only = []
    viol_file = None
    for a in args:
        if a.startswith("--files="):
            only = a.split("=", 1)[1].split(",")
        if a.startswith("--viol="):
            viol_file = a.split("=", 1)[1]
    if viol_file:
        with open(viol_file) as f:
            vdata = json.load(f)
        viol = [(v["file"], v["line"], v.get("column", 0)) for v in vdata]
    else:
        with open(REPORT) as f:
            d = json.load(f)
        viol = [(v["file"], v["line"], v.get("column", 0)) for v in d["violations_raw"]
                if isinstance(v, dict) and v.get("rule_id") == "misra-c2023-15.6"]
    by_file = defaultdict(list)
    for f, line, col in viol:
        by_file[f].append((f, line, col))
    n_edit = 0
    n_skip = 0
    for f in sorted(by_file):
        p = Path(f)
        if only and p.name not in only and str(p) not in only:
            continue
        text, edits = fix_file(p, by_file[f])
        new_text = apply_edits(text, edits)
        n_edit += len(edits)
        n_skip += len([s for s in edits if s is None])
        if new_text != text and apply_mode:
            p.write_text(new_text)
    print(f"files={len(by_file)} edits={n_edit} skips={n_skip}")
    if not apply_mode:
        print("(dry-run)")


if __name__ == "__main__":
    main()
