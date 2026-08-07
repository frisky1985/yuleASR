#!/usr/bin/env python3
"""Fix MISRA C:2023 Rule 10.4 — operands of same essential type category.

Mechanisms (behavior-preserving):
  1. `unsigned_expr OP int_literal`        -> suffix literal with U
  2. `signed_expr OP 0U/1U/...`  (==/!=)   -> drop U suffix (identical semantics)
  3. `signed_expr OP U-literal`  (ordering)-> cast signed expr to unsigned
     matching the literal's suffix width
  4. macro operand with numeric body       -> fix #define body (add/keep U)
  5. two-identifier mismatch               -> cast signed side to unsigned
     counterpart (from declaration scan)

Usage: python3 fix_10_4.py [--apply] [--files ...] [--dryjson FILE]
"""
import json
import re
import sys
from collections import defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from ctok import tokenize, prec, is_binary_op, UNARY  # noqa: E402
from fix_12_1 import span_left, span_right, line_col_to_offset  # noqa: E402

PROJECT = Path("/Users/stefan/.openclaw/workspace/yuleASR")
REPORT = PROJECT / ".yuleosh" / "reports" / "misra-report.json"

OP_SET = {"+", "-", "*", "/", "%", "&", "|", "^", "+=", "-=",
          "==", "!=", "<", ">", "<=", ">=", ":"}
CMP = {"==", "!=", "<", ">", "<=", ">="}

UNSIGNED_TYPES = {"uint8", "uint16", "uint32", "uint64", "uint8_t", "uint16_t",
                  "uint32_t", "uint64_t", "unsigned", "boolean", "size_t",
                  "uint8_least", "uint16_least", "uint32_least", "uint64_least",
                  "uint8_fast", "uint16_fast", "uint32_fast", "uint64_fast",
                  "uintptr_t", "u8", "u16", "u32", "u64", "Std_ReturnType",
                  "char8_t", "uint8_least_t"}
SIGNED_TYPES = {"sint8", "sint16", "sint32", "sint64", "int8_t", "int16_t",
                "int32_t", "int64_t", "int", "signed", "char", "short",
                "long", "int8", "int16", "int32", "int64", "s8", "s16",
                "s32", "s64", "float32", "float64", "float", "double",
                "boolean_1", "sint8_least", "sint16_least", "sint32_least",
                "sint64_least", "sint8_t", "sint16_t", "sint32_t", "sint64_t",
                "int_least8_t", "int_least16_t", "int_least32_t", "int_least64_t"}
FLOAT_TYPES = {"float", "double", "float32", "float64", "long double"}

NUM_RE = re.compile(
    r"(?:0[xX][0-9a-fA-F]+|0[bB][01]+|[0-9]+)(?:[eEpP][+-]?[0-9]+)?([uUlLfF]{0,3})")
# standalone numeric literal: not preceded/followed by identifier chars
STANDALONE_NUM_RE = re.compile(r"(?<![A-Za-z0-9_])(?:0[xX][0-9a-fA-F]+|0[bB][01]+|[0-9]+)(?:[eEpP][+-]?[0-9]+)?([uUlLfF]{0,3})(?![A-Za-z0-9_])")
FLOAT_RE = re.compile(r"^(\d*\.\d+|\d+\.\d*|\d+[eE][+-]?\d+|[0-9]+[fF])")


def has_u_suffix(num):
    m = NUM_RE.match(num)
    return bool(m) and "u" in m.group(1).lower()


def add_u(num):
    m = NUM_RE.match(num)
    if not m or "u" in m.group(1).lower():
        return num
    suf = m.group(1)
    # insert U before trailing L/LL
    if "l" in suf.lower():
        return num[: len(num) - len(suf)] + "U" + suf
    return num + "U"


def del_u(num):
    m = NUM_RE.match(num)
    if not m:
        return num
    suf = m.group(1)
    i = suf.lower().find("u")
    if i < 0:
        return num
    return num[: len(num) - len(suf) + i] + suf[:i] + suf[i + 1:]


def cast_for_literal(lit):
    """unsigned cast type matching a U-suffixed literal's width."""
    suf = ""
    m = NUM_RE.match(lit)
    if m:
        suf = m.group(1).lower()
    if "ll" in suf:
        return "unsigned long long"
    if "l" in suf:
        return "unsigned long"
    return "unsigned int"


def is_float_literal(num):
    if num.lower().startswith(("0x", "0b")):
        return False
    if FLOAT_RE.match(num):
        return True
    # f/F suffix = float; l/L on decimal without '.'/'e' is long (not float)
    return num.lower().endswith("f")


def body_is_unsigned(body):
    """True if macro body contains a U-suffixed numeric literal (or is all-U)."""
    if not body:
        return False
    nums = NUM_RE.findall(body)
    if not nums:
        return False
    return all("u" in n.lower() for n in NUM_RE.findall(body))


# ---------------- enum constant scan ----------------
ENUM_BLOCK_RE = re.compile(r"\benum\s*(?:\{[^}]*\}|\w+\s*\{[^}]*\})", re.DOTALL)
ENUM_ITEM_RE = re.compile(r"\b([A-Za-z_]\w*)\s*(?:=\s*[^,}]+)?")


def scan_enums(text):
    """Return set of enum constant names (essential type: signed int)."""
    names = set()
    for m in ENUM_BLOCK_RE.finditer(text):
        block = m.group(0)
        body = block[block.find('{'):]
        for im in ENUM_ITEM_RE.finditer(body):
            nm = im.group(1)
            if nm and nm not in ("enum",):
                names.add(nm)
    return names


# ---------------- declaration scan ----------------
DECL_RE = re.compile(
    r"^\s*(?:static\s+|extern\s+|const\s+|volatile\s+|register\s+|restrict\s+)*"
    r"(?P<type>[A-Za-z_]\w*(?:\s+[A-Za-z_]\w*)?(?:\s*\*)*)\s+"
    r"(?P<name>[A-Za-z_]\w*)\s*(?:\[[^\]]*\])?\s*(?:=|;|,|\))"
)


def type_category(type_str):
    t = type_str.strip()
    if "*" in t:
        return "p"
    tl = t.lower()
    for key in (tl,):
        if key in UNSIGNED_TYPES:
            return "u"
        if key in SIGNED_TYPES:
            return "s"
        if key in FLOAT_TYPES:
            return "f"
    # two-word types like "unsigned int" / "unsigned long"
    words = t.split()
    if words and words[0] in ("unsigned",):
        return "u"
    if words and words[0] in ("signed",):
        return "s"
    return None


def scan_decls(text):
    """Return {name: category('u'/'s'/'p'/'f'/None)} from simple declarations."""
    out = {}
    for m in DECL_RE.finditer(text):
        name = m.group("name")
        if name in ("if", "for", "while", "switch", "return", "sizeof"):
            continue
        cat = type_category(m.group("type"))
        if cat:
            out[name] = cat
    # struct/union members: TYPE name; inside braces — DECL_RE already matches
    # function params
    for m in re.finditer(
            r"\(([^()]*?)\b([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?\s*[,)]", text):
        pass  # handled by DECL_RE when on one line
    return out


# ---------------- macro scan ----------------
DEFINE_RE = re.compile(r"#\s*define\s+([A-Za-z_]\w*)(?:[ \t]*\([^)\n]*\))?[ \t]+(.*)$", re.MULTILINE)


def scan_defines(text):
    out = {}
    for m in DEFINE_RE.finditer(text):
        out[m.group(1)] = m.group(2).strip()
    return out


def macro_body_edit(body, define_map=None):
    """Return (new_body, embedded_macro_names) or (None, []) if unchanged.

    Adds U to numeric literals in the body; reports embedded macro
    identifiers that themselves need fixing (resolved by caller).
    """
    edits = []
    embedded = []
    seen_pos = set()
    for m in STANDALONE_NUM_RE.finditer(body):
        if m.start() in seen_pos:
            continue
        seen_pos.add(m.start())
        tok = m.group(0)
        if FLOAT_RE.match(tok) and "." in tok:
            continue
        if not has_u_suffix(tok):
            edits.append((m.end(), "U"))
    if define_map:
        for m in re.finditer(r"\b([A-Za-z_]\w*)\b", body):
            nm = m.group(1)
            if nm in define_map and nm not in embedded:
                nb = macro_body_edit(define_map[nm])
                if nb[0] is not None:
                    embedded.append(nm)
    if not edits and not embedded:
        return None, []
    fixed = list(body)
    for pos, ins in sorted(edits, reverse=True):
        fixed.insert(pos, ins)
    return "".join(fixed), embedded


# ---------------- main fixer ----------------
def find_op_token(toks, line, col):
    """Find the flagged operator token near (line,col)."""
    cands = [t for t in toks if t["line"] == line and t["col"] <= col and t["col"] >= col - 4]
    # prefer op tokens
    for t in sorted(cands, key=lambda t: -t["col"]):
        if t["kind"] == "op" and t["text"] in OP_SET:
            return t
    # fallback: any op token on the line
    for t in toks:
        if t["line"] == line and t["kind"] == "op" and t["text"] in OP_SET:
            return t
    return None


def operand_kind(toks, s, e):
    """Classify operand span: ('lit', text) / ('macro', name) / ('expr', None)."""
    if s > e:
        return ("expr", None)
    # single token
    if s == e:
        t = toks[s]
        if t["kind"] == "num":
            return ("lit", t["text"])
        if t["kind"] == "id":
            return ("id", t["text"])
        return ("expr", None)
    # -NUM / +NUM
    if e - s == 1 and toks[s]["kind"] == "op" and toks[s]["text"] in ("-", "+") and toks[s + 1]["kind"] == "num":
        return ("lit", toks[s]["text"] + toks[s + 1]["text"])
    # (NUM) or (0x..)
    if e - s >= 2 and toks[s]["text"] == "(" and toks[e]["text"] == ")":
        inner = [t for t in toks[s + 1:e] if t["kind"] != "comment"]
        if len(inner) == 1 and inner[0]["kind"] == "num":
            return ("lit", inner[0]["text"])
        if len(inner) == 2 and inner[0]["kind"] == "op" and inner[0]["text"] in ("-", "+") and inner[1]["kind"] == "num":
            return ("lit", inner[0]["text"] + inner[1]["text"])
    # single id inside parens e.g. (SOMEIP_MAX_...) -> id
    if e - s >= 2 and toks[s]["text"] == "(" and toks[e]["text"] == ")":
        inner = [t for t in toks[s + 1:e] if t["kind"] != "comment"]
        if len(inner) == 1 and inner[0]["kind"] == "id":
            return ("id", inner[0]["text"])
    return ("expr", None)


def fix_file(path, violations, define_map, enum_consts=None):
    """Return (new_text, actions) where actions are dicts describing fixes."""
    text = path.read_text()
    toks = tokenize(text)
    if not toks:
        return text, []
    decls = scan_decls(text)
    defines = scan_defines(text)
    if enum_consts is None:
        enum_consts = scan_enums(text)
    pos_map = {}
    for idx, t in enumerate(toks):
        pos_map.setdefault((t["line"], t["col"]), idx)
    actions = []  # (kind, start_off, end_off, insert_text, meta)
    macro_ops = []  # dicts {name, new_body, embedded, use_file}

    def tok_off(t):
        return line_col_to_offset(text, t["line"], t["col"])

    def span_txt(toks, s, e):
        if s > e:
            return ""
        return text[tok_off(toks[s]): tok_off(toks[e]) + len(toks[e]["text"])]

    def span_first_tok(s, e):
        """first non-comment/pp token in span"""
        for kk in range(s, e + 1):
            if toks[kk]["kind"] not in ("comment", "pp"):
                return kk
        return s

    for f, line, col in violations:
        if f != str(path):
            continue
        op = find_op_token(toks, line, col)
        if op is None:
            # position points at a macro id (or macro call arg) whose BODY has the mismatch
            macro_fix = False
            # collect macro ids on the same line (function-like calls or object-like ids)
            line_macros = []
            for t in toks:
                if t["line"] == line and t["kind"] == "id":
                    nm = t["text"]
                    if nm in defines or nm in define_map:
                        body = defines.get(nm) or define_map.get(nm)
                        line_macros.append((nm, body, t["col"]))
            # prefer macro at/after the flagged col, else any on the line
            line_macros.sort(key=lambda x: (0 if x[2] >= col - 1 else 1, x[2]))
            merged_map = dict(define_map)
            merged_map.update(defines)
            for nm, body, _ in line_macros:
                nb, emb = macro_body_edit(body, merged_map)
                if nb or emb:
                    emb_s = f" emb={','.join(emb)}" if emb else ""
                    actions.append(("MACRO", 0, 0, nb or body,
                                    f"{line}:{col} pos-macro={nm} {body!r}->{nb or body!r}{emb_s}"))
                    macro_ops.append({"name": nm, "new_body": nb or body, "orig_body": body,
                                      "embedded": emb, "use_file": str(path)})
                    macro_fix = True
                    break
            if not macro_fix:
                actions.append(("SKIP-notok", 0, 0, "", f"{line}:{col} no op token"))
            continue
        idx = pos_map.get((op["line"], op["col"]))
        if idx is None:
            actions.append(("SKIP-pos", 0, 0, "", f"{line}:{col}->{op['line']}:{op['col']}"))
            continue
        optxt = op["text"]
        p = prec(op)
        if optxt not in OP_SET:
            actions.append(("SKIP-opset", 0, 0, "", f"{line}:{col} op={optxt}"))
            continue
        ls, le = span_left(toks, idx)
        rs, re_ = span_right(toks, idx)
        lk, lv = operand_kind(toks, ls, le)
        rk, rv = operand_kind(toks, rs, re_)
        meta = f"L={span_txt(toks,ls,le)[:40]!r} R={span_txt(toks,rs,re_)[:40]!r} op={optxt}"
        # ---- case 1/2/3: one side is a literal ----
        if lk == "lit" and rk == "lit":
            # both literals: fix the first non-U one (if any)
            for side in ("l", "r"):
                t = lv if side == "l" else rv
                if is_float_literal(t):
                    continue
                if not has_u_suffix(t):
                    s, e = (ls, le) if side == "l" else (rs, re_)
                    num_tok = toks[s] if e - s == 0 else toks[e] if toks[e]["kind"] == "num" else toks[s + 1] if toks[s]["text"] == "(" else toks[s]
                    off = tok_off(num_tok) + len(num_tok["text"])
                    actions.append(("LIT+U", off, off, "U", f"{meta} {side}={t}"))
                    break
            continue
        lit_side, lit_txt, other_side, other_s, other_e = None, None, None, None, None
        if lk == "lit":
            lit_side, lit_txt = "l", lv
            other_side, other_s, other_e = "r", rs, re_
        elif rk == "lit":
            lit_side, lit_txt = "r", rv
            other_side, other_s, other_e = "l", ls, le
        if lit_side:
            if is_float_literal(lit_txt):
                actions.append(("SKIP-float", 0, 0, "", meta))
                continue
            # macro call vs literal: cast call to unsigned + U-suffix literal
            other_s = rs if other_side == "r" else ls
            other_e = re_ if other_side == "r" else le
            fs = span_first_tok(other_s, other_e)
            if (toks[fs]["kind"] == "id" and fs + 1 <= other_e
                    and toks[fs + 1]["text"] == "("
                    and (toks[fs]["text"] in defines or toks[fs]["text"] in define_map)):
                s_off = tok_off(toks[fs])
                e_off = tok_off(toks[other_e]) + len(toks[other_e]["text"])
                actions.append(("CAST", s_off, e_off, "uint32_t", f"{meta} macrocall-lit cast-call"))
                if not has_u_suffix(lit_txt):
                    num_tok = toks[ls] if le - ls == 0 else toks[le] if toks[le]["kind"] == "num" else toks[ls + 1]
                    off = tok_off(num_tok) + len(num_tok["text"])
                    actions.append(("LIT+U", off, off, "U", f"{meta} lit={lit_txt}"))
                continue
            if not has_u_suffix(lit_txt):
                # add U at literal end
                if lit_side == "l":
                    num_tok = toks[ls] if le - ls == 0 else toks[le] if toks[le]["kind"] == "num" else toks[ls + 1]
                else:
                    num_tok = toks[rs] if re_ - rs == 0 else toks[re_] if toks[re_]["kind"] == "num" else toks[rs + 1]
                off = tok_off(num_tok) + len(num_tok["text"])
                actions.append(("LIT+U", off, off, "U", f"{meta} lit={lit_txt}"))
            else:
                # U literal; other side signed
                if optxt in ("==", "!="):
                    # drop U (identical semantics)
                    if lit_side == "l":
                        num_tok = toks[ls] if le - ls == 0 else toks[le] if toks[le]["kind"] == "num" else toks[ls + 1]
                    else:
                        num_tok = toks[rs] if re_ - rs == 0 else toks[re_] if toks[re_]["kind"] == "num" else toks[rs + 1]
                    s_off = tok_off(num_tok)
                    e_off = s_off + len(num_tok["text"])
                    actions.append(("LIT-U", s_off, e_off, del_u(num_tok["text"]), f"{meta} lit={lit_txt}"))
                else:
                    # cast other side to unsigned
                    if other_s is not None and other_e >= other_s:
                        cast = cast_for_literal(lit_txt)
                        s_off = tok_off(toks[other_s])
                        e_off = tok_off(toks[other_e]) + len(toks[other_e]["text"])
                        actions.append(("CAST", s_off, e_off, cast, f"{meta} cast={cast}"))
                    else:
                        actions.append(("SKIP-nocast", 0, 0, "", meta))
            continue
        # ---- special: char literal vs unsigned/signed ----
        char_side = None
        for sname, s, e, k, v in [("l", ls, le, lk, lv), ("r", rs, re_, rk, rv)]:
            if s == e and toks[s]["kind"] == "char":
                char_side = (sname, s, e, toks[s]["text"])
                break
        if char_side:
            sname, s, e, ctext = char_side
            fs = span_first_tok(s, e)
            actions.append(("CAST", tok_off(toks[fs]), tok_off(toks[e]) + len(toks[e]["text"]),
                            "uint8_t", f"{meta} char={ctext} cast-char-u8"))
            continue
        # ---- special: case labels with enum constants ----
        if optxt == ":" and toks[ls]["kind"] == "id":
            nm = toks[ls]["text"]
            if nm in enum_consts:
                actions.append(("CAST", tok_off(toks[ls]), tok_off(toks[le]) + len(toks[le]["text"]),
                                "uint32_t", f"{meta} case-enum={nm}"))
                continue
        # ---- special: enum constant vs other (cast enum side to unsigned) ----
        enum_side = None
        for sname, s, e, k, v in [("l", ls, le, lk, lv), ("r", rs, re_, rk, rv)]:
            if k == "id" and v in enum_consts:
                enum_side = (sname, s, e, v)
                break
        if enum_side:
            sname, s, e, nm = enum_side
            other = "r" if sname == "l" else "l"
            os_, oe_ = (rs, re_) if other == "r" else (ls, le)
            # other side unsigned or numeric? cast enum side to unsigned to match
            ok_, ov_ = (rk, rv) if other == "r" else (lk, lv)
            if ok_ in ("lit", "expr", "id"):
                actions.append(("CAST", tok_off(toks[s]), tok_off(toks[e]) + len(toks[e]["text"]),
                                "uint32_t", f"{meta} enum={nm} cast-enum-u32"))
                continue
        # ---- case 4/5: identifiers / macros / exprs ----
        sides = [("l", ls, le, lk, lv), ("r", rs, re_, rk, rv)]
        # resolve macro bodies
        resolved = {}
        for sname, s, e, k, v in sides:
            if k == "id":
                body = None
                if v in defines:
                    body = defines[v]
                elif v in define_map:
                    body = define_map[v]
                resolved[sname] = ("id", v, body)
            elif k == "expr":
                resolved[sname] = ("expr", None, None)
            else:
                resolved[sname] = (k, v, None)
        lm, lv2, lb = resolved["l"]
        rm, rv2, rb = resolved["r"]

        def cast_action(side_s, side_e, cast, why):
            fs = span_first_tok(side_s, side_e)
            s_off = tok_off(toks[fs])
            e_off = tok_off(toks[side_e]) + len(toks[side_e]["text"])
            actions.append(("CAST", s_off, e_off, cast, f"{meta} {why}"))

        # macro vs id/expr
        macro_side = None
        if lm == "id" and lb is not None:
            macro_side = "l"
        elif rm == "id" and rb is not None:
            macro_side = "r"
        if macro_side:
            body = lb if macro_side == "l" else rb
            newbody, emb = macro_body_edit(body, define_map)
            if newbody:
                mname = lv2 if macro_side == "l" else rv2
                emb_s = f" emb={','.join(emb)}" if emb else ""
                actions.append(("MACRO", 0, 0, newbody,
                                f"{meta} macro={mname} body={body!r}->{newbody!r}{emb_s}"))
                macro_ops.append({"name": mname, "new_body": newbody, "orig_body": body,
                                  "embedded": emb, "use_file": str(path)})
            elif body_is_unsigned(body):
                # macro already unsigned; other side signed -> cast other side
                # to the macro literal's width (preserves semantics for int64 too)
                other_sn = "r" if macro_side == "l" else "l"
                other_k, other_v, other_b = resolved[other_sn]
                os_, oe_ = (rs, re_) if other_sn == "r" else (ls, le)
                lit = next(iter(NUM_RE.findall(body)), "")
                cast = cast_for_literal(lit) if lit else "uint32_t"
                cast_action(os_, oe_, cast, f"cast-other-{cast.replace(' ','_')}")
            else:
                # macro already unsigned; other side is signed -> cast other side
                other_sn = "r" if macro_side == "l" else "l"
                other_k, other_v, other_b = resolved[other_sn]
                os_, oe_ = (rs, re_) if other_sn == "r" else (ls, le)
                if other_k == "expr" and other_v is None:
                    # function call / composite: cast to uint8_t (Std_ReturnType style)
                    if optxt in ("==", "!="):
                        cast_action(os_, oe_, "uint8_t", "cast-call-u8")
                    else:
                        cast_action(os_, oe_, "uint32_t", "cast-call-u32")
                else:
                    actions.append(("SKIP-macro", 0, 0, "",
                                    f"{meta} macro={lv2 if macro_side=='l' else rv2}"))
            continue
        # function-like macro call (e.g. COMM_IS_INITIALIZED())
        macrocall_side = None
        for sname, s, e, k, v in sides:
            if k == "expr":
                # check first token: id followed by '(' => macro call?
                fs = span_first_tok(s, e)
                t0 = toks[fs]
                if (t0["kind"] == "id" and fs + 1 <= e and toks[fs + 1]["text"] == "("):
                    body = defines.get(t0["text"]) or define_map.get(t0["text"])
                    if body is not None:
                        macrocall_side = (sname, s, e, t0["text"], body)
                        break
        if macrocall_side:
            sn, s, e, mname, body = macrocall_side
            other_sn = "r" if sn == "l" else "l"
            other_k, other_v, other_e2 = (rk, rv, re_) if other_sn == "r" else (lk, lv, le)
            other_s2 = rs if other_sn == "r" else ls
            # macro call vs literal: cast call result to unsigned (semantics-safe),
            # and U-suffix the literal if needed (e.g. MQTT_OID_CMP(...) == 0)
            if other_k == "lit" and not is_float_literal(other_v):
                fs = span_first_tok(s, e)
                cast_action(s, e, "uint32_t", f"cast-macrocall-{mname}")
                if not has_u_suffix(other_v):
                    num_tok = toks[other_s2] if other_e2 - other_s2 == 0 else toks[other_e2] if toks[other_e2]["kind"] == "num" else toks[other_s2 + 1]
                    off = tok_off(num_tok) + len(num_tok["text"])
                    actions.append(("LIT+U", off, off, "U", f"{meta} macrocall-lit={other_v}"))
                continue
            newbody, emb = macro_body_edit(body, define_map)
            if newbody:
                emb_s = f" emb={','.join(emb)}" if emb else ""
                actions.append(("MACRO", 0, 0, newbody,
                                f"{meta} macro-call={mname} body={body!r}->{newbody!r}{emb_s}"))
                macro_ops.append({"name": mname, "new_body": newbody,
                                  "embedded": emb, "use_file": str(path)})
            else:
                actions.append(("SKIP-macro", 0, 0, "", f"{meta} macro-call={mname}"))
            continue
        # composite containing a macro/literal mismatch: fix inner macro or literal
        def find_inner(s, e):
            """look for macro ids / non-U literals inside span; return action"""
            for kk in range(s, e + 1):
                t = toks[kk]
                if t["kind"] == "id":
                    body = defines.get(t["text"]) or define_map.get(t["text"])
                    if body is not None:
                        nb, emb2 = macro_body_edit(body, define_map)
                        if nb:
                            return ("MACRO", t["text"], body, nb, emb2)
                elif t["kind"] == "num":
                    if not is_float_literal(t["text"]) and not has_u_suffix(t["text"]):
                        off = tok_off(t) + len(t["text"])
                        return ("LIT+U", t["text"], None, off)
            return None
        inner = find_inner(ls, le) or find_inner(rs, re_)
        if inner:
            if inner[0] == "MACRO":
                _, mname, body, nb, emb2 = inner
                emb_s = f" emb={','.join(emb2)}" if emb2 else ""
                actions.append(("MACRO", 0, 0, nb,
                                f"{meta} inner-macro={mname} {body!r}->{nb!r}{emb_s}"))
                macro_ops.append({"name": mname, "new_body": nb,
                                  "embedded": emb2, "use_file": str(path)})
            else:
                _, numtxt, _, off = inner
                actions.append(("LIT+U", off, off, "U", f"{meta} inner-lit={numtxt}"))
            continue
        # two identifiers/exprs: cast signed side
        cats = {}
        for sname, s, e, k, v in sides:
            if k == "id":
                cats[sname] = decls.get(v, None)
            else:
                cats[sname] = None
        cl, cr = cats["l"], cats["r"]
        if cl == "s" and cr == "u":
            cast_action(ls, le, "(uint32_t)", "cast-left")
        elif cr == "s" and cl == "u":
            cast_action(rs, re_, "(uint32_t)", "cast-right")
        # one side enum (signed), other side unsigned id/expr
        elif cl == "s" and cr is None and (rk in ("lit", "expr") or (rk == "id" and decls.get(rv) == "u")):
            cast_action(ls, le, "(uint32_t)", "cast-left-u")
        elif cr == "s" and cl is None and (lk in ("lit", "expr") or (lk == "id" and decls.get(lv) == "u")):
            cast_action(rs, re_, "(uint32_t)", "cast-right-u")
        else:
            actions.append(("SKIP-unk", 0, 0, "", meta))
    return text, actions, macro_ops


def apply_actions(text, actions):
    """Apply text-edit actions (LIT+U, LIT-U, CAST) to file text."""
    edits = []  # (offset, kind, payload)
    for kind, s, e, ins, meta in actions:
        if kind == "LIT+U":
            edits.append((s, "ins", "U"))
        elif kind == "LIT-U":
            edits.append((s, "repl", (e - s, ins)))
        elif kind == "CAST":
            edits.append((s, "ins", "(" + ins + ")("))
            edits.append((e, "ins", ")"))
    if not edits:
        return text, 0
    by_off = defaultdict(list)
    for off, kind, payload in edits:
        by_off[off].append((kind, payload))
    parts = []
    prev = 0
    for off in sorted(by_off):
        if off > prev:
            parts.append(text[prev:off])
        evs = by_off[off]
        # insertions first (order: CAST before U), then replacements
        for kind, payload in sorted(evs, key=lambda x: 0 if x[0] == "ins" else 1):
            if kind == "ins":
                parts.append(payload)
            elif kind == "repl":
                n, repl = payload
                parts.append(repl)
                prev = off + n
        if all(k == "ins" for k, _ in evs):
            prev = off
    parts.append(text[prev:])
    return "".join(parts), len(edits)


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
                if isinstance(v, dict) and v.get("rule_id") == "misra-c2023-10.4"]
    # global define map: name -> body (search src headers once)
    define_map = {}
    enum_consts = set()
    scan_roots = [(PROJECT / "src"), (PROJECT / "include")]
    for root in scan_roots:
        for p in sorted(root.rglob("*.h")):
            try:
                define_map.update(scan_defines(p.read_text()))
                enum_consts |= scan_enums(p.read_text())
            except Exception:
                pass
        for p in sorted(root.rglob("*.c")):
            try:
                enum_consts |= scan_enums(p.read_text())
            except Exception:
                pass
    by_file = defaultdict(list)
    for f, line, col in viol:
        by_file[f].append((f, line, col))
    total_actions = 0
    report = defaultdict(list)
    macro_fixes = {}  # name -> {new_body, embedded, use_file, orig_body}
    for f in sorted(by_file):
        p = Path(f)
        if only and p.name not in only and str(p) not in only:
            continue
        text, actions, macro_ops = fix_file(p, by_file[f], define_map, enum_consts)
        for a in actions:
            report[a[0]].append(a[4])
        new_text, n_edits = apply_actions(text, actions)
        total_actions += n_edits
        if new_text != text and apply_mode:
            p.write_text(new_text)
        for op in macro_ops:
            macro_fixes.setdefault(op["name"], op)
    # expand embedded macro fixes
    queue = list(macro_fixes.values())
    while queue:
        op = queue.pop(0)
        for nm in op["embedded"]:
            if nm not in macro_fixes:
                body = define_map.get(nm)
                # embedded macro may be defined in the same use file
                if body is None:
                    try:
                        body = scan_defines(Path(op["use_file"]).read_text()).get(nm)
                    except Exception:
                        body = None
                if body is not None:
                    nb, emb = macro_body_edit(body, define_map)
                    if nb:
                        macro_fixes[nm] = {"name": nm, "new_body": nb, "orig_body": body,
                                           "embedded": emb, "use_file": op["use_file"]}
                        queue.append(macro_fixes[nm])
    # apply macro fixes in defining files
    applied_macros = 0
    if apply_mode and macro_fixes:
        search_paths = list((PROJECT / "src").rglob("*.h")) + \
                       list((PROJECT / "src").rglob("*.c")) + \
                       list((PROJECT / "third_party").rglob("*.h"))
        for name, op in macro_fixes.items():
            use_file = op["use_file"]
            new_body = op["new_body"]
            orig_body = op.get("orig_body")
            # prefer file whose define body == orig_body; else use_file; else first match
            cand = []
            for p in sorted(search_paths):
                try:
                    t = p.read_text()
                except Exception:
                    continue
                m = re.search(r"#\s*define\s+" + re.escape(name) + r"(?:[ \t]*\([^)\n]*\))?[ \t]+(.*)$",
                              t, re.MULTILINE)
                if m:
                    cand.append((p, m))
            p, m = None, None
            if orig_body:
                for cp, cm in cand:
                    if cm.group(1).strip() == orig_body:
                        p, m = cp, cm
                        break
            if p is None:
                for cp, cm in cand:
                    if str(cp) == use_file:
                        p, m = cp, cm
                        break
            if p is None and cand:
                p, m = cand[0]
            if p is None:
                print(f"  !! MACRO {name} defining file not found (used in {use_file})")
                continue
            t = p.read_text()
            # apply to every matching #define line in the file (handles #ifdef/#else variants)
            new = t
            hits = 0
            for m in re.finditer(r"(#\s*define\s+" + re.escape(name) + r"(?:[ \t]*\([^)\n]*\))?[ \t]+)(.*)$",
                                 new, re.MULTILINE):
                nb2, _ = macro_body_edit(m.group(2).strip(), define_map)
                if nb2 and nb2 != m.group(2).strip():
                    new = new[:m.start(2)] + nb2 + new[m.end(2):]
                    hits += 1
            if new != t:
                p.write_text(new)
                applied_macros += 1
            elif hits == 0:
                print(f"  !! MACRO {name}: no body edit matched in {p}")
    print(f"files={len(by_file)} edit-ops={total_actions} macros={applied_macros}/{len(macro_fixes)}")
    for k in sorted(report):
        print(f"  {k}: {len(report[k])}")
    if not apply_mode:
        print("(dry-run)")


if __name__ == "__main__":
    main()
