#!/usr/bin/env python3
"""Run golden file tests for yuleASR-Configurator."""

import sys
from pathlib import Path

# Add package to path
sys.path.insert(0, str(Path(__file__).parent / "tools" / "yuleasr-configurator"))

from yuleasr.core.code_generator import CodeGenerator
from yuleasr.core.golden_test import GoldenFileTester

REPO = Path(__file__).parent
CONFIG = REPO / "config" / "bsw_config.json"
TEMPLATES = REPO / "tools" / "yuleasr-configurator" / "templates"
GENERATED = Path("/tmp/yuleasr-golden-test")
GOLDEN = REPO / "config" / "generated"


def main():
    print("=" * 70)
    print("Golden File Test Suite")
    print("=" * 70)

    # Step 1: Generate output
    print("\n[1/3] Generating configuration files...")
    gen = CodeGenerator(CONFIG, TEMPLATES, GENERATED)
    report = gen.run(validate=False)

    if not report.ok:
        print(f"✗ Generation failed with {len(report.errors)} errors")
        for err in report.errors:
            print(f"  {err}")
        return 1

    print(f"✓ Generated {len(report.written)} files")

    # Step 2: Run golden file tests
    print("\n[2/3] Running golden file tests...")
    tester = GoldenFileTester(GOLDEN, GENERATED)
    matches, mismatches = tester.run_all()

    # Step 3: Print report
    print("\n[3/3] Test report:")
    tester.print_report(matches, mismatches, show_diff=True)

    # Summary
    total = len(matches) + len(mismatches)
    match_pct = (len(matches) / total * 100) if total > 0 else 0

    print(f"\nMatch rate: {match_pct:.1f}% ({len(matches)}/{total})")

    if mismatches:
        print(f"\n✗ {len(mismatches)} files differ from golden versions")
        print("\nTo update golden files (if changes are intentional):")
        print(f"  python3 tools/yuleasr-configurator/yuleasr/core/golden_test.py --update")
        return 1
    else:
        print("\n✓ All files match golden versions!")
        return 0


if __name__ == "__main__":
    sys.exit(main())
