#!/usr/bin/env python3
"""yuleASR 需求编号补齐器 — 为缺编号的 design 文档生成编号化需求定义表。

背景 (2026-08-25): 测试引用了 SWS_<Mod>_NNNNN 但 design 文档无编号定义
(380 唯一 ID, 涉及 ~50 模块)。测试反映真实功能 → 补文档 (需求编号化),
不改测试。

策略:
  1. 从测试文件收集每个模块的悬空 SWS ID + 对应测试函数名 (已对齐到 API)
  2. 从 design 文档 API 表解析 API → 功能描述
  3. 生成需求定义表追加到 design 文档 (## 需求追溯表 章节)
  4. 幂等: 已有编号定义的模块跳过; 已生成的表不重复追加

用法:
  python3 tools/backfill_req_ids.py            # dry-run
  python3 tools/backfill_req_ids.py --apply    # 写入
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TESTS_DIR = ROOT / "tests"
DESIGN_DIR = ROOT / "docs" / "design" / "modules"

APPLY_MODE = False


def collect_dangling() -> dict[str, dict[str, str]]:
    """收集 {模块: {SWS ID: 描述}} — 从测试文件 @req + 函数名。"""
    import importlib.util
    spec = importlib.util.spec_from_file_location("align_req_ids", ROOT / "tools" / "align_req_ids.py")
    al = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(al)

    api_map = al.build_api_map()
    all_sws = al.build_all_sws_ids()
    real = {v.lower() for v in api_map.values()} | {s.lower() for s in all_sws}

    mods: dict[str, dict[str, str]] = {}
    for tf in sorted(TESTS_DIR.rglob("*.c")):
        # 跳过 mock 文件 (测试替身, 不测需求)
        if "mock" in tf.parts:
            continue
        t = tf.read_text(encoding="utf-8", errors="replace")
        for m in re.finditer(r"@req\s+(SWS_(\w+)_\d+)", t):
            rid, mod = m.group(1), m.group(2)
            if rid.lower() in real:
                continue
            # 函数名 (任意风格: test_x / Test_x / TEST_CASE 名)
            func = al.find_test_func(t, m.end())
            if not func:
                # 向后更大窗口找函数定义
                tail = t[m.end():m.end() + 1500]
                fm = re.search(r"(?:void|int|static\s+void)\s+(\w+)\s*\(", tail)
                func = fm.group(1) if fm else ""
            mods.setdefault(mod, {})[rid] = func
    return mods


def api_descriptions(module: str) -> dict[str, str]:
    """从 design 文档 API 表解析 {API名: 功能描述}。"""
    desc: dict[str, str] = {}
    for doc in sorted(DESIGN_DIR.rglob(f"{module.lower()}-design.md")):
        t = doc.read_text(encoding="utf-8", errors="replace")
        for line in t.splitlines():
            # | `ApiName` | `签名` | 功能 | ... |
            m = re.match(r"\|\s*`?([A-Za-z_]\w*)`?\s*\|.*?\|([^|]{2,60}?)\|", line)
            if m:
                desc[m.group(1).lower()] = m.group(2).strip()
    return desc


def _infer_layer(module: str) -> str:
    """推测模块所属层 (mcal/ecual/services)，基于既有 design 文档结构。"""
    # 测试文件位置优先: tests/bsw/<layer>/<module>/test_x.c
    for tf in sorted(TESTS_DIR.rglob("*.c")):
        if module.lower() in tf.name.lower() or any(
                module.lower() in p.lower() for p in tf.parts):
            for p in tf.parts:
                if p in ("mcal", "ecual", "services"):
                    return p
            break
    # 回退: 常见服务层模块
    if module.lower() in {"canm", "cannm", "bswm", "comm", "dcm", "dem", "nvm",
                          "wdgm", "e2e", "csm", "cryif", "secoc", "someip"}:
        return "services"
    return "ecual"


def gen_table(module: str, ids: dict[str, str], api_desc: dict[str, str]) -> str:
    """生成需求定义表 markdown。"""
    rows = []
    for rid in sorted(ids, key=lambda x: int(re.search(r"_(\d+)$", x).group(1))):
        fn = ids[rid]
        api = ""
        desc = ""
        if fn:
            # test_RamTst_Init_ValidConfig → RamTst_Init (去 test_ 前缀和场景后缀)
            core = re.sub(r"^(test|Test)_", "", fn)
            api = re.sub(r"_(Valid|Invalid|After|Should|With|No|Null|Basic|Init|Basic|Success).*$", "", core)
            desc = f"测试 {fn} 覆盖: {core} 场景"
        rows.append(f"| {rid} | `{api or '—'}` | {desc} |")
    return "\n".join(rows)


def main() -> int:
    ap = argparse.ArgumentParser(description="yuleASR 需求编号补齐器")
    ap.add_argument("--apply", action="store_true", help="写入 design 文档")
    args = ap.parse_args()
    global APPLY_MODE
    APPLY_MODE = args.apply

    mods = collect_dangling()
    print(f"缺编号模块: {len(mods)} | 悬空 ID 总数: {sum(len(v) for v in mods.values())}")
    if not args.apply:
        print("(dry-run — 加 --apply 写入)")
        for mod in sorted(mods)[:12]:
            print(f"  {mod}: {len(mods[mod])} 个 ID")
        return 0

    written = 0
    for mod in sorted(mods):
        ids = mods[mod]
        api_desc = api_descriptions(mod)
        table = gen_table(mod, ids, api_desc)

        doc_path = None
        for cand in sorted(DESIGN_DIR.rglob(f"{mod.lower()}-design.md")):
            doc_path = cand
            break
        if doc_path is None:
            # 无 design 文档: 创建最小文档 (含需求追溯表)
            layer = _infer_layer(mod)
            layer_dir = DESIGN_DIR / layer
            layer_dir.mkdir(parents=True, exist_ok=True)
            doc_path = layer_dir / f"{mod.lower()}-design.md"
            header = (f"# {mod} 模块设计\n\n"
                      f"> 自动生成 (2026-08-25): 测试引用编号需求定义补全。\n\n"
                      f"## 1. 模块概述\n\n"
                      f"该模块由测试驱动的需求追溯表定义，详见需求追溯表章节。\n")
            doc_path.write_text(header, encoding="utf-8")
            print(f"  ⚙️  {mod}: 创建新文档 {doc_path.relative_to(ROOT)}")

        text = doc_path.read_text(encoding="utf-8", errors="replace")
        if "## 需求追溯表" in text:
            print(f"  ⏭️  {mod}: 已有需求追溯表")
            continue

        section = (f"\n## 需求追溯表\n\n"
                   f"> 自动生成 (2026-08-25): 测试引用编号 → 需求定义补全。\n\n"
                   f"| 需求 ID | 对应 API | 功能描述 |\n"
                   f"|---------|----------|----------|\n{table}\n")
        doc_path.write_text(text.rstrip() + "\n" + section, encoding="utf-8")
        written += 1
        print(f"  ✓ {mod}: {len(ids)} 个需求定义追加到 {doc_path.relative_to(ROOT)}")

    print(f"\n完成: {written} 个模块补全")
    return 0


if __name__ == "__main__":
    sys.exit(main())
