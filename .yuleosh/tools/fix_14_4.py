#!/usr/bin/env python3
"""Fix MISRA C:2023 Rule 14.4 — controlling expressions essentially Boolean.

Transforms:
  if (X)        -> if (X != 0U) / (X != 0) / (X != NULL)   (type-aware)
  if (a & b)    -> if ((a & b) != 0U)
  while (x--)   -> while ((x--) != 0U)
  while (1U)    -> for (;;)
  TEST_ASSERT(TRUE, ...) -> TEST_ASSERT(TRUE != FALSE, ...)

Usage: python3 fix_14_4.py [--apply] [--viol FILE] [--files ...]
"""
import json
import re
import sys
from collections import defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from ctok import tokenize  # noqa: E402
from fix_10_4 import scan_decls, type_category, line_col_to_offset  # noqa: E402

PROJECT = Path("/Users/stefan/.openclaw/workspace/yuleASR")
REPORT = PROJECT / ".yuleosh" / "reports" / "misra-report.json"

BITWISE = {"&", "|", "^", "<<", ">>"}


def build_global_type_maps():
    """identifier/function -> 'u'/'s'/'p'/'f' across src .c+.h"""
    decls = {}
    funcs = {}
    for p in sorted(list((PROJECT / "src").rglob("*.c")) +
                    list((PROJECT / "src").rglob("*.h"))):
        try:
            t = p.read_text()
        except Exception:
            continue
        decls.update(scan_decls(t))
        # function return types:  TYPE name(  or TYPE *name(
        for m in re.finditer(
                r"(?m)^\s*(?:static\s+|extern\s+|inline\s+)*"
                r"(?P<type>[A-Za-z_]\w*(?:\s+[A-Za-z_]\w*)?(?:\s*\*)*)\s+"
                r"(?P<name>[A-Za-z_]\w*)\s*\(", t):
            nm = m.group("name")
            if nm not in ("if", "for", "while", "switch", "return", "sizeof"):
                funcs.setdefault(nm, type_category(m.group("type")))
    return decls, funcs


def find_control_expr(toks, line):
    """Return (expr_start_tok_idx, expr_end_tok_idx, close_paren_idx) for the
    if/while/for control expression on `line`, or None."""
    # find control keyword on the line
    ki = None
    for i, t in enumerate(toks):
        if t["line"] == line and t["text"] in ("if", "while", "for", "switch"):
            ki = i
            break
    if ki is None:
        return None
    # find '(' after keyword
    j = ki + 1
    while j < len(toks) and toks[j]["text"] != "(":
        j += 1
    if j >= len(toks):
        return None
    # match parens
    depth = 0
    k = j
    while k < len(toks):
        if toks[k]["text"] == "(":
            depth += 1
        elif toks[k]["text"] == ")":
            depth -= 1
            if depth == 0:
                return j + 1, k - 1, k
        k += 1
    return None


def expr_category(toks, s, e, decls, funcs):
    """Best-effort essential category of the control expression."""
    txts = [t["text"] for t in toks[s:e + 1] if t["kind"] != "comment"]
    if not txts:
        return "u"
    # pointer-ish identifiers
    for t in toks[s:e + 1]:
        if t["kind"] == "id":
            name = t["text"]
            if decls.get(name) == "p":
                return "p"
            if decls.get(name) == "s":
                return "s"
            if decls.get(name) == "u":
                return "u"
            if name in funcs:
                c = funcs[name]
                if c in ("u", "s", "p", "f"):
                    return c
    # member chains: look up the member name
    m = re.match(r"(?:.*?(?:->|\.))?([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?$", "".join(txts))
    if m:
        name = m.group(1)
        if decls.get(name) == "p":
            return "p"
        if decls.get(name) in ("u", "s"):
            return decls[name]
    return "u"  # default: unsigned (AUTOSAR bias)


def fix_file(path, violations, decls, funcs):
    text = path.read_text()
    toks = tokenize(text)
    if not toks:
        return text, []
    edits = []  # (offset, insert_text)
    skipped = []
    for f, line, col in violations:
        if f != str(path):
            continue
        line_src = text.split("\n")[line - 1] if line - 1 < len(text.split("\n")) else ""
        # TEST_ASSERT special: TEST_ASSERT(TRUE, ...) -> TEST_ASSERT(TRUE != FALSE, ...)
        m = re.search(r"TEST_ASSERT\(\s*TRUE\s*,", line_src)
        if m and "TEST_ASSERT" in line_src:
            off = line_col_to_offset(text, line, m.start() + len("TEST_ASSERT("))
            edits.append((off, "TRUE != FALSE, "))
            continue
        ce = find_control_expr(toks, line)
        if ce is None:
            skipped.append(f"{line}:{col} no control expr")
            continue
        s, e, cp = ce
        if s > e:
            skipped.append(f"{line}:{col} empty expr")
            continue
        expr_txt = "".join(t["text"] for t in toks[s:e + 1] if t["kind"] != "comment")
        top_ops = [t for t in toks[s:e + 1]
                   if t["kind"] == "op" and t["text"] in BITWISE and t["text"] not in (">", "<")]
        cat = expr_category(toks, s, e, decls, funcs)
        # while (1U) / while (1)
        kw = None
        for t in toks:
            if t["line"] == line and t["text"] in ("while", "if"):
                kw = t["text"]
                break
        if kw == "while" and expr_txt in ("1U", "1", "(1U)", "(1)", "1u"):
            # replace 'while (X)' -> 'for (;;)'
            wi = None
            for i, t in enumerate(toks):
                if t["line"] == line and t["text"] == "while":
                    wi = i
                    break
            if wi is not None:
                w_off = line_col_to_offset(text, toks[wi]["line"], toks[wi]["col"])
                close_off = line_col_to_offset(text, toks[cp]["line"], toks[cp]["col"]) + 1
                edits.append(("REPL", w_off, close_off))
            continue
        # choose comparison
        if cat == "p":
            comp = " != NULL"
        elif cat == "s":
            comp = " != 0"
        else:
            comp = " != 0U"
        # wrap expression in parens
        s_off = line_col_to_offset(text, toks[s]["line"], toks[s]["col"])
        e_off = line_col_to_offset(text, toks[e]["line"], toks[e]["col"]) + len(toks[e]["text"])
        if toks[s]["text"] == "(" and toks[e]["text"] == ")":
            # already fully parenthesized: insert comparison after close paren
            edits.append((e_off, comp))
        else:
            edits.append((s_off, "("))
            edits.append((e_off, ")" + comp))
    return text, edits


def apply_edits(text, edits):
    # 'REPL' entries: (marker, w_off, close_off) — handled separately
    repls = [e for e in edits if e[0] == "REPL"]
    simple = [(o, ins) for o, ins in edits if isinstance(o, int)]
    by_off = defaultdict(list)
    for off, ins in simple:
        by_off[off].append(ins)
    parts = []
    prev = 0
    for off in sorted(by_off):
        if off > prev:
            parts.append(text[prev:off])
        parts.append("".join(by_off[off]))
        prev = off
    parts.append(text[prev:])
    out = "".join(parts)
    for marker, w_off, close_off in repls:
        # replace 'while (1U)' span with 'for (;;)'
        out = out[:w_off] + "for (;;)" + out[close_off:]
    return out


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
                if isinstance(v, dict) and v.get("rule_id") == "misra-c2023-14.4"]
    decls, funcs = build_global_type_maps()
    by_file = defaultdict(list)
    for f, line, col in viol:
        by_file[f].append((f, line, col))
    n_edit = 0
    n_skip = 0
    for f in sorted(by_file):
        p = Path(f)
        if only and p.name not in only and str(p) not in only:
            continue
        text, edits = fix_file(p, by_file[f], decls, funcs)
        new_text = apply_edits(text, edits)
        n_edit += sum(1 for e in edits if isinstance(e[0], int) and e[1])
        n_skip += sum(1 for e in edits if e[0] == "SKIP")
        if new_text != text and apply_mode:
            p.write_text(new_text)
    print(f"files={len(by_file)} edits={n_edit} skips={n_skip}")
    if not apply_mode:
        print("(dry-run)")


if __name__ == "__main__":
    main()
