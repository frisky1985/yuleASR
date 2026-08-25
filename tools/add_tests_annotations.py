#!/usr/bin/env python3
"""yuleASR @tests 标注器 — 为测试文件添加 @tests 注释（测试→代码追溯）。

映射策略（按目录+命名约定推断，不依赖手工映射表）:
1. tests/unit/autosar/<layer>/test_<Module>.c → src/bsw/<layer>/<module>/src/<Module>.c
2. tests/unit/<layer>/test_<Module>.c      → src/bsw/<layer>/<module>/src/<Module>.c
3. tests/unit/<layer>/test_<Module>.c      → src/<layer>/<module>/src/<Module>.c (回退)
4. 匹配失败: 记录 unmatched（不写文件）

用法:
  python3 tools/add_tests_annotations.py            # dry-run
  python3 tools/add_tests_annotations.py --apply    # 实际写入
"""
from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TESTS_DIR = ROOT / "tests"
SRC_DIR = ROOT / "src"

# 跳过非单元测试目录（mock/qemu/集成等 —— 无对应单一源码）
SKIP_DIR_PARTS = {"mock", "mocks", "qemu", "qemu_m33", "qemu_full_stack",
                  "fixtures", "prebuilt", "crypto_benchmark", "hil",
                  "integration", "e2e"}


def _module_dir(layer: str, module: str) -> Path | None:
    """查找 src/bsw/<layer>/<module>/ 或 src/<layer>/<module>/ 目录（大小写不敏感）。"""
    for base in (SRC_DIR / "bsw", SRC_DIR):
        base_layer = base / layer
        if not base_layer.is_dir():
            continue
        # 精确匹配优先
        d = base_layer / module
        if d.is_dir():
            return d
        # 大小写不敏感回退
        low = module.lower()
        for cand in base_layer.iterdir():
            if cand.is_dir() and cand.name.lower() == low:
                return cand
    return None


def resolve_sources(test_file: Path) -> list[str]:
    """按路径+命名推断被测源文件，返回相对项目根的路径列表。"""
    rel = test_file.relative_to(TESTS_DIR)
    parts = list(rel.parts)  # e.g. ['unit','autosar','services','test_Dcm.c']

    name = parts[-1]
    if not name.startswith("test_"):
        return []
    # test_Dcm.c → Dcm ; test_dcm.c → dcm ; test_Mcu_Init.c → Mcu
    stem = name[5:]
    if stem.endswith(".c"):
        stem = stem[:-2]
    # 剥离测试变体后缀: test_bswm_svc.c → bswm, test_icu_new.c → icu
    for suf in ("_svc", "_new", "_test", "_ut", "_verify", "_check"):
        if stem.lower().endswith(suf):
            stem = stem[: -len(suf)]
            break
    # 已知命名漂移 (测试名 → 模块名)
    NAME_DRIFT = {
        "canm": "cannm",        # test_canm.c → CanNm.c
        "lntm": "lintp",        # test_lntm.c → LinTp.c
        "someipsd2": "someipsd",  # test_someipsd2.c → someipsd/
        "cantpsyn": "cantp",    # 归并到 CanTp
        "flsstst": "fls",       # test_flsstst.c → FLS
        "dcm_obd": "dcm",       # test_dcm_obd.c → dcm 模块
    }
    stem = NAME_DRIFT.get(stem.lower(), stem)
    # 聚合契约测试: test_<layer>_api_contracts.c → 映射到该层 (无单一模块)
    LAYER_CONTRACTS = {
        "mcal_api_contracts": "mcal",
        "ecual_api_contracts": "ecual",
        "services_api_contracts": "services",
        "service_api_contracts": "services",
    }
    layer_override = LAYER_CONTRACTS.get(stem.lower())
    if layer_override:
        # 用层目录: src/bsw/<layer>/ 全部源码
        layer = layer_override
        module = ""  # 特殊标记: 匹配层目录内全部源文件
    else:
        module = stem

    # 跳过明确无源码对应的目录
    if any(p in SKIP_DIR_PARTS for p in parts[:-1]):
        return []

    # 定位 layer (layer_override 已设置时跳过 — 聚合契约测试已指定层)
    if not layer_override:
        #   tests/bsw/<layer>/<module>/test_x.c  → layer = parts[1] 后的下一个
        #   tests/unit/autosar/<layer>/...       → layer = 'autosar' 后一个
        #   tests/unit/<layer>/...               → layer = parts[1]
        try:
            ai = parts.index("autosar")
            layer = parts[ai + 1] if len(parts) > ai + 1 else ""
        except ValueError:
            if len(parts) >= 3 and parts[0] == "bsw":
                layer = parts[1]  # tests/bsw/services/... → services
            elif len(parts) == 2 and parts[1].startswith("test_"):
                # tests/<module>/test_x.c → layer 未知, 直接全 src 搜模块
                layer = ""
            elif len(parts) == 2 and parts[0] == "unit" and parts[1].startswith("test_"):
                # tests/unit/test_X.c → layer 未知, 用文件名模块名搜全 src
                layer = ""
            else:
                layer = parts[1] if len(parts) > 1 else ""

    if not layer:
        # tests/<module>/ 顶层结构: 优先用目录名（tests/dcm/ → dcm），
        # 目录名不可用时回退文件名模块名
        dir_module = parts[0].lower()
        mdir = None
        for root, dirs, _files in os.walk(SRC_DIR):
            root_p = Path(root)
            if len(root_p.relative_to(SRC_DIR).parts) > 3:
                dirs[:] = []
                continue
            for d in dirs:
                if d.lower() == dir_module and (root_p / d / "src").is_dir():
                    mdir = root_p / d
                    break
            if mdir is not None:
                break
        if mdir is None:
            # 目录名未命中: 回退文件名模块名
            for root, dirs, _files in os.walk(SRC_DIR):
                root_p = Path(root)
                if len(root_p.relative_to(SRC_DIR).parts) > 3:
                    dirs[:] = []
                    continue
                for d in dirs:
                    if d.lower() == module.lower() and (root_p / d / "src").is_dir():
                        mdir = root_p / d
                        break
                if mdir is not None:
                    break
        if mdir is None:
            return []
    elif not module:
        # 层聚合契约测试: module="" → 匹配 src/bsw/<layer>/ 下全部模块源码
        layer_dir = SRC_DIR / "bsw" / layer
        if not layer_dir.is_dir():
            layer_dir = SRC_DIR / layer
        if not layer_dir.is_dir():
            return []
        src_files = sorted(layer_dir.rglob("*.c"))
        # 排除测试/生成物
        src_files = [f for f in src_files
                     if f.is_file() and not f.name.startswith("test_")
                     and "_test" not in f.stem.lower()]
        if not src_files:
            return []
        # 最多标注前 12 个核心文件
        return [str(f.relative_to(ROOT)) for f in src_files[:12]]
    else:
        mdir = _module_dir(layer, module.lower())
    low = module.lower()

    if mdir is None and layer:
        # 回退: 全 src 下按模块名搜索（大小写不敏感，限制深度避免误配）
        for base in (SRC_DIR,):
            for root, dirs, _files in os.walk(base):
                root_p = Path(root)
                depth = root_p.relative_to(base).parts
                if len(depth) > 3:
                    dirs[:] = []
                    continue
                for d in dirs:
                    if d.lower() == low and (root_p / d / "src").is_dir():
                        mdir = root_p / d
                        break
                if mdir is not None:
                    break
            if mdir is not None:
                break
    if mdir is None:
        return []

    # 候选源文件: 目录 src/ 下与模块名匹配的文件（大小写不敏感）
    src_dir = mdir / "src"
    inc_dir = mdir / "include"
    candidates: list[Path] = []

    def _match_file(d: Path, exts: tuple[str, ...]) -> list[Path]:
        if not d.is_dir():
            return []
        hits = []
        for f in sorted(d.iterdir()):
            if f.is_file() and f.suffix in exts and f.stem.lower() == low:
                hits.append(f)
        return hits

    candidates = _match_file(src_dir, (".c",))
    # 顶层结构（tests/<module>/）目录名定位: 模块目录内全部 .c 均为被测对象
    if not candidates and not layer:
        candidates = [f for f in sorted(src_dir.iterdir())
                      if f.is_file() and f.suffix == ".c"
                      and not f.stem.lower().startswith("test_")]
    # 无 src 匹配时允许 config 文件（*_cfg.c）
    if not candidates:
        for f in sorted(src_dir.iterdir()) if src_dir.is_dir() else []:
            if f.is_file() and f.suffix == ".c" and f.stem.lower().endswith("_cfg") \
               and low in f.stem.lower():
                candidates.append(f)
    candidates += _match_file(inc_dir, (".h",))

    # 去重（同一文件可能多 pattern 命中）
    seen, uniq = set(), []
    for c in candidates:
        key = str(c)
        if key not in seen:
            seen.add(key)
            uniq.append(c)

    return [str(c.relative_to(ROOT)) for c in uniq[:3]]


def add_annotation(filepath: Path, src_files: list[str]) -> bool:
    """插入 @tests 注释（文件头，紧跟 @file 块之后）。返回是否修改。"""
    content = filepath.read_text(encoding="utf-8", errors="replace")
    if "@tests" in content:
        return False  # 已有标注

    # C 源文件用 // 注释（# 会被当预处理器指令）
    is_c = filepath.suffix in (".c", ".h", ".cpp", ".hpp")
    prefix = "//" if is_c else "#"
    annotation = prefix + " @tests " + ("  @tests ".join(src_files))

    lines = content.split("\n")
    insert_idx = 0
    # 跳过 shebang
    if lines and lines[0].startswith("#!"):
        insert_idx = 1
    # 跳过文件头注释块 (/* ... */ 或 // 连续注释)
    i = insert_idx
    if i < len(lines) and lines[i].strip().startswith("/*"):
        for j in range(i, len(lines)):
            if "*/" in lines[j]:
                insert_idx = j + 1
                break
    elif i < len(lines):
        while i < len(lines) and lines[i].strip().startswith(("//", "#", "*")):
            i += 1
        insert_idx = i

    new_lines = lines[:insert_idx] + ["", annotation] + lines[insert_idx:]
    filepath.write_text("\n".join(new_lines), encoding="utf-8")
    return True


def main() -> int:
    ap = argparse.ArgumentParser(description="yuleASR @tests 标注器")
    ap.add_argument("--apply", action="store_true", help="实际写入（默认 dry-run）")
    args = ap.parse_args()

    test_files = sorted(TESTS_DIR.rglob("test_*.c"))
    matched, unmatched = [], []

    for tf in test_files:
        srcs = resolve_sources(tf)
        if srcs:
            matched.append((tf, srcs))
        else:
            unmatched.append(tf)

    print(f"测试文件: {len(test_files)} | 可匹配: {len(matched)} | 未匹配: {len(unmatched)}")
    if not args.apply:
        print("(dry-run — 加 --apply 实际写入)")
        for tf, srcs in matched[:15]:
            print(f"  {tf.relative_to(ROOT)} → {srcs}")
        if unmatched:
            print(f"\n未匹配样例 ({min(10, len(unmatched))}):")
            for tf in unmatched[:10]:
                print(f"  - {tf.relative_to(ROOT)}")
        return 0

    added = 0
    for tf, srcs in matched:
        if add_annotation(tf, srcs):
            added += 1
            print(f"  + {tf.relative_to(ROOT)} → {srcs}")
    print(f"\n完成: {added} 个测试文件已标注, {len(matched) - added} 已有, {len(unmatched)} 未匹配")
    return 0


if __name__ == "__main__":
    sys.exit(main())
