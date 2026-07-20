#!/bin/bash
"""
Coverage Rebuild Script (P0-4)

Expands coverage measurement from Crc.c+Det.c (2 files, 114 lines) 
to the entire src/ tree.

Usage:
  bash tools/rebuild_coverage.sh

Requirements: gcc, gcov, lcov, gcovr
"""

set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_DIR"

echo "=== yuleASR Coverage Rebuild (P0-4) ==="
echo ""

BUILD_DIR="build-coverage"

# Step 1: Configure cmake with coverage enabled
echo "1. Configuring cmake with coverage flags..."
cmake -B "$BUILD_DIR" -S . \
    -DBUILD_TESTING=ON \
    -DENABLE_COVERAGE=ON \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_C_FLAGS="--coverage -g -O0 -fprofile-arcs -ftest-coverage" \
    -DCMAKE_EXE_LINKER_FLAGS="--coverage" \
    -DCMAKE_SHARED_LINKER_FLAGS="--coverage" 2>&1 | tail -5

# Step 2: Build everything (compiles all src/ with --coverage)
echo ""
echo "2. Building all targets with --coverage..."
cmake --build "$BUILD_DIR" -j$(nproc) 2>&1 | tail -20

# Step 3: Build E2E tests with coverage
echo ""
echo "3. Building E2E test binaries with --coverage..."
python3 -c "
import subprocess, sys
import glob
from pathlib import Path

e2e_dir = Path('tests/e2e')
build_dir = e2e_dir / '.build'
build_dir.mkdir(exist_ok=True)
project_dir = Path('.')

# Compile all test_e2e_*.c with --coverage
sources = sorted(e2e_dir.glob('test_e2e_*.c'))
for src in sources:
    out = build_dir / src.stem
    cmd = ['gcc', '--coverage', '-g', '-O0', '-o', str(out), str(src)]
    # Add all module source files for linking
    for cfile in sorted(project_dir.glob('src/bsw/**/*.c')):
        cmd.append(str(cfile))
    for cfile in sorted(project_dir.glob('src/rte/**/*.c')):
        cmd.append(str(cfile))
    for cfile in sorted(project_dir.glob('src/platform/**/*.c')):
        cmd.append(str(cfile))
    for incdir in [
        'include/autosar',
        'src',
        'src/bsw/os/include',
        'src/bsw/services/det/include',
        'src/bsw/services/crc/include',
        'src/bsw/services/com/include',
        'src/bsw/services/dcm/include',
        'src/bsw/services/dem/include',
        'src/rte/include',
    ]:
        cmd.extend(['-I', str(project_dir / incdir)])
    print(f'  Compiling {src.name}...')
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
    if r.returncode != 0:
        print(f'  ⚠️  Compile failed for {src.name} (non-fatal):')
        for line in r.stderr.split(chr(10))[:5]:
            print(f'    {line}')
    else:
        print(f'  ✅ Compiled {src.name}')
" 2>&1

# Step 4: Run the E2E tests (exercises Crc, Det, and other linked code)
echo ""
echo "4. Running E2E tests..."
cd tests/e2e/.build
for test_bin in test_e2e_*; do
    if [ -x "$test_bin" ] && [ ! -d "$test_bin" ]; then
        echo "  Running $test_bin..."
        ./"$test_bin" 2>&1 || echo "  ⚠️  $test_bin failed"
    fi
done
cd "$PROJECT_DIR"

# Step 5: Run gcov on all gcda files
echo ""
echo "5. Running gcov to collect coverage data..."
for gcda_file in $(find "$BUILD_DIR" tests/e2e/.build -name '*.gcda' 2>/dev/null); do
    dir=$(dirname "$gcda_file")
    echo "  Processing $(basename "$gcda_file")"
    (cd "$dir" && gcov -b -l -p $(basename "$gcda_file") 2>/dev/null) || true
done

# Step 6: Generate comprehensive coverage report using lcov
echo ""
echo "6. Generating lcov coverage report..."
# Find all .gcda files
GCDA_FILES=$(find "$BUILD_DIR" tests/e2e/.build -name '*.gcda' 2>/dev/null | head -200)
if [ -z "$GCDA_FILES" ]; then
    echo "  ⚠️  No .gcda files found"
else
    # Use lcov to capture coverage
    lcov --capture --directory "$BUILD_DIR" \
         --directory tests/e2e/.build \
         --output-file coverage-full.info 2>&1 | tail -5

    # Filter to only src/ files
    lcov --extract coverage-full.info "$(pwd)/src/*" \
         --output-file coverage-filtered.info 2>&1 | tail -5

    # Generate HTML report
    genhtml coverage-filtered.info --output-directory coverage-report-html 2>&1 | tail -5
fi

# Step 7: Generate gcovr JSON report
echo ""
echo "7. Generating gcovr JSON report..."
gcovr --root . \
    --filter "src/.*" \
    --exclude "tests/.*" \
    --exclude "third_party/.*" \
    --json --output .yuleosh/reports/c-coverage.json 2>&1 || echo "  ⚠️  gcovr JSON failed"

gcovr --root . \
    --filter "src/.*" \
    --exclude "tests/.*" \
    --exclude "third_party/.*" 2>&1 | tee .yuleosh/reports/c-coverage-summary.txt || true

# Step 8: Apply threshold check with fix (no 0.0 bypass)
echo ""
echo "8. Coverage threshold check..."
python3 -c "
import json
try:
    with open('.yuleosh/reports/c-coverage.json') as f:
        report = json.load(f)
except (FileNotFoundError, json.JSONDecodeError):
    print('❌ No coverage report generated')
    exit(1)

line_rate = report.get('line_rate', 0)
if isinstance(line_rate, float):
    line_rate_pct = line_rate * 100
else:
    line_rate_pct = float(line_rate)

branch_rate = report.get('branch_rate', 0)
if isinstance(branch_rate, float):
    branch_rate_pct = branch_rate * 100
else:
    branch_rate_pct = float(branch_rate)

total_files = len(report.get('files', []))
total_lines = report.get('totals', {}).get('lines', {}).get('found', 0)
total_hit = report.get('totals', {}).get('lines', {}).get('hit', 0)

print(f'  Files measured: {total_files}')
print(f'  Lines found: {total_lines}')
print(f'  Lines hit: {total_hit}')
print(f'  Line rate: {line_rate_pct:.2f}%')
print(f'  Branch rate: {branch_rate_pct:.2f}%')

# Fix: ensure 0.0 line rate doesn't bypass threshold
THRESHOLD = 60.0
if total_lines == 0:
    print(f'  ❌ 0 lines measured - coverage collection failed')
    print(f'  ❌ Gate FAILED: no coverage data')
    exit(1)
if line_rate_pct < THRESHOLD:
    print(f'  ❌ Line rate {line_rate_pct:.1f}% < threshold {THRESHOLD}%')
    print(f'  ❌ Gate FAILED')
    exit(1)
print(f'  ✅ Line rate {line_rate_pct:.1f}% >= threshold {THRESHOLD}%')
print(f'  ✅ Gate PASSED')
print(f'  ✅ Coverage baseline established: {total_files} files, {total_lines} lines')
"
