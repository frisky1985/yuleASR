#!/usr/bin/env python3
"""Minimal C tokenizer for MISRA mechanical fixes (yuleASR round 2).

Produces a token list: {text, line (1-based), col (1-based), kind}.
Kinds: 'id', 'num', 'str', 'char', 'op', 'punct', 'comment', 'pp' (preprocessor line).
"""
import re

# Multi-char operators first
_OPS3 = ["<<=", ">>=", "..."]
_OPS2 = ["<<", ">>", "<=", ">=", "==", "!=", "&&", "||", "+=", "-=", "*=", "/=",
         "%=", "&=", "|=", "^=", "++", "--", "->"]
_OPS1 = set("+-*/%&|^~!<>=?:.,;()[]{}")

_OP_RE = re.compile(
    r"\.\.\.|<<=|>>=|<<|>>|<=|>=|==|!=|&&|\|\||\+=|-=|\*=|/=|%=|&=|\|=|\^=|\+\+|--|->|"
    r"[+\-*/%&|^~!<>=?:.,;()\[\]{}]"
)
_ID_RE = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")
_NUM_RE = re.compile(
    r"(?:0[xX][0-9a-fA-F]+|0[bB][01]+|0[0-7]*|[0-9]+(?:\.[0-9]*)?|\.[0-9]+)"
    r"(?:[eEpP][+-]?[0-9]+)?(?:[uUlLfF]{1,3})?"
)
_STR_RE = re.compile(r'"(?:[^"\\\n]|\\.)*"')
_CHAR_RE = re.compile(r"'(?:[^'\\\n]|\\.)+'")
_WS = re.compile(r"[ \t\f\v]+")


def tokenize(text):
    """Tokenize C source text. Returns list of dicts."""
    toks = []
    i, n = 0, len(text)
    line, col = 1, 1

    def adv(s):
        nonlocal line, col
        nl = s.count("\n")
        if nl:
            line += nl
            col = len(s.rsplit("\n", 1)[-1]) + 1
        else:
            col += len(s)

    while i < n:
        c = text[i]
        # newline
        if c == "\n":
            adv("\n")
            i += 1
            continue
        # whitespace
        m = _WS.match(text, i)
        if m:
            adv(m.group(0))
            i = m.end()
            continue
        # preprocessor
        if c == "#" and col == 1:
            j = text.find("\n", i)
            if j < 0:
                j = n
            toks.append({"text": text[i:j], "line": line, "col": col, "kind": "pp"})
            adv(text[i:j])
            i = j
            continue
        # line comment
        if text.startswith("//", i):
            j = text.find("\n", i)
            if j < 0:
                j = n
            toks.append({"text": text[i:j], "line": line, "col": col, "kind": "comment"})
            adv(text[i:j])
            i = j
            continue
        # block comment
        if text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            toks.append({"text": text[i:j], "line": line, "col": col, "kind": "comment"})
            adv(text[i:j])
            i = j
            continue
        # string
        m = _STR_RE.match(text, i)
        if m:
            toks.append({"text": m.group(0), "line": line, "col": col, "kind": "str"})
            adv(m.group(0))
            i = m.end()
            continue
        # char
        m = _CHAR_RE.match(text, i)
        if m:
            toks.append({"text": m.group(0), "line": line, "col": col, "kind": "char"})
            adv(m.group(0))
            i = m.end()
            continue
        # number
        m = _NUM_RE.match(text, i)
        if m:
            toks.append({"text": m.group(0), "line": line, "col": col, "kind": "num"})
            adv(m.group(0))
            i = m.end()
            continue
        # identifier
        m = _ID_RE.match(text, i)
        if m:
            toks.append({"text": m.group(0), "line": line, "col": col, "kind": "id"})
            adv(m.group(0))
            i = m.end()
            continue
        # operators
        m = _OP_RE.match(text, i)
        if m:
            t = m.group(0)
            kind = "op" if t not in "()[]{}.,;" else "punct"
            toks.append({"text": t, "line": line, "col": col, "kind": kind})
            adv(t)
            i = m.end()
            continue
        # unknown char
        toks.append({"text": c, "line": line, "col": col, "kind": "other"})
        adv(c)
        i += 1
    return toks


PREC = {
    ",": 0, "=": 1, "+=": 1, "-=": 1, "*=": 1, "/=": 1, "%=": 1,
    "&=": 1, "|=": 1, "^=": 1, "<<=": 1, ">>=": 1,
    "?": 2, ":": 2,
    "||": 3, "&&": 4, "|": 5, "^": 6, "&": 7,
    "==": 8, "!=": 8, "<": 9, ">": 9, "<=": 9, ">=": 9,
    "<<": 10, ">>": 10, "+": 11, "-": 11, "*": 12, "/": 12, "%": 12,
}
UNARY = {"!", "~", "-", "+", "*", "&", "++", "--", "sizeof"}
ASSIGN = {"=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>="}


def prec(tok):
    return PREC.get(tok["text"], 16)


def is_binary_op(tok):
    if tok["kind"] != "op":
        return False
    return tok["text"] in PREC and tok["text"] not in ("?", ":", ",")


def find_token(toks, line, col):
    """Find token index whose (line,col) matches exactly."""
    for i, t in enumerate(toks):
        if t["line"] == line and t["col"] == col:
            return i
    return None
