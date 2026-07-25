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

# ── Generate lcov report + JSON ──
generate_reports() {
    echo ""
    echo "=== Generating Coverage Reports ==="
    
    cd "$PROJECT_DIR"
    
    # Find the coverage info file
    FINAL_INFO=""
    for candidate in coverage_src.info coverage_filtered.info coverage.info; do
        if [ -f "$PROJECT_DIR/$candidate" ]; then
            FINAL_INFO="$PROJECT_DIR/$candidate"
            break
        fi
        if [ -f "$BUILD_DIR/$candidate" ]; then
            FINAL_INFO="$BUILD_DIR/$candidate"
            break
        fi
    done
    
    # If no existing info file, run lcov capture manually
    if [ -z "$FINAL_INFO" ]; then
        echo "  -> No existing .info file — running lcov capture..."
        # Find directories with .gcda files
        GCDA_DIRS=$(find "$PROJECT_DIR" -name "*.gcda" -exec dirname {} \; | sort -u 2>/dev/null || true)
        if [ -n "$GCDA_DIRS" ]; then
            lcov --capture --directory "$PROJECT_DIR" --output-file "$PROJECT_DIR/coverage_raw.info" \
                 --rc lcov_branch_coverage=1 2>/dev/null || true
            if [ -f "$PROJECT_DIR/coverage_raw.info" ]; then
                lcov --remove "$PROJECT_DIR/coverage_raw.info" \
                     '/usr/*' '*/third_party/*' '*/tests/*' '*/build*' \
                     --output-file "$PROJECT_DIR/coverage_filtered.info" 2>/dev/null || true
                lcov --extract "$PROJECT_DIR/coverage_filtered.info" "$PROJECT_DIR/src/*" \
                     --output-file "$PROJECT_DIR/coverage_src.info" 2>/dev/null || true
                FINAL_INFO="$PROJECT_DIR/coverage_src.info"
                if [ ! -f "$FINAL_INFO" ]; then
                    FINAL_INFO="$PROJECT_DIR/coverage_filtered.info"
                fi
            fi
        else
            echo "  ⚠️  No .gcda files found — coverage data may be empty"
        fi
    fi
    
    if [ -z "$FINAL_INFO" ] || [ ! -f "$FINAL_INFO" ]; then
        echo "  ❌ No coverage data generated"
        return 1
    fi
    
    echo "  -> Using: $FINAL_INFO"
    
    # Generate HTML report
    mkdir -p "$PROJECT_DIR/coverage_report"
    if command -v genhtml &>/dev/null; then
        genhtml "$FINAL_INFO" \
            --output-directory "$PROJECT_DIR/coverage_report" \
            --title "yuleASR C Coverage" \
            --legend \
            --rc lcov_branch_coverage=1 \
            2>&1 | tail -5
        echo "  -> HTML report: coverage_report/index.html"
    else
        echo "  ⚠️  genhtml not available (macOS) — skipping HTML report"
        echo "     Install: brew install lcov"
    fi
    
    # Print summary
    echo ""
    echo "=== Coverage Summary ==="
    if command -v lcov &>/dev/null; then
        lcov --summary "$FINAL_INFO" 2>&1 || true
    fi
    
    # Generate CI JSON report
    echo ""
    echo "=== Generating CI JSON Report ==="
    mkdir -p "$PROJECT_DIR/.yuleosh/reports"
    LIBC_COV="$PROJECT_DIR/tools/generate_c_coverage_json.py"
    if [ -f "$LIBC_COV" ]; then
        python3 "$LIBC_COV" "$FINAL_INFO" "$PROJECT_DIR/.yuleosh/reports/c-coverage.json"
    else
        echo "  ⚠️  generate_c_coverage_json.py not found — generating inline..."
        python3 -c "
import json, re, os

info_file = '$FINAL_INFO'
output_file = '$PROJECT_DIR/.yuleosh/reports/c-coverage.json'

files = []
totals = {'lines': {'found': 0, 'hit': 0}, 'functions': {'found': 0, 'hit': 0}, 'branches': {'found': 0, 'hit': 0}}
current = None

with open(info_file) as f:
    for line in f:
        line = line.strip()
        m_sf = re.match(r'^SF:(.+)\$', line)
        if m_sf:
            if current:
                files.append(current)
            current = {
                'file': m_sf.group(1),
                'lines': {'found': 0, 'hit': 0},
                'functions': {'found': 0, 'hit': 0},
                'branches': {'found': 0, 'hit': 0}
            }
            continue
        if current is None:
            continue
        m_da = re.match(r'^DA:(\d+),(\d+)\$', line)
        if m_da:
            count = int(m_da.group(2))
            current['lines']['found'] += 1
            if count > 0:
                current['lines']['hit'] += 1
            continue
        m_fnf = re.match(r'^FNF:(\d+)\$', line)
        if m_fnf:
            current['functions']['found'] = int(m_fnf.group(1))
            continue
        m_fnh = re.match(r'^FNH:(\d+)\$', line)
        if m_fnh:
            current['functions']['hit'] = int(m_fnh.group(1))
            continue
        m_brf = re.match(r'^BRF:(\d+)\$', line)
        if m_brf:
            current['branches']['found'] = int(m_brf.group(1))
            continue
        m_brh = re.match(r'^BRH:(\d+)\$', line)
        if m_brh:
            current['branches']['hit'] = int(m_brh.group(1))
            continue
        if line == 'end_of_record' and current:
            for k in ['lines', 'functions', 'branches']:
                totals[k]['found'] += current[k]['found']
                totals[k]['hit'] += current[k]['hit']
            files.append(current)
            current = None

total_lines_found = totals['lines']['found']
total_lines_hit = totals['lines']['hit']
line_rate = round(total_lines_hit / total_lines_found * 100, 2) if total_lines_found > 0 else 0.0

total_br_found = totals['branches']['found']
total_br_hit = totals['branches']['hit']
branch_rate = round(total_br_hit / total_br_found * 100, 2) if total_br_found > 0 else 0.0

report = {
    'success': True,
    'line_rate': line_rate,
    'branch_rate': branch_rate,
    'total_files': len(files),
    'totals': {
        'lines': {'found': total_lines_found, 'hit': total_lines_hit},
        'functions': {'found': totals['functions']['found'], 'hit': totals['functions']['hit']},
        'branches': {'found': total_br_found, 'hit': total_br_hit}
    },
    'files': [
        {
            'file': f['file'],
            'line_rate': round(f['lines']['hit'] / max(f['lines']['found'], 1) * 100, 2),
            'branch_rate': round(f['branches']['hit'] / max(f['branches']['found'], 1) * 100, 2),
            'lines': f['lines'],
            'functions': f['functions']
        }
        for f in files
    ]
}

with open(output_file, 'w') as f:
    json.dump(report, f, indent=2)

# Print summary
print(f'    Report: {output_file}')
print(f'    Line rate: {line_rate}%')
print(f'    Branch rate: {branch_rate}%')
print(f'    Files: {len(files)}')
print(f'    Lines: {total_lines_hit}/{total_lines_found}')
print(f'    Functions: {totals[\"functions\"][\"hit\"]}/{totals[\"functions\"][\"found\"]}')
print(f'    Branches: {totals[\"branches\"][\"hit\"]}/{totals[\"branches\"][\"found\"]}')
" 2>&1
    fi
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

MODE="${1:-batch}"

case "$MODE" in
    cmake)
        clean_gcda
        do_cmake_build || do_batch_build
        generate_reports
        ;;
    batch)
        clean_gcda
        do_batch_build
        generate_reports
        ;;
    baseline)
        # Generate fresh coverage then save baseline
        clean_gcda
        do_batch_build
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
