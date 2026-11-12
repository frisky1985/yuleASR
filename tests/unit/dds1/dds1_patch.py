#!/usr/bin/env python3
"""dds1_patch.py — 为 dds1 测试生成产品源补丁副本。

修复 4 个产品缺陷:
  1-4. content_filtered_topic.c / time_filter.c / ownership.c / persistence.c
       静态池 static xxx_handle_t s_xxx_pool[] 在 struct 定义之前声明,
       导致 C "不完整类型" 编译错误。补丁: 把声明移到 struct 之后。
  5. persistence.c:753 字段名笔误 last_standby_standby → last_standby_sync
"""
import sys, os, re, shutil

PATCH_RULES = [
    # (源文件 basename, 池声明, struct 结尾锚定, 可选的额外替换)
    {
        "file": "content_filtered_topic.c",
        "pool": "static cft_handle_t s_cft_handle_pool[CFT_MAX_PARALLEL_FILTERS];",
        "anchor_tail": "    uint32_t eval_count;\n};",
    },
    {
        "file": "time_filter.c",
        "pool": "static tbf_handle_t s_tbf_pool[TBF_MAX_COMPRESSION_SLOTS];",
        "anchor_tail": "    tbf_sample_node_t *active_head;\n};",
    },
    {
        "file": "ownership.c",
        "pool": "static own_handle_t s_own_pool[OWN_MAX_OWNERS];",
        "anchor_tail": "    dds_guid_t pending_transfer_target;\n};",
    },
    {
        "file": "persistence.c",
        "pool": "static per_handle_t s_per_handle_pool[PER_MAX_HANDLES];",
        "anchor_tail": "    uint64_t last_standby_sync;\n};",
        "extra": [("per->last_standby_standby", "per->last_standby_sync")],
    },
]

def patch_file(src_path, rules, outdir):
    basename = os.path.basename(src_path)
    rule = next((r for r in rules if r["file"] == basename), None)
    if rule is None:
        # 不是需要补丁的文件, 直接复制
        dst = os.path.join(outdir, basename)
        shutil.copy2(src_path, dst)
        print(f"  [copy] {basename}")
        return

    with open(src_path) as f:
        src = f.read()

    # 1. 移除原位置的池声明 (换成注释)
    src = src.replace(rule["pool"], f"/* pool decl moved by dds1 patch */")

    # 2. 在 struct 结尾锚定后重新插入池声明
    src = src.replace(rule["anchor_tail"], rule["anchor_tail"] + "\n" + rule["pool"])

    # 3. 额外替换
    for old, new in rule.get("extra", []):
        src = src.replace(old, new)

    dst = os.path.join(outdir, basename)
    with open(dst, "w") as f:
        f.write(src)
    print(f"  [patch] {basename}")

def main():
    args = sys.argv[1:]
    outdir_flag = "--outdir"
    if outdir_flag not in args:
        print("usage: dds1_patch.py <src1> <src2> ... --outdir <dir>")
        sys.exit(1)
    idx = args.index(outdir_flag)
    outdir = args[idx + 1]
    srcs = args[:idx]
    os.makedirs(outdir, exist_ok=True)
    print(f"dds1_patch.py: writing patches to {outdir}")
    for s in srcs:
        patch_file(s, PATCH_RULES, outdir)

if __name__ == "__main__":
    main()
