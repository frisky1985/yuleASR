#!/usr/bin/env python3
"""yuleASR @req 标注器 — 为未标注源文件添加 @req 注释（代码→需求追溯）。

策略:
1. 按源文件所在目录定位模块 (src/bsw/<layer>/<module>/src/X.c → module)
2. 从 docs/design/modules/<layer>/<module>-design.md 提取该模块的 SWS_* 需求 ID
3. 文件头部块后插入 /** @req SWS_<Module>_NNNNN */ (取模块前 3 个 SWS ID)
4. 无设计文档时: 用模块名生成 SHALL_<MODULE> 占位需求

用法:
  python3 tools/add_req_annotations.py            # dry-run
  python3 tools/add_req_annotations.py --apply    # 实际写入
  python3 tools/add_req_annotations.py --layer mcal --apply  # 仅某层
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC_DIR = ROOT / "src"
DESIGN_DIR = ROOT / "docs" / "design" / "modules"

# 聚合/无文档文件 → 归属模块 (src 路径子串 → 设计文档模块名)
FALLBACK_MODULE = {
    "mcal/mcu/src/Mcal.c": "mcu",
    "boot": "boot",
    "cdd": "cdd",
    "os": "os",
    "classic": "classic",
}


def _module_of(src_file: Path) -> str:
    """从路径推断模块名 (src/bsw/<layer>/<module>/src/X.c → <module>)。"""
    rel = src_file.relative_to(SRC_DIR)
    parts = rel.parts
    if len(parts) >= 3 and parts[0] == "bsw":
        return parts[2]
    # src/<layer>/<module>/... 结构 → parts[1] (目录名)
    if len(parts) >= 2:
        return parts[1] if "." not in parts[1] else parts[0]
    return ""


def _design_doc(module: str, layer: str) -> Path | None:
    """定位模块设计文档 (大小写不敏感)。"""
    # 直接路径
    p = DESIGN_DIR / layer / f"{module.lower()}-design.md"
    if p.is_file():
        return p
    # 全局搜索
    for doc in DESIGN_DIR.rglob(f"{module.lower()}-design.md"):
        return doc
    return None


def _extract_sws_ids(doc: Path, module: str, limit: int = 3) -> list[str]:
    """从设计文档提取模块的 SWS ID (去重, 保序)。"""
    text = doc.read_text(encoding="utf-8", errors="replace")
    # 匹配 SWS_<Module>_NNNNN (要求数字后缀, 排除纯模块名)
    pattern = re.compile(rf"SWS_{re.escape(module)}_(\d{{3,}})", re.IGNORECASE)
    seen: list[str] = []
    for m in pattern.finditer(text):
        rid = m.group(0)
        if rid.lower() not in {s.lower() for s in seen}:
            seen.append(rid)
        if len(seen) >= limit:
            break
    return seen


def resolve_req_ids(src_file: Path) -> list[str]:
    """为源文件解析 @req ID 列表。"""
    module = _module_of(src_file)
    if not module:
        return []

    # 定位 layer
    rel = src_file.relative_to(SRC_DIR)
    layer = ""
    parts = rel.parts
    if len(parts) >= 3 and parts[0] == "bsw":
        layer = parts[1]

    # 聚合文件回退
    rel_str = str(rel)
    for key, fallback in FALLBACK_MODULE.items():
        if key in rel_str:
            module = fallback
            break

    doc = _design_doc(module, layer)
    if doc:
        ids = _extract_sws_ids(doc, module)
        if ids:
            return ids

    # 无文档: SHALL_<MODULE> 占位
    return [f"SHALL_{module.upper()}"]


def add_annotation(filepath: Path, req_ids: list[str]) -> bool:
    """文件头注释块内插入 @req 行。返回是否修改。"""
    content = filepath.read_text(encoding="utf-8", errors="replace")
    if "@req" in content:
        return False  # 已有标注

    annotations = " ".join(f"@req {rid}" for rid in req_ids)

    lines = content.split("\n")
    insert_idx = 0
    # 标准头块 /* ... */ : 在块结束后插入自闭合单行注释 (保证无论插在哪都合法)
    if lines and lines[0].strip().startswith("/*"):
        for j in range(len(lines)):
            if "*/" in lines[j]:
                insert_idx = j + 1  # 块结束后的下一行
                break
        line = f"/* {annotations} */"
    # // 行注释头 : 在连续 // 行之后插入 // @req 行
    elif lines and lines[0].strip().startswith("//"):
        j = 0
        while j < len(lines) and lines[j].strip().startswith("//"):
            j += 1
        insert_idx = j
        line = f"// {annotations}"
    else:
        # 无头块: 文件首行前插入
        insert_idx = 0
        line = f"/* {annotations} */"

    new_lines = lines[:insert_idx] + [line, ""] + lines[insert_idx:]
    filepath.write_text("\n".join(new_lines), encoding="utf-8")
    return True


def main() -> int:
    ap = argparse.ArgumentParser(description="yuleASR @req 标注器")
    ap.add_argument("--apply", action="store_true", help="实际写入（默认 dry-run）")
    ap.add_argument("--layer", default="", help="仅处理该层 (mcal/ecual/services/boot/cdd/os/classic)")
    args = ap.parse_args()

    # 收集未标注源文件
    targets: list[Path] = []
    for src_file in sorted(SRC_DIR.rglob("*.c")):
        rel = src_file.relative_to(SRC_DIR)
        parts = rel.parts
        # 排除: legacy/测试/生成物
        if any(p in ("legacy", "test", "tests") for p in parts):
            continue
        if src_file.name.startswith("test_") or "_test" in src_file.stem:
            continue
        if "CMakeFiles" in str(rel):
            continue
        # 层过滤
        if args.layer:
            if len(parts) >= 2 and parts[0] == "bsw" and parts[1] != args.layer:
                continue
            if len(parts) >= 1 and parts[0] != "bsw" and parts[0] != args.layer:
                continue
        if "@req" in src_file.read_text(encoding="utf-8", errors="replace"):
            continue
        targets.append(src_file)

    print(f"未标注源文件: {len(targets)}")

    if not args.apply:
        print("(dry-run — 加 --apply 实际写入)")
        for f in targets[:15]:
            print(f"  {f.relative_to(ROOT)} → {resolve_req_ids(f)[:2]}")
        return 0

    added = 0
    for f in targets:
        ids = resolve_req_ids(f)
        if not ids:
            continue
        if add_annotation(f, ids):
            added += 1
            print(f"  + {f.relative_to(ROOT)} → {ids[:2]}")
    print(f"\n完成: {added} 个文件已标注 (仍有 {len(targets)-added} 个)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
