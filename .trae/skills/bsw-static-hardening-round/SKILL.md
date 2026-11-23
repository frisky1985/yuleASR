---
name: bsw-static-hardening-round
description: yuleASR BSW static-hardening round: pick a clang-tidy family, full scan, freeze triage, fix real defects only, verify, commit to feature branch. Invoke when user asks for the next hardening round.
---

# BSW 静态硬化循环（yuleASR 专用）

**仅适用于本工作区**（`/workspace`，GitHub `frisky1985/yuleASR`，C99 AUTOSAR Classic BSW，约 400 个 `.c`）。
目标：每轮选一个**尚未覆盖的 clang-tidy 规则族** → 全量扫描 → **先冻结分诊口径再改** → 只修真实缺陷 → 复扫 + 全量构建 + 串行 CTest → 提交并推送 feature 分支。**不自动合并 master。**

> 核心纪律（Experience 100006543）：**分诊口径必须在动代码之前写死**，否则同一会话内结论会反复、补丁互相推翻。纯样式一律只分诊不改，避免无意义 churn。

---

## 0. 开工前置检查（务必按序）

```bash
cd /workspace
git rev-parse --abbrev-ref HEAD
git rev-parse HEAD
git status --short          # 必须干净
git log --oneline -1 master
# 关键：确认基线 == origin/master，避免从过期分支/版本起扫
git fetch origin master --quiet
git rev-parse master origin/master
```

- 若当前分支基线 ≠ `origin/master`，先 `git checkout master && git pull --ff-only`，再 `git checkout -b feat/roundNN-hardening`。
- **历史踩坑**：曾从 `3119dc96` 起分支而 master 已到 `1dc3e917`，漏掉前几轮修复。**每次都必须核对 base。**
- `/tmp` 在上下文切换/沙箱重启后可能被清空 —— 构建目录和扫描文件都要能一键重建（见下）。

## 1. 选择规则族 + 写 checks

把本轮规则集写入 `/tmp/checks.txt`。**格式：启用目标族，再逐个用 `-` 关掉高噪音/纯样式子规则。**

本轮（readability + performance）示例：

```
readability-*,performance-*,-readability-magic-numbers,-readability-identifier-naming,-readability-non-const-parameter,-readability-function-size,-readability-function-cognitive-complexity,-readability-isolate-declaration,-readability-redundant-member-init,-readability-signed-size,-readability-else-after-return,-performance-type-promotion-in-math-fn
```

挑选原则：
- 一轮聚焦一个 family（如 `bugprone-*`、`cert-*`、`cppcoreguidelines-*`、`readability-*`+`performance-*`），避免一次铺太宽。
- **先看历史矩阵**（若存在 `.qoder/skills/autosar-benchmark-compare/references/` 或往轮提交记录），跳过已覆盖族。

## 2. 构建 + 生成全量编译单元列表

```bash
BUILD=/tmp/yule-static            # 固定目录名，便于重建
cmake -S /workspace -B "$BUILD" \
  -DBUILD_TESTING=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build "$BUILD" -j 6       # 先确保能编译，否则 tidy 会大量 parse error

# 从 compile_commands.json 抽真实编译单元（去重、排序）
python3 - <<'PY' > /tmp/static_files.txt
import json
d = json.load(open('/tmp/yule-static/compile_commands.json'))
print('\n'.join(sorted({e['file'] for e in d})))
PY
wc -l /tmp/static_files.txt       # 参考：约 356 个
```

## 3. 全量扫描

```bash
checks=$(cat /tmp/checks.txt)
xargs -a /tmp/static_files.txt -n 4 -P 6 \
  clang-tidy -p /tmp/yule-static --checks="$checks" \
  > /tmp/tidy.out 2>&1
echo "scan_exit=$?"               # 常为 123：xargs 因个别 TU parse error 判失败，输出仍完整，属预期
```

统计分布，先看清结构再决定改什么：

```bash
grep -oE '\[(readability|performance|bugprone|cert|cppcoreguidelines)[a-z0-9.-]*\]' /tmp/tidy.out \
  | sort | uniq -c | sort -rn | head -40
```

## 4. 冻结分诊口径（先写下来，再动代码）

对每个规则族逐条给出**三选一**结论，并在回复里明示：

| 判定 | 处理 |
|:-----|:-----|
| **真实缺陷**（行为/安全/性能/耦合正确性） | 修，且尽量最小差异 |
| **样式噪音**（命名长度、字面量后缀、大括号、魔法数等） | **只分诊不改**，记录条数 |
| **误报** | 读代码证实后明确标注"不改"，给出证据行号 |

本工作区已确认的**默认噪音/不动清单**（除非用户单独要求）：
`readability-identifier-length`、`readability-uppercase-literal-suffix`、`performance-no-int-to-ptr`（多为有意的 MMIO 强转）、`readability-redundant-casting`、`readability-redundant-declaration`（配置重声明）、`EnumCastOutOfRange`（哨兵值）、`DeprecatedOrUnsafeBufferHandling`（安全用法）、`readability-braces-around-statements`、`*Padding`（改字段序＝ABI 变更，禁止）、`readability-avoid-nested-conditional-operator`、`format-nonliteral`、`readability-named-parameter`。

**误报纪律**：遇到 `readability-suspicious-call-argument` 之类，先 `Read` 被点名的函数与调用点，确认真实参数顺序再决定；曾证实 `dds_auth_sha256(hash_input, hash_len, signature)` 为误报（`hash_input` 确为 data 缓冲，`signature` 确为输出缓冲），不动。

## 5. 只修真实缺陷（常见正确修法）

1. **`performance-unnecessary-value-param`（C++ `std::function`）**
   - 参数只被 `(void)` 化 → 改 `const T&`，且**头声明与 .cpp 定义同时改**（否则反向触发 `inconsistent-declaration-parameter-name`）。
   - 参数赋值给成员 → 保持按值传参，赋值处补 `std::move(...)`，并确保 `#include <utility>`。
2. **`readability-inconsistent-declaration-parameter-name`（C/C++）**
   - 对齐时优先保留 **AUTOSAR 公共 API 名**（通常头文件/spec 名），改定义的参数名及其函数体内引用。
   - 若定义已是正确名且文件内文档也用该名 → 改头文件 + 头内 `@param` 注释。
   - 纯改名，不改签名类型/顺序，**零 ABI 影响**；`third_party/*` 一律跳过。
3. 其余族按最小差异修复；**绝不顺手重构**，不改结构体字段顺序，不动公开符号名。

修每一处都要能说出"为什么这是真实缺陷"，否则归入"仅分诊"。

## 6. 复扫 + 全量验证

```bash
# 6.1 仅复扫改动文件，确认目标告警清零且无新增
checks=$(cat /tmp/checks.txt)
clang-tidy -p /tmp/yule-static --checks="$checks" $(git diff --name-only -- '*.c' '*.cpp' '*.h') \
  > /tmp/rescan.out 2>&1
grep -E 'unnecessary-value-param|inconsistent-declaration-parameter-name|suspicious-call-argument' /tmp/rescan.out || echo "ACTIONABLE=0"

# 6.2 全量构建
cmake --build /tmp/yule-static -j 6 > /tmp/build.log 2>&1; echo "build_exit=$?"
grep -iE 'error:' /tmp/build.log | head || true

# 6.3 CTest 必须串行（-j 1），基线约 216
ctest --test-dir /tmp/yule-static -j 1 --output-on-failure | tail -20
```

**`dds_log` 已知 flaky**（全量串行时偶发 SEGFAULT，隔离必过）：
```bash
ctest --test-dir /tmp/yule-static -R '^dds_log$' --output-on-failure   # 隔离验证
ctest --test-dir /tmp/yule-static -j 1 -E '^dds_log$'                  # 其余全绿
# 二者合计 216/216 即视为通过；在汇报中如实说明 dds_log 偶发、与改动无关
```

## 7. 提交 + 推送（feature 分支，不合并 master）

分支命名：`feat/roundNN-hardening`（**禁止版本号命名分支**）。

```bash
cd /workspace && git add -u && git status --short
git -c user.name='frisky1985' -c user.email='frisky1985@users.noreply.github.com' \
  commit -m "$(cat <<'EOF'
fix(static-hardening): 第NN轮 <规则族> 硬化

- <规则> : <改了哪些文件/为什么是真实缺陷>
- <规则> : ...
- 跳过 third_party/*（第三方代码不改）
- 确认 <N> 处 <规则> 为误报（<证据>），不修改
- 其余 style 噪音仅分诊不改，避免无意义 churn

验证: 全量构建通过；改动文件复扫 actionable 清零；CTest 216（dds_log 已知偶发 flaky，隔离通过）
EOF
)"
git push -u origin feat/roundNN-hardening
git ls-remote origin refs/heads/feat/roundNN-hardening   # 远端 hash 必须 == git rev-parse HEAD
```

- **绝不自动合并 master**：仅当用户明确说"合并到 master"才执行。
- 汇报用结构化中文：**规则族 / 扫描规模 / 真实缺陷与修复 / 红绿验证 / 已分诊未修改 / Git**。

## 常见坑速查

| 坑 | 解决 |
|:---|:-----|
| 基线错误，漏前轮修复 | 开扫前 `git rev-parse master origin/master` 对齐 |
| `/tmp` 被清，构建/文件列表消失 | 按 §2 重建，固定 `BUILD=/tmp/yule-static` |
| `scan_exit=123` | xargs 因个别 TU parse error 判失败，属预期，输出完整 |
| 改 const-ref 只改了 .cpp | 头声明必须同步，否则触发反向 parameter-name 告警 |
| 误把样式当缺陷大改 | 先冻结口径；纯样式只分诊 |
| `dds_log` 全量 SEGFAULT | 隔离单跑必过，其余 `-E` 排除后全绿等效 216/216 |
| 想顺手合并 master | 不做；等用户明确指令 |
