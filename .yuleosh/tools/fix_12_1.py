#!/usr/bin/env python3
"""Fix MISRA C:2023 Rule 12.1 — operator precedence made explicit.

Strategy: tokenize each file; for every flagged operator (from the report),
find its left/right operand spans with a precedence-aware scan, then wrap
each composite operand (root precedence > operator precedence) in parens.

Nested/overlapping wraps are merged by offset-level paren insertion.

Usage: python3 fix_12_1.py [--apply] [--check] [--files FILE...]
"""
import json
import sys
from collections import defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from ctok import tokenize, prec, is_binary_op, UNARY  # noqa: E402

PROJECT = Path("/Users/stefan/.openclaw/workspace/yuleASR")
REPORT = PROJECT / ".yuleosh" / "reports" / "misra-report.json"

KEYWORDS_STOP = {"if", "while", "for", "do", "switch", "case", "return", "else",
                 "sizeof", "typedef", "struct", "union", "enum", "goto", "break",
                 "continue", "default", "static", "const", "volatile", "extern"}


def collect_violations():
    with open(REPORT) as f:
        d = json.load(f)
    out = []
    for v in d["violations_raw"]:
        if isinstance(v, dict) and v.get("rule_id") == "misra-c2023-12.1":
            out.append((v["file"], v["line"], v.get("column", 0)))
    return out


def line_col_to_offset(text, line, col):
    """1-based line/col -> byte offset."""
    lines = text.split("\n")
    off = 0
    for i in range(line - 1):
        off += len(lines[i]) + 1
    return off + (col - 1)


def span_left(toks, i):
    """Return token index range [start..i-1] of the left operand of toks[i]."""
    op = toks[i]
    p = prec(op)
    depth = 0
    j = i - 1
    start = i
    while j >= 0:
        t = toks[j]
        txt = t["text"]
        if t["kind"] in ("comment", "pp"):
            j -= 1
            continue
        if t["kind"] in ("str", "char"):
            j -= 1
            continue
        if txt in (")", "]", "}"):
            depth += 1
            j -= 1
            continue
        if txt in ("(", "[", "{"):
            if depth > 0:
                depth -= 1
                j -= 1
                continue
            break  # enclosing group start: operand ends after it
        if depth > 0:
            j -= 1
            continue
        if txt in (";", ","):
            break
        if txt in KEYWORDS_STOP:
            break
        if t["kind"] == "op":
            tp = prec(t)
            if txt in ("?", ":"):
                break
            if tp <= p:
                break
            if txt in UNARY and tp >= 16:
                # prefix unary: part of a further-left operand — stop only if
                # this unary is itself an operand root (e.g. '!' of '!a')
                # '!' is always unary; '-'/'+'/'*'/'&' could be binary but
                # binary ones have tp<=12 so already handled above.
                if txt in ("!", "~", "sizeof"):
                    j -= 1
                    continue
                # ambiguous prefix ( -, +, *, &, ++, -- ): treat as part of
                # operand (continue) — safe since root prec check follows.
                j -= 1
                continue
            # other binary op with tp > p: continue (part of operand)
            j -= 1
            continue
        j -= 1
    start = j + 1
    return start, i - 1


def span_right(toks, i):
    """Return token index range [i+1..end] of the right operand of toks[i]."""
    op = toks[i]
    p = prec(op)
    depth = 0
    j = i + 1
    n = len(toks)
    end = i
    while j < n:
        t = toks[j]
        txt = t["text"]
        if t["kind"] in ("comment", "pp"):
            j += 1
            continue
        if t["kind"] in ("str", "char"):
            j += 1
            continue
        if txt in ("(", "[", "{"):
            depth += 1
            j += 1
            continue
        if txt in (")", "]", "}"):
            if depth > 0:
                depth -= 1
                j += 1
                continue
            break
        if depth > 0:
            j += 1
            continue
        if txt in (";", ","):
            break
        if txt in ("?", ":"):
            break
        if txt in KEYWORDS_STOP:
            break
        if t["kind"] == "op":
            tp = prec(t)
            if tp <= p:
                break
            # higher precedence binary op or unary: continue (part of operand)
            j += 1
            continue
        j += 1
    end = j - 1
    return i + 1, end


def root_prec(toks, s, e):
    """Lowest precedence of binary ops at depth 0 within toks[s..e]."""
    if s > e:
        return 16
    depth = 0
    best = 16
    for k in range(s, e + 1):
        txt = toks[k]["text"]
        if txt in ("(", "[", "{"):
            depth += 1
        elif txt in (")", "]", "}"):
            depth -= 1
        elif depth == 0 and toks[k]["kind"] == "op":
            tp = prec(toks[k])
            if txt in ("?", ":"):
                if tp < best:
                    best = tp
            elif is_binary_op(toks[k]):
                if tp < best:
                    best = tp
    return best


def fix_file(path, violations):
    """Return (new_text, fixed_count, skipped)."""
    text = path.read_text()
    toks = tokenize(text)
    if not toks:
        return text, 0, 0
    # map (line,col) -> token index
    pos_map = {}
    for idx, t in enumerate(toks):
        pos_map.setdefault((t["line"], t["col"]), idx)
    wraps = []  # (start_off, end_off)
    skipped = 0
    for f, line, col in violations:
        if f != str(path):
            continue
        idx = pos_map.get((line, col))
        if idx is None:
            skipped += 1
            continue
        tok = toks[idx]
        if not is_binary_op(tok) or prec(tok) > 12:
            skipped += 1
            continue
        p = prec(tok)
        ls, le = span_left(toks, idx)
        rs, re = span_right(toks, idx)
        for s, e in ((ls, le), (rs, re)):
            if s > e:
                continue
            rp = root_prec(toks, s, e)
            if rp > p and rp <= 12:
                # ensure we don't wrap a single token (e.g. unary result)
                if e - s >= 1 or root_prec(toks, s, e) != 16:
                    start_off = line_col_to_offset(text, toks[s]["line"], toks[s]["col"])
                    last = toks[e]
                    end_off = line_col_to_offset(text, last["line"], last["col"]) + len(last["text"])
                    if end_off > start_off:
                        wraps.append((start_off, end_off))
    if not wraps:
        return text, 0, skipped
    # merge: drop spans fully inside another span that starts at same token
    # (keeps outer only) — outer wrap covers inner violations' needs partially;
    # we keep ALL spans (nested parens are correct).
    # group events by offset
    events = defaultdict(list)
    for s, e in wraps:
        events[s].append(("open", s, e))
        events[e].append(("close", s, e))
    parts = []
    prev = 0
    for off in sorted(events):
        if off > prev:
            parts.append(text[prev:off])
        evs = events[off]
        opens = [e for e in evs if e[0] == "open"]
        closes = [e for e in evs if e[0] == "close"]
        # opens: outer first = span ending later first
        opens.sort(key=lambda x: -x[2])
        # closes: inner first = span starting later first
        closes.sort(key=lambda x: -x[1])
        parts.append("".join("(" for _ in opens))
        parts.append("".join(")" for _ in closes))
        prev = off
    parts.append(text[prev:])
    return "".join(parts), len(wraps), skipped


def main():
    args = sys.argv[1:]
    apply_mode = "--apply" in args
    only = []
    for a in args:
        if a.startswith("--files="):
            only = a.split("=", 1)[1].split(",")
    viol = collect_violations()
    by_file = defaultdict(list)
    for f, line, col in viol:
        by_file[f].append((f, line, col))
    total_fixed = 0
    total_skip = 0
    nfiles = 0
    for f in sorted(by_file):
        p = Path(f)
        if only and p.name not in only and str(p) not in only:
            continue
        new_text, fixed, skipped = fix_file(p, by_file[f])
        total_fixed += fixed
        total_skip += skipped
        nfiles += 1
        if new_text != p.read_text() and apply_mode:
            p.write_text(new_text)
    print(f"files={nfiles} wraps={total_fixed} skipped={total_skip}")
    print("(dry-run)" if not apply_mode else "(applied)")


if __name__ == "__main__":
    main()
