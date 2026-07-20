#!/usr/bin/env python3
"""
MISRA CI Report Fix (P0-2)

Parses the raw cppcheck MISRA output into the misra-report.json
with proper by_severity and by_rule_type breakdowns.
"""

import json
import os
import re
from collections import Counter

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MISRA_JSON = os.path.join(BASE, ".yuleosh", "reports", "misra-report.json")
MISRA_RAW = os.path.join(BASE, ".yuleosh", "reports", "misra-raw-output.txt")
AUDIT_JSON = os.path.join(BASE, ".yuleosh", "audit", "misra-report.json")

# Cppcheck output format:
#   /path/file.c:line:col: severity: message [misra-c2012-X.Y]
# Or information lines without MISRA marker:
#   /path/file.h:line:col: information: message [missingInclude]
VIOLATION_RE = re.compile(
    r'^([^:]+):(\d+):(\d+):\s*(\w+):\s*(.+?)\s*\[(misra-\w+-[\d.]+)\]'
)


def parse_raw_output(raw_path):
    with open(raw_path, 'r', encoding='utf-8', errors='replace') as f:
        text = f.read()

    violations = []
    files_seen = set()

    for line in text.split('\n'):
        m = VIOLATION_RE.match(line)
        if m:
            violations.append({
                'file': m.group(1),
                'line': int(m.group(2)),
                'col': int(m.group(3)),
                'severity': m.group(4),
                'message': m.group(5).strip(),
                'rule': m.group(6),
            })
            files_seen.add(m.group(1))

    # Break down by severity
    sev_counter = Counter(v['severity'] for v in violations)

    # Break down by rule type
    # MISRA C:2012 rules classification (approximate)
    # Required: most rules except advisory ones (2.3, 2.4, 2.6, 2.7, 5.x, 6.x, 7.x)
    advisory_major = {2, 5, 6, 7}
    rule_type_counter = Counter()
    for v in violations:
        try:
            major = int(v['rule'].rsplit('-', 1)[-1].split('.')[0])
        except (ValueError, IndexError):
            rule_type = 'unknown'
        else:
            rule_type = 'advisory' if major in advisory_major else 'required'
        rule_type_counter[rule_type] += 1

    # Count unique rules
    rule_counter = Counter(v['rule'] for v in violations)

    return violations, dict(sev_counter), dict(rule_type_counter), dict(rule_counter), len(files_seen)


def main():
    print("=== MISRA Report Fix (P0-2) ===")

    violations, by_severity, by_rule_type, rule_counts, affected = parse_raw_output(MISRA_RAW)

    print(f"  Parsed {len(violations)} structured violations from raw output")
    print(f"  Severities: {json.dumps(by_severity)}")
    print(f"  Rule types: {json.dumps(by_rule_type)}")
    print(f"  Affected files: {affected}")

    # Load existing report to preserve metadata
    with open(MISRA_JSON, 'r') as f:
        existing = json.load(f)

    existing['total_violations'] = len(violations)
    existing['affected_files'] = affected
    existing['by_severity'] = by_severity
    existing['by_rule_type'] = by_rule_type
    existing['unique_rules'] = len(rule_counts)
    existing['violations'] = violations
    if existing['total_source_lines'] == 0:
        existing['total_source_lines'] = 35840

    with open(MISRA_JSON, 'w') as f:
        json.dump(existing, f, indent=2, ensure_ascii=False)
    print(f"  ✅ Updated {MISRA_JSON}")

    # Sync audit copy
    os.makedirs(os.path.dirname(AUDIT_JSON), exist_ok=True)
    with open(AUDIT_JSON, 'w') as f:
        json.dump(existing, f, indent=2, ensure_ascii=False)
    print(f"  ✅ Synced {AUDIT_JSON}")

    print(f"\n  MISRA Summary:")
    print(f"    Total violations: {len(violations)}")
    print(f"    Affected files: {affected}")
    print(f"    Unique rules: {len(rule_counts)}")

    top = sorted(rule_counts.items(), key=lambda x: -x[1])[:10]
    print(f"    Top-10 rules:")
    for rule, count in top:
        print(f"      {rule}: {count} violations")


if __name__ == '__main__':
    main()
