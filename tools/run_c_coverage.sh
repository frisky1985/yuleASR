#!/bin/bash
# run_c_coverage.sh — yuleASR C 代码覆盖率运行脚本
#
# 用途: 统一入口，编译所有 C 目标并收集覆盖率数据。
#       支持 CMake (ENABLE_COVERAGE) 和 batch 两种模式。
#
# 用法:
#   bash tools/run_c_coverage.sh              # 默认: batch 模式
#   bash tools/run_c_coverage.sh cmake         # CMake 模式
#   bash tools/run_c_coverage.sh batch         # Batch 模式 (batch10_coverage.sh)
#   bash tools/run_c_coverage.sh summary       # 只看上次结果摘要
#   bash tools/run_c_coverage.sh baseline      # 生成覆盖率基线
#
# 环境变量:
#   BUILD_DIR      构建目录 (默认: build-cov)
#   PROJECT_DIR    项目根目录 (默认: 脚本所在目录的上级)
#   THRESHOLD      门限百分比 (默认: 70)
#
# 输出:
#   .yuleosh/reports/c-coverage.json   CI 可读的覆盖率报告
#   .yuleosh/reports/c-coverage-baseline.json  覆盖率基线
#   coverage_src.info       lcov 覆盖率数据 (仅 src/)
#   coverage_filtered.info  lcov 覆盖率数据 (过滤后)
#   coverage_report/        HTML 覆盖率报告 (genhtml)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="${PROJECT_DIR:-"$(cd "$SCRIPT_DIR/.." && pwd)"}"
BUILD_DIR="${BUILD_DIR:-"$PROJECT_DIR/build-cov"}"
THRESHOLD="${THRESHOLD:-70}"

cd "$PROJECT_DIR"

echo "=============================================="
echo "  yuleASR C Coverage Runner"
echo "=============================================="
echo "  Project:  $PROJECT_DIR"
echo "  Build:    $BUILD_DIR"
echo "  Mode:     ${1:-batch}"
echo "=============================================="
echo ""

clean_gcda() {
    find "$PROJECT_DIR" -name "*.gcda" -delete 2>/dev/null || true
    echo "  -> Cleaned stale .gcda files"
}

# ── CMake build with coverage flags ──
do_cmake_build() {
    echo "=== [CMake Mode] Building with ENABLE_COVERAGE ==="
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    cmake "$PROJECT_DIR" \
        -DENABLE_COVERAGE=ON \
        -DCMAKE_BUILD_TYPE=Debug \
        -DBUILD_TESTING=ON \
        2>&1 | tail -5
    
    echo ""
    echo "  -> Compiling (make -k to continue on error)..."
    make -k -j4 2>&1 | tail -15
    
    GCNO_COUNT=$(find "$BUILD_DIR" -name "*.gcno" 2>/dev/null | wc -l)
    echo "  -> .gcno files produced: $GCNO_COUNT"
    
    # List all test executables
    TEST_BINS=$(find "$BUILD_DIR/tests" -type f -executable -name "*_test*" -o -name "*_UnitTest*" 2>/dev/null)
    if [ -n "$TEST_BINS" ]; then
        echo "  -> Test binaries found: $(echo "$TEST_BINS" | wc -l)"
    fi
    
    echo ""
    echo ""
    echo "  -> Running available test binaries to generate .gcda..."
    ctest --output-on-failure 2>&1 | tail -10 || true
    
    # Also run any standalone test binaries that ctest missed
    for test_bin in $(find "$BUILD_DIR/tests" "$BUILD_DIR/bin" -type f -executable 2>/dev/null); do
        if file "$test_bin" | grep -q "Mach-O\|ELF"; then
            echo "    Running: $test_bin"
            "$test_bin" 2>/dev/null || true
        fi
    done 2>/dev/null || true
    
    GCDA_COUNT=$(find "$BUILD_DIR" -name "*.gcda" 2>/dev/null | wc -l)
    echo "  -> .gcda files produced: $GCDA_COUNT"
    echo "  -> CMake build completed (with -k, errors ignored for coverage infrastructure)"
    
    cd "$PROJECT_DIR"
    return 0
}

# ── Batch mode (uses batch10_coverage.sh) ──
do_batch_build() {
    echo "=== [Batch Mode] Running batch10_coverage.sh ==="
    if [ -f "$PROJECT_DIR/batch10_coverage.sh" ]; then
        bash "$PROJECT_DIR/batch10_coverage.sh" 2>&1 | tail -30
        echo ""
        echo "  -> batch10_coverage.sh completed"
    else
        echo "  ⚠️  batch10_coverage.sh not found — nothing to run"
        return 1
    fi
}

# ── Generate gcovr report + trusted JSON ──
generate_reports() {
    echo ""
    echo "=== Generating Coverage Reports (gcovr) ==="

    cd "$PROJECT_DIR"
    mkdir -p "$BUILD_DIR/coverage"

    if ! command -v gcovr &>/dev/null; then
        echo "  ❌ gcovr not installed — install with: pip install gcovr"
        return 1
    fi

    # Capture coverage from the build dir, filter to src/, exclude tests/third_party/build.
    # --gcov-ignore-errors all tolerates cross-compiled .gcda without native gcov data.
    gcovr --root "$PROJECT_DIR" \
          --object-directory "$BUILD_DIR" \
          --filter "src/.*" \
          --exclude "tests/.*" --exclude "third_party/.*" --exclude "build-cov/.*" \
          --gcov-ignore-parse-errors \
          --gcov-ignore-errors all \
          --json \
          --output "$BUILD_DIR/coverage/coverage.json" \
          2>&1 | tail -5

    GCOVR_JSON="$BUILD_DIR/coverage/coverage.json"
    if [ ! -f "$GCOVR_JSON" ]; then
        echo "  ❌ gcovr did not produce a JSON report"
        return 1
    fi

    # HTML report (optional)
    if command -v genhtml &>/dev/null || true; then
        gcovr --root "$PROJECT_DIR" \
              --object-directory "$BUILD_DIR" \
              --filter "src/.*" --exclude "tests/.*" --exclude "third_party/.*" \
              --gcov-ignore-parse-errors --gcov-ignore-errors all \
              --html --html-details \
              --output "$PROJECT_DIR/coverage_report/index.html" \
              2>/dev/null && echo "  -> HTML report: coverage_report/index.html" || true
    fi

    # ── WP2: produce the trusted, reproducible JSON (relative paths + provenance) ──
    mkdir -p "$PROJECT_DIR/.yuleosh/reports"
    python3 "$PROJECT_DIR/tools/generate_c_coverage_json.py" \
            "$GCOVR_JSON" \
            "$PROJECT_DIR/.yuleosh/reports/c-coverage.json" \
            --root "$PROJECT_DIR"

    echo ""
    echo "=== Coverage Summary ==="
    python3 -c "
import json
d = json.load(open('$PROJECT_DIR/.yuleosh/reports/c-coverage.json'))
p = d.get('provenance', {})
print(f\"  Line rate:   {d['line_rate']}%  ({d['totals']['lines']['hit']}/{d['totals']['lines']['found']})\")
print(f\"  Branch rate: {d['branch_rate']}%\")
print(f\"  Files:       {d['total_files']}\")
print(f\"  git_sha:     {p.get('git_sha','?')[:8]}  dirty={p.get('git_dirty')}\")
"
}

# ── Save coverage baseline ──
save_baseline() {
    echo ""
    echo "=== Saving Coverage Baseline ==="
    
    JSON="$PROJECT_DIR/.yuleosh/reports/c-coverage.json"
    BASELINE="$PROJECT_DIR/.yuleosh/reports/c-coverage-baseline.json"
    
    if [ ! -f "$JSON" ]; then
        echo "  ❌ No c-coverage.json found — run coverage first"
        return 1
    fi
    
    TIMESTAMP=$(date -u +"%Y-%m-%dT%H:%M:%SZ" 2>/dev/null || date -u +"%FT%TZ")
    COMMIT=$(git rev-parse HEAD 2>/dev/null || echo "unknown")
    
    python3 -c "
import json
with open('$JSON') as f:
    data = json.load(f)
baseline = {
    'baseline': True,
    'timestamp': '$TIMESTAMP',
    'commit': '$COMMIT',
    'threshold': $THRESHOLD,
    'coverage': data
}
with open('$BASELINE', 'w') as f:
    json.dump(baseline, f, indent=2)
print(f'    Baseline saved: $BASELINE')
print(f'    Commit: $COMMIT')
print(f'    Line rate: {data.get(\"line_rate\", \"?\")}%')
print(f'    Threshold: ${THRESHOLD}%')
print(f'    Status: {\"✅ MEETS THRESHOLD\" if data.get(\"line_rate\", 0) >= ${THRESHOLD} else \"❌ BELOW THRESHOLD\"}')
"
}

# ── Show last result summary ──
show_summary() {
    JSON="$PROJECT_DIR/.yuleosh/reports/c-coverage.json"
    if [ ! -f "$JSON" ]; then
        echo "No coverage data available at $JSON"
        exit 1
    fi
    
    echo "=== Last Coverage Summary ==="
    python3 -c "
import json
with open('$JSON') as f:
    d = json.load(f)
print(f'  Line rate:   {d.get(\"line_rate\", 0)}%')
print(f'  Branch rate: {d.get(\"branch_rate\", 0)}%')
print(f'  Files:       {d.get(\"total_files\", 0)}')
print(f'')
tot = d.get('totals', {})
lines = tot.get('lines', {})
print(f'  Lines:      {lines.get(\"hit\",0)}/{lines.get(\"found\",0)}')
funcs = tot.get('functions', {})
print(f'  Functions:  {funcs.get(\"hit\",0)}/{funcs.get(\"found\",0)}')
br = tot.get('branches', {})
print(f'  Branches:   {br.get(\"hit\",0)}/{br.get(\"found\",0)}')
print(f'')
files = d.get('files', [])
if files:
    bottom5 = sorted(files, key=lambda x: x.get('line_rate', 100))[:5]
    print(f'  Bottom-covered files:')
    for f in bottom5:
        fp = f.get('file', '?')
        short = fp.split('/')[-1] if '/' in fp else fp
        print(f'    {short}: {f.get(\"line_rate\", 0)}%')
"
    exit 0
}

# ════════════════════════════════════════════════════
# Main
# ════════════════════════════════════════════════════

MODE="${1:-cmake}"

case "$MODE" in
    cmake)
        clean_gcda
        do_cmake_build
        generate_reports
        ;;
    batch)
        clean_gcda
        do_batch_build || { echo "batch mode unavailable, falling back to cmake"; do_cmake_build; }
        generate_reports
        ;;
    baseline)
        # Generate fresh coverage then save baseline
        clean_gcda
        do_cmake_build
        generate_reports
        save_baseline
        ;;
    summary)
        show_summary
        ;;
    *)
        echo "Usage: $0 {cmake|batch|baseline|summary}"
        echo ""
        echo "  cmake     CMake build with ENABLE_COVERAGE + run tests"
        echo "  batch     batch10_coverage.sh (default, proven for Apple Clang)"
        echo "  baseline  Generate coverage then save baseline"
        echo "  summary   Show last coverage summary"
        exit 1
        ;;
esac

echo ""
echo "=============================================="
echo "  Done"
echo "=============================================="
