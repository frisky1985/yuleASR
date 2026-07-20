# Phase 0 — MISRA 扫描范围修复 验证报告

## 诊断发现

### 问题根源
`.misra_config` 的 `[include]` 段与 yuleOSH 框架的 `_detect_include_paths()` 存在同步问题：
1. `_detect_include_paths()` 硬编码了约 75 个 include 目录（在 `review.py` 中）
2. `.misra_config` 的 `[include]` 段硬编码了约 82 个 include 目录
3. 实际磁盘上有 **116 个** `**/include/` 目录
4. 两套配置都不完整，且互不同步

### 差异清单
`_detect_include_paths()` 缺失的 41 个 include 目录：

```
src/asw/communication_manager/include
src/asw/diagnostic_manager/include
src/asw/engine_control/include
src/asw/io_control/include
src/asw/mode_manager/include
src/asw/storage_manager/include
src/asw/vehicle_dynamics/include
src/asw/watchdog_manager/include
src/bsw/boot/include
src/bsw/ecual/canNm/include
src/bsw/mcal/fee/include
src/bsw/services/cansm/include
src/bsw/services/comM/include
src/bsw/services/dlt/include
src/bsw/services/docan/include
src/bsw/services/doip/include
src/bsw/services/ecuC/include
src/bsw/services/ethsm/include
src/bsw/services/fim/include
src/bsw/services/ipdum/include
src/bsw/services/j1939nm/include
src/bsw/services/j1939tp/include
src/bsw/services/linm/include
src/bsw/services/linsm/include
src/bsw/services/lntm/include
src/bsw/services/mqtt/include
src/bsw/services/nm/include
src/bsw/services/ramsafety/include
src/bsw/services/schm/include
src/bsw/services/swc/include
src/bsw/services/udpNm/include
src/micro-dds/include
src/platform/s32k312/include
tests/crypto_benchmark/include
third_party/crypto/aes_modes/include
third_party/crypto/blake2/include
third_party/crypto/hash/include
third_party/crypto/mbedtls/include
third_party/yule-mbedtls-adapter/include
```

## 修复方案

### 统一为单一路径源
重写 `_detect_include_paths()` 为**动态文件系统扫描**，消除所有硬编码路径：

1. **`review.py` — `_detect_include_paths()`**：不再硬编码路径，改为调用 `_scan_include_dirs()` 递归扫描 `src/`, `include/`, `tests/`, `third_party/` 目录
2. **新增 `_scan_include_dirs()`**：遍历目录树，收集所有 `**/include/` 目录及包含 `.h` 文件的模块级 `**/src` 目录
3. **`.misra_config`**：`[include]` 段改为注释说明由 yuleOSH 动态发现，不再硬编码
4. **`run_misra_check.sh`**：改为通过 `python3` 调用 `_detect_include_paths()` 获取动态 `-I` 参数

### 修改文件
| 文件 | 修改内容 |
|------|---------|
| `~/.openclaw/workspace/tasks/yuleOSH/src/yuleosh/ci/stages/review.py` | `_detect_include_paths()` 重写为动态扫描 |
| `.misra_config` | `[include]` 段改为自动发现注释 |
| `tools/run_misra_check.sh` | 改为使用动态 include 路径 |

## 验证结果

### 运行 cppcheck 扫描
```
总计 .c 文件（src/ 下）: 300
Include 路径数: 120（原来是 92/75）
扫描耗时: 67.1s
```

### 覆盖率
| 指标 | 修复前 | 修复后 | 预期 |
|------|--------|--------|------|
| .c 文件扫描数 | 300 | 300 | ≥80% |
| 有违规的 .c 文件 | 3 | 143 | ≥58 |
| 有违规的文件（含 .h） | 4 | 490 | ≥80 |
| MISRA 违规总数 | 258 | 9,980 | 非零 ✓ |
| Include 路径数 | 75/92 | 120 | 完整 ✓ |

### 验证清单
- [x] 120 个 include 目录全被使用（磁盘上共 116 个 `**/include/` 目录）
- [x] 300/300 = 100% 的 src/ .c 文件被扫描
- [x] 总 .c 文件数: 609（含 tests/ 和 third_party/，这些在 ci-config 中配置了 exclude）
- [x] MISRA 违规数: 9,980（非零 ✓）
- [x] `affected_files`（.c 文件）: 143 ✓
- [x] 修完后未退化（从 3 个文件增加到 143 个）

### CI 验证
- [x] yuleosh CI 运行 Layer 1（含 MISRA 检查）正常完成
- [x] MISRA 违规数非零（9,980 个违规）
- [x] 扫描超时问题已解决（67s vs 之前超时）

### 质量保证
- [x] 每次改完已测试验证
- [x] 改动前已备份原始文件（`review.py.bak`, `run_misra_check.sh.bak`）
- [x] 未出现"修完后比修前更差"的情况
- [x] `.misra_config` 更新为自动发现模式
