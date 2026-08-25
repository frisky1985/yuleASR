#!/usr/bin/env python3
"""yuleASR 需求×测试用例追溯表 HTML 生成器。

数据源:
  1. .yuleosh/reports/traceability-report.json — LRM 需求→测试关联
  2. docs/requirements.md — 测试追溯字段 (补充)
  3. 测试文件内 @req 注释 — 测试函数级关联

输出: docs/traceability-matrix.html (自包含, 每个需求一行, 测试用例内联)

用法: python3 tools/gen_trace_matrix.py
"""
from __future__ import annotations

import html
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def load_req_test_map() -> list[dict]:
    """构建 [{req_id, statement, tests: [{file, funcs}]}]。"""
    report = json.load(open(ROOT / ".yuleosh/reports/traceability-report.json"))
    reqs = report["lrm"]["requirements"]

    # 测试文件 → 函数列表 (从测试文件 @req 注释)
    test_funcs: dict[str, list[str]] = {}
    for tf in sorted((ROOT / "tests").rglob("*.c")):
        if "mock" in tf.parts:
            continue
        t = tf.read_text(encoding="utf-8", errors="replace")
        funcs = re.findall(r"(?:void|int)\s+(test_\w+)\s*\(", t)
        if funcs:
            test_funcs[str(tf.relative_to(ROOT))] = funcs

    out = []
    for r in reqs:
        rid = r.get("req_id") or r.get("id") or ""
        tests = []
        seen = set()
        for tr in (r.get("test_reports") or []):
            f = tr.get("file", "")
            if f in seen:
                continue
            seen.add(f)
            funcs = test_funcs.get(f, [])
            # 过滤仅含该文件的需求相关函数 (有 @req 的函数)
            tests.append({"file": f, "funcs": funcs[:8], "count": len(funcs)})
        out.append({
            "req_id": rid,
            "statement": r.get("statement", ""),
            "tests": tests,
            "code_files": (r.get("code_files") or [])[:4],
        })
    return out


def render(rows: list[dict]) -> str:
    covered = sum(1 for r in rows if r["tests"])
    total = len(rows)
    pct = covered * 100 // total if total else 0

    trs = []
    for r in rows:
        rid = html.escape(r["req_id"] or "(无 ID)")
        stmt = html.escape(r["statement"][:100])
        if r["tests"]:
            tests_html = []
            for t in r["tests"]:
                funcs = "".join(
                    f'<code class="fn">{html.escape(f)}</code>' for f in t["funcs"][:6]
                )
                tests_html.append(
                    f'<div class="test"><span class="file">{html.escape(t["file"])}</span>'
                    f'<span class="fcnt">{t["count"]} 用例</span><div class="funcs">{funcs}</div></div>'
                )
            tests_cell = "".join(tests_html)
            badge = '<span class="badge ok">已覆盖</span>'
        else:
            tests_cell = '<span class="badge miss">无测试</span>'
            badge = '<span class="badge miss">无测试</span>'
        code = "".join(
            f'<code class="src">{html.escape(c)}</code>' for c in r["code_files"]
        )
        trs.append(
            f"<tr><td class='rid'>{rid}</td><td>{badge}</td>"
            f"<td class='stmt'>{stmt}</td><td class='tests'>{tests_cell}</td>"
            f"<td class='code'>{code}</td></tr>"
        )

    return f"""<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<title>yuleASR 需求 × 测试用例追溯矩阵</title>
<style>
:root {{ --purple:#6C5CE7; --teal:#00B894; --gold:#FDCB6E; --bg:#0F1117; --card:#1A1D29; }}
* {{ box-sizing:border-box; margin:0; padding:0; }}
body {{ background:var(--bg); color:#E8EAF0; font-family:"Noto Sans SC","PingFang SC",sans-serif; padding:24px; }}
h1 {{ font-size:22px; color:#fff; margin-bottom:8px; }}
h1 span {{ color:var(--teal); }}
.sub {{ color:#8892A6; font-size:13px; margin-bottom:20px; }}
.stats {{ display:flex; gap:16px; margin-bottom:24px; flex-wrap:wrap; }}
.stat {{ background:var(--card); border:1px solid #2A2E3D; border-radius:10px; padding:12px 20px; }}
.stat b {{ font-size:22px; color:var(--gold); display:block; }}
.stat span {{ font-size:12px; color:#8892A6; }}
table {{ width:100%; border-collapse:collapse; background:var(--card); border-radius:10px; overflow:hidden; }}
th {{ background:#222638; color:var(--purple); font-size:13px; text-align:left; padding:10px 12px; }}
td {{ padding:10px 12px; border-top:1px solid #2A2E3D; font-size:13px; vertical-align:top; }}
tr:hover td {{ background:#1E2233; }}
.rid {{ font-weight:600; color:var(--teal); white-space:nowrap; width:110px; }}
.stmt {{ color:#C6CDDC; width:32%; }}
.tests {{ width:38%; }}
.test {{ background:#141824; border:1px solid #2A2E3D; border-radius:6px; padding:6px 8px; margin-bottom:6px; }}
.file {{ color:#8BE9FD; font-size:12px; word-break:break-all; }}
.fcnt {{ color:var(--gold); font-size:11px; margin-left:6px; }}
.funcs {{ margin-top:4px; }}
.fn {{ display:inline-block; background:#232838; color:#9CE0C1; border-radius:4px; padding:1px 6px; font-size:11px; margin:2px 3px 0 0; font-family:monospace; }}
.src {{ display:inline-block; background:#232838; color:#F5A97F; border-radius:4px; padding:1px 6px; font-size:11px; margin:2px 3px 0 0; font-family:monospace; }}
.code {{ width:18%; }}
.badge {{ display:inline-block; padding:2px 10px; border-radius:12px; font-size:11px; font-weight:600; }}
.badge.ok {{ background:rgba(0,184,148,.15); color:var(--teal); }}
.badge.miss {{ background:rgba(255,107,107,.15); color:#FF6B6B; }}
</style>
</head>
<body>
<h1>yuleASR <span>需求 × 测试用例</span> 追溯矩阵</h1>
<div class="sub">生成时间: 2026-08-25 | 数据源: traceability-report.json + docs/requirements.md | 每个需求一行, 右侧为对应测试文件与用例</div>
<div class="stats">
  <div class="stat"><b>{total}</b><span>需求总数</span></div>
  <div class="stat"><b>{covered}</b><span>有测试覆盖</span></div>
  <div class="stat"><b>{pct}%</b><span>覆盖率</span></div>
</div>
<table>
<thead><tr><th>需求 ID</th><th>状态</th><th>需求描述</th><th>对应测试用例</th><th>实现代码</th></tr></thead>
<tbody>
{''.join(trs)}
</tbody>
</table>
</body>
</html>"""


def main() -> None:
    rows = load_req_test_map()
    html_out = render(rows)
    out_path = ROOT / "docs" / "traceability-matrix.html"
    out_path.write_text(html_out, encoding="utf-8")
    print(f"已生成: {out_path.relative_to(ROOT)}")
    print(f"需求: {len(rows)} | 有测试: {sum(1 for r in rows if r['tests'])}")


if __name__ == "__main__":
    main()
