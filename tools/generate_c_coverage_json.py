#!/usr/bin/env python3
"""
Generate .yuleosh/reports/c-coverage.json from an lcov .info file.
Used by batch10_coverage.sh and the CI pipeline.

Usage:
    python3 tools/generate_c_coverage_json.py <lcov_info_file> [output_path]
"""
import json
import re
import sys


def parse_lcov_info(info_file: str) -> dict:
    """Parse an lcov .info file and produce a coverage report dict."""
    files = []
    totals = {
        'lines': {'found': 0, 'hit': 0},
        'functions': {'found': 0, 'hit': 0},
        'branches': {'found': 0, 'hit': 0},
    }
    current = None

    def _finalize():
        nonlocal current
        if current is None:
            return
        for k in ['lines', 'functions', 'branches']:
            totals[k]['found'] += current[k]['found']
            totals[k]['hit'] += current[k]['hit']
        files.append(current)
        current = None

    with open(info_file) as f:
        for line in f:
            stripped = line.strip()

            m_sf = re.match(r'^SF:(.+)$', stripped)
            if m_sf:
                _finalize()
                current = {
                    'file': m_sf.group(1),
                    'lines': {'found': 0, 'hit': 0},
                    'functions': {'found': 0, 'hit': 0},
                    'branches': {'found': 0, 'hit': 0},
                }
                continue

            if current is None:
                continue

            m_da = re.match(r'^DA:(\d+),(\d+)$', stripped)
            if m_da:
                count = int(m_da.group(2))
                current['lines']['found'] += 1
                if count > 0:
                    current['lines']['hit'] += 1
                continue

            m_fnf = re.match(r'^FNF:(\d+)$', stripped)
            if m_fnf:
                current['functions']['found'] = int(m_fnf.group(1))
                continue

            m_fnh = re.match(r'^FNH:(\d+)$', stripped)
            if m_fnh:
                current['functions']['hit'] = int(m_fnh.group(1))
                continue

            m_brf = re.match(r'^BRF:(\d+)$', stripped)
            if m_brf:
                current['branches']['found'] = int(m_brf.group(1))
                continue

            m_brh = re.match(r'^BRH:(\d+)$', stripped)
            if m_brh:
                current['branches']['hit'] = int(m_brh.group(1))
                continue

            if stripped == 'end_of_record':
                _finalize()

    # Finalize any remaining file
    _finalize()

    total_lines_found = totals['lines']['found']
    total_lines_hit = totals['lines']['hit']
    line_rate = round(total_lines_hit / max(total_lines_found, 1) * 100, 2)

    total_br_found = totals['branches']['found']
    total_br_hit = totals['branches']['hit']
    branch_rate = round(total_br_hit / max(total_br_found, 1) * 100, 2)

    report = {
        'success': True,
        'line_rate': line_rate,
        'branch_rate': branch_rate,
        'total_files': len(files),
        'totals': totals,
        'files': [
            {
                'file': f['file'],
                'line_rate': round(f['lines']['hit'] / max(f['lines']['found'], 1) * 100, 2),
                'branch_rate': round(f['branches']['hit'] / max(f['branches']['found'], 1) * 100, 2),
                'lines': f['lines'],
                'functions': f['functions'],
            }
            for f in files
        ],
    }
    return report


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python3 tools/generate_c_coverage_json.py <lcov_info_file> [output_path]")
        sys.exit(1)

    info_file = sys.argv[1]
    output_path = sys.argv[2] if len(sys.argv) > 2 else '.yuleosh/reports/c-coverage.json'

    report = parse_lcov_info(info_file)
    with open(output_path, 'w') as f:
        json.dump(report, f, indent=2)

    s = report['totals']['lines']
    print(f"Coverage report saved: {output_path}")
    print(f"  Lines: {s['hit']}/{s['found']} ({report['line_rate']}%)")
    print(f"  Files: {report['total_files']}")
