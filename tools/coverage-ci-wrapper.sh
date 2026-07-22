#!/bin/bash
# coverage-ci-wrapper.sh — CI Coverage Wrapper for yuleASR
#
# This script is called by the yuleOSH CI pipeline's C coverage stage.
# It runs batch10_coverage.sh to generate fresh coverage data, then
# produces the c-coverage.json in the format expected by the CI gate.
#
# Usage: bash tools/coverage-ci-wrapper.sh [project_dir]
#   project_dir defaults to $PWD

set -euo pipefail

PROJECT_DIR="${1:-$PWD}"
cd "$PROJECT_DIR"

echo "=== Coverage CI Wrapper ==="
echo "Project: $PROJECT_DIR"

# Step 1: Remove stale .gcda files so batch10_coverage.sh runs as fallback
find . -name "*.gcda" -delete 2>/dev/null
echo " -> Cleaned stale .gcda files"

# Step 2: Run batch10_coverage.sh
echo " -> Running batch10_coverage.sh..."
bash batch10_coverage.sh 2>&1 | tail -15

# Step 3: Check for generated coverage info files
FINAL_INFO=""
if [ -f coverage_src.info ]; then
    FINAL_INFO="coverage_src.info"
elif [ -f coverage_filtered.info ]; then
    FINAL_INFO="coverage_filtered.info"
elif [ -f coverage.info ]; then
    FINAL_INFO="coverage.info"
fi

if [ -z "$FINAL_INFO" ]; then
    echo "  ❌ No coverage info file generated!"
    exit 1
fi

echo " -> Using coverage info: $FINAL_INFO"

# Step 4: Generate c-coverage.json from the info file using lcov summary
mkdir -p .yuleosh/reports

# Extract line coverage rate
LINE_RATE=$(lcov --summary "$FINAL_INFO" 2>&1 | grep "lines" | awk '{print $2}' | sed 's/%//')
BRANCH_RATE=$(lcov --summary "$FINAL_INFO" 2>&1 | grep "branches" | awk '{print $2}' | sed 's/%//' || echo "0.0")
TOTAL_LINES=$(lcov --summary "$FINAL_INFO" 2>&1 | grep "lines" | awk '{print $4}' | sed 's/ of /:/')
COVERED_LINES=$(echo "$TOTAL_LINES" | cut -d: -f1)
FOUND_LINES=$(echo "$TOTAL_LINES" | cut -d: -f2)

# Fallbacks
LINE_RATE="${LINE_RATE:-0.0}"
BRANCH_RATE="${BRANCH_RATE:-0.0}"
COVERED_LINES="${COVERED_LINES:-0}"
FOUND_LINES="${FOUND_LINES:-0}"

echo " -> Line rate: ${LINE_RATE}%"
echo " -> Branch rate: ${BRANCH_RATE}%"

# Step 5: Parse per-file coverage from the .info file
python3 -c "
import json, re, os

info_file = '$FINAL_INFO'
output_file = '.yuleosh/reports/c-coverage.json'

files = []
totals = {'lines': {'found': 0, 'hit': 0}, 'functions': {'found': 0, 'hit': 0}, 'branches': {'found': 0, 'hit': 0}}
current = None
src_prefix = os.path.abspath('$PROJECT_DIR') + '/'

with open(info_file) as f:
    for line in f:
        line = line.strip()
        m_sf = re.match(r'^SF:(.+)\$', line)
        if m_sf:
            if current:
                files.append(current)
            current = {'file': m_sf.group(1), 'lines': {'found': 0, 'hit': 0}, 'functions': {'found': 0, 'hit': 0}, 'branches': {'found': 0, 'hit': 0}}
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
    'files': [{'file': f['file'], 'line_rate': round(f['lines']['hit']/max(f['lines']['found'],1)*100,2),
               'branch_rate': round(f['branches']['hit']/max(f['branches']['found'],1)*100,2),
               'lines': f['lines'], 'functions': f['functions']} for f in files]
}

with open(output_file, 'w') as f:
    json.dump(report, f, indent=2)

print(f'    Report: {output_file}')
print(f'    Line rate: {line_rate}%')
print(f'    Branch rate: {branch_rate}%')
print(f'    Files: {len(files)}')
" 2>&1

echo "=== Coverage CI Wrapper Done ==="
