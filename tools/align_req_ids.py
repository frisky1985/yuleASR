#!/usr/bin/env python3
"""yuleASR @req ID 对齐器 v2 — 将测试文件悬空 @req ID 映射到 design 真实 SWS ID。

背景 (2026-08-25 审计): 测试自创编号段 (SWS_Adc_00201) + 伪 ID
(MCU_CLOCK_001) 共 ~608 个悬空引用。

3 类映射:
  1. 真实 ID (design 文档存在) → 跳过
  2. SWS 悬空段 (SWS_<Mod>_NNNNN 但 design 无此 ID) → 按模块+函数名匹配
  3. 伪 ID (<MOD>_<FUNC>_NNN) → 按模块前缀定位 design 文档, 函数名匹配

匹配维度 (宽松→严格):
  a. 函数名精确 = API 名 (test_adc_init → Adc_Init)
  b. 函数名前缀匹配 API (test_init_deinit → Adc_Init)
  c. 函数名包含 API 名或反之

用法: python3 tools/align_req_ids.py [--apply]
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TESTS_DIR = ROOT / "tests"
DESIGN_DIR = ROOT / "docs" / "design" / "modules"

# dry-run 模式 (默认 True; --apply 时 False)。控制 align_file 是否写盘。
APPLY_MODE = False


def build_api_map() -> dict[str, str]:
    """解析全部 design 文档 → {API名: SWS_ID}。"""
    api_map: dict[str, str] = {}
    for doc in sorted(DESIGN_DIR.rglob("*-design.md")):
        text = doc.read_text(encoding="utf-8", errors="replace")
        for line in text.splitlines():
            # API 表行: | ApiName | ... | SWS_Mod_00001 | (API 名可带反引号)
            m = re.match(r"\|\s*`?([A-Za-z_]\w*)`?\s*\|.*\|\s*(SWS_\w+_\d+)\s*\|", line)
            if m:
                api_map.setdefault(m.group(1).lower(), m.group(2))
    return api_map


def find_test_func(text: str, pos: int) -> str:
    """从 @req 位置向后找最近的可匹配标识 (函数名 / @coverage API 列表)。"""
    tail = text[pos:pos + 600]
    # 1. @coverage Mcu_InitClock, Mcu_ConfigureClock → API 列表 (最可靠)
    m = re.search(r"@coverage\s+([A-Za-z_][\w, ]*)", tail)
    if m:
        apis = [a.strip() for a in m.group(1).split(",") if a.strip()]
        if apis:
            return apis[0]  # 取第一个 API
    # 2. TEST_CASE(mcu_init_clock_valid)
    m = re.search(r"TEST_CASE\s*\(\s*(\w+)", tail)
    if m:
        return m.group(1)
    # 3. void test_xxx(void) 或 void Test_Xxx(void) (Unity 两种命名)
    m = re.search(r"(?:void|int|static\s+void)\s+([tT]est_\w+)\s*\(", tail)
    if m:
        return m.group(1)
    return ""


def _match_api(func_norm: str, api_map: dict[str, str], module: str = "") -> str | None:
    """函数名 → API → SWS ID (大小写不敏感 + 组合名分词 + 模块限定)。"""
    key = func_norm.lower().strip()
    if not key:
        return None  # 空函数名不匹配
    # 模块限定: 仅匹配该模块的 API (避免跨模块误配, 如 init → Xcp)
    mod_apis = {api: sws for api, sws in api_map.items()
                if not module or api.lower().startswith(module.lower())}
    if not mod_apis and module:
        return None  # 模块无 API 定义, 不匹配
    if key in mod_apis:
        return mod_apis[key]

    # API 去模块前缀表 (adc_init → init)
    api_suffix: dict[str, str] = {}
    for api, sws in mod_apis.items():
        suffix = api.split("_", 1)[1] if "_" in api else api
        api_suffix[suffix] = sws

    # 1) 直接后缀匹配 (init_deinit → init)
    if key in api_suffix:
        return api_suffix[key]

    # 2) 组合名分词匹配: test_init_deinit → {init, deinit} 找交集
    parts = set(key.split("_"))
    for part in parts:
        if part in api_suffix:
            return api_suffix[part]

    # 2b) 词→后缀子串匹配: notification → enablegroupnotification
    for part in parts:
        for suffix, sws in api_suffix.items():
            if len(part) >= 4 and (part in suffix or suffix in part):
                return sws

    # 3) 前缀/包含匹配 (取最长的匹配, 模块内)
    best = None
    for api, sws in mod_apis.items():
        if api.startswith(key) or key.startswith(api) or api in key:
            if best is None or len(api) > len(best[0]):
                best = (api, sws)
    return best[1] if best else None


def build_all_sws_ids() -> set[str]:
    """提取全部 design 文档中的 SWS ID (含裸模块名 SWS_Adc 和编号 SWS_Adc_00001)。"""
    ids: set[str] = set()
    for doc in sorted(DESIGN_DIR.rglob("*-design.md")):
        text = doc.read_text(encoding="utf-8", errors="replace")
        ids.update(re.findall(r"SWS_\w+_\d+", text))
        ids.update(re.findall(r"SWS_[A-Za-z]+", text))
    return ids


def align_file(path: Path, api_map: dict[str, str], all_sws: set[str]) -> tuple[int, list[str], list[str]]:
    """对齐单个文件。返回 (替换数, 无法匹配的悬空ID, 替换明细)。"""
    text = path.read_text(encoding="utf-8", errors="replace")
    if "@req" not in text:
        return 0, [], []

    real_ids = {v.lower() for v in api_map.values()} | {s.lower() for s in all_sws}
    replaced = 0
    unmatched: list[str] = []
    details: list[str] = []

    def _replace(m: re.Match) -> str:
        nonlocal replaced
        rid = m.group(1)
        if rid.lower() in real_ids:
            return m.group(0)  # 真实 ID 跳过
        func = find_test_func(text, m.end())
        if not func:
            unmatched.append(rid)
            return m.group(0)
        # @coverage/TEST_CASE 返回 API 名或函数名; 仅 test_/Test_ 前缀需剥离
        func_norm = func[5:].lower() if func.lower().startswith("test_") else func.lower()
        # 从悬空 ID 提取模块 (SWS_Adc_00201 → Adc; MCU_CLOCK_001 → MCU)
        mod_m = re.match(r"(?:SWS_|SHALL_)?([A-Za-z]+)_", rid)
        module = mod_m.group(1) if mod_m else ""
        sws = _match_api(func_norm, api_map, module)
        if sws:
            replaced += 1
            details.append(f"    {rid} → {sws} ({func})")
            return f"@req {sws}"
        unmatched.append(rid)
        return m.group(0)

    new_text = re.sub(r"@req\s+([A-Za-z0-9_\-\.]+)", _replace, text)
    if new_text != text:
        if APPLY_MODE:
            path.write_text(new_text, encoding="utf-8")
        else:
            # dry-run: 不写盘, 但统计替换 (用临时判断)
            pass
    return replaced, unmatched, details


def main() -> int:
    ap = argparse.ArgumentParser(description="yuleASR @req ID 对齐器 v2")
    ap.add_argument("--apply", action="store_true", help="实际替换 (默认 dry-run)")
    args = ap.parse_args()
    global APPLY_MODE
    APPLY_MODE = args.apply

    api_map = build_api_map()
    all_sws = build_all_sws_ids()
    print(f"design 文档 API→SWS 映射: {len(api_map)} 个 API | 全部 SWS ID: {len(all_sws)}")

    total_replaced = 0
    all_unmatched: list[str] = []
    files_touched = 0

    for tf in sorted(TESTS_DIR.rglob("*.c")):
        if "@req" not in tf.read_text(encoding="utf-8", errors="replace"):
            continue
        n, unmatched, details = align_file(tf, api_map, all_sws)
        if n:
            files_touched += 1
            total_replaced += n
            print(f"  {tf.relative_to(ROOT)}: {n} 替换")
            if args.apply:
                for d in details[:4]:
                    print(d)
        all_unmatched.extend(unmatched)

    print(f"\n总计: {files_touched} 文件, {total_replaced} 个悬空 ID 替换")
    print(f"无法匹配: {len(set(all_unmatched))} 个唯一 ID")
    for u in sorted(set(all_unmatched))[:15]:
        print(f"  - {u}")

    if not args.apply:
        print("\n(dry-run — 加 --apply 实际写入)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
